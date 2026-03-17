/*
 * main.c -- Entry point and main loop.
 *
 * Apollo 11 Command Module Guidance Computer
 * Colossus 2A / Comanche 055
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

#include "agc_cpu.h"
#include "args.h"
#include "dsky_backend.h"
#include "executive.h"
#include "hal.h"
#include "menu.h"
#include "navigation.h"
#include "service.h"
#include "timer.h"

#include <stdio.h>

/* ----------------------------------------------------------------
 * Main
 * ---------------------------------------------------------------- */

int
main(int argc, char* argv[])
{
  dsky_backend_t* backend;
  long last_time;
  int accumulated_ms;

  backend = args_parse_backend(argc, argv);
  if (!backend) {
    backend = menu_select_backend();
  }

  printf("Initializing AGC...\n");

  agc_init();
  nav_init();
  fresh_start();

  backend->init();

  last_time = hal_time_ms();
  accumulated_ms = 0;

  /* Main loop: 100 Hz (10ms per tick) */
  while (1) {
    long current_time = hal_time_ms();
    int elapsed = (int)(current_time - last_time);
    last_time = current_time;

    accumulated_ms += elapsed;
    if (accumulated_ms > 500) {
      accumulated_ms = 500;
    }

    while (accumulated_ms >= 10) {
      timer_tick();
      exec_run();
      accumulated_ms -= 10;
    }

    backend->update();
    backend->poll_input();

    if (accumulated_ms < 10) {
      backend->sleep_ms(10 - accumulated_ms);
    }
  }

  backend->cleanup();
  return 0;
}
