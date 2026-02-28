/*
 * alarm.c -- Alarm and abort handling
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port
 */

#include "alarm.h"
#include "agc_cpu.h"
#include "executive.h"

/* ----------------------------------------------------------------
 * State
 * ---------------------------------------------------------------- */

int agc_alarm_code = 0;
int agc_prog_alarm = 0;

/* ----------------------------------------------------------------
 * Set alarm
 * ---------------------------------------------------------------- */

void alarm_set(int code)
{
    agc_alarm_code = code;
    agc_prog_alarm = 1;
    /* Light PROG alarm on DSKY via channel 11 */
    agc_channels[CHAN_DSALMOUT] |= BIT11;
}

/* ----------------------------------------------------------------
 * Abort (POODOO)
 * ---------------------------------------------------------------- */

void alarm_abort(int code)
{
    alarm_set(code);
    /* End the current job */
    exec_endofjob();
}

/* ----------------------------------------------------------------
 * Reset alarms (RSET key)
 * ---------------------------------------------------------------- */

void alarm_reset(void)
{
    agc_alarm_code = 0;
    agc_prog_alarm = 0;
    /* Clear PROG alarm light */
    agc_channels[CHAN_DSALMOUT] &= ~BIT11;
}
