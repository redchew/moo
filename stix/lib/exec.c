/*
 * $Id$
 *
    Copyright (c) 2014-2016 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WAfRRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "stix-prv.h"

/* TODO: remove these headers after having migrated system-dependent functions of of this file */
#if defined(_WIN32)
#	include <windows.h>
#elif defined(__OS2__)
#	define INCL_DOSMISC
#	define INCL_DOSDATETIME
#	define INCL_DOSERRORS
#	include <os2.h>
#	include <time.h>
#elif defined(__MSDOS__)
#	include <time.h>
#elif defined(macintosh)
#	include <Types.h>
#	include <OSUtils.h>
#	include <Timer.h>
#else
#	if defined(HAVE_TIME_H)
#		include <time.h>
#	endif
#	if defined(HAVE_SYS_TIME_H)
#		include <sys/time.h>
#	endif
#endif

#if defined(USE_DYNCALL)
/* TODO: defined dcAllocMem and dcFreeMeme before builing the dynload and dyncall library */
#	include <dyncall.h> /* TODO: remove this. make dyXXXX calls to callbacks */
#endif


#define PROC_STATE_RUNNING 3
#define PROC_STATE_WAITING 2
#define PROC_STATE_RUNNABLE 1
#define PROC_STATE_SUSPENDED 0
#define PROC_STATE_TERMINATED -1

#define SEM_LIST_INC 256
#define SEM_HEAP_INC 256
#define SEM_LIST_MAX (SEM_LIST_INC * 1000)
#define SEM_HEAP_MAX (SEM_HEAP_INC * 1000)

#define SEM_HEAP_PARENT(x) (((x) - 1) / 2)
#define SEM_HEAP_LEFT(x)   ((x) * 2 + 1)
#define SEM_HEAP_RIGHT(x)  ((x) * 2 + 2)

#define SEM_HEAP_EARLIER_THAN(stx,x,y) ( \
	(STIX_OOP_TO_SMOOI((x)->heap_ftime_sec) < STIX_OOP_TO_SMOOI((y)->heap_ftime_sec)) || \
	(STIX_OOP_TO_SMOOI((x)->heap_ftime_sec) == STIX_OOP_TO_SMOOI((y)->heap_ftime_sec) && STIX_OOP_TO_SMOOI((x)->heap_ftime_nsec) < STIX_OOP_TO_SMOOI((y)->heap_ftime_nsec)) \
)


#define LOAD_IP(stix, v_ctx) ((stix)->ip = STIX_OOP_TO_SMOOI((v_ctx)->ip))
#define STORE_IP(stix, v_ctx) ((v_ctx)->ip = STIX_SMOOI_TO_OOP((stix)->ip))

#define LOAD_SP(stix, v_ctx) ((stix)->sp = STIX_OOP_TO_SMOOI((v_ctx)->sp))
#define STORE_SP(stix, v_ctx) ((v_ctx)->sp = STIX_SMOOI_TO_OOP((stix)->sp))

#define LOAD_ACTIVE_IP(stix) LOAD_IP(stix, (stix)->active_context)
#define STORE_ACTIVE_IP(stix) STORE_IP(stix, (stix)->active_context)

#define LOAD_ACTIVE_SP(stix) LOAD_SP(stix, (stix)->processor->active)
#define STORE_ACTIVE_SP(stix) STORE_SP(stix, (stix)->processor->active)

#define SWITCH_ACTIVE_CONTEXT(stix,v_ctx) \
	do \
	{ \
		STORE_ACTIVE_IP (stix); \
		(stix)->active_context = (v_ctx); \
		(stix)->active_method = (stix_oop_method_t)(stix)->active_context->origin->method_or_nargs; \
		SET_ACTIVE_METHOD_CODE(stix); \
		LOAD_ACTIVE_IP (stix); \
		(stix)->processor->active->current_context = (stix)->active_context; \
	} while (0)

#define FETCH_BYTE_CODE(stix) ((stix)->active_code[(stix)->ip++])
#define FETCH_BYTE_CODE_TO(stix, v_ooi) (v_ooi = FETCH_BYTE_CODE(stix))
#if (STIX_BCODE_LONG_PARAM_SIZE == 2)
#	define FETCH_PARAM_CODE_TO(stix, v_ooi) \
		do { \
			v_ooi = FETCH_BYTE_CODE(stix); \
			v_ooi = (v_ooi << 8) | FETCH_BYTE_CODE(stix); \
		} while (0)
#else
#	define FETCH_PARAM_CODE_TO(stix, v_ooi) (v_ooi = FETCH_BYTE_CODE(stix))
#endif


#if defined(STIX_DEBUG_VM_EXEC)
#	define LOG_MASK_INST (STIX_LOG_IC | STIX_LOG_MNEMONIC)

/* TODO: for send_message, display the method name. or include the method name before 'ip' */
#	define LOG_INST_0(stix,fmt) STIX_LOG1(stix, LOG_MASK_INST, " %06zd " fmt "\n", fetched_instruction_pointer)
#	define LOG_INST_1(stix,fmt,a1) STIX_LOG2(stix, LOG_MASK_INST, " %06zd " fmt "\n",fetched_instruction_pointer, a1)
#	define LOG_INST_2(stix,fmt,a1,a2) STIX_LOG3(stix, LOG_MASK_INST, " %06zd " fmt "\n", fetched_instruction_pointer, a1, a2)
#	define LOG_INST_3(stix,fmt,a1,a2,a3) STIX_LOG4(stix, LOG_MASK_INST, " %06zd " fmt "\n", fetched_instruction_pointer, a1, a2, a3)
#else
#	define LOG_INST_0(stix,fmt)
#	define LOG_INST_1(stix,fmt,a1)
#	define LOG_INST_2(stix,fmt,a1,a2)
#	define LOG_INST_3(stix,fmt,a1,a2,a3)
#endif

#define __PRIMITIVE_NAME__ (&__FUNCTION__[4])

/* ------------------------------------------------------------------------- */
static STIX_INLINE void vm_gettime (stix_t* stix, stix_ntime_t* now)
{
#if defined(_WIN32)

	/* TODO: */

#elif defined(__OS2__)
	ULONG out;

/* TODO: handle overflow?? */
/* TODO: use DosTmrQueryTime() and DosTmrQueryFreq()? */
	DosQuerySysInfo (QSV_MS_COUNT, QSV_MS_COUNT, &out, STIX_SIZEOF(out)); /* milliseconds */
	/* it must return NO_ERROR */

	STIX_INITNTIME (now, STIX_MSEC_TO_SEC(out), STIX_MSEC_TO_NSEC(out));
#elif defined(__MSDOS__) && defined(_INTELC32_)
	clock_t c;

/* TODO: handle overflow?? */
	c = clock ();
	now->sec = c / CLOCKS_PER_SEC;
	#if (CLOCKS_PER_SEC == 1000)
		now->nsec = STIX_MSEC_TO_NSEC(c % CLOCKS_PER_SEC);
	#elif (CLOCKS_PER_SEC == 1000000L)
		now->nsec = STIX_USEC_TO_NSEC(c % CLOCKS_PER_SEC);
	#elif (CLOCKS_PER_SEC == 1000000000L)
		now->nsec = (c % CLOCKS_PER_SEC);
	#else
	#	error UNSUPPORTED CLOCKS_PER_SEC
	#endif
#elif defined(macintosh)
	UnsignedWide tick;
	stix_uint64_t tick64;

	Microseconds (&tick);

	tick64 = *(stix_uint64_t*)&tick;
	STIX_INITNTIME (now, STIX_USEC_TO_SEC(tick64), STIX_USEC_TO_NSEC(tick64));

#elif defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_MONOTONIC)
	struct timespec ts;
	clock_gettime (CLOCK_MONOTONIC, &ts);
	STIX_INITNTIME(now, ts.tv_sec, ts.tv_nsec);

#elif defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_REALTIME)
	struct timespec ts;
	clock_gettime (CLOCK_REALTIME, &ts);
	STIX_INITNTIME(now, ts.tv_sec, ts.tv_nsec);
	STIX_SUBNTIME (now, now, &stix->vm_time_offset); /* offset */
#else
	struct timeval tv;
	gettimeofday (&tv, STIX_NULL);
	STIX_INITNTIME(now, tv.tv_sec, STIX_USEC_TO_NSEC(tv.tv_usec));

	/* at the first call, vm_time_offset should be 0. so subtraction takes
	 * no effect. once it becomes non-zero, it offsets the actual time.
	 * this is to keep the returned time small enough to be held in a 
	 * small integer on platforms where the small integer is not large enough */
	STIX_SUBNTIME (now, now, &stix->vm_time_offset); 
#endif
}

static STIX_INLINE void vm_sleep (stix_t* stix, const stix_ntime_t* dur)
{
#if defined(_WIN32)
	if (stix->waitable_timer)
	{
		LARGE_INTEGER li;
		li.QuadPart = -STIX_SECNSEC_TO_NSEC(dur->sec, dur->nsec);
		if(SetWaitableTimer(timer, &li, 0, STIX_NULL, STIX_NULL, FALSE) == FALSE) goto normal_sleep;
		WaitForSingleObject(timer, INFINITE);
	}
	else
	{
	normal_sleep:
		/* fallback to normal Sleep() */
		Sleep (STIX_SECNSEC_TO_MSEC(dur->sec,dur->nsec));
	}
#elif defined(__OS2__)

	/* TODO: in gui mode, this is not a desirable method??? 
	 *       this must be made event-driven coupled with the main event loop */
	DosSleep (STIX_SECNSEC_TO_MSEC(dur->sec,dur->nsec));

#elif defined(macintosh)

	/* TODO: ... */

#elif defined(__MSDOS__) && defined(_INTELC32_)

	clock_t c;

	c = clock ();
	c += dur->sec * CLOCKS_PER_SEC;
	#if (CLOCKS_PER_SEC == 1000)
		c += STIX_NSEC_TO_MSEC(dur->nsec);
	#elif (CLOCKS_PER_SEC == 1000000L)
		c += STIX_NSEC_TO_USEC(dur->nsec);
	#elif (CLOCKS_PER_SEC == 1000000000L)
		c += dur->nsec;
	#else
	#	error UNSUPPORTED CLOCKS_PER_SEC
	#endif

/* TODO: handle clock overvlow */
/* TODO: check if there is abortion request or interrupt */
	while (c > clock()) ;

#else
	struct timespec ts;
	ts.tv_sec = dur->sec;
	ts.tv_nsec = dur->nsec;
	nanosleep (&ts, STIX_NULL);
#endif
}


static void vm_startup (stix_t* stix)
{
	stix_ntime_t now;

#if defined(_WIN32)
	stix->waitable_timer = CreateWaitableTimer(STIX_NULL, TRUE, STIX_NULL);
#endif

	/* reset stix->vm_time_offset so that vm_gettime is not affected */
	STIX_INITNTIME(&stix->vm_time_offset, 0, 0);
	vm_gettime (stix, &now);
	stix->vm_time_offset = now;
}

static void vm_cleanup (stix_t* stix)
{
#if defined(_WIN32)
	if (stix->waitable_timer)
	{
		CloseHandle (stix->waitable_timer);
		stix->waitable_timer = STIX_NULL;
	}
#endif
}

/* ------------------------------------------------------------------------- */

static stix_oop_process_t make_process (stix_t* stix, stix_oop_context_t c)
{
	stix_oop_process_t proc;

	stix_pushtmp (stix, (stix_oop_t*)&c);
	proc = (stix_oop_process_t)stix_instantiate (stix, stix->_process, STIX_NULL, stix->option.dfl_procstk_size);
	stix_poptmp (stix);
	if (!proc) return STIX_NULL;

	proc->state = STIX_SMOOI_TO_OOP(PROC_STATE_SUSPENDED);
	proc->initial_context = c;
	proc->current_context = c;
	proc->sp = STIX_SMOOI_TO_OOP(-1);

	STIX_ASSERT ((stix_oop_t)c->sender == stix->_nil);

#if defined(STIX_DEBUG_VM_PROCESSOR)
	STIX_LOG2 (stix, STIX_LOG_IC | STIX_LOG_DEBUG, "Processor - made process %O of size %zu\n", proc, STIX_OBJ_GET_SIZE(proc));
#endif
	return proc;
}

static STIX_INLINE void sleep_active_process (stix_t* stix, int state)
{
#if defined(STIX_DEBUG_VM_PROCESSOR)
	STIX_LOG3 (stix, STIX_LOG_IC | STIX_LOG_DEBUG, "Processor - put process %O context %O ip=%zd to sleep\n", stix->processor->active, stix->active_context, stix->ip);
#endif

	STORE_ACTIVE_SP(stix);

	/* store the current active context to the current process.
	 * it is the suspended context of the process to be suspended */
	STIX_ASSERT (stix->processor->active != stix->nil_process);
	stix->processor->active->current_context = stix->active_context;
	stix->processor->active->state = STIX_SMOOI_TO_OOP(state);
}

static STIX_INLINE void wake_new_process (stix_t* stix, stix_oop_process_t proc)
{
	/* activate the given process */
	proc->state = STIX_SMOOI_TO_OOP(PROC_STATE_RUNNING);
	stix->processor->active = proc;

	LOAD_ACTIVE_SP(stix);

	/* activate the suspended context of the new process */
	SWITCH_ACTIVE_CONTEXT (stix, proc->current_context);

#if defined(STIX_DEBUG_VM_PROCESSOR)
	STIX_LOG3 (stix, STIX_LOG_IC | STIX_LOG_DEBUG, "Processor - woke up process %O context %O ip=%zd\n", stix->processor->active, stix->active_context, stix->ip);
#endif
}

static void switch_to_process (stix_t* stix, stix_oop_process_t proc, int new_state_for_old_active)
{
	/* the new process must not be the currently active process */
	STIX_ASSERT (stix->processor->active != proc);

	/* the new process must be in the runnable state */
	STIX_ASSERT (proc->state == STIX_SMOOI_TO_OOP(PROC_STATE_RUNNABLE) ||
	             proc->state == STIX_SMOOI_TO_OOP(PROC_STATE_WAITING));

	sleep_active_process (stix, new_state_for_old_active);
	wake_new_process (stix, proc);

	stix->proc_switched = 1;
}

static STIX_INLINE stix_oop_process_t find_next_runnable_process (stix_t* stix)
{
	stix_oop_process_t npr;

	STIX_ASSERT (stix->processor->active->state == STIX_SMOOI_TO_OOP(PROC_STATE_RUNNING));
	npr = stix->processor->active->next;
	if ((stix_oop_t)npr == stix->_nil) npr = stix->processor->runnable_head;
	return npr;
}

static STIX_INLINE void switch_to_next_runnable_process (stix_t* stix)
{
	stix_oop_process_t nrp;

	nrp = find_next_runnable_process (stix);
	if (nrp != stix->processor->active) switch_to_process (stix, nrp, PROC_STATE_RUNNABLE);
}

static STIX_INLINE int chain_into_processor (stix_t* stix, stix_oop_process_t proc)
{
	/* the process is not scheduled at all. 
	 * link it to the processor's process list. */
	stix_ooi_t tally;

	STIX_ASSERT ((stix_oop_t)proc->prev == stix->_nil);
	STIX_ASSERT ((stix_oop_t)proc->next == stix->_nil);

	STIX_ASSERT (proc->state == STIX_SMOOI_TO_OOP(PROC_STATE_SUSPENDED));

	tally = STIX_OOP_TO_SMOOI(stix->processor->tally);

	STIX_ASSERT (tally >= 0);
	if (tally >= STIX_SMOOI_MAX)
	{
#if defined(STIX_DEBUG_VM_PROCESSOR)
		STIX_LOG0 (stix, STIX_LOG_IC | STIX_LOG_FATAL, "Processor - too many process\n");
#endif
		stix->errnum = STIX_EPFULL;
		return -1;
	}

	/* append to the runnable list */
	if (tally > 0)
	{
		proc->prev = stix->processor->runnable_tail;
		stix->processor->runnable_tail->next = proc;
	}
	else
	{
		stix->processor->runnable_head = proc;
	}
	stix->processor->runnable_tail = proc;

	tally++;
	stix->processor->tally = STIX_SMOOI_TO_OOP(tally);

	return 0;
}

static STIX_INLINE void unchain_from_processor (stix_t* stix, stix_oop_process_t proc, int state)
{
	stix_ooi_t tally;

	/* the processor's process chain must be composed of running/runnable
	 * processes only */
	STIX_ASSERT (proc->state == STIX_SMOOI_TO_OOP(PROC_STATE_RUNNING) ||
	             proc->state == STIX_SMOOI_TO_OOP(PROC_STATE_RUNNABLE));

	tally = STIX_OOP_TO_SMOOI(stix->processor->tally);
	STIX_ASSERT (tally > 0);

	if ((stix_oop_t)proc->prev != stix->_nil) proc->prev->next = proc->next;
	else stix->processor->runnable_head = proc->next;
	if ((stix_oop_t)proc->next != stix->_nil) proc->next->prev = proc->prev;
	else stix->processor->runnable_tail = proc->prev;

	proc->prev = (stix_oop_process_t)stix->_nil;
	proc->next = (stix_oop_process_t)stix->_nil;
	proc->state = STIX_SMOOI_TO_OOP(state);

	tally--;
	if (tally == 0) stix->processor->active = stix->nil_process;
	stix->processor->tally = STIX_SMOOI_TO_OOP(tally);
}

static STIX_INLINE void chain_into_semaphore (stix_t* stix, stix_oop_process_t proc, stix_oop_semaphore_t sem)
{
	/* append a process to the process list of a semaphore*/

	STIX_ASSERT ((stix_oop_t)proc->sem == stix->_nil);
	STIX_ASSERT ((stix_oop_t)proc->prev == stix->_nil);
	STIX_ASSERT ((stix_oop_t)proc->next == stix->_nil);

	if ((stix_oop_t)sem->waiting_head == stix->_nil)
	{
		STIX_ASSERT ((stix_oop_t)sem->waiting_tail == stix->_nil);
		sem->waiting_head = proc;
	}
	else
	{
		proc->prev = sem->waiting_tail;
		sem->waiting_tail->next = proc;
	}
	sem->waiting_tail = proc;

	proc->sem = sem;
}

static STIX_INLINE void unchain_from_semaphore (stix_t* stix, stix_oop_process_t proc)
{
	stix_oop_semaphore_t sem;

	STIX_ASSERT ((stix_oop_t)proc->sem != stix->_nil);

	sem = proc->sem;
	if ((stix_oop_t)proc->prev != stix->_nil) proc->prev->next = proc->next;
	else sem->waiting_head = proc->next;
	if ((stix_oop_t)proc->next != stix->_nil) proc->next->prev = proc->prev;
	else sem->waiting_tail = proc->prev;

	proc->prev = (stix_oop_process_t)stix->_nil;
	proc->next = (stix_oop_process_t)stix->_nil;

	proc->sem = (stix_oop_semaphore_t)stix->_nil;
}

