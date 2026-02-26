/*
 * pinball.c -- Verb/Noun processing, display interface, monitor verbs
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port
 * The "Pinball Game" -- DSKY software driver
 *
 * Handles all keyboard input processing, verb/noun dispatch,
 * display formatting, monitor verbs, and data entry.
 */

#include <string.h>
#include "pinball.h"
#include "agc_cpu.h"
#include "alarm.h"
#include "executive.h"
#include "waitlist.h"

/* Forward declarations for verb handlers */
static void verb_display_decimal(void);
static void verb_display_octal(void);
static void verb_monitor_decimal(void);
static void verb_load_component(void);
static void verb_lamp_test(void);
static void verb_fresh_start(void);
static void verb_change_program(void);
static void verb_orbit_display(void);

/* Forward declarations from other modules */
extern void program_change(int prognum);
extern void program_r30_v82(void);
extern void fresh_start(void);

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

/* Internal proceed flag for ENDIDLE */
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

/* Display a 5-digit signed decimal value in register 1, 2, or 3 */
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

    if (is_signed) {
        *sign = (value >= 0) ? 1 : -1;
    } else {
        *sign = 0;
    }

    absval = (value < 0) ? -value : value;
    for (i = 4; i >= 0; i--) {
        digits[i] = absval % 10;
        absval /= 10;
    }
}

/* Display octal value in register */
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

/* Blank a register display */
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
 * Input buffer management
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
    for (i = 0; i < pinball_incount; i++) {
        val = val * 10 + pinball_inbuf[i];
    }
    return val;
}

/* ----------------------------------------------------------------
 * Noun data access
 * ----------------------------------------------------------------
 * Maps noun numbers to data sources for display verbs.
 */

/* Get the value for a noun/component combination.
 * component: 1=R1, 2=R2, 3=R3
 * Returns the value to display. */
