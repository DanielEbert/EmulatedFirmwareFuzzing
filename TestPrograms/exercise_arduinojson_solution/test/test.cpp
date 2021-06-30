#include <Arduino.h>
#include <unity.h>
#include <ArduinoJson.h>

char fuzz_input[256];
volatile uint16_t fuzz_input_length;

void test_deserialize_and_serialize(void) {
  DynamicJsonDocument doc(256);
  DeserializationError ret = deserializeJson(doc, fuzz_input, fuzz_input_length);
  if (ret != DeserializationError::Ok) {
    // We could optionally also remove this DeserializationError::Ok check to se
    // how serializeJson handles DynamicJsonDocument with errors.
    return;
  }
  char buf[256];
  size_t num_bytes_written = serializeJson(doc, buf, 256);
  // We cannot test 
  // TEST_ASSERT_EQUAL_STRING(fuzz_input, buf); or
  // TEST_ASSERT_EQUAL_INT(strlen(fuzz_input), num_bytes_written);
  // here, because for example if you deserialize and serialize the string '00', 
  // the result is '0' because the string is interpreted as a number.
  // In other words, the deserializeJson 'function' is surjective.
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_deserialize_and_serialize);
  UNITY_END();
}

void loop(){}
