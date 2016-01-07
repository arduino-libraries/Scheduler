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

#include "Scheduler.h"

extern "C" {

#define _NONINLINE_ __attribute__((noinline))
#define _NAKED_	 __attribute__((naked))
#define _UNUSED_	  __attribute__((unused))

#if defined(ARDUINO_ARCH_AVR)
	typedef uint8_t reg_t;
#ifdef  EIND
	#define GET_FAR_ADDRESS(var) ({ \
					uint32_t tmp;\
					__asm__ __volatile__( \
								"ldi	%A0, lo8(%1) \n\t" \
								"ldi	%B0, hi8(%1) \n\t" \
								"ldi	%C0, hh8(%1) \n\t" \
								"clr	%D0		  \n\t" \
								: \
								"=d" (tmp) \
								: \
								"p"  (&(var)) \
								); \
					tmp;\
					})
#endif
	#define NUM_REGS 40 // r0/31 + sp(2) + pc(2) + data(2) + ftask(2)
	#define SPL_REG 32
	#define SPH_REG 33
	#define PCL_REG 34
	#define PCH_REG 35
	#define DATAL_REG 36
	#define DATAH_REG 37
	#define TASKFL_REG 38
	#define TASKFH_REG 39

	#define STACK_EM0 0xFFFF
	#define STACK_EM1 0xFFFE

#elif defined(ARDUINO_ARCH_SAM) || defined(ARDUINO_ARCH_SAMD)
	typedef uint32_t reg_t;
	#define NUM_REGS 10	// r4-r11, sp, pc
	#define SP_REG 8
	#define PC_REG 9
	#define TASKF_REG 0
	#define DATA_REG 1

	#define STACK_EM0 0xFFFFFFFF
	#define STACK_EM1 0xFFFFFFFE

#endif

typedef struct CoopTask {
	reg_t regs[NUM_REGS];
	void* stackPtr;
	struct CoopTask* next;
	struct CoopTask* prev;
} CoopTask;

static CoopTask *cur = 0;

static CoopTask* __attribute__((noinline)) coopSchedule(char taskDied) {
	CoopTask* next = cur->next;

	if (taskDied) {
		// Halt if last task died.
		if (next == cur)
			while (1)
				;
		// Delete task
		if (cur->stackPtr)
			free(cur->stackPtr);
		cur->next->prev = cur->prev;
		cur->prev->next = cur->next;
		free(cur);
	}

	cur = next;

	return next;
}

#if defined(ARDUINO_ARCH_AVR)

static void _NAKED_ _NONINLINE_ coopTaskStart(void) {
	asm (
		"movw	r4, r24		;prepare to call ftask(data) \n\t"
		"movw	30, r4		\n\t"
		"ldd	r26, Z+34	;increment PC, next call ret without call ftask(data) \n\t"
		"ldd	r27, Z+35	\n\t"
		"adiw	r26, 62		;offset ret\n\t"
		"movw	r18, r26	\n\t"
		"std	Z+34, r18	\n\t"
		"std	Z+35, r19	\n\t"
		"ldd	r24, Z+36	;data \n\t"
		"ldd	r25, Z+37	\n\t"
		"ldd	r6, Z+38	\n\t"
		"ldd	r7, Z+39	\n\t"
		"movw	r30, r6		\n\t"
		"icall			;ftask(data) \n\t"
		"ldi	r24, 1		;current task die\n\t"
		"call   coopSchedule	;and get next coop\n\t"
		"movw	r4, r24		;r25:r24 cur task\n\t"
		"movw	30, r4		;load context \n\t"
		"ldd	r6, Z+32	;load stack\n\t"
		"mov	r28,r6		\n\t"
		"out	__SP_L__, r6	\n\t"
		"ldd	r6, Z+33	\n\t"
		"mov	r29,r6		\n\t"
		"out	__SP_H__, r6	\n\t"
		"ldd	r0, Z+0		;load register \n\t"
		"ldd	r1, Z+1		\n\t"
		"ldd	r2, Z+2		\n\t"
		"ldd	r3, Z+3		\n\t"
		"ldd	r4, Z+4		\n\t"
		"ldd	r5, Z+5		\n\t"
		"ldd	r6, Z+6		\n\t"
		"ldd	r7, Z+7		\n\t"
		"ldd	r8, Z+8		\n\t"
		"ldd	r9, Z+9		\n\t"
		"ldd	r10, Z+10	\n\t"
		"ldd	r11, Z+11	\n\t"
		"ldd	r12, Z+12	\n\t"
		"ldd	r13, Z+13	\n\t"
		"ldd	r14, Z+14	\n\t"
		"ldd	r15, Z+15	\n\t"
		"ldd	r16, Z+16	\n\t"
		"ldd	r17, Z+17	\n\t"
		"ldd	r18, Z+18	\n\t"
		"ldd	r19, Z+19	\n\t"
		"ldd	r20, Z+20	\n\t"
		"ldd	r21, Z+21	\n\t"
		"ldd	r22, Z+22	\n\t"
		"ldd	r23, Z+23	\n\t"
		"ldd	r24, Z+24	\n\t"
		"ldd	r25, Z+25	\n\t"
		"ldd	r26, Z+26	\n\t"
		"ldd	r27, Z+27	\n\t"
		"ldd	r28, Z+28	\n\t"
		"ldd	r29, Z+29	\n\t"
		"push   r4		\n\t"
		"push   r5		\n\t"
		"ldd	r4, Z+30	\n\t"
		"ldd	r5, Z+31	\n\t"
		"movw   r30, r4		\n\t"
		"pop	r5		\n\t"
		"pop	r4		\n\t"
		"ret			\n\t"
	);
}

