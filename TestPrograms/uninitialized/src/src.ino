
void setup() {
  Serial.begin(9600);
  int a = random();
  int ret = myFunction(a);
  Serial.println((String)ret);
}

int myFunction(int a) {
  int counter = a;
  if (counter == 0) {
    return 1;
  }
  return 0;
}

void loop(){}
