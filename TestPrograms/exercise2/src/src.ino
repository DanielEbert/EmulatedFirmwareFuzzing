#include <ArduinoJson.h>
#include <assert.h>

char fuzz_input[256];
uint16_t fuzz_input_length;

void execute_me() {
  Serial.println("You win!");
  void (*foo)(void) = (void (*)())0xdeadbeef;
  foo(); // *crash*
}

void setup() {
  Serial.begin(9600);
  if (fuzz_input_length >= 6 &&
      fuzz_input[0] == 'H' &&
      fuzz_input[1] == 'e' &&
      fuzz_input[2] == 'l' &&
      fuzz_input[3] == 'l' &&
      fuzz_input[4] == 'o' &&
      fuzz_input[5] == '!') {
    execute_me();
  }
}

void loop() {};
