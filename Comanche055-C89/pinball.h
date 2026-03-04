/*
 * pinball.h -- Verb/Noun processing, display interface, monitor verbs.
 *
 * The "Pinball Game" — DSKY software driver.  Handles keyboard
 * input processing (CHARIN), verb/noun dispatch, display formatting,
 * data entry (V21-V25), monitor verbs (V16), and ENDIDLE operator
 * wait.  Maps to PINBALL_GAME_BUTTONS_AND_LIGHTS.agc.
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

#ifndef PINBALL_H
#define PINBALL_H

#include "agc.h"
#include "dsky.h"

/* ----------------------------------------------------------------
 * Verb/Noun state
 * ---------------------------------------------------------------- */

extern int pinball_verb;
extern int pinball_noun;

#define PINBALL_BUF_SIZE 6
extern int pinball_inbuf[PINBALL_BUF_SIZE];
extern int pinball_incount;

#define PINBALL_MODE_IDLE     0
#define PINBALL_MODE_VERB     1
#define PINBALL_MODE_NOUN     2
#define PINBALL_MODE_DATA     3
#define PINBALL_MODE_PROCEED  4
extern int pinball_mode;

extern int pinball_data_reg;

extern int pinball_monitor_active;
extern int pinball_monitor_verb;
extern int pinball_monitor_noun;

extern int pinball_endidle;

/* ----------------------------------------------------------------
 * Pinball API
 * ---------------------------------------------------------------- */

void pinball_init(void);
void pinball_keypress(int keycode);
int pinball_nvsub(int verb, int noun);
int pinball_wait_endidle(void);
void pinball_display_val(int reg, int value, int is_signed);
void pinball_display_octal(int reg, int value);
void pinball_show_verb(int v);
void pinball_show_noun(int n);
void pinball_show_prog(int p);

#endif /* PINBALL_H */
