
#include "fuzz_patch_instructions.h"
#include "fuzz_fuzzer.h"
#include "fuzz_server_notify.h"
#include "sim_avr.h"
#include <collectc/cc_hashset.h>
#include <stdio.h>
#include <unistd.h>

int avr_raise_interrupt(avr_t *avr, avr_int_vector_t *vector);

void initialize_patch_instructions(avr_t *avr) {
  Patch_Side_Effects *patch_side_effects = malloc(sizeof(Patch_Side_Effects));
  if (patch_side_effects == NULL) {
    fprintf(stderr, "ERROR: malloc failed\n");
    exit(1);
  }
  patch_side_effects->run_return_instruction = 0;
  avr->patch_side_effects = patch_side_effects;

  patched_instructions = NULL;

  setup_state_dictionary(avr);
  setup_patches(avr);
}

void setup_state_dictionary(avr_t *avr) {
  CC_HashTableConf config;
  cc_hashtable_conf_init(&config);
  config.key_length = sizeof(StateKey);
  config.hash = GENERAL_HASH;
  config.key_compare = state_key_compare;

  CC_HashTable *state;
  if (cc_hashtable_new_conf(&config, &state) != CC_OK) {
    printf("ERROR: state dictionary allocation failed. Exiting...\n");
    exit(1);
  }
  avr->SUT_state = state;
}

// If the user did not 'overwrite' the setup_patches function, call
// the following function instead. Overwrite means the user used the
// LD_PRELOAD environment variable to load a shared object that defines
// a function with the name 'setup_patches'.
void setup_patches(avr_t *avr) { printf("Using no patches.\n"); }

int patch_instruction(avr_flashaddr_t vaddr, void *patch_pointer, void *arg) {
  patched_instruction *p = get_or_create_patched_instruction(vaddr);

  Patch *patch = create_function_patch(patch_pointer, arg);
  DL_APPEND(p->patches, patch);

  return 0;
}

int patch_function(char *function_name, void *patch_pointer, void *arg,
                   avr_t *avr) {
  return patch_instruction(get_symbol_address(function_name, avr),
                           patch_pointer, arg);
}

patched_instruction *get_or_create_patched_instruction(avr_flashaddr_t key) {
  patched_instruction *entry;
  HASH_FIND_UINT32(patched_instructions, &key, entry);

  // Create empty list as value at :key: if no value exists yet
  if (entry == NULL) {
    // Set empty list as value
    Patch *patch = NULL;
    entry = malloc(sizeof(patched_instruction));
    if (entry == NULL) {
      fprintf(stderr, "ERROR: malloc failed\n");
      exit(1);
    }
    entry->vaddr = key;
    entry->patches = patch;
    HASH_ADD_UINT32(patched_instructions, vaddr, entry);
  }

  return entry;
}

Patch *create_function_patch(void *patch_pointer, void *arg) {
  Patch *entry = malloc(sizeof(Patch));
  if (entry == NULL) {
    fprintf(stderr, "ERROR: malloc failed\n");
    exit(1);
  }
  entry->patch_pointer = patch_pointer;
  entry->arg = arg;
  entry->next = 0;
  entry->prev = 0;
  return entry;
}

void check_run_patch(avr_t *avr) {
  patched_instruction *patch;
  HASH_FIND_INT(patched_instructions, &(avr->pc), patch);
  if (patch != NULL) {
    Patch *t;
    DL_FOREACH(patch->patches, t) {
      void (*s)() = t->patch_pointer;
      (*s)(t->arg);
    }
  }
}

