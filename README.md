# Paper Notes

## Design

### Patch Instructions

- https://gcc.gnu.org/wiki/avr-gcc Section Calling Convention
  - MSB is most significant byte, LSB is least ...

### Sanitizer

- AVR_STACK_WATCH did not work if pointer is passed to function. e.g. to struct or class and code does sth like s->a = 1; so what i needed to do is to only check if e.g. the return pointer is overriden. like sth that we really do not want to override. => Implemented via avr->stack_return_address, can talk about 'rcall .1 is used as a cheap "push 16 bits of room on the stack" special case

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

unique crashes. can do full stack trace (bad), or pc at crash
on crash: reset

## patches

## Sanitizers

- uninit testing in \_avr_get_ram maybe. still not sure if it works anyway. low prio probably
- callback on malloc to look for heap out-of-bounds?. or i can say that malloc not used often in embedded or arduino so we dont care for it. malloc_heap_start and \_end show
  - ! \_end is set to 0 which which makes malloc() assume the heap is below the stack. its actually a higher number than \_start
  - \_\_malloc_margin will be considered if the heap is operating below the stack
  - https://www.nongnu.org/avr-libc/user-manual/malloc.html shows RAM map
- can we error on read and writes below stack pointer? if malloc is used it think this wont work. can we check if
- TODO: find tests that catch these, and for implemented sanitizers: send input and reason; and reset
- check what 'sanitizers' gamozo uses
- we can reuse 'avr_core_watch_read' and ...write, also check AVR_STACK_WATCH and enable it by default. say avr_stack_watch for buffer overflows, and make test for that. simple strcpy
- can i check if we call uninitialized code? will likely be handled by invalid opcode i assume. can write that this is an option tought, but i didnt think its worth the time
- load from data space before initialized? also there is the typical for ... != END
- check most common errors (esp. memory errors) and check if we can catch them. check what sanitizers check for. i cant check for heap overflow currenly (if heap exists at all)
- check fuzzer most found from google
- check integer overflow (add instr? how is sanitizer for that implemented?)
- invalid frees? use the built in symbol stuff to figure out vaddr of malloc and free?

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
