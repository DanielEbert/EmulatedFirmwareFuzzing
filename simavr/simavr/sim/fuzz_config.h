#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Limit the maximum length of seeds and generated inputs to MAX_INPUT_LENGTH
// bytes.
#define MAX_INPUT_LENGTH 4096 // TODO: use this, include in patches

// The average number of mutations applied is (NUM_MUTATIONS / 2) ^ 2 to
// every generated input.
#define NUM_MUTATIONS 5

#ifdef __cplusplus
};
#endif