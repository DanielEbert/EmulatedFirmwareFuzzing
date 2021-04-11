
void setup() {
  Serial.begin(9600);
  myFunction();
}

void myFunction() {
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(18, INPUT_PULLUP);
  pinMode(19, INPUT_PULLUP);
  pinMode(20, INPUT_PULLUP);
  pinMode(21, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), myIRQ, CHANGE);
  attachInterrupt(digitalPinToInterrupt(3), myIRQ2, CHANGE);
  attachInterrupt(digitalPinToInterrupt(18), myIRQ3, CHANGE);
  attachInterrupt(digitalPinToInterrupt(19), myIRQ4, CHANGE);
  attachInterrupt(digitalPinToInterrupt(20), myIRQ5, CHANGE);
  attachInterrupt(digitalPinToInterrupt(21), myIRQ6, CHANGE);
}

void myIRQ() {
  Serial.println("triggered");
}

void myIRQ2() {
  Serial.println("triggered2");
}

void myIRQ3() {
  Serial.println("triggered3");
}
void myIRQ4() {
  Serial.println("triggered4");
}

void myIRQ5() {
  Serial.println("triggered5");
}
void myIRQ6() {
  Serial.println("triggered6");
}
void loop(){}
