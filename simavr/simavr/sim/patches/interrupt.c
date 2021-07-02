#include "fuzz_patch_instructions.h"
#include "sim_avr.h"
#include <stdio.h>

void raise_interrupt(void *arg) {
  avr_t *avr = (avr_t *)arg;
  raise_external_interrupt(19, avr);
}

void setup_patches(avr_t *avr) {
  //patch_instruction(get_symbol_address("setup", avr), write_fuzz_input_global, avr);

  patch_instruction(get_symbol_address("loop", avr), raise_interrupt, avr);
  patch_instruction(get_symbol_address("_Z14wait_for_resetv", avr), fuzz_reset, avr);
}
