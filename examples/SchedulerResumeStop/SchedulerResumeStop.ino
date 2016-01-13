/*
 Scheduler Resume Stop Task

 Demonstrates to stop and resume task

 Hardware required :
 Serial

 Send 1 to resume task, 0 to stop task.
 
 created 2 fed 2016
*/

// Include Scheduler since we want to manage multiple tasks.
#include <Scheduler.h>

// loop2
TaskContext taskContext2;
TaskStack taskStack2[ DEFAULT_STACK_SIZE ];
tid_t loop2ID;

void setup() {
  Serial.begin(9600);
  pinMode(13, OUTPUT);
  
  //start loop2 and get task id
  loop2ID = Scheduler.startLoop(loop2, DEFAULT_STACK_SIZE, taskStack2, taskContext2);
}

void loop() {
  if ( Serial.available() ) {
    char cmd = Serial.read();

    switch ( cmd ) {
      case '0':
        Scheduler.taskStop(loop2ID);
      break;

      case '1':
        Scheduler.taskResume(loop2ID);
      break;
    }
  }
  yield();
}

void loop2() {
  digitalWrite(13, HIGH);
  delay(500);
  digitalWrite(13, LOW);
  delay(500);
}

