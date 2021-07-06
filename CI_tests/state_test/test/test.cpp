#include <Arduino.h>
#include <unity.h>
#include <ArduinoJson.h>

volatile char fuzz_input[256];
volatile uint16_t fuzz_input_length;

volatile uint32_t fuzz_state;

void test_state(void) {
  fuzz_state = (uint8_t)fuzz_input[0];
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_state);
  UNITY_END();
}

void loop(){}
