/*
 * dsky_backend.h -- DSKY display backend abstraction.
 *
 * Virtual table allowing the main loop to drive any display
 * backend (console ANSI, Win32 GDI, or HTTP/SSE web) through
 * a uniform interface.
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

#ifndef DSKY_BACKEND_H
#define DSKY_BACKEND_H

typedef struct
{
  void (*init)(void);
  void (*update)(void);
  void (*poll_input)(void);
  void (*cleanup)(void);
  void (*sleep_ms)(int ms);
} dsky_backend_t;

#endif /* DSKY_BACKEND_H */
