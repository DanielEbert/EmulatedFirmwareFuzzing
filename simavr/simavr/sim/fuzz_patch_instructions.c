
#include "fuzz_patch_instructions.h"
#include "fuzz_fuzzer.h"
#include "fuzz_util.h"
#include "sim_avr.h"
#include <stdio.h>

void initialize_patch_instructions(avr_t *avr) {
  Patch_Side_Effects *patch_side_effects = malloc(sizeof(Patch_Side_Effects));
  patch_side_effects->run_return_instruction = 0;
  patch_side_effects->skip_patched_instruction = 0;
  avr->patch_side_effects = patch_side_effects;

  // TODOE make this part of avr_t
  patched_instructions = NULL;
  // patch_instruction(0x87c, test_patch_function, avr);
  // patch_instruction(0x8fe, test_patch_function, avr);
  // patch_instruction(0x694, test_patch_function, avr);
  // patch_instruction(0x90e, test_reset, avr);

  // test2
  // patch_instruction(0x4e2, test_patch_function, avr);
  // patch_instruction(0x916, test_reset, avr);

  // deserializeJson

  patch_instruction(0x370c, override_args, avr);
  patch_instruction(0x37d4, test_reset, avr);
}

int patch_instruction(avr_flashaddr_t vaddr, void *function_pointer,
                      void *arg) {
  patched_instruction *p = get_or_create_patched_instruction(vaddr);

  DL_APPEND(p->function_patches, create_function_patch(function_pointer, arg));

  return 0;
}

patched_instruction *get_or_create_patched_instruction(avr_flashaddr_t key) {
  patched_instruction *entry;
  HASH_FIND_UINT32(patched_instructions, &key, entry);

  // Create empty list as value at :key: if no value exists yet
  if (entry == NULL) {
    // Set empty list as value for key target_vaddr in vaddr_hooks_table
    function_patch *patch = NULL;
    entry = malloc(sizeof(patched_instruction));
    entry->vaddr = key;
    entry->function_patches = patch;
    HASH_ADD_UINT32(patched_instructions, vaddr, entry);
  }

  return entry;
}

function_patch *create_function_patch(void *function, void *arg) {
  function_patch *entry = malloc(sizeof(function_patch));
  entry->function_pointer = function;
  entry->arg = arg;
  return entry;
}

int check_run_patch(avr_t *avr) {
  patched_instruction *patch;
  HASH_FIND_INT(patched_instructions, &(avr->pc), patch);
  if (patch != NULL) {
    function_patch *t;
    int max = 0;
    DL_FOREACH(patch->function_patches, t) {
      int (*s)() = t->function_pointer;
      int cur = (*s)(t->arg);
      if (cur > max)
        max = cur;
    }
    return max;
  }
  return 0;
}

void reset_patch_side_effects(avr_t *avr) {
  avr->patch_side_effects->skip_patched_instruction = 0;
  avr->patch_side_effects->run_return_instruction = 0;
}

int test_patch_function(void *arg) {
  ((avr_t *)arg)->patch_side_effects->skip_patched_instruction = 1;
  ((avr_t *)arg)->patch_side_effects->run_return_instruction = 1;
  printf("Hello from test_patch_function, arg: %ld\n", ((avr_t *)arg)->cycle);
  return 1;
}

int test_reset(void *arg) {
  avr_t *avr = (avr_t *)arg;
  printf("Resetting, arg: %ld\n", avr->cycle);
  evaluate_input(avr);
  generate_input(avr, avr->fuzzer);
  avr_reset(avr);
  return 0;
}

int override_args(void *arg) {
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
  return 0;
}