static void terminate_process (stix_t* stix, stix_oop_process_t proc)
{
	if (proc->state == STIX_SMOOI_TO_OOP(PROC_STATE_RUNNING) ||
	    proc->state == STIX_SMOOI_TO_OOP(PROC_STATE_RUNNABLE))
	{
		/* RUNNING/RUNNABLE ---> TERMINATED */

	#if defined(STIX_DEBUG_VM_PROCESSOR)
		STIX_LOG1 (stix, STIX_LOG_IC | STIX_LOG_DEBUG, "Processor - process %O RUNNING/RUNNABLE->TERMINATED\n", proc);
	#endif

		if (proc == stix->processor->active)
		{
			stix_oop_process_t nrp;

			nrp = find_next_runnable_process (stix);

			unchain_from_processor (stix, proc, PROC_STATE_TERMINATED);
			proc->sp = STIX_SMOOI_TO_OOP(-1); /* invalidate the process stack */
			proc->current_context = proc->initial_context; /* not needed but just in case */

			/* a runnable or running process must not be chanined to the
			 * process list of a semaphore */
			STIX_ASSERT ((stix_oop_t)proc->sem == stix->_nil);

			if (nrp == proc)
			{
				/* no runnable process after termination */
				STIX_ASSERT (stix->processor->active == stix->nil_process);
				STIX_LOG0 (stix, STIX_LOG_IC | STIX_LOG_DEBUG, "No runnable process after process termination\n");
			}
			else
			{
				switch_to_process (stix, nrp, PROC_STATE_TERMINATED);
			}
		}
		else
		{
			unchain_from_processor (stix, proc, PROC_STATE_TERMINATED);
			proc->sp = STIX_SMOOI_TO_OOP(-1); /* invalidate the process stack */
		}
	}
	else if (proc->state == STIX_SMOOI_TO_OOP(PROC_STATE_SUSPENDED))
	{
		/* SUSPENDED ---> TERMINATED */
	#if defined(STIX_DEBUG_VM_PROCESSOR)
		STIX_LOG1 (stix, STIX_LOG_IC | STIX_LOG_DEBUG, "Processor - process %O SUSPENDED->TERMINATED\n", proc);
	#endif

		proc->state = STIX_SMOOI_TO_OOP(PROC_STATE_TERMINATED);
		proc->sp = STIX_SMOOI_TO_OOP(-1); /* invalidate the proce stack */

		if ((stix_oop_t)proc->sem != stix->_nil)
		{
			unchain_from_semaphore (stix, proc);
		}
	}
	else if (proc->state == STIX_SMOOI_TO_OOP(PROC_STATE_WAITING))
	{
		/* WAITING ---> TERMINATED */
		/* TODO: */
	}
}

static void resume_process (stix_t* stix, stix_oop_process_t proc)
{
	if (proc->state == STIX_SMOOI_TO_OOP(PROC_STATE_SUSPENDED))
	{
		/* SUSPENED ---> RUNNING */
		STIX_ASSERT ((stix_oop_t)proc->prev == stix->_nil);
		STIX_ASSERT ((stix_oop_t)proc->next == stix->_nil);

	#if defined(STIX_DEBUG_VM_PROCESSOR)
		STIX_LOG1 (stix, STIX_LOG_IC | STIX_LOG_DEBUG, "Processor - process %O SUSPENDED->RUNNING\n", proc);
	#endif

		chain_into_processor (stix, proc); /* TODO: error check */

		/*proc->current_context = proc->initial_context;*/
		proc->state = STIX_SMOOI_TO_OOP(PROC_STATE_RUNNABLE);

		/* don't switch to this process. just set the state to RUNNING */
	}
#if 0
	else if (proc->state == STIX_SMOOI_TO_OOP(PROC_STATE_RUNNABLE))
	{
		/* RUNNABLE ---> RUNNING */
		/* TODO: should i allow this? */
		STIX_ASSERT (stix->processor->active != proc);
		switch_to_process (stix, proc, PROC_STATE_RUNNABLE);
	}
#endif
}

static void suspend_process (stix_t* stix, stix_oop_process_t proc)
{
	if (proc->state == STIX_SMOOI_TO_OOP(PROC_STATE_RUNNING) ||
	    proc->state == STIX_SMOOI_TO_OOP(PROC_STATE_RUNNABLE))
	{
		/* RUNNING/RUNNABLE ---> SUSPENDED */

	#if defined(STIX_DEBUG_VM_PROCESSOR)
		STIX_LOG1 (stix, STIX_LOG_IC | STIX_LOG_DEBUG, "Processor - process %O RUNNING/RUNNABLE->SUSPENDED\n", proc);
	#endif

		if (proc == stix->processor->active)
		{
			stix_oop_process_t nrp;

			nrp = find_next_runnable_process (stix);

			if (nrp == proc)
			{
				/* no runnable process after suspension */
				sleep_active_process (stix, PROC_STATE_RUNNABLE);
				unchain_from_processor (stix, proc, PROC_STATE_SUSPENDED);

				/* the last running/runnable process has been unchained 
				 * from the processor and set to SUSPENDED. the active
				 * process must be the nil process */
				STIX_ASSERT (stix->processor->active == stix->nil_process);
			}
			else
			{
				/* keep the unchained process at the runnable state for
				 * the immediate call to switch_to_process() below */
				unchain_from_processor (stix, proc, PROC_STATE_RUNNABLE);
				/* unchain_from_processor() leaves the active process
				 * untouched unless the unchained process is the last
				 * running/runnable process. so calling switch_to_process()
				 * which expects the active process to be valid is safe */
				STIX_ASSERT (stix->processor->active != stix->nil_process);
				switch_to_process (stix, nrp, PROC_STATE_SUSPENDED);
			}
		}
		else
		{
			unchain_from_processor (stix, proc, PROC_STATE_SUSPENDED);
		}
	}
}

static void yield_process (stix_t* stix, stix_oop_process_t proc)
{
	if (proc->state == STIX_SMOOI_TO_OOP(PROC_STATE_RUNNING))
	{
		/* RUNNING --> RUNNABLE */

		stix_oop_process_t nrp;

		STIX_ASSERT (proc == stix->processor->active);

		nrp = find_next_runnable_process (stix); 
		/* if there are more than 1 runnable processes, the next
		 * runnable process must be different from proc */
		if (nrp != proc) 
		{
		#if defined(STIX_DEBUG_VM_PROCESSOR)
			STIX_LOG1 (stix, STIX_LOG_IC | STIX_LOG_DEBUG, "Processor - process %O RUNNING->RUNNABLE\n", proc);
		#endif
			switch_to_process (stix, nrp, PROC_STATE_RUNNABLE);
		}
	}
}

static int async_signal_semaphore (stix_t* stix, stix_oop_semaphore_t sem)
{
	if (stix->sem_list_count >= SEM_LIST_MAX)
	{
		stix->errnum = STIX_ESLFULL;
		return -1;
	}

	if (stix->sem_list_count >= stix->sem_list_capa)
	{
		stix_oow_t new_capa;
		stix_oop_semaphore_t* tmp;

		new_capa = stix->sem_list_capa + SEM_LIST_INC; /* TODO: overflow check.. */
		tmp = stix_reallocmem (stix, stix->sem_list, STIX_SIZEOF(stix_oop_semaphore_t) * new_capa);
		if (!tmp) return -1;

		stix->sem_list = tmp;
		stix->sem_list_capa = new_capa;
	}

	stix->sem_list[stix->sem_list_count] = sem;
	stix->sem_list_count++;
	return 0;
}

static stix_oop_process_t signal_semaphore (stix_t* stix, stix_oop_semaphore_t sem)
{
	stix_oop_process_t proc;
	stix_ooi_t count;

	if ((stix_oop_t)sem->waiting_head == stix->_nil)
	{
		/* no process is waiting on this semaphore */
		count = STIX_OOP_TO_SMOOI(sem->count);
		count++;
		sem->count = STIX_SMOOI_TO_OOP(count);

		/* no process has been resumed */
		return (stix_oop_process_t)stix->_nil;
	}
	else
	{
		proc = sem->waiting_head;

		/* [NOTE] no GC must occur as 'proc' isn't protected with stix_pushtmp(). */

		unchain_from_semaphore (stix, proc);
		resume_process (stix, proc); /* TODO: error check */

		/* return the resumed process */
		return proc;
	}
}

static void await_semaphore (stix_t* stix, stix_oop_semaphore_t sem)
{
	stix_oop_process_t proc;
	stix_ooi_t count;

	count = STIX_OOP_TO_SMOOI(sem->count);
	if (count > 0)
	{
		/* it's already signalled */
		count--;
		sem->count = STIX_SMOOI_TO_OOP(count);
	}
	else
	{
		/* not signaled. need to wait */
		proc = stix->processor->active;

		/* suspend the active process */
		suspend_process (stix, proc); 

		/* link the suspended process to the semaphore's process list */
		chain_into_semaphore (stix, proc, sem); 

		STIX_ASSERT (sem->waiting_tail == proc);

		STIX_ASSERT (stix->processor->active != proc);
	}
}

static void sift_up_sem_heap (stix_t* stix, stix_ooi_t index)
{
	if (index > 0)
	{
		stix_ooi_t parent;
		stix_oop_semaphore_t sem, parsem;

		parent = SEM_HEAP_PARENT(index);
		sem = stix->sem_heap[index];
		parsem = stix->sem_heap[parent];
		if (SEM_HEAP_EARLIER_THAN(stix, sem, parsem))
		{
			do
			{
				/* move down the parent to the current position */
				parsem->heap_index = STIX_SMOOI_TO_OOP(index);
				stix->sem_heap[index] = parsem;

				/* traverse up */
				index = parent;
				if (index <= 0) break;

				parent = SEM_HEAP_PARENT(parent);
				parsem = stix->sem_heap[parent];
			}
			while (SEM_HEAP_EARLIER_THAN(stix, sem, parsem));

			sem->heap_index = STIX_SMOOI_TO_OOP(index);
			stix->sem_heap[index] = sem;
		}
	}
}

static void sift_down_sem_heap (stix_t* stix, stix_ooi_t index)
{
	stix_ooi_t base = stix->sem_heap_count / 2;

	if (index < base) /* at least 1 child is under the 'index' position */
	{
		stix_ooi_t left, right, child;
		stix_oop_semaphore_t sem, chisem;

		sem = stix->sem_heap[index];
		do
		{
			left = SEM_HEAP_LEFT(index);
			right = SEM_HEAP_RIGHT(index);

			if (right < stix->sem_heap_count && SEM_HEAP_EARLIER_THAN(stix, stix->sem_heap[left], stix->sem_heap[right]))
			{
				child = right;
			}
			else
			{
				child = left;
			}

			chisem = stix->sem_heap[child];
			if (SEM_HEAP_EARLIER_THAN(stix, sem, chisem)) break;

			chisem->heap_index = STIX_SMOOI_TO_OOP(index);
			stix->sem_heap[index ] = chisem;

			index = child;
		}
		while (index < base);

		sem->heap_index = STIX_SMOOI_TO_OOP(index);
		stix->sem_heap[index] = sem;
	}
}

static int add_to_sem_heap (stix_t* stix, stix_oop_semaphore_t sem)
{
	stix_ooi_t index;

	if (stix->sem_heap_count >= SEM_HEAP_MAX)
	{
		stix->errnum = STIX_ESHFULL;
		return -1;
	}

	if (stix->sem_heap_count >= stix->sem_heap_capa)
	{
		stix_oow_t new_capa;
		stix_oop_semaphore_t* tmp;

		/* no overflow check when calculating the new capacity
		 * owing to SEM_HEAP_MAX check above */
		new_capa = stix->sem_heap_capa + SEM_HEAP_INC;
		tmp = stix_reallocmem (stix, stix->sem_heap, STIX_SIZEOF(stix_oop_semaphore_t) * new_capa);
		if (!tmp) return -1;

		stix->sem_heap = tmp;
		stix->sem_heap_capa = new_capa;
	}

	STIX_ASSERT (stix->sem_heap_count <= STIX_SMOOI_MAX);

	index = stix->sem_heap_count;
	stix->sem_heap[index] = sem;
	sem->heap_index = STIX_SMOOI_TO_OOP(index);
	stix->sem_heap_count++;

	sift_up_sem_heap (stix, index);
	return 0;
}

static void delete_from_sem_heap (stix_t* stix, stix_ooi_t index)
{
	stix_oop_semaphore_t sem, lastsem;

	sem = stix->sem_heap[index];
	sem->heap_index = STIX_SMOOI_TO_OOP(-1);

	stix->sem_heap_count--;
	if (stix->sem_heap_count > 0 && index != stix->sem_heap_count)
	{
		/* move the last item to the deletion position */
		lastsem = stix->sem_heap[stix->sem_heap_count];
		lastsem->heap_index = STIX_SMOOI_TO_OOP(index);
		stix->sem_heap[index] = lastsem;

		if (SEM_HEAP_EARLIER_THAN(stix, lastsem, sem)) 
			sift_up_sem_heap (stix, index);
		else
			sift_down_sem_heap (stix, index);
	}
}

static void update_sem_heap (stix_t* stix, stix_ooi_t index, stix_oop_semaphore_t newsem)
{
	stix_oop_semaphore_t sem;

	sem = stix->sem_heap[index];
	sem->heap_index = STIX_SMOOI_TO_OOP(-1);

	newsem->heap_index = STIX_SMOOI_TO_OOP(index);
	stix->sem_heap[index] = newsem;

	if (SEM_HEAP_EARLIER_THAN(stix, newsem, sem))
		sift_up_sem_heap (stix, index);
	else
		sift_down_sem_heap (stix, index);
}

static stix_oop_process_t start_initial_process (stix_t* stix, stix_oop_context_t c)
{
	stix_oop_process_t proc;

	/* there must be no active process when this function is called */
	STIX_ASSERT (stix->processor->tally == STIX_SMOOI_TO_OOP(0));
	STIX_ASSERT (stix->processor->active == stix->nil_process);

	proc = make_process (stix, c);
	if (!proc) return STIX_NULL;

	if (chain_into_processor (stix, proc) <= -1) return STIX_NULL;
	proc->state = STIX_SMOOI_TO_OOP(PROC_STATE_RUNNING); /* skip RUNNABLE and go to RUNNING */
	stix->processor->active = proc;

	/* do somthing that resume_process() would do with less overhead */
	STIX_ASSERT ((stix_oop_t)proc->current_context != stix->_nil);
	STIX_ASSERT (proc->current_context == proc->initial_context);
	SWITCH_ACTIVE_CONTEXT (stix, proc->current_context);

	return proc;
}

static STIX_INLINE int activate_new_method (stix_t* stix, stix_oop_method_t mth, stix_ooi_t actual_nargs)
{
	stix_oop_context_t ctx;
	stix_ooi_t i, j;
	stix_ooi_t ntmprs, nargs, actual_ntmprs;

	ntmprs = STIX_OOP_TO_SMOOI(mth->tmpr_count);
	nargs = STIX_OOP_TO_SMOOI(mth->tmpr_nargs);

	STIX_ASSERT (ntmprs >= 0);
	STIX_ASSERT (nargs <= ntmprs);

	if (actual_nargs > nargs)
	{
		/* more arguments than the method specification have been passed in. 
		 * it must be a variadic unary method. othewise, the compiler is buggy */
		STIX_ASSERT (STIX_METHOD_GET_PREAMBLE_FLAGS(STIX_OOP_TO_SMOOI(mth->preamble)) & STIX_METHOD_PREAMBLE_FLAG_VARIADIC);
		actual_ntmprs = ntmprs + (actual_nargs - nargs);
	}
	else actual_ntmprs = ntmprs;

	stix_pushtmp (stix, (stix_oop_t*)&mth);
	ctx = (stix_oop_context_t)stix_instantiate (stix, stix->_method_context, STIX_NULL, actual_ntmprs);
	stix_poptmp (stix);
	if (!ctx) return -1;

	ctx->sender = stix->active_context; 
	ctx->ip = STIX_SMOOI_TO_OOP(0);
	/* ctx->sp will be set further down */

	/* A context is compose of a fixed part and a variable part.
	 * the variable part holds temporary varibles including arguments.
	 *
	 * Assuming a method context with 2 arguments and 3 local temporary
	 * variables, the context will look like this.
	 *   +---------------------+
	 *   | fixed part          |
	 *   |                     |
	 *   |                     |
	 *   |                     |
	 *   +---------------------+
	 *   | tmp1 (arg1)         | slot[0]
	 *   | tmp2 (arg2)         | slot[1]
	 *   | tmp3                | slot[2] 
	 *   | tmp4                | slot[3]
	 *   | tmp5                | slot[4]
	 *   +---------------------+
	 */

	ctx->ntmprs = STIX_SMOOI_TO_OOP(ntmprs);
	ctx->method_or_nargs = (stix_oop_t)mth;
	/* the 'home' field of a method context is always stix->_nil.
	ctx->home = stix->_nil;*/
	ctx->origin = ctx; /* point to self */

	/* 
	 * Assume this message sending expression:
	 *   obj1 do: #this with: #that with: #it
	 * 
	 * It would be compiled to these logical byte-code sequences shown below:
	 *   push obj1
	 *   push #this
	 *   push #that
	 *   push #it
	 *   send #do:with:
	 *
	 * After three pushes, the stack looks like this.
	 * 
	 *  | #it   | <- sp
	 *  | #that |    sp - 1  
	 *  | #this |    sp - 2
	 *  | obj1  |    sp - nargs
	 *
	 * Since the number of arguments is 3, stack[sp - 3] points to
	 * the receiver. When the stack is empty, sp is -1.
	 */
	if (actual_nargs >= nargs)
	{
		for (i = actual_nargs, j = ntmprs + (actual_nargs - nargs); i > nargs; i--)
		{
			/* place variadic arguments after local temporaries */
			ctx->slot[--j] = STIX_STACK_GETTOP (stix);
			STIX_STACK_POP (stix);
		}
		STIX_ASSERT (i == nargs);
		while (i > 0)
		{
			/* place normal argument before local temporaries */
			ctx->slot[--i] = STIX_STACK_GETTOP (stix);
			STIX_STACK_POP (stix);
		}
	}
	else
	{
		for (i = actual_nargs; i > 0; )
		{
			/* place normal argument before local temporaries */
			ctx->slot[--i] = STIX_STACK_GETTOP (stix);
			STIX_STACK_POP (stix);
		}
	}
	/* copy receiver */
	ctx->receiver_or_source = STIX_STACK_GETTOP (stix);
	STIX_STACK_POP (stix);

	STIX_ASSERT (stix->sp >= -1);

	/* the stack pointer in a context is a stack pointer of a process 
	 * before it is activated. this stack pointer is stored to the context
	 * so that it is used to restore the process stack pointer upon returning
	 * from a method context. */
	ctx->sp = STIX_SMOOI_TO_OOP(stix->sp);

	/* switch the active context to the newly instantiated one*/
	SWITCH_ACTIVE_CONTEXT (stix, ctx);

	return 0;
}

