#pragma once

#include <collectc/cc_array.h>
#include <sim_avr.h>
#include "fuzz_fuzzer.h"

#ifdef __cplusplus
extern "C" {
#endif

void initialize_fuzzer(avr_t *avr, char *path_to_seeds, char *run_once_file);
void initialize_mutator(Fuzzer *fuzzer);
void initialize_seeds(CC_Array *previous_interesting_inputs, char *path_to_seeds);
void add_seed_from_file(CC_Array *previous_interesting_inputs, char *file_path);
void add_previous_interesting_input(CC_Array *previous_interesting_inputs,
                                    char *buf, size_t buf_len);
Input *get_random_previous_interesting_input(CC_Array *inputs);
void generate_input(avr_t *avr, Fuzzer *fuzzer);
void mutate(Input *input);
void evaluate_input(avr_t *avr);

#ifdef __cplusplus
};
#endif