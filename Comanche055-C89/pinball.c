/*
 * pinball.c -- Verb/Noun processing, display interface, monitor verbs.
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

#include <string.h>
#include "pinball.h"
#include "agc_cpu.h"
#include "alarm.h"
#include "dsky.h"
#include "executive.h"
#include "navigation.h"
#include "programs.h"
#include "service.h"
#include "waitlist.h"

/* Static forward declarations for verb handlers */
static void verb_display_decimal(void);
static void verb_display_octal(void);
static void verb_monitor_decimal(void);
static void verb_load_component(void);
static void verb_lamp_test(void);
static void verb_fresh_start(void);
static void verb_change_program(void);
static void verb_orbit_display(void);

/* ----------------------------------------------------------------
 * State
 * ---------------------------------------------------------------- */

int pinball_verb = 0;
int pinball_noun = 0;
int pinball_inbuf[PINBALL_BUF_SIZE];
int pinball_incount = 0;
int pinball_mode = PINBALL_MODE_IDLE;
int pinball_data_reg = 0;
int pinball_monitor_active = 0;
int pinball_monitor_verb = 0;
int pinball_monitor_noun = 0;
int pinball_endidle = 0;

static int proceed_flag = 0;

/* ----------------------------------------------------------------
 * Init
 * ---------------------------------------------------------------- */

void pinball_init(void)
{
    pinball_verb = 0;
    pinball_noun = 0;
    pinball_incount = 0;
    pinball_mode = PINBALL_MODE_IDLE;
    pinball_data_reg = 0;
    pinball_monitor_active = 0;
    pinball_endidle = 0;
    proceed_flag = 0;
    memset(pinball_inbuf, 0, sizeof(pinball_inbuf));
}

/* ----------------------------------------------------------------
 * Display helpers
 * ---------------------------------------------------------------- */

void pinball_show_verb(int v)
{
    pinball_verb = v;
    dsky_display.verb[0] = v / 10;
    dsky_display.verb[1] = v % 10;
}

void pinball_show_noun(int n)
{
    pinball_noun = n;
    dsky_display.noun[0] = n / 10;
    dsky_display.noun[1] = n % 10;
}

void pinball_show_prog(int p)
{
    agc_current_program = p;
    dsky_display.prog[0] = p / 10;
    dsky_display.prog[1] = p % 10;
}

void pinball_display_val(int reg, int value, int is_signed)
{
    int *digits;
    int *sign;
    int absval, i;

    switch (reg) {
    case 1: digits = dsky_display.r1; sign = &dsky_display.r1_sign; break;
    case 2: digits = dsky_display.r2; sign = &dsky_display.r2_sign; break;
    case 3: digits = dsky_display.r3; sign = &dsky_display.r3_sign; break;
    default: return;
    }

    *sign = is_signed ? ((value >= 0) ? 1 : -1) : 0;

    absval = (value < 0) ? -value : value;
    for (i = 4; i >= 0; i--) {
        digits[i] = absval % 10;
        absval /= 10;
    }
}

void pinball_display_octal(int reg, int value)
{
    int *digits;
    int absval, i;

    switch (reg) {
    case 1: digits = dsky_display.r1; dsky_display.r1_sign = 0; break;
    case 2: digits = dsky_display.r2; dsky_display.r2_sign = 0; break;
    case 3: digits = dsky_display.r3; dsky_display.r3_sign = 0; break;
    default: return;
    }

    absval = (value < 0) ? -value : value;
    for (i = 4; i >= 0; i--) {
        digits[i] = absval & 7;
        absval >>= 3;
    }
}

static void blank_register(int reg)
{
    int *digits;
    int *sign;
    int i;

    switch (reg) {
    case 1: digits = dsky_display.r1; sign = &dsky_display.r1_sign; break;
    case 2: digits = dsky_display.r2; sign = &dsky_display.r2_sign; break;
    case 3: digits = dsky_display.r3; sign = &dsky_display.r3_sign; break;
    default: return;
    }
    *sign = 0;
    for (i = 0; i < 5; i++) digits[i] = -1;
}

