
void setup() {
  Serial.begin(9600);
  myFunction();
}

void myFunction() {
  pinMode(2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), myIRQ, CHANGE);
}

void myIRQ() {
  Serial.println("triggered");
}

void loop(){}
