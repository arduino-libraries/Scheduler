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


/* work in progress */
// Manca:   ContextSwitch per Mega -> da testare -> controllare call convenction per i puntatori a funzione come argomento, adesso 25,24,23
//      :   Volore di ritorno -> ok
//      :   Memoria Statica -> ok
//      :   stackGuard -> testare
//      :   statoTask -> stop/pending/start -> da testare -> uno deve rimanere attivo
//      :   ?tick
//      :   ?Priorità
//      :   ?Semafori -> chiedere al prof
//      :   ?watchdog
//      :   ?preemptive
/* ***************** */

#include "Scheduler.h"

/* FLAGS
 * |0|0|0|000|00|
 *  D O U  P   S
 *  i v n  r   t
 *  n e u  i   a
 *  a r s  o   t
 *  m f e  r   e
 *  i l d  i
 *  c o    t
 *    w    y
*/

#define TASK_FLAG_DINAMIC  0x80
#define TASK_FLAG_STOP     0x10  
#define TASK_FLAG_PRIORITY 0x07

extern "C" {

#define _NONINLINE_ __attribute__((noinline))
#define _NAKED_	    __attribute__((naked))
#define _UNUSED_	__attribute__((unused))

#if defined(ARDUINO_ARCH_AVR)
#ifdef  EIND
    #define GET_FAR_ADDRESS(var) ({ \
                                    uint32_t tmp;\
                                    __asm__ __volatile__( \
                                                            "ldi    %A0, lo8(%1) \n\t" \
                                                            "ldi    %B0, hi8(%1) \n\t" \
                                                            "ldi    %C0, hh8(%1) \n\t" \
                                                            "clr    %D0          \n\t" \
                                                            : \
                                                            "=d" (tmp) \
                                                            : \
                                                            "p"  (&(var)) \
                                                        ); \
                                    tmp;\
                                 })
#endif

#ifdef  EIND
	#define NUM_REGS 44 // r0/31 + sp(2) + pc(2) + data(2) + ftask(2)
    #define SPE_REG 40
    #define PCE_REG 41
    #define DATAE_REG 42
	#define TASKFE_REG 43
    
    #if CONFIG_AVR_OPTIMIZE_COOPERATIVE == 0
        #define OFFSET_SKIP 71
    #else
        #define OFFSET_SKIP 53
    #endif
#else
    #define NUM_REGS 40 // r0/31 + sp(2) + pc(2) + data(2) + ftask(2)
    
    #if CONFIG_AVR_OPTIMIZE_COOPERATIVE == 0
        #define OFFSET_SKIP 66
    #else
        #define OFFSET_SKIP 48
    #endif
#endif
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
    void* stackEnd;
    reg_t flags;
	struct CoopTask* next;
	struct CoopTask* prev;
} CoopTask;

static CoopTask *ktask = NULL;
static CoopTask *cur = NULL;
static CoopTask *taskStopped = NULL;
#if DEFAULT_LOOP_CONTEXT_STATIC == 1
static CoopTask __contextloop;
#endif

#define HANDLER_DISABLE    0
#define HANDLER_MALLOC     1

static uint8_t handler = 0;
static void*   hrarg  = 0;
static void*   hrret  = 0;
static tid_t   caller  = 0;
static uint8_t defpriority = 0;

static CoopTask* _NONINLINE_ coopSchedule(char taskDied) {
	CoopTask* next = cur->next;

	if ( 1 == taskDied && (cur->flags & TASK_FLAG_DINAMIC) ) {
		// Halt if last task died.
		if (next == cur)
			while(1);
		// Delete task
        if (cur->stackPtr)
			free(cur->stackPtr);
		cur->next->prev = cur->prev;
		cur->prev->next = cur->next;
		free(cur);
	}
    
    cur = next;
	return cur;
}

#if defined(ARDUINO_ARCH_AVR)

