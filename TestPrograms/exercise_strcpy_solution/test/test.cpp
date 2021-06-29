#include <Arduino.h>
#include <unity.h>
#include "main.cpp"

char fuzz_input[256];
volatile uint16_t fuzz_input_length;

void test_process_input(void) {
  process_input(fuzz_input);
}

void setup() {
  Serial.begin(9600);
  UNITY_BEGIN();
  RUN_TEST(test_process_input);
  UNITY_END();
}

void loop(){}
