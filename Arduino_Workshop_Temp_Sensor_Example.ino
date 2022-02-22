// by Dr. Marra from EN.530.116 Spring 2022 MechE Freshman Lab II
// modified by Eleanor O'Callaghan

//Include library for TC board (this library includes custom functions that we'll use)
#include "max6675.h"

// thermocouple uses a communication prptocol called SPI (serial peripheral interface)
// SPI required 3 connections to Arduino
const int SO = 8; //Looks like "90" on the TC board
const int CS = 9;
const int CLK = 10; //"SCK" on the TC board

float degreeCelsius;

const int redLEDPin = 6;
float redLEDVoltage;
int redLEDPWMOutput; //Voltage level between 0 (0v) and 255 (5v)

const int blueLEDPin = 5;
float blueLEDVoltage;
int blueLEDPWMOutput; //Voltage level between 0 (0v) and 255 (5v)

MAX6675 thermoCouple(CLK, CS, SO); // set up thermocouple and tell Arduino which pins are in use
// Note: this line means that we don't need to use pinMode() to set up the thermocouple connections

void setup() {
  Serial.begin(9600);
  delay(500); // give the MAX a little time to settle
  pinMode(redLEDPin, OUTPUT);
  pinMode(blueLEDPin, OUTPUT);
}

void loop() {
  degreeCelsius = thermoCouple.readCelsius(); // Read the temperature in Celsius
  blueLEDVoltage = 0.0;
  redLEDVoltage = 0.0;

  if (degreeCelsius < 75) { //
    blueLEDVoltage = (-5. / 75.) * degreeCelsius + 5.0;
    // blue LED should decrease linearly from full brightness to
    // off for 0-75 deg C, then shut off completely for 75+ deg C
  }

  if (degreeCelsius > 25) {
    redLEDVoltage = (5. / 75.) * degreeCelsius - 5.0 / 3.0;
    // blue LED should decrease linearly from full brightness to
    // off for 0-75 deg C, then shut off completely for 75+ deg C
  }

  // convert 0-5v to 0-255 PWM
  blueLEDPWMOutput = 255. * blueLEDVoltage / 5.;
  redLEDPWMOutput = 255. * redLEDVoltage / 5.;

  analogWrite(redLEDPin, redLEDPWMOutput);
  analogWrite(blueLEDPin, blueLEDPWMOutput);

  Serial.print(degreeCelsius);
  Serial.print("\t");
  Serial.print(redLEDPWMOutput);
  Serial.print("\t");
  Serial.print(blueLEDPWMOutput);
  Serial.println();

  delay(200); //Don't decrease this, or board may not work
}
