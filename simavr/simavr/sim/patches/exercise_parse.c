#include "fuzz_patch_instructions.h"
#include "sim_avr.h"
#include <stdio.h>

void setup_patches(avr_t *avr) {
  // Uncomment the line below if you want to print the generated inputs to the terminal.
  // patch_function(get_symbol_address("setup", avr), print_current_input, avr, avr);
  patch_function("setup", write_fuzz_input_global, avr, avr);
  patch_function("UnityConcludeTest", fuzz_reset, avr, avr);
}
