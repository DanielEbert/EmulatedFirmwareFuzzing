
void setup() {
  Serial.begin(9600);
  myFunction();
}

void __attribute__ ((noinline)) tmp() {
  volatile int i;
  Serial.println(F("tmp"));
}

void myFunction() {
  char buffer[5];
  tmp();
  strcpy(buffer, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
  Serial.println(buffer);
}

void loop(){}
