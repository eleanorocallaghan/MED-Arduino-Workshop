// by Eleanor O'Callaghan and Sophie Dunn
// December 2021

// libraries and custom fonts for display
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <String.h>

// for remote to take user input
#include <IRremote.h>

// for saving and reading previous plantData array
#include <EEPROM.h>
#include <EEWrap.h>

enum States {startup, checkPlants, maintenance, refill};
States myState = startup;

// for color changing LED
enum colorOptions {red, green, blue, raspberry, cyan, magenta, yellow, white};
colorOptions color = white;


int16_e numPlants EEMEM; // will be saved if user loads previous setup
int plantData[4] = {0, 0, 0, 0}; // 1st index is plant #1 size,
// 2nd index is plant #1 water need,
// 3rd index is plant #2 size, etc.

// variables keeping track of interrupt button to requst data
volatile int buttonState = 1;
volatile unsigned long buttonTime = millis();

// naming pins used for sensors/displays/LED
#define WATERLEVEL A1 // reservoir water level sensor
#define PUMP1 4 // water pump for plant #1
#define PUMP2 5 // water pump for plant #2
#define MOISTURE1 A0 // soil moisture sensor for plant #1
#define MOISTURE2 A4 // soil moisture sensor for plant #2
#define redLight 35
#define greenLight 37
#define blueLight 39
#define BUTTON1 2 // button for user to request plant moisture data
#define RECV_PIN 7 // IR reciever

// set up IR reciever
IRrecv irrecv(RECV_PIN);
decode_results results;

Adafruit_SSD1306 display(128, 64);  // Create display

const int STARTING_EEPROM_ADDRESS = 17; // used to store plantData array
// for loading previous setup,
// seeds beginning storage location

void setup() {
  Serial.begin(9600);

  // set up input/output pins
  pinMode(WATERLEVEL, INPUT);
  pinMode(PUMP1, OUTPUT);
  pinMode(PUMP2, OUTPUT);
  pinMode(MOISTURE1, INPUT);
  pinMode(MOISTURE2, INPUT);
  pinMode(redLight, OUTPUT);
  pinMode(greenLight, OUTPUT);
  pinMode(blueLight, OUTPUT);
  pinMode(BUTTON1, INPUT_PULLUP);
  attachInterrupt(0, dataButton, RISING);

  // set up IR remote sensor
  // code from https://www.circuitbasics.com/arduino-ir-remote-receiver-tutorial/
  irrecv.enableIRIn();
  irrecv.blink13(true);

  // set up display
  // code from https://randomnerdtutorials.com/guide-for-oled-display-with-arduino/
  delay(100);  // This delay is needed to let the display to initialize
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // Initialize display with the I2C
  // address of 0x3C
  display.clearDisplay();  // Clear the buffer
  display.setTextColor(WHITE);  // Set color of the text
  display.setRotation(0);  // Set orientation. Goes from 0, 1, 2 or 3
  display.setTextWrap(true);  // By default, long lines of text are set to
  // automatically “wrap” back to the leftmost column.
  display.dim(0);  //Set brightness (0 is maximun and 1 is a little dim)
}

