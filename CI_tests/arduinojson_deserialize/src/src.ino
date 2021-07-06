#include <ArduinoJson.h>
#include <assert.h>

char fuzz_input[256];
// 'volatile', otherwise compiler optimizes it out
uint16_t fuzz_input_length;

void setup() {
  //Serial.begin(9600);
  DynamicJsonDocument doc(256);
  // TODO: doc says fuzz_input should be zero terminated
  DeserializationError ret = deserializeJson(doc, fuzz_input, fuzz_input_length);
  if (!ret) {
    //Serial.print("X\n");
    char buf[256];
    size_t ret2 = serializeJson(doc, buf, 256);
    assert(ret2 <= 256);
  }
  //Serial.print("Y\n");
  //delay(100);
}

void loop() {};
