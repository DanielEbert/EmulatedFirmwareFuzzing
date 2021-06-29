#include <Arduino.h>
#include <unity.h>
#include "main.cpp"

volatile char fuzz_input[256];
volatile uint16_t fuzz_input_length;

void test_x(void) {
  x(fuzz_input, fuzz_input_length);
}

void setup() {
  Serial.begin(9600);
  UNITY_BEGIN();
  RUN_TEST(test_x);
  UNITY_END();
}

void loop(){}
