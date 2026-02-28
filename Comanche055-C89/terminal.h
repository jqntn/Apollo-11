/*
 * terminal.h -- Cross-platform terminal UI utilities
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port
 * Provides flicker-free terminal rendering across Windows, Linux, and macOS
 */

#ifndef TERMINAL_H
#define TERMINAL_H

/* Terminal capabilities structure */
typedef struct {
    int supports_alternate_buffer;
    int supports_cursor_positioning;
    int supports_line_clearing;
    int supports_cursor_hide;
} term_capabilities_t;

/* Cross-platform terminal functions */
void term_init(void);
void term_cleanup(void);
void term_set_cursor(int line, int column);
void term_clear_line(int from_start);
void term_write_at(int line, int column, const char *text);

/* Get terminal capabilities (for advanced usage) */
extern term_capabilities_t term_caps;

#endif /* TERMINAL_H */
