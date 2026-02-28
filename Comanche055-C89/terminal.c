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

#ifdef _WIN32
#include <windows.h>
#endif

/* Global terminal capabilities */
term_capabilities_t term_caps;

/* Cross-platform terminal detection and setup */
static void term_detect_capabilities(void)
{
    /* Assume basic ANSI support everywhere */
    term_caps.supports_cursor_positioning = 1;
    term_caps.supports_line_clearing = 1;
    term_caps.supports_cursor_hide = 1;
    term_caps.supports_alternate_buffer = 0;
    
#ifdef _WIN32
    /* Windows: Check if ANSI is supported */
    HANDLE h_out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    if (GetConsoleMode(h_out, &mode)) {
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (SetConsoleMode(h_out, mode)) {
            term_caps.supports_alternate_buffer = 1;
        }
    }
#else
    /* Unix/macOS: Assume modern terminal support */
    term_caps.supports_alternate_buffer = 1;
#endif
}

void term_init(void)
{
    term_detect_capabilities();
    
    if (term_caps.supports_alternate_buffer) {
        printf("\033[?1049h");  /* Enable alternate screen buffer */
    }
    
    if (term_caps.supports_cursor_hide) {
        printf("\033[?25l");  /* Hide cursor */
    }
    fflush(stdout);
}

void term_cleanup(void)
{
    if (term_caps.supports_cursor_hide) {
        printf("\033[?25h");  /* Show cursor */
    }
    
    if (term_caps.supports_alternate_buffer) {
        printf("\033[?1049l");  /* Restore original screen buffer */
    }
    fflush(stdout);
}

void term_set_cursor(int line, int column)
{
    if (term_caps.supports_cursor_positioning) {
        printf("\033[%d;%dH", line, column);
    }
}

void term_clear_line(int from_start)
{
    if (term_caps.supports_line_clearing) {
        if (from_start) {
            printf("\033[1K");  /* Clear from beginning to cursor */
        } else {
            printf("\033[K");    /* Clear from cursor to end */
        }
    }
}

void term_write_at(int line, int column, const char *text)
{
    term_set_cursor(line, column);
    printf("%s", text);
}