/* ----------------------------------------------------------------
 * Input buffer
 * ---------------------------------------------------------------- */

static void clear_inbuf(void)
{
    pinball_incount = 0;
    memset(pinball_inbuf, 0, sizeof(pinball_inbuf));
}

static int inbuf_to_int(void)
{
    int val = 0;
    int i;
    for (i = 0; i < pinball_incount; i++)
        val = val * 10 + pinball_inbuf[i];
    return val;
}

/* ----------------------------------------------------------------
 * Noun data access
 * ---------------------------------------------------------------- */

static int noun_get_value(int noun, int component)
{
    switch (noun) {
    case 36: /* Mission elapsed time */
    {
        long total_cs = (long)agc_time2 * 16384L + (long)agc_time1;
        long total_secs = total_cs / 100;
        switch (component) {
        case 1: return (int)(total_secs / 3600);
        case 2: return (int)((total_secs % 3600) / 60);
        case 3: return (int)(total_secs % 60);
        }
    }
    break;

    case 1: /* Specified address value */
        return 0;

    case 9: /* Alarm codes */
        if (component == 1) return agc_alarm_code;
        return 0;

    case 43: /* Latitude, longitude, altitude (stub) */
        switch (component) {
        case 1: return 28553;   /* ~28.553 N (KSC) */
        case 2: return -80649;  /* ~-80.649 W (KSC) */
        case 3: return 0;
        }
        break;

    case 44: /* Apogee, perigee, TFF (filled by R30) */
        switch (component) {
        case 1: return agc_read_erasable(5, 0);
        case 2: return agc_read_erasable(5, 1);
        case 3: return agc_read_erasable(5, 2);
        }
        break;

    default:
        break;
    }
    return 0;
}

/* ----------------------------------------------------------------
 * Verb dispatch
 * ---------------------------------------------------------------- */

static void dispatch_verb(void)
{
    switch (pinball_verb) {
    case 1:
    case 4:
        verb_display_octal();
        break;
    case 5:
    case 6:
        verb_display_decimal();
        break;
    case 16:
        verb_monitor_decimal();
        break;
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
        verb_load_component();
        break;
    case 35:
        verb_lamp_test();
        break;
    case 36:
        verb_fresh_start();
        break;
    case 37:
        verb_change_program();
        break;
    case 82:
        verb_orbit_display();
        break;
    default:
        agc_channels[CHAN_DSALMOUT] |= BIT12;
        dsky_display.light_opr_err = 1;
        break;
    }
}

/* ----------------------------------------------------------------
 * Verb implementations
 * ---------------------------------------------------------------- */

static void verb_display_octal(void)
{
    int n = pinball_noun;
    if (pinball_verb >= 1)
        pinball_display_octal(1, noun_get_value(n, 1));
    if (pinball_verb >= 4)
        pinball_display_octal(2, noun_get_value(n, 2));
}

static void verb_display_decimal(void)
{
    int n = pinball_noun;
    if (pinball_verb == 5 || pinball_verb == 6) {
        pinball_display_val(1, noun_get_value(n, 1), 1);
        pinball_display_val(2, noun_get_value(n, 2), 1);
    }
    if (pinball_verb == 6)
        pinball_display_val(3, noun_get_value(n, 3), 1);
}

static void monitor_task(void)
{
    if (pinball_monitor_active) {
        int n = pinball_monitor_noun;
        pinball_display_val(1, noun_get_value(n, 1), 1);
        pinball_display_val(2, noun_get_value(n, 2), 1);
        pinball_display_val(3, noun_get_value(n, 3), 1);
        waitlist_add(ONE_SEC, monitor_task);
    }
}

static void verb_monitor_decimal(void)
{
    pinball_monitor_active = 1;
    pinball_monitor_verb = pinball_verb;
    pinball_monitor_noun = pinball_noun;

    pinball_display_val(1, noun_get_value(pinball_noun, 1), 1);
    pinball_display_val(2, noun_get_value(pinball_noun, 2), 1);
    pinball_display_val(3, noun_get_value(pinball_noun, 3), 1);

    waitlist_add(ONE_SEC, monitor_task);
}

