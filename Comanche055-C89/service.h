/*
 * service.h -- Flag routines (UPFLAG/DOWNFLAG), noun tables
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port
 */

#ifndef SERVICE_H
#define SERVICE_H

#include "agc.h"

/* ----------------------------------------------------------------
 * Noun table entry
 * ----------------------------------------------------------------
 * Maps noun numbers to data characteristics for Pinball display/load.
 */

typedef struct {
    int noun_num;       /* Noun number (0-99) */
    int num_components; /* Number of display components (1-3) */
    int is_signed;      /* 1 if display uses +/- sign */
    int scale_factor;   /* Scale factor exponent for conversion */
} noun_table_entry_t;

/* Noun table */
extern const noun_table_entry_t noun_table[];
extern const int noun_table_size;

/* Look up a noun in the table. Returns pointer or NULL. */
const noun_table_entry_t *noun_lookup(int noun_num);

/* ----------------------------------------------------------------
 * Fresh start
 * ---------------------------------------------------------------- */

/* Perform a fresh start (V36 or power-on) */
void fresh_start(void);

#endif /* SERVICE_H */
