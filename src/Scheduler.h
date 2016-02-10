/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include <Arduino.h>

#define CONFIG_AVR_OPTIMIZE_COOPERATIVE 0

#define DEFAULT_MIN_STACK_SIZE 50
#ifndef DEFAULT_LOOP_CONTEXT_STATIC
#define DEFAULT_LOOP_CONTEXT_STATIC 1
#endif

#if defined(ARDUINO_ARCH_AVR)

    #ifdef malloc
        #pragma push_macro("malloc")
        #undef malloc
    #else
        #define malloc malloc
        #pragma push_macro("malloc")
        #undef malloc
    #endif
    
    #define malloc __kmalloc
    
    typedef uint8_t reg_t;
	typedef uint16_t stacksz_t;
    
	#if ((RAMEND - RAMSTART) < 1000)
		#pragma GCC error "board is not supported"
		//invoche a real fatal error
		#include "board is not supported"	
	#elif ((RAMEND - RAMSTART) < 2000)
		#define DEFAULT_STACK_SIZE 200
	#elif ((RAMEND - RAMSTART) < 3000)
		#define DEFAULT_STACK_SIZE 250
	#elif ((RAMEND - RAMSTART) < 10000)
		#define DEFAULT_STACK_SIZE 900
	#else
		#define DEFAULT_STACK_SIZE 1024
	#endif
    
    #ifdef EIND
        #define TASK_CONTEXT_SIZE (44 + 13)
        typedef uint32_t tid_t;
    #else
        #define TASK_CONTEXT_SIZE (40 + 9)
        typedef uint16_t tid_t;
    #endif
    
#elif defined(ARDUINO_ARCH_SAM) || defined(ARDUINO_ARCH_SAMD)
	#define DEFAULT_STACK_SIZE 1024
    #define TASK_CONTEXT_SIZE (10 + 20)
    
    typedef uint32_t reg_t;
	typedef uint32_t stacksz_t;
    typedef uint32_t tid_t;
#endif


extern "C" {
	typedef void (*SchedulerTask)(void);
	typedef void (*SchedulerParametricTask)(void *);
    
    #if defined(ARDUINO_ARCH_AVR)
        void *__kmalloc(size_t len);
    #endif
    
    typedef reg_t TaskStack;
    typedef reg_t TaskContext[TASK_CONTEXT_SIZE];
    void switchTask(tid_t taskid);
    
    typedef uint8_t sem_t;
    typedef sem_t mutex_t;
}

class SchedulerClass {
public:
	SchedulerClass();
	static tid_t startLoop(SchedulerTask task, stacksz_t stackSize = DEFAULT_STACK_SIZE);
    static tid_t startLoop(SchedulerTask task, stacksz_t stackSize, TaskStack* stack);
    static tid_t startLoop(SchedulerTask task, stacksz_t stackSize, TaskStack* stack, uint8_t* context);
	static tid_t start(SchedulerTask task, stacksz_t stackSize = DEFAULT_STACK_SIZE);
	static tid_t start(SchedulerParametricTask task, void *data, stacksz_t stackSize = DEFAULT_STACK_SIZE);
    uint32_t stackPtr(void);
    uint32_t stackEnd(void);
    uint32_t contextPtr(void);
    uint32_t heapStart(void);
    uint32_t heapEnd(void);
    void taskStop(tid_t taskid);
    void taskResume(tid_t taskid);
    void setPriority(uint8_t priority);
    void taskPriority(tid_t taskid, uint8_t priority);
    void wait(uint32_t ms);
    static void switchTask(tid_t taskid){::switchTask(taskid);};
	static void yield() { ::yield(); };
};

extern SchedulerClass Scheduler;

#define sem_set(S, V) ({ S = V; })
#define sem_get(S)    ({S;})
#define sem_op(S,O)   ({ (S) += O; while( 0 == (S) ) yield(); )}

#define mutex_init(M)   sem_set(M, 1)
#define mutex_lock(M)   sem_op(M,-1)
#define mutex_unlock(M) sem_op(M, +1)


#endif
