/*
 * menu.h -- Backend selection menu.
 *
 * Presents an interactive console menu for choosing the DSKY
 * display backend (console, Win32 GDI, or HTTP/SSE web).
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

#ifndef MENU_H
#define MENU_H

#include "dsky_backend.h"

dsky_backend_t*
menu_select_backend(void);

#endif /* MENU_H */
