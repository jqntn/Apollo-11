/*
 * main.c -- Entry point, main loop, timing
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port
 *
 * Apollo 11 Command Module Guidance Computer
 * Colossus 2A / Comanche 055
 *
 * This is a faithful C89 port of the AGC flight software.
 * Supports both an ANSI console backend and a Win32 graphical backend.
 * The user selects the display mode at startup.
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
#ifdef _WIN32
#include "dsky_gui.h"
#include "dsky_web.h"
#endif

int main(void)
{
    dsky_backend_t *backend;
    int choice;

    printf("===========================================\n");
    printf("  COMANCHE 055 -- Colossus 2A\n");
    printf("  Apollo 11 CM Guidance Computer\n");
    printf("  ANSI C89 Port\n");
    printf("===========================================\n");

    /* Select display backend */
    printf("\nSelect display mode:\n");
#ifdef _WIN32
    printf("  [1] Console  (ANSI terminal)\n");
    printf("  [2] Graphical (Win32 GDI)\n");
    printf("  [3] Web (HTTP/SSE localhost:8080)\n");
#else
    printf("  [1] Console  (ANSI terminal)\n");
#endif
    printf("> ");
    fflush(stdout);
    choice = getchar();

#ifdef _WIN32
    if (choice == '2') {
        backend = &dsky_gui_backend;
    } else if (choice == '3') {
        backend = &dsky_web_backend;
    } else {
        backend = &dsky_console_backend;
    }
#else
    backend = &dsky_console_backend;
    (void)choice;
#endif

    printf("\nInitializing AGC...\n");

    /* Initialize all subsystems */
    agc_init();
    exec_init();
    waitlist_init();
    timer_init();
    dsky_init();
    pinball_init();

    /* Perform fresh start (DOFSTART) */
    fresh_start();

    /* Initialize navigation state */
    nav_init();

    /* Initialize display backend */
    backend->init();

    /* Console mode: wait for keypress before entering main loop */
    if (backend == &dsky_console_backend) {
        printf("AGC ready. Entering P00 (CMC Idling).\n");
        printf("Press any key to start...\n");
        hal_getch();
    }

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

        /* Refresh DSKY display */
        backend->update();

        /* Check for input */
        backend->poll_input();

        /* Sleep 10ms (~100 Hz cycle) */
        backend->sleep_ms(10);
    }

    /* Not reached, but for completeness */
    backend->cleanup();
    return 0;
}
