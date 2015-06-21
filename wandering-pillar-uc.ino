/* 
  Wandering Pillar V0.0.1
  -----------------------
  
  short description
  
  ©2015, Jonas Probst <- TODO: license here!!! 
*/

/*TODO:
- Add execution timmer
- communication with database
- manuel mode */


#include <Bridge.h>
#include <Process.h>

//Settings
#define DEVICE_ID "1" //to identify with database
#define RUN_INTERVAL 5000 // in ms
#define GEAR_0_RPM 50
#define GEAR_1_RPM 100

#define CW LOW
#define CCW HIGH

//Analog pin Setup
#define PIN_BATTERY_LEVEL A0
#define PIN_M_RGT_RPM A1
#define PIN_M_RGT_CURRENT A2
#define PIN_M_LFT_RPM A3
#define PIN_M_LFT_CURRENT A4

//Digital pin setup
#define PIN_M_LFT_STOP 2
#define PIN_M_LFT_ENABLE 3
#define PIN_M_LFT_DIRECTION 4
#define PIN_M_LFT_GEAR 5
#define PIN_M_RGT_STOP 6
#define PIN_M_RGT_ENABLE 7
#define PIN_M_RGT_DIRECTION 8
#define PIN_M_RGT_GEAR 9

//wandering pillar protocoll
#define CMD_STOP 0
#define CMD_FORWARD 1
#define CMD_BACKWARD 2
#define CMD_RIGHT 3
#define CMD_LEFT 4
#define CMD_RESTART 5

//State
typedef enum State{
    stopped,
    movingForward,
    movingBackward,
    turningLeft,
    turningRight,
    stopping
};
State state = stopped;

//Event
typedef enum Event{
  cmd_stop,
  cmd_forward,
  cmd_backward,
  cmd_left,
  cmd_right,
  cmd_resume,
  cmd_restart,
  battery_empty
};
Event event = cmd_stop;

typedef struct Motor {
  int rpm;
  int current;
  int stopped;
  int enabled;
  int dir;
  int gear;
};
Motor motorLft;
Motor motorRgt;

//variables
unsigned long ts = 0; //"current" time for delays
unsigned long timer = 0;
unsigned long lastRun = 0;
boolean stateChange = true;
boolean gear = 0;
boolean autoMode = true;
int batteryLevel = 0;
int autoStepNbr = 0;

void setup() {
  // setup pin modes
  pinMode(PIN_BATTERY_LEVEL, INPUT);
  pinMode(PIN_M_RGT_RPM, INPUT);
  pinMode(PIN_M_RGT_CURRENT, INPUT);
  pinMode(PIN_M_LFT_RPM, INPUT);
  pinMode(PIN_M_LFT_CURRENT, INPUT);
  pinMode(PIN_M_LFT_STOP, OUTPUT);
  pinMode(PIN_M_LFT_ENABLE, OUTPUT);
  pinMode(PIN_M_LFT_DIRECTION, OUTPUT);
  pinMode(PIN_M_LFT_GEAR, OUTPUT);
  pinMode(PIN_M_RGT_STOP, OUTPUT);
  pinMode(PIN_M_RGT_ENABLE, OUTPUT);
  pinMode(PIN_M_RGT_DIRECTION, OUTPUT);
  pinMode(PIN_M_RGT_GEAR, OUTPUT);
  
  //don't worry about the gear for now.
  digitalWrite(PIN_M_RGT_GEAR, LOW);
  digitalWrite(PIN_M_LFT_GEAR, LOW);
  
  //start bridge
  Bridge.begin();
}

