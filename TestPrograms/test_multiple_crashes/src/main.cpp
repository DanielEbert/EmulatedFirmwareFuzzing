#include <Arduino.h>

int crash1() {
  int (*foo)(void) = (int (*)())0xdeadbeef;
	return foo(); // *crash*
}

int crash2() {
  int (*foo)(void) = (int (*)())0xdeadbeea;
	return foo(); // *crash*
}

int crash3() {
  int (*foo)(void) = (int (*)())0xdeadbeeb;
	return foo(); // *crash*
}

int x(char *fuzz_input, uint16_t fuzz_input_size) {
	if (fuzz_input_size == 0) return;

	if (fuzz_input[0] < (int)'a') crash1();
	else if (fuzz_input[0] < (int)'g') crash2();
	else if (fuzz_input[0] < (int)'y') crash3();
}
