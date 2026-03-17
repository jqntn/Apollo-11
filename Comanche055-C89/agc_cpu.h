/*
 * agc_cpu.h -- AGC hardware state: erasable memory, I/O channels,
 *              timers, flag words, and their access routines.
 *
 * Maps to the AGC's erasable storage (8 E-banks x 256 words),
 * I/O channel registers, TIME1-TIME6 counters, and the 12 flag
 * words from ERASABLE_ASSIGNMENTS.agc.
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

#ifndef AGC_CPU_H
#define AGC_CPU_H

#include "agc.h"

/* ----------------------------------------------------------------
 * AGC global state
 * ---------------------------------------------------------------- */

extern agc_word_t agc_erasable[NUM_EBANKS][EBANK_SIZE];
extern agc_word_t agc_channels[NUM_CHANNELS];
extern int agc_ebank;

extern agc_word_t agc_time1;
extern agc_word_t agc_time2;
extern agc_word_t agc_time3;
extern agc_word_t agc_time4;
extern agc_word_t agc_time5;
extern agc_word_t agc_time6;

extern int agc_inhint;
extern agc_word_t agc_flagwords[NUM_FLAGWORDS];
extern int agc_current_program;

/* ----------------------------------------------------------------
 * Erasable memory access
 * ---------------------------------------------------------------- */

agc_word_t
agc_read_erasable(int ebank, int addr);
void
agc_write_erasable(int ebank, int addr, agc_word_t val);

/* ----------------------------------------------------------------
 * I/O channel access
 * ---------------------------------------------------------------- */

agc_word_t
agc_read_channel(int chan);
void
agc_write_channel(int chan, agc_word_t val);

/* ----------------------------------------------------------------
 * Flag operations (UPFLAG / DOWNFLAG)
 * ---------------------------------------------------------------- */

void
agc_flag_set(int flagword, agc_word_t bitmask);
void
agc_flag_clear(int flagword, agc_word_t bitmask);
int
agc_flag_test(int flagword, agc_word_t bitmask);

/* ----------------------------------------------------------------
 * Flag definitions (from ERASABLE_ASSIGNMENTS.agc)
 * Format: FLAGWRD index, BIT number
 * ---------------------------------------------------------------- */

/* FLAGWRD0 */
#define FLGWRD0 0
#define FREEFLAG BIT1
#define DPTS_FLG BIT2
#define NODOV37 BIT3
#define OW0FLAG BIT4
#define ENGOFLAG BIT5
#define AXISFLG3 BIT6
#define NODO37FL BIT7
#define TFFSW BIT8

/* FLAGWRD1 */
#define FLGWRD1 1
#define TRACKFLG BIT1
#define UPDTEFLAG BIT2
#define RENTEFLAG BIT3
#define AVEMIDSW BIT4
#define RNDVZFLG BIT5

/* FLAGWRD2 */
#define FLGWRD2 2
#define LUNTEFLAG BIT1
#define STTEFLAG BIT2
#define MIDFLAG BIT3
#define SURTEFLAG BIT4

/* FLAGWRD3 */
#define FLGWRD3 3
#define VINTFLAG BIT1
#define INTYPFLG BIT2
#define D6OR9FLG BIT3
#define DIM0FLAG BIT4
#define MOESSION BIT5

/* FLAGWRD5 */
#define FLGWRD5 5
#define DSKYFLAG BIT1
#define XDSPFLAG BIT2
#define R1D1EXEC BIT3
#define MESSION BIT4
#define IMPESSION BIT5

/* FLAGWRD7 */
#define FLGWRD7 7
#define V37FLAG BIT1
#define PRGESSION BIT2
#define CMESSION BIT3

/* FLAGWRD8 */
#define FLGWRD8 8
#define LMESSION BIT1

/* ----------------------------------------------------------------
 * Initialization
 * ---------------------------------------------------------------- */

void
agc_init(void);

#endif /* AGC_CPU_H */