static void _NAKED_ _NONINLINE_ coopDoYield(CoopTask* curTask _UNUSED_) {
	asm (
		"push   r30		;store context, r25:r24 holds address of next task context \n\t"
		"push   r31		\n\t"
		"movw   r30, r24	\n\t"
		"std	Z+0, r0		\n\t"
		"std	Z+1, r1		\n\t"
		"std	Z+2, r2		\n\t"
		"std	Z+3, r3		\n\t"
		"std	Z+4, r4		\n\t"
		"std	Z+5, r5		\n\t"
		"std	Z+6, r6		\n\t"
		"std	Z+7, r7		\n\t"
		"std	Z+8, r8		\n\t"
		"std	Z+9, r9		\n\t"
		"std	Z+10, r10	\n\t"
		"std	Z+11, r11	\n\t"
		"std	Z+12, r12	\n\t"
		"std	Z+13, r13	\n\t"
		"std	Z+14, r14	\n\t"
		"std	Z+15, r15	\n\t"
		"std	Z+16, r16	\n\t"
		"std	Z+17, r17	\n\t"
		"std	Z+18, r18	\n\t"
		"std	Z+19, r19	\n\t"
		"std	Z+20, r20	\n\t"
		"std	Z+21, r21	\n\t"
		"std	Z+22, r22	\n\t"
		"std	Z+23, r23	\n\t"
		"std	Z+24, r24	\n\t"
		"std	Z+25, r25	\n\t"
		"std	Z+26, r26	\n\t"
		"std	Z+27, r27	\n\t"
		"std	Z+28, r28	\n\t"
		"std	Z+29, r29	\n\t"
		"pop	r5		\n\t"
		"pop	r4		\n\t"
		"std	Z+30, r4	\n\t"
		"std	Z+31, r5	\n\t"
		"in	 r4, __SP_L__	;store stack \n\t"
		"std	Z+32, r4	\n\t"
		"in	 r4, __SP_H__	\n\t"
		"std	Z+33, r4	\n\t"
		"ldi	r24, 0		;next coop \n\t"
		"call  coopSchedule	\n\t"
		"movw	r4, r24		;load context \n\t"
		"movw	30, r4		\n\t"
		"ldd	r6, Z+32	\n\t"
		"mov	r28,r6		\n\t"
		"out	__SP_L__, r6	;load stack\n\t"
		"ldd	r6, Z+33	\n\t"
		"mov	r29,r6		\n\t"
		"out	__SP_H__, r6	\n\t"
		"ldd	r6, Z+34	;load pc\n\t"
		"ldd	r7, Z+35	\n\t"
		"movw	r30, r6		\n\t"
		"icall			;call coopTaskStart if begin else return after icall \n\t"
		"movw   r30, r4		;need reload structure \n\t"
		"ldd	r0, Z+0		;load register \n\t"
		"ldd	r1, Z+1		\n\t"
		"ldd	r2, Z+2		\n\t"
		"ldd	r3, Z+3		\n\t"
		"ldd	r4, Z+4		\n\t"
		"ldd	r5, Z+5		\n\t"
		"ldd	r6, Z+6		\n\t"
		"ldd	r7, Z+7		\n\t"
		"ldd	r8, Z+8		\n\t"
		"ldd	r9, Z+9		\n\t"
		"ldd	r10, Z+10	\n\t"
		"ldd	r11, Z+11	\n\t"
		"ldd	r12, Z+12	\n\t"
		"ldd	r13, Z+13	\n\t"
		"ldd	r14, Z+14	\n\t"
		"ldd	r15, Z+15	\n\t"
		"ldd	r16, Z+16	\n\t"
		"ldd	r17, Z+17	\n\t"
		"ldd	r18, Z+18	\n\t"
		"ldd	r19, Z+19	\n\t"
		"ldd	r20, Z+20	;load register \n\t"
		"ldd	r21, Z+21	\n\t"
		"ldd	r22, Z+22	\n\t"
		"ldd	r23, Z+23	\n\t"
		"ldd	r24, Z+24	\n\t"
		"ldd	r25, Z+25	\n\t"
		"ldd	r26, Z+26	\n\t"
		"ldd	r27, Z+27	\n\t"
		"ldd	r28, Z+28	\n\t"
		"ldd	r29, Z+29	\n\t"
		"push   r4		\n\t"
		"push   r5		\n\t"
		"ldd	r4, Z+30	\n\t"
		"ldd	r5, Z+31	\n\t"
		"movw   r30, r4		\n\t"
		"pop	r5		\n\t"
		"pop	r4		\n\t"
		"ret			\n\t"
		);
}

