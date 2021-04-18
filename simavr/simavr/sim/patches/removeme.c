#include "fuzz_patch_instructions.h"
#include "sim_avr.h"
#include <stdio.h>

void setup_patches(avr_t *avr) {
  printf("Hi\n");
  patch_instruction(0x6bc, print_current_input, avr);
  patch_instruction(0x67e, fuzz_reset, avr);
}
