/*
 * timer.c -- Real-time clock (TIME1-TIME6), interrupt dispatch
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port
 *
 * The main loop calls timer_tick() every 10ms (100 Hz).
 * Each tick increments TIME1 and manages the countdown timers
 * TIME3 (waitlist), TIME4 (DSKY display scan), TIME5/TIME6 (DAP, idle).
 *
 * TIME1 counts up every 10ms. When TIME1 overflows (>16383), TIME2
 * increments (giving ~163.84 sec per TIME2 tick). Together TIME1+TIME2
 * form the mission elapsed time clock.
 */

#include "timer.h"
#include "agc_cpu.h"
#include "waitlist.h"

/* Forward declarations for T4RUPT handler (defined in pinball/dsky) */
extern void dsky_t4rupt(void);

/* ----------------------------------------------------------------
 * State
 * ---------------------------------------------------------------- */

int t4rupt_phase = 0;

/* Centisecond accumulator: we tick at 10ms (100 Hz), but the AGC
 * centisecond clock ticks at 10ms too, so 1 tick = 1 centisecond */
static int cs_accumulator = 0;

/* T3RUPT countdown: fires every centisecond to drive waitlist */
static int t3_counter = 1;

/* T4RUPT countdown: fires every 7.5ms in real AGC, we approximate
 * by firing every other tick (every 20ms ~= 50 Hz display scan) */
static int t4_counter = 2;

/* ----------------------------------------------------------------
 * Init
 * ---------------------------------------------------------------- */

void timer_init(void)
{
    agc_time1 = 0;
    agc_time2 = 0;
    agc_time3 = 0;
    agc_time4 = 0;
    agc_time5 = 0;
    agc_time6 = 0;
    t4rupt_phase = 0;
    cs_accumulator = 0;
    t3_counter = 1;
    t4_counter = 2;
}

/* ----------------------------------------------------------------
 * Timer tick (called every 10ms from main loop)
 * ---------------------------------------------------------------- */

void timer_tick(void)
{
    /* Increment TIME1 (mission elapsed time, centiseconds) */
    agc_time1++;
    if (agc_time1 > 16383) {
        agc_time1 = 0;
        /* TIME2 increments on TIME1 overflow */
        agc_time2++;
        if (agc_time2 > 16383) {
            agc_time2 = 0;
        }
    }

    /* T3RUPT: drive the waitlist every centisecond */
    t3_counter--;
    if (t3_counter <= 0) {
        t3_counter = 1;
        if (!agc_inhint) {
            waitlist_t3rupt();
        }
    }

    /* T4RUPT: drive DSKY display scan (~50 Hz) */
    t4_counter--;
    if (t4_counter <= 0) {
        t4_counter = 2;
        if (!agc_inhint) {
            dsky_t4rupt();
        }
    }
}
