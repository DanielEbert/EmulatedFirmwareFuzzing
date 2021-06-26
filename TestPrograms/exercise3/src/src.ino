#include <ArduinoJson.h>
#include <assert.h>

volatile char fuzz_input[256];
volatile size_t fuzz_input_length;

void execute_me() {
  Serial.println("You win!");
  void (*foo)(void) = (void (*)())0xdeadbeef;
  foo(); // *crash*
}

void parse(char *input, size_t input_length) {
  if (input_length >= 6 &&
      input[0] == 'H' &&
      input[1] == 'e' &&
      input[2] == 'l' &&
      input[3] == 'l' &&
      input[4] == 'o' &&
      input[5] == '!') {
    execute_me();
  }
}

void setup() {
  Serial.begin(9600);
}

void loop() {
  parse(fuzz_input, fuzz_input_length);
};
