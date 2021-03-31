#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "sim_avr_types.h"
#include "sim_avr.h"
#include "fuzz_coverage.h"

typedef struct avr_t avr_t;
typedef struct Edge Edge;

typedef struct Server_Connection {
  int s;
  int connection_established;
} Server_Connection;

void initialize_server_notify(avr_t *avr);
int send_coverage(Server_Connection *server_connection, Edge *edge);
int send_header(Server_Connection *server_connection, char msg_ID, uint32_t body_size);
int send_raw(Server_Connection *server_connection, void *buf, uint32_t buf_len);


#ifdef __cplusplus
};
#endif
