# Paper Notes

## Design

### Patch Instructions

- https://gcc.gnu.org/wiki/avr-gcc Section Calling Convention
  - MSB is most significant byte, LSB is least ...

### Sanitizer

- invalid opcode is decoded: invalid opcode is decoded
- avr_core_watch_read and \_write +
- bad instruction reading past the end of the flash

- detecting uninitialized memory requires compiler instrumentation. https://static.googleusercontent.com/media/research.google.com/de//pubs/archive/43308.pdf Do they mean that we need the original src code? Detecting this seems difficult.
  - actually its not difficult but the program can copy around uninitialised memory. https://github.com/rantoniello/valgrind/blob/21be3371beb74cff971e0bbf48fbecc7bf970e49/memcheck/docs/mc-manual.xml#L153 so thats the reason why we might not do it
  - with realloc for example we might also copy uninitialized data. i can give this as an example why we didnt do it. and also what they say in valgrind src comment
- also talk about what happens when a crash is detected

### Coverage

### server.py

- the client side, e.g. how the information from emulator (think Edge struct, ...) is used to provide visual feedback to the user about the current status of the fuzzing campagne. Say that we want to let the user know whats going on. "Fuzzer is doing stuff", and not fuzzer is stuck early and doesnt reach deep parts or interesting parts of the sut

# TODO

- i need to use \_avr_set_ram if i want callbacks. sometimes iwant it, sometimes not
- package server as service and autostart on OS start

## patches

## Sanitizers

- uninit testing in \_avr_get_ram maybe. still not sure if it works anyway. low prio probably

## Visualization

https://github.com/andreafioraldi/FuzzSplore/tree/master/report/img

- coverage growth (# edges)

# TOFIX

# Notes

- gcc: -O3 and then after that pass the fno-... otherwise they are overwritten by -O
- avr->interrupts.vector[i] to digitalPinToInterrupt(x):
  - v[6] is digitalPinToInterrupt(2)
  - v[7] is digitalPinToInterrupt(3)
  - v[2] is digitalPinToInterrupt(21)
  - v[3] is digitalPinToInterrupt(20)
  - v[4] is digitalPinToInterrupt(19)
  - v[5] is digitalPinToInterrupt(18)
    > > TODO: make a function digitalPinToInterrupt that does this conversion
- write about possible solutions to student challenges
