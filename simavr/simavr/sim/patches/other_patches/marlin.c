#include "fuzz_patch_instructions.h"
#include "sim_avr.h"
#include <stdio.h>

void setup_patches(avr_t *avr) {
  //patch_instruction(get_symbol_address("setup", avr), print_current_input,
  //avr);
  patch_instruction(get_symbol_address("setup", avr), write_fuzz_input_global,
                    avr);
  patch_instruction(get_symbol_address("_Z10reset_herev", avr), fuzz_reset,
                    avr);
  patch_instruction(get_symbol_address("_Z7minkillb", avr), fuzz_reset, avr);
}
