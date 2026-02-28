/*
 * dsky.c -- Console DSKY HAL: ASCII rendering + keyboard input
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port
 * Renders the DSKY as ASCII art, maps keyboard to AGC key codes.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dsky.h"
#include "terminal.h"
#include "agc_cpu.h"
#include "alarm.h"

/* ----------------------------------------------------------------
 * Platform HAL: non-blocking keyboard input
 * ---------------------------------------------------------------- */

#ifdef _WIN32

#include <conio.h>
#include <windows.h>

int hal_kbhit(void) { return _kbhit(); }
int hal_getch(void) { return _getch(); }
void hal_term_init(void) { }
void hal_term_cleanup(void) { }
void hal_sleep_ms(int ms) { Sleep(ms); }

#else

#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/select.h>
#include <time.h>

static struct termios orig_termios;
static int term_raw = 0;

void hal_term_init(void)
{
    struct termios raw;
    if (!term_raw) {
        tcgetattr(STDIN_FILENO, &orig_termios);
        raw = orig_termios;
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
        term_raw = 1;
    }
}

void hal_term_cleanup(void)
{
    if (term_raw) {
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
        term_raw = 0;
    }
}

int hal_kbhit(void)
{
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}

int hal_getch(void)
{
    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) == 1) return (int)c;
    return -1;
}

void hal_sleep_ms(int ms)
{
    struct timeval tv;
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
    select(0, NULL, NULL, NULL, &tv);
}

#endif

/* ----------------------------------------------------------------
 * DSKY display state
 * ---------------------------------------------------------------- */

dsky_display_t dsky_display;

/* Previous display state for dirty-checking (avoid flicker) */
static dsky_display_t dsky_prev;
static int dsky_needs_redraw = 1;

/* Forward declaration */
extern void pinball_keypress(int keycode);

/* ----------------------------------------------------------------
 * Init
 * ---------------------------------------------------------------- */

void dsky_init(void)
{
    int i;
    memset(&dsky_display, 0, sizeof(dsky_display));
    memset(&dsky_prev, 0, sizeof(dsky_prev));

    /* Blank all digits */
    dsky_display.prog[0] = -1; dsky_display.prog[1] = -1;
    dsky_display.verb[0] = -1; dsky_display.verb[1] = -1;
    dsky_display.noun[0] = -1; dsky_display.noun[1] = -1;
    for (i = 0; i < 5; i++) {
        dsky_display.r1[i] = -1;
        dsky_display.r2[i] = -1;
        dsky_display.r3[i] = -1;
    }
    dsky_display.r1_sign = 0;
    dsky_display.r2_sign = 0;
    dsky_display.r3_sign = 0;

    dsky_needs_redraw = 1;
}

/* ----------------------------------------------------------------
 * Render helpers
 * ---------------------------------------------------------------- */

static char digit_char(int d)
{
    if (d < 0 || d > 9) return ' ';
    return (char)('0' + d);
}

static const char *sign_str(int s)
{
    if (s > 0) return "+";
    if (s < 0) return "-";
    return " ";
}

static void light_str(char *buf, int on, const char *name)
{
    if (on) {
        int i, len;
        buf[0] = '[';
        len = 0;
        while (name[len] && len < 7) {
            buf[1 + len] = name[len];
            len++;
        }
        for (i = len; i < 7; i++) buf[1 + i] = ' ';
        buf[8] = ']';
        buf[9] = '\0';
    } else {
        memcpy(buf, "         ", 10);
    }
}

/* ----------------------------------------------------------------
 * Render DSKY to console
 * ---------------------------------------------------------------- */

