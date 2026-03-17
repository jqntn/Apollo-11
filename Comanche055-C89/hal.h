/*
 * hal.h -- Platform abstraction for console I/O and timing.
 *
 * Provides non-blocking keyboard input, terminal mode setup/restore,
 * sleep, and monotonic time services for front-end modules.
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

#ifndef HAL_H
#define HAL_H

int
hal_kbhit(void);
int
hal_getch(void);
void
hal_term_init(void);
void
hal_term_cleanup(void);
void
hal_sleep_ms(int ms);
long
hal_time_ms(void);

#endif /* HAL_H */
