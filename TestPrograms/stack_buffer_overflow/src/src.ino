
void setup() {
  Serial.begin(9600);
  myFunction();
}

void myFunction() {
  char buffer[5];
  strcpy(buffer, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
  Serial.println(buffer);
}

void loop(){}
