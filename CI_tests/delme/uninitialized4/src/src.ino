
void setup() {
  Serial.begin(9600);
  volatile int i = myFunction();
}

int myFunction() {
  volatile int *p;
  p += 1;
  volatile int a = *p;
  if (a > 4) return 4;
  return 6;
}

void loop() {
  // We need a volatile variable and an assignment because otherwise loop()
  // is not called due to compiler optimizations.
  volatile int call_loop = 0;
};
