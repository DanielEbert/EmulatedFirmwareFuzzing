#include <Arduino.h>
#include <unity.h>
#include "main.cpp"

void test_process_input(void) {
  char *input = (char *)"Hi";
  TEST_ASSERT_EQUAL(1, process_input(input));
}

void setup() {
  Serial.begin(9600);
  UNITY_BEGIN();
  RUN_TEST(test_process_input);
  UNITY_END();
}

void loop(){}