static void dsky_render(void)
{
    static int first_render = 1;
    
    if (first_render) {
        /* First render: clear screen and draw static layout */
        term_set_cursor(1, 1);
        printf("+------------- DSKY -------------+\n");
        printf("|                                |\n");
        first_render = 0;
    }
    
    /* Update status lights row 1 */
    {
        char a[12], b[12], c[12];
        light_str(a, dsky_display.light_uplink_acty, "UPLINK");
        light_str(b, dsky_display.light_temp, "TEMP");
        light_str(c, dsky_display.light_prog_alarm, "PROG");
        term_set_cursor(3, 2);
        printf("| %s %s %s  |\n", a, b, c);
    }

    /* Update status lights row 2 */
    {
        char a[12], b[12], c[12];
        light_str(a, dsky_display.light_gimbal_lock, "GIMBAL");
        light_str(b, dsky_display.light_stby, "STBY");
        light_str(c, dsky_display.light_restart, "RSTART");
        term_set_cursor(4, 2);
        printf("| %s %s %s  |\n", a, b, c);
    }

    /* Update status lights row 3 */
    {
        char a[12], b[12], c[12];
        light_str(a, dsky_display.light_no_att, "NO ATT");
        light_str(b, dsky_display.light_key_rel, "KEY RL");
        light_str(c, dsky_display.light_tracker, "TRACKER");
        term_set_cursor(5, 2);
        printf("| %s %s %s  |\n", a, b, c);
    }

    /* Update status lights row 4 */
    {
        char a[12], b[12], c[12];
        light_str(a, dsky_display.light_opr_err, "OPR ER");
        light_str(b, dsky_display.light_vel, "VEL");
        light_str(c, dsky_display.light_alt, "ALT");
        term_set_cursor(6, 2);
        printf("| %s %s %s  |\n", a, b, c);
    }

    printf("|                                |\n");

    /* COMP ACTY and PROG display */
    term_set_cursor(7, 2);
    printf("|  %s   PROG  %c%c          |\n",
           dsky_display.light_comp_acty ? "COMP ACTY" : "         ",
           digit_char(dsky_display.prog[0]),
           digit_char(dsky_display.prog[1]));

    /* VERB and NOUN */
    term_set_cursor(8, 2);
    printf("|  VERB  %c%c    NOUN  %c%c          |\n",
           digit_char(dsky_display.verb[0]),
           digit_char(dsky_display.verb[1]),
           digit_char(dsky_display.noun[0]),
           digit_char(dsky_display.noun[1]));

    printf("|                                |\n");

    /* R1 */
    term_set_cursor(10, 2);
    printf("|  R1   %s%c%c%c%c%c                   |\n",
           sign_str(dsky_display.r1_sign),
           digit_char(dsky_display.r1[0]),
           digit_char(dsky_display.r1[1]),
           digit_char(dsky_display.r1[2]),
           digit_char(dsky_display.r1[3]),
           digit_char(dsky_display.r1[4]));

    /* R2 */
    term_set_cursor(11, 2);
    printf("|  R2   %s%c%c%c%c%c                   |\n",
           sign_str(dsky_display.r2_sign),
           digit_char(dsky_display.r2[0]),
           digit_char(dsky_display.r2[1]),
           digit_char(dsky_display.r2[2]),
           digit_char(dsky_display.r2[3]),
           digit_char(dsky_display.r2[4]));

    /* R3 */
    term_set_cursor(12, 2);
    printf("|  R3   %s%c%c%c%c%c                   |\n",
           sign_str(dsky_display.r3_sign),
           digit_char(dsky_display.r3[0]),
           digit_char(dsky_display.r3[1]),
           digit_char(dsky_display.r3[2]),
           digit_char(dsky_display.r3[3]),
           digit_char(dsky_display.r3[4]));

    printf("|                                |\n");
    printf("| Keys: V=VERB N=NOUN E=ENTR     |\n");
    printf("| 0-9=digits +=PLUS -=MINUS      |\n");
    printf("| C=CLR  P=PRO  K=KREL  R=RSET   |\n");
    printf("| Q=QUIT                         |\n");
    printf("+--------------------------------+\n");

    fflush(stdout);
}

/* ----------------------------------------------------------------
 * Update display (called from main loop)
 * ---------------------------------------------------------------- */

void dsky_update(void)
{
    /* Only redraw if display state changed */
    if (dsky_needs_redraw ||
        memcmp(&dsky_display, &dsky_prev, sizeof(dsky_display)) != 0) {
        dsky_render();
        dsky_prev = dsky_display;
        dsky_needs_redraw = 0;
    }
}

/* ----------------------------------------------------------------
 * T4RUPT handler: scan display buffer
 * ----------------------------------------------------------------
 * In the real AGC, T4RUPT cycles through relay words to update
 * the DSKY's electroluminescent display. Here we update our
 * display struct from Pinball's state.
 */