#elif defined(ARDUINO_ARCH_SAM) || defined(ARDUINO_ARCH_SAMD)

static void _NAKED_ _NONINLINE_ coopTaskStart(void) {
	asm (
		"mov   r0, r5;"
		"blx   r4;"
		/* schedule. */
		"mov   r0, #1;"	  /* returned from task func: task done */
		"bl	coopSchedule;"
		/* r0 holds address of next task context */
#if defined(ARDUINO_ARCH_SAMD)
		/* for cortex m0, ldm and stm are restricted to low registers */
		/* load high registers */
		"add   r0, #16;"	 /* they are 4 words higher in memory */
		"ldmia r0, {r1-r6};" /* load them in low registers first... */
		"mov   r8, r1;"	  /* ...and move them into high registers... */
		"mov   r9, r2;"
		"mov   r10, r3;"
		"mov   r11, r4;"
		"mov   r12, r5;"
		"mov   lr, r6;"
		/* load low registers */
		"sub   r0, r0, #40;" /* back to begin of context */
		"ldmia r0, {r4-r7};"
#else
		"ldmia r0, {r4-r12, lr};"
#endif
		/* restore task stack */
		"mov   sp, r12;"
		/* resume task */
		"bx	lr;"
	);
}

static void _NAKED_ _NONINLINE_ coopDoYield(CoopTask* curTask) {
	asm (
		"mov   r12, sp;"
#if defined(ARDUINO_ARCH_SAMD)
		/* store low registers */
		"stmia r0, {r4-r7};"
		/* store high registers */
		"mov   r1, r8;"	  /* move them to low registers first. */
		"mov   r2, r9;"
		"mov   r3, r10;"
		"mov   r4, r11;"
		"mov   r5, r12;"
		"mov   r6, lr;"
		"stmia r0, {r1-r6};"
#else
		"stmia r0, {r4-r12, lr};"
#endif
		/* schedule. */
		"mov   r0, #0;"	  /* previous task did not complete */
		"bl	coopSchedule;"
		/* r0 holds address of next task context */
#if defined(ARDUINO_ARCH_SAMD)
		/* for cortex m0, ldm and stm are restricted to low registers */
		/* load high registers */
		"add   r0, #16;"	 /* they are 4 words higher in memory */
		"ldmia r0, {r1-r6};" /* load them in low registers first... */
		"mov   r8, r1;"	  /* ...and move them into high registers... */
		"mov   r9, r2;"
		"mov   r10, r3;"
		"mov   r11, r4;"
		"mov   r12, r5;"
		"mov   lr, r6;"
		/* load low registers */
		"sub   r0, r0, #40;" /* back to begin of context */
		"ldmia r0, {r4-r7};"
#else
		"ldmia r0, {r4-r12, lr};"
#endif
		/* restore task stack */
		"mov   sp, r12;"
		/* resume task */
		"bx	lr;"
	);
}
#endif //ARCH

