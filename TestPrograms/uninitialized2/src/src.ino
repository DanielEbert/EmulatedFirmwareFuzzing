
void setup() {
  Serial.begin(9600);
  int ret = myFunction();
  Serial.println(ret, DEC);
}

int myFunction() {
  int c = 3;
  volatile int a[10];
  if (a[5] == 30) c = 1;
  return c;
}
int c2(int a) {
return a;
}


void loop(){}
