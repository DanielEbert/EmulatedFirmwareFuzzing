#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Limit the maximum length of seeds and generated inputs to MAX_INPUT_LENGTH
// bytes.
#define MAX_INPUT_LENGTH 4096

// The average number of mutations applied is (NUM_MUTATIONS / 2) ^ 2 to
// every generated input.
#define NUM_MUTATIONS 0 // TODOE back to 5

#ifdef __cplusplus
};
#endif