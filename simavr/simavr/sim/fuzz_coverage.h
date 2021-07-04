#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "sim_avr_types.h"
#include "sim_avr.h"

typedef uint32_t avr_flashaddr_t;
typedef struct avr_t avr_t;

// Represents an edge in the control flow graph of the emulated program.
// 'from' and 'to' represent addresses.
// 'from' is the address of a branch instruction
// 'to' is the destination address of this branch instruction.
typedef struct Edge {
	avr_flashaddr_t from, to;
} Edge;

// Sets up the coverage hash set that contains the Edge structs that
// the fuzzer found.
void initialize_coverage(avr_t *avr);

// Check if the <from, to> pair is in the coverage hash set. If yes,
// add to hash set and mark input as interesting.
void edge_triggered(avr_t *avr, avr_flashaddr_t from, avr_flashaddr_t to);

// Check if two edges are equal.
// key1 and key2 must point to Edge structs.
// Returns 0 if both edges are equal (i.e. struct members match). 
int edge_compare(const void *key1, const void *key2);


#ifdef __cplusplus
};
#endif
