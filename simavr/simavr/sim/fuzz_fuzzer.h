#pragma once

#include <collectc/cc_array.h>
#include <sim_avr.h>
#include "fuzz_fuzzer.h"

#ifdef __cplusplus
extern "C" {
#endif

// Sets up the current_input and the list of previous interesting inputs/seeds.
void initialize_fuzzer(avr_t *avr, char *path_to_seeds, char *run_once_file, 
                       char* mutator_so_path);

// The idea for this initialization (i.e. loading and calling the mutator
// functions from a .so file) is from AFL++:
// https://github.com/AFLplusplus/AFLplusplus/blob/48cef3c74727407f82c44800d382737265fe65b4/src/afl-fuzz-mutators.c#L138
void initialize_mutator(Fuzzer *fuzzer, char* mutator_so_path);

// For each file in the 'path_to_seeds' folder, add the contents of this file to 
// the previous_interesting_inputs array.
void initialize_seeds(avr_t *avr, CC_Array *previous_interesting_inputs, char *path_to_seeds);

// Read the contents of the 'file_path' file and append it to the list of 
// previous interesting inputs.
void add_seed_from_file(avr_t *avr, CC_Array *previous_interesting_inputs, char *file_path);

// Read 'buf' and append to previous intersting inputs.
void add_previous_interesting_input(CC_Array *previous_interesting_inputs,
                                    char *buf, size_t buf_len);

// Return a random element from the previous interesting inputs.
Input *get_random_previous_interesting_input(CC_Array *inputs);

// Mutate the fuzzer->current_input.
void generate_input(avr_t *avr, Fuzzer *fuzzer);

// A simple mutation of 'input'. This mutator is typically not used.
// Typically the mutator from libFuzzer is used.
uint32_t mutate(Input *input, size_t);

// Check if the current\_input increased coverage and if this is the case,
// copy the current\_input and add the copy to the previous interesting inputs
// array.
void evaluate_input(avr_t *avr);

#ifdef __cplusplus
};
#endif