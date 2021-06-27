#include <Arduino.h>
#include <unity.h>
#include "main.cpp"

void test_add(void) {
  TEST_ASSERT_EQUAL_INT(4, add(1, 3));
}

void setup() {
  Serial.begin(9600);
  UNITY_BEGIN();
  RUN_TEST(test_add);
  UNITY_END();
}

void loop(){}
