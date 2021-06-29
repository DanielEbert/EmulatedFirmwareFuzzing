#include <Arduino.h>
#include <unity.h>
#include <ArduinoJson.h>

volatile char fuzz_input[256];
volatile uint16_t fuzz_input_length;

void test_deserialize_and_serialize(void) {
  DynamicJsonDocument doc(256);
  DeserializationError ret = deserializeJson(doc, (char *)fuzz_input, fuzz_input_length);
  if (!ret) {
    char buf[256];
    size_t ret2 = serializeJson(doc, buf, 256);
    assert(ret2 <= 256);
  }
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_deserialize_and_serialize);
  UNITY_END();
}

void loop(){}
