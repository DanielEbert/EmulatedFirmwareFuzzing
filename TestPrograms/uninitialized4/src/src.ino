
void setup() {
  Serial.begin(9600);
  myFunction();
}

int myFunction() {
  int *p;
  int a = *p;
  if (a > 4) return 4;
  return 6;
}

void loop(){}
