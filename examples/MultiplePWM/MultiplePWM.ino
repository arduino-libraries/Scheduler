/*
 Multiple PWM

 Demonstrates the use of the Scheduler library

 Hardware required :
 * LEDs connected to PWM pins

 created 28 Dic 2015
 by Testato
 
*/


// Include Scheduler since we want to manage multiple tasks.
#include <Scheduler.h>

#define LED1 13
#define LED2 10
#define LED3 11
byte counter1;
byte counter2;


void setup() {

  // Setup the two led pins as OUTPUT
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  // Add "loop2 and loop3" to scheduler
  // "loop" is always started by default
  Scheduler.startLoop(loop2);
  Scheduler.startLoop(loop3);
}


// Task no.1 (standard Arduino loop() )
void loop() {
  digitalWrite(LED1, HIGH);
  delay(1000);
  digitalWrite(LED1, LOW);
  delay(1000);
  
  // When multiple tasks are running, 'delay' passes control to
  // other tasks while waiting and guarantees they get executed
  // It is not necessary to call yield() when using delay() 
}





