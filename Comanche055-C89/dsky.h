/*
 * dsky.h -- Console DSKY HAL: ASCII rendering + keyboard input
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port
 * Renders the DSKY display as ASCII art in the console terminal.
 */

#ifndef DSKY_H
#define DSKY_H

#include "agc.h"
#include "dsky_backend.h"

/* ----------------------------------------------------------------
 * DSKY display state
 * ---------------------------------------------------------------- */

/* DSKY digit positions (5 digits per register, 2 per verb/noun/prog) */
typedef struct {
    /* Status lights (1 = on, 0 = off) */
    int light_uplink_acty;
    int light_temp;
    int light_key_rel;
    int light_vel;
    int light_no_att;
    int light_alt;
    int light_gimbal_lock;
    int light_tracker;
    int light_prog_alarm;
    int light_stby;
    int light_restart;
    int light_opr_err;
    int light_comp_acty;

    /* Digit displays (each is 0-9 or -1 for blank) */
    int prog[2];    /* Program number digits */
    int verb[2];    /* Verb digits */
    int noun[2];    /* Noun digits */

    /* R1, R2, R3: sign + 5 digits each */
    int r1_sign;    /* 0=blank, 1=plus, -1=minus */
    int r1[5];
    int r2_sign;
    int r2[5];
    int r3_sign;
    int r3[5];
} dsky_display_t;

extern dsky_display_t dsky_display;

/* DSKY key codes (matching AGC channel 15 encoding) */
#define DSKY_KEY_0      020
#define DSKY_KEY_1      001
#define DSKY_KEY_2      002
#define DSKY_KEY_3      003
#define DSKY_KEY_4      004
#define DSKY_KEY_5      005
#define DSKY_KEY_6      006
#define DSKY_KEY_7      007
#define DSKY_KEY_8      010
#define DSKY_KEY_9      011
#define DSKY_KEY_VERB   021
#define DSKY_KEY_NOUN   037
#define DSKY_KEY_PLUS   032
#define DSKY_KEY_MINUS  033
#define DSKY_KEY_ENTR   034
#define DSKY_KEY_CLR    036
#define DSKY_KEY_PRO    -1   /* PRO handled separately */
#define DSKY_KEY_KREL   031
#define DSKY_KEY_RSET   022

/* ----------------------------------------------------------------
 * DSKY API
 * ---------------------------------------------------------------- */

/* Initialize DSKY display */
void dsky_init(void);

/* Update console display (called from main loop) */
void dsky_update(void);

/* Poll for keyboard input (called from main loop) */
void dsky_poll_input(void);

/* T4RUPT handler: scan DSKY display buffer */
void dsky_t4rupt(void);

/* Set COMP ACTY light */
void dsky_set_comp_acty(int on);

/* ----------------------------------------------------------------
 * Platform HAL for keyboard input
 * ---------------------------------------------------------------- */

/* Non-blocking keyboard check: returns non-zero if key available */
int hal_kbhit(void);

/* Get a character without echo or waiting for Enter */
int hal_getch(void);

/* Platform-specific terminal setup/cleanup */
void hal_term_init(void);
void hal_term_cleanup(void);

/* Sleep for milliseconds */
void hal_sleep_ms(int ms);

/* Console backend (ANSI terminal) */
extern dsky_backend_t dsky_console_backend;

#endif /* DSKY_H */
