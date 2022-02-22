// by Dr. Brown from EN.530.421 Spring 2022 Mechatronics
// modified by Eleanor O'Callaghan

#include <Servo.h>

const int PingSignal = 8; // plug Ping signal wire into digital I/O pin 8
int distance;
unsigned long pulseduration = 0;

void setup() {
  pinMode(PingSignal, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  // call measureDistance() function, will set pulseDuration
  // equal to how long it took to recieve returned signal
  measureDistance();

  // use linear fit to convert pulseDuration to distance
  // this fit was created by manually calibrating and plotting
  // the relationship between pluseDuration and the measured
  // distance of an object
  distance = 0.0272 * pulseduration - 2.65664;

  Serial.print("Distance = ");
  Serial.print(distance);
  Serial.println(" cm");
  delay(500);
}

// this function will use the Ping sensor to send a pulse,
// wait for it to bounce off of the object whose distance
// is being measured, and then read the returning signal
void measureDistance() {
  pinMode(PingSignal, OUTPUT);  // set pin as output so we can send a pulse

  // set output to LOW
  digitalWrite(PingSignal, LOW);
  delayMicroseconds(5);

  // now send the 5uS pulse out to activate Ping
  digitalWrite(PingSignal, HIGH);
  delayMicroseconds(5);
  digitalWrite(PingSignal, LOW);

  // now we need to change the digital pin
  // to input to read the incoming pulse
  pinMode(PingSignal, INPUT);

  // finally, measure the length of the incoming pulse
  pulseduration = pulseIn(PingSignal, HIGH);
}
