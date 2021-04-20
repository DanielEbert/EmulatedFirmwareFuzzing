#include "fuzz_patch_instructions.h"
#include "sim_avr.h"
#include <stdio.h>

void write_fuzz_input_global(void *);
void set_shadow_map(avr_flashaddr_t start, size_t size, uint8_t value,
                    avr_t *avr);

void setup_patches(avr_t *avr) {
  patch_instruction(get_symbol_address("setup", avr), write_fuzz_input_global,
                    avr);
  patch_instruction(get_symbol_address("loop", avr), fuzz_reset, avr);
}

void write_fuzz_input_global(void *arg) {
  avr_t *avr = (avr_t *)arg;
  avr_flashaddr_t fuzz_input_addr =
      get_symbol_address("fuzz_input", avr) - 0x800000;
  avr_flashaddr_t length_addr =
      get_symbol_address("fuzz_input_length", avr) - 0x800000;

  write_to_flashaddr(fuzz_input_addr, avr->fuzzer->current_input->buf,
                     avr->fuzzer->current_input->buf_len, avr);

  uint8_t length_addr_little_endian[] = {
      avr->fuzzer->current_input->buf_len % 256,
      (avr->fuzzer->current_input->buf_len >> 8) % 256};
  write_to_flashaddr(length_addr, length_addr_little_endian, 2, avr);
}
