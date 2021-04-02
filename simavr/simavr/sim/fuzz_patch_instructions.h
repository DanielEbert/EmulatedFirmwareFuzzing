#pragma once

#include "sim_avr.h"
#include "sim_avr_types.h"
#include "sim_uthash.h"
#include "sim_utlist.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct function_patch {
  void *function_pointer;
  void *arg;
  struct function_patch *next, *prev;
} function_patch;

typedef struct patched_instruction {
  avr_flashaddr_t vaddr; /* we'll use this field as the key */
  struct function_patch *function_patches;
  UT_hash_handle hh; /* makes this structure hashable */
} patched_instruction;

// Global Hash Table
// Key: virtual address
// Value: list of function pointers
struct patched_instruction *patched_instructions;

void initialize_patch_instructions(struct avr_t *);
int patch_instruction(avr_flashaddr_t vaddr, void *function_pointer, void *arg);
patched_instruction *get_or_create_patched_instruction(avr_flashaddr_t key);
function_patch *create_function_patch(void *function, void *arg);
int check_run_patch(avr_t *avr);
void reset_patch_side_effects(avr_t *avr);
int test_patch_function(void *arg);
int test_reset(void *arg);

#ifdef __cplusplus
};
#endif
