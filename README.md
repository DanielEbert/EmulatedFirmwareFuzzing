# Paper Notes

### Patch Instructions

- https://gcc.gnu.org/wiki/avr-gcc Section Calling Convention
  - MSB is most significant byte, LSB is least ...

# TODO

- i need to use \_avr_set_ram if i want callbacks. sometimes iwant it, sometimes not
- package server as service and autostart on OS start

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