static stix_oop_method_t find_method (stix_t* stix, stix_oop_t receiver, const stix_oocs_t* message, int super)
{
	stix_oop_class_t cls;
	stix_oop_association_t ass;
	stix_oop_t c;
	stix_oop_set_t mthdic;
	int dic_no;
/* TODO: implement method lookup cache */

	cls = (stix_oop_class_t)STIX_CLASSOF(stix, receiver);
	if ((stix_oop_t)cls == stix->_class)
	{
		/* receiver is a class object (an instance of Class) */
		c = receiver; 
		dic_no = STIX_METHOD_CLASS;
	}
	else
	{
		/* receiver is not a class object. so take its class */
		c = (stix_oop_t)cls;
		dic_no = STIX_METHOD_INSTANCE;
	}

	STIX_ASSERT (c != stix->_nil);

	if (super) 
	{
		/*
		stix_oop_method_t m;
		STIX_ASSERT (STIX_CLASSOF(stix, stix->active_context->origin) == stix->_method_context);
		m = (stix_oop_method_t)stix->active_context->origin->method_or_nargs;
		c = ((stix_oop_class_t)m->owner)->superclass;
		*/
		STIX_ASSERT (stix->active_method);
		STIX_ASSERT (stix->active_method->owner);
		c = ((stix_oop_class_t)stix->active_method->owner)->superclass;
		if (c == stix->_nil) goto not_found; /* reached the top of the hierarchy */
	}

	do
	{
		mthdic = ((stix_oop_class_t)c)->mthdic[dic_no];
		STIX_ASSERT ((stix_oop_t)mthdic != stix->_nil);
		STIX_ASSERT (STIX_CLASSOF(stix, mthdic) == stix->_method_dictionary);

		ass = (stix_oop_association_t)stix_lookupdic (stix, mthdic, message);
		if (ass) 
		{
			/* found the method */
			STIX_ASSERT (STIX_CLASSOF(stix, ass->value) == stix->_method);
			return (stix_oop_method_t)ass->value;
		}
		c = ((stix_oop_class_t)c)->superclass;
	}
	while (c != stix->_nil);

not_found:
	if ((stix_oop_t)cls == stix->_class)
	{
		/* the object is an instance of Class. find the method
		 * in an instance method dictionary of Class also */
		mthdic = ((stix_oop_class_t)cls)->mthdic[STIX_METHOD_INSTANCE];
		STIX_ASSERT ((stix_oop_t)mthdic != stix->_nil);
		STIX_ASSERT (STIX_CLASSOF(stix, mthdic) == stix->_method_dictionary);

		ass = (stix_oop_association_t)stix_lookupdic (stix, mthdic, message);
		if (ass) 
		{
			STIX_ASSERT (STIX_CLASSOF(stix, ass->value) == stix->_method);
			return (stix_oop_method_t)ass->value;
		}
	}

	STIX_DEBUG3 (stix, "Method [%.*S] not found for %O\n", message->len, message->ptr, receiver);
	stix->errnum = STIX_ENOENT;
	return STIX_NULL;
}

static int start_initial_process_and_context (stix_t* stix, const stix_oocs_t* objname, const stix_oocs_t* mthname)
{
	/* the initial context is a fake context. if objname is 'Stix' and
	 * mthname is 'main', this function emulates message sending 'Stix main'.
	 * it should emulate the following logical byte-code sequences:
	 *
	 *    push Stix
	 *    send #main
	 */
	stix_oop_context_t ctx;
	stix_oop_association_t ass;
	stix_oop_method_t mth;
	stix_oop_process_t proc;

	/* create a fake initial context. */
	ctx = (stix_oop_context_t)stix_instantiate (stix, stix->_method_context, STIX_NULL, 0);
	if (!ctx) return -1;

	ass = stix_lookupsysdic (stix, objname);
	if (!ass) return -1;

	mth = find_method (stix, ass->value, mthname, 0);
	if (!mth) return -1;

	if (STIX_OOP_TO_SMOOI(mth->tmpr_nargs) > 0)
	{
		/* this method expects more than 0 arguments. 
		 * i can't use it as a start-up method.
TODO: overcome this problem 
		 */
		stix->errnum = STIX_EINVAL;
		return -1;
	}

/* TODO: handle preamble */

	/* the initial context starts the life of the entire VM
	 * and is not really worked on except that it is used to call the
	 * initial method. so it doesn't really require any extra stack space. */
/* TODO: verify this theory of mine. */
	stix->ip = 0;
	stix->sp = -1;

	ctx->ip = STIX_SMOOI_TO_OOP(0); /* point to the beginning */
	ctx->sp = STIX_SMOOI_TO_OOP(-1); /* pointer to -1 below the bottom */
	ctx->origin = ctx; /* point to self */
	ctx->method_or_nargs = (stix_oop_t)mth; /* fake. help SWITCH_ACTIVE_CONTEXT() not fail. TODO: create a static fake method and use it... instead of 'mth' */

	/* [NOTE]
	 *  the receiver field and the sender field of ctx are nils.
	 *  especially, the fact that the sender field is nil is used by 
	 *  the main execution loop for breaking out of the loop */

	STIX_ASSERT (stix->active_context == STIX_NULL);
	STIX_ASSERT (stix->active_method == STIX_NULL);

	/* stix_gc() uses stix->processor when stix->active_context
	 * is not NULL. at this poinst, stix->processor should point to
	 * an instance of ProcessScheduler. */
	STIX_ASSERT ((stix_oop_t)stix->processor != stix->_nil);
	STIX_ASSERT (stix->processor->tally == STIX_SMOOI_TO_OOP(0));

	/* start_initial_process() calls the SWITCH_ACTIVE_CONTEXT() macro.
	 * the macro assumes a non-null value in stix->active_context.
	 * let's force set active_context to ctx directly. */
	stix->active_context = ctx;

	stix_pushtmp (stix, (stix_oop_t*)&ctx);
	stix_pushtmp (stix, (stix_oop_t*)&mth);
	stix_pushtmp (stix, (stix_oop_t*)&ass);
	proc = start_initial_process (stix, ctx); 
	stix_poptmps (stix, 3);
	if (!proc) return -1;

	STIX_STACK_PUSH (stix, ass->value); /* push the receiver - the object referenced by 'objname' */
	STORE_ACTIVE_SP (stix); /* stix->active_context->sp = STIX_SMOOI_TO_OOP(stix->sp) */

	STIX_ASSERT (stix->processor->active == proc);
	STIX_ASSERT (stix->processor->active->initial_context == ctx);
	STIX_ASSERT (stix->processor->active->current_context == ctx);
	STIX_ASSERT (stix->active_context == ctx);

	/* emulate the message sending */
	return activate_new_method (stix, mth, 0);
}

/* ------------------------------------------------------------------------- */
static int pf_dump (stix_t* stix, stix_ooi_t nargs)
{
	stix_ooi_t i;

	STIX_ASSERT (nargs >=  0);

	stix_logbfmt (stix, 0, "RECEIVER: %O\n", STIX_STACK_GET(stix, stix->sp - nargs));
	for (i = nargs; i > 0; )
	{
		--i;
		stix_logbfmt (stix, 0, "ARGUMENT %zd: %O\n", i, STIX_STACK_GET(stix, stix->sp - i));
	}

	STIX_STACK_SETRETTORCV (stix, nargs); /* ^self */
	return 1; /* success */
}

static void log_char_object (stix_t* stix, stix_oow_t mask, stix_oop_char_t msg)
{
	stix_ooi_t n;
	stix_oow_t rem;
	const stix_ooch_t* ptr;

	STIX_ASSERT (STIX_OBJ_GET_FLAGS_TYPE(msg) == STIX_OBJ_TYPE_CHAR);

	rem = STIX_OBJ_GET_SIZE(msg);
	ptr = msg->slot;

start_over:
	while (rem > 0)
	{
		if (*ptr == '\0') 
		{
			n = stix_logbfmt (stix, mask, "%C", *ptr);
			STIX_ASSERT (n == 1);
			rem -= n;
			ptr += n;
			goto start_over;
		}

		n = stix_logbfmt (stix, mask, "%.*S", rem, ptr);
		if (n <= -1) break;
		if (n == 0) 
		{
			/* to skip the unprinted character. 
			 * actually, this check is not needed because of '\0' skipping
			 * at the beginning  of the loop */
			n = stix_logbfmt (stix, mask, "%C", *ptr);
			STIX_ASSERT (n == 1);
		}
		rem -= n;
		ptr += n;
	}
}

static int pf_log (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t msg, level;
	stix_oow_t mask;
	stix_ooi_t k;

	STIX_ASSERT (nargs >=  2);

	level = STIX_STACK_GETARG(stix, nargs, 0);
	if (!STIX_OOP_IS_SMOOI(level)) mask = STIX_LOG_APP | STIX_LOG_INFO; 
	else mask = STIX_LOG_APP | STIX_OOP_TO_SMOOI(level);

	for (k = 1; k < nargs; k++)
	{
		msg = STIX_STACK_GETARG (stix, nargs, k);

		if (msg == stix->_nil || msg == stix->_true || msg == stix->_false) 
		{
			goto dump_object;
		}
		else if (STIX_OOP_IS_POINTER(msg))
		{
			if (STIX_OBJ_GET_FLAGS_TYPE(msg) == STIX_OBJ_TYPE_CHAR)
			{
				log_char_object (stix, mask, (stix_oop_char_t)msg);
			}
			else if (STIX_OBJ_GET_FLAGS_TYPE(msg) == STIX_OBJ_TYPE_OOP)
			{
				/* visit only 1-level down into an array-like object */
				stix_oop_t inner, _class;
				stix_oow_t i, spec;

				_class = STIX_CLASSOF(stix, msg);

				spec = STIX_OOP_TO_SMOOI(((stix_oop_class_t)_class)->spec);
				if (STIX_CLASS_SPEC_NAMED_INSTVAR(spec) > 0 || !STIX_CLASS_SPEC_IS_INDEXED(spec)) goto dump_object;

				for (i = 0; i < STIX_OBJ_GET_SIZE(msg); i++)
				{
					inner = ((stix_oop_oop_t)msg)->slot[i];

					if (i > 0) stix_logbfmt (stix, mask, " ");
					if (STIX_OOP_IS_POINTER(inner) &&
					    STIX_OBJ_GET_FLAGS_TYPE(inner) == STIX_OBJ_TYPE_CHAR)
					{
						log_char_object (stix, mask, (stix_oop_char_t)inner);
					}
					else
					{
						stix_logbfmt (stix, mask, "%O", inner);
					}
				}
			}
			else goto dump_object;
		}
		else
		{
		dump_object:
			stix_logbfmt (stix, mask, "%O", msg);
		}
	}

	STIX_STACK_SETRETTORCV (stix, nargs); /* ^self */
	return 1;
}

static int pf_identical (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, b;

	STIX_ASSERT (nargs == 1);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	arg = STIX_STACK_GETARG(stix, nargs, 0);

	b = (rcv == arg)? stix->_true: stix->_false;

	STIX_STACK_SETRET (stix, nargs, b);
	return 1;
}

static int pf_not_identical (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, b;

	STIX_ASSERT (nargs == 1);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	arg = STIX_STACK_GETARG(stix, nargs, 0);

	b = (rcv != arg)? stix->_true: stix->_false;

	STIX_STACK_SETRET (stix, nargs, b);
	return 1;
}

static int pf_class (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, c;

	STIX_ASSERT (nargs ==  0);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	c = STIX_CLASSOF(stix, rcv);

	STIX_STACK_SETRET (stix, nargs, c);
	return 1; /* success */
}

static int pf_basic_new (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, obj;

	STIX_ASSERT (nargs ==  0);

	rcv = STIX_STACK_GETRCV (stix, nargs);
	if (STIX_CLASSOF(stix, rcv) != stix->_class) 
	{
		/* the receiver is not a class object */
		return 0;
	}

	obj = stix_instantiate (stix, rcv, STIX_NULL, 0);
	if (!obj) return -1;

	STIX_STACK_SETRET (stix, nargs, obj);
	return 1; /* success */
}

static int pf_basic_new_with_size (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, szoop, obj;
	stix_oow_t size;

	STIX_ASSERT (nargs ==  1);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	if (STIX_CLASSOF(stix, rcv) != stix->_class) 
	{
		/* the receiver is not a class object */
		return 0;
	}

	szoop = STIX_STACK_GETARG(stix, nargs, 0);
	if (stix_inttooow (stix, szoop, &size) <= 0)
	{
		/* integer out of range or not integer */
		return 0;
	}

	/* stix_instantiate() ignores size if the instance specification 
	 * disallows indexed(variable) parts. */
	/* TODO: should i check the specification before calling 
	 *       stix_instantiate()? */
	obj = stix_instantiate (stix, rcv, STIX_NULL, size);
	if (!obj) 
	{
		return -1; /* hard failure */
	}

	STIX_STACK_SETRET (stix, nargs, obj);
	return 1; /* success */
}

static int pf_ngc_new (stix_t* stix, stix_ooi_t nargs)
{
	int n;

	n = pf_basic_new (stix, nargs);
	if (n <= 0) return n;

	return 1;
}

static int pf_ngc_new_with_size (stix_t* stix, stix_ooi_t nargs)
{
	int n;

	n = pf_basic_new_with_size (stix, nargs);
	if (n <= 0) return n;

	return 1;
}

static int pf_ngc_dispose (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv;

	STIX_ASSERT (nargs ==  0);
	rcv = STIX_STACK_GETRCV (stix, nargs);

	stix_freemem (stix, rcv);

	STIX_STACK_SETRET (stix, nargs, stix->_nil);
	return 1; /* success */
}

static int pf_shallow_copy (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, obj;

	STIX_ASSERT (nargs ==  0);

	rcv = STIX_STACK_GETRCV (stix, nargs);

	obj = stix_shallowcopy (stix, rcv);
	if (!obj) return -1;

	/* emulate 'pop receiver' and 'push result' */
	STIX_STACK_SETRET (stix, nargs, obj);
	return 1; /* success */
}

static int pf_basic_size (stix_t* stix, stix_ooi_t nargs)
{
	/* return the number of indexable fields */

	stix_oop_t rcv, sz;

	STIX_ASSERT (nargs == 0);

	rcv = STIX_STACK_GETRCV (stix, nargs);

	if (!STIX_OOP_IS_POINTER(rcv))
	{
		sz = STIX_SMOOI_TO_OOP(0);
	}
	else
	{
		sz = stix_oowtoint (stix, STIX_OBJ_GET_SIZE(rcv));
		if (!sz) return -1; /* hard failure */
	}

	STIX_STACK_SETRET(stix, nargs, sz);
	return 1;
}

static int pf_basic_at (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, pos, v;
	stix_oow_t idx;

	STIX_ASSERT (nargs == 1);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	if (!STIX_OOP_IS_POINTER(rcv))
	{
		/* the receiver is a special numeric object, not a normal pointer */
		return 0;
	}

	pos = STIX_STACK_GETARG(stix, nargs, 0);
	if (stix_inttooow (stix, pos, &idx) <= 0)
	{
		/* negative integer or not integer */
		return 0;
	}
	if (idx >= STIX_OBJ_GET_SIZE(rcv))
	{
		/* index out of range */
		return 0;
	}

	switch (STIX_OBJ_GET_FLAGS_TYPE(rcv))
	{
		case STIX_OBJ_TYPE_BYTE:
			v = STIX_SMOOI_TO_OOP(((stix_oop_byte_t)rcv)->slot[idx]);
			break;

		case STIX_OBJ_TYPE_CHAR:
			v = STIX_CHAR_TO_OOP(((stix_oop_char_t)rcv)->slot[idx]);
			break;

		case STIX_OBJ_TYPE_HALFWORD:
			/* TODO: LargeInteger if the halfword is too large */
			v = STIX_SMOOI_TO_OOP(((stix_oop_halfword_t)rcv)->slot[idx]);
			break;

		case STIX_OBJ_TYPE_WORD:
			/* TODO: LargeInteger if the word is too large */
			v = STIX_SMOOI_TO_OOP(((stix_oop_word_t)rcv)->slot[idx]);
			break;

		case STIX_OBJ_TYPE_OOP:
			v = ((stix_oop_oop_t)rcv)->slot[idx];
			break;

		default:
			stix->errnum = STIX_EINTERN;
			return -1;
	}

	STIX_STACK_SETRET (stix, nargs, v);
	return 1;
}

static int pf_basic_at_put (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, pos, val;
	stix_oow_t idx;

	STIX_ASSERT (nargs == 2);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	if (!STIX_OOP_IS_POINTER(rcv))
	{
		/* the receiver is a special numeric object, not a normal pointer */
		return 0;
	}
	pos = STIX_STACK_GETARG(stix, nargs, 0);
	val = STIX_STACK_GETARG(stix, nargs, 1);

	if (stix_inttooow (stix, pos, &idx) <= 0)
	{
		/* negative integer or not integer */
		return 0;
	}
	if (idx >= STIX_OBJ_GET_SIZE(rcv))
	{
		/* index out of range */
		return 0;
	}

	if (STIX_OBJ_GET_CLASS(rcv) == stix->_symbol)
	{
/* TODO: disallow change of some key kernel objects???? */
		/* TODO: is it better to introduct a read-only mark in the object header instead of this class check??? */
		/* read-only object */ /* TODO: DEVISE A WAY TO PASS a proper error from the primitive handler to STIX */
		return 0;
	}

	switch (STIX_OBJ_GET_FLAGS_TYPE(rcv))
	{
		case STIX_OBJ_TYPE_BYTE:
			if (!STIX_OOP_IS_SMOOI(val))
			{
				/* the value is not a character */
				return 0;
			}
/* TOOD: must I check the range of the value? */
			((stix_oop_char_t)rcv)->slot[idx] = STIX_OOP_TO_SMOOI(val);
			break;

		case STIX_OBJ_TYPE_CHAR:
			if (!STIX_OOP_IS_CHAR(val))
			{
				/* the value is not a character */
				return 0;
			}
			((stix_oop_char_t)rcv)->slot[idx] = STIX_OOP_TO_CHAR(val);
			break;

		case STIX_OBJ_TYPE_HALFWORD:
			if (!STIX_OOP_IS_SMOOI(val))
			{
				/* the value is not a number */
				return 0;
			}

			/* if the small integer is too large, it will get truncated */
			((stix_oop_halfword_t)rcv)->slot[idx] = STIX_OOP_TO_SMOOI(val);
			break;

		case STIX_OBJ_TYPE_WORD:
			/* TODO: handle LargeInteger */
			if (!STIX_OOP_IS_SMOOI(val))
			{
				/* the value is not a number */
				return 0;
			}
			((stix_oop_word_t)rcv)->slot[idx] = STIX_OOP_TO_SMOOI(val);
			break;

		case STIX_OBJ_TYPE_OOP:
			((stix_oop_oop_t)rcv)->slot[idx] = val;
			break;

		default:
			stix->errnum = STIX_EINTERN;
			return -1;
	}

/* TODO: return receiver or value? */
	STIX_STACK_SETRET (stix, nargs, val);
	return 1;
}

