#include <Arduino.h>

int process_input(char *input) {
  char buffer[40]; 
  strcpy(buffer, input);
  Serial.println(buffer);
  return 0;
}
