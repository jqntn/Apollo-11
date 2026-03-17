/*
 * waitlist.h -- Timer-driven task scheduler (WAITLIST.agc).
 *
 * Schedules tasks to fire after a delay in centiseconds.  Each
 * T3RUPT tick decrements all active timers; when a timer reaches
 * zero the task is dispatched.  Supports up to 9 task slots (LST1/
 * LST2), FIXDELAY rescheduling, and LONGCALL for delays exceeding
 * 16383 centiseconds (~163.84 sec).
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

#ifndef WAITLIST_H
#define WAITLIST_H

#include "agc.h"

typedef struct
{
  int delta_t; /* Centiseconds until fire (0 = empty) */
  agc_taskfunc_t task;
} agc_waitlist_slot_t;

extern agc_waitlist_slot_t agc_waitlist[NUM_WAITLIST_TASKS];

void
waitlist_init(void);
int
waitlist_add(int dt_centisecs, agc_taskfunc_t task);
int
waitlist_fixdelay(int dt_centisecs, agc_taskfunc_t task);
int
waitlist_longcall(int dt_centisecs, agc_taskfunc_t task);
void
waitlist_t3rupt(void);

#endif /* WAITLIST_H */
