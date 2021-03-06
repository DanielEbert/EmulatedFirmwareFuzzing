#include "fuzz_coverage.h"
#include "fuzz_crash_handler.h"
#include "sim_avr.h"
#include "sim_avr_types.h"
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/******************************

Sends messages of the following structure:
| Header | Body |
Header has a fixed size of 5 bytes. The first byte specifies the ID of the
message. The latter 4 bytes specify the length in bytes of the Body.

ID 0: Path to the executable file that is emulated.
ID 1: Coverage Information in form of an Edge struct and the input that
increased the coverage.
ID 2: Information about a crash that includes the
crashing input. ID 3: Fuzzer Statistics, e.g. the
avr->fuzzer_stats.inputs_executed value.

******************************/

// Server must listen on localhost:8123
void initialize_server_notify(avr_t *avr, int do_connect, char *filename) {
  if (filename == NULL) {
    fprintf(stderr, "Path to executable argument missing.\nExiting.\n");
    exit(1);
  }

  Server_Connection *server_connection = malloc(sizeof(Server_Connection));
  if (server_connection == NULL) {
    perror("malloc failed: ");
  }
  server_connection->connection_established = 0;
  server_connection->s = -1;
  avr->server_connection = server_connection;

  if (do_connect != 0) {
    printf("No connection initiated due to run_once\n");
    return;
  }

  struct sockaddr_in server;
  server_connection->s = socket(AF_INET, SOCK_STREAM, 0);
  if (server_connection->s == -1) {
    server_connection->connection_established = 0;
    printf("Could not create socket\n");
    return;
  }
  server.sin_addr.s_addr = inet_addr("127.0.0.1");
  server.sin_family = AF_INET;
  server.sin_port = htons(8123);

  if (connect(server_connection->s, (struct sockaddr *)&server,
              sizeof(server)) < 0) {
    server_connection->connection_established = 0;
    printf("Could not connect to 127.0.0.1:8123\n");
    return;
  }

  printf("Connected to 127.0.0.1:8123\n");
  server_connection->connection_established = 1;

  send_target_info(server_connection, filename);
}

void send_target_info(Server_Connection *server_connection, char *filename) {
  printf("filename: %s\n", filename);
  // Check if filename is a relative path
  if (filename[0] != '/') {
    // Prepend current working directory to filename
    char path_to_file[1024];
    if (getcwd(path_to_file, 1023) == NULL) {
      perror("getcwd Error ");
      exit(1);
    }
    // getcwd string is null terminated
    size_t path_size = strlen(path_to_file);
    path_to_file[path_size] = '/';
    path_size++;
    strncpy(path_to_file + path_size, filename, 1023 - path_size);
    send_path_to_target_executable(server_connection, path_to_file);
  } else {
    send_path_to_target_executable(server_connection, filename);
  }
}

int send_path_to_target_executable(Server_Connection *server_connection,
                                   char *filename) {
  char msg_ID = 0;
  size_t filename_size = strlen(filename);
  uint32_t body_size = filename_size;
  if (send_header(server_connection, msg_ID, body_size) < 0 ||
      send_raw(server_connection, filename, filename_size) < 0) {
    return -1;
  }
  return 0;
}

int send_crash(avr_t *avr, Crash *crash) {
  // Body consists of: | crash_id | crashing_addr | origin_addr |
  // | crashing_input_length | crashing_input | stack_frame_size |
  // stack_frame_size * stack_frame_pc |
  Server_Connection *server_connection = avr->server_connection;
  char msg_ID = 2;
  uint32_t crashing_input_length = crash->crashing_input->buf_len;
  // + 1 because we also send current PC
  uint32_t stack_frame_index = avr->trace_data->stack_frame_index + 1;
  uint32_t body_size = 1 + 4 + 4 + 4 + crash->crashing_input->buf_len + 4 +
                       stack_frame_index * 4;
  if (send_header(server_connection, msg_ID, body_size) < 0 ||
      send_raw(server_connection, &(crash->crash_id), 1) < 0 ||
      send_raw(server_connection, &(crash->crashing_addr), 4) < 0 ||
      send_raw(server_connection, &(crash->origin_addr), 4) < 0 ||
      send_raw(server_connection, &crashing_input_length, 4) < 0 ||
      send_raw(server_connection, crash->crashing_input->buf,
               crash->crashing_input->buf_len) < 0 ||
      send_raw(server_connection, &stack_frame_index, 4) < 0) {
    return -1;
  }
  // Send program counters of stackframes
  // -1 because we send current PC after loop
  for (int i = 0; i < stack_frame_index - 1; i++) {
    if (send_raw(server_connection, &(avr->trace_data->stack_frame[i].pc), 4) <
        0) {
      return -1;
    }
  }
  // Send current PC
  if (send_raw(server_connection, &(crash->crashing_addr), 4) < 0) {
    return -1;
  }
  return 0;
}

int send_coverage(avr_t *avr, Edge *edge) {
  // Body consists of: | from edge pc | to edge pc| input_length | input |
  Server_Connection *server_connection = avr->server_connection;
  char msg_ID = 1;
  uint32_t input_length = avr->fuzzer->current_input->buf_len;
  uint32_t body_size = sizeof(edge->from) + sizeof(edge->to) + 4 + input_length;
  if (send_header(server_connection, msg_ID, body_size) < 0 ||
      send_raw(server_connection, &edge->from, sizeof(edge->from)) < 0 ||
      send_raw(server_connection, &edge->to, sizeof(edge->to)) < 0 ||
      send_raw(server_connection, &input_length, 4) < 0 ||
      send_raw(server_connection, avr->fuzzer->current_input->buf,
               input_length) < 0) {
    return -1;
  }
  return 0;
}

int send_fuzzer_stats(avr_t *avr) {
  Server_Connection *server_connection = avr->server_connection;
  char msg_ID = 3;
  uint32_t body_size = 4 * 3;
  if (send_header(server_connection, msg_ID, body_size) < 0 ||
      send_raw(server_connection, &avr->fuzzer_stats.inputs_executed, 4) < 0 ||
      send_raw(server_connection, &avr->fuzzer_stats.total_crashes, 4) < 0 ||
      send_raw(server_connection, &avr->fuzzer_stats.max_depth, 4) < 0) {
    return -1;
  }
  return 0;
}

int send_header(Server_Connection *server_connection, char msg_ID,
                uint32_t body_size) {
  if (send_raw(server_connection, &msg_ID, 1) < 0 ||
      send_raw(server_connection, &body_size, 4) < 0) {
    return -1;
  }
  return 0;
}

int send_raw(Server_Connection *server_connection, void *buf,
             uint32_t buf_len) {
  if (server_connection->connection_established == 0) {
    return -1;
  }

  ssize_t n;
  const char *p = buf;

  while (buf_len > 0) {
    n = send(server_connection->s, p, buf_len, MSG_NOSIGNAL);
    if (n <= 0) {
      if (errno == EPIPE) {
        printf(
            "Lost the connection to the server. Continuing the emulation.\n");
        server_connection->connection_established = 0;
      }
      return -2;
    }
    p += n;
    buf_len -= n;
  }

  return 0;
}