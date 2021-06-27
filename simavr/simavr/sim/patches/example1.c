#include "fuzz_patch_instructions.h"
#include "sim_avr.h"
#include <stdio.h>

void print_hello(void *arg) {
  printf("Hello\n");
}

void setup_patches(avr_t *avr) {
  patch_instruction(get_symbol_address("setup", avr), print_hello, NULL);
}

