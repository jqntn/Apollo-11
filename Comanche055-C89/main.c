/*
 * main.c -- Entry point, main loop, timing
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port
 *
 * Apollo 11 Command Module Guidance Computer
 * Colossus 2A / Comanche 055
 *
 * This is a faithful C89 port of the AGC flight software.
 * Supports both an ANSI console backend and a Win32 graphical backend.
 * The user selects the display mode at startup.
 */

#include <stdio.h>
#include <stdlib.h>
#include "agc.h"
#include "agc_cpu.h"
#include "agc_math.h"
#include "executive.h"
#include "waitlist.h"
#include "timer.h"
#include "dsky.h"
#include "pinball.h"
#include "alarm.h"
#include "service.h"
#include "programs.h"
#include "navigation.h"
#ifdef _WIN32
#include <windows.h>
#include "dsky_gui.h"
#include "dsky_web.h"
#endif

typedef struct {
    const char *label;
    dsky_backend_t *backend;
} backend_option_t;

#define MENU_KEY_NONE   0
#define MENU_KEY_UP     1
#define MENU_KEY_DOWN   2
#define MENU_KEY_ENTER  3
#define MENU_KEY_SELECT 4

static void menu_clear_screen(void)
{
#ifdef _WIN32
    HANDLE h_out;
    COORD top_left;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD written;
    DWORD cells;

    h_out = GetStdHandle(STD_OUTPUT_HANDLE);
    top_left.X = 0;
    top_left.Y = 0;

    if (GetConsoleScreenBufferInfo(h_out, &csbi)) {
        cells = (DWORD)csbi.dwSize.X * (DWORD)csbi.dwSize.Y;
        FillConsoleOutputCharacterA(h_out, ' ', cells, top_left, &written);
        FillConsoleOutputAttribute(h_out, csbi.wAttributes, cells, top_left, &written);
        SetConsoleCursorPosition(h_out, top_left);
    } else {
        printf("\033[2J\033[H");
    }
#else
    printf("\033[2J\033[H");
#endif
}

static int menu_read_key(int *selected_index)
{
    int ch;
    int next;
#ifndef _WIN32
    int third;
#endif
    *selected_index = -1;

    if (!hal_kbhit()) return MENU_KEY_NONE;

    ch = hal_getch();
    if (ch < 0) return MENU_KEY_NONE;

    if (ch == '\r' || ch == '\n') return MENU_KEY_ENTER;
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

static void menu_render(const backend_option_t *options, int count, int selected)
{
    int i;
    menu_clear_screen();

    printf("===========================================\n");
    printf("  COMANCHE 055 -- Colossus 2A\n");
    printf("  Apollo 11 CM Guidance Computer\n");
    printf("  ANSI C89 Port\n");
    printf("===========================================\n");
    printf("\nSelect display mode (Up/Down + Enter, or 1-%d):\n\n", count);

    for (i = 0; i < count; i++) {
        if (i == selected) {
            printf(" > [%d] %s\n", i + 1, options[i].label);
        } else {
            printf("   [%d] %s\n", i + 1, options[i].label);
        }
    }

    fflush(stdout);
}

static dsky_backend_t *select_backend_interactive(void)
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
            if (selected > 0) {
                selected--;
                needs_render = 1;
            }
        } else if (key == MENU_KEY_DOWN) {
            if (selected < count - 1) {
                selected++;
                needs_render = 1;
            }
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
    menu_clear_screen();
    return options[selected].backend;
}

#ifdef _WIN32
static void maybe_open_web_ui(dsky_backend_t *backend)
{
    int rc;
    if (backend != &dsky_web_backend) return;

    printf("Opening browser at http://127.0.0.1:8080/\n");
    rc = system("cmd /c start \"\" \"http://127.0.0.1:8080/\"");
    if (rc != 0) {
        printf("Could not open browser automatically.\n");
        printf("Open this URL manually: http://127.0.0.1:8080/\n");
    }
}
#endif

int main(void)
{
    dsky_backend_t *backend;

    /* Select display backend */
    backend = select_backend_interactive();

    printf("\nInitializing AGC...\n");

    /* Initialize all subsystems */
    agc_init();
    exec_init();
    waitlist_init();
    timer_init();
    dsky_init();
    pinball_init();

    /* Perform fresh start (DOFSTART) */
    fresh_start();

    /* Initialize navigation state */
    nav_init();

    /* Initialize display backend */
    backend->init();
#ifdef _WIN32
    maybe_open_web_ui(backend);
#endif

    /* Console mode: wait for keypress before entering main loop */
    if (backend == &dsky_console_backend) {
        printf("AGC ready. Entering P00 (CMC Idling).\n");
        printf("Press any key to start...\n");
        hal_getch();
    }

    /* Force initial display */
    pinball_show_prog(0);
    pinball_show_verb(0);
    pinball_show_noun(0);

    /* ---- Main loop: 100 Hz (10ms per tick) ---- */
    while (1) {
        /* Advance timers, fire interrupts (T3RUPT, T4RUPT) */
        timer_tick();

        /* Run highest priority job (one quantum) */
        exec_run();

        /* Refresh DSKY display */
        backend->update();

        /* Check for input */
        backend->poll_input();

        /* Sleep 10ms (~100 Hz cycle) */
        backend->sleep_ms(10);
    }

    /* Not reached, but for completeness */
    backend->cleanup();
    return 0;
}
