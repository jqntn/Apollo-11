/*
 * waitlist.c -- Timer-driven task scheduler.
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

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
    int i;
    for (i = 0; i < NUM_WAITLIST_TASKS; i++) {
        agc_waitlist[i].delta_t = 0;
        agc_waitlist[i].task = NULL;
    }
}

/* ----------------------------------------------------------------
 * Add / reschedule
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
    return -1;
}

/* ----------------------------------------------------------------
 * T3RUPT dispatch (called every centisecond)
 * ---------------------------------------------------------------- */

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
