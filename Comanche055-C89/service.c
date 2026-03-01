/*
 * service.c -- Flag routines (UPFLAG/DOWNFLAG), noun tables, fresh start
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port
 */

#include <string.h>
#include "service.h"
#include "agc_cpu.h"
#include "executive.h"
#include "waitlist.h"
#include "dsky.h"
#include "pinball.h"
#include "alarm.h"
#include "timer.h"

/* ----------------------------------------------------------------
 * Noun table
 * ----------------------------------------------------------------
 * Defines display characteristics for each supported noun.
 */

const noun_table_entry_t noun_table[] = {
    /* noun, components, signed, scale_factor */
    {  1, 3, 0,  0 },
    {  9, 3, 0,  0 },
    { 36, 3, 1,  0 },
    { 43, 3, 1,  0 },
    { 44, 3, 1,  0 },
    { 65, 3, 1,  0 },
};

const int noun_table_size = sizeof(noun_table) / sizeof(noun_table[0]);

const noun_table_entry_t *noun_lookup(int noun_num)
{
    int i;
    for (i = 0; i < noun_table_size; i++) {
        if (noun_table[i].noun_num == noun_num) {
            return &noun_table[i];
        }
    }
    return NULL;
}

/* ----------------------------------------------------------------
 * Fresh start (DOFSTART equivalent)
 * ----------------------------------------------------------------
 * Reinitializes the AGC to a known state. Called on V36 or power-on.
 */

void fresh_start(void)
{
    /* Clear erasable memory */
    memset(agc_erasable, 0, sizeof(agc_erasable));

    /* Clear flagwords */
    memset(agc_flagwords, 0, sizeof(agc_flagwords));

    /* Reset channels to initial state */
    memset(agc_channels, 0, sizeof(agc_channels));
    agc_channels[CHAN_CHAN30] = 037777;
    agc_channels[CHAN_CHAN31] = 037777;
    agc_channels[CHAN_CHAN32] = 037777;
    agc_channels[CHAN_CHAN33] = 037777;

    /* Reset timers */
    timer_init();

    /* Reset interrupt inhibit */
    agc_inhint = 0;
    agc_ebank = 0;

    /* Reset Executive */
    exec_init();

    /* Reset Waitlist */
    waitlist_init();

    /* Reset DSKY display */
    dsky_init();

    /* Reset Pinball */
    pinball_init();

    /* Reset alarms */
    alarm_reset();

    /* Set program to P00 (CMC Idling) */
    pinball_show_prog(0);
    pinball_show_verb(0);
    pinball_show_noun(0);

    agc_current_program = 0;
}
