/*
 * programs.h -- P00, V35/V36/V37, program stubs
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port
 */

#ifndef PROGRAMS_H
#define PROGRAMS_H

#include "agc.h"

/* Change to a new program (V37 dispatch) */
void program_change(int prognum);

/* P00: CMC Idling -- the default idle program */
void program_p00(void);

/* Program stubs (callable, show alarm) */
void program_stub(int prognum);

#endif /* PROGRAMS_H */