uint32_t get_symbol_address(char *symbol_name, avr_t *avr) {
  // 0 if no symbol found, 1 if no const, 2 if const
  int prefix_type = 0;
  // Symbol name has prefix '_ZL10' if the symbol is a 'const'.
  char prefix_symbol[517] = "_ZL10";
  strncpy(prefix_symbol + 5, symbol_name, 511);
  if (cc_hashtable_contains_key(avr->symbols, symbol_name)) {
    prefix_type = 1;
  } else if (cc_hashtable_contains_key(avr->symbols, prefix_symbol)) {
    prefix_type = 2;
  } else {
    // Because this function is called during setup, we can exit here to give
    // fast feedback to the user
    fprintf(stderr, "get_symbol_name Error: No such symbol: %s\n", symbol_name);
    exit(1);
  }
  void *entry;
  if (cc_hashtable_get(avr->symbols,
                       prefix_type == 1 ? symbol_name : prefix_symbol,
                       &entry) != CC_OK) {
    fprintf(stderr, "Failed to retrieve hashtable value for symbol %s\n",
            symbol_name);
    exit(1);
  }
  avr_symbol_t *symbol_entry = (avr_symbol_t *)entry;
  // Addresses in symbols have an 0x800000 offset if they are in RAM
  return symbol_entry->addr % 0x800000;
}

void write_to_ram(uint32_t dst, void *src, size_t num_bytes, avr_t *avr) {
  // avr-> data is the start address of the memory block that stores the
  // virtual RAM of the emulated SUT
  memcpy(avr->data + dst, src, num_bytes);

  // If we overwrite RAM, we must set the shadow bits to 1 (i.e. defined).
  set_shadow_map(dst, num_bytes, 1, avr);
}

uint64_t read_from_ram(uint32_t addr, size_t num_bytes, avr_t *avr) {
  // For AVR, ELF symbols have a 0x800000 offset if the value is in RAM.
  // avr->data is start of a buffer that stores virtual RAM of emulated process.
  uint8_t *src = avr->data + (addr % 0x800000);
  uint64_t ret = 0;
  for (int i = num_bytes - 1; i >= 0; i--) {
    ret <<= 1;
    ret += src[i];
  }
  return ret;
}

void set_shadow_map(avr_flashaddr_t start, size_t size, uint8_t value,
                    avr_t *avr) {
  avr_flashaddr_t end = start + size;
  for (int i = start; i < end; i++) {
    avr->shadow[i] = 1;
  }
}

void print_current_input(void *arg) {
  avr_t *avr = (avr_t *)arg;
  printf("Current input (Length %ld):\n", avr->fuzzer->current_input->buf_len);
  fwrite(avr->fuzzer->current_input->buf, avr->fuzzer->current_input->buf_len,
         1, stdout);
  printf("\n");
}

void fuzz_reset(void *arg) {
  avr_t *avr = (avr_t *)arg;
  avr->fuzzer_stats.inputs_executed += 1;
  if (avr->run_once) {
    printf("Exiting normally\n");
    sleep(1);
    exit(0);
  }
  evaluate_input(avr);
  generate_input(avr, avr->fuzzer);

  // Reset stack area of shadow map
  uint32_t stack_top = 1 << 16;
  for (avr_flashaddr_t i = avr->stackframe_min_sp; i < stack_top; i++) {
    avr->shadow[i] = 0;
  }

  if (avr->fuzzer_stats.inputs_executed % 10000 == 0) {
    send_fuzzer_stats(avr);
  }

  avr_reset(avr);
}

void raise_external_interrupt(uint8_t pin, avr_t *avr) {
  avr_raise_interrupt(avr, digitalPinToInterrupt(pin, avr));
}

avr_int_vector_t *digitalPinToInterrupt(uint8_t pin, avr_t *avr) {
  uint8_t vector_index = 0;
  switch (pin) {
  case 2:
    vector_index = 6;
    break;
  case 3:
    vector_index = 7;
    break;
  case 21:
    vector_index = 2;
    break;
  case 20:
    vector_index = 3;
    break;
  case 19:
    vector_index = 4;
    break;
  case 18:
    vector_index = 5;
    break;
  default:
    fprintf(stderr,
            "ERROR: Unsupported pin value %d for the digitalPinToInterrupt "
            "user API function.\n",
            pin);
    exit(1);
    break;
  }
  return avr->interrupts.vector[vector_index];
}

