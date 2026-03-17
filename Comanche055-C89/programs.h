/*
 * programs.h -- P00 (CMC Idling), V37 dispatch, program stubs.
 *
 * P00 is the default idle program.  All other programs are stubs
 * that display a PROG alarm; they exist as placeholders for future
 * port iterations.  program_change() implements the V37 dispatch
 * table.
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

#ifndef PROGRAMS_H
#define PROGRAMS_H

void
program_change(int prognum);
void
program_p00(void);
void
program_stub(int prognum);

#endif /* PROGRAMS_H */
