#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// The average number of mutations applied is (NUM_MUTATIONS / 2) ^ 2 to
// every generated input.
// Note to readers: LibFuzzer does not use this. This is only for the 'fallback'
// solution (i.e. mutator) if libFuzzer linking fails (typically libFuzzer
// linking does not fail).
#define NUM_MUTATIONS 5

#ifdef __cplusplus
};
#endif