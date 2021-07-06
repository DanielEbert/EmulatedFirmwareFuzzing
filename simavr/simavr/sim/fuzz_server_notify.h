#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "sim_avr_types.h"
#include "sim_avr.h"
#include "fuzz_coverage.h"
#include "fuzz_crash_handler.h"

typedef struct avr_t avr_t;
typedef struct Edge Edge;
typedef struct Crash Crash;

typedef struct Server_Connection {
  int s; // Stores a file descriptor for a socket. 
  int connection_established; // 1 if the connection is currently established.
} Server_Connection;

// Try and setup a socket connection to the UI server. The UI server must listen 
// on localhost:8123. It is OK if no connection could be set up. This is 
// typically the case if the user did not start the UI server.
void initialize_server_notify(avr_t *avr, int do_connect, char *filename);

// Get the path to the executable file that the emulator emulates and pass
// this path to the send_path_to_target_executable function.
void send_target_info(Server_Connection *server_connection, char *filename);

// Send the path to the executable file that the emulator emulates.
// Sending means data is sent tot the UI server via a socket. The file
// descriptor of this socket is stored in the 's' member of the
// Server_Connection struct.
int send_path_to_target_executable(Server_Connection *server_connection,
                                   char *filename);

// Send a message of the 'New Crash' type.
int send_crash(avr_t *avr, Crash *crash);

// Send a message of the 'New Coverage' type.
int send_coverage(avr_t *avr, Edge *edge);

// Send a message of the 'Updated Fuzzer Statistics' type.
int send_fuzzer_stats(avr_t *avr);

// Header consists of | msg_ID | body_size | and is followed by one or multiple 
// calls to the send_raw function that then sends the message body.
int send_header(Server_Connection *server_connection, char msg_ID, uint32_t body_size);

// Send 'buf_len' bytes starting at the buffer at address 'buf' via the 
// socket to the UI Server.
int send_raw(Server_Connection *server_connection, void *buf, uint32_t buf_len);

#ifdef __cplusplus
};
#endif

