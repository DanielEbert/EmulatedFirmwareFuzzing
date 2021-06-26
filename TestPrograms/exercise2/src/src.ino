#include <ArduinoJson.h>
#include <assert.h>

char fuzz_input[256];
uint16_t fuzz_input_length;

void execute_me() {
  Serial.println("You win!");
}

void setup() {
  Serial.begin(9600);
  if (fuzz_input_length >= 3 &&
      fuzz_input[0] == 'H' &&
      fuzz_input[1] == 'i' &&
      fuzz_input[2] == '!') {
    execute_me();
  }
}

void loop() {};
