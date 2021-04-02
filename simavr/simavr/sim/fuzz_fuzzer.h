#pragma once

#include <collectc/cc_array.h>
#include <sim_avr.h>

#ifdef __cplusplus
extern "C" {
#endif



void initialize_fuzzer(avr_t *avr, char *path_to_seeds, size_t max_input_len);
void initialize_seeds(CC_Array *previous_interesting_inputs,
                      char *path_to_seeds, size_t max_len);
void add_seed_from_file(CC_Array *previous_interesting_inputs, char *file_path,
                        size_t max_len);
void add_previous_interesting_input(CC_Array *previous_interesting_inputs,
                                    char *buf, size_t buf_len);
Input *get_random_previous_interesting_input(CC_Array *inputs);
void generate_input(avr_t *avr, Fuzzer *fuzzer);
void mutate(void *buffer, size_t buf_len, size_t max_len);
void evaluate_input(avr_t *avr);

#ifdef __cplusplus
};
#endif