void dsky_t4rupt(void)
{
    /* Update status lights from channel 11 */
    agc_word_t ch11 = agc_channels[CHAN_DSALMOUT];
    dsky_display.light_comp_acty    = (ch11 & BIT1) ? 1 : 0;
    dsky_display.light_uplink_acty  = (ch11 & BIT2) ? 1 : 0;
    dsky_display.light_temp         = (ch11 & BIT4) ? 1 : 0;
    dsky_display.light_key_rel      = (ch11 & BIT5) ? 1 : 0;
    dsky_display.light_vel          = (ch11 & BIT6) ? 1 : 0;
    dsky_display.light_no_att       = (ch11 & BIT7) ? 1 : 0;
    dsky_display.light_alt          = (ch11 & BIT8) ? 1 : 0;
    dsky_display.light_gimbal_lock  = (ch11 & BIT9) ? 1 : 0;
    dsky_display.light_tracker      = (ch11 & BIT10) ? 1 : 0;
    dsky_display.light_prog_alarm   = (ch11 & BIT11) ? 1 : 0;
    dsky_display.light_stby         = (ch11 & BIT13) ? 1 : 0;
    dsky_display.light_restart      = (ch11 & BIT14) ? 1 : 0;
    dsky_display.light_opr_err      = (ch11 & BIT12) ? 1 : 0;
}

/* ----------------------------------------------------------------
 * COMP ACTY light control
 * ---------------------------------------------------------------- */

void dsky_set_comp_acty(int on)
{
    if (on) {
        agc_channels[CHAN_DSALMOUT] |= BIT1;
    } else {
        agc_channels[CHAN_DSALMOUT] &= ~BIT1;
    }
    dsky_display.light_comp_acty = on;
}

/* ----------------------------------------------------------------
 * Shared key submission helper
 * ----------------------------------------------------------------
 */

void dsky_submit_key(int keycode)
{
    if (keycode == DSKY_KEY_PRO) {
        pinball_keypress(-1);
        return;
    }

    if (keycode >= 0) {
        /* Write key code to channel 15 and raise KEYRUPT */
        agc_channels[CHAN_MNKEYIN] = (agc_word_t)keycode;
        pinball_keypress(keycode);
    }
}

/* ----------------------------------------------------------------
 * Poll keyboard input
 * ----------------------------------------------------------------
 * Maps console keys to AGC DSKY key codes and forwards to Pinball.
 */

void dsky_poll_input(void)
{
    int ch, keycode;
    if (!hal_kbhit()) return;

    ch = hal_getch();
    keycode = -2;

    switch (ch) {
        case '0': keycode = DSKY_KEY_0; break;
        case '1': keycode = DSKY_KEY_1; break;
        case '2': keycode = DSKY_KEY_2; break;
        case '3': keycode = DSKY_KEY_3; break;
        case '4': keycode = DSKY_KEY_4; break;
        case '5': keycode = DSKY_KEY_5; break;
        case '6': keycode = DSKY_KEY_6; break;
        case '7': keycode = DSKY_KEY_7; break;
        case '8': keycode = DSKY_KEY_8; break;
        case '9': keycode = DSKY_KEY_9; break;
        case 'v': case 'V': keycode = DSKY_KEY_VERB; break;
        case 'n': case 'N': keycode = DSKY_KEY_NOUN; break;
        case '+': case '=': keycode = DSKY_KEY_PLUS; break;
        case '-': case '_': keycode = DSKY_KEY_MINUS; break;
        case 'e': case 'E': case '\r': case '\n':
            keycode = DSKY_KEY_ENTR; break;
        case 'c': case 'C': keycode = DSKY_KEY_CLR; break;
        case 'p': case 'P': keycode = DSKY_KEY_PRO; break;
        case 'k': case 'K': keycode = DSKY_KEY_KREL; break;
        case 'r': case 'R': keycode = DSKY_KEY_RSET; break;
        case 'q': case 'Q':
            hal_term_cleanup();
            printf("\nComanche055 terminated.\n");
            exit(0);
            break;
        default:
            break;
    }

    if (keycode != -2) {
        dsky_submit_key(keycode);
    }
}

/* ----------------------------------------------------------------
 * Console backend struct
 * ---------------------------------------------------------------- */

static void console_be_init(void)    
{ 
    hal_term_init(); 
    term_init();  /* Initialize shared terminal system */
}

static void console_be_cleanup(void) 
{ 
    term_cleanup();  /* Cleanup shared terminal system */
    hal_term_cleanup(); 
}

dsky_backend_t dsky_console_backend = {
    console_be_init,
    dsky_update,
    dsky_poll_input,
    console_be_cleanup,
    hal_sleep_ms
};
