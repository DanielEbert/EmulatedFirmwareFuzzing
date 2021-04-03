#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Limit the maximum length of seeds and generated inputs to MAX_INPUT_LENGTH
// bytes.
const unsigned int MAX_INPUT_LENGTH = 4096 ;

// The average number of mutations applied is (NUM_MUTATIONS / 2) ^ 2 to
// every generated input.
const unsigned int NUM_MUTATIONS = 5;

#ifdef __cplusplus
};
#endif