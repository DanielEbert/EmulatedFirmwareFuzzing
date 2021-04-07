#include "fuzz_coverage.h"
#include "fuzz_crash_handler.h"
#include "sim_avr.h"
#include "sim_avr_types.h"
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>

/******************************

Sends messages of the following structure:
| Header | Body |
Header has a fixed size of 5 bytes. The first byte specifies the ID of the
message. The latter 4 bytes specify the length in bytes of the Body.

ID 1: Coverage Information in form of an Edge struct.
ID 2: Information about a crash that includes the crashing input

******************************/

// Client must run on localhost:8123
void initialize_server_notify(avr_t *avr) {
  Server_Connection *server_connection = malloc(sizeof(Server_Connection));
  server_connection->connection_established = 0;
  server_connection->s = -1;
  avr->server_connection = server_connection;

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
}

int send_crash(Server_Connection *server_connection, Crash *crash) {
  // Body consists of: | crash_id | crashing_addr | crashing_input |
  // There is no need to send the crashing_input length here, since that is
  // always body_size - 1 - 4 bytes (crashing_addr has a size of 4 bytes)
  char msg_ID = 2;
  uint32_t body_size = 1 + 4 + crash->crashing_input->buf_len;
  if (send_header(server_connection, msg_ID, body_size) < 0 ||
      send_raw(server_connection, &(crash->crash_id), 1) < 0 ||
      send_raw(server_connection, &(crash->crashing_addr), 4) < 0 ||
      send_raw(server_connection, crash->crashing_input->buf,
               crash->crashing_input->buf_len) < 0) {
    return -1;
  }
  return 0;
}

int send_coverage(Server_Connection *server_connection, Edge *edge) {
  return 0; // TODOE remove me
  char msg_ID = 1;
  uint32_t body_size = sizeof(edge->from) + sizeof(edge->to);
  if (send_header(server_connection, msg_ID, body_size) < 0 ||
      send_raw(server_connection, &edge->from, sizeof(edge->from)) < 0 ||
      send_raw(server_connection, &edge->to, sizeof(edge->to)) < 0) {
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