void loop() {

  // Finite state machine architecture that uses a switch...case system to check
  // which state the device is in and perform the required actions

  switch (myState) {

    // startup case is what device boots in to, allows user to enter data for
    // number of plants, plant size, required amount of water, etc.
    case startup: {
        LED(green);
        int previousOrNew = menu1(); // ask user if they want to load previous plant data or set up new

        switch (previousOrNew) {
          case 1: { // if user entered 1 in menu1 (load previous)
              // use helper function to pull data from EEPROM from previous setup
              readIntArrayFromEEPROM(STARTING_EEPROM_ADDRESS, plantData, 4);
              myState = checkPlants; // once data is loaded, begin checking on reservoir/plants
              break;
            }

          case 2: {// if user entered 2 in menu1 (start new setup)
              numPlants = menu2(); // ask user how many plants they would like to set up
              while (numPlants == 0) { //while user enters invalid inputs
                badInput(); // tell user their input was invalid
                numPlants = menu2(); // ask user to reenter a valid response
              }
              int curPlant = 1;
              int sizeIndex = 0;
              int waterIndex = 1;
              while (curPlant <= numPlants) { // while there are plants that need data to be entered

                // indexes for plantData array, every plant gets two pieces of info stored (size, required wetness)
                int sizeIndex = curPlant * 2 - 2;
                int waterIndex = curPlant * 2 - 1;
                plantData[sizeIndex] = menu3(curPlant); // ask user size of current plant, save data
                while (plantData[sizeIndex] == 0) { //while user enters invalid inputs
                  badInput(); // tell user their input was invalid
                  plantData[sizeIndex] = menu3(curPlant); // ask user to reenter a valid response
                }

                plantData[waterIndex] = menu4(curPlant); // ask user how much water plant needs
                while (plantData[waterIndex] == 0) { //while user enters invalid inputs
                  badInput(); // tell user their input was invalid
                  plantData[waterIndex] = menu4(curPlant); // ask user to reenter a valid response
                }
                curPlant++; // increment plant being set up, once it's equal to the total number, exit the while loop

              }
              writeIntArrayIntoEEPROM(STARTING_EEPROM_ADDRESS, plantData, 4);
              myState = checkPlants;
              break;
            }

          case 0: { //if user entered invalid input in first menu
              myState = startup; // restart setup routine
              break;
            }
        }
        break; // exit setup case
      }

    // checkPlants case checks the reservoir to make sure there is water, then checks the moisture of each plant, 
    // and waters them if necessary
    case checkPlants: {
        outputToScreen("Checking plants...");
        LED(magenta);
        int curPlant = 1;

        if (!checkWater()) { // if reservoir is empty
          myState = refill;
        } else {
          while (curPlant <= numPlants) { // while there are plants that need to be checked
            if (checkMoisture(curPlant)) { // if plant has enough moisture
              curPlant++; // check the next plant
            } else { // if plant needs to be watered
              water(curPlant);
              curPlant++; // check the next plant
            }
          }
          myState = maintenance; // if all plants are moist/have been watered, move to maintenance state
        }
        break; // exit checkPlants case
      }

    // maintenance case idles device, displaying the time until next checkPlants routine
    case maintenance: {

        // variables used to track how long it's been since last checkPlnats routine
        unsigned long initTime = millis();
        unsigned long curTime = millis();
        unsigned long timeToNextCheck = 0;
        unsigned long LEDTime = millis(); // to track time since last LED color change

        // Strings/char arrays that are concatenated to output to screen
        char message1[] = "Checking plants in ";
        char message2[] = " seconds";
        int LEDValue = red;
        int waitTime = 10000; // wait for this long before checking if plants need to be watered again
        char messageToSend[100];

        while ((curTime - initTime) < waitTime) { // while it has been less than the required 
                                                  // wait time until next checkPlants routine

          // if user has requested data update for plant moisture
          if ((buttonState == 0) && (millis() - buttonTime) > 200) { // button debouncing so state 
                                                                     // won't change unless it's been 
                                                                     // at least .2 seconds
            displayData();
            buttonState = 1; // reset button state
            delay(1000); // delay so user has time to read data
          }

          // calculate how long until next checkPlants routine and display to user
          curTime = millis();
          timeToNextCheck = ((initTime + waitTime) - curTime) / 1000; //in seconds
          if ((curTime - LEDTime) > 1000) {
            LEDTime = millis();
            LED(LEDValue);
            
            // concatenate string to output to user
            String message = message1 + String(timeToNextCheck) + message2;
            
            // convert to character array so it's appropriate for helper function
            message.toCharArray(messageToSend, 100);
            outputToScreen(messageToSend);

            // cycle through all LED colors, using enum defined above to increment using words
            if (LEDValue == white) { // if LED has gotten to last color in list
              LEDValue = red; // start from beginning
            } else {
              LEDValue++;
            }
          }
        }
        myState = checkPlants;
        break; // end of maintenance state
      }

    // refill state is triggered when reservoir is empty
    case refill: {
        LED(red); // warning LED
        outputToScreen("Please refill reservoir");

        // wait for user to refill reservoir
        while (1 < 2) {
          if (checkWater()) { // if water reservoir sensor says there is water
            break;
          }
          delay(1000); // check reservoir every 1 second
        }
        myState = checkPlants; // once reservoir has been refilled, go back to regular checkPlants routine
        break; // end of refill state
      }
  }
}


