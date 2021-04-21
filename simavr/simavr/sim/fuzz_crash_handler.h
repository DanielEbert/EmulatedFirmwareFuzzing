#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "sim_avr_types.h"
#include "sim_avr.h"

typedef uint32_t avr_flashaddr_t;
typedef struct avr_t avr_t;
typedef struct Input Input;

typedef struct CrashKey {
  uint8_t crash_id;
  avr_flashaddr_t crashing_addr;
  avr_flashaddr_t origin_addr;  // optional, 0 if not used
} CrashKey;

typedef struct Crash {
  // 0 = Stack Buffer Overflow
  // 1 = uninitialized value is used
  uint8_t crash_id;
  avr_flashaddr_t crashing_addr;
  avr_flashaddr_t origin_addr; // only used if crash_id is 1
  Input *crashing_input;
} Crash;

void initialize_crash_handler(avr_t *avr);
void initialize_uninitialized_sanitizer(avr_t *avr);
void uninitialized_value_used_found(avr_t *avr, avr_flashaddr_t crashing_addr,
                                    avr_flashaddr_t origin_addr);
int crash_compare(const void *key1, const void *key2);
void stack_buffer_overflow_found(avr_t *avr, avr_flashaddr_t crashing_addr);
void crash_found(avr_t *avr, avr_flashaddr_t crashing_vaddr, 
                 avr_flashaddr_t origin_addr, uint8_t crash_id);

#ifdef __cplusplus
};
#endif