static void verb_load_component(void)
{
    /* V21->R1, V22->R2, V23->R3 */
    pinball_mode = PINBALL_MODE_DATA;
    pinball_data_reg = pinball_verb - 20;
    if (pinball_data_reg < 1) pinball_data_reg = 1;
    if (pinball_data_reg > 3) pinball_data_reg = 3;
    clear_inbuf();
    blank_register(pinball_data_reg);
}

static void verb_lamp_test(void)
{
    int i;
    agc_word_t all_lights = 0x7FFF;
    agc_channels[CHAN_DSALMOUT] = all_lights;

    dsky_display.light_uplink_acty = 1;
    dsky_display.light_temp = 1;
    dsky_display.light_key_rel = 1;
    dsky_display.light_vel = 1;
    dsky_display.light_no_att = 1;
    dsky_display.light_alt = 1;
    dsky_display.light_gimbal_lock = 1;
    dsky_display.light_tracker = 1;
    dsky_display.light_prog_alarm = 1;
    dsky_display.light_stby = 1;
    dsky_display.light_restart = 1;
    dsky_display.light_opr_err = 1;
    dsky_display.light_comp_acty = 1;

    dsky_display.prog[0] = 8; dsky_display.prog[1] = 8;
    dsky_display.verb[0] = 8; dsky_display.verb[1] = 8;
    dsky_display.noun[0] = 8; dsky_display.noun[1] = 8;

    for (i = 0; i < 5; i++) {
        dsky_display.r1[i] = 8;
        dsky_display.r2[i] = 8;
        dsky_display.r3[i] = 8;
    }
    dsky_display.r1_sign = 1;
    dsky_display.r2_sign = 1;
    dsky_display.r3_sign = 1;
}

static void verb_fresh_start(void)
{
    fresh_start();
}

static void verb_change_program(void)
{
    pinball_mode = PINBALL_MODE_DATA;
    pinball_data_reg = 0;
    clear_inbuf();
    blank_register(1);
    blank_register(2);
    blank_register(3);
}

static void verb_orbit_display(void)
{
    program_r30_v82();
}

/* ----------------------------------------------------------------
 * NVSUB: internal verb-noun call
 * ---------------------------------------------------------------- */

int pinball_nvsub(int verb, int noun)
{
    pinball_show_verb(verb);
    pinball_show_noun(noun);
    pinball_verb = verb;
    pinball_noun = noun;
    dispatch_verb();
    return 0;
}

/* ----------------------------------------------------------------
 * ENDIDLE: wait for operator
 * ---------------------------------------------------------------- */

int pinball_wait_endidle(void)
{
    pinball_endidle = 1;
    proceed_flag = 0;
    return proceed_flag;
}

/* ----------------------------------------------------------------
 * Keypress handler (CHARIN equivalent)
 * ---------------------------------------------------------------- */