// __________________________________________HELPER FUNCTIONS__________________________________________
// the following functions are called by eachother and called in the void loop above, 
// used to make code more readable and not repeat commonly used sections of code

// outputToScreen takes character array as input and displays message to screen, 
// message remians on display until cleared or overwritten
// parts of code from https://randomnerdtutorials.com/guide-for-oled-display-with-arduino/
void outputToScreen(char message[]) {
  display.clearDisplay();  // Clear the display so we can refresh
  display.setFont(&FreeMono9pt7b);  // Set a custom font
  display.setTextSize(0);  // Set text size. We are using a custom font so you 
                           // should always use the text size of 0
  // Print text:
  display.setCursor(0, 10);  // (x,y)
  display.println(message);  // Text or value to print
  display.display();  // Print everything we set previously
}

// function called that allows code to be written with colors instead of having 
// to know RGB values for different colors
// stores RGB values for various colors
// keep red as first in list and white as last in list, if more colors are added, 
// add them in the middle of the list to not mess up color incrementation in maintenance routine
void LED(colorOptions color) {
  switch (color) {
    case red: LEDHelper(255, 0, 0); break; // keep this as first in list
    case green: LEDHelper(0, 255, 0); break;
    case blue: LEDHelper(0, 0, 255); break;
    case raspberry: LEDHelper(255, 255, 125); break;
    case cyan: LEDHelper(0, 255, 255); break;
    case magenta: LEDHelper(255, 0, 255); break;
    case yellow: LEDHelper(255, 255, 0); break;
    case white: LEDHelper(255, 255, 255); break; // keep this as last in list
    default: LEDHelper(0, 0, 0); break; // default state is no light
  }
}

// function called by LED function above that actually writes the RGB values to the LED
void LEDHelper(int red, int green, int blue) {
  analogWrite(redLight, red);
  analogWrite(greenLight, green);
  analogWrite(blueLight, blue);
}

// function to tell user that they entered an invalid input in the relevant menu
void badInput() {
  char message[] = "Invalid input, please try again.";
  outputToScreen(message);
  delay(3000); // let user read error message before clearing
  display.clearDisplay();
}

// reads IR remote inputs using IR reciever
// parts of code from https://www.circuitbasics.com/arduino-ir-remote-receiver-tutorial/
int readKeyInput(int minVal, int maxVal) {
  int valueEntered = 0;
  unsigned long key_value = 0;
  key_value = irrecv.decode();
  while (valueEntered == 0) {
    if (irrecv.decode()) {
      if (irrecv.decodedIRData.decodedRawData == 0XFFFFFFFF)
        irrecv.decodedIRData.decodedRawData = key_value;

      // currently only set up to read 1-4, if other buttons are pressed the device will not react
      switch (irrecv.decodedIRData.decodedRawData) {
        case 0xF30CFF00: {
            valueEntered = 1;
            break ;
          }
        case 0xE718FF00: {
            valueEntered = 2;
            break ;
          }
        case 0xA15EFF00: {
            valueEntered = 3;
            break ;
          }
        case 0xF708FF00: {
            valueEntered = 4;
            break ;
          }
      }
      key_value = results.value;
      irrecv.resume();
    }
  }

  if (valueEntered > maxVal || valueEntered < minVal) { // if outside of expected bounds
    badInput(); // tell user input was invalid
    return 0;
  } else { // if value entered is within expected bounds
    return valueEntered;
  }
}

