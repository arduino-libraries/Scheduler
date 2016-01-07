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

#if defined(ARDUINO_ARCH_AVR)
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

#elif defined(ARDUINO_ARCH_SAM) || defined(ARDUINO_ARCH_SAMD)
	#define DEFAULT_STACK_SIZE 1024
	typedef uint32_t stacksz_t;
#endif


extern "C" {
	typedef void (*SchedulerTask)(void);
	typedef void (*SchedulerParametricTask)(void *);
}

class SchedulerClass {
public:
	SchedulerClass();
	static void startLoop(SchedulerTask task, stacksz_t stackSize = DEFAULT_STACK_SIZE);
	static void start(SchedulerTask task, stacksz_t stackSize = DEFAULT_STACK_SIZE);
	static void start(SchedulerParametricTask task, void *data, stacksz_t stackSize = DEFAULT_STACK_SIZE);

	static void yield() { ::yield(); };
};

extern SchedulerClass Scheduler;

#endif
