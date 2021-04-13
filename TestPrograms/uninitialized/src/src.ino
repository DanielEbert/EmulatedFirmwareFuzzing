
void setup() {
  Serial.begin(9600);
  int ret = myFunction();
  Serial.println((String)ret);
}

int myFunction() {
  int c;
  volatile int a[10];
  if (rand()) c = 1;
  return c2(c);
}
int c2(int a) {
return a;
}


void loop(){}