void test_raise_interrupt(void *arg) {
  avr_t *avr = (avr_t *)arg;
  avr_raise_interrupt(avr, digitalPinToInterrupt(18, avr));
}

void write_fuzz_input_global(void *arg) {
  avr_t *avr = (avr_t *)arg;
  avr_flashaddr_t fuzz_input_addr = get_symbol_address("fuzz_input", avr);
  avr_flashaddr_t length_addr = get_symbol_address("fuzz_input_length", avr);

  write_to_ram(fuzz_input_addr, avr->fuzzer->current_input->buf,
               avr->fuzzer->current_input->buf_len, avr);

  uint8_t length_addr_little_endian[] = {
      avr->fuzzer->current_input->buf_len % 256,
      (avr->fuzzer->current_input->buf_len >> 8) % 256};
  write_to_ram(length_addr, length_addr_little_endian, 2, avr);
}

StatePatch *create_state_patch(char *symbol_name, enum StatePatchWhen when,
                               avr_t *avr) {
  StatePatch *state_patch = malloc(sizeof(StatePatch));
  if (state_patch == NULL) {
    fprintf(stderr, "ERROR: malloc failed\n");
    exit(1);
  }
  state_patch->state_patch_when = when;
  state_patch->avr = avr;

  if (!cc_hashtable_contains_key(avr->symbols, symbol_name)) {
    // Because this function is called during setup, we can exit here to give
    // fast feedback to the user
    fprintf(stderr, "add_state Error: No such symbol: %s\n", symbol_name);
    exit(1);
  }
  void *entry;
  if (cc_hashtable_get(avr->symbols, symbol_name, &entry) != CC_OK) {
    fprintf(stderr, "Failed to retrieve hashtable value for symbol %s\n",
            symbol_name);
    exit(1);
  }
  state_patch->symbol = (avr_symbol_t *)entry;
  return state_patch;
}

