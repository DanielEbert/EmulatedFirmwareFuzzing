#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// This source code is from https://en.wikipedia.org/wiki/Xorshift

// The same seed is used in every run to make individual fuzzing runs consistent
// and comparable.
// After developing this fuzzer, one can use a random seed every time,
// e.g. via rand().
uint64_t _rng_state = UINT64_C(0x10e0072601d190e);

uint64_t fast_random() {
	_rng_state ^= _rng_state >> 12; // a
	_rng_state ^= _rng_state << 25; // b
	_rng_state ^= _rng_state >> 27; // c
	return _rng_state * UINT64_C(0x2545F4914F6CDD1D);
} 

#ifdef __cplusplus
};
#endif