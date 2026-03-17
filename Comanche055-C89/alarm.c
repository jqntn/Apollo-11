/*
 * alarm.c -- Alarm and abort handling.
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

#include "agc.h"
#include "agc_cpu.h"
#include "alarm.h"
#include "executive.h"

int agc_alarm_code = 0;
int agc_prog_alarm = 0;

void
alarm_set(int code)
{
  agc_alarm_code = code;
  agc_prog_alarm = 1;
  agc_channels[CHAN_DSALMOUT] |= BIT11;
}

void
alarm_abort(int code)
{
  alarm_set(code);
  exec_endofjob();
}

void
alarm_reset(void)
{
  agc_alarm_code = 0;
  agc_prog_alarm = 0;
  agc_channels[CHAN_DSALMOUT] &= ~BIT11;
}
