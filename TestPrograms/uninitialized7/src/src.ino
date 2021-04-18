
void setup() {
  Serial.begin(9600);
  setShadow();
  myFunction();
}

void setShadow() {
  int a[200];
  for (int i = 0; i < 200; i++) {
    a[i] = i;
  }
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
