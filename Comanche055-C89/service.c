/*
 * service.c -- Noun tables and fresh start.
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

#include "agc.h"
#include "agc_cpu.h"
#include "alarm.h"
#include "dsky.h"
#include "executive.h"
#include "pinball.h"
#include "service.h"
#include "timer.h"
#include "waitlist.h"

#include <string.h>

/* ----------------------------------------------------------------
 * Noun table
 * ---------------------------------------------------------------- */

const noun_table_entry_t noun_table[] = {
  /* noun, components, signed, scale_factor */
  { 1, 3, 0, 0 },  { 9, 3, 0, 0 },  { 36, 3, 1, 0 },
  { 43, 3, 1, 0 }, { 44, 3, 1, 0 }, { 65, 3, 1, 0 }
};

const int noun_table_size = sizeof(noun_table) / sizeof(noun_table[0]);

/* ----------------------------------------------------------------
 * Fresh start (DOFSTART equivalent)
 * ---------------------------------------------------------------- */

void
fresh_start(void)
{
  memset(agc_erasable, 0, sizeof(agc_erasable));
  memset(agc_flagwords, 0, sizeof(agc_flagwords));

  memset(agc_channels, 0, sizeof(agc_channels));
  agc_channels[CHAN_CHAN30] = 037777;
  agc_channels[CHAN_CHAN31] = 037777;
  agc_channels[CHAN_CHAN32] = 037777;
  agc_channels[CHAN_CHAN33] = 037777;

  timer_init();
  agc_inhint = 0;
  agc_ebank = 0;

  exec_init();
  waitlist_init();
  dsky_init();
  pinball_init();
  alarm_reset();

  pinball_show_prog(0);
  pinball_show_verb(0);
  pinball_show_noun(0);

  agc_current_program = 0;
}
