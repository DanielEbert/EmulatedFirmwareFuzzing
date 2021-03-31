#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "sim_avr_types.h"
#include "sim_avr.h"

typedef uint32_t avr_flashaddr_t;
typedef struct avr_t avr_t;

typedef struct Edge {
	avr_flashaddr_t from, to;
} Edge;

void initialize_coverage(avr_t *avr);
void edge_triggered(avr_t *avr, avr_flashaddr_t from, avr_flashaddr_t to);
int edge_compare(const void *key1, const void *key2);


#ifdef __cplusplus
};
#endif
