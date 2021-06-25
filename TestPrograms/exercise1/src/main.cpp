#include <Arduino.h>

int process_input(char *input) {
  char buffer[40]; 
  strcpy(buffer, "Hello World!\n");
  Serial.println(buffer);
  return 1;
}