// function to do math for how long to water for different sizes and water requirements
int calcWaterTime(int plantSize, int waterReq) { // returns time in milliseconds
  int waterTime = plantSize * waterReq * 1000; // this is a made up equation, 
                                               // we would need to do testing/plant 
                                               // research to come up with real numbers
  return waterTime; // number of seconds to pump motor
}

// reads data from water reservoir sensor
bool checkWater() {
  int waterLevel = analogRead(WATERLEVEL);
  int minWaterLevel = 500; // minimum sensor value that means reservoir is empty
  if (waterLevel < minWaterLevel) {
    return 0;
  } else {
    return 1;
  }
}

// reads data from moisture sensors in plant
bool checkMoisture (int plant) {
  float sensorValue = 0;
  float minMoistureValue = 1030; // 1033 is the value for completely dry
  switch (plant) {
    // take average sensor value over .1 seconds
    case 1: {
        for (int i = 0; i <= 100; i++) {
          sensorValue = sensorValue + analogRead(MOISTURE1);
          delay(1);
        }
        break;
      }
    case 2: {
        for (int i = 0; i <= 100; i++) {
          sensorValue = sensorValue + analogRead(MOISTURE2);
          delay(1);
        }
        break;
      }
  }
  sensorValue = sensorValue / 100.0;
  delay(30);

  if (sensorValue >= minMoistureValue) { // if plant moisture level is below minimum acceptable
    return 0;
  } else {
    return 1;
  }
}

// function turns on correct water pump for caluclated amount of time for plant
void water(int plant) {
  outputToScreen("Watering...");
  LED(cyan);
  // indexes to keep track of data for current plant
  int sizeIndex = plant * 2 - 2;
  int waterIndex = plant * 2 - 1;

  // calculate water time based on plant size and water requirement
  int waterTime = calcWaterTime(plantData[sizeIndex], plantData[waterIndex]);

  unsigned long initTime = millis();
  unsigned long curTime = millis();
  if (plant == 1) {
    while ((curTime - initTime) < waterTime) { // while it's been less than the 
                                               // required length of time to water plant
      digitalWrite(PUMP1, HIGH);
      curTime = millis();
    }
    digitalWrite(PUMP1, LOW);
  } else if (plant == 2) {
    while ((curTime - initTime) < waterTime) {
      digitalWrite(PUMP2, HIGH);
      curTime = millis();
    }
    digitalWrite(PUMP2, LOW);
  }
}

// first setup menu
int menu1() {
  char message[] = "Load previous setup (1) or start new setup (2)?";
  outputToScreen(message);
  int menuSelection = readKeyInput(1, 2); // 1 or 2 are the only valid answers
  if (menuSelection != 0) { // if readKeyInput says the value input was valid
    return menuSelection;
  } else {
    return 0; // invalid input
  }
}

// second setup menu
int menu2() {
  char message[] = "How many plants do you want to water (1 or 2)?";
  outputToScreen(message);
  int menuSelection = readKeyInput(1, 2); // 1 or 2 are the only valid answers
  if (menuSelection != 0) { // if readKeyInput says the value input was valid
    return menuSelection;
  } else {
    return 0;
  }
}

// third setup menu
int menu3(int plantNum) {
  String message = " ";
  char messageToDisplay[100];
  switch (plantNum) {
    case 1: {
        message = "Plant 1 size? 1 = sm, 2 = med, 3 = lg";
        break;
      }
    case 2: {
        message = "Plant 2 size? 1 = sm, 2 = med, 3 = lg";
        break;
      }
  }
  message.toCharArray(messageToDisplay, 100);
  outputToScreen(messageToDisplay);
  int menuSelection = readKeyInput(1, 3); // 1, 2, or 3 are the only valid answers
  if (menuSelection != 0) { // if readKeyInput says the value input was valid
    return menuSelection;
  } else {
    return 0;
  }
}

