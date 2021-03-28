
#include "sim_hook_function.h"
#include "sim_avr.h"
#include <stdio.h>

// TODOE do i even need :avr: here?
void initialize_function_hooks(avr_t * avr) {
  vaddr_hooks_table = NULL;
  add_function_hook(0x87c, test_hook_function);
}

int add_function_hook(int vaddr, void* function_pointer) {
  struct vaddr_hook *hook_table_value_at_vaddr = get_or_create_hooks_table_value(vaddr);

  DL_APPEND(hook_table_value_at_vaddr->function_hooks, create_function_hook(function_pointer));

  return 0;
}

struct vaddr_hook* get_or_create_hooks_table_value(int key) {
  struct vaddr_hook *entry;
  HASH_FIND_INT(vaddr_hooks_table, &key, entry);

  // Create empty list as value at :key: if no value exists yet
	if (entry == NULL) {
		// Set empty list as value for key target_vaddr in vaddr_hooks_table
		struct function_hook *function_hook_list = NULL;
		entry = malloc(sizeof(struct vaddr_hook));
		entry->vaddr = key;
		entry->function_hooks = function_hook_list;
		HASH_ADD_INT(vaddr_hooks_table, vaddr, entry);
	}

  return entry;
}

struct function_hook* create_function_hook(void* function) {
  struct function_hook *entry = malloc(sizeof(struct function_hook));
  entry->hook_function_pointer = function;
  return entry;
}

void test_hook_function() {
	printf("Hello from test_hook_function\n");
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