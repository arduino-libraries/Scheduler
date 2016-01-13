/*
 Multiple Sketch

 Demonstrates the use of .start and .startLoop method of the Scheduler library

 Hardware required :
 * LEDs connected to pins

 created 07 Gen 2016
 by Testato
 
*/


// Include Scheduler since we want to manage multiple tasks.
#include <Scheduler.h>

#define LED1 13
#define LED2 10
#define LED3 11


// Sketch no.1 

void setup() {
  // Setup the led pin as OUTPUT
  pinMode(LED1, OUTPUT);

  // Add "setup2 and setup3" to scheduler
  Scheduler.start(setup2);
  Scheduler.start(setup3);
  // Add "loop2 and loop3" to scheduler
  Scheduler.startLoop(loop2);
  Scheduler.startLoop(loop3);
}


void loop() {
  digitalWrite(LED1, HIGH);
  delay(1000);
  digitalWrite(LED1, LOW);
  delay(1000);
  
  // When multiple tasks are running, 'delay' passes control to
  // other tasks while waiting and guarantees they get executed
  // It is not necessary to call yield() when using delay() 
}




