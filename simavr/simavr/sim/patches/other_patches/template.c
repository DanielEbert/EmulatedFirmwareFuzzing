#include "fuzz_patch_instructions.h"
#include "sim_avr.h"
#include <stdio.h>

void setup_patches(avr_t *avr) {
  // patch_instruction(uint32_t address, void *function_pointer, void *function_argument);
  // The 'address' argument is the address of an instruction in the SUT's program space. 
  // Prior to emulating this instruction, the emulator calls the function pointed to by 
  // the 'function_pointer' argument. Thereby the 'function_argument' argument is passed 
  // as a parameter to the 'function_pointer' function.
}

