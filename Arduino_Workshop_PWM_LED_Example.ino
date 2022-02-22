// by Eleanor O'Callaghan, Spring 2022

const int LEDpin = 5; // LED plugged into digital I/O pin 5 (a PWM pin)
const int buttonPin = 3; // button plugged into digital I/O pin 3

float multiplier; // multiplier to set brightness of LED
int brightnessPWM; // value from 0-255 to send to LED with PWM signal

void setup() {
  Serial.begin(9600);
  pinMode(LEDpin, OUTPUT);
  pinMode(buttonPin, INPUT);
}

void loop() {
  multiplier = 0.0; // reset multiplier to 0 (LED off)
  Serial.println("Brightness = 0%");

  while (multiplier <= 1) { // while LED is between 0% and 100% brightness
    brightnessPWM = 255 * multiplier; // scale multiplier to 0-255
    analogWrite(LEDpin, brightnessPWM); // send PWM signal to LED

    if (digitalRead(3) == HIGH) { // if button is being pressed
      multiplier = multiplier + 0.1; // increment LED brightness by 10%
      Serial.print("Brightness = ");
      Serial.print(multiplier * 100, 0); // convert 0-1 to 0-100% and have 0 digits after the decimal
      Serial.println("%");
      delay(200); // make sure single press isn't double-counted
    }
  }
}