// fourth setup menu
int menu4(int plantNum) {
  String message = " ";
  char messageToDisplay[100];
  switch (plantNum) {
    case 1: {
        message = "Plant 1 wetness? 1 = not very, 2 = avg, 3 = very";
        break;
      }
    case 2: {
        message = "Plant 2 wetness? 1 = not very, 2 = avg, 3 = very";
        break;
      }
  }
  message.toCharArray(messageToDisplay, 100);
  outputToScreen(messageToDisplay);
  int menuSelection = readKeyInput(1, 3); // 1, 2, or 3 are the only valid answers
  if (menuSelection != 0) { // if readKeyInput says the value input was valid
    return menuSelection;
  } else {
    return 0;
  }
}

// interrupt service routine for data request button
void dataButton() {
  if (myState == maintenance) { // user can only request data if in maintenance mode
    buttonState = 0;
    buttonTime = millis(); // record when button was pressed, used for debouncing
  }
}

// same as checkMoisture, but returns actual sensor value instead of boolean, 
// used for displaying data to user
int getMoistureValue (int plant) {
  float sensorValue = 0;
  float dryMoistureValue = 1030; // 1033 is the value for completely dry
  float wetMoistureValue = 100; // anything between these values is moist

  switch (plant) {
    case 1: {
        for (int i = 0; i <= 100; i++) {
          sensorValue = sensorValue + analogRead(MOISTURE1);
          delay(1);
        }
        break;
      }
    case 2: {
        for (int i = 0; i <= 100; i++) {
          sensorValue = sensorValue + analogRead(MOISTURE2);
          delay(1);
        }
        break;
      }
  }
  sensorValue = sensorValue / 100.0;
  delay(30);

  if (sensorValue >= dryMoistureValue) {
    return 1;
  } else if (sensorValue <= wetMoistureValue) {
    return 2;
  } else {
    return 3;
  }
}

// function that writes data message and displays to user
void displayData() {
  int curPlant = 1;
  int moistureValue = 0;

  // strings and arrays to store message to be concatenated and displayed
  String plantMessages[2] = {"a", "a"};
  String message1 = "Plant ";
  String message2 = " is ";
  String messageBuffer = "";
  char messageToSend[200];
  char curPlantString[1] = {" "};

  while (curPlant <= numPlants) { // while there are plants that need to have their data checked
    moistureValue = getMoistureValue(curPlant);
    switch (moistureValue) {
      case 1: { //dry
          plantMessages[curPlant - 1] = "dry ";
          break;
        }
      case 2: { //wet
          plantMessages[curPlant - 1] = "wet ";
          break;
        }
      case 3: { //medium
          plantMessages[curPlant - 1] = "moist ";
          break;
        }
    }
    messageBuffer += message1 + itoa(curPlant, curPlantString, 10) + message2 + plantMessages[curPlant - 1];
    curPlant++;
  }
  messageBuffer.toCharArray(messageToSend, 200);
  outputToScreen(messageToSend);
}

// function that writes each array value to EEPROM to be called when user wants to use previous setup
// parts of code from https://roboticsbackend.com/arduino-store-array-into-eeprom/
void writeIntArrayIntoEEPROM(int address, int numbers[], int arraySize) {
  int addressIndex = address;
  for (int i = 0; i < arraySize; i++) {
    EEPROM.write(addressIndex, numbers[i] >> 8);
    EEPROM.write(addressIndex + 1, numbers[i] & 0xFF);
    addressIndex += 2;
  }
}

// function that reads each array value to EEPROM for when user wants to use previous setup
// parts of code from https://roboticsbackend.com/arduino-store-array-into-eeprom/
void readIntArrayFromEEPROM(int address, int numbers[], int arraySize) {
  int addressIndex = address;
  for (int i = 0; i < arraySize; i++) {
    numbers[i] = (EEPROM.read(addressIndex) << 8) + EEPROM.read(addressIndex + 1);
    addressIndex += 2;
  }
}
