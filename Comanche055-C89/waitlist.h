/*
 * waitlist.h -- Timer-driven task scheduler
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port
 * Faithful reproduction of the AGC Waitlist (9 task slots)
 */

#ifndef WAITLIST_H
#define WAITLIST_H

#include "agc.h"

/* ----------------------------------------------------------------
 * Waitlist task slot
 * ---------------------------------------------------------------- */

typedef struct {
    int             delta_t;    /* Centiseconds until fire (0 = empty) */
    agc_taskfunc_t  task;       /* Task function to call */
} agc_waitlist_slot_t;

/* Task slots: LST1 holds delta-times, LST2 holds task addresses */
extern agc_waitlist_slot_t agc_waitlist[NUM_WAITLIST_TASKS];

/* ----------------------------------------------------------------
 * Waitlist API
 * ---------------------------------------------------------------- */

/* Initialize the waitlist (clear all slots) */
void waitlist_init(void);

/* Schedule a task to run after dt centiseconds.
 * Returns slot index, or -1 if no free slot. */
int waitlist_add(int dt_centisecs, agc_taskfunc_t task);

/* Reschedule from within a running task: fixed delay */
int waitlist_fixdelay(int dt_centisecs, agc_taskfunc_t task);

/* Long call: for delays > 16383 centiseconds (163.84 sec)
 * Note: Only one longcall can be active at a time (original AGC behavior) */
int waitlist_longcall(int dt_centisecs, agc_taskfunc_t task);

/* T3RUPT dispatch: decrement all timers, fire any that reach zero.
 * Called by the timer module every centisecond tick. */
void waitlist_t3rupt(void);

#endif /* WAITLIST_H */
