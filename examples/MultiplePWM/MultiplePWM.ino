/*
 Multiple PWM

 Demonstrates the use of the Scheduler library

 Hardware required :
 * LEDs connected to pins 10 and 11 for the UNO board (on other board change the pwm pins)

 created 28 Dic 2015
 by Testato
 
*/


// Include Scheduler since we want to manage multiple tasks.
#include <Scheduler.h>

byte counter1;
byte counter2;
byte led1 = 10;
byte led2 = 11;


void setup() {

  // Setup the two led pins as OUTPUT
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);

  // Add "loop2 and loop3" to scheduler
  // "loop" is always started by default
  Scheduler.startLoop(loop2);
  Scheduler.startLoop(loop3);
}


// Task no.1 (standard Arduino loop() )
void loop() {
  yield();
}





