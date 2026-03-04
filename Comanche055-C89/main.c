/*
 * main.c -- Entry point and main loop.
 *
 * Apollo 11 Command Module Guidance Computer
 * Colossus 2A / Comanche 055
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

#include <stdio.h>
#include "args.h"
#include "menu.h"
#include "agc_cpu.h"
#include "executive.h"
#include "timer.h"
#include "service.h"
#include "navigation.h"

/* ----------------------------------------------------------------
 * Main
 * ---------------------------------------------------------------- */

int main(int argc, char *argv[])
{
    dsky_backend_t *backend;

    backend = args_parse_backend(argc, argv);
    if (!backend)
        backend = menu_select_backend();

    printf("Initializing AGC...\n");

    agc_init();
    nav_init();
    fresh_start();

    backend->init();

    /* Main loop: 100 Hz (10ms per tick) */
    while (1) {
        timer_tick();
        exec_run();
        backend->update();
        backend->poll_input();
        backend->sleep_ms(10);
    }

    backend->cleanup();
    return 0;
}
