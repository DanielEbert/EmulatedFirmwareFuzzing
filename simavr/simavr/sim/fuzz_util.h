#pragma once

#include "sim_avr.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint64_t sram_to_host_vaddr(avr_t *avr, uint16_t sram_vaddr);
void write_to_sram(avr_t *avr, uint16_t sram_vaddr, void *buf, size_t buf_len);


#ifdef __cplusplus
};
#endif