static void _NAKED_ _NONINLINE_ coopTaskStart(void) {
	asm (
		"movw	r4, r24		;prepare to call ftask(data) \n\t"
		"movw	30, r4		\n\t"
		"ldd	r26, Z+34	;increment PC, next call ret without call ftask(data) \n\t"
		"ldd	r27, Z+35	\n\t"
#if CONFIG_AVR_OPTIMIZE_COOPERATIVE == 0
		"adiw	r26, 60		;offset ret\n\t"
#else
        "adiw	r26, 42		;offset ret\n\t"
#endif
#ifdef EIND
        "adiw	r26, 11		;offset ret\n\t"
#else
        "adiw	r26, 6		;offset ret\n\t"
#endif
		"movw	r18, r26	\n\t"
		"std	Z+34, r18	\n\t"
		"std	Z+35, r19	\n\t"
#ifdef EIND
        "ldd	r23, Z+36	;data \n\t"
		"ldd	r24, Z+37	\n\t"
        "ldd	r25, Z+42	\n\t"
#else
		"ldd	r24, Z+36	;data \n\t"
		"ldd	r25, Z+37	\n\t"
#endif
		
#ifdef EIND
        "movw	r6, Z+43	\n\t"
        "out    EIND, r6    \n\t"
#endif
        "ldd	r6, Z+38	\n\t"
		"ldd	r7, Z+39	\n\t"
        "movw	r30, r6		\n\t"
#ifdef EIND
        "eicall			;ftask(data) \n\t"
#else
		"icall			;ftask(data) \n\t"
#endif
		"ldi	r24, 1		;current task die\n\t"
		"call   coopSchedule	;and get next coop\n\t"
		"movw	r4, r24		;r25:r24 cur task\n\t"
		"movw	30, r4		;load context \n\t"
		"ldd	r6, Z+32	;load stack\n\t"
        "in     r0, __SREG__    ;safe interrupt\n\t"
        "cli                \n\t"
		"mov	r28,r6		\n\t"
		"out	__SP_L__, r6	\n\t"
		"ldd	r6, Z+33	\n\t"
		"mov	r29,r6		\n\t"
		"out	__SP_H__, r6	\n\t"
#ifdef EIND
        "ldd    r6, Z+40    ;load 3byte stack\n\t"
        "out	EIND, r6	\n\t"
#endif
        "out    __SREG__, r0    \n\t"
#if CONFIG_AVR_OPTIMIZE_COOPERATIVE == 0
		"ldd	r0, Z+0		;load register \n\t"
		"ldd	r1, Z+1		\n\t"
#else
        "clr    r1          \n\t"
#endif
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
#if CONFIG_AVR_OPTIMIZE_COOPERATIVE == 0
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
#endif
		"ldd	r28, Z+28	\n\t"
		"ldd	r29, Z+29	\n\t"
#if CONFIG_AVR_OPTIMIZE_COOPERATIVE == 0
		"push   r4		\n\t"
		"push   r5		\n\t"
		"ldd	r4, Z+30	\n\t"
		"ldd	r5, Z+31	\n\t"
		"movw   r30, r4		\n\t"
		"pop	r5		\n\t"
		"pop	r4		\n\t"
#endif
		"ret			\n\t"
	);
}

