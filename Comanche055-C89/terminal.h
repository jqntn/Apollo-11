/*
 * terminal.h -- Cross-platform terminal UI utilities
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port
 * Provides flicker-free terminal rendering across Windows, Linux, and macOS
 */

#ifndef TERMINAL_H
#define TERMINAL_H

/* ANSI escape code constants */
#define ANSI_CLEAR_SCREEN "\033[2J\033[H"
#define ANSI_HIDE_CURSOR "\033[?25l"
#define ANSI_SHOW_CURSOR "\033[?25h"
#define ANSI_ALT_BUFFER_ON "\033[?1049h"
#define ANSI_ALT_BUFFER_OFF "\033[?1049l"
#define ANSI_CURSOR_POS "\033[%d;%dH"
#define ANSI_CLEAR_LINE "\033[K"
#define ANSI_CLEAR_LINE_START "\033[1K"

/* Cross-platform terminal functions */
void term_init(void);
void term_cleanup(void);
void term_set_cursor(int line, int column);
void term_clear_screen(void);

#endif /* TERMINAL_H */
