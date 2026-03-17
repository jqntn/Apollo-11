/*
 * timer.c -- Real-time clock and interrupt dispatch.
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

#include "agc_cpu.h"
#include "dsky.h"
#include "timer.h"
#include "waitlist.h"

/* ----------------------------------------------------------------
 * State
 * ---------------------------------------------------------------- */

static int t4rupt_phase = 0;

static int cs_accumulator = 0;
static int t3_counter = 1;
static int t4_counter = 2; /* ~50 Hz display scan */

/* ----------------------------------------------------------------
 * Init
 * ---------------------------------------------------------------- */

void
timer_init(void)
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

void
timer_tick(void)
{
  /* Increment TIME1 (centiseconds) */
  agc_time1++;
  if (agc_time1 > 16383) {
    agc_time1 = 0;
    agc_time2++;
    if (agc_time2 > 16383) {
      agc_time2 = 0;
    }
  }

  /* T3RUPT: drive waitlist every centisecond */
  t3_counter--;
  if (t3_counter <= 0) {
    t3_counter = 1;
    if (!agc_inhint) {
      waitlist_t3rupt();
    }
  }

  /* T4RUPT: drive DSKY display scan */
  t4_counter--;
  if (t4_counter <= 0) {
    t4_counter = 2;
    if (!agc_inhint) {
      dsky_t4rupt();
    }
  }
}