void pinball_keypress(int keycode)
{
    agc_channels[CHAN_DSALMOUT] &= ~BIT12;
    dsky_display.light_opr_err = 0;

    if (keycode == DSKY_KEY_RSET) {
        alarm_reset();
        pinball_monitor_active = 0;
        dsky_display.light_opr_err = 0;
        dsky_display.light_restart = 0;
        return;
    }

    if (keycode == DSKY_KEY_KREL) {
        agc_channels[CHAN_DSALMOUT] &= ~BIT5;
        dsky_display.light_key_rel = 0;
        return;
    }

    if (keycode == DSKY_KEY_CLR) {
        clear_inbuf();
        if (pinball_mode == PINBALL_MODE_DATA)
            blank_register(pinball_data_reg ? pinball_data_reg : 1);
        return;
    }

    /* PRO (proceed) */
    if (keycode == -1) {
        if (pinball_endidle) {
            proceed_flag = 1;
            pinball_endidle = 0;
        }
        return;
    }

    if (keycode == DSKY_KEY_VERB) {
        pinball_mode = PINBALL_MODE_VERB;
        clear_inbuf();
        dsky_display.verb[0] = -1;
        dsky_display.verb[1] = -1;
        return;
    }

    if (keycode == DSKY_KEY_NOUN) {
        pinball_mode = PINBALL_MODE_NOUN;
        clear_inbuf();
        dsky_display.noun[0] = -1;
        dsky_display.noun[1] = -1;
        return;
    }

    if (keycode == DSKY_KEY_ENTR) {
        if (pinball_mode == PINBALL_MODE_VERB) {
            pinball_show_verb(inbuf_to_int());
            pinball_mode = PINBALL_MODE_IDLE;
            clear_inbuf();
            dispatch_verb();
        } else if (pinball_mode == PINBALL_MODE_NOUN) {
            pinball_show_noun(inbuf_to_int());
            pinball_mode = PINBALL_MODE_IDLE;
            clear_inbuf();
            dispatch_verb();
        } else if (pinball_mode == PINBALL_MODE_DATA) {
            int val = inbuf_to_int();
            if (pinball_data_reg == 0) {
                /* V37: program number */
                program_change(val);
                pinball_mode = PINBALL_MODE_IDLE;
            } else {
                int sign = 0;
                switch (pinball_data_reg) {
                case 1: sign = dsky_display.r1_sign; break;
                case 2: sign = dsky_display.r2_sign; break;
                case 3: sign = dsky_display.r3_sign; break;
                }
                if (sign < 0) val = -val;
                pinball_display_val(pinball_data_reg, val, 1);
                /* V24/V25: advance to next register */
                if (pinball_verb == 24 && pinball_data_reg == 1) {
                    pinball_data_reg = 2;
                    clear_inbuf();
                    blank_register(2);
                    return;
                }
                if (pinball_verb == 25) {
                    if (pinball_data_reg < 3) {
                        pinball_data_reg++;
                        clear_inbuf();
                        blank_register(pinball_data_reg);
                        return;
                    }
                }
                pinball_mode = PINBALL_MODE_IDLE;
            }
            clear_inbuf();
        } else if (pinball_endidle) {
            proceed_flag = 0;
            pinball_endidle = 0;
        } else {
            dispatch_verb();
        }
        return;
    }

    /* Sign keys in data mode */
    if (keycode == DSKY_KEY_PLUS || keycode == DSKY_KEY_MINUS) {
        if (pinball_mode == PINBALL_MODE_DATA && pinball_data_reg > 0) {
            int s = (keycode == DSKY_KEY_PLUS) ? 1 : -1;
            switch (pinball_data_reg) {
            case 1: dsky_display.r1_sign = s; break;
            case 2: dsky_display.r2_sign = s; break;
            case 3: dsky_display.r3_sign = s; break;
            }
            clear_inbuf();
        }
        return;
    }

    /* Digit keys (0-9) */
    if ((keycode >= 0 && keycode <= 011) || keycode == DSKY_KEY_0) {
        int digit;
        if (keycode == DSKY_KEY_0)
            digit = 0;
        else
            digit = keycode;

        if (pinball_mode == PINBALL_MODE_VERB) {
            if (pinball_incount < 2) {
                pinball_inbuf[pinball_incount++] = digit;
                if (pinball_incount == 1)
                    dsky_display.verb[0] = digit;
                else
                    dsky_display.verb[1] = digit;
            }
        } else if (pinball_mode == PINBALL_MODE_NOUN) {
            if (pinball_incount < 2) {
                pinball_inbuf[pinball_incount++] = digit;
                if (pinball_incount == 1)
                    dsky_display.noun[0] = digit;
                else
                    dsky_display.noun[1] = digit;
            }
        } else if (pinball_mode == PINBALL_MODE_DATA) {
            if (pinball_incount < 5) {
                int *digits = NULL;
                pinball_inbuf[pinball_incount] = digit;
                switch (pinball_data_reg) {
                case 0: digits = dsky_display.r1; break;
                case 1: digits = dsky_display.r1; break;
                case 2: digits = dsky_display.r2; break;
                case 3: digits = dsky_display.r3; break;
                }
                if (digits != NULL)
                    digits[pinball_incount] = digit;
                pinball_incount++;
            }
        }
    }
}
