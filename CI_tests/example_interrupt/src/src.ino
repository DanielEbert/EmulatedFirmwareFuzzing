
void setup() {
  Serial.begin(9600);
  pinMode(19, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(19), my_interrupt_service_routine, CHANGE);
}

void my_interrupt_service_routine() {
  Serial.println("Handling IRQ.");
}

void wait_for_reset() {
}

void loop(){
  // Delay before reset so that the Serial has time to print the "Handling IRQ." string to terminal.
  delay(200);
  wait_for_reset();
}