static int noun_get_value(int noun, int component)
{
    switch (noun) {
        case 36: /* Mission elapsed time (hours, minutes, seconds) */
        {
            /* TIME1 counts centiseconds, TIME2 counts TIME1 overflows */
            long total_cs = (long)agc_time2 * 16384L + (long)agc_time1;
            long total_secs = total_cs / 100;
            switch (component) {
                case 1: return (int)(total_secs / 3600);       /* hours */
                case 2: return (int)((total_secs % 3600) / 60); /* minutes */
                case 3: return (int)(total_secs % 60);          /* seconds */
            }
        }
        break;

        case 1: /* Specified address value (machine address in R1) */
            /* For V21N01, R1 holds the address, R2/R3 show value */
            if (component == 1) return 0;
            return 0;

        case 9: /* Alarm codes */
            switch (component) {
                case 1: return agc_alarm_code;
                case 2: return 0;
                case 3: return 0;
            }
            break;

        case 43: /* Latitude, longitude, altitude (stub) */
            switch (component) {
                case 1: return 28553;  /* ~28.553 N (KSC latitude * 1000) */
                case 2: return -80649; /* ~-80.649 W (KSC longitude * 1000) */
                case 3: return 0;      /* Altitude */
            }
            break;

        case 44: /* Apogee, perigee, TFF (orbit params, set by R30) */
            /* These are filled in by navigation.c R30 routine */
            switch (component) {
                case 1: return agc_read_erasable(5, 0);  /* Apogee NM */
                case 2: return agc_read_erasable(5, 1);  /* Perigee NM */
                case 3: return agc_read_erasable(5, 2);  /* TFF minutes */
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
        case 1:  /* Display octal, component 1 in R1 */
        case 4:  /* Display octal, components 1,2 in R1,R2 */
            verb_display_octal();
            break;

        case 5:  /* Display decimal, component 1,2 in R1,R2 */
            verb_display_decimal();
            break;

        case 6:  /* Display decimal, all 3 components */
            verb_display_decimal();
            break;

        case 16: /* Monitor decimal: display and update periodically */
            verb_monitor_decimal();
            break;

        case 21: /* Load component 1 */
        case 22: /* Load component 2 */
        case 23: /* Load component 3 */
        case 24: /* Load component 1,2 */
        case 25: /* Load component 1,2,3 */
            verb_load_component();
            break;

        case 35: /* Lamp test */
            verb_lamp_test();
            break;

        case 36: /* Fresh start */
            verb_fresh_start();
            break;

        case 37: /* Change program */
            verb_change_program();
            break;

        case 82: /* Orbit parameter display (R30) */
            verb_orbit_display();
            break;

        default:
            /* Unknown verb: flash OPR ERR */
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
    if (pinball_verb >= 1) {
        pinball_display_octal(1, noun_get_value(n, 1));
    }
    if (pinball_verb >= 4) {
        pinball_display_octal(2, noun_get_value(n, 2));
    }
}

static void verb_display_decimal(void)
{
    int n = pinball_noun;
    if (pinball_verb == 5 || pinball_verb == 6) {
        pinball_display_val(1, noun_get_value(n, 1), 1);
        pinball_display_val(2, noun_get_value(n, 2), 1);
    }
    if (pinball_verb == 6) {
        pinball_display_val(3, noun_get_value(n, 3), 1);
    }
}

/* Monitor task: re-display every second */
static void monitor_task(void)
{
    if (pinball_monitor_active) {
        int n = pinball_monitor_noun;
        pinball_display_val(1, noun_get_value(n, 1), 1);
        pinball_display_val(2, noun_get_value(n, 2), 1);
        pinball_display_val(3, noun_get_value(n, 3), 1);
        /* Reschedule */
        waitlist_add(ONE_SEC, monitor_task);
    }
}

static void verb_monitor_decimal(void)
{
    /* V16: start monitoring noun display, update every second */
    pinball_monitor_active = 1;
    pinball_monitor_verb = pinball_verb;
    pinball_monitor_noun = pinball_noun;

    /* Display immediately */
    pinball_display_val(1, noun_get_value(pinball_noun, 1), 1);
    pinball_display_val(2, noun_get_value(pinball_noun, 2), 1);
    pinball_display_val(3, noun_get_value(pinball_noun, 3), 1);

    /* Schedule periodic update */
    waitlist_add(ONE_SEC, monitor_task);
}

static void verb_load_component(void)
{
    /* V21-V25: enter data load mode.
     * The astronaut enters digits, then ENTR to load.
     * For now, we enter data mode for the first component. */
    pinball_mode = PINBALL_MODE_DATA;
    pinball_data_reg = pinball_verb - 20;  /* V21->R1, V22->R2, V23->R3 */
    if (pinball_data_reg < 1) pinball_data_reg = 1;
    if (pinball_data_reg > 3) pinball_data_reg = 3;
    clear_inbuf();
    /* Blank the target register to show we're waiting for input */
    blank_register(pinball_data_reg);
}

static void verb_lamp_test(void)
{
    /* V35: turn on all lights, all digits show 8 */
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
    /* V36: fresh start request - handled by service module */
    fresh_start();
}

static void verb_change_program(void)
{
    /* V37: enter program number.
     * Switch to data entry mode, wait for 2-digit program number. */
    pinball_mode = PINBALL_MODE_DATA;
    pinball_data_reg = 0;  /* Special: program number entry */
    clear_inbuf();
    /* Blank R1 to show we're waiting */
    blank_register(1);
    blank_register(2);
    blank_register(3);
}

static void verb_orbit_display(void)
{
    /* V82: request orbit parameter display (R30) */
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
    /* In our cooperative model, the job should call this and then
     * endofjob. The result will be available on next dispatch.
     * For simplicity, we return immediately with the current state. */
    return proceed_flag;
}

/* ----------------------------------------------------------------
 * Keypress handler (CHARIN equivalent)
 * ---------------------------------------------------------------- */

void pinball_keypress(int keycode)
{
    /* Clear OPR ERR on any keypress */
    agc_channels[CHAN_DSALMOUT] &= ~BIT12;
    dsky_display.light_opr_err = 0;

    /* Handle RSET key */
    if (keycode == DSKY_KEY_RSET) {
        alarm_reset();
        pinball_monitor_active = 0;
        dsky_display.light_opr_err = 0;
        dsky_display.light_restart = 0;
        return;
    }

    /* Handle KEY REL */
    if (keycode == DSKY_KEY_KREL) {
        agc_channels[CHAN_DSALMOUT] &= ~BIT5;
        dsky_display.light_key_rel = 0;
        return;
    }

    /* Handle CLR */
    if (keycode == DSKY_KEY_CLR) {
        clear_inbuf();
        if (pinball_mode == PINBALL_MODE_DATA) {
            blank_register(pinball_data_reg ? pinball_data_reg : 1);
        }
        return;
    }

    /* Handle PRO (proceed) */
    if (keycode == -1) {
        if (pinball_endidle) {
            proceed_flag = 1;
            pinball_endidle = 0;
        }
        return;
    }

    /* VERB key: switch to verb entry mode */
    if (keycode == DSKY_KEY_VERB) {
        pinball_mode = PINBALL_MODE_VERB;
        clear_inbuf();
        dsky_display.verb[0] = -1;
        dsky_display.verb[1] = -1;
        return;
    }

    /* NOUN key: switch to noun entry mode */
    if (keycode == DSKY_KEY_NOUN) {
        pinball_mode = PINBALL_MODE_NOUN;
        clear_inbuf();
        dsky_display.noun[0] = -1;
        dsky_display.noun[1] = -1;
        return;
    }

    /* ENTR key: execute current verb-noun or confirm data entry */
    if (keycode == DSKY_KEY_ENTR) {
        if (pinball_mode == PINBALL_MODE_VERB) {
            pinball_show_verb(inbuf_to_int());
            pinball_mode = PINBALL_MODE_IDLE;
            clear_inbuf();
            /* If noun is already set, dispatch */
            dispatch_verb();
        } else if (pinball_mode == PINBALL_MODE_NOUN) {
            pinball_show_noun(inbuf_to_int());
            pinball_mode = PINBALL_MODE_IDLE;
            clear_inbuf();
            /* Dispatch with current verb */
            dispatch_verb();
        } else if (pinball_mode == PINBALL_MODE_DATA) {
            /* Data entry complete */
            int val = inbuf_to_int();
            if (pinball_data_reg == 0) {
                /* V37: program number entry */
                program_change(val);
                pinball_mode = PINBALL_MODE_IDLE;
            } else {
                /* V21-V25: store loaded value */
                pinball_display_val(pinball_data_reg, val, 1);
                /* For V24/V25, advance to next register */
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
            /* ENTR pressed during ENDIDLE */
            proceed_flag = 0;
            pinball_endidle = 0;
        } else {
            /* Re-execute current verb-noun */
            dispatch_verb();
        }
        return;
    }

    /* Handle +/- sign keys in data mode */
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
        /* Convert AGC key code to digit value */
        if (keycode == DSKY_KEY_0) {
            digit = 0;
        } else {
            digit = keycode;  /* KEY_1=1, KEY_2=2, ..., KEY_9=9 */
        }

        if (pinball_mode == PINBALL_MODE_VERB) {
            if (pinball_incount < 2) {
                pinball_inbuf[pinball_incount++] = digit;
                /* Update display as digits are entered */
                if (pinball_incount == 1) {
                    dsky_display.verb[0] = digit;
                } else {
                    dsky_display.verb[1] = digit;
                }
            }
        } else if (pinball_mode == PINBALL_MODE_NOUN) {
            if (pinball_incount < 2) {
                pinball_inbuf[pinball_incount++] = digit;
                if (pinball_incount == 1) {
                    dsky_display.noun[0] = digit;
                } else {
                    dsky_display.noun[1] = digit;
                }
            }
        } else if (pinball_mode == PINBALL_MODE_DATA) {
            if (pinball_incount < 5) {
                int *digits = NULL;
                pinball_inbuf[pinball_incount] = digit;
                /* Show digit in the appropriate register */
                switch (pinball_data_reg) {
                    case 0: /* V37 program number: show in R1 */
                        digits = dsky_display.r1;
                        break;
                    case 1: digits = dsky_display.r1; break;
                    case 2: digits = dsky_display.r2; break;
                    case 3: digits = dsky_display.r3; break;
                }
                if (digits != NULL) {
                    digits[pinball_incount] = digit;
                }
                pinball_incount++;
            }
        }
    }
}

/* ----------------------------------------------------------------
 * Monitor update (called by waitlist)
 * ---------------------------------------------------------------- */

void pinball_monitor_update(void)
{
    if (pinball_monitor_active) {
        monitor_task();
    }
}
