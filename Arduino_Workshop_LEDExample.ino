// by Eleanor O'Callaghan, Spring 2022

const int LEDpin = 2; // LED is plugged into digital I/O pin 2

void setup() {
  pinMode(LEDpin, OUTPUT); // set LEDpin up as an output
  Serial.begin(9600);
}

void loop() {
  digitalWrite(LEDpin, HIGH); // turn LED on
  Serial.println("LED on");
  delay(2000); // keep LED on for 2 seconds

  digitalWrite(LEDpin, LOW); // turn LED off
  Serial.println("LED off");
  delay(1000); // keep LED off for 1 second
}
