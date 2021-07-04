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
  uint8_t crash_id; // Specifies the Type of Crash. Either UUM, timeout, ...
  avr_flashaddr_t crashing_addr; // SUT program counter when the crash was first detected.
  avr_flashaddr_t origin_addr;  // optional, only used for UUM, 0 if not used
} CrashKey;

// Like CrashKey, but also contains crashing_input. crashing_input is also sent
// to the UI server.
typedef struct Crash {
  uint8_t crash_id;
  avr_flashaddr_t crashing_addr;
  avr_flashaddr_t origin_addr; // only used if crash_id is 1
  Input *crashing_input;
} Crash;

// Sets up a dictionary to deduplicate crashes. Dictionary keys are 'CrashKey'
// structs. Values are Crash structs.
void initialize_crash_handler(avr_t *avr);

// The following *_found functions are called when a sanitizer or emulator
// finds the corresponding bug type.
void initialize_uninitialized_sanitizer(avr_t *avr);
void uninitialized_value_used_found(avr_t *avr, avr_flashaddr_t crashing_addr,
                                    avr_flashaddr_t origin_addr);
void timeout_found(avr_t *avr, avr_flashaddr_t crashing_addr);
void invalid_write_address_found(avr_t *avr, avr_flashaddr_t crashing_addr);
void bad_jump_found(avr_t *avr, avr_flashaddr_t crashing_addr);
void reading_past_end_of_flash_found(avr_t *avr,
                                     avr_flashaddr_t crashing_addr);
void stack_buffer_overflow_found(avr_t *avr, avr_flashaddr_t crashing_addr);
void crash_found(avr_t *avr, avr_flashaddr_t crashing_vaddr, 
                 avr_flashaddr_t origin_addr, uint8_t crash_id);

// Check if two crashes are equal.
// key1 and key2 must point to CrashKey structs.
// Returns 0 if both CrashKeys are equal (i.e. struct members match). 
int crash_compare(const void *key1, const void *key2);

#ifdef __cplusplus
};
#endif