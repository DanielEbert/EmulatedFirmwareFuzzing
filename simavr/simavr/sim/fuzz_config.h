#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// The average number of mutations applied is (NUM_MUTATIONS / 2) ^ 2 to
// every generated input.
// Note: LibFuzzer does not use this.
#define NUM_MUTATIONS 5

#ifdef __cplusplus
};
#endif