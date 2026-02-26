/*
 * main.c -- Entry point, main loop, timing
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port
 *
 * Apollo 11 Command Module Guidance Computer
 * Colossus 2A / Comanche 055
 *
 * This is a faithful C89 port of the AGC flight software.
 * The DSKY is rendered as ASCII art in the console.
 * Type verb/noun commands using the keyboard mapping shown on screen.
 */

#include <stdio.h>
#include <stdlib.h>
#include "agc.h"
#include "agc_cpu.h"
#include "agc_math.h"
#include "executive.h"
#include "waitlist.h"
#include "timer.h"
#include "dsky.h"
#include "pinball.h"
#include "alarm.h"
#include "service.h"
#include "programs.h"
#include "navigation.h"

int main(void)
{
    printf("===========================================\n");
    printf("  COMANCHE 055 -- Colossus 2A\n");
    printf("  Apollo 11 CM Guidance Computer\n");
    printf("  ANSI C89 Port\n");
    printf("===========================================\n");
    printf("\nInitializing AGC...\n");

    /* Initialize platform terminal (raw mode on Linux/macOS) */
    hal_term_init();

    /* Initialize all subsystems */
    agc_init();
    exec_init();
    waitlist_init();
    timer_init();
    dsky_init();
    pinball_init();
    nav_init();

    /* Perform fresh start (DOFSTART) */
    fresh_start();

    /* Initialize navigation state */
    nav_init();

    printf("AGC ready. Entering P00 (CMC Idling).\n");
    printf("Press any key to start...\n");
    hal_getch();

    /* Force initial display */
    pinball_show_prog(0);
    pinball_show_verb(0);
    pinball_show_noun(0);

    /* ---- Main loop: 100 Hz (10ms per tick) ---- */
    while (1) {
        /* Advance timers, fire interrupts (T3RUPT, T4RUPT) */
        timer_tick();

        /* Run highest priority job (one quantum) */
        exec_run();

        /* Refresh console DSKY display */
        dsky_update();

        /* Check for keyboard input */
        dsky_poll_input();

        /* Sleep 10ms (~100 Hz cycle) */
        hal_sleep_ms(10);
    }

    /* Not reached, but for completeness */
    hal_term_cleanup();
    return 0;
}
