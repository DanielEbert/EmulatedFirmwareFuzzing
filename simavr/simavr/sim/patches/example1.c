#include "fuzz_patch_instructions.h"
#include "sim_avr.h"
#include <stdio.h>

void print_hello(void *arg) {
  printf("Hello\n");
}

void setup_patches(avr_t *avr) {
  patch_function("setup", print_hello, NULL, avr);
}

