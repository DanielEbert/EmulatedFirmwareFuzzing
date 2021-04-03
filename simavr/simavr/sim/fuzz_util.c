#include "fuzz_util.h"
#include "sim_avr.h"
#include <stdint.h>

uint64_t sram_to_host_vaddr(avr_t *avr, uint16_t sram_vaddr) {
  return (uint64_t)avr->data + sram_vaddr;
}

// :buf_len: in bytes
void write_to_sram(avr_t *avr, uint16_t sram_vaddr, void *buf, size_t buf_len) {
  uint64_t host_vaddr = sram_to_host_vaddr(avr, sram_vaddr);
  memcpy((void *)host_vaddr, buf, buf_len);
}