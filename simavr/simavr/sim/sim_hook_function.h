#ifndef __SIM_HOOK_FUNCTION__
#define __SIM_HOOK_FUNCTION__

#include "sim_avr_types.h"
#include "sim_uthash.h"
#include "sim_utlist.h"

#ifdef __cplusplus
extern "C" {
#endif

struct function_hook {
	void* hook_function_pointer;
	struct function_hook *next, *prev;
};

struct vaddr_hook { 
  int vaddr;            /* we'll use this field as the key */
	struct function_hook* function_hooks;    
  UT_hash_handle hh; /* makes this structure hashable */
};

// Global Hash Table
// Key: virtual address
// Value: list of function pointers
struct vaddr_hook *vaddr_hooks_table;

void initialize_function_hooks(struct avr_t *);
int add_function_hook(int vaddr, void* function_pointer);
struct vaddr_hook* get_or_create_hooks_table_value(int key);
struct function_hook* create_function_hook(void* function);
void test_hook_function();

#ifdef __cplusplus
};
#endif

#endif // __SIM_HOOK_FUNCTION__ 