static int pf_context_goto (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv;
	stix_oop_t pc;

	/* this primivie provides the similar functionality to  MethodContext>>pc:
	 * except that it pops the receiver and arguments and doesn't push a
	 * return value. it's useful when you want to change the instruction
	 * pointer while maintaining the stack level before the call */

	STIX_ASSERT (nargs == 1);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	if (STIX_CLASSOF(stix, rcv) != stix->_method_context)
	{
		STIX_LOG2 (stix, STIX_LOG_PRIMITIVE | STIX_LOG_ERROR, 
			"Error(%hs) - invalid receiver, not a method context - %O\n", __PRIMITIVE_NAME__, rcv);
		return 0;
	}

	pc = STIX_STACK_GETARG(stix, nargs, 0);
	if (!STIX_OOP_IS_SMOOI(pc) || STIX_OOP_TO_SMOOI(pc) < 0)
	{
		STIX_LOG1 (stix, STIX_LOG_PRIMITIVE | STIX_LOG_ERROR,
			"Error(%hs) - invalid pc\n", __PRIMITIVE_NAME__);
		return 0;
	}

	((stix_oop_context_t)rcv)->ip = pc;
	LOAD_ACTIVE_IP (stix);

	STIX_ASSERT (nargs + 1 == 2);
	STIX_STACK_POPS (stix, 2); /* pop both the argument and the receiver */
	return 1;
}

static int __block_value (stix_t* stix, stix_oop_context_t rcv_blkctx, stix_ooi_t nargs, stix_ooi_t num_first_arg_elems, stix_oop_context_t* pblkctx)
{
	/* prepare a new block context for activation.
	 * the receiver must be a block context which becomes the base
	 * for a new block context. */

	stix_oop_context_t blkctx;
	stix_ooi_t local_ntmprs, i;
	stix_ooi_t actual_arg_count;

	actual_arg_count = (num_first_arg_elems > 0)? num_first_arg_elems: nargs;

	/* TODO: find a better way to support a reentrant block context. */

	/* | sum |
	 * sum := [ :n | (n < 2) ifTrue: [1] ifFalse: [ n + (sum value: (n - 1))] ].
	 * (sum value: 10).
	 * 
	 * For the code above, sum is a block context and it is sent value: inside
	 * itself. Let me simply clone a block context to allow reentrancy like this
	 * while the block context is active
	 */

	/* the receiver must be a block context */
	STIX_ASSERT (STIX_CLASSOF(stix, rcv_blkctx) == stix->_block_context);
	if (rcv_blkctx->receiver_or_source != stix->_nil)
	{
		/* the 'source' field is not nil.
		 * this block context has already been activated once.
		 * you can't send 'value' again to reactivate it.
		 * For example, [thisContext value] value. */
		STIX_ASSERT (STIX_OBJ_GET_SIZE(rcv_blkctx) > STIX_CONTEXT_NAMED_INSTVARS);
		STIX_LOG2 (stix, STIX_LOG_PRIMITIVE | STIX_LOG_ERROR, 
			"Error(%hs) - re-valuing of a block context - %O\n", __PRIMITIVE_NAME__, rcv_blkctx);
		return 0;
	}
	STIX_ASSERT (STIX_OBJ_GET_SIZE(rcv_blkctx) == STIX_CONTEXT_NAMED_INSTVARS);

	if (STIX_OOP_TO_SMOOI(rcv_blkctx->method_or_nargs) != actual_arg_count /* nargs */)
	{
		STIX_LOG4 (stix, STIX_LOG_PRIMITIVE | STIX_LOG_ERROR, 
			"Error(%hs) - wrong number of arguments to a block context %O - expecting %zd, got %zd\n",
			__PRIMITIVE_NAME__, rcv_blkctx, STIX_OOP_TO_SMOOI(rcv_blkctx->method_or_nargs), actual_arg_count);
		return 0;
	}

	/* the number of temporaries stored in the block context
	 * accumulates the number of temporaries starting from the origin.
	 * simple calculation is needed to find the number of local temporaries */
	local_ntmprs = STIX_OOP_TO_SMOOI(rcv_blkctx->ntmprs) -
	               STIX_OOP_TO_SMOOI(((stix_oop_context_t)rcv_blkctx->home)->ntmprs);
	STIX_ASSERT (local_ntmprs >= actual_arg_count);

	/* create a new block context to clone rcv_blkctx */
	stix_pushtmp (stix, (stix_oop_t*)&rcv_blkctx);
	blkctx = (stix_oop_context_t) stix_instantiate (stix, stix->_block_context, STIX_NULL, local_ntmprs); 
	stix_poptmp (stix);
	if (!blkctx) return -1;

#if 0
	/* shallow-copy the named part including home, origin, etc. */
	for (i = 0; i < STIX_CONTEXT_NAMED_INSTVARS; i++)
	{
		((stix_oop_oop_t)blkctx)->slot[i] = ((stix_oop_oop_t)rcv_blkctx)->slot[i];
	}
#else
	blkctx->ip = rcv_blkctx->ip;
	blkctx->ntmprs = rcv_blkctx->ntmprs;
	blkctx->method_or_nargs = rcv_blkctx->method_or_nargs;
	blkctx->receiver_or_source = (stix_oop_t)rcv_blkctx;
	blkctx->home = rcv_blkctx->home;
	blkctx->origin = rcv_blkctx->origin;
#endif

/* TODO: check the stack size of a block context to see if it's large enough to hold arguments */
	if (num_first_arg_elems > 0)
	{
		/* the first argument should be an array. this function is ordered
		 * to pass array elements to the new block */
		stix_oop_oop_t xarg;
		STIX_ASSERT (nargs == 1);
		xarg = (stix_oop_oop_t)STIX_STACK_GETTOP (stix);
		STIX_ASSERT (STIX_ISTYPEOF(stix,xarg,STIX_OBJ_TYPE_OOP)); 
		STIX_ASSERT (STIX_OBJ_GET_SIZE(xarg) == num_first_arg_elems); 
		for (i = 0; i < num_first_arg_elems; i++)
		{
			blkctx->slot[i] = xarg->slot[i];
		}
	}
	else
	{
		/* copy the arguments to the stack */
		for (i = 0; i < nargs; i++)
		{
			blkctx->slot[i] = STIX_STACK_GETARG(stix, nargs, i);
		}
	}
	STIX_STACK_POPS (stix, nargs + 1); /* pop arguments and receiver */

	STIX_ASSERT (blkctx->home != stix->_nil);
	blkctx->sp = STIX_SMOOI_TO_OOP(-1); /* not important at all */
	blkctx->sender = stix->active_context;

	*pblkctx = blkctx;
	return 1;
}

static int pf_block_value (stix_t* stix, stix_ooi_t nargs)
{
	int x;
	stix_oop_context_t rcv_blkctx, blkctx;

	rcv_blkctx = (stix_oop_context_t)STIX_STACK_GETRCV(stix, nargs);
	if (STIX_CLASSOF(stix, rcv_blkctx) != stix->_block_context)
	{
		/* the receiver must be a block context */
		STIX_LOG2 (stix, STIX_LOG_PRIMITIVE | STIX_LOG_ERROR, 
			"Error(%hs) - invalid receiver, not a block context - %O\n", __PRIMITIVE_NAME__, rcv_blkctx);
		return 0;
	}

	x = __block_value (stix, rcv_blkctx, nargs, 0, &blkctx);
	if (x <= 0) return x; /* hard failure and soft failure */

	SWITCH_ACTIVE_CONTEXT (stix, (stix_oop_context_t)blkctx);
	return 1;
}

static int pf_block_new_process (stix_t* stix, stix_ooi_t nargs)
{
	/* create a new process from a block context.
	 * the receiver must be be a block.
	 *   [ 1 + 2 ] newProcess.
	 *   [ :a :b | a + b ] newProcess: #(1 2)
	 */

	int x;
	stix_oop_context_t rcv_blkctx, blkctx;
	stix_oop_process_t proc;
	stix_ooi_t num_first_arg_elems = 0;

	if (nargs > 1)
	{
		/* too many arguments */
/* TODO: proper error handling */
		return 0;
	}

	if (nargs == 1)
	{
		stix_oop_t xarg;

		xarg = STIX_STACK_GETARG(stix, nargs, 0);
		if (!STIX_ISTYPEOF(stix,xarg,STIX_OBJ_TYPE_OOP))
		{
			/* the only optional argument must be an OOP-indexable 
			 * object like an array */
			return 0;
		}

		num_first_arg_elems = STIX_OBJ_GET_SIZE(xarg);
	}

	rcv_blkctx = (stix_oop_context_t)STIX_STACK_GETRCV(stix, nargs);
	if (STIX_CLASSOF(stix, rcv_blkctx) != stix->_block_context)
	{
		/* the receiver must be a block context */
		STIX_LOG2 (stix, STIX_LOG_PRIMITIVE | STIX_LOG_ERROR, 
			"Error(%hs) - invalid receiver, not a block context - %O\n", __PRIMITIVE_NAME__, rcv_blkctx);
		return 0;
	}

	/* this primitive creates a new process with a block as if the block
	 * is sent the value message */
	x = __block_value (stix, rcv_blkctx, nargs, num_first_arg_elems, &blkctx);
	if (x <= 0) return x; /* both hard failure and soft failure */

	/* reset the sender field to stix->_nil because this block context
	 * will be the initial context of a new process. you can simply
	 * inspect the sender field to see if a context is an initial
	 * context of a process. */
	blkctx->sender = (stix_oop_context_t)stix->_nil;

	proc = make_process (stix, blkctx);
	if (!proc) return -1; /* hard failure */ /* TOOD: can't this be treated as a soft failure? */

	/* __block_value() has popped all arguments and the receiver. 
	 * PUSH the return value instead of changing the stack top */
	STIX_STACK_PUSH (stix, (stix_oop_t)proc);
	return 1;
}

static int pf_process_resume (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv;
	STIX_ASSERT (nargs == 0);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	if (STIX_CLASSOF(stix,rcv) != stix->_process) return 0;

	resume_process (stix, (stix_oop_process_t)rcv); /* TODO: error check */

	/* keep the receiver in the stack top */
	return 1;
}

static int pf_process_terminate (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv;
	STIX_ASSERT (nargs == 0);

/* TODO: need to run ensure blocks here..
 * when it's executed here. it does't have to be in Exception>>handleException when there is no exception handler */
	rcv = STIX_STACK_GETRCV(stix, nargs);
	if (STIX_CLASSOF(stix,rcv) != stix->_process) return 0;

	terminate_process (stix, (stix_oop_process_t)rcv);

	/* keep the receiver in the stack top */
	return 1;
}

static int pf_process_yield (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv;
	STIX_ASSERT (nargs == 0);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	if (STIX_CLASSOF(stix,rcv) != stix->_process) return 0;

	yield_process (stix, (stix_oop_process_t)rcv);

	/* keep the receiver in the stack top */
	return 1;
}

static int pf_process_suspend (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv;
	STIX_ASSERT (nargs == 0);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	if (STIX_CLASSOF(stix,rcv) != stix->_process) return 0;

	suspend_process (stix, (stix_oop_process_t)rcv);

	/* keep the receiver in the stack top */
	return 1;
}

static int pf_semaphore_signal (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv;
	STIX_ASSERT (nargs == 0);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	if (STIX_CLASSOF(stix,rcv) != stix->_semaphore) return 0;

	signal_semaphore (stix, (stix_oop_semaphore_t)rcv);

	/* keep the receiver in the stack top */
	return 1;
}

static int pf_semaphore_wait (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv;
	STIX_ASSERT (nargs == 0);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	if (STIX_CLASSOF(stix,rcv) != stix->_semaphore) return 0;

	await_semaphore (stix, (stix_oop_semaphore_t)rcv);

	/* keep the receiver in the stack top */
	return 1;
}

static int pf_processor_schedule (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg;

	STIX_ASSERT (nargs == 1);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	arg = STIX_STACK_GETARG(stix, nargs, 0);

	if (rcv != (stix_oop_t)stix->processor || STIX_CLASSOF(stix,arg) != stix->_process)
	{
		return 0;
	}

	resume_process (stix, (stix_oop_process_t)arg); /* TODO: error check */
	return 1;
}

static int pf_processor_add_timed_semaphore (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, sec, nsec;
	stix_oop_semaphore_t sem;
	stix_ntime_t now, ft;

	STIX_ASSERT (nargs >= 2 || nargs <= 3);

	if (nargs == 3) 
	{
		nsec = STIX_STACK_GETARG (stix, nargs, 2);
		if (!STIX_OOP_IS_SMOOI(nsec)) return 0;
	}
	else nsec = STIX_SMOOI_TO_OOP(0);

	sec = STIX_STACK_GETARG(stix, nargs, 1);
	sem = (stix_oop_semaphore_t)STIX_STACK_GETARG(stix, nargs, 0);
	rcv = STIX_STACK_GETRCV(stix, nargs);

	if (rcv != (stix_oop_t)stix->processor) return 0;
	if (STIX_CLASSOF(stix,sem) != stix->_semaphore) return 0;
	if (!STIX_OOP_IS_SMOOI(sec)) return 0;

	if (STIX_OOP_IS_SMOOI(sem->heap_index) && 
	    sem->heap_index != STIX_SMOOI_TO_OOP(-1))
	{
		delete_from_sem_heap (stix, STIX_OOP_TO_SMOOI(sem->heap_index));
		STIX_ASSERT(sem->heap_index == STIX_SMOOI_TO_OOP(-1));

		/*
		Is this more desired???
		STIX_STACK_SETRET (stix, nargs, stix->_false);
		return 1;
		*/
	}

/* TODO: make clock_gettime to be platform independent 
 * 
 * this code assumes that the monotonic clock returns a small value
 * that can fit into a SmallInteger, even after some additions... */
	vm_gettime (stix, &now);
	STIX_ADDNTIMESNS (&ft, &now, STIX_OOP_TO_SMOOI(sec), STIX_OOP_TO_SMOOI(nsec));
	if (ft.sec < 0 || ft.sec > STIX_SMOOI_MAX) 
	{
		/* soft error - cannot represent the expiry time in
		 *              a small integer. */
		STIX_LOG3 (stix, STIX_LOG_PRIMITIVE | STIX_LOG_ERROR, 
			"Error(%hs) - time (%ld) out of range(0 - %zd) when adding a timed semaphore\n", 
			__PRIMITIVE_NAME__, (unsigned long int)ft.sec, (stix_ooi_t)STIX_SMOOI_MAX);
		return 0;
	}

	sem->heap_ftime_sec = STIX_SMOOI_TO_OOP(ft.sec);
	sem->heap_ftime_nsec = STIX_SMOOI_TO_OOP(ft.nsec);

	if (add_to_sem_heap (stix, sem) <= -1) return -1;

	STIX_STACK_SETRETTORCV (stix, nargs); /* ^self */
	return 1;
}

static int pf_processor_remove_semaphore (stix_t* stix, stix_ooi_t nargs)
{
	/* remove a semaphore from processor's signal scheduling */

	stix_oop_t rcv;
	stix_oop_semaphore_t sem;

	STIX_ASSERT (nargs == 1);

	sem = (stix_oop_semaphore_t)STIX_STACK_GETARG(stix, nargs, 0);
	rcv = STIX_STACK_GETRCV(stix, nargs);

/* TODO: remove a semaphore from IO handler if it's registered...
 *       remove a semaphore from XXXXXXXXXXXXXX */

	if (rcv != (stix_oop_t)stix->processor) return 0;
	if (STIX_CLASSOF(stix,sem) != stix->_semaphore) return 0;

	if (STIX_OOP_IS_SMOOI(sem->heap_index) && 
	    sem->heap_index != STIX_SMOOI_TO_OOP(-1))
	{
		/* the semaphore is in the timed semaphore heap */
		delete_from_sem_heap (stix, STIX_OOP_TO_SMOOI(sem->heap_index));
		STIX_ASSERT(sem->heap_index == STIX_SMOOI_TO_OOP(-1));
	}

	STIX_STACK_SETRETTORCV (stix, nargs); /* ^self */
	return 1;
}

static int pf_processor_return_to (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, ret, ctx;

	STIX_ASSERT (nargs == 2);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	ret = STIX_STACK_GETARG(stix, nargs, 0);
	ctx = STIX_STACK_GETARG(stix, nargs, 1);

	if (rcv != (stix_oop_t)stix->processor) return 0;

	if (STIX_CLASSOF(stix, ctx) != stix->_block_context &&
	    STIX_CLASSOF(stix, ctx) != stix->_method_context) return 0;

	STIX_STACK_POPS (stix, nargs + 1); /* pop arguments and receiver */
/* TODO: verify if this is correct? does't it correct restore the stack pointer?
 *       test complex chains of method contexts and block contexts */
	if (STIX_CLASSOF(stix, ctx) == stix->_method_context)
	{
		/* when returning to a method context, load the sp register with 
		 * the value stored in the context */
		stix->sp = STIX_OOP_TO_SMOOI(((stix_oop_context_t)ctx)->sp);
	}
	STIX_STACK_PUSH (stix, ret);

	SWITCH_ACTIVE_CONTEXT (stix, (stix_oop_context_t)ctx);
	return 1;
}

static int pf_integer_add (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, res;

	STIX_ASSERT (nargs == 1);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	arg = STIX_STACK_GETARG(stix, nargs, 0);

	res = stix_addints (stix, rcv, arg);
	if (!res) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */

	STIX_STACK_SETRET (stix, nargs, res);
	return 1;
}

static int pf_integer_sub (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, res;

	STIX_ASSERT (nargs == 1);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	arg = STIX_STACK_GETARG(stix, nargs, 0);

	res = stix_subints (stix, rcv, arg);
	if (!res) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */

	STIX_STACK_SETRET (stix, nargs, res);
	return 1;
}

