/*
 * menu.c -- Backend selection menu.
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

#ifdef _WIN32
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include "menu.h"
#include "dsky.h"
#include "terminal.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "dsky_gui.h"
#include "dsky_web.h"
#endif

/* ----------------------------------------------------------------
 * Types and constants
 * ---------------------------------------------------------------- */

typedef struct {
    const char *label;
    dsky_backend_t *backend;
} backend_option_t;

#define MENU_KEY_NONE   0
#define MENU_KEY_UP     1
#define MENU_KEY_DOWN   2
#define MENU_KEY_ENTER  3
#define MENU_KEY_SELECT 4

/* ----------------------------------------------------------------
 * Input
 * ---------------------------------------------------------------- */

static int menu_read_key(int *selected_index)
{
    int ch;
    int next;
#ifndef _WIN32
    int third;
#endif
    *selected_index = -1;

    if (!hal_kbhit())
        return MENU_KEY_NONE;

    ch = hal_getch();
    if (ch < 0)
        return MENU_KEY_NONE;

    if (ch == '\r' || ch == '\n')
        return MENU_KEY_ENTER;
    if (ch >= '1' && ch <= '9') {
        *selected_index = ch - '1';
        return MENU_KEY_SELECT;
    }

#ifdef _WIN32
    if (ch == 0 || ch == 224) {
        next = hal_getch();
        if (next == 72) return MENU_KEY_UP;
        if (next == 80) return MENU_KEY_DOWN;
    }
#else
    if (ch == 27) {
        next = hal_getch();
        if (next != '[') return MENU_KEY_NONE;
        third = hal_getch();
        if (third == 'A') return MENU_KEY_UP;
        if (third == 'B') return MENU_KEY_DOWN;
    }
#endif

    return MENU_KEY_NONE;
}

/* ----------------------------------------------------------------
 * Rendering
 * ---------------------------------------------------------------- */

static void menu_render(const backend_option_t *options, int count,
                        int selected)
{
    int i;
    static int prev_selected = -1;
    static int first_render = 1;
    char line_buffer[256];

    if (first_render) {
        term_init();

        term_set_cursor(1, 0);
        printf("===========================================");
        term_set_cursor(2, 2);
        printf("COMANCHE 055 -- Colossus 2A");
        term_set_cursor(3, 2);
        printf("Apollo 11 CM Guidance Computer");
        term_set_cursor(4, 2);
        printf("ANSI C89 Port");
        term_set_cursor(5, 0);
        printf("===========================================");

        sprintf(line_buffer,
                "Select display mode (Up/Down + Enter, or 1-%d):",
                count);
        term_set_cursor(7, 0);
        printf("%s", line_buffer);

        for (i = 0; i < count; i++) {
            sprintf(line_buffer, "%s [%d] %s",
                    (i == selected) ? ">" : " ", i + 1,
                    options[i].label);
            term_set_cursor(9 + i, 0);
            printf("%s", line_buffer);
        }

        fflush(stdout);
        first_render = 0;
        prev_selected = selected;
    } else if (selected != prev_selected) {
        int menu_start_line = 9;

        sprintf(line_buffer, "  [%d] %s",
                prev_selected + 1, options[prev_selected].label);
        term_set_cursor(menu_start_line + prev_selected, 0);
        printf("%s", line_buffer);

        sprintf(line_buffer, "> [%d] %s",
                selected + 1, options[selected].label);
        term_set_cursor(menu_start_line + selected, 0);
        printf("%s", line_buffer);

        fflush(stdout);
        prev_selected = selected;
    }
}

/* ----------------------------------------------------------------
 * Backend selection
 * ---------------------------------------------------------------- */

dsky_backend_t *menu_select_backend(void)
{
    backend_option_t options[3];
    int count;
    int selected;
    int key;
    int direct_index;
    int needs_render;

#ifdef _WIN32
    options[0].label = "Console   (ANSI terminal)";
    options[0].backend = &dsky_console_backend;
    options[1].label = "Graphical (Win32 GDI)";
    options[1].backend = &dsky_gui_backend;
    options[2].label = "Web       (HTTP/SSE)";
    options[2].backend = &dsky_web_backend;
    count = 3;
#else
    options[0].label = "Console   (ANSI terminal)";
    options[0].backend = &dsky_console_backend;
    count = 1;
#endif

    selected = 0;
    needs_render = 1;
    hal_term_init();

    while (1) {
        if (needs_render) {
            menu_render(options, count, selected);
            needs_render = 0;
        }

        key = menu_read_key(&direct_index);
        if (key == MENU_KEY_UP) {
            if (selected > 0) { selected--; needs_render = 1; }
        } else if (key == MENU_KEY_DOWN) {
            if (selected < count - 1) { selected++; needs_render = 1; }
        } else if (key == MENU_KEY_ENTER) {
            break;
        } else if (key == MENU_KEY_SELECT) {
            if (direct_index >= 0 && direct_index < count) {
                selected = direct_index;
                break;
            }
        }

        hal_sleep_ms(10);
    }

    hal_term_cleanup();
    term_cleanup();
    term_clear_screen();

#ifdef _WIN32
    if (options[selected].backend == &dsky_web_backend) {
        int rc;
        printf("Opening browser at http://127.0.0.1:8080/\n");
        rc = system("cmd /c start \"\" \"http://127.0.0.1:8080/\"");
        if (rc != 0) {
            printf("Could not open browser automatically.\n");
            printf("Open this URL manually: http://127.0.0.1:8080/\n");
        }
    }
#endif

    return options[selected].backend;
}