void add_state(void *arg) {
  StatePatch *state_patch = (StatePatch *)arg;
  avr_t *avr = state_patch->avr;
  StateKey *state_key = malloc(sizeof(StateKey));
  if (state_key == NULL) {
    fprintf(stderr, "ERROR: malloc failed\n");
    exit(1);
  }
  state_key->when_check = avr->pc;
  state_key->variable_addr = state_patch->symbol->addr;
  state_key->when_interesting = state_patch->state_patch_when;

  // This state system supports unsigned integers with a maxium bitlength of 64.
  if (state_patch->symbol->size > 8) {
    fprintf(stderr,
            "ERROR: state with state_patch_when supports a maximum size "
            "of 8 bytes. Your ELF symbol %s has a size of %ld bytes\n",
            state_patch->symbol->symbol, state_patch->symbol->size);
    exit(1);
  }

  uint64_t *min_or_max;
  CC_HashSetConf config;
  // Check if state exists already
  if (!cc_hashtable_contains_key(avr->SUT_state, state_key)) {
    // if not, create new entry and add it to the state dictionary
    switch (state_patch->state_patch_when) {
    case MAX:
    case MIN:
      // Even if the symbol size is less than 8 bytes (e.g. if the state is a
      // uint32_t), we store the maximum value in a uint64_t so we have less
      // branches.
      min_or_max = malloc(sizeof(uint64_t));
      if (min_or_max == NULL) {
        fprintf(stderr, "ERROR: malloc failed\n");
        exit(1);
      }
      *min_or_max = read_from_ram(state_patch->symbol->addr,
                                  state_patch->symbol->size, avr);
      if (cc_hashtable_add(avr->SUT_state, state_key, min_or_max) != CC_OK) {
        fprintf(stderr, "ERROR: Failed to add state to state hashtable");
        exit(1);
      }
      break;
    case UNIQUE:
      cc_hashset_conf_init(&config);
      config.key_length = sizeof(uint64_t);
      config.hash = GENERAL_HASH;
      config.key_compare = uint64_t_compare;
      CC_HashSet *unique_hashset;
      if (cc_hashset_new_conf(&config, &unique_hashset) != CC_OK) {
        printf("ERROR: 'unique' state hashset allocation failed. Exiting...\n");
        exit(1);
      }
      if (cc_hashtable_add(avr->SUT_state, state_key, unique_hashset) !=
          CC_OK) {
        fprintf(stderr, "ERROR: Failed to add state to state hashtable");
        exit(1);
      }
      uint64_t *current = malloc(sizeof(uint64_t));
      if (current == NULL) {
        fprintf(stderr, "ERROR: malloc failed\n");
        exit(1);
      }
      *current = read_from_ram(state_patch->symbol->addr,
                               state_patch->symbol->size, avr);
      if (cc_hashset_add(unique_hashset, current) != CC_OK) {
        fprintf(stderr, "ERROR: Failed to add state to state hashset");
        exit(1);
      }
      break;
    default:
      fprintf(stderr,
              "Warning: Unknown state_patch_when value for state key with "
              "PC 0x%04x\n",
              avr->pc);
      free(state_key);
      break;
    }
    return;
  }

  void *entry = NULL;
  if (cc_hashtable_get(avr->SUT_state, state_key, &entry) != CC_OK) {
    fprintf(stderr,
            "Warning: Failed to retrieve hashtable value for state key with "
            "PC 0x%04x\n",
            avr->pc);
    free(state_key);
    return;
  }

  // if previous state exists, compare the previous state with the current
  // state (i.e. the current_value)
  uint64_t current_value =
      read_from_ram(state_patch->symbol->addr, state_patch->symbol->size, avr);
  // Check if new state. if yes, avr->interestin = 1
  switch (state_patch->state_patch_when) {
  case MIN:
    if (current_value < *(uint64_t *)entry) {
      printf("New MIN found for state %s from %lu to %lu\n",
             state_patch->symbol->symbol, *(uint64_t *)entry, current_value);
      *(uint64_t *)entry = current_value;
      avr->input_has_reached_new_coverage = 1;
    }
    break;
  case MAX:
    if (current_value > *(uint64_t *)entry) {
      printf("New MAX found for state %s from %lu to %lu\n",
             state_patch->symbol->symbol, *(uint64_t *)entry, current_value);
      *(uint64_t *)entry = current_value;
      avr->input_has_reached_new_coverage = 1;
    }
    break;
  case UNIQUE:
    if (cc_hashset_contains(entry, &current_value)) {
      break;
    }
    printf("New state UNIQUE with value %lu\n", current_value);
    uint64_t *current_value_heap = malloc(sizeof(uint64_t));
    *current_value_heap = current_value;
    if (cc_hashset_add(entry, current_value_heap) != CC_OK) {
      fprintf(stderr, "ERROR: Failed to add state to state hashset");
      exit(1);
    }
    avr->input_has_reached_new_coverage = 1;
    break;
  default:
    fprintf(stderr,
            "Warning: Unknown state_patch_when value for state key with "
            "PC 0x%04x\n",
            avr->pc);
    break;
  }
  free(state_key);
}

int state_key_compare(const void *key1, const void *key2) {
  StateKey *e1 = (StateKey *)key1;
  StateKey *e2 = (StateKey *)key2;
  return !(e1->when_check == e2->when_check &&
           e1->variable_addr == e2->variable_addr &&
           e1->when_interesting == e2->when_interesting);
}

int uint64_t_compare(const void *key1, const void *key2) {
  uint64_t *e1 = (uint64_t *)key1;
  uint64_t *e2 = (uint64_t *)key2;
  return !(*e1 == *e2);
}