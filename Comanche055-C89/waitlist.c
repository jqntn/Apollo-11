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

static agc_taskfunc_t longcall_target = NULL;
static int longcall_remaining = 0;
static int longcall_active = 0;

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
    longcall_target = NULL;
    longcall_remaining = 0;
    longcall_active = 0;
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

static int waitlist_fixdelay(int dt_centisecs, agc_taskfunc_t task)
{
    return waitlist_add(dt_centisecs, task);
}

/* ----------------------------------------------------------------
 * LONGCALL: for delays > 16383 centiseconds
 * ---------------------------------------------------------------- */

/* LONGCYCL continuation task */
static void longcyl(void)
{
    if (longcall_remaining > 16383) {
        /* MUCHTIME path */
        longcall_remaining -= 16383;
        waitlist_add(16383, longcyl);
    } else if (longcall_remaining > 0) {
        /* LASTTIME path */
        int dt = longcall_remaining;
        agc_taskfunc_t target = longcall_target;
        longcall_remaining = 0;
        longcall_target = NULL;
        longcall_active = 0;
        waitlist_add(dt, target);
    }
}

static int waitlist_longcall(int dt_centisecs, agc_taskfunc_t task)
{
    int slot;
    if (dt_centisecs <= 16383)
        return waitlist_add(dt_centisecs, task);

    if (longcall_active)
        return -1;

    slot = waitlist_add(16383, longcyl);
    if (slot >= 0) {
        longcall_target = task;
        longcall_remaining = dt_centisecs - 16383;
        longcall_active = 1;
    }
    return slot;
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
