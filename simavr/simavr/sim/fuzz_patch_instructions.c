
#include "fuzz_patch_instructions.h"
#include "sim_avr.h"
#include <stdio.h>

void initialize_patch_instructions(avr_t *avr) {
  patched_instructions = NULL;
  patch_instruction(0x87c, test_patch_function);
}

int patch_instruction(avr_flashaddr_t vaddr, void* function_pointer) {
  patched_instruction *p = get_or_create_patched_instruction(vaddr);

  DL_APPEND(p->function_patches, create_function_patch(function_pointer));

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

function_patch* create_function_patch(void* function) {
  function_patch *entry = malloc(sizeof(function_patch));
  entry->function_pointer = function;
  return entry;
}

void test_patch_function() {
	printf("Hello from test_patch_function\n");
}
//struct vaddr_hook *hook_at_vaddr; 
//	int target_vaddr = 0x87c;
//	HASH_FIND_INT(vaddr_hooks_table, &target_vaddr, hook_at_vaddr);
//
//	// TODOE this ofc runs every instr. put this in setup. check workflow for where
//	// Create List Element for hook function
//	struct function_hook *new_function_hook = malloc(sizeof(struct function_hook));
//	new_function_hook->hook_function_pointer = test_hook_function;
//
//	// Append hook function to list
//	DL_APPEND(hook_at_vaddr->function_hooks, new_function_hook);
//
//	hook_at_vaddr = NULL;
//	HASH_FIND_INT(vaddr_hooks_table, &(avr->pc), hook_at_vaddr);
//	if (hook_at_vaddr != NULL) {
//		int count = 0;
//		struct function_hook *t;
//		DL_COUNT(hook_at_vaddr->function_hooks, t, count);
//		printf("RUN %d\n", count);
//		DL_FOREACH(hook_at_vaddr->function_hooks, t) {
//			//void (*s)() = t->hook_function_pointer;
//			//(*s)();
//		}
//	}