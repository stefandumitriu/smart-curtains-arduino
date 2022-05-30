#include <IRremote.hpp>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <BH1750.h>

#define AUTOMATIC_MODE 0
#define MANUAL_MODE 1
#define SETUP_MODE 2
#define IR_RECEIVE_PIN 6
#define MICROSWITCH_PIN 7
#define STEPPER_PIN_1 9
#define STEPPER_PIN_2 10
#define STEPPER_PIN_3 11
#define STEPPER_PIN_4 12
#define ONE_ROTATION_STEPS 512
#define FULL_STROKE_STEPS 10000
#define LIGHT_SENSOR_PIN 3

int step_number;
int mode;
int isOpen;
long lastTimeLCDUpdated;
int lastTimeRemotePressed;
int stepsToFullOpen;

LiquidCrystal_I2C lcd(0x27, 16, 2);
BH1750 lightMeter(0x23);

void setup() {
  Serial.begin(9600);
  
  lcd.init();
  lcd.clear();
  lcd.backlight();
  lcd.setCursor(0,0);
  pinMode(LIGHT_SENSOR_PIN, INPUT);
  pinMode(STEPPER_PIN_1, OUTPUT);
  pinMode(STEPPER_PIN_2, OUTPUT);
  pinMode(STEPPER_PIN_3, OUTPUT);
  pinMode(STEPPER_PIN_4, OUTPUT);
  pinMode(MICROSWITCH_PIN, INPUT_PULLUP);
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  // search the I2C devices addresses
  searchAddress();
  // bring curtain to closed position (default)
  initRoutine();
  step_number = 0;
  lastTimeLCDUpdated = 0;
  lastTimeRemotePressed = 0;
  mode = AUTOMATIC_MODE;
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println(F("BH1750 Advanced begin"));
  }
  else {
    Serial.println(F("Error initialising BH1750"));
  }
  lcd.print("Setup done!");
}

void loop() {
  catchRemoteCommand();
  changeStateCurtain();
}

// function to execute one step on the motor (cycle the HIGH signal through the input pins)
void OneStep(bool dir){
  if(dir) {
    switch(step_number){
      case 0:
      digitalWrite(STEPPER_PIN_1, HIGH);
      digitalWrite(STEPPER_PIN_2, LOW);
      digitalWrite(STEPPER_PIN_3, LOW);
      digitalWrite(STEPPER_PIN_4, LOW);
      break;
      case 1:
      digitalWrite(STEPPER_PIN_1, LOW);
      digitalWrite(STEPPER_PIN_2, HIGH);
      digitalWrite(STEPPER_PIN_3, LOW);
      digitalWrite(STEPPER_PIN_4, LOW);
      break;
      case 2:
      digitalWrite(STEPPER_PIN_1, LOW);
      digitalWrite(STEPPER_PIN_2, LOW);
      digitalWrite(STEPPER_PIN_3, HIGH);
      digitalWrite(STEPPER_PIN_4, LOW);
      break;
      case 3:
      digitalWrite(STEPPER_PIN_1, LOW);
      digitalWrite(STEPPER_PIN_2, LOW);
      digitalWrite(STEPPER_PIN_3, LOW);
      digitalWrite(STEPPER_PIN_4, HIGH);
      break;
    } 
  } else {
    switch(step_number){
      case 0:
      digitalWrite(STEPPER_PIN_1, LOW);
      digitalWrite(STEPPER_PIN_2, LOW);
      digitalWrite(STEPPER_PIN_3, LOW);
      digitalWrite(STEPPER_PIN_4, HIGH);
      break;
      case 1:
      digitalWrite(STEPPER_PIN_1, LOW);
      digitalWrite(STEPPER_PIN_2, LOW);
      digitalWrite(STEPPER_PIN_3, HIGH);
      digitalWrite(STEPPER_PIN_4, LOW);
      break;
      case 2:
      digitalWrite(STEPPER_PIN_1, LOW);
      digitalWrite(STEPPER_PIN_2, HIGH);
      digitalWrite(STEPPER_PIN_3, LOW);
      digitalWrite(STEPPER_PIN_4, LOW);
      break;
      case 3:
      digitalWrite(STEPPER_PIN_1, HIGH);
      digitalWrite(STEPPER_PIN_2, LOW);
      digitalWrite(STEPPER_PIN_3, LOW);
      digitalWrite(STEPPER_PIN_4, LOW);
    } 
  }
  step_number++;
  if(step_number > 3){
    step_number = 0;
  }
}

