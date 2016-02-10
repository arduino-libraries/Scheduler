/*
 Scheduler Static Memory

 Demonstrates to create task in static memory

 Hardware required :
 Serial

 created 2 fed 2016
*/

// Include Scheduler since we want to manage multiple tasks.
#include <Scheduler.h>

// loop2 safe way, use Separate stack memory and context
TaskContext taskContext2;
TaskStack taskStack2[ DEFAULT_STACK_SIZE ];

// loop3 unsafe, use stack and context in same memory
TaskStack taskStack3[ DEFAULT_STACK_SIZE + TASK_CONTEXT_SIZE];

void setup() {
  Serial.begin(9600);

  //only .startLoop() can lanch task in static memory
  //.start() lanch task always in dynamic memory
  Scheduler.startLoop(loop2, DEFAULT_STACK_SIZE, taskStack2, taskContext2);
  Scheduler.startLoop(loop3, sizeof taskStack3 , taskStack3);  

}

void loop() {
  Serial.println(F("loop"));
  delay(1000);
}

void loop2() {
  Serial.println(F("loop2"));
  delay(1000);
}

void loop3() {
  Serial.println(F("loop3"));
  delay(1000);
}
