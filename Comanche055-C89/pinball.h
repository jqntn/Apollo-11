/*
 * pinball.h -- Verb/Noun processing, display interface, monitor verbs
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port
 * The "Pinball Game" -- DSKY software driver
 */

#ifndef PINBALL_H
#define PINBALL_H

#include "agc.h"
#include "dsky.h"

/* ----------------------------------------------------------------
 * Verb/Noun state
 * ---------------------------------------------------------------- */

/* Current verb and noun registers */
extern int pinball_verb;
extern int pinball_noun;

/* Input buffer for digit entry */
#define PINBALL_BUF_SIZE 6
extern int pinball_inbuf[PINBALL_BUF_SIZE];
extern int pinball_incount;

/* Current input mode */
#define PINBALL_MODE_IDLE     0
#define PINBALL_MODE_VERB     1
#define PINBALL_MODE_NOUN     2
#define PINBALL_MODE_DATA     3
#define PINBALL_MODE_PROCEED  4
extern int pinball_mode;

/* Data entry target register (1=R1, 2=R2, 3=R3) */
extern int pinball_data_reg;

/* Monitor verb state */
extern int pinball_monitor_active;
extern int pinball_monitor_verb;
extern int pinball_monitor_noun;

/* ENDIDLE state: waiting for operator */
extern int pinball_endidle;

/* ----------------------------------------------------------------
 * Pinball API
 * ---------------------------------------------------------------- */

/* Initialize Pinball */
void pinball_init(void);

/* Process a DSKY keypress (CHARIN equivalent) */
void pinball_keypress(int keycode);

/* Internal verb-noun call (NVSUB equivalent):
 * Programs call this to display data on DSKY.
 * verb, noun: the verb/noun to execute
 * Returns: 0 on success */
int pinball_nvsub(int verb, int noun);

/* ENDIDLE: wait for operator to press ENTR or PRO.
 * Returns: 0 if ENTR pressed, 1 if PRO pressed */
int pinball_wait_endidle(void);

/* Display a 5-digit signed value in a register (1=R1, 2=R2, 3=R3) */
void pinball_display_val(int reg, int value, int is_signed);

/* Display octal value in a register */
void pinball_display_octal(int reg, int value);

/* Update monitor display (called periodically by waitlist task) */
void pinball_monitor_update(void);

/* Set verb/noun display digits */
void pinball_show_verb(int v);
void pinball_show_noun(int n);
void pinball_show_prog(int p);

#endif /* PINBALL_H */
