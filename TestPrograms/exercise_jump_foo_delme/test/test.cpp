#include <Arduino.h>
#include <unity.h>
#include "main.cpp"

char fuzz_input[256];
// We need volatile here, because otherwise the compiler optimizes this
// value out.
volatile uint16_t fuzz_input_length;

void test_parse(void) {
  TEST_ASSERT_EQUAL_INT(0, parse(fuzz_input, fuzz_input_length));
}

void setup() {
  Serial.begin(9600);
  UNITY_BEGIN();
  RUN_TEST(test_parse);
  UNITY_END();
}

void loop(){}
