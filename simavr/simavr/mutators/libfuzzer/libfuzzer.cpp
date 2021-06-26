#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
//#include "config.h"
//#include "debug.h"

#include "sim_avr.h"
#include "fuzz_config.h"
#include <stdio.h>

extern "C" size_t LLVMFuzzerMutate(uint8_t *Data, size_t Size, size_t MaxSize);
extern "C" void   LLVMFuzzerMyInit(int (*UserCb)(const uint8_t *Data,
                                               size_t         Size),
                                   unsigned int Seed);

extern "C" int dummy(const uint8_t *Data, size_t Size) {

  fprintf(stderr, "dummy() called\n");
  (void)(Data);
  (void)(Size);
  fprintf(stderr, "dummy() called\n");
  return 0;

}

extern "C" void initialize_mutator(unsigned int seed) {
  LLVMFuzzerMyInit(dummy, seed);
}

extern "C" uint32_t mutator_mutate(Input *input, size_t max_input_length) {
  uint32_t ret = LLVMFuzzerMutate((uint8_t *)input->buf, input->buf_len, max_input_length);
  return ret;
}

