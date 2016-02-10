/*
 Scheduler View Memory Organization

 Demonstrates to view and debug task memory

 created 21 gen 2016
*/

// Include Scheduler since we want to manage multiple tasks.
#include <Scheduler.h>

// loop2 static memory
// set memory size
const int taskMemory = DEFAULT_STACK_SIZE;
// Create memory for loop2
TaskContext taskContext2;
TaskStack taskStack2[ taskMemory ];


//vector of structure contiene dati di ogni processo, sp spend context heap start heapend.

typedef enum {TYPE_SPEND, TYPE_SP, TYPE_CONTEXT, TYPE_CONTEXTEND, TYPE_HEAP, TYPE_HEAPEND, TYPE_COUNT} AddressType;

struct MemoryDump {
  uint32_t address;
  AddressType type;
  char name[7];
} dump[14];
byte curDump = 0;



void setup() {
  Serial.begin(9600);

  // Add "loop2" in global memory and "loop3" in heap memory.
  // "loop" is always started by default, use global memory for the context and is unico can be used the stack.
  Scheduler.startLoop(loop2, taskMemory, taskStack2, taskContext2);
  Scheduler.startLoop(loop3);
}

// Task no.1: passo subito l'esecuzione agli altri task che salveranno i dati
void loop() {
  yield();
  
  dump[curDump].address = Scheduler.stackPtr();
  dump[curDump].type = TYPE_SP;
  strcpy(dump[curDump].name, "loop");
  ++curDump;
  
  dump[curDump].address = Scheduler.stackEnd();
  dump[curDump].type = TYPE_SPEND;
  strcpy(dump[curDump].name, "loop");
  ++curDump;
  
  dump[curDump].address = Scheduler.contextPtr();
  dump[curDump].type = TYPE_CONTEXT;
  strcpy(dump[curDump].name, "loop");
  ++curDump;

  dump[curDump].address = Scheduler.contextPtr() + TASK_CONTEXT_SIZE;
  dump[curDump].type = TYPE_CONTEXTEND;
  strcpy(dump[curDump].name, "loop");
  ++curDump;
  
  dump[curDump].address = Scheduler.heapStart();
  dump[curDump].type = TYPE_HEAP;
  strcpy(dump[curDump].name, "sram");
  ++curDump;

  dump[curDump].address = Scheduler.heapEnd();
  dump[curDump].type = TYPE_HEAPEND;
  strcpy(dump[curDump].name, "sram");
  ++curDump;
  
  //riordino dati
  struct MemoryDump temp;
  byte swap;
  do {
    byte i;
    for( swap = 0, i = 0; i < curDump - 1; ++i ) {
      if ( dump[i].address > dump[i + 1].address ) {
        temp = dump[i];
        dump[i] = dump[i + 1];
        dump[i + 1] = temp;
        swap = 1;
      }
    }
  }while(swap);

  const char* memName[] = { "stack end", "stack pointer", "context start", "context end", "heap start", "heap end"};
  char adr[8];
  byte i;
  
  Serial.println(F("     --------------"));
  Serial.println(F("     ---- SRAM ----"));
  Serial.println(F("     --------------"));
  Serial.println(F("     ______________"));  
  for( swap = 0, i = 0; i < curDump; ++i ) {
      Serial.print(F("     | "));
      sprintf(adr,"0x%.8X",(unsigned)dump[i].address);
      Serial.print(adr);
      Serial.print(F(" |<--"));
      Serial.print(memName[dump[i].type]);
      Serial.print(" ");
      Serial.println(dump[i].name);
      if ( dump[i].type & 1 || dump[i].type == 4)
        Serial.println(F("     |____________|"));
      else
        Serial.println(F("     |            |"));
  }
  Serial.println(F("     --------------"));
  
  while(1);
}

// Task no.2: save data
void loop2() {
  dump[curDump].address = Scheduler.stackPtr();
  dump[curDump].type = TYPE_SP;
  strcpy(dump[curDump].name, "loop2");
  ++curDump;
  
  dump[curDump].address = Scheduler.stackEnd();
  dump[curDump].type = TYPE_SPEND;
  strcpy(dump[curDump].name, "loop2");
  ++curDump;
  
  dump[curDump].address = Scheduler.contextPtr();
  dump[curDump].type = TYPE_CONTEXT;
  strcpy(dump[curDump].name, "loop2");
  ++curDump;

  dump[curDump].address = Scheduler.contextPtr() + TASK_CONTEXT_SIZE;
  dump[curDump].type = TYPE_CONTEXTEND;
  strcpy(dump[curDump].name, "loop2");
  ++curDump;
  
  yield();
  while(1);
}

// Task no.3:
void loop3() {
  dump[curDump].address = Scheduler.stackPtr();
  dump[curDump].type = TYPE_SP;
  strcpy(dump[curDump].name, "loop3");
  ++curDump;
  
  dump[curDump].address = Scheduler.stackEnd();
  dump[curDump].type = TYPE_SPEND;
  strcpy(dump[curDump].name, "loop3");
  ++curDump;
  
  dump[curDump].address = Scheduler.contextPtr();
  dump[curDump].type = TYPE_CONTEXT;
  strcpy(dump[curDump].name, "loop3");
  ++curDump;

  dump[curDump].address = Scheduler.contextPtr() + TASK_CONTEXT_SIZE;
  dump[curDump].type = TYPE_CONTEXTEND;
  strcpy(dump[curDump].name, "loop3");
  ++curDump;
  
  yield();
  while(1);
}