static void _NAKED_ _NONINLINE_ coopDoYield(CoopTask* curTask _UNUSED_) {
	asm (
#if CONFIG_AVR_OPTIMIZE_COOPERATIVE == 0
		"push   r30		;store context, r25:r24 holds address of next task context \n\t"
		"push   r31		\n\t"
#endif
		"movw   r30, r24	\n\t"
#if CONFIG_AVR_OPTIMIZE_COOPERATIVE == 0
		"std	Z+0, r0		\n\t"
		"std	Z+1, r1		\n\t"
#endif
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
#if CONFIG_AVR_OPTIMIZE_COOPERATIVE == 0
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
#endif
		"std	Z+28, r28	\n\t"
		"std	Z+29, r29	\n\t"
#if CONFIG_AVR_OPTIMIZE_COOPERATIVE == 0
		"pop	r5		\n\t"
		"pop	r4		\n\t"
		"std	Z+30, r4	\n\t"
		"std	Z+31, r5	\n\t"
#endif
		"in	 r4, __SP_L__	;store stack \n\t"
		"std	Z+32, r4	\n\t"
		"in	 r4, __SP_H__	\n\t"
		"std	Z+33, r4	\n\t"
#ifdef EIND
        "in	 r4, EIND	\n\t"
		"std	Z+40, r4	\n\t"
#endif
		"ldi	r24, 0		;next coop \n\t"
		"call  coopSchedule	\n\t"
        "in     r0, __SREG__    ;safe interrupt\n\t"
        "cli                \n\t"
		"movw	r4, r24		;load context \n\t"
		"movw	30, r4		\n\t"
		"ldd	r6, Z+32	\n\t"
		"mov	r28,r6		\n\t"
		"out	__SP_L__, r6	;load stack\n\t"
		"ldd	r6, Z+33	\n\t"
		"mov	r29,r6		\n\t"
		"out	__SP_H__, r6	\n\t"
#ifdef EIND
        "ldd	r6,Z+40	\n\t"
        "out	EIND,r6	\n\t"
#endif
		"ldd	r6, Z+34	;load pc\n\t"
		"ldd	r7, Z+35	\n\t"
#ifdef EIND
        "ldd	r6,Z+41	\n\t"
        "out	EIND,r6	\n\t"
#endif
		"movw	r30, r6		\n\t"
        "out    __SREG__, r0    \n\t"
#ifdef EIND
        "eicall			;call coopTaskStart if begin else return after icall \n\t"
#else
		"icall			;call coopTaskStart if begin else return after icall \n\t"
#endif
		"movw   r30, r4		;need reload structure \n\t"
#if CONFIG_AVR_OPTIMIZE_COOPERATIVE == 0
		"ldd	r0, Z+0		;load register \n\t"
		"ldd	r1, Z+1		\n\t"
#else
        "clr    r1          \n\t"
#endif
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
#if CONFIG_AVR_OPTIMIZE_COOPERATIVE == 0
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
#endif
		"ldd	r28, Z+28	\n\t"
		"ldd	r29, Z+29	\n\t"
#if CONFIG_AVR_OPTIMIZE_COOPERATIVE == 0
		"push   r4		\n\t"
		"push   r5		\n\t"
		"ldd	r4, Z+30	\n\t"
		"ldd	r5, Z+31	\n\t"
		"movw   r30, r4		\n\t"
		"pop	r5		\n\t"
		"pop	r4		\n\t"
#endif
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
    
#if DEFAULT_LOOP_CONTEXT_STATIC == 1
	task = &__contextloop;
    task->flags = 7;
#else
    task = reinterpret_cast<CoopTask *>(malloc(sizeof(CoopTask)));
    
	if (!task)
		return 0;
    task->flags = TASK_FLAG_DINAMIC | 7;
    
#endif
	task->next = task;
	task->prev = task;
	task->stackPtr = NULL;
    task->stackEnd = NULL;
#ifdef ARDUINO_ARCH_AVR
	task->regs[SPL_REG] = 0;
	task->regs[SPH_REG] = 0;
#ifdef  EIND
    uint32_t pf = GET_FAR_ADDRESS(coopTaskStart);
    task->regs[PCL_REG] = (pf + OFFSET_SKIP)        & 0xFF;
	task->regs[PCH_REG] = (pf + OFFSET_SKIP) >> 8)  & 0xFF;
    task->regs[PCE_REG] = (pf + OFFSET_SKIP) >> 16) & 0xFF;
    task->regs[SPE_REG] = 0;
    task->regs[DATAE_REG] = 0;
    task->regs[TASKFE_REG] = 0;
#else
	task->regs[PCL_REG] = ((uint16_t)(coopTaskStart) + OFFSET_SKIP) & 0xFF;
	task->regs[PCH_REG] = (((uint16_t)(coopTaskStart) + OFFSET_SKIP) >> 8) & 0xFF;
#endif
	task->regs[DATAL_REG] = 0;
	task->regs[DATAH_REG] = 0;
	task->regs[TASKFL_REG] = 0;
	task->regs[TASKFH_REG] = 0;
#endif
    
	ktask = cur = task;
    
	return 1;
}