static int pf_integer_mul (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, res;

	STIX_ASSERT (nargs == 1);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	arg = STIX_STACK_GETARG(stix, nargs, 0);

	res = stix_mulints (stix, rcv, arg);
	if (!res) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */

	STIX_STACK_SETRET (stix, nargs, res);
	return 1;
}

static int pf_integer_quo (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, quo;

	STIX_ASSERT (nargs == 1);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	arg = STIX_STACK_GETARG(stix, nargs, 0);

	quo = stix_divints (stix, rcv, arg, 0, STIX_NULL);
	if (!quo) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */
/* TODO: STIX_EDIVBY0 soft or hard failure? */

	STIX_STACK_SETRET (stix, nargs, quo);
	return 1;
}

static int pf_integer_rem (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, quo, rem;

	STIX_ASSERT (nargs == 1);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	arg = STIX_STACK_GETARG(stix, nargs, 0);

	quo = stix_divints (stix, rcv, arg, 0, &rem);
	if (!quo) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */
/* TODO: STIX_EDIVBY0 soft or hard failure? */

	STIX_STACK_SETRET (stix, nargs, rem);
	return 1;
}

static int pf_integer_quo2 (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, quo;

	STIX_ASSERT (nargs == 1);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	arg = STIX_STACK_GETARG(stix, nargs, 0);

	quo = stix_divints (stix, rcv, arg, 1, STIX_NULL);
	if (!quo) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */
/* TODO: STIX_EDIVBY0 soft or hard failure? */

	STIX_STACK_SETRET (stix, nargs, quo);
	return 1;
}

static int pf_integer_rem2 (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, quo, rem;

	STIX_ASSERT (nargs == 1);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	arg = STIX_STACK_GETARG(stix, nargs, 0);

	quo = stix_divints (stix, rcv, arg, 1, &rem);
	if (!quo) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */
/* TODO: STIX_EDIVBY0 soft or hard failure? */

	STIX_STACK_SETRET (stix, nargs, rem);
	return 1;
}

static int pf_integer_negated (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, res;

	STIX_ASSERT (nargs == 0);

	rcv = STIX_STACK_GETRCV(stix, nargs);

	res = stix_negateint (stix, rcv);
	if (!res) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */

	STIX_STACK_SETRET (stix, nargs, res);
	return 1;
}

static int pf_integer_bitat (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, res;

	STIX_ASSERT (nargs == 1);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	arg = STIX_STACK_GETARG(stix, nargs, 0);

	res = stix_bitatint (stix, rcv, arg);
	if (!res) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */

	STIX_STACK_SETRET (stix, nargs, res);
	return 1;
}

static int pf_integer_bitand (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, res;

	STIX_ASSERT (nargs == 1);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	arg = STIX_STACK_GETARG(stix, nargs, 0);

	res = stix_bitandints (stix, rcv, arg);
	if (!res) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */

	STIX_STACK_SETRET (stix, nargs, res);
	return 1;
}

static int pf_integer_bitor (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, res;

	STIX_ASSERT (nargs == 1);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	arg = STIX_STACK_GETARG(stix, nargs, 0);

	res = stix_bitorints (stix, rcv, arg);
	if (!res) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */

	STIX_STACK_SETRET (stix, nargs, res);
	return 1;
}

static int pf_integer_bitxor (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, res;

	STIX_ASSERT (nargs == 1);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	arg = STIX_STACK_GETARG(stix, nargs, 0);

	res = stix_bitxorints (stix, rcv, arg);
	if (!res) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */

	STIX_STACK_SETRET (stix, nargs, res);
	return 1;
}

static int pf_integer_bitinv (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, res;

	STIX_ASSERT (nargs == 0);

	rcv = STIX_STACK_GETRCV(stix, nargs);

	res = stix_bitinvint (stix, rcv);
	if (!res) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */

	STIX_STACK_SETRET (stix, nargs, res);
	return 1;
}

static int pf_integer_bitshift (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, res;

	STIX_ASSERT (nargs == 1);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	arg = STIX_STACK_GETARG(stix, nargs, 0);

	res = stix_bitshiftint (stix, rcv, arg);
	if (!res) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */

	STIX_STACK_SETRET (stix, nargs, res);
	return 1;
}

static int pf_integer_eq (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, res;

	STIX_ASSERT (nargs == 1);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	arg = STIX_STACK_GETARG(stix, nargs, 0);

	res = stix_eqints (stix, rcv, arg);
	if (!res) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */

	STIX_STACK_SETRET (stix, nargs, res);
	return 1;
}

static int pf_integer_ne (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, res;

	STIX_ASSERT (nargs == 1);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	arg = STIX_STACK_GETARG(stix, nargs, 0);

	res = stix_neints (stix, rcv, arg);
	if (!res) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */

	STIX_STACK_SETRET (stix, nargs, res);
	return 1;
}

static int pf_integer_lt (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, res;

	STIX_ASSERT (nargs == 1);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	arg = STIX_STACK_GETARG(stix, nargs, 0);

	res = stix_ltints (stix, rcv, arg);
	if (!res) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */

	STIX_STACK_SETRET (stix, nargs, res);
	return 1;
}

static int pf_integer_gt (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, res;

	STIX_ASSERT (nargs == 1);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	arg = STIX_STACK_GETARG(stix, nargs, 0);

	res = stix_gtints (stix, rcv, arg);
	if (!res) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */

	STIX_STACK_SETRET (stix, nargs, res);
	return 1;
}

static int pf_integer_le (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, res;

	STIX_ASSERT (nargs == 1);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	arg = STIX_STACK_GETARG(stix, nargs, 0);

	res = stix_leints (stix, rcv, arg);
	if (!res) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */

	STIX_STACK_SETRET (stix, nargs, res);
	return 1;
}

static int pf_integer_ge (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, res;

	STIX_ASSERT (nargs == 1);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	arg = STIX_STACK_GETARG(stix, nargs, 0);

	res = stix_geints (stix, rcv, arg);
	if (!res) return (stix->errnum == STIX_EINVAL? 0: -1); /* soft or hard failure */

	STIX_STACK_SETRET (stix, nargs, res);
	return 1;
}

static int pf_integer_inttostr (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg, str;
	stix_ooi_t radix;

	STIX_ASSERT (nargs == 1);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	arg = STIX_STACK_GETARG(stix, nargs, 0);

	if (!STIX_OOP_IS_SMOOI(arg)) return 0; /* soft failure */
	radix = STIX_OOP_TO_SMOOI(arg);

	if (radix < 2 || radix > 36) return 0; /* soft failure */
	str = stix_inttostr (stix, rcv, radix);
	if (!str) return (stix->errnum == STIX_EINVAL? 0: -1);

	STIX_STACK_SETRET (stix, nargs, str);
	return 1;
}

static int pf_ffi_open (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg;
	void* handle;

	STIX_ASSERT (nargs == 1);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	arg = STIX_STACK_GETARG(stix, nargs, 0);

	if (!STIX_ISTYPEOF(stix, arg, STIX_OBJ_TYPE_CHAR))
	{
		/* TODO: more info on error */
		return 0;
	}

	if (!stix->vmprim.dl_open)
	{
		/* TODO: more info on error */
		return 0;
	}


/* TODO: check null-termination... */
	handle = stix->vmprim.dl_open (stix, ((stix_oop_char_t)arg)->slot);
	if (!handle)
	{
		/* TODO: more info on error */
		return 0;
	}

	STIX_STACK_POP (stix);
/* TODO: how to hold an address? as an integer????  or a byte array? fix this not to loose accuracy*/
	STIX_STACK_SETTOP (stix, STIX_SMOOI_TO_OOP(handle));

	return 1;
}

static int pf_ffi_close (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, arg;
	void* handle;

	STIX_ASSERT (nargs == 1);

	rcv = STIX_STACK_GETRCV(stix, nargs);
	arg = STIX_STACK_GETARG(stix, nargs, 0);


	if (!STIX_OOP_IS_SMOOI(arg))
	{
		/* TODO: more info on error */
		return 0;
	}

	STIX_STACK_POP (stix);

	handle = (void*)STIX_OOP_TO_SMOOI(arg); /* TODO: how to store void* ???. fix this not to loose accuracy */
	if (stix->vmprim.dl_close) stix->vmprim.dl_close (stix, handle);
	return 1;
}

static int pf_ffi_call (stix_t* stix, stix_ooi_t nargs)
{
#if defined(USE_DYNCALL)
	stix_oop_t rcv, fun, sig, args;

	STIX_ASSERT (nargs == 3);

	rcv = STIX_STACK_GET(stix, stix->sp - 3);
	fun = STIX_STACK_GET(stix, stix->sp - 2);
	sig = STIX_STACK_GET(stix, stix->sp - 1);
	args = STIX_STACK_GET(stix, stix->sp);

	if (!STIX_OOP_IS_SMOOI(fun)) /* TODO: how to store pointer  */
	{
		/* TODO: more info on error */
		return 0;
	}

	if (!STIX_ISTYPEOF(stix, sig, STIX_OBJ_TYPE_CHAR) || STIX_OBJ_GET_SIZE(sig) <= 0)
	{
STIX_DEBUG0 (stix, "FFI: wrong signature...\n");
		return 0;
	}

	if (STIX_CLASSOF(stix,args) != stix->_array) /* TODO: check if arr is a kind of array??? or check if it's indexed */
	{
		/* TODO: more info on error */
		return 0;
	}

	{
		stix_oow_t i;
		DCCallVM* dc;
		void* f;
		stix_oop_oop_t arr;
		int mode_set;

		f = STIX_OOP_TO_SMOOI(fun); /* TODO: decode pointer properly */
		arr = (stix_oop_oop_t)args;

		dc = dcNewCallVM (4096);
		if (!dc) return -1; /* TODO: proper error handling */

STIX_DEBUG1 (stix, "FFI: CALLING............%p\n", f);
		/*dcMode (dc, DC_CALL_C_DEFAULT);
		dcReset (dc);*/

		/*for (i = 2; i < STIX_OBJ_GET_SIZE(sig); i++)
		{
			if (((stix_oop_char_t)sig)->slot[i] == '|') 
			{
				dcMode (dc, DC_CALL_C_ELLIPSIS);
STIX_DEBUG0 (stix, "CALL MODE 111 ERROR %d %d\n", dcGetError (dc), DC_ERROR_UNSUPPORTED_MODE);
				mode_set = 1;
				break;
			}
		}
		if (!mode_set) */ dcMode (dc, DC_CALL_C_DEFAULT);

		for (i = 2; i < STIX_OBJ_GET_SIZE(sig); i++)
		{
STIX_DEBUG1 (stix, "FFI: CALLING ARG %c\n", ((stix_oop_char_t)sig)->slot[i]);
			switch (((stix_oop_char_t)sig)->slot[i])
			{
			/* TODO: support more types... */
				/*
				case '|':
					dcMode (dc, DC_CALL_C_ELLIPSIS_VARARGS);
STIX_DEBUG2 (stix, "CALL MODE 222 ERROR %d %d\n", dcGetError (dc), DC_ERROR_UNSUPPORTED_MODE);
					break;
				*/

				case 'c':
					/* TODO: sanity check on the argument type */
					dcArgChar (dc, STIX_OOP_TO_CHAR(arr->slot[i - 2]));
					break;

				case 'i':
					dcArgInt (dc, STIX_OOP_TO_SMOOI(arr->slot[i - 2]));
					break;

				case 'l':
					dcArgLong (dc, STIX_OOP_TO_SMOOI(arr->slot[i - 2]));
					break;

				case 'L':
					dcArgLongLong (dc, STIX_OOP_TO_SMOOI(arr->slot[i - 2]));
					break;

				case 's':
				{
					stix_oow_t bcslen, ucslen;
					stix_bch_t bcs[1024];

					ucslen = STIX_OBJ_GET_SIZE(arr->slot[i - 2]);
					stix_oocstobcs (stix, ((stix_oop_char_t)arr->slot[i - 2])->slot, &ucslen, bcs, &bcslen); /* proper string conversion */

					bcs[bcslen] = '\0';
					dcArgPointer (dc, bcs);
					break;
				}

				default:
					/* TODO: ERROR HANDLING */
					break;
			}

		}

		STIX_STACK_POPS (stix, nargs);

		switch (((stix_oop_char_t)sig)->slot[0])
		{
/* TODO: support more types... */
/* TODO: proper return value conversion */
			case 'c':
			{
				char r = dcCallChar (dc, f);
				STIX_STACK_SETTOP (stix, STIX_CHAR_TO_OOP(r));
				break;
			}

			case 'i':
			{
				int r = dcCallInt (dc, f);
STIX_DEBUG1 (stix, "CALLED... %d\n", r);
STIX_DEBUG2 (stix, "CALL ERROR %d %d\n", dcGetError (dc), DC_ERROR_UNSUPPORTED_MODE);
				STIX_STACK_SETTOP (stix, STIX_SMOOI_TO_OOP(r));
				break;
			}

			case 'l':
			{
				long r = dcCallLong (dc, f);
				STIX_STACK_SETTOP (stix, STIX_SMOOI_TO_OOP(r));
				break;
			}

			case 'L':
			{
				long long r = dcCallLongLong (dc, f);
				STIX_STACK_SETTOP (stix, STIX_SMOOI_TO_OOP(r));
				break;
			}

			case 's':
			{
				stix_oow_t bcslen, ucslen;
				stix_ooch_t ucs[1024];
				stix_oop_t s;
				char* r = dcCallPointer (dc, f);

				bcslen = strlen(r); 
				stix_bcstooocs (stix, r, &bcslen, ucs, &ucslen); /* proper string conversion */

				s = stix_makestring(stix, ucs, ucslen)
				if (!s) 
				{
					dcFree (dc);
					return -1; /* TODO: proper error h andling */
				}

				STIX_STACK_SETTOP (stix, s); 
				break;
			}

			default:
				/* TOOD: ERROR HANDLING */
				break;
		}

		dcFree (dc);
	}

	return 1;
#else
	return 0;
#endif
}

static int pf_ffi_getsym (stix_t* stix, stix_ooi_t nargs)
{
	stix_oop_t rcv, hnd, fun;
	void* sym;

	STIX_ASSERT (nargs == 2);

	rcv = STIX_STACK_GET(stix, stix->sp - 2);
	fun = STIX_STACK_GET(stix, stix->sp - 1);
	hnd = STIX_STACK_GET(stix, stix->sp);

	if (!STIX_OOP_IS_SMOOI(hnd)) /* TODO: how to store pointer  */
	{
		/* TODO: more info on error */
		return 0;
	}

	if (!STIX_ISTYPEOF(stix,fun,STIX_OBJ_TYPE_CHAR))
	{
STIX_DEBUG0 (stix, "wrong function name...\n");
		return 0;
	}

	if (!stix->vmprim.dl_getsym)
	{
		return 0;
	}

	sym = stix->vmprim.dl_getsym (stix, (void*)STIX_OOP_TO_SMOOI(hnd), ((stix_oop_char_t)fun)->slot);
	if (!sym)
	{
		return 0;
	}

/* TODO: how to hold an address? as an integer????  or a byte array? */
	STIX_STACK_SETRET (stix, nargs, STIX_SMOOI_TO_OOP(sym));

	return 1;
}

#define MAX_NARGS STIX_TYPE_MAX(stix_ooi_t)
struct pf_t
{
	stix_ooi_t       min_nargs;   /* expected number of arguments */
	stix_ooi_t       max_nargs;   /* expected number of arguments */
	stix_pfimpl_t    handler;
	const char*      name;    /* the name is supposed to be 7-bit ascii only */
};
typedef struct pf_t pf_t;

static pf_t pftab[] =
{
	{   0, MAX_NARGS,  pf_dump,                      "_dump"                },
	{   2, MAX_NARGS,  pf_log,                       "_log"                 },

	{   1,  1,  pf_identical,                        "_identical"           },
	{   1,  1,  pf_not_identical,                    "_not_identical"       },
	{   0,  0,  pf_class,                            "_class"               },

	{   0,  0,  pf_basic_new,                        "_basic_new"           },
	{   1,  1,  pf_basic_new_with_size,              "_basic_new_with_size" },
	{   0,  0,  pf_ngc_new,                          "_ngc_new"             },
	{   1,  1,  pf_ngc_new_with_size,                "_ngc_new_with_size"   },
	{   0,  0,  pf_ngc_dispose,                      "_ngc_dispose"         },
	{   0,  0,  pf_shallow_copy,                     "_shallow_copy"        },

	{   0,  0,  pf_basic_size,                       "_basic_size"          },
	{   1,  1,  pf_basic_at,                         "_basic_at"            },
	{   2,  2,  pf_basic_at_put,                     "_basic_at_put"        },


	{   1,  1,  pf_context_goto,                     "_context_goto"        },
	{   0, MAX_NARGS,  pf_block_value,               "_block_value"         },
	{   0, MAX_NARGS,  pf_block_new_process,         "_block_new_process"   },

	{   0,  0,  pf_process_resume,                   "_process_resume"      },
	{   0,  0,  pf_process_terminate,                "_process_terminate"   },
	{   0,  0,  pf_process_yield,                    "_process_yield"       },
	{   0,  0,  pf_process_suspend,                  "_process_suspend"     },
	{   0,  0,  pf_semaphore_signal,                 "_semaphore_signal"    },
	{   0,  0,  pf_semaphore_wait,                   "_semaphore_wait"      },

	{   1,  1,  pf_processor_schedule,               "_processor_schedule"            },
	{   2,  3,  pf_processor_add_timed_semaphore,    "_processor_add_timed_semaphore" },
	{   1,  1,  pf_processor_remove_semaphore,       "_processor_remove_semaphore" },
	{   2,  2,  pf_processor_return_to,              "_processor_return_to" },

	{   1,  1,  pf_integer_add,                      "_integer_add"         },
	{   1,  1,  pf_integer_sub,                      "_integer_sub"         },
	{   1,  1,  pf_integer_mul,                      "_integer_mul"         },
	{   1,  1,  pf_integer_quo,                      "_integer_quo"         },
	{   1,  1,  pf_integer_rem,                      "_integer_rem"         },
	{   1,  1,  pf_integer_quo2,                     "_integer_quo2"        },
	{   1,  1,  pf_integer_rem2,                     "_integer_rem2"        },
	{   0,  0,  pf_integer_negated,                  "_integer_negated"     },
	{   1,  1,  pf_integer_bitat,                    "_integer_bitat"       },
	{   1,  1,  pf_integer_bitand,                   "_integer_bitand"      },
	{   1,  1,  pf_integer_bitor,                    "_integer_bitor"       },
	{   1,  1,  pf_integer_bitxor,                   "_integer_bitxor"      },
	{   0,  0,  pf_integer_bitinv,                   "_integer_bitinv"      },
	{   1,  1,  pf_integer_bitshift,                 "_integer_bitshift"    },
	{   1,  1,  pf_integer_eq,                       "_integer_eq"          },
	{   1,  1,  pf_integer_ne,                       "_integer_ne"          },
	{   1,  1,  pf_integer_lt,                       "_integer_lt"          },
	{   1,  1,  pf_integer_gt,                       "_integer_gt"          },
	{   1,  1,  pf_integer_le,                       "_integer_le"          },
	{   1,  1,  pf_integer_ge,                       "_integer_ge"          },
	{   1,  1,  pf_integer_inttostr,                 "_integer_inttostr"    },

	{   1,  1,  pf_ffi_open,                         "_ffi_open"            },
	{   1,  1,  pf_ffi_close,                        "_ffi_close"           },
	{   2,  2,  pf_ffi_getsym,                       "_ffi_getsym"          },
	{   3,  3,  pf_ffi_call,                         "_ffi_call"            }
	
};

