/*
 * terminal.c -- Cross-platform terminal UI utilities implementation
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port
 * Provides flicker-free terminal rendering across Windows, Linux, and macOS
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "terminal.h"

void term_init(void)
{
    /* Enable alternate screen buffer and hide cursor */
    printf(ANSI_ALT_BUFFER_ON);
    printf(ANSI_HIDE_CURSOR);
    fflush(stdout);
}

void term_cleanup(void)
{
    /* Show cursor and restore original screen buffer */
    printf(ANSI_SHOW_CURSOR);
    printf(ANSI_ALT_BUFFER_OFF);
    fflush(stdout);
}

void term_set_cursor(int line, int column)
{
    printf(ANSI_CURSOR_POS, line, column);
}

void term_clear_screen(void)
{
    printf(ANSI_CLEAR_SCREEN);
}
