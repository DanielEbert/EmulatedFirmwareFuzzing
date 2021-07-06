#include <ArduinoJson.h>
#include <assert.h>

char fuzz_input[256];
// 'volatile', otherwise compiler optimizes it out
volatile uint16_t fuzz_input_length;

void setup() {
  DynamicJsonDocument doc(256);
  DeserializationError ret = deserializeMsgPack(doc, fuzz_input, fuzz_input_length);
  if (!ret) {
    char buf[256];
    size_t ret2 = serializeMsgPack(doc, buf, 256);
    assert(ret2 <= 256);
  }
  //Serial.print("Y\n");
  //delay(100);
}

void loop() {
  // We need a volatile variable and an assignment because otherwise loop()
  // is not called due to compiler optimizations.
  volatile int call_loop = 0;
};
