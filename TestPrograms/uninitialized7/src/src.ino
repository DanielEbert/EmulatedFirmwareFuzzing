
void setup() {
  Serial.begin(9600);
  myFunction();
}

void myFunction() {
  int a[10];
  if (x(a[4]) > 10) Serial.println("XXXX");
  else Serial.println("AAAA");
}

int x(int i) {
  if (i > 1) return i + 3;
  return i + 5;
}

void loop(){}
