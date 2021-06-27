#include "fuzz_crash_handler.h"
#include "fuzz_config.h"
#include "fuzz_patch_instructions.h"
#include "fuzz_server_notify.h"
#include "sim_gdb.h"
#include <stdio.h>

void initialize_crash_handler(avr_t *avr) {
  // unique crashes
  CC_HashTableConf config;
  cc_hashtable_conf_init(&config);
  config.key_length = sizeof(CrashKey);
  config.hash = GENERAL_HASH;
  config.key_compare = crash_compare;

  CC_HashTable *crashes;
  if (cc_hashtable_new_conf(&config, &crashes) != CC_OK) {
    printf("ERROR: coverage set allocation failed. Exiting...\n");
    exit(1);
  }
  avr->unique_crashes = crashes;

  initialize_uninitialized_sanitizer(avr);
}

void initialize_uninitialized_sanitizer(avr_t *avr) {
  avr->shadow = calloc(1, 1 << 16);
  avr->shadow_propagation = calloc(sizeof(avr_flashaddr_t), 1 << 16);

  // Set the registers and IO registers to defined (1)
  for (int i = 0; i < 0x200; i++) {
    avr->shadow[i] = 1;
  }

  // If the .data section exists, set to defined (1). The .data section
  // contains static data which was defined in the code.
  if (cc_hashtable_contains_key(avr->symbols, "__data_start")) {
    assert(cc_hashtable_contains_key(avr->symbols, "__data_end"));
    uint32_t __data_start = get_symbol_address("__data_start", avr);
    uint32_t __data_end = get_symbol_address("__data_end", avr);
    assert(__data_start <= __data_end);
    for (uint32_t i = __data_start; i < __data_end; i++) {
      avr->shadow[i] = 1;
    }
  }
}

void stack_buffer_overflow_found(avr_t *avr, avr_flashaddr_t crashing_addr) {
  crash_found(avr, crashing_addr, 0, 0);
}

void uninitialized_value_used_found(avr_t *avr, avr_flashaddr_t crashing_addr,
                                    avr_flashaddr_t origin_addr) {
  crash_found(avr, crashing_addr, origin_addr, 1);
}

void timeout_found(avr_t *avr, avr_flashaddr_t crashing_addr) {
  crash_found(avr, crashing_addr, 0, 2);
}

void invalid_write_address_found(avr_t *avr, avr_flashaddr_t crashing_addr) {
  crash_found(avr, crashing_addr, 0, 3);
}

void bad_jump_found(avr_t *avr, avr_flashaddr_t crashing_addr) {
  crash_found(avr, crashing_addr, 0, 4);
}

void reading_past_end_of_flash_found(avr_t *avr,
                                     avr_flashaddr_t crashing_addr) {
  crash_found(avr, crashing_addr, 0, 5);
}

void crash_found(avr_t *avr, avr_flashaddr_t crashing_addr,
                 avr_flashaddr_t origin_addr, uint8_t crash_id) {
  avr->fuzzer_stats.total_crashes += 1;
  // On non-recoverable crashes: reset
  if (crash_id == 0 || crash_id == 3) {
    avr->do_reset = 1;
  }
  // for crash_id 2, 4, 5 we call fuzz_reset immediately
  // Do not (immediately) reset on uninitialized value usages

  CrashKey *key = malloc(sizeof(CrashKey));
  key->crash_id = crash_id;
  key->crashing_addr = crashing_addr;
  key->origin_addr = origin_addr;
  if (cc_hashtable_contains_key(avr->unique_crashes, key)) {
    free(key);
    return;
  }

  switch (crash_id) {
  case 0:
    printf("Stack Smashing Detected PC %04x\n", crashing_addr);
    break;
  case 1:
    printf("New unique use of uninitialized memory found at PC 0x%04x, with "
           "origin 0x%04x",
           crashing_addr, origin_addr);
    break;
  case 2:
    printf("Timeout found at pc: 0x%04x\n", crashing_addr);
    break;
  case 3:
    printf("Invalid write address PC 0x%04x\n", crashing_addr);
    break;
  case 4:
    printf("Bad jump found at pc: 0x%04x\n", crashing_addr);
    break;
  case 5:
    printf("Reading past end of flash found at pc: 0x%04x\n", crashing_addr);
    break;
  }

  // TODOE
  if (avr->gdb) {
    avr_gdb_handle_watchpoints(avr, crashing_addr, AVR_GDB_WATCH_READ);
  }

  Crash *crash = malloc(sizeof(Crash));
  crash->crash_id = crash_id;
  crash->crashing_addr = crashing_addr;
  crash->origin_addr = origin_addr;

  Input *crashing_input = malloc(sizeof(Input));
  crashing_input->buf_len = avr->fuzzer->current_input->buf_len;
  void *buffer = malloc(avr->max_input_length);
  memcpy(buffer, avr->fuzzer->current_input->buf, avr->max_input_length);
  crashing_input->buf = buffer;

  crash->crashing_input = crashing_input;

  if (cc_hashtable_add(avr->unique_crashes, key, crash) != CC_OK) {
    fprintf(stderr, "ERROR: Failed to add crash to crashing_inputs hashtable");
    exit(1);
  }

  send_crash(avr, crash);
}

// key comparator function which returns true if the keys are identical
int crash_compare(const void *key1, const void *key2) {
  CrashKey *e1 = (CrashKey *)key1;
  CrashKey *e2 = (CrashKey *)key2;
  return (e1->crash_id == e2->crash_id &&
          e1->crashing_addr == e2->crashing_addr &&
          e1->origin_addr == e2->origin_addr);
}