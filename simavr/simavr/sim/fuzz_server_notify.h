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
  int s;
  int connection_established;
} Server_Connection;

void initialize_server_notify(avr_t *avr, char *filename);
void send_target_info(Server_Connection *server_connection, char *filename);
int send_path_to_target_executable(Server_Connection *server_connection,
                                   char *filename);
int send_crash(Server_Connection *server_connection, Crash *crash);
int send_coverage(Server_Connection *server_connection, Edge *edge);
int send_header(Server_Connection *server_connection, char msg_ID, uint32_t body_size);
int send_raw(Server_Connection *server_connection, void *buf, uint32_t buf_len);

#ifdef __cplusplus
};
#endif

