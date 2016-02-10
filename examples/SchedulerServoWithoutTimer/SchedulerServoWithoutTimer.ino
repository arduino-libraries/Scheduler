/*
 Scheduler Servo without timer
 Demonstrates to control servomotor without use timer

 Hardware required :
 Serial, Servo

 Software:
 Connect servo to pin 5 otherwise change SERVO_PIN
 the default time servo is 10ms for 0°, 20ms for 180° and 200ms period, read your datasheet and change SERVO_MIN, SERVO_MAX and SERVO_PERIOD.
 Serial monitor end of char '\n'

 Usage:
 Send 'a' for attach
 Send 'd' for detach
 Send value in range '0' ... '180' to move servo.
 

 created 5 feb 2016
*/

// Include Scheduler since we want to manage multiple tasks.
#include <Scheduler.h>


/* GLOBAL FOR SERVO */
//Task
TaskContext servoContext;
TaskStack servoStack[ DEFAULT_STACK_SIZE ];
tid_t servoID;
//degrees servo
unsigned long servoTon  = 1500;
unsigned long servoTime = 0;
//pin servo
#define SERVO_PIN         5
#define SERVO_OFFSET_TIME 90
#define SERVO_PERIOD      20000
#define SERVO_MIN         1000
#define SERVO_MAX         2000
/********************/


/* GLOBAL FOR REMOTE COMMAND */
//task
TaskContext cmdContext;
TaskStack cmdStack[ DEFAULT_STACK_SIZE ];
tid_t cmdID;
/*******************************/


/* FUNCTION SERVO */

void servo_setup() {
  servoID = Scheduler.startLoop(servo_loop, sizeof servoStack, servoStack, servoContext);
  servoTon = 1500;
  Scheduler.taskStop(servoID);
}

void servo_attach() {
  pinMode(SERVO_PIN, OUTPUT);
  digitalWrite(SERVO_PIN, HIGH);
  servoTon = 1500;
  servoTime = micros();
  Scheduler.taskResume(servoID);
}

void servo_detach() {
  digitalWrite(SERVO_PIN, LOW);
  Scheduler.taskStop(servoID);
}

void servo_loop() {
  while ( micros() - servoTime < servoTon - SERVO_OFFSET_TIME ) yield();
  while ( micros() - servoTime < servoTon);
  digitalWrite(SERVO_PIN, LOW);
  
  while ( micros() - servoTime < SERVO_PERIOD - SERVO_OFFSET_TIME ) yield();
  while ( micros() - servoTime < SERVO_PERIOD);
  digitalWrite(SERVO_PIN, HIGH);

  servoTime = micros();
}
/******************/

/* FUNCTION COMMAND */

void command_setup() {
    cmdID = Scheduler.startLoop(command_loop, sizeof cmdStack, cmdStack, cmdContext);
}

void command_loop() {
  while ( Serial.available() < 1 ) yield();

  char ctype = Serial.read();

  switch ( ctype ){
      case 'a':
        servo_attach();
      return;

      case 'd':
        servo_detach();
      return;

      case '0' ... '9': break;
      
      default: return;
  }

  yield();
  char cval[5];
  byte i = 1;
  cval[0] = ctype;
  while( i < 3 ) {
      while ( Serial.available() < 1) yield();
      cval[i] = Serial.read();
      if ( cval[i] == '\n' ) break;
      ++i;
      yield();
  }
  
  cval[i] = '\0';

  unsigned long value = atoi(cval);
  yield();
  if ( value > 180 ) value = 180;
  servoTon = SERVO_MIN + (((((SERVO_MAX- SERVO_MIN) * 10) / 18) * value)>>2)/25;;
  yield();
}

/********************/

void setup() {
  Serial.begin(9600);
  pinMode(13, OUTPUT);
  command_setup();
  servo_setup();
}

//alive
void loop() {
  digitalWrite(13, HIGH);
  delay(350);
  digitalWrite(13, LOW);
  delay(350);
}



