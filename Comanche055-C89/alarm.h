/*
 * alarm.h -- Alarm and abort handling (ALARM_AND_ABORT.agc).
 *
 * Provides alarm_set (PROGLARM), alarm_abort (POODOO), and
 * alarm_reset (RSET key handler).  Alarm codes are octal values
 * from the AGC documentation.
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

#ifndef ALARM_H
#define ALARM_H

#include "agc.h"

/* Alarm codes (octal) */
#define ALARM_POODOO     01407
#define ALARM_BAILOUT    01410
#define ALARM_PROG_ALARM 01520
#define ALARM_NO_VAC     01201
#define ALARM_NO_CORE    01202
#define ALARM_EXEC_OVF   01203

extern int agc_alarm_code;
extern int agc_prog_alarm;

void alarm_set(int code);
void alarm_abort(int code);
void alarm_reset(void);

#endif /* ALARM_H */
