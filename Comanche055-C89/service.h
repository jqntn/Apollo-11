/*
 * service.h -- Noun tables and fresh start (SERVICE_ROUTINES.agc,
 *              FRESH_START_AND_RESTART.agc).
 *
 * The noun table maps noun numbers to display characteristics
 * (component count, signedness, scale factor) for Pinball.
 * fresh_start() reinitializes all subsystems (V36 or power-on).
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

#ifndef SERVICE_H
#define SERVICE_H

#include "agc.h"

typedef struct {
    int noun_num;
    int num_components;
    int is_signed;
    int scale_factor;
} noun_table_entry_t;

extern const noun_table_entry_t noun_table[];
extern const int noun_table_size;

void fresh_start(void);

#endif /* SERVICE_H */
