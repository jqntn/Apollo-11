/*
 * alarm.h -- Alarm and abort handling
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port
 */

#ifndef ALARM_H
#define ALARM_H

#include "agc.h"

/* Alarm codes (octal, from AGC documentation) */
#define ALARM_POODOO     01407  /* POODOO - restart or abort */
#define ALARM_BAILOUT     01410  /* BAILOUT - software restart */
#define ALARM_PROG_ALARM  01520  /* General program alarm */
#define ALARM_NO_VAC      01201  /* No VAC area available */
#define ALARM_NO_CORE     01202  /* No core set available */
#define ALARM_EXEC_OVF    01203  /* Executive overflow */

/* Last alarm code displayed */
extern int agc_alarm_code;

/* PROG alarm light state */
extern int agc_prog_alarm;

/* ----------------------------------------------------------------
 * Alarm API
 * ---------------------------------------------------------------- */

/* Set an alarm: stores code, lights PROG alarm on DSKY */
void alarm_set(int code);

/* Set alarm and abort current job (POODOO) */
void alarm_abort(int code);

/* Clear the alarm display (RSET key) */
void alarm_reset(void);

#endif /* ALARM_H */