int stix_getpfnum (stix_t* stix, const stix_ooch_t* ptr, stix_oow_t len)
{
	int i;

	for (i = 0; i < STIX_COUNTOF(pftab); i++)
	{
		if (stix_compucharsbcstr(ptr, len, pftab[i].name) == 0)
		{
			return i;
		}
	}

	stix->errnum = STIX_ENOENT;
	return -1;
}

/* ------------------------------------------------------------------------- */
static int start_method (stix_t* stix, stix_oop_method_t method, stix_oow_t nargs)
{
	stix_ooi_t preamble, preamble_code;

#if defined(STIX_DEBUG_VM_EXEC)
	stix_ooi_t fetched_instruction_pointer = 0; /* set it to a fake value */
#endif

	preamble = STIX_OOP_TO_SMOOI(method->preamble);

	/*STIX_ASSERT (STIX_OOP_TO_SMOOI(method->tmpr_nargs) == nargs);*/
	if (nargs != STIX_OOP_TO_SMOOI(method->tmpr_nargs))
	{

		stix_ooi_t preamble_flags;

		preamble_flags = STIX_METHOD_GET_PREAMBLE_FLAGS(preamble);
		if (!(preamble_flags & STIX_METHOD_PREAMBLE_FLAG_VARIADIC))
		{
/* TODO: better to throw a stix exception so that the caller can catch it??? */
			STIX_LOG3 (stix, STIX_LOG_IC | STIX_LOG_FATAL, 
				"Fatal error - Argument count mismatch for a non-variadic method [%O] - %zd expected, %zu given\n",
				method->name, STIX_OOP_TO_SMOOI(method->tmpr_nargs), nargs);
			stix->errnum = STIX_EINVAL;
			return -1;
		}
	}

	preamble_code = STIX_METHOD_GET_PREAMBLE_CODE(preamble);
	switch (preamble_code)
	{
		case STIX_METHOD_PREAMBLE_RETURN_RECEIVER:
			LOG_INST_0 (stix, "preamble_return_receiver");
			STIX_STACK_POPS (stix, nargs); /* pop arguments only*/
			break;

		case STIX_METHOD_PREAMBLE_RETURN_NIL:
			LOG_INST_0 (stix, "preamble_return_nil");
			STIX_STACK_POPS (stix, nargs);
			STIX_STACK_SETTOP (stix, stix->_nil);
			break;

		case STIX_METHOD_PREAMBLE_RETURN_TRUE:
			LOG_INST_0 (stix, "preamble_return_true");
			STIX_STACK_POPS (stix, nargs);
			STIX_STACK_SETTOP (stix, stix->_true);
			break;

		case STIX_METHOD_PREAMBLE_RETURN_FALSE:
			LOG_INST_0 (stix, "preamble_return_false");
			STIX_STACK_POPS (stix, nargs);
			STIX_STACK_SETTOP (stix, stix->_false);
			break;

		case STIX_METHOD_PREAMBLE_RETURN_INDEX: 
			/* preamble_index field is used to store a positive integer */
			LOG_INST_1 (stix, "preamble_return_index %zd", STIX_METHOD_GET_PREAMBLE_INDEX(preamble));
			STIX_STACK_POPS (stix, nargs);
			STIX_STACK_SETTOP (stix, STIX_SMOOI_TO_OOP(STIX_METHOD_GET_PREAMBLE_INDEX(preamble)));
			break;

		case STIX_METHOD_PREAMBLE_RETURN_NEGINDEX:
			/* preamble_index field is used to store a negative integer */
			LOG_INST_1 (stix, "preamble_return_negindex %zd", STIX_METHOD_GET_PREAMBLE_INDEX(preamble));
			STIX_STACK_POPS (stix, nargs);
			STIX_STACK_SETTOP (stix, STIX_SMOOI_TO_OOP(-STIX_METHOD_GET_PREAMBLE_INDEX(preamble)));
			break;

		case STIX_METHOD_PREAMBLE_RETURN_INSTVAR:
		{
			stix_oop_oop_t rcv;

			STIX_STACK_POPS (stix, nargs); /* pop arguments only */

			LOG_INST_1 (stix, "preamble_return_instvar %zd", STIX_METHOD_GET_PREAMBLE_INDEX(preamble));

			/* replace the receiver by an instance variable of the receiver */
			rcv = (stix_oop_oop_t)STIX_STACK_GETTOP(stix);
			STIX_ASSERT (STIX_OBJ_GET_FLAGS_TYPE(rcv) == STIX_OBJ_TYPE_OOP);
			STIX_ASSERT (STIX_OBJ_GET_SIZE(rcv) > STIX_METHOD_GET_PREAMBLE_INDEX(preamble));

			if (rcv == (stix_oop_oop_t)stix->active_context)
			{
				/* the active context object doesn't keep
				 * the most up-to-date information in the 
				 * 'ip' and 'sp' field. commit these fields
				 * when the object to be accessed is 
				 * the active context. this manual commit
				 * is required because this premable handling
				 * skips activation of a new method context
				 * that would commit these fields. 
				 */
				STORE_ACTIVE_IP (stix);
				STORE_ACTIVE_SP (stix);
			}

			/* this accesses the instance variable of the receiver */
			STIX_STACK_SET (stix, stix->sp, rcv->slot[STIX_METHOD_GET_PREAMBLE_INDEX(preamble)]);
			break;
		}

		case STIX_METHOD_PREAMBLE_PRIMITIVE:
		{
			stix_ooi_t pfnum;

			pfnum = STIX_METHOD_GET_PREAMBLE_INDEX(preamble);
			LOG_INST_1 (stix, "preamble_primitive %zd", pf_no);

			if (pfnum >= 0 && pfnum < STIX_COUNTOF(pftab) && 
			    (nargs >= pftab[pfnum].min_nargs && nargs <= pftab[pfnum].max_nargs))
			{
				int n;

				stix_pushtmp (stix, (stix_oop_t*)&method);
				n = pftab[pfnum].handler (stix, nargs);
				stix_poptmp (stix);
				if (n <= -1) return -1; /* hard primitive failure */
				if (n >= 1) break; /* primitive ok */
			}

			/* soft primitive failure */
			if (activate_new_method (stix, method, nargs) <= -1) return -1;
			break;
		}

		case STIX_METHOD_PREAMBLE_NAMED_PRIMITIVE:
		{
			stix_ooi_t pf_name_index;
			stix_oop_t name;
			stix_pfimpl_t handler;
			stix_oow_t w;
			stix_ooi_t /*sp,*/ sb;

			/*sp = stix->sp;*/
			sb = stix->sp - nargs - 1; /* stack base before receiver and arguments */

			pf_name_index = STIX_METHOD_GET_PREAMBLE_INDEX(preamble);
			LOG_INST_1 (stix, "preamble_named_primitive %zd", pf_name_index);

			/* merge two SmallIntegers to get a full pointer */
			w = (stix_oow_t)STIX_OOP_TO_SMOOI(method->preamble_data[0]) << (STIX_OOW_BITS / 2) | 
			    (stix_oow_t)STIX_OOP_TO_SMOOI(method->preamble_data[1]);
			handler = (stix_pfimpl_t)w;
			if (handler) goto exec_handler;
			else
			{
				STIX_ASSERT (pf_name_index >= 0);
				name = method->slot[pf_name_index];

				STIX_ASSERT (STIX_ISTYPEOF(stix,name,STIX_OBJ_TYPE_CHAR));
				STIX_ASSERT (STIX_OBJ_GET_FLAGS_EXTRA(name));
				STIX_ASSERT (STIX_CLASSOF(stix,name) == stix->_symbol);

				handler = stix_querymod (stix, ((stix_oop_char_t)name)->slot, STIX_OBJ_GET_SIZE(name));
			}

			if (handler)
			{
				int n;

				/* split a pointer to two OOP fields as SmallIntegers for storing. */
				method->preamble_data[0] = STIX_SMOOI_TO_OOP((stix_oow_t)handler >> (STIX_OOW_BITS / 2));
				method->preamble_data[1] = STIX_SMOOI_TO_OOP((stix_oow_t)handler & STIX_LBMASK(stix_oow_t, STIX_OOW_BITS / 2));

			exec_handler:
				stix_pushtmp (stix, (stix_oop_t*)&method);

				/* the primitive handler is executed without activating the method itself.
				 * one major difference between the primitive function and the normal method
				 * invocation is that the primitive function handler should access arguments
				 * directly in the stack unlik a normal activated method context where the
				 * arguments are copied to the back. */

				n = handler (stix, nargs);

				stix_poptmp (stix);
				if (n <= -1) 
				{
					STIX_DEBUG2 (stix, "Hard failure indicated by primitive function %p - return code %d\n", handler, n);
					return -1; /* hard primitive failure */
				}
				if (n >= 1) break; /* primitive ok*/

				/* soft primitive failure */
				STIX_DEBUG1 (stix, "Soft failure indicated by primitive function %p\n", handler);
			}
			else
			{
				/* no handler found */
				STIX_DEBUG0 (stix, "Soft failure for non-existent primitive function\n");
			}

		#if defined(STIX_USE_OBJECT_TRAILER)
			STIX_ASSERT (STIX_OBJ_GET_FLAGS_TRAILER(method));
			if (STIX_METHOD_GET_CODE_SIZE(method) == 0) /* this trailer size field not a small integer */
		#else
			if (method->code == stix->_nil)
		#endif
			{
				/* no byte code to execute */
/* TODO: what is the best tactics? emulate "self primitiveFailed"? */

				/* force restore stack pointers */
				stix->sp = sb;
				STIX_STACK_PUSH (stix, stix->_nil);
				return -1;
			}
			else
			{
				if (activate_new_method (stix, method, nargs) <= -1) return -1;
			}
			break;
		}

		default:
			STIX_ASSERT (preamble_code == STIX_METHOD_PREAMBLE_NONE ||
			             preamble_code == STIX_METHOD_PREAMBLE_EXCEPTION ||
			             preamble_code == STIX_METHOD_PREAMBLE_ENSURE);
			if (activate_new_method (stix, method, nargs) <= -1) return -1;
			break;
	}

	return 0;
}

static int send_message (stix_t* stix, stix_oop_char_t selector, int to_super, stix_ooi_t nargs)
{
	stix_oocs_t mthname;
	stix_oop_t receiver;
	stix_oop_method_t method;

	STIX_ASSERT (STIX_OOP_IS_POINTER(selector));
	STIX_ASSERT (STIX_OBJ_GET_FLAGS_TYPE(selector) == STIX_OBJ_TYPE_CHAR);
	STIX_ASSERT (STIX_CLASSOF(stix, selector) == stix->_symbol);

	receiver = STIX_STACK_GET(stix, stix->sp - nargs);

	mthname.ptr = selector->slot;
	mthname.len = STIX_OBJ_GET_SIZE(selector);
	method = find_method (stix, receiver, &mthname, to_super);
	if (!method) 
	{
		static stix_ooch_t fbm[] = { 
			'd', 'o', 'e', 's', 
			'N', 'o', 't',
			'U', 'n', 'd', 'e', 'r', 's', 't', 'a', 'n', 'd', ':'
		};
		mthname.ptr = fbm;
		mthname.len = 18;

		method = find_method (stix, receiver, &mthname, 0);
		if (!method)
		{
			/* this must not happen as long as doesNotUnderstand: is implemented under Apex.
			 * this check should indicate a very serious internal problem */
			STIX_LOG4 (stix, STIX_LOG_IC | STIX_LOG_FATAL, 
				"Fatal error - receiver [%O] of class [%O] does not understand a message [%.*S]\n", 
				receiver, STIX_CLASSOF(stix, receiver), mthname.len, mthname.ptr);

			stix->errnum = STIX_EMSGSND;
			return -1;
		}
		else
		{
			/* manipulate the stack as if 'receier doesNotUnderstand: selector' 
			 * has been called. */
/* TODO: if i manipulate the stack this way here, the stack trace for the last call is kind of lost.
 *       how can i preserve it gracefully? */
			STIX_STACK_POPS (stix, nargs);
			nargs = 1;
			STIX_STACK_PUSH (stix, (stix_oop_t)selector);
		}
	}

	return start_method (stix, method, nargs);
}

static int send_private_message (stix_t* stix, const stix_ooch_t* nameptr, stix_oow_t namelen, int to_super, stix_ooi_t nargs)
{
	stix_oocs_t mthname;
	stix_oop_t receiver;
	stix_oop_method_t method;

	receiver = STIX_STACK_GET(stix, stix->sp - nargs);

	mthname.ptr = (stix_ooch_t*)nameptr;
	mthname.len = namelen;
	method = find_method (stix, receiver, &mthname, to_super);
	if (!method)
	{
		STIX_LOG4 (stix, STIX_LOG_IC | STIX_LOG_FATAL, 
			"Fatal error - receiver [%O] of class [%O] does not understand a private message [%.*S]\n", 
			receiver, STIX_CLASSOF(stix, receiver), mthname.len, mthname.ptr);
		stix->errnum = STIX_EMSGSND;
		return -1;
	}

	return start_method (stix, method, nargs);
}
/* ------------------------------------------------------------------------- */

