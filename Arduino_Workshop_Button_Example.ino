// by Eleanor O'Callaghan, Spring 2022

const int LEDpin = 2; // LED is plugged into digital I/O pin 2
const int buttonPin = 3; // button is plugged into digital I/O pin 3

void setup() {
  pinMode(LEDpin, OUTPUT); // set LEDpin up as an output
  pinMode(buttonPin, INPUT); // set buttonPin up as an input
  Serial.begin(9600);
}

void loop() {
  // digitalRead(buttonPin) is 0 when pressed and 1 when not pressed
  if (!digitalRead(buttonPin)) { // if button is being pressed
    digitalWrite(LEDpin, HIGH); // turn LED on
    Serial.println("LED on");
  } else {
    digitalWrite(LEDPin, LOW); // turn LED off
    Serial.println("LED off");
  }
}