void loop() {
  readCurrentState();
  //Report current state to database
  if(millis() - lastRun >= RUN_INTERVAL){
    reportCurrentState();
    autoMode = readCurrentMode();
  }
  // emergency break?
  // if battery empty -> state = stopping
  if(autoMode){
    //AUTO MODE
    switch(state){
      case stopped:
        if(millis() <= ts + timer){
          //read next step from Programm in db
          //and set pin states accordingly
          if(stateChange){
            readNextAutoStep();
            ts = millis(); //remember the start time
          }
          stateChange = true;
          state = stopped;
        }
        break;
      case movingForward:
        //only if timer is not up yet
        if(millis() <= ts + timer){
          //set direction on state enter only
          if(stateChange) {
            moveForward();
            ts = millis();
          }
        } else {
          stateChange = true;
          state = stopping;
        }
        break;
      case movingBackward:
        if(millis() <= ts + timer){
          //set direction on state enter only
          if(stateChange) {
            moveBackward();
            ts = millis();
          }
        } else {
          stateChange = true;
          state = stopping;
        }
        break;
      case turningLeft:
        if(millis() <= ts + timer){
          //set direction on state enter only
          if(stateChange) {
            turnLeft();
            ts = millis();
          }
        } else {
          stateChange = true;
          state = stopping;
        }
        break;
      case turningRight:
        if(millis() <= ts + timer){
          //set direction on state enter only
          if(stateChange) {
            turnRight();
            ts = millis();
          }
        } else {
          stateChange = true;
          state = stopping;
        }
        break;
      case stopping:
        
        if(true){
          //set direction on state enter only
          if(stateChange) {
            fullStop();
          }
        } else {
          disableMotors();
          readyMotors(); //remove stop flag
          stateChange = true;
          state = stopped;
        }
        break;
      default: state = stopping;
    } 
  } else {
    // read next instruction from database
    //MANUEL MODE
  }
}


void moveForward(){
/*move forward by
  - turning right wheel clockwise
  - turning left wheel counter clock wise*/
  digitalWrite(PIN_M_RGT_DIRECTION, CW);
  digitalWrite(PIN_M_LFT_DIRECTION, CCW);
  
  // ready, set ... go!
  enableMotors();
}

void moveBackward(){
/*move backward by
  - turning right wheel counter clock wise
  - turning left wheel clock wise*/
  digitalWrite(PIN_M_RGT_DIRECTION, CCW);
  digitalWrite(PIN_M_LFT_DIRECTION, CW);
  
  // ready, set ... go!
  enableMotors();
  
}

void turnRight(){
/*turn right by
  - turning right wheel counter clock wise
  - turning left wheel counter clock wise*/
  digitalWrite(PIN_M_RGT_DIRECTION, CCW);
  digitalWrite(PIN_M_LFT_DIRECTION, CCW);
  
  // ready, set ... go!
  enableMotors();
}

void turnLeft(){
/*turn left by
  - turning right wheel clock wise
  - turning left wheel clock wise*/
  digitalWrite(PIN_M_RGT_DIRECTION, CW);
  digitalWrite(PIN_M_LFT_DIRECTION, CW);
  
  // ready, set ... go!
  enableMotors();
}

void fullStop(){
/*stop the whole shebang by
  - setting the stop flag (not disable!!)
  - this will slow things down nicely*/
  digitalWrite(PIN_M_RGT_STOP, HIGH);
  digitalWrite(PIN_M_LFT_STOP, HIGH);
}

void disableMotors(){
  digitalWrite(PIN_M_RGT_ENABLE, LOW);
  digitalWrite(PIN_M_LFT_ENABLE, LOW);
}

void enableMotors(){
  digitalWrite(PIN_M_RGT_ENABLE, HIGH);
  digitalWrite(PIN_M_LFT_ENABLE, HIGH);
}

void readyMotors(){
  // undo the stop flag, get the motors ready for action.
  digitalWrite(PIN_M_RGT_STOP, LOW);
  digitalWrite(PIN_M_LFT_STOP, LOW);
}

