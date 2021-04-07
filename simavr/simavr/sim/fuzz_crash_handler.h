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
} CrashKey;

typedef struct Crash {
  // 0 = Stack Buffer Overflow
  uint8_t crash_id;
  avr_flashaddr_t crashing_addr;
  Input *crashing_input;
} Crash;

void initialize_crash_handler(avr_t *avr);
int crash_compare(const void *key1, const void *key2);
void crash_found(avr_t *avr, avr_flashaddr_t crashing_vaddr, uint8_t crash_id);

#ifdef __cplusplus
};
#endif