static void coopOrder(CoopTask* task) {
    CoopTask* taskHigh = cur;
    CoopTask* ftask = cur->next;
    
    //find high task
    for (; ftask != cur; ftask = ftask->next)
        if ( (taskHigh->flags & TASK_FLAG_PRIORITY) < (ftask->flags & TASK_FLAG_PRIORITY) )
            taskHigh = ftask;
    
    //find priority
    if ( (task->flags & TASK_FLAG_PRIORITY) > (taskHigh->flags & TASK_FLAG_PRIORITY) ) {
        ftask = taskHigh;
    }
    else {
        for (ftask = taskHigh->next; (ftask != taskHigh) && ((task->flags & TASK_FLAG_PRIORITY) < (ftask->flags & TASK_FLAG_PRIORITY)); ftask = ftask->next);
    }
    
    //push
    task->next = ftask;
    task->prev = ftask->prev;
    ftask->prev->next = task;
    ftask->prev = task;  
}

static tid_t coopSpawn(SchedulerParametricTask taskF, void* taskData, uint32_t stackSz, void* stack, void* context) {
	
    CoopTask *task = NULL;
    
    if ( NULL == stack || NULL == context )
    {
        stack = (uint8_t*)malloc(stackSz);

        if (!stack)
            return 0;
            
        task = reinterpret_cast<CoopTask *>(malloc(sizeof(CoopTask)));
        if (!task) {
            free(stack);
            return 0;
        }
        
        task->flags = TASK_FLAG_DINAMIC | defpriority;
    }
    else
    {
        task = (CoopTask*)(context);
        task->flags = defpriority;
    }

	task->stackPtr = stack;

#ifdef ARDUINO_ARCH_AVR

    
#ifdef  EIND
    uint32_t pf = GET_FAR_ADDRESS(coopTaskStart);
    task->regs[PCL_REG] = pf & 0xFF;
	task->regs[PCH_REG] = (pf >> 8) & 0xFF;
    task->regs[PCE_REG] = (pf >> 16) & 0xFF;
    pf = GET_FAR_ADDRESS(taskF);
    task->regs[TASKFL_REG] = pf & 0xFF;
	task->regs[TASKFH_REG] = (pf >> 8) & 0xFF;
    task->regs[TASKFE_REG] = (pf >> 16) & 0xFF;
    pf = GET_FAR_ADDRESS(taskData);
	task->regs[DATAL_REG]  = pf & 0xFF;
	task->regs[DATAH_REG]  = (pf >> 8) & 0xFF;
    task->regs[DATAE_REG]  = (pf >> 16) & 0xFF;
#else
    task->regs[TASKFL_REG] = (uint16_t)(taskF) & 0xFF;
	task->regs[TASKFH_REG] = ((uint16_t)(taskF) >> 8) & 0xFF;
	task->regs[DATAL_REG]  = (uint16_t)(taskData) & 0xFF;
	task->regs[DATAH_REG]  = ((uint16_t)(taskData) >> 8) & 0xFF;
	task->regs[PCL_REG]	= (uint16_t)(coopTaskStart) & 0xFF;
	task->regs[PCH_REG]	= ((uint16_t)(coopTaskStart) >> 8) & 0xFF;
	task->regs[SPL_REG]	= (uint16_t)((uint8_t*)(stack) + stackSz - 1) & 0xFF;
	task->regs[SPH_REG]	= ((uint16_t)((uint8_t*)(stack) + stackSz - 1 ) >> 8) & 0xFF;
#endif	

#elif defined(ARDUINO_ARCH_SAM) || defined(ARDUINO_ARCH_SAMD)
	task->regs[TASKF_REG] = (uint32_t) taskF;
	task->regs[DATA_REG]  = (uint32_t) taskData;
	task->regs[SP_REG]	= ((uint32_t)((uint32_t*)(stack) + stackSz)) & ~7;
	task->regs[PC_REG]	= (uint32_t) & coopTaskStart;

#endif
    
    task->stackEnd = stack;
    
    //add in priority 
    coopOrder(task);

	// These are here so compiler is sure that function is
	// referenced in both variants (cancels a warning)
	if (stackSz == STACK_EM0)
		coopSchedule(0);
	if (stackSz == STACK_EM1)
		coopSchedule(1);

	return (tid_t)(task);
}