static int coopInit(void) {
	CoopTask* task;

	task = reinterpret_cast<CoopTask *>(malloc(sizeof(CoopTask)));
	if (!task)
		return 0;

	task->next = task;
	task->prev = task;
	task->stackPtr = NULL;

#ifdef ARDUINO_ARCH_AVR
	task->regs[SPL_REG] = 0;
	task->regs[SPH_REG] = 0;
#ifdef  EIND
	uint32_t pf = GET_FAR_ADDRESS(coopTaskStart);
	task->regs[PCL_REG] = ((uint16_t)(pf) + 62) & 0xFF;
	task->regs[PCH_REG] = (((uint16_t)(pf) + 62) >> 8) & 0xFF;
#else
	task->regs[PCL_REG] = ((uint16_t)(coopTaskStart) + 62) & 0xFF;
	task->regs[PCH_REG] = (((uint16_t)(coopTaskStart) + 62) >> 8) & 0xFF;
#endif
	task->regs[DATAL_REG] = 0;
	task->regs[DATAH_REG] = 0;
	task->regs[TASKFL_REG] = 0;
	task->regs[TASKFH_REG] = 0;
#endif

	cur = task;

	return 1;
}

static int coopSpawn(SchedulerParametricTask taskF, void* taskData, uint32_t stackSz) {
	uint8_t *stack = (uint8_t*)malloc(stackSz);

	if (!stack)
		return 0;

	CoopTask *task = reinterpret_cast<CoopTask *>(malloc(sizeof(CoopTask)));
	if (!task) {
		free(stack);
		return 0;
	}

	task->stackPtr = stack;

#ifdef ARDUINO_ARCH_AVR
	task->regs[TASKFL_REG] = (uint16_t)(taskF) & 0xFF;
	task->regs[TASKFH_REG] = ((uint16_t)(taskF) >> 8) & 0xFF;
	task->regs[DATAL_REG]  = (uint16_t)(taskData) & 0xFF;
	task->regs[DATAH_REG]  = ((uint16_t)(taskData) >> 8) & 0xFF;
#ifdef  EIND
	uint32_t pf = GET_FAR_ADDRESS(coopTaskStart);
	task->regs[PCL_REG] = (uint16_t)(pf) & 0xFF;
	task->regs[PCH_REG] = ((uint16_t)(pf) >> 8) & 0xFF;
#else
	task->regs[PCL_REG]	= (uint16_t)(coopTaskStart) & 0xFF;
	task->regs[PCH_REG]	= ((uint16_t)(coopTaskStart) >> 8) & 0xFF;
#endif
	task->regs[SPL_REG]	= (uint16_t)(stack + stackSz - 1) & 0xFF;
	task->regs[SPH_REG]	= ((uint16_t)(stack + stackSz - 1) >> 8) & 0xFF;

#elif defined(ARDUINO_ARCH_SAM) || defined(ARDUINO_ARCH_SAMD)
	task->regs[TASKF_REG] = (uint32_t) taskF;
	task->regs[DATA_REG]  = (uint32_t) taskData;
	task->regs[SP_REG]	= ((uint32_t)(stack + stackSz)) & ~7;
	task->regs[PC_REG]	= (uint32_t) & coopTaskStart;

#endif

	task->prev = cur;
	task->next = cur->next;
	cur->next->prev = task;
	cur->next = task;

	// These are here so compiler is sure that function is
	// referenced in both variants (cancels a warning)
	if (stackSz == STACK_EM0)
		coopSchedule(0);
	if (stackSz == STACK_EM1)
		coopSchedule(1);

	return 1;
}

void yield(void) {
	coopDoYield(cur);
}

}; // extern "C"

SchedulerClass::SchedulerClass() {
	coopInit();
}

static void startLoopHelper(void *taskData) {

	SchedulerTask task = reinterpret_cast<SchedulerTask>(taskData);

	while (true)
		task();
}

void SchedulerClass::startLoop(SchedulerTask task, stacksz_t stackSize)
{
	coopSpawn(startLoopHelper, reinterpret_cast<void *>(task), stackSize);
}

static void startTaskHelper(void *taskData) {
	SchedulerTask task = reinterpret_cast<SchedulerTask>(taskData);
	task();
#if defined(ARDUINO_ARCH_AVR)
	yield();
#endif
}

void SchedulerClass::start(SchedulerTask task, stacksz_t stackSize) {
	coopSpawn(startTaskHelper, reinterpret_cast<void *>(task), stackSize);
}

void SchedulerClass::start(SchedulerParametricTask task, void *taskData, stacksz_t stackSize) {
	coopSpawn(task, taskData, stackSize);
}

SchedulerClass Scheduler;

#undef _NONINLINE_
#undef _NOKED_
#undef _UNUSED_
