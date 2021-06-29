#include <Arduino.h>
#include <unity.h>
#include <ArduinoJson.h>

void test_deserialize_and_serialize(void) {
  DynamicJsonDocument doc(256);
  const char *input = "{\"hello\":42}";
  DeserializationError ret = deserializeJson(doc, input, strlen(input));
  TEST_ASSERT(ret == DeserializationError::Ok);
  char buf[256];
  size_t num_bytes_written = serializeJson(doc, buf, 256);
  TEST_ASSERT_EQUAL_INT(strlen(input), num_bytes_written);
  TEST_ASSERT_EQUAL_STRING(input, buf);
}

void setup() {
  Serial.begin(9600);
  UNITY_BEGIN();
  RUN_TEST(test_deserialize_and_serialize);
  UNITY_END();
}

void loop(){}
