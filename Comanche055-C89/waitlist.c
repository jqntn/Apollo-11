/*
 * waitlist.c -- Timer-driven task scheduler
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port
 * Faithful reproduction of the AGC Waitlist (9 task slots)
 *
 * Tasks are scheduled with a delay in centiseconds. Each T3RUPT tick
 * (every centisecond) decrements all active timers. When a timer reaches
 * zero, the associated task is dispatched immediately.
 */

#include <string.h>
#include "waitlist.h"

/* ----------------------------------------------------------------
 * State
 * ---------------------------------------------------------------- */

agc_waitlist_slot_t agc_waitlist[NUM_WAITLIST_TASKS];

/* ----------------------------------------------------------------
 * Init
 * ---------------------------------------------------------------- */

void waitlist_init(void)
{
    memset(agc_waitlist, 0, sizeof(agc_waitlist));
}

/* ----------------------------------------------------------------
 * Add a task (WAITLIST calling sequence)
 * ---------------------------------------------------------------- */

int waitlist_add(int dt_centisecs, agc_taskfunc_t task)
{
    int i;
    if (dt_centisecs <= 0) dt_centisecs = 1;
    if (task == NULL) return -1;

    for (i = 0; i < NUM_WAITLIST_TASKS; i++) {
        if (agc_waitlist[i].task == NULL) {
            agc_waitlist[i].delta_t = dt_centisecs;
            agc_waitlist[i].task = task;
            return i;
        }
    }
    return -1;  /* No free slot */
}

/* ----------------------------------------------------------------
 * Reschedule from within a running task
 * ---------------------------------------------------------------- */

int waitlist_fixdelay(int dt_centisecs, agc_taskfunc_t task)
{
    return waitlist_add(dt_centisecs, task);
}

/* ----------------------------------------------------------------
 * Long call: for delays > 16383 centiseconds
 * ---------------------------------------------------------------- */

static void longcall_continue(void);
static agc_taskfunc_t longcall_target;
static int longcall_remaining;

static void longcall_continue(void)
{
    if (longcall_remaining > 16383) {
        longcall_remaining -= 16383;
        waitlist_add(16383, longcall_continue);
    } else if (longcall_remaining > 0) {
        int dt = longcall_remaining;
        longcall_remaining = 0;
        waitlist_add(dt, longcall_target);
    }
}

int waitlist_longcall(int dt_centisecs, agc_taskfunc_t task)
{
    if (dt_centisecs <= 16383) {
        return waitlist_add(dt_centisecs, task);
    }
    longcall_target = task;
    longcall_remaining = dt_centisecs - 16383;
    return waitlist_add(16383, longcall_continue);
}

/* ----------------------------------------------------------------
 * T3RUPT dispatch
 * ----------------------------------------------------------------
 * Called every centisecond. Decrements all active timers.
 * Fires tasks whose timers reach zero.
 */

void waitlist_t3rupt(void)
{
    int i;
    for (i = 0; i < NUM_WAITLIST_TASKS; i++) {
        if (agc_waitlist[i].task != NULL) {
            agc_waitlist[i].delta_t--;
            if (agc_waitlist[i].delta_t <= 0) {
                agc_taskfunc_t task = agc_waitlist[i].task;
                agc_waitlist[i].task = NULL;
                agc_waitlist[i].delta_t = 0;
                task();
            }
        }
    }
}