int stix_execute (stix_t* stix)
{
	stix_oob_t bcode;
	stix_oow_t b1, b2;
	stix_oop_t return_value;
	int unwind_protect;
	stix_oop_context_t unwind_start;
	stix_oop_context_t unwind_stop;

#if defined(STIX_PROFILE_VM)
	stix_uintmax_t inst_counter = 0;
#endif

#if defined(STIX_DEBUG_VM_EXEC)
	stix_ooi_t fetched_instruction_pointer;
#endif

	STIX_ASSERT (stix->active_context != STIX_NULL);

	vm_startup (stix);
	stix->proc_switched = 0;

	while (1)
	{
		if (stix->sem_heap_count > 0)
		{
			stix_ntime_t ft, now;
			vm_gettime (stix, &now);

			do
			{
				STIX_ASSERT (STIX_OOP_IS_SMOOI(stix->sem_heap[0]->heap_ftime_sec));
				STIX_ASSERT (STIX_OOP_IS_SMOOI(stix->sem_heap[0]->heap_ftime_nsec));

				STIX_INITNTIME (&ft,
					STIX_OOP_TO_SMOOI(stix->sem_heap[0]->heap_ftime_sec),
					STIX_OOP_TO_SMOOI(stix->sem_heap[0]->heap_ftime_nsec)
				);

				if (STIX_CMPNTIME(&ft, (stix_ntime_t*)&now) <= 0)
				{
					stix_oop_process_t proc;

					/* waited long enough. signal the semaphore */

					proc = signal_semaphore (stix, stix->sem_heap[0]);
					/* [NOTE] no stix_pushtmp() on proc. no GC must occur
					 *        in the following line until it's used for
					 *        wake_new_process() below. */
					delete_from_sem_heap (stix, 0);

					/* if no process is waiting on the semaphore, 
					 * signal_semaphore() returns stix->_nil. */

					if (stix->processor->active == stix->nil_process && (stix_oop_t)proc != stix->_nil)
					{
						/* this is the only runnable process. 
						 * switch the process to the running state.
						 * it uses wake_new_process() instead of
						 * switch_to_process() as there is no running 
						 * process at this moment */
						STIX_ASSERT (proc->state == STIX_SMOOI_TO_OOP(PROC_STATE_RUNNABLE));
						STIX_ASSERT (proc == stix->processor->runnable_head);

						wake_new_process (stix, proc);
						stix->proc_switched = 1;
					}
				}
				else if (stix->processor->active == stix->nil_process)
				{
					STIX_SUBNTIME (&ft, &ft, (stix_ntime_t*)&now);
					vm_sleep (stix, &ft); /* TODO: change this to i/o multiplexer??? */
					vm_gettime (stix, &now);
				}
				else 
				{
					break;
				}
			} 
			while (stix->sem_heap_count > 0);
		}

		if (stix->processor->active == stix->nil_process) 
		{
			/* no more waiting semaphore and no more process */
			STIX_ASSERT (stix->processor->tally = STIX_SMOOI_TO_OOP(0));
			STIX_LOG0 (stix, STIX_LOG_IC | STIX_LOG_DEBUG, "No more runnable process\n");

			#if 0
			if (there is semaphore awaited.... )
			{
			/* DO SOMETHING */
			}
			#endif

			break;
		}

		while (stix->sem_list_count > 0)
		{
			/* handle async signals */
			--stix->sem_list_count;
			signal_semaphore (stix, stix->sem_list[stix->sem_list_count]);
		}
		/*
		if (semaphore heap has pending request)
		{
			signal them...
		}*/

		/* TODO: implement different process switching scheme - time-slice or clock based??? */
#if defined(STIX_EXTERNAL_PROCESS_SWITCH)
		if (!stix->proc_switched && stix->switch_proc) { switch_to_next_runnable_process (stix); }
		stix->switch_proc = 0;
#else
		if (!stix->proc_switched) { switch_to_next_runnable_process (stix); }
#endif

		stix->proc_switched = 0;

#if defined(STIX_DEBUG_VM_EXEC)
		fetched_instruction_pointer = stix->ip;
#endif
		FETCH_BYTE_CODE_TO (stix, bcode);
		/*while (bcode == BCODE_NOOP) FETCH_BYTE_CODE_TO (stix, bcode);*/

#if defined(STIX_PROFILE_VM)
		inst_counter++;
#endif

		switch (bcode)
		{
			/* ------------------------------------------------- */

			case BCODE_PUSH_INSTVAR_X:
				FETCH_PARAM_CODE_TO (stix, b1);
				goto push_instvar;
			case BCODE_PUSH_INSTVAR_0:
			case BCODE_PUSH_INSTVAR_1:
			case BCODE_PUSH_INSTVAR_2:
			case BCODE_PUSH_INSTVAR_3:
			case BCODE_PUSH_INSTVAR_4:
			case BCODE_PUSH_INSTVAR_5:
			case BCODE_PUSH_INSTVAR_6:
			case BCODE_PUSH_INSTVAR_7:
				b1 = bcode & 0x7; /* low 3 bits */
			push_instvar:
				LOG_INST_1 (stix, "push_instvar %zu", b1);
				STIX_ASSERT (STIX_OBJ_GET_FLAGS_TYPE(stix->active_context->origin->receiver_or_source) == STIX_OBJ_TYPE_OOP);
				STIX_STACK_PUSH (stix, ((stix_oop_oop_t)stix->active_context->origin->receiver_or_source)->slot[b1]);
				break;

			/* ------------------------------------------------- */

			case BCODE_STORE_INTO_INSTVAR_X:
				FETCH_PARAM_CODE_TO (stix, b1);
				goto store_instvar;
			case BCODE_STORE_INTO_INSTVAR_0:
			case BCODE_STORE_INTO_INSTVAR_1:
			case BCODE_STORE_INTO_INSTVAR_2:
			case BCODE_STORE_INTO_INSTVAR_3:
			case BCODE_STORE_INTO_INSTVAR_4:
			case BCODE_STORE_INTO_INSTVAR_5:
			case BCODE_STORE_INTO_INSTVAR_6:
			case BCODE_STORE_INTO_INSTVAR_7:
				b1 = bcode & 0x7; /* low 3 bits */
			store_instvar:
				LOG_INST_1 (stix, "store_into_instvar %zu", b1);
				STIX_ASSERT (STIX_OBJ_GET_FLAGS_TYPE(stix->active_context->receiver_or_source) == STIX_OBJ_TYPE_OOP);
				((stix_oop_oop_t)stix->active_context->origin->receiver_or_source)->slot[b1] = STIX_STACK_GETTOP(stix);
				break;

			/* ------------------------------------------------- */
			case BCODE_POP_INTO_INSTVAR_X:
				FETCH_PARAM_CODE_TO (stix, b1);
				goto pop_into_instvar;
			case BCODE_POP_INTO_INSTVAR_0:
			case BCODE_POP_INTO_INSTVAR_1:
			case BCODE_POP_INTO_INSTVAR_2:
			case BCODE_POP_INTO_INSTVAR_3:
			case BCODE_POP_INTO_INSTVAR_4:
			case BCODE_POP_INTO_INSTVAR_5:
			case BCODE_POP_INTO_INSTVAR_6:
			case BCODE_POP_INTO_INSTVAR_7:
				b1 = bcode & 0x7; /* low 3 bits */
			pop_into_instvar:
				LOG_INST_1 (stix, "pop_into_instvar %zu", b1);
				STIX_ASSERT (STIX_OBJ_GET_FLAGS_TYPE(stix->active_context->receiver_or_source) == STIX_OBJ_TYPE_OOP);
				((stix_oop_oop_t)stix->active_context->origin->receiver_or_source)->slot[b1] = STIX_STACK_GETTOP(stix);
				STIX_STACK_POP (stix);
				break;

			/* ------------------------------------------------- */
			case BCODE_PUSH_TEMPVAR_X:
			case BCODE_STORE_INTO_TEMPVAR_X:
			case BCODE_POP_INTO_TEMPVAR_X:
				FETCH_PARAM_CODE_TO (stix, b1);
				goto handle_tempvar;

			case BCODE_PUSH_TEMPVAR_0:
			case BCODE_PUSH_TEMPVAR_1:
			case BCODE_PUSH_TEMPVAR_2:
			case BCODE_PUSH_TEMPVAR_3:
			case BCODE_PUSH_TEMPVAR_4:
			case BCODE_PUSH_TEMPVAR_5:
			case BCODE_PUSH_TEMPVAR_6:
			case BCODE_PUSH_TEMPVAR_7:
			case BCODE_STORE_INTO_TEMPVAR_0:
			case BCODE_STORE_INTO_TEMPVAR_1:
			case BCODE_STORE_INTO_TEMPVAR_2:
			case BCODE_STORE_INTO_TEMPVAR_3:
			case BCODE_STORE_INTO_TEMPVAR_4:
			case BCODE_STORE_INTO_TEMPVAR_5:
			case BCODE_STORE_INTO_TEMPVAR_6:
			case BCODE_STORE_INTO_TEMPVAR_7:
			case BCODE_POP_INTO_TEMPVAR_0:
			case BCODE_POP_INTO_TEMPVAR_1:
			case BCODE_POP_INTO_TEMPVAR_2:
			case BCODE_POP_INTO_TEMPVAR_3:
			case BCODE_POP_INTO_TEMPVAR_4:
			case BCODE_POP_INTO_TEMPVAR_5:
			case BCODE_POP_INTO_TEMPVAR_6:
			case BCODE_POP_INTO_TEMPVAR_7:
			{
				stix_oop_context_t ctx;
				stix_ooi_t bx;

				b1 = bcode & 0x7; /* low 3 bits */
			handle_tempvar:

			#if defined(STIX_USE_CTXTEMPVAR)
				/* when CTXTEMPVAR inststructions are used, the above 
				 * instructions are used only for temporary access 
				 * outside a block. i can assume that the temporary
				 * variable index is pointing to one of temporaries
				 * in the relevant method context */
				ctx = stix->active_context->origin;
				bx = b1;
				STIX_ASSERT (STIX_CLASSOF(stix, ctx) == stix->_method_context);
			#else
				/* otherwise, the index may point to a temporaries
				 * declared inside a block */

				if (stix->active_context->home != stix->_nil)
				{
					/* this code assumes that the method context and
					 * the block context place some key fields in the
					 * same offset. such fields include 'home', 'ntmprs' */
					stix_oop_t home;
					stix_ooi_t home_ntmprs;

					ctx = stix->active_context;
					home = ctx->home;

					do
					{
						/* ntmprs contains the number of defined temporaries 
						 * including those defined in the home context */
						home_ntmprs = STIX_OOP_TO_SMOOI(((stix_oop_context_t)home)->ntmprs);
						if (b1 >= home_ntmprs) break;

						ctx = (stix_oop_context_t)home;
						home = ((stix_oop_context_t)home)->home;
						if (home == stix->_nil)
						{
							home_ntmprs = 0;
							break;
						}
					}
					while (1);

					/* bx is the actual index within the actual context 
					 * containing the temporary */
					bx = b1 - home_ntmprs;
				}
				else
				{
					ctx = stix->active_context;
					bx = b1;
				}
			#endif

				if ((bcode >> 4) & 1)
				{
					/* push - bit 4 on */
					LOG_INST_1 (stix, "push_tempvar %zu", b1);
					STIX_STACK_PUSH (stix, ctx->slot[bx]);
				}
				else
				{
					/* store or pop - bit 5 off */
					ctx->slot[bx] = STIX_STACK_GETTOP(stix);

					if ((bcode >> 3) & 1)
					{
						/* pop - bit 3 on */
						LOG_INST_1 (stix, "pop_into_tempvar %zu", b1);
						STIX_STACK_POP (stix);
					}
					else
					{
						LOG_INST_1 (stix, "store_into_tempvar %zu", b1);
					}
				}

				break;
			}

			/* ------------------------------------------------- */
			case BCODE_PUSH_LITERAL_X:
				FETCH_PARAM_CODE_TO (stix, b1);
				goto push_literal;

			case BCODE_PUSH_LITERAL_0:
			case BCODE_PUSH_LITERAL_1:
			case BCODE_PUSH_LITERAL_2:
			case BCODE_PUSH_LITERAL_3:
			case BCODE_PUSH_LITERAL_4:
			case BCODE_PUSH_LITERAL_5:
			case BCODE_PUSH_LITERAL_6:
			case BCODE_PUSH_LITERAL_7:
				b1 = bcode & 0x7; /* low 3 bits */
			push_literal:
				LOG_INST_1 (stix, "push_literal @%zu", b1);
				STIX_STACK_PUSH (stix, stix->active_method->slot[b1]);
				break;

			/* ------------------------------------------------- */
			case BCODE_PUSH_OBJECT_X:
			case BCODE_STORE_INTO_OBJECT_X:
			case BCODE_POP_INTO_OBJECT_X:
				FETCH_PARAM_CODE_TO (stix, b1);
				goto handle_object;

			case BCODE_PUSH_OBJECT_0:
			case BCODE_PUSH_OBJECT_1:
			case BCODE_PUSH_OBJECT_2:
			case BCODE_PUSH_OBJECT_3:
			case BCODE_STORE_INTO_OBJECT_0:
			case BCODE_STORE_INTO_OBJECT_1:
			case BCODE_STORE_INTO_OBJECT_2:
			case BCODE_STORE_INTO_OBJECT_3:
			case BCODE_POP_INTO_OBJECT_0:
			case BCODE_POP_INTO_OBJECT_1:
			case BCODE_POP_INTO_OBJECT_2:
			case BCODE_POP_INTO_OBJECT_3:
			{
				stix_oop_association_t ass;

				b1 = bcode & 0x3; /* low 2 bits */
			handle_object:
				ass = (stix_oop_association_t)stix->active_method->slot[b1];
				STIX_ASSERT (STIX_CLASSOF(stix, ass) == stix->_association);

				if ((bcode >> 3) & 1)
				{
					/* store or pop */
					ass->value = STIX_STACK_GETTOP(stix);

					if ((bcode >> 2) & 1)
					{
						/* pop */
						LOG_INST_1 (stix, "pop_into_object @%zu", b1);
						STIX_STACK_POP (stix);
					}
					else
					{
						LOG_INST_1 (stix, "store_into_object @%zu", b1);
					}
				}
				else
				{
					/* push */
					LOG_INST_1 (stix, "push_object @%zu", b1);
					STIX_STACK_PUSH (stix, ass->value);
				}
				break;
			}

			/* -------------------------------------------------------- */

			case BCODE_JUMP_FORWARD_X:
				FETCH_PARAM_CODE_TO (stix, b1);
				LOG_INST_1 (stix, "jump_forward %zu", b1);
				stix->ip += b1;
				break;

			case BCODE_JUMP_FORWARD_0:
			case BCODE_JUMP_FORWARD_1:
			case BCODE_JUMP_FORWARD_2:
			case BCODE_JUMP_FORWARD_3:
				LOG_INST_1 (stix, "jump_forward %zu", (stix_oow_t)(bcode & 0x3));
				stix->ip += (bcode & 0x3); /* low 2 bits */
				break;

			case BCODE_JUMP_BACKWARD_X:
				FETCH_PARAM_CODE_TO (stix, b1);
				LOG_INST_1 (stix, "jump_backward %zu", b1);
				stix->ip += b1;
				break;

			case BCODE_JUMP_BACKWARD_0:
			case BCODE_JUMP_BACKWARD_1:
			case BCODE_JUMP_BACKWARD_2:
			case BCODE_JUMP_BACKWARD_3:
				LOG_INST_1 (stix, "jump_backward %zu", (stix_oow_t)(bcode & 0x3));
				stix->ip -= (bcode & 0x3); /* low 2 bits */
				break;

			case BCODE_JUMP_IF_TRUE_X:
			case BCODE_JUMP_IF_FALSE_X:
			case BCODE_JUMP_IF_TRUE_0:
			case BCODE_JUMP_IF_TRUE_1:
			case BCODE_JUMP_IF_TRUE_2:
			case BCODE_JUMP_IF_TRUE_3:
			case BCODE_JUMP_IF_FALSE_0:
			case BCODE_JUMP_IF_FALSE_1:
			case BCODE_JUMP_IF_FALSE_2:
			case BCODE_JUMP_IF_FALSE_3:
STIX_LOG0 (stix, STIX_LOG_IC | STIX_LOG_FATAL, "<<<<<<<<<<<<<< JUMP NOT IMPLEMENTED YET >>>>>>>>>>>>\n");
stix->errnum = STIX_ENOIMPL;
return -1;

			case BCODE_JUMP2_FORWARD:
				FETCH_PARAM_CODE_TO (stix, b1);
				LOG_INST_1 (stix, "jump2_forward %zu", b1);
				stix->ip += MAX_CODE_JUMP + b1;
				break;

			case BCODE_JUMP2_BACKWARD:
				FETCH_PARAM_CODE_TO (stix, b1);
				LOG_INST_1 (stix, "jump2_backward %zu", b1);
				stix->ip -= MAX_CODE_JUMP + b1;
				break;

			/* -------------------------------------------------------- */

			case BCODE_PUSH_CTXTEMPVAR_X:
			case BCODE_STORE_INTO_CTXTEMPVAR_X:
			case BCODE_POP_INTO_CTXTEMPVAR_X:
				FETCH_PARAM_CODE_TO (stix, b1);
				FETCH_PARAM_CODE_TO (stix, b2);
				goto handle_ctxtempvar;
			case BCODE_PUSH_CTXTEMPVAR_0:
			case BCODE_PUSH_CTXTEMPVAR_1:
			case BCODE_PUSH_CTXTEMPVAR_2:
			case BCODE_PUSH_CTXTEMPVAR_3:
			case BCODE_STORE_INTO_CTXTEMPVAR_0:
			case BCODE_STORE_INTO_CTXTEMPVAR_1:
			case BCODE_STORE_INTO_CTXTEMPVAR_2:
			case BCODE_STORE_INTO_CTXTEMPVAR_3:
			case BCODE_POP_INTO_CTXTEMPVAR_0:
			case BCODE_POP_INTO_CTXTEMPVAR_1:
			case BCODE_POP_INTO_CTXTEMPVAR_2:
			case BCODE_POP_INTO_CTXTEMPVAR_3:
			{
				stix_ooi_t i;
				stix_oop_context_t ctx;

				b1 = bcode & 0x3; /* low 2 bits */
				FETCH_BYTE_CODE_TO (stix, b2);

			handle_ctxtempvar:

				ctx = stix->active_context;
				STIX_ASSERT ((stix_oop_t)ctx != stix->_nil);
				for (i = 0; i < b1; i++)
				{
					ctx = (stix_oop_context_t)ctx->home;
				}

				if ((bcode >> 3) & 1)
				{
					/* store or pop */
					ctx->slot[b2] = STIX_STACK_GETTOP(stix);

					if ((bcode >> 2) & 1)
					{
						/* pop */
						STIX_STACK_POP (stix);
						LOG_INST_2 (stix, "pop_into_ctxtempvar %zu %zu", b1, b2);
					}
					else
					{
						LOG_INST_2 (stix, "store_into_ctxtempvar %zu %zu", b1, b2);
					}
				}
				else
				{
					/* push */
					STIX_STACK_PUSH (stix, ctx->slot[b2]);
					LOG_INST_2 (stix, "push_ctxtempvar %zu %zu", b1, b2);
				}

				break;
			}
			/* -------------------------------------------------------- */

			case BCODE_PUSH_OBJVAR_X:
			case BCODE_STORE_INTO_OBJVAR_X:
			case BCODE_POP_INTO_OBJVAR_X:
				FETCH_PARAM_CODE_TO (stix, b1);
				FETCH_PARAM_CODE_TO (stix, b2);
				goto handle_objvar;

			case BCODE_PUSH_OBJVAR_0:
			case BCODE_PUSH_OBJVAR_1:
			case BCODE_PUSH_OBJVAR_2:
			case BCODE_PUSH_OBJVAR_3:
			case BCODE_STORE_INTO_OBJVAR_0:
			case BCODE_STORE_INTO_OBJVAR_1:
			case BCODE_STORE_INTO_OBJVAR_2:
			case BCODE_STORE_INTO_OBJVAR_3:
			case BCODE_POP_INTO_OBJVAR_0:
			case BCODE_POP_INTO_OBJVAR_1:
			case BCODE_POP_INTO_OBJVAR_2:
			case BCODE_POP_INTO_OBJVAR_3:
			{
				stix_oop_oop_t t;

				/* b1 -> variable index to the object indicated by b2.
				 * b2 -> object index stored in the literal frame. */
				b1 = bcode & 0x3; /* low 2 bits */
				FETCH_BYTE_CODE_TO (stix, b2);

			handle_objvar:
				t = (stix_oop_oop_t)stix->active_method->slot[b2];
				STIX_ASSERT (STIX_OBJ_GET_FLAGS_TYPE(t) == STIX_OBJ_TYPE_OOP);
				STIX_ASSERT (b1 < STIX_OBJ_GET_SIZE(t));

				if ((bcode >> 3) & 1)
				{
					/* store or pop */

					t->slot[b1] = STIX_STACK_GETTOP(stix);

					if ((bcode >> 2) & 1)
					{
						/* pop */
						STIX_STACK_POP (stix);
						LOG_INST_2 (stix, "pop_into_objvar %zu %zu", b1, b2);
					}
					else
					{
						LOG_INST_2 (stix, "store_into_objvar %zu %zu", b1, b2);
					}
				}
				else
				{
					/* push */
					LOG_INST_2 (stix, "push_objvar %zu %zu", b1, b2);
					STIX_STACK_PUSH (stix, t->slot[b1]);
				}
				break;
			}

			/* -------------------------------------------------------- */
			case BCODE_SEND_MESSAGE_X:
			case BCODE_SEND_MESSAGE_TO_SUPER_X:
				/* b1 -> number of arguments 
				 * b2 -> selector index stored in the literal frame */
				FETCH_PARAM_CODE_TO (stix, b1);
				FETCH_PARAM_CODE_TO (stix, b2);
				goto handle_send_message;

			case BCODE_SEND_MESSAGE_0:
			case BCODE_SEND_MESSAGE_1:
			case BCODE_SEND_MESSAGE_2:
			case BCODE_SEND_MESSAGE_3:
			case BCODE_SEND_MESSAGE_TO_SUPER_0:
			case BCODE_SEND_MESSAGE_TO_SUPER_1:
			case BCODE_SEND_MESSAGE_TO_SUPER_2:
			case BCODE_SEND_MESSAGE_TO_SUPER_3:
			{
				stix_oop_char_t selector;

				b1 = bcode & 0x3; /* low 2 bits */
				FETCH_BYTE_CODE_TO (stix, b2);

			handle_send_message:
				/* get the selector from the literal frame */
				selector = (stix_oop_char_t)stix->active_method->slot[b2];

				LOG_INST_3 (stix, "send_message%hs %zu @%zu", (((bcode >> 2) & 1)? "_to_super": ""), b1, b2);

				if (send_message (stix, selector, ((bcode >> 2) & 1), b1) <= -1) goto oops;
				break; /* CMD_SEND_MESSAGE */
			}

			/* -------------------------------------------------------- */

			case BCODE_PUSH_RECEIVER:
				LOG_INST_0 (stix, "push_receiver");
				STIX_STACK_PUSH (stix, stix->active_context->origin->receiver_or_source);
				break;

			case BCODE_PUSH_NIL:
				LOG_INST_0 (stix, "push_nil");
				STIX_STACK_PUSH (stix, stix->_nil);
				break;

			case BCODE_PUSH_TRUE:
				LOG_INST_0 (stix, "push_true");
				STIX_STACK_PUSH (stix, stix->_true);
				break;

			case BCODE_PUSH_FALSE:
				LOG_INST_0 (stix, "push_false");
				STIX_STACK_PUSH (stix, stix->_false);
				break;

			case BCODE_PUSH_CONTEXT:
				LOG_INST_0 (stix, "push_context");
				STIX_STACK_PUSH (stix, (stix_oop_t)stix->active_context);
				break;

			case BCODE_PUSH_PROCESS:
				LOG_INST_0 (stix, "push_process");
				STIX_STACK_PUSH (stix, (stix_oop_t)stix->processor->active);
				break;

			case BCODE_PUSH_NEGONE:
				LOG_INST_0 (stix, "push_negone");
				STIX_STACK_PUSH (stix, STIX_SMOOI_TO_OOP(-1));
				break;

			case BCODE_PUSH_ZERO:
				LOG_INST_0 (stix, "push_zero");
				STIX_STACK_PUSH (stix, STIX_SMOOI_TO_OOP(0));
				break;

			case BCODE_PUSH_ONE:
				LOG_INST_0 (stix, "push_one");
				STIX_STACK_PUSH (stix, STIX_SMOOI_TO_OOP(1));
				break;

			case BCODE_PUSH_TWO:
				LOG_INST_0 (stix, "push_two");
				STIX_STACK_PUSH (stix, STIX_SMOOI_TO_OOP(2));
				break;

			case BCODE_PUSH_INTLIT:
				FETCH_PARAM_CODE_TO (stix, b1);
				LOG_INST_1 (stix, "push_intlit %zu", b1);
				STIX_STACK_PUSH (stix, STIX_SMOOI_TO_OOP(b1));
				break;

			case BCODE_PUSH_NEGINTLIT:
			{
				stix_ooi_t num;
				FETCH_PARAM_CODE_TO (stix, b1);
				num = b1;
				LOG_INST_1 (stix, "push_negintlit %zu", b1);
				STIX_STACK_PUSH (stix, STIX_SMOOI_TO_OOP(-num));
				break;
			}

			case BCODE_PUSH_CHARLIT:
				FETCH_PARAM_CODE_TO (stix, b1);
				LOG_INST_1 (stix, "push_charlit %zu", b1);
				STIX_STACK_PUSH (stix, STIX_CHAR_TO_OOP(b1));
				break;
			/* -------------------------------------------------------- */

			case BCODE_DUP_STACKTOP:
			{
				stix_oop_t t;
				LOG_INST_0 (stix, "dup_stacktop");
				STIX_ASSERT (!STIX_STACK_ISEMPTY(stix));
				t = STIX_STACK_GETTOP(stix);
				STIX_STACK_PUSH (stix, t);
				break;
			}

			case BCODE_POP_STACKTOP:
				LOG_INST_0 (stix, "pop_stacktop");
				STIX_ASSERT (!STIX_STACK_ISEMPTY(stix));
				STIX_STACK_POP (stix);
				break;

			case BCODE_RETURN_STACKTOP:
				LOG_INST_0 (stix, "return_stacktop");
				return_value = STIX_STACK_GETTOP(stix);
				STIX_STACK_POP (stix);
				goto handle_return;

			case BCODE_RETURN_RECEIVER:
				LOG_INST_0 (stix, "return_receiver");
				return_value = stix->active_context->origin->receiver_or_source;

			handle_return:
			#if 0
				/* put the instruction pointer back to the return
				 * instruction (RETURN_RECEIVER or RETURN_RECEIVER)
				 * if a context returns into this context again,
				 * it'll be able to return as well again.
				 * 
				 * Consider a program like this:
				 *
				 * #class MyObject(Object)
				 * {
				 *   #declare(#classinst) t1 t2.
				 *   #method(#class) xxxx
				 *   {
				 *     | g1 g2 |
				 *     t1 dump.
				 *     t2 := [ g1 := 50. g2 := 100. ^g1 + g2 ].
				 *     (t1 < 100) ifFalse: [ ^self ].
				 *     t1 := t1 + 1. 
				 *     ^self xxxx.
				 *   }
				 *   #method(#class) main
				 *   {
				 *     t1 := 1.
				 *     self xxxx.
				 *     t2 := t2 value.  
				 *     t2 dump.
				 *   }
				 * }
				 *
				 * the 'xxxx' method invoked by 'self xxxx' has 
				 * returned even before 't2 value' is executed.
				 * the '^' operator makes the active context to
				 * switch to its 'origin->sender' which is the
				 * method context of 'xxxx' itself. placing its
				 * instruction pointer at the 'return' instruction
				 * helps execute another return when the switching
				 * occurs.
				 * 
				 * TODO: verify if this really works
				 *
				 */
				stix->ip--; 
			#else
				if (stix->active_context->origin == stix->processor->active->initial_context->origin)
				{
					/* method return from a processified block
					 * 
 					 * #method(#class) main
					 * {
					 *    [^100] newProcess resume.
					 *    '1111' dump.
					 *    '1111' dump.
					 *    '1111' dump.
					 *    ^300.
					 * }
					 * 
					 * ^100 doesn't terminate a main process as the block
					 * has been processified. on the other hand, ^100
					 * in the following program causes main to exit.
					 * 
					 * #method(#class) main
					 * {
					 *    [^100] value.
					 *    '1111' dump.
					 *    '1111' dump.
					 *    '1111' dump.
					 *    ^300.
					 * }
					 */

					STIX_ASSERT (STIX_CLASSOF(stix, stix->active_context) == stix->_block_context);
					STIX_ASSERT (STIX_CLASSOF(stix, stix->processor->active->initial_context) == stix->_block_context);

					/* decrement the instruction pointer back to the return instruction.
					 * even if the context is reentered, it will just return.
					 *stix->ip--;*/

					terminate_process (stix, stix->processor->active);
				}
				else 
				{
					unwind_protect = 0;

					/* set the instruction pointer to an invalid value.
					 * this is stored into the current method context
					 * before context switching and marks a dead context */
					if (stix->active_context->origin == stix->active_context)
					{
						/* returning from a method */
						STIX_ASSERT (STIX_CLASSOF(stix, stix->active_context) == stix->_method_context);
						stix->ip = -1;
					}
					else
					{
						stix_oop_context_t ctx;

						/* method return from within a block(including a non-local return) */
						STIX_ASSERT (STIX_CLASSOF(stix, stix->active_context) == stix->_block_context);

						ctx = stix->active_context;
						while ((stix_oop_t)ctx != stix->_nil)
						{
							if (STIX_CLASSOF(stix, ctx) == stix->_method_context)
							{
								stix_ooi_t preamble;
								preamble = STIX_OOP_TO_SMOOI(((stix_oop_method_t)ctx->method_or_nargs)->preamble);
								if (STIX_METHOD_GET_PREAMBLE_CODE(preamble) == STIX_METHOD_PREAMBLE_ENSURE)
								{
									if (!unwind_protect)
									{
										unwind_protect = 1;
										unwind_start = ctx;
									}
									unwind_stop = ctx;
								}
							}
							if (ctx == stix->active_context->origin) goto non_local_return_ok;
							ctx = ctx->sender;
						}

						/* cannot return from a method that has returned already */
						STIX_ASSERT (STIX_CLASSOF(stix, stix->active_context->origin) == stix->_method_context);
						STIX_ASSERT (stix->active_context->origin->ip == STIX_SMOOI_TO_OOP(-1));

						STIX_LOG0 (stix, STIX_LOG_IC | STIX_LOG_ERROR, "Error - cannot return from dead context\n");
						stix->errnum = STIX_EINTERN; /* TODO: can i make this error catchable at the stix level? */
						return -1;

					non_local_return_ok:
/*STIX_DEBUG2 (stix, "NON_LOCAL RETURN OK TO... %p %p\n", stix->active_context->origin, stix->active_context->origin->sender);*/
						stix->active_context->origin->ip = STIX_SMOOI_TO_OOP(-1);
					}

					STIX_ASSERT (STIX_CLASSOF(stix, stix->active_context->origin) == stix->_method_context);
					/* restore the stack pointer */
					stix->sp = STIX_OOP_TO_SMOOI(stix->active_context->origin->sp);
					SWITCH_ACTIVE_CONTEXT (stix, stix->active_context->origin->sender);

					if (unwind_protect)
					{
						static stix_ooch_t fbm[] = { 
							'u', 'n', 'w', 'i', 'n', 'd', 'T', 'o', ':', 
							'r', 'e', 't', 'u', 'r', 'n', ':'
						};

						STIX_STACK_PUSH (stix, (stix_oop_t)unwind_start);
						STIX_STACK_PUSH (stix, (stix_oop_t)unwind_stop);
						STIX_STACK_PUSH (stix, (stix_oop_t)return_value);

						if (send_private_message (stix, fbm, 16, 0, 2) <= -1) return -1;
					}
					else
					{
						/* push the return value to the stack of the new active context */
						STIX_STACK_PUSH (stix, return_value);

						if (stix->active_context == stix->initial_context)
						{
							/* the new active context is the fake initial context.
							 * this context can't get executed further. */
							STIX_ASSERT ((stix_oop_t)stix->active_context->sender == stix->_nil);
							STIX_ASSERT (STIX_CLASSOF(stix, stix->active_context) == stix->_method_context);
							STIX_ASSERT (stix->active_context->receiver_or_source == stix->_nil);
							STIX_ASSERT (stix->active_context == stix->processor->active->initial_context);
							STIX_ASSERT (stix->active_context->origin == stix->processor->active->initial_context->origin);
							STIX_ASSERT (stix->active_context->origin == stix->active_context);

							/* NOTE: this condition is true for the processified block context also.
							 *   stix->active_context->origin == stix->processor->active->initial_context->origin
							 *   however, the check here is done after context switching and the
							 *   processified block check has been done against the context before switching */

							/* the stack contains the final return value so the stack pointer must be 0. */
							STIX_ASSERT (stix->sp == 0); 

							if (stix->option.trait & STIX_AWAIT_PROCS)
								terminate_process (stix, stix->processor->active);
							else
								goto done;

							/* TODO: store the return value to the VM register.
							 * the caller to stix_execute() can fetch it to return it to the system */
						}
					}
				}

			#endif

				break;

			case BCODE_RETURN_FROM_BLOCK:
				LOG_INST_0 (stix, "return_from_block");

				STIX_ASSERT(STIX_CLASSOF(stix, stix->active_context) == stix->_block_context);

				if (stix->active_context == stix->processor->active->initial_context)
				{
					/* the active context to return from is an initial context of
					 * the active process. this process must have been created 
					 * over a block using the newProcess method. let's terminate
					 * the process. */

					STIX_ASSERT ((stix_oop_t)stix->active_context->sender == stix->_nil);
					terminate_process (stix, stix->processor->active);
				}
				else
				{
					/* it is a normal block return as the active block context 
					 * is not the initial context of a process */

					/* the process stack is shared. the return value 
					 * doesn't need to get moved. */
					SWITCH_ACTIVE_CONTEXT (stix, (stix_oop_context_t)stix->active_context->sender);
				}

				break;

			case BCODE_MAKE_BLOCK:
			{
				stix_oop_context_t blkctx;

				/* b1 - number of block arguments
				 * b2 - number of block temporaries */
				FETCH_PARAM_CODE_TO (stix, b1);
				FETCH_PARAM_CODE_TO (stix, b2);

				LOG_INST_2 (stix, "make_block %zu %zu", b1, b2);

				STIX_ASSERT (b1 >= 0);
				STIX_ASSERT (b2 >= b1);

				/* the block context object created here is used as a base
				 * object for block context activation. pf_block_value()
				 * clones a block context and activates the cloned context.
				 * this base block context is created with no stack for 
				 * this reason */
				blkctx = (stix_oop_context_t)stix_instantiate (stix, stix->_block_context, STIX_NULL, 0); 
				if (!blkctx) return -1;

				/* the long forward jump instruction has the format of 
				 *   11000100 KKKKKKKK or 11000100 KKKKKKKK KKKKKKKK 
				 * depending on STIX_BCODE_LONG_PARAM_SIZE. change 'ip' to point to
				 * the instruction after the jump. */
				blkctx->ip = STIX_SMOOI_TO_OOP(stix->ip + STIX_BCODE_LONG_PARAM_SIZE + 1);
				/* stack pointer below the bottom. this base block context
				 * has an empty stack anyway. */
				blkctx->sp = STIX_SMOOI_TO_OOP(-1);
				/* the number of arguments for a block context is local to the block */
				blkctx->method_or_nargs = STIX_SMOOI_TO_OOP(b1);
				/* the number of temporaries here is an accumulated count including
				 * the number of temporaries of a home context */
				blkctx->ntmprs = STIX_SMOOI_TO_OOP(b2);

				/* set the home context where it's defined */
				blkctx->home = (stix_oop_t)stix->active_context; 
				/* no source for a base block context. */
				blkctx->receiver_or_source = stix->_nil; 

				blkctx->origin = stix->active_context->origin;

				/* push the new block context to the stack of the active context */
				STIX_STACK_PUSH (stix, (stix_oop_t)blkctx);
				break;
			}

			case BCODE_SEND_BLOCK_COPY:
			{
				stix_ooi_t nargs, ntmprs;
				stix_oop_context_t rctx;
				stix_oop_context_t blkctx;

				LOG_INST_0 (stix, "send_block_copy");

				/* it emulates thisContext blockCopy: nargs ofTmprCount: ntmprs */
				STIX_ASSERT (stix->sp >= 2);

				STIX_ASSERT (STIX_CLASSOF(stix, STIX_STACK_GETTOP(stix)) == stix->_small_integer);
				ntmprs = STIX_OOP_TO_SMOOI(STIX_STACK_GETTOP(stix));
				STIX_STACK_POP (stix);

				STIX_ASSERT (STIX_CLASSOF(stix, STIX_STACK_GETTOP(stix)) == stix->_small_integer);
				nargs = STIX_OOP_TO_SMOOI(STIX_STACK_GETTOP(stix));
				STIX_STACK_POP (stix);

				STIX_ASSERT (nargs >= 0);
				STIX_ASSERT (ntmprs >= nargs);

				/* the block context object created here is used
				 * as a base object for block context activation.
				 * pf_block_value() clones a block 
				 * context and activates the cloned context.
				 * this base block context is created with no 
				 * stack for this reason. */
				blkctx = (stix_oop_context_t)stix_instantiate (stix, stix->_block_context, STIX_NULL, 0); 
				if (!blkctx) return -1;

				/* get the receiver to the block copy message after block context instantiation
				 * not to get affected by potential GC */
				rctx = (stix_oop_context_t)STIX_STACK_GETTOP(stix);
				STIX_ASSERT (rctx == stix->active_context);

				/* [NOTE]
				 *  blkctx->sender is left to nil. it is set to the 
				 *  active context before it gets activated. see
				 *  pf_block_value().
				 *
				 *  blkctx->home is set here to the active context.
				 *  it's redundant to have them pushed to the stack
				 *  though it is to emulate the message sending of
				 *  blockCopy:withNtmprs:. BCODE_MAKE_BLOCK has been
				 *  added to replace BCODE_SEND_BLOCK_COPY and pusing
				 *  arguments to the stack.
				 *
				 *  blkctx->origin is set here by copying the origin
				 *  of the active context.
				 */

				/* the extended jump instruction has the format of 
				 *   0000XXXX KKKKKKKK or 0000XXXX KKKKKKKK KKKKKKKK 
				 * depending on STIX_BCODE_LONG_PARAM_SIZE. change 'ip' to point to
				 * the instruction after the jump. */
				blkctx->ip = STIX_SMOOI_TO_OOP(stix->ip + STIX_BCODE_LONG_PARAM_SIZE + 1);
				blkctx->sp = STIX_SMOOI_TO_OOP(-1);
				/* the number of arguments for a block context is local to the block */
				blkctx->method_or_nargs = STIX_SMOOI_TO_OOP(nargs);
				/* the number of temporaries here is an accumulated count including
				 * the number of temporaries of a home context */
				blkctx->ntmprs = STIX_SMOOI_TO_OOP(ntmprs);

				blkctx->home = (stix_oop_t)rctx;
				blkctx->receiver_or_source = stix->_nil;

#if 0
				if (rctx->home == stix->_nil)
				{
					/* the context that receives the blockCopy message is a method context */
					STIX_ASSERT (STIX_CLASSOF(stix, rctx) == stix->_method_context);
					STIX_ASSERT (rctx == (stix_oop_t)stix->active_context);
					blkctx->origin = (stix_oop_context_t)rctx;
				}
				else
				{
					/* a block context is active */
					STIX_ASSERT (STIX_CLASSOF(stix, rctx) == stix->_block_context);
					blkctx->origin = ((stix_oop_block_context_t)rctx)->origin;
				}
#else

				/* [NOTE]
				 * the origin of a method context is set to itself
				 * when it's created. so it's safe to simply copy
				 * the origin field this way.
				 */
				blkctx->origin = rctx->origin;
#endif

				STIX_STACK_SETTOP (stix, (stix_oop_t)blkctx);
				break;
			}

			case BCODE_NOOP:
				/* do nothing */
				LOG_INST_0 (stix, "noop");
				break;


			default:
				STIX_LOG1 (stix, STIX_LOG_IC | STIX_LOG_FATAL, "Fatal error - unknown byte code 0x%zx\n", bcode);
				stix->errnum = STIX_EINTERN;
				goto oops;
		}
	}

done:

	vm_cleanup (stix);
#if defined(STIX_PROFILE_VM)
	STIX_LOG1 (stix, STIX_LOG_IC | STIX_LOG_INFO, "TOTAL_INST_COUTNER = %zu\n", inst_counter);
#endif
	return 0;

oops:
	/* TODO: anything to do here? */
	return -1;
}

int stix_invoke (stix_t* stix, const stix_oocs_t* objname, const stix_oocs_t* mthname)
{
	int n;

	STIX_ASSERT (stix->initial_context == STIX_NULL);
	STIX_ASSERT (stix->active_context == STIX_NULL);
	STIX_ASSERT (stix->active_method == STIX_NULL);

	if (start_initial_process_and_context (stix, objname, mthname) <= -1) return -1;
	stix->initial_context = stix->processor->active->initial_context;

	n = stix_execute (stix);

/* TODO: reset processor fields. set processor->tally to zero. processor->active to nil_process... */
	stix->initial_context = STIX_NULL;
	stix->active_context = STIX_NULL;
	stix->active_method = STIX_NULL;
	return n;
}

