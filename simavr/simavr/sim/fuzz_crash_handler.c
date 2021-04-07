#include "fuzz_crash_handler.h"
#include "fuzz_config.h"
#include "fuzz_server_notify.h"
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
}

void crash_found(avr_t *avr, avr_flashaddr_t crashing_addr, uint8_t crash_id) {
  avr->do_reset = 1;

  CrashKey crashKey = {.crash_id = crash_id, .crashing_addr = crashing_addr};
  if (cc_hashtable_contains_key(avr->unique_crashes, &crashKey)) {
    return;
  }

  Crash *crash = malloc(sizeof(Crash));
  crash->crash_id = crash_id;
  crash->crashing_addr = crashing_addr;

  Input *crashing_input = malloc(sizeof(Input));
  crashing_input->buf_len = avr->fuzzer->current_input->buf_len;
  void *buffer = malloc(MAX_INPUT_LENGTH);
  memcpy(buffer, avr->fuzzer->current_input->buf, MAX_INPUT_LENGTH);
  crashing_input->buf = buffer;

  crash->crashing_input = crashing_input;

  CrashKey *key = malloc(sizeof(CrashKey));
  key->crash_id = crash_id;
  key->crashing_addr = crashing_addr;
  if (cc_hashtable_add(avr->unique_crashes, key, crash) != CC_OK) {
    fprintf(stderr, "ERROR: Failed to add crash to crashing_inputs hashtable");
    exit(1);
  }

  send_crash(avr->server_connection, crash);
}

// Returns zero if keys are equal
int crash_compare(const void *key1, const void *key2) {
  CrashKey *e1 = (CrashKey *)key1;
  CrashKey *e2 = (CrashKey *)key2;
  return !(e1->crash_id == e2->crash_id &&
           e1->crashing_addr == e2->crashing_addr);
}