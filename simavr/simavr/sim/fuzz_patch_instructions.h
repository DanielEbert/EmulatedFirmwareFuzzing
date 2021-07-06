#pragma once

#include "sim_avr.h"
#include "sim_avr_types.h"
#include "sim_uthash.h"
#include "sim_utlist.h"

#ifdef __cplusplus
extern "C" {
#endif

// 'patch_pointer' is the address of a function. 'arg' (argument) is passed to
// this function. Patch is an element in a double linked list. This list
// is the value in the patched_instructions dictionary. The value's key is
// the an address that specifies when 'patch_pointer' should be called with
// 'arg' as the function argument during emulation.
typedef struct Patch {
  void *patch_pointer;
  void *arg;
  struct Patch *next, *prev;
} Patch;

// Elements in the patched_instructions dictionary are patched_instruction
// structs.
typedef struct patched_instruction {
  avr_flashaddr_t vaddr; // we use this field as the key
  struct Patch *patches; // this is the value, a list of Patch structs
  UT_hash_handle hh;     // Only requied by the hash table implementation.
                         // This makes this structure hashable
} patched_instruction;

// Specifies when a new value in a variable is interesting.
// UNIQUE: The value has not been observed in this variable before.
// MAX: Largest value in this variable observed so far.
// MIN: Smallest value in this variable observed so far.
enum StatePatchWhen { UNIQUE = 0, MIN = 1, MAX = 2 };

// Keys for the SUT_state dictionary. The SUT_state dictionary is used and
// described in section 'Identify Interesting Program States via Data Values'.
typedef struct StateKey {
  uint32_t when_check; // at what value of the program counter should the check
                       // occur?
  uint32_t variable_addr; // Address of the variable whos value we check.
  enum StatePatchWhen when_interesting; // see StatePatchWhen enum docs above
} StateKey;

// Value for the SUT_state dictionary.
typedef struct StatePatch {
  avr_symbol_t *symbol; // ELF symbol of the variable we are checking.
  enum StatePatchWhen state_patch_when; // see StatePatchWhen enum docs above
  avr_t *avr;
} StatePatch;

// Hash Table. Implementation from https://troydhanson.github.io/uthash/
// Key: virtual address that specifies when the functions in the value should be
// called.
// Value: list of Patch structs (i.e. function pointers and arguments
// for these functions)
struct patched_instruction *patched_instructions;

// Initialize Patch_Side_Effects struct.
// Via this struct the user can modify the control flow of the SUT. For example,
// a user can specify that the emulator should emulate the 'RETURN' instruction.
// This is useful if the user wants to 'skip' a function call.
// For example, if the SUT calls function X, the user can specify that the
// function X should immediately return. Thus the function body of X is not
// executed.
// To do this, the user has to set via the user API:
// 'avr->patch_side_effects->run_return_instruction = 1'.
// You need to be careful if you use this feature: The last element pushed
// onto the stack must be the return address. (The return instruction pops an
// element from the stack and sets the program counter to this value.) For
// example, immediately after a 'call' instruction, the topmost element on the
// stack is a return address.
void initialize_patch_instructions(struct avr_t *);

// If the user did not 'overwrite' the setup_patches function, call
// the following function instead. Overwrite means the user used the
// LD_PRELOAD environment variable to load a shared object that defines
// a function with the name 'setup_patches'.
// For this reason the 'weak' attribute is used.
void setup_patches(avr_t *avr) __attribute__((weak));

// Initializes the SUT_state dictionary.
void setup_state_dictionary(avr_t *avr);

// Specify that prior to emulating the instruction at address 'vaddr', the
// emulator should call function 'patch_pointer' with argument 'arg'.
// 'vaadr' is an unsigned integer that specifies an address in the address
// space of the SUT.
// 'patch_pointer' is a pointer to a function, i.e. 'patch_pointer' is an
// address in the address space of the emulator process.
// When the 'patch_pointer' function is called, 'arg' is passed to this function
// as a pointer (i.e. arg is not dereferenced).
// The student exercises and the paper describes this in more detail. The
// student exercises include examples on how this function is used.
int patch_instruction(avr_flashaddr_t vaddr, void *patch_pointer, void *arg);

// A small wrapper around the patch_instruction where 'function_name' is
// translated to an address before  patch_instruction is called with this
// address. 'patch_pointer' and 'arg' is like the patch_instruction above.
// The student exercises and the paper describes this in more detail. The
// student exercises include examples on how this function is used.
int patch_function(char *function_name, void *patch_pointer, void *arg,
                   avr_t *avr);

// Try and return the value of key 'key' in the patched_instructions dictionary.
// If no such value exists, create a new value and return it.
patched_instruction *get_or_create_patched_instruction(avr_flashaddr_t key);

// Allocate a Patch struct and initialize it with the default values.
Patch *create_function_patch(void *function, void *arg);

// Check if the user specifies that functions should be called.
// If this is the case, call these functions with their argument.
// 'avr->pc' is the current program counter of the emulated SUT.
void check_run_patch(avr_t *avr);

// Evaluate the current input (i.e. check if current input increased code
// coverage), then generate a new input and reset any relevant fuzzer and
// emulator state (e.g. the stack of the shadow map from the UUM sanitizer).
void fuzz_reset(void *arg);

// Print the current input to the terminal.
void print_current_input(void *arg);

// Emulate sending an external interrupt via the GPIO pin 'pin'. The 'External
// Interrupts' section in the Design chapter includes an example of how this
// function can be used in the user API.
void raise_external_interrupt(uint8_t pin, avr_t *avr);

// Convert a GPIO Pin number to a avr_int_vector_t struct. The emulator uses
// this avr_int_vector_t struct to specify the interrupt vector.
avr_int_vector_t *digitalPinToInterrupt(uint8_t pin, avr_t *avr);

// Return the address of the ELF symbol with the name 'symbol_name'. The
// returned address is an address in the SUT's address space.
// The paper in section 'Symbols and Writing to Global Variables' describes
// this in more detail.
uint32_t get_symbol_address(char *symbol_name, avr_t *avr);

// Write 'num_bytes' bytes starting from 'src' to 'dst'. 'src' is an address
// in the address space of the emulator process. 'dst' is an address
// in the address space of the emulated SUT process.
// The paper in section 'Patch Instruction and Direct Memory Access' describes
// this in more detail.
// This also correctly updates the shadow map for the UUM sanitizer.
void write_to_ram(uint32_t dst, void *src, size_t num_bytes, avr_t *avr);

// Set the shadow map all offsets (i.e. addresses in the SUT's program space)
// from offset 'start' to offset 'start'+'size-1' to 'value'.
// For example with the call
// set_shadow_map(0x1000, 2, 1, avr);
// The two bytes in the SUT's RAM at address 0x1000 and 0x1001 are set to
// defined.
void set_shadow_map(avr_flashaddr_t start, size_t size, uint8_t value,
                    avr_t *avr);

// Write the current input to the fuzz_input variable in the emulated SUT.
// Additionally, write the input length to the fuzz_input_length variable in the
// emulated SUT. The caller (i.e. user) must make sure that the fuzz_input
// variable/buffer in the SUT is large enought to store all bytes of the current
// input. The default maximum input length is 128 bytes. A user can change this
// default value via the --max_input_length command line argument for 'emu'.
// The student exercises and the paper in section 'Symbols and Writing to
// Global Variables' describe this in more detail.
void write_fuzz_input_global(void *arg);

// Allocate a StatePatch struct and initialize its members with the
// function arguments 'symbol_name', 'when', and 'avr'.
StatePatch *create_state_patch(char *symbol_name, enum StatePatchWhen when,
                               avr_t *avr);

// The usage of this function is described in the paper in section 'Identify
// interesting Program States via Data Values'.
void add_state(void *arg);

// Compare two keys for equality. These keys point to StateKey structs.
// Two StateKey structs are equal if all member values are the same.
// Return 0 if structs are equal.
int state_key_compare(const void *key1, const void *key2);

// Compare two uint64_t variables for equality. Both keys point to a uint64_t.
// Return 0 if equal numbers are equal.
int uint64_t_compare(const void *key1, const void *key2);

#ifdef __cplusplus
};
#endif
