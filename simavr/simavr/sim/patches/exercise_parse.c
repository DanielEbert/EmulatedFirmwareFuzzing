#include "fuzz_patch_instructions.h"
#include "sim_avr.h"
#include <stdio.h>

void setup_patches(avr_t *avr) {
  // Uncomment the line below if you want to print the generated inputs to the terminal.
  // patch_instruction(get_symbol_address("setup", avr), print_current_input, avr);
  patch_instruction(get_symbol_address("setup", avr), write_fuzz_input_global, avr);
  patch_instruction(get_symbol_address("UnityConcludeTest", avr), fuzz_reset, avr);
}
