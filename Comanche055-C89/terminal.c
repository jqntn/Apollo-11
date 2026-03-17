/*
 * terminal.c -- Cross-platform terminal UI utilities.
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

#include "terminal.h"

#include <stdio.h>

void
term_init(void)
{
  printf(ANSI_ALT_BUFFER_ON);
  printf(ANSI_HIDE_CURSOR);
  fflush(stdout);
}

void
term_cleanup(void)
{
  printf(ANSI_SHOW_CURSOR);
  printf(ANSI_ALT_BUFFER_OFF);
  fflush(stdout);
}

void
term_set_cursor(int line, int column)
{
  printf(ANSI_CURSOR_POS, line, column);
}

void
term_clear_screen(void)
{
  printf(ANSI_CLEAR_SCREEN);
}