void *__kmalloc(size_t len)
{
    #if defined(ARDUINO_ARCH_AVR)
    if ( cur == ktask )
        #pragma pop_macro("malloc")
        return malloc(len);
        #pragma push_macro("malloc")
        #undef malloc
        #define malloc __kmalloc
    #else
        return malloc(len);
    #endif
    handler = HANDLER_MALLOC;
    hrarg  = &len;
    caller = (tid_t)(cur);
    switchTask((tid_t)(ktask));
    return hrret;
}

static void ksys(void)
{
    if ( handler == HANDLER_MALLOC ){
        size_t* l = (size_t*)(hrarg);
        #if defined(ARDUINO_ARCH_AVR)
            #pragma pop_macro("malloc")
            hrret = malloc(*l);
            #pragma push_macro("malloc")
            #undef malloc
            #define malloc __kmalloc
        #else
            hrret = malloc(*l);
        #endif
    }
    handler = HANDLER_DISABLE;
    switchTask(caller);
}

void switchTask(tid_t taskid) {
    CoopTask* task = ( 0 != taskid ) ? (CoopTask*)(taskid) : cur;
    CoopTask* excur = cur;
    
    if ( task->flags & TASK_FLAG_STOP ) return;
    
    for(; cur->next != task; cur = cur->next);
    
    coopDoYield(excur);
    while ( handler != HANDLER_DISABLE && cur == ktask)
        ksys();
}

