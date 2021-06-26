#include <Arduino.h>

int execute_me() {
  Serial.println("You win!");
  int (*foo)(void) = (int (*)())0xdeadbeef;
  return foo(); // *crash*
}

int parse(char *input, size_t input_length) {
  if (input_length >= 6 &&
      input[0] == 'H' &&
      input[1] == 'e' &&
      input[2] == 'l' &&
      input[3] == 'l' &&
      input[4] == 'o' &&
      input[5] == '!') {
    return execute_me();
  }
  return 0;
}
