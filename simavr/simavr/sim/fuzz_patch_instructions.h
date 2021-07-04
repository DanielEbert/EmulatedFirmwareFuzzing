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
  avr_flashaddr_t vaddr; // we use this field as the key
  struct Patch *patches;
  UT_hash_handle hh; // Only requied by the hash table implementation.
                     // This makes this structure hashable
} patched_instruction;

// Specifies when a value in a variable is interesting.
// UNIQUE: The value has not been observed in this variable before.
// MAX: Largest value in this variable observed so far.
// MIN: Smallest value in this variable observed so far.
enum StatePatchWhen { UNIQUE = 0, MIN = 1, MAX = 2 };

typedef struct StatePatch {
  avr_symbol_t *symbol;
  enum StatePatchWhen state_patch_when;
  avr_t *avr;
} StatePatch;

typedef struct StateKey {
  uint32_t when_check;
  uint32_t variable_addr;
  enum StatePatchWhen when_interesting;
} StateKey;

// Hash Table. Implementation from https://troydhanson.github.io/uthash/
// Key: virtual address
// Value: list of function pointers and arguments for these functions
struct patched_instruction *patched_instructions;

void initialize_patch_instructions(struct avr_t *);
void setup_patches(avr_t *avr) __attribute__((weak));
void setup_state_dictionary(avr_t *avr);
int patch_instruction(avr_flashaddr_t vaddr, void *patch_pointer, void *arg);
int patch_function(char *function_name, void *patch_pointer, void *arg,
                   avr_t *avr);
patched_instruction *get_or_create_patched_instruction(avr_flashaddr_t key);
Patch *create_function_patch(void *function, void *arg);
void check_run_patch(avr_t *avr);
void test_patch_function(void *arg);
void fuzz_reset(void *arg);
void print_current_input(void *arg);
void override_args(void *arg);
void raise_external_interrupt(uint8_t pin, avr_t *avr);
avr_int_vector_t *digitalPinToInterrupt(uint8_t pin, avr_t *avr);
void noop(avr_t *avr);
uint32_t get_symbol_address(char *symbol_name, avr_t *avr);
void write_to_ram(uint32_t dst, void *src, size_t num_bytes, avr_t *avr);
void set_shadow_map(avr_flashaddr_t start, size_t size, uint8_t value,
                    avr_t *avr);
void write_fuzz_input_global(void *arg);
StatePatch *create_state_patch(char *symbol_name, enum StatePatchWhen when,
                               avr_t *avr);
int state_key_compare(const void *key1, const void *key2);
int uint64_t_compare(const void *key1, const void *key2);
void add_state(void *arg);

#ifdef __cplusplus
};
#endif
