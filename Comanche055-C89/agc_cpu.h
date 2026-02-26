/*
 * agc_cpu.h -- AGC state: erasable memory, channels, timers, flags
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port
 */

#ifndef AGC_CPU_H
#define AGC_CPU_H

#include "agc.h"

/* ----------------------------------------------------------------
 * AGC global state
 * ---------------------------------------------------------------- */

/* Erasable memory: 8 E-banks x 256 words */
extern agc_word_t agc_erasable[NUM_EBANKS][EBANK_SIZE];

/* I/O channels */
extern agc_word_t agc_channels[NUM_CHANNELS];

/* Current E-bank register (0-7) */
extern int agc_ebank;

/* Timers: TIME1 through TIME6 */
extern agc_word_t agc_time1;
extern agc_word_t agc_time2;
extern agc_word_t agc_time3;
extern agc_word_t agc_time4;
extern agc_word_t agc_time5;
extern agc_word_t agc_time6;

/* Interrupt inhibit flag */
extern int agc_inhint;

/* Flag words (FLAGWRD0 through FLAGWRD11) */
extern agc_word_t agc_flagwords[NUM_FLAGWORDS];

/* Current program number (for DSKY PROG display) */
extern int agc_current_program;

/* ----------------------------------------------------------------
 * Erasable memory access
 * ---------------------------------------------------------------- */

/* Read a word from erasable memory by E-bank relative address */
agc_word_t agc_read_erasable(int ebank, int addr);

/* Write a word to erasable memory */
void agc_write_erasable(int ebank, int addr, agc_word_t val);

/* ----------------------------------------------------------------
 * I/O channel access
 * ---------------------------------------------------------------- */

/* Read an I/O channel */
agc_word_t agc_read_channel(int chan);

/* Write an I/O channel */
void agc_write_channel(int chan, agc_word_t val);

/* ----------------------------------------------------------------
 * Flag operations (UPFLAG / DOWNFLAG)
 * ---------------------------------------------------------------- */

/* Set a flag bit: flagword_index (0-11), bit_mask */
void agc_flag_set(int flagword, agc_word_t bitmask);

/* Clear a flag bit */
void agc_flag_clear(int flagword, agc_word_t bitmask);

/* Test a flag bit: returns non-zero if set */
int agc_flag_test(int flagword, agc_word_t bitmask);

/* ----------------------------------------------------------------
 * Flag definitions (from ERASABLE_ASSIGNMENTS.agc)
 *
 * Format: FLAGWRD index, BIT number
 * ---------------------------------------------------------------- */

/* FLAGWRD0 */
#define FLGWRD0   0
#define FREEFLAG  BIT1    /* Free flag */
#define DPTS_FLG  BIT2    /* Drop out of servicer flag */
#define NODOV37   BIT3    /* Inhibit V37 */
#define OW0FLAG   BIT4    /* Orbital W-matrix init */
#define ENGOFLAG  BIT5    /* Engine off flag */
#define AXISFLG3  BIT6    /* 3-axis autopilot flag */
#define NODO37FL  BIT7    /* Another V37 inhibit */
#define TFFSW     BIT8    /* TFF switch */

/* FLAGWRD1 */
#define FLGWRD1   1
#define TRACKFLG  BIT1    /* Tracking flag */
#define UPDTEFLAG BIT2    /* Update flag */
#define RENTEFLAG BIT3    /* Reentry flag */
#define AVEMIDSW  BIT4    /* Average midcourse switch */
#define RNDVZFLG  BIT5    /* Rendezvous flag */

/* FLAGWRD2 */
#define FLGWRD2   2
#define LUNTEFLAG BIT1    /* Lunar terrain flag */
#define STTEFLAG  BIT2    /* State flag */
#define MIDFLAG   BIT3    /* Midcourse flag */
#define SURTEFLAG BIT4    /* Surface flag */

/* FLAGWRD3 */
#define FLGWRD3   3
#define VINTFLAG  BIT1    /* V-integration flag */
#define INTYPFLG  BIT2    /* Integration type flag */
#define D6OR9FLG  BIT3    /* Dimension flag */
#define DIM0FLAG  BIT4    /* Dimension 0 flag */
#define MOESSION  BIT5    /* Moon flag for integration */

/* FLAGWRD5 */
#define FLGWRD5   5
#define DSKYFLAG  BIT1    /* DSKY use flag */
#define XDSPFLAG  BIT2    /* Extended display flag */
#define R1D1EXEC  BIT3    /* R1D1 executive flag */
#define MESSION   BIT4    /* Mission flag */
#define IMPESSION BIT5    /* Impulsive flag */

/* FLAGWRD7 */
#define FLGWRD7   7
#define V37FLAG   BIT1    /* V37 execution flag */
#define PRGESSION BIT2    /* Program change flag */
#define CMESSION  BIT3    /* CM separation flag */

/* FLAGWRD8 */
#define FLGWRD8   8
#define LMESSION  BIT1    /* LM separation flag */

/* ----------------------------------------------------------------
 * Initialization
 * ---------------------------------------------------------------- */

/* Zero all erasable memory, channels, and timers */
void agc_init(void);

#endif /* AGC_CPU_H */
