
#include "fuzz_patch_instructions.h"
#include "fuzz_fuzzer.h"
#include "fuzz_server_notify.h"
#include "fuzz_util.h"
#include "sim_avr.h"
#include <stdio.h>
#include <unistd.h>

void initialize_patch_instructions(avr_t *avr) {
  Patch_Side_Effects *patch_side_effects = malloc(sizeof(Patch_Side_Effects));
  patch_side_effects->run_return_instruction = 0;
  avr->patch_side_effects = patch_side_effects;

  // TODOE make this part of avr_t. currently its a global variable
  patched_instructions = NULL;

  setup_patches(avr);

  // patch_instruction(0x87c, test_patch_function, avr);
  // patch_instruction(0x8fe, test_patch_function, avr);
  // patch_instruction(0x694, test_patch_function, avr);
  // patch_instruction(0x90e, test_reset, avr);

  // test2
  // patch_instruction(0x4e2, test_patch_function, avr);
  // patch_instruction(0x916, test_reset, avr);

  // deserializeJson

  // patch_instruction(0x370c, override_args, avr);
  // patch_instruction(0x37d4, test_reset, avr);

  // irq tests
  // patch_instruction(0x5f4, noop, avr);
  // patch_instruction(0x606, test_raise_interrupt, avr);
  // patch_instruction(0x6bc, print_current_input, avr);
  // patch_instruction(0x67e, test_reset, avr);
}

void setup_patches(avr_t *avr) { printf("Using no patches.\n"); }

int patch_instruction(avr_flashaddr_t vaddr, void *patch_pointer, void *arg) {
  patched_instruction *p = get_or_create_patched_instruction(vaddr);

  DL_APPEND(p->patches, create_function_patch(patch_pointer, arg));

  return 0;
}

patched_instruction *get_or_create_patched_instruction(avr_flashaddr_t key) {
  patched_instruction *entry;
  HASH_FIND_UINT32(patched_instructions, &key, entry);

  // Create empty list as value at :key: if no value exists yet
  if (entry == NULL) {
    // Set empty list as value for key target_vaddr in vaddr_hooks_table
    Patch *patch = NULL;
    entry = malloc(sizeof(patched_instruction));
    entry->vaddr = key;
    entry->patches = patch;
    HASH_ADD_UINT32(patched_instructions, vaddr, entry);
  }

  return entry;
}

Patch *create_function_patch(void *patch_pointer, void *arg) {
  Patch *entry = malloc(sizeof(Patch));
  entry->patch_pointer = patch_pointer;
  entry->arg = arg;
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
  if (!cc_hashtable_contains_key(avr->symbols, symbol_name)) {
    // Because this function is called during setup, we can exit here to give
    // fast feedback to the user
    fprintf(stderr, "get_symbol_name Error: No such symbol: %s\n", symbol_name);
    exit(1);
  }
  void *entry;
  if (cc_hashtable_get(avr->symbols, symbol_name, &entry) != CC_OK) {
    fprintf(stderr, "Failed to retrieve hashtable value for symbol %s\n",
            symbol_name);
    exit(1);
  }
  avr_symbol_t *symbol_entry = (avr_symbol_t *)entry;
  // Addresses in symbols have an 0x800000 offset if they are in RAM
  return symbol_entry->addr % 0x800000;
}

void write_to_ram(uint32_t dst, void *src, size_t num_bytes, avr_t *avr) {
  memcpy(avr->data + dst, src, num_bytes);

  set_shadow_map(dst, num_bytes, 1, avr);
}

void set_shadow_map(avr_flashaddr_t start, size_t size, uint8_t value,
                    avr_t *avr) {
  avr_flashaddr_t end = start + size;
  for (int i = start; i < end; i++) {
    avr->shadow[i] = 1;
  }
}

void noop(avr_t *avr) {}

void print_current_input(void *arg) {
  avr_t *avr = (avr_t *)arg;
  printf("Current input (Length %ld):\n", avr->fuzzer->current_input->buf_len);
  fwrite(avr->fuzzer->current_input->buf, avr->fuzzer->current_input->buf_len,
         1, stdout);
  printf("\n");
}

// TODOE this doesnt work the way i expected?
void test_patch_function(void *arg) {
  ((avr_t *)arg)->patch_side_effects->run_return_instruction = 1;
  printf("Hello from test_patch_function, arg: %ld\n", ((avr_t *)arg)->cycle);
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

  if (avr->fuzzer_stats.inputs_executed % 10000 ==
      0) { // TODOE: instead use a timer and check current time
    send_fuzzer_stats(avr);
  }

  avr_reset(avr);
}

void override_args(void *arg) {
  avr_t *avr = (avr_t *)arg;

  uint16_t input_length = avr->fuzzer->current_input->buf_len;
  avr->data[22] = input_length % 256;
  avr->data[23] = (input_length >> 8) % 256;

  // for (int i = 15; i < 30; i++) {
  //  printf("Register %d = %x\n", i, avr->data[i]);
  //}
  // printf("Data: %x\n", *(avr->data + avr->data[24] + avr->data[25] * 256
  // + 2)); avr->data[24] = 0;
  // TODO: now i need a method to convert emulator virtual address to host
  // vaddr
  printf("Overriding args\n");
  write_to_sram(avr, avr->data[24] + avr->data[25] * 256,
                avr->fuzzer->current_input->buf,
                avr->fuzzer->current_input->buf_len);
  // set arg 2
}

void test_raise_interrupt(void *arg) {
  avr_t *avr = (avr_t *)arg;
  avr_raise_interrupt(avr, avr->interrupts.vector[5]);
  // TODO: i cant call above in a loop. how many cycles do i need to wait?
  // should i call reset?
}