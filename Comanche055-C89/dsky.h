/*
 * dsky.h -- DSKY display abstraction and console backend.
 *
 * Defines the DSKY display state (lights, digits, registers),
 * key codes matching AGC channel 15 encoding, and the console
 * backend that renders the DSKY as ASCII art via ANSI terminal
 * escape codes.
 *
 * Maps to T4RUPT_PROGRAM.agc (display scan) and
 * KEYRUPT_UPRUPT.agc (keyboard input).
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

#ifndef DSKY_H
#define DSKY_H

#include "agc.h"
#include "dsky_backend.h"

/* ----------------------------------------------------------------
 * DSKY display state
 * ---------------------------------------------------------------- */

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

    /* Digit displays (0-9 or -1 for blank) */
    int prog[2];
    int verb[2];
    int noun[2];

    /* R1, R2, R3: sign + 5 digits each */
    int r1_sign;    /* 0=blank, 1=plus, -1=minus */
    int r1[5];
    int r2_sign;
    int r2[5];
    int r3_sign;
    int r3[5];
} dsky_display_t;

extern dsky_display_t dsky_display;

/* DSKY key codes (AGC channel 15 encoding) */
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
#define DSKY_KEY_PRO    -1
#define DSKY_KEY_KREL   031
#define DSKY_KEY_RSET   022

/* ----------------------------------------------------------------
 * DSKY API
 * ---------------------------------------------------------------- */

void dsky_init(void);
void dsky_update(void);
void dsky_poll_input(void);
void dsky_submit_key(int keycode);
void dsky_t4rupt(void);
void dsky_set_comp_acty(int on);

extern dsky_backend_t dsky_console_backend;

#endif /* DSKY_H */
