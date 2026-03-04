/*
 * timer.h -- Real-time clock (TIME1-TIME6), interrupt dispatch.
 *
 * The main loop calls timer_tick() every 10ms.  Each tick increments
 * TIME1 and manages T3RUPT (waitlist) and T4RUPT (DSKY display scan).
 * TIME1+TIME2 form the mission elapsed time clock.
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

#ifndef TIMER_H
#define TIMER_H

#include "agc.h"

void timer_init(void);
void timer_tick(void);

#endif /* TIMER_H */
