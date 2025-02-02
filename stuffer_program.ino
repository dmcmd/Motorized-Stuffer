//include the library code
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ********** MAKE SURE THE SCREEN ADDRESS IS CORRECT ***********
// ********** USE THE I2C SCANNER PROGRAM TO FIND YOUR SCREEN'S ADDRESS **********
// set up the screen
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ********** SWITCH THESE TWO NUMBERS IF YOUR MOTOR IS GOING THE WRONG DIRECTION ***********
// ********** I.E. MAKE FORWARD = 1 AND BACKWARD = 0 ***********
// make the boolean for the motor direction.
const int forward = 0;
const int backward = 1;

// ********** SWITCH THESE TWO PINS IF YOUR ENCODER IS TURNING THE WRONG WAY ***********
// ********** I.E. MAKE encoderPinA = 2 AND encoderPinB = 3 ***********
// set up the rotary encoder pins
const int encoderPinA = 2;
const int encoderPinB = 3;

// ********* This is how long the motor will reverse for after you let off the pedal ***********
// ********* You can make this however long you want.  It's measured in milliseconds ***********
// ********* Default is 2 seconds (2000 Milliseconds).  You can change it if you want ***********
// set up the reverse time
int reverseTime = 2000;

// set up the rotary encoder button pin
const int encoderButtonPin = 8;

// set up motor pwm output pin
const int motorPin = 10;

// set up motor direction pin
const int directionPin = 7;

// set up the pedal
const int pedal = 12;
bool pedalOn = 0;

// set up limit switch
const int limitSwitch = 4;
bool limit = 0;

// this is the variable to tell the motor to reverse for a short time after the pedal is released
bool reverse = 0;
bool pause = 0;

// reverse timer variables
unsigned long reverseMillis;
unsigned long prevReverseMillis;

//pause timer variables
unsigned long pauseMillis;
unsigned long prevPauseMillis;
int pauseTime = 200;

// rotary encoder variables:
volatile int encoderPos = 0;
unsigned int lastReportedPos = 0;
static boolean rotating = false;
boolean A_set = false;
boolean B_set = false;
unsigned char encoderA;
unsigned char encoderB;
//unsigned char encoderAPrev = 0;
//unsigned long encoderTime;
int debounce = 2;

// set up rotary encoder button
int buttonState = 0;
int lastButtonState = 1;

// make the motor speed variable - this is the pwm output which will be from 0 to 255
int motorSpeed = 255;

// set up variable to print the speed percentage on the screen
int screenSpeed = 0;

// make the rotary encoder change the motor speed by this much per step
// you can make this whatever number you want to give you more speed control
// 5 is a good starting point
int encoderJump = 5;

void setup() {

  // set up the screen
  lcd.backlight();
  lcd.init();
  lcd.clear();

  // set up motor and direction pins
  pinMode(motorPin, OUTPUT);
  pinMode(directionPin, OUTPUT);

  // set up encoder pins
  pinMode(encoderPinA, INPUT);
  pinMode(encoderPinB, INPUT);
  pinMode(encoderButtonPin, INPUT);

  // set up the pedal pin
  pinMode(pedal, INPUT);

  // turn on pullup resistors
  digitalWrite(encoderPinA, HIGH);
  digitalWrite(encoderPinB, HIGH);
  digitalWrite(encoderButtonPin, HIGH);
  digitalWrite(pedal, HIGH);
  digitalWrite(limitSwitch, HIGH);

  // **** set up interrupts
  attachInterrupt(0, doEncoderA, CHANGE);
  attachInterrupt(1, doEncoderB, CHANGE);
}

void loop() {

  // first thing, check the limit switch to see if it has been hit
  limit = digitalRead(limitSwitch);

  // if the limit switch has been hit, thus connecting it to ground, stop the program and run the motor backward for 2 seconds and add a delay in to get your attention
  if (limit == LOW) {
    analogWrite(motorPin, 0);
    delay(100);
    digitalWrite(directionPin, backward);
    analogWrite(motorPin, 255);
    delay(2000);
    analogWrite(motorPin, 0);
    delay(1000);
  }

  // print out the speed on the screen
  lcd.setCursor(0, 0);
  lcd.print("     Speed:     ");
  lcd.setCursor(6, 1);
  screenSpeed = map(motorSpeed, 0, 255, 0, 100);
  lcd.print(screenSpeed);
  lcd.print(" ");

  // set motor direction to forward
  digitalWrite(directionPin, forward);

  //check to see if the pedal has been pressed
  pedalOn = digitalRead(pedal);

  // make sure motor is off if the pedal is off and it is not in reverse
  if ((pedalOn == HIGH) && (reverse == 0)) {
    analogWrite(motorPin, 0);
  }

  // run the motor if the pedal is on
  if (pedalOn == LOW) {
    analogWrite(motorPin, motorSpeed);
    // set up the pause and reverse function once the pedal is let off
    prevReverseMillis = millis();
    prevPauseMillis = millis();
    reverse = 1;
    pause = 1;
  }

  // the is the reverse code once the pedal has been let off
  if ((pedalOn == HIGH) && (reverse == 1)) {
    // put a pause in because the pedal just turned off to not put too much stress on the motor
    if (pause == 1) {
      // Stop the motor
      analogWrite(motorPin, 0);
      pauseMillis = millis();
      if ((pauseMillis - prevPauseMillis) >= pauseTime) {
        pause = 0;
      }
    }
    // once the pause is over, run the motor backward for the reverseTime
    else {
      reverseMillis = millis();
      if ((reverseMillis - prevReverseMillis) >= reverseTime) {
        reverse = 0;
      }
      digitalWrite(directionPin, backward);
      analogWrite(motorPin, motorSpeed);
    }
  }

  // make the stuffer reverse while the rotary encoder button is pressed
  buttonState = digitalRead(encoderButtonPin);
  while (buttonState == LOW) {
    buttonState = digitalRead(encoderButtonPin);
    digitalWrite(directionPin, backward);
    analogWrite(motorPin, motorSpeed);
  }

  // look for rotation of the rotary encooder
  rotating = true;
  if (lastReportedPos != encoderPos) {

    int encoderChange = (encoderPos - lastReportedPos);
    if (encoderChange > 0) {
      encoderChange = 1;
    }
    if (encoderChange < 0) {
      encoderChange = -1;
    }

    motorSpeed = (motorSpeed + (encoderChange * encoderJump));
    lastReportedPos = encoderPos;

    if (motorSpeed > 255) {
      motorSpeed = 255;
    }
    if (motorSpeed < 0) {
      motorSpeed = 0;
    }
  }
}

// Interrupt on A changing state
void doEncoderA() {
  // debounce
  if (rotating) {
    delay(debounce);  // wait a little until the bouncing is done
  }
  // Test transition, did things really change?
  if (digitalRead(encoderPinA) != A_set) {
    A_set = !A_set;

    // adjust counter -1 if A leads B
    if (A_set && !B_set)
      encoderPos -= 1;

    rotating = false;  // no more debouncing until loop() hits again
  }
}

// Interrupt on B changing state, same as A above
void doEncoderB() {
  if (rotating) {
    delay(debounce);
  }
  if (digitalRead(encoderPinB) != B_set) {
    B_set = !B_set;
    //  adjust counter + 1 if B leads A
    if (B_set && !A_set)
      encoderPos += 1;

    rotating = false;
  }
}
