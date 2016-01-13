/*
 Scheduler Priority

 Demonstrates the use of the priority

 Hardware required :
 Serial

 Serial Monitor:
 Send 'r' to reverse priority and send 'n' to reset priority

 created 3 feb 2016
*/


// Include Scheduler since we want to manage multiple tasks.
#include <Scheduler.h>

TaskContext taskContext2;
TaskContext taskContext3;
TaskStack taskStack2[ DEFAULT_STACK_SIZE ];
TaskStack taskStack3[ DEFAULT_STACK_SIZE ];
tid_t loop2ID;
tid_t loop3ID;

void setup() {
  Serial.begin(9600);

  //Default priority loop == 7(max)
  //Default priority Task == 0(min)

  //First set priority and after start task
  Scheduler.setPriority(5);
  loop2ID = Scheduler.startLoop(loop2, sizeof taskStack2, taskStack2, taskContext2);
  Scheduler.setPriority(3);
  loop3ID = Scheduler.startLoop(loop3, sizeof taskStack3, taskStack3, taskContext3);  
  //execution:
  //loop(7) -> loop2(5) -> loop3(3)
}

void loop() {
  Serial.println(F("loop"));
  if ( Serial.available() ) {
      switch ( Serial.read() ) {
          case 'r':
            //reverse priority
            Scheduler.taskPriority(loop3ID, 7);
            //0 is current task
            Scheduler.taskPriority(0, 3);
          break;

          case 'n':
            //normal priority
            Scheduler.taskPriority(loop3ID, 3);
            //0 is current task
            Scheduler.taskPriority(0, 7);
          break;
      }
      
  }

  //delay without yield
  Scheduler.wait(800);
  yield();
}

void loop2() {
  Serial.println(F("loop2"));
  //delay without yield
  Scheduler.wait(800);
  yield();
}

void loop3() {
  Serial.println(F("loop3"));
  Serial.println("");
  //delay without yield
  Scheduler.wait(800);
  yield();
}