// set all pins on LOW (to avoid motor overheating)
void disableMotor() {
  digitalWrite(STEPPER_PIN_1, LOW);
  digitalWrite(STEPPER_PIN_2, LOW);
  digitalWrite(STEPPER_PIN_3, LOW);
  digitalWrite(STEPPER_PIN_4, LOW);
}

void initRoutine() {
  rotateCCW(FULL_STROKE_STEPS);
}

void catchRemoteCommand() {
  if (IrReceiver.decode()) {
    // debouncing for the remote (wait between presses 0.5s, not recording the same command twice)
    if (millis() - lastTimeRemotePressed >= 500) {
      if (mode != SETUP_MODE && IrReceiver.decodedIRData.command == 22) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("1 - auto");
        lcd.setCursor(0, 1);
        lcd.print("2 - manual");
        mode = SETUP_MODE;  
      } else if (mode == MANUAL_MODE && IrReceiver.decodedIRData.command == 8) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Going Down");
        rotateCCW(ONE_ROTATION_STEPS);
      } else if (mode == MANUAL_MODE && IrReceiver.decodedIRData.command == 90) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Going up");
        rotateCW(ONE_ROTATION_STEPS);
      } else if (mode == SETUP_MODE && IrReceiver.decodedIRData.command == 69) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Enabled");
        lcd.setCursor(0, 1);
        lcd.print("Auto Mode");
        mode = AUTOMATIC_MODE;
      } else if (mode == SETUP_MODE && IrReceiver.decodedIRData.command == 70) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Enabled");
        lcd.setCursor(0, 1);
        lcd.print("Manual Mode");
        mode = MANUAL_MODE;
      } else if (mode == SETUP_MODE && IrReceiver.decodedIRData.command == 22) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Already in setup");
        lcd.setCursor(0, 1);
        lcd.print("1-auto;2-manual");
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("CMD not found");
      }
      lastTimeRemotePressed = millis();
    }
      IrReceiver.resume();
  }
}

// function to rotate the motor clockwise a number of steps
void rotateCW(int steps) {
  if (steps > stepsToFullOpen)
    steps = stepsToFullOpen;
  for (int i = 0; i < steps; i++) {
    OneStep(true);
    delay(2);
  }
  disableMotor();
  stepsToFullOpen -= steps;
  if (stepsToFullOpen == 0) {
    isOpen = 1;
  }
}

// function to rotate the motor counter-clockwise a number of steps
void rotateCCW(int steps) {
  // rotate number of steps given, or until reaching the limit
  for (int i = 0; i < steps && digitalRead(MICROSWITCH_PIN) == HIGH; i++) {
    OneStep(false);
    delay(2);
  }
  stepsToFullOpen += steps;
  // we are sure that the curtains are closed only if the number of steps is FULL_STROKE_STEPS or that the switch is clicked
  if (digitalRead(MICROSWITCH_PIN) == LOW || steps == FULL_STROKE_STEPS) {
    isOpen = 0;
    stepsToFullOpen = FULL_STROKE_STEPS;
    Serial.println("Microswitch Clicked");
  }
  disableMotor();
}

// change the curtain state accordingly to the lightLevel shown by the light sensor, if needed
void changeStateCurtain() {
  if (mode == AUTOMATIC_MODE) {
    float lightLevel = lightMeter.readLightLevel();
    if (millis() - lastTimeLCDUpdated > 3000) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Light level is:");
      lcd.setCursor(0, 1);
      lcd.print(lightLevel);
      lcd.print(" lx");  
      lastTimeLCDUpdated = millis();
    }
    // if the curtain is closed and the light is above threshold, open
    if(lightLevel > 30 && isOpen == 0) {
      rotateCW(FULL_STROKE_STEPS);
    }
    // else if it is below and it is open, close
    else if (lightLevel < 30 && isOpen == 1) {
      rotateCCW(FULL_STROKE_STEPS);
    }
  } 
}

// function to get the addresses of the I2C devices connected
byte searchAddress() {
  byte addr = 0;
  while (!Serial);
  Serial.println ("I2C scanner. Scanning ...");
  byte count = 0;
  
  Wire.begin();
  for (byte i = 8; i < 120; i++)
  {
    Wire.beginTransmission (i);
    // there is a I2C device at the endpoint of that 'i' address
    if (Wire.endTransmission () == 0)
      {
      Serial.print ("Found address: ");
      Serial.print (i, DEC);
      Serial.print (" (0x");
      Serial.print (i, HEX);
      addr = i;
      Serial.println (")");
      count++;
      delay (1);
      }
  }
  Serial.println ("Done.");
  Serial.print ("Found ");
  Serial.print (count, DEC);
  Serial.println (" device(s).");
  return addr;
}
