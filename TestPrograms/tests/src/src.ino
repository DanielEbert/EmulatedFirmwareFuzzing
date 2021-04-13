
void setup() {
  Serial.begin(9600);
  myFunction();
}

int myFunction() {
  volatile int i = 23;
  i <<= 32;
  return i;
}

void loop(){}
