/*
 * hal.c -- Platform abstraction for console I/O and timing.
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

/* Keep GNU libc from hiding POSIX APIs used below in strict C89 mode. */
#ifndef _WIN32
#define _POSIX_C_SOURCE 199309L
#endif

#include "hal.h"

#ifdef _WIN32

#include <conio.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static DWORD hal_orig_console_mode = 0;
static int hal_console_mode_saved = 0;

int
hal_kbhit(void)
{
  return _kbhit();
}

int
hal_getch(void)
{
  return _getch();
}

void
hal_sleep_ms(int ms)
{
  Sleep(ms);
}

long
hal_time_ms(void)
{
  return (long)GetTickCount64();
}

void
hal_term_init(void)
{
  HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
  if (h != INVALID_HANDLE_VALUE) {
    GetConsoleMode(h, &hal_orig_console_mode);
    hal_console_mode_saved = 1;
    SetConsoleMode(h,
                   hal_orig_console_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
  }
}

void
hal_term_cleanup(void)
{
  if (hal_console_mode_saved) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleMode(h, hal_orig_console_mode);
    hal_console_mode_saved = 0;
  }
}

#else

#include <sys/select.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

static struct termios orig_termios;
static int term_raw = 0;

int
hal_kbhit(void)
{
  struct timeval tv;
  fd_set fds;
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  FD_ZERO(&fds);
  FD_SET(STDIN_FILENO, &fds);
  return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}

int
hal_getch(void)
{
  unsigned char c;
  if (read(STDIN_FILENO, &c, 1) == 1)
    return (int)c;
  return -1;
}

void
hal_sleep_ms(int ms)
{
  struct timeval tv;
  tv.tv_sec = ms / 1000;
  tv.tv_usec = (ms % 1000) * 1000;
  select(0, NULL, NULL, NULL, &tv);
}

long
hal_time_ms(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (long)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

void
hal_term_init(void)
{
  struct termios raw;
  if (!term_raw) {
    tcgetattr(STDIN_FILENO, &orig_termios);
    raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    term_raw = 1;
  }
}

void
hal_term_cleanup(void)
{
  if (term_raw) {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    term_raw = 0;
  }
}

#endif