void readNextAutoStep(){
/* read the next step in our programm on the database
  - communicate with the CLI*/
  Process p;
  p.begin("python ~/code/wandering-pillar-cli/step.py");
  p.addParameter(DEVICE_ID);
  p.addParameter("a");
  p.addParameter("-s " + String(autoStepNbr));
  p.run();
  while (p.available()>0) {
    char c = p.read();
    // do something with the response.
    //comand,duration,gear
  }
  
  switch(cmd){
    case CMD_STOP:
      state = 
      break;
    case CMD_FORWARD:
      break;
    case CMD_BACKWARD:
      break;
    case CMD_RIGHT:
      break;
    case CMD_LEFT:
      break;
    case CMD_RESTART:
      break;
  }
  autoStepNbr++;
}

void readNextManStep(){
/* read the next manual step from the database.
  - communicate with the CLI.*/
  Process p;
  p.begin("python ~/code/wandering-pillar-cli/step.py");
  p.addParameter(DEVICE_ID);
  p.addParameter("m");
  p.run();
  while (p.available()>0) {
    char c = p.read();
    // do something with the response.
  }
}

boolean readCurrentMode(){
/*check if mode has changed in database.*/
  Process p;
  p.begin("python ~/code/wandering-pillar-cli/mode.py");
  p.addParameter(DEVICE_ID);
  p.run();
  while (p.available()>0) {
    char c = p.read();
    // do something with the response.
  }
  
}

void reportCurrentState(){
/*report current state including the values of all pins.
  - communicate via the CLI
  - run it asynchroniously -> non blocking.*/
  Process p;
  p.begin("python ~/code/wandering-pillar-cli/report.py ");
  //rpm, current, stop, enable, direction, gear
  p.addParameter(DEVICE_ID);
  p.addParameter("-l " + String(motorLft.rpm) + " " + String(motorLft.current) + " " + String(motorLft.stopped) + " " + String(motorLft.enabled) + " " + String(motorLft.dir) + " " + String(motorLft.gear));
  p.addParameter("-r " + String(motorRgt.rpm) + " " + String(motorRgt.current) + " " + String(motorRgt.stopped) + " " + String(motorRgt.enabled) + " " + String(motorRgt.dir) + " " + String(motorRgt.gear));
  p.addParameter("-b " + String(batteryLevel));
  p.addParameter("-t " + String(timer));
  p.addParameter("-s " + String(autoStepNbr));
  p.runAsynchronously();
}

void readCurrentState(){
/*reads the current pin states into the global variables */
  //left motor
  motorLft.rpm = getRpm(PIN_M_LFT_RPM); 
  motorLft.current = getCurrent(PIN_M_LFT_CURRENT);
  motorLft.stopped = digitalRead(PIN_M_LFT_STOP);
  motorLft.enabled = digitalRead(PIN_M_LFT_ENABLE);
  motorLft.dir = digitalRead(PIN_M_LFT_DIRECTION);
  motorLft.gear = digitalRead(PIN_M_LFT_GEAR);
  
  //right motor
  motorRgt.rpm = getRpm(PIN_M_RGT_RPM); //do some calculations with this!!
  motorRgt.current = getCurrent(PIN_M_RGT_CURRENT); //do some calculations with this!!
  motorRgt.stopped = digitalRead(PIN_M_RGT_STOP);
  motorRgt.enabled = digitalRead(PIN_M_RGT_ENABLE);
  motorRgt.dir = digitalRead(PIN_M_RGT_DIRECTION);
  motorRgt.gear = digitalRead(PIN_M_RGT_GEAR);
  
  //battery
  batteryLevel = getBatteryLevel(PIN_BATTERY_LEVEL);
}

int getRpm(int pin){
  int rpm = analogRead(pin);
  rpm = map(rpm, 0, 1023, 0, 150);
  //do some more calculations ?!
  return rpm;
}

int getCurrent(int pin){
  int current = analogRead(pin);
  current = map(current, 0, 1023, 0, 4);
  return current;
}

int getBatteryLevel(int pin){
  int batteryLevel = analogRead(pin);
  batteryLevel = map(batteryLevel, 0, 1023, 0, 4.75) * 3; //Ubat = (Uref/1024) / 100k * 300k
  return batteryLevel;
}
