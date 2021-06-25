#pragma once

#include "sim_avr.h"
#include "sim_avr_types.h"
#include "sim_uthash.h"
#include "sim_utlist.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Patch {
  void *patch_pointer;
  void *arg;
  struct Patch *next, *prev;
} Patch;

typedef struct patched_instruction {
  avr_flashaddr_t vaddr; /* we'll use this field as the key */
  struct Patch *patches;
  UT_hash_handle hh; /* makes this structure hashable */
} patched_instruction;

// Global Hash Table
// Key: virtual address
// Value: list of function pointers
struct patched_instruction *patched_instructions;

void initialize_patch_instructions(struct avr_t *);
void setup_patches(avr_t *avr) __attribute__((weak));
int patch_instruction(avr_flashaddr_t vaddr, void *patch_pointer, void *arg);
patched_instruction *get_or_create_patched_instruction(avr_flashaddr_t key);
Patch *create_function_patch(void *function, void *arg);
void check_run_patch(avr_t *avr);
void reset_patch_side_effects(avr_t *avr);
void test_patch_function(void *arg);
void fuzz_reset(void *arg);
void print_current_input(void *arg);
void override_args(void *arg);
void test_raise_interrupt(void *arg);
void noop(avr_t *avr);
uint32_t get_symbol_address(char *symbol_name, avr_t *avr);
void write_to_flashaddr(avr_flashaddr_t dst, void *src, size_t num_bytes,
                        avr_t *avr);
void set_shadow_map(avr_flashaddr_t start, size_t size, uint8_t value,
                    avr_t *avr);

#ifdef __cplusplus
};
#endif
