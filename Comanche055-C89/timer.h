/*
 * timer.h -- Real-time clock (TIME1-TIME6), interrupt dispatch
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port
 */

#ifndef TIMER_H
#define TIMER_H

#include "agc.h"

/* T4RUPT display phase counter (cycles through DSKY relay word outputs) */
extern int t4rupt_phase;

/* Initialize timers */
void timer_init(void);

/* Tick the timer system: called every 10ms from main loop.
 * Advances TIME1-6, fires T3RUPT (waitlist), T4RUPT (DSKY), etc. */
void timer_tick(void);

#endif /* TIMER_H */