void yield(void) {
	coopDoYield(cur);
    while ( handler != HANDLER_DISABLE && cur == ktask )
        ksys();
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

tid_t SchedulerClass::startLoop(SchedulerTask task, stacksz_t stackSize)
{
    if ( stackSize < DEFAULT_MIN_STACK_SIZE)
         stackSize = DEFAULT_MIN_STACK_SIZE;
	return coopSpawn(startLoopHelper, reinterpret_cast<void *>(task), stackSize, NULL, NULL);
}

tid_t SchedulerClass::startLoop(SchedulerTask task, stacksz_t stackSize, TaskStack* stack)
{
    if ( NULL == stack ) return 0;
    if ( stackSize < sizeof(CoopTask) + DEFAULT_MIN_STACK_SIZE) return 0;
	return coopSpawn(startLoopHelper, reinterpret_cast<void *>(task), stackSize, (reg_t*)(stack) + sizeof(CoopTask), stack);
}

tid_t SchedulerClass::startLoop(SchedulerTask task, stacksz_t stackSize, TaskStack* stack, uint8_t* context)
{
    if ( NULL == stack ) return 0;
    if ( stackSize < DEFAULT_MIN_STACK_SIZE) return 0;
	return coopSpawn(startLoopHelper, reinterpret_cast<void *>(task), stackSize, stack, context);
}

static void startTaskHelper(void *taskData) {
	SchedulerTask task = reinterpret_cast<SchedulerTask>(taskData);
	task();
#ifdef ARDUINO_ARCH_AVR   
    yield();
#endif
}

tid_t SchedulerClass::start(SchedulerTask task, stacksz_t stackSize) {
    if ( stackSize < DEFAULT_MIN_STACK_SIZE)
         stackSize = DEFAULT_MIN_STACK_SIZE;
	return coopSpawn(startTaskHelper, reinterpret_cast<void *>(task), stackSize, NULL, NULL);
}

tid_t SchedulerClass::start(SchedulerParametricTask task, void *taskData, stacksz_t stackSize) {
    if ( stackSize < DEFAULT_MIN_STACK_SIZE)
         stackSize = DEFAULT_MIN_STACK_SIZE;
    return coopSpawn(task, taskData, stackSize, NULL, NULL);
}

uint32_t SchedulerClass::stackPtr(void) {
    uint32_t sp = (uint32_t)(cur->regs[SPH_REG]) << 8;
    sp |= cur->regs[SPL_REG];
    return sp;
}

uint32_t SchedulerClass::stackEnd(void) {
    return (cur->stackEnd == NULL) ? heapEnd() + 1 : (uint32_t)(cur->stackEnd);
}

uint32_t SchedulerClass::contextPtr(void) {
    return (uint32_t)(cur);
}

uint32_t SchedulerClass::heapStart(void) {
    extern char* __malloc_heap_start;
    return (uint32_t)(__malloc_heap_start);
}

uint32_t SchedulerClass::heapEnd(void) {
    extern char* __brkval;
    return (uint32_t)(__brkval);
}

void SchedulerClass::taskStop(tid_t taskid) {
    CoopTask* task = ( 0 != taskid ) ? (CoopTask*)(taskid) : cur;
    CoopTask* prev = cur->prev;
    
    task->flags |= TASK_FLAG_STOP;
    if ( task == task->next ) 
        while(1);
    
    //pull
    task->next->prev = task->prev;
    task->prev->next = task->next;
    
    //push to stop
    if ( taskStopped == NULL ) {
        taskStopped = task;
        task->next = task;
        task->prev = task;
    }
    else {
        task->prev = taskStopped;
        task->next = taskStopped->next;
        taskStopped->next->prev = task;
        taskStopped->next = task;    
    }
    
    if ( task == cur ){
        cur = prev;
        coopDoYield(task);
    }
    return;
}

void SchedulerClass::taskResume(tid_t taskid) {
    CoopTask* task = (CoopTask*)(taskid);
    
    if ( !(task->flags & TASK_FLAG_STOP) ) return;
    task->flags &= ~TASK_FLAG_STOP;
    
    //pull
    if ( task == taskStopped ) {
        if ( task == task->next )
            taskStopped = NULL;
        else
            taskStopped = taskStopped->next;
    }
    task->next->prev = task->prev;
    task->prev->next = task->next;
    
    //push to run
    coopOrder(task);
    
    return;
}

void SchedulerClass::setPriority(uint8_t priority) {
    defpriority = priority & TASK_FLAG_PRIORITY;
    return;
}

void SchedulerClass::taskPriority(tid_t taskid, uint8_t priority) {
    CoopTask* task = ( 0 != taskid ) ? (CoopTask*)(taskid) : cur;
    
    priority &= TASK_FLAG_PRIORITY;
    task->flags &= ~TASK_FLAG_PRIORITY;
    task->flags |= priority;
    if ( task == task->next ) return;
    if ( cur == task ) {
         cur = cur->next;
         taskid = 0;
    }
    //pull
    task->next->prev = task->prev;
    task->prev->next = task->next;
    //push
    coopOrder(task);
    if ( 0 == taskid ) cur = task;
}

void SchedulerClass::wait(uint32_t ms) {
	uint32_t start = micros();

	while (ms > 0) {
		while ( ms > 0 && (micros() - start) >= 1000) {
			ms--;
			start += 1000;
		}
	}
}

SchedulerClass Scheduler;

#undef TASK_FLAG_DINAMIC
#undef TASK_FLAG_STOP
#undef TASK_FLAG_PRIORITY


#undef NUMREGS
#undef STACK_EM0
#undef STACK_EM1

#ifdef ARDUINO_ARCH_AVR 
#ifdef EIND
    #undef SPE_REG
    #undef PCE_REG
    #undef DATAE_REG
	#undef TASKFE_REG
    #undef GET_FAR_ADDRESS
#endif
    #undef NUM_REGS
    #undef SPL_REG
	#undef SPH_REG
    #undef PCL_REG
	#undef PCH_REG
	#undef DATAL_REG
	#undef DATAH_REG
	#undef TASKFL_REG
	#undef TASKFH_REG
#elif defined(ARDUINO_ARCH_SAM) || defined(ARDUINO_ARCH_SAMD)
	#undef NUM_REGS
	#undef SP_REG
	#undef PC_REG
	#undef TASKF_REG
	#undef DATA_REG
#endif

#undef HANDLER_DISABLE
#undef HANDLER_MALLOC

#ifdef _NAKED_
#undef _NAKED_
#endif
#ifdef _NONINLINE_
#undef _NONINLINE_
#endif
#ifdef _UNUSED_
#undef _UNUSED_
#endif
