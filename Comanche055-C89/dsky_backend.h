/*
 * dsky_backend.h -- DSKY display backend abstraction
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port
 * Allows switching between console (ANSI) and graphical (Win32) backends.
 */

#ifndef DSKY_BACKEND_H
#define DSKY_BACKEND_H

typedef struct {
    void (*init)(void);
    void (*update)(void);
    void (*poll_input)(void);
    void (*cleanup)(void);
    void (*sleep_ms)(int ms);
} dsky_backend_t;

#endif /* DSKY_BACKEND_H */
