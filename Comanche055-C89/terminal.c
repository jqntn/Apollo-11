/*
 * terminal.c -- Cross-platform terminal UI utilities implementation
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port
 * Provides flicker-free terminal rendering across Windows, Linux, and macOS
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "terminal.h"

/* Global terminal capabilities */
term_capabilities_t term_caps;

/* Minimal terminal detection - just try ANSI and see what works */
static void term_detect_capabilities(void)
{
    /* Assume everything works - modern terminals support ANSI */
    term_caps.supports_alternate_buffer = 1;
    term_caps.supports_cursor_positioning = 1;
    term_caps.supports_line_clearing = 1;
    term_caps.supports_cursor_hide = 1;
}

void term_init(void)
{
    term_detect_capabilities();
    
    /* Try alternate screen buffer - if it doesn't work, user gets fallback behavior */
    printf("\033[?1049h");  /* Enable alternate screen buffer */
    printf("\033[?25l");     /* Hide cursor */
    fflush(stdout);
}

void term_cleanup(void)
{
    /* Always try to restore - if alternate buffer wasn't enabled, this does nothing */
    printf("\033[?25h");     /* Show cursor */
    printf("\033[?1049l");  /* Restore original screen buffer */
    fflush(stdout);
}

void term_set_cursor(int line, int column)
{
    printf("\033[%d;%dH", line, column);
}

void term_clear_line(int from_start)
{
    if (from_start) {
        printf("\033[1K");  /* Clear from beginning to cursor */
    } else {
        printf("\033[K");    /* Clear from cursor to end */
    }
}

void term_write_at(int line, int column, const char *text)
{
    term_set_cursor(line, column);
    printf("%s", text);
}
