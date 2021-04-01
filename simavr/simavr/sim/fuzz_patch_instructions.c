
#include "fuzz_patch_instructions.h"
#include "sim_avr.h"
#include <stdio.h>

void initialize_patch_instructions(avr_t *avr) {
  patched_instructions = NULL;
  patch_instruction(0x87c, test_patch_function, avr);
}

int patch_instruction(avr_flashaddr_t vaddr, void *function_pointer, void *arg) {
  patched_instruction *p = get_or_create_patched_instruction(vaddr);

  DL_APPEND(p->function_patches, create_function_patch(function_pointer, arg));

  return 0;
}

patched_instruction* get_or_create_patched_instruction(avr_flashaddr_t key) {
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

function_patch* create_function_patch(void *function, void *arg) {
  function_patch *entry = malloc(sizeof(function_patch));
  entry->function_pointer = function;
  entry->arg = arg;
  return entry;
}

void check_run_patch(avr_t *avr) {
  patched_instruction *patch;
	HASH_FIND_INT(patched_instructions, &(avr->pc), patch);
	if (patch != NULL) {
		function_patch *t;
		DL_FOREACH(patch->function_patches, t) {
			void (*s)() = t->function_pointer;
			(*s)(t->arg);
		}
	}
}

void test_patch_function(void *arg) {
	printf("Hello from test_patch_function, arg: %ld\n", ((avr_t *)arg)->cycle);
}
