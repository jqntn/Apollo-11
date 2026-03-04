/*
 * agc.h -- Core AGC types, constants, and macros.
 *
 * Defines the fundamental data types for the AGC's 15-bit + sign
 * 1's complement arithmetic, bit/mask/priority tables from
 * FIXED_FIXED_CONSTANT_POOL, channel definitions, memory layout
 * constants, and function pointer types for jobs and tasks.
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

#ifndef AGC_H
#define AGC_H

#include <stddef.h>  /* NULL */

/* AGC word: 16-bit signed, represents 15 data bits + sign */
typedef short agc_word_t;

/* Double precision: 32-bit signed, holds 30-bit DP values */
typedef int agc_dp_t;

/*
 * 64-bit integer for intermediate calculations (avoids 32-bit overflow).
 * Required by DP multiply, divide, and sqrt for bit-accurate results.
 * Neither __int64 nor long long is standard C89, but both are universally
 * supported as pre-C99 extensions.  Suppress -Wlong-long where needed.
 */
#ifdef _MSC_VER
typedef __int64 agc_int64_t;
#else
typedef long long agc_int64_t;
#endif

/* ----------------------------------------------------------------
 * AGC 1's complement constants
 * ---------------------------------------------------------------- */

#define AGC_POSMAX   ((agc_word_t)16383)   /* 037777 octal */
#define AGC_NEGMAX   ((agc_word_t)-16383)
#define AGC_POS_ZERO ((agc_word_t)0)

/* Bit table (octal values from FIXED_FIXED_CONSTANT_POOL) */
#define BIT1   0x0001
#define BIT2   0x0002
#define BIT3   0x0004
#define BIT4   0x0008
#define BIT5   0x0010
#define BIT6   0x0020
#define BIT7   0x0040
#define BIT8   0x0080
#define BIT9   0x0100
#define BIT10  0x0200
#define BIT11  0x0400
#define BIT12  0x0800
#define BIT13  0x1000
#define BIT14  0x2000
#define BIT15  0x4000

/* Mask constants */
#define LOW4       0x000F
#define LOW5       0x001F
#define LOW7       0x007F
#define LOW8       0x00FF
#define LOW9       0x01FF
#define LOW10      0x03FF
#define LOW11      0x07FF
#define HIGH4      0x7800
#define HIGH9      0x7FC0
#define BANKMASK   0x7C00
#define OCT1400    0x0300
#define OCT37776   0x3FFE
#define OCT77770   0x7FF8

/* Priority constants (octal, from FIXED_FIXED_CONSTANT_POOL) */
#define PRIO1   BIT10
#define PRIO2   BIT11
#define PRIO3   01400
#define PRIO5   02400
#define PRIO6   03000
#define PRIO7   03400
#define PRIO10  BIT13
#define PRIO11  04400
#define PRIO12  05000
#define PRIO13  05400
#define PRIO14  06000
#define PRIO15  06400
#define PRIO16  07000
#define PRIO17  07400
#define PRIO20  BIT14
#define PRIO21  010400
#define PRIO22  011000
#define PRIO23  011400
#define PRIO24  012000
#define PRIO25  012400
#define PRIO26  013000
#define PRIO27  013400
#define PRIO30  014000
#define PRIO31  014400
#define PRIO32  015000
#define PRIO33  015400
#define PRIO34  016000
#define PRIO35  016400
#define PRIO36  017000
#define PRIO37  017400

/* Time constants (centiseconds) */
#define HALF_SEC   50
#define ONE_SEC    100
#define TWO_SECS   200
#define THREE_SECS 300
#define FOUR_SECS  400

/* Fixed-point math constants scaled at 1 (15-bit fraction) */
#define AGC_HALF     ((agc_word_t)0x4000)
#define AGC_QUARTER  ((agc_word_t)0x2000)
#define AGC_NEG_HALF ((agc_word_t)-0x4000)

/* SP sin/cos polynomial coefficients (from SINGLE_PRECISION_SUBROUTINES) */
#define SP_C1_2  ((agc_word_t)0x6488)   /* C1/2 = 0.7853134 */
#define SP_C3_2  ((agc_word_t)-0x2969)  /* C3/2 = -0.3216147 */
#define SP_C5_2  ((agc_word_t)0x04A9)   /* C5/2 = 0.0363551 */

/* ----------------------------------------------------------------
 * 1's complement arithmetic
 * ----------------------------------------------------------------
 *
 * The AGC uses 1's complement where:
 *   +0 = 0x0000, -0 = 0x7FFF (all 15 data bits set)
 *
 * We model AGC words in 2's complement C, mapping:
 *   AGC positive: 1..16383
 *   AGC negative: -1..-16383
 *   Overflow: +16384 / -16384 (one beyond POSMAX/NEGMAX)
 */

agc_word_t agc_overflow_correct(int val);
agc_word_t agc_add(agc_word_t a, agc_word_t b);
agc_word_t agc_negate(agc_word_t val);
agc_word_t agc_abs(agc_word_t val);

/* Diminished absolute value (CCS behavior) */
agc_word_t agc_dabs(agc_word_t val);

/* CCS 4-way branch index: >0 -> 0, +0 -> 1, <0 -> 2, -0 -> 3 */
int agc_ccs_branch(agc_word_t val);

#define AGC_IS_POSITIVE(v) ((v) > 0)
#define AGC_IS_NEGATIVE(v) ((v) < 0)
#define AGC_IS_ZERO(v)     ((v) == 0)

/* ----------------------------------------------------------------
 * AGC channel definitions
 * ---------------------------------------------------------------- */

#define CHAN_L        0x0001
#define CHAN_Q        0x0002
#define CHAN_HISCALAR 0x0003
#define CHAN_LOSCALAR 0x0004
#define CHAN_PYJETS   0x0005
#define CHAN_ROLLJETS  0x0006
#define CHAN_SUPERBNK 0x0007
#define CHAN_OUT0     0x000A  /* Channel 10: DSKY display relay words */
#define CHAN_DSALMOUT 0x000B  /* Channel 11: DSKY alarm/status lights */
#define CHAN_CHAN12   0x000C
#define CHAN_CHAN13   0x000D
#define CHAN_CHAN14   0x000E
#define CHAN_MNKEYIN  0x000F  /* Channel 15: keyboard input */
#define CHAN_NAVKEYIN 0x0010  /* Channel 16: nav DSKY keyboard */
#define CHAN_CHAN30   0x001E
#define CHAN_CHAN31   0x001F
#define CHAN_CHAN32   0x0020
#define CHAN_CHAN33   0x0021
#define CHAN_CHAN34   0x0022
#define CHAN_CHAN77   0x003F

/* ----------------------------------------------------------------
 * E-bank and memory layout
 * ---------------------------------------------------------------- */

#define NUM_EBANKS      8
#define EBANK_SIZE    256
#define NUM_CHANNELS  256

#define NUM_CORE_SETS   7
#define NUM_VAC_AREAS   5
#define VAC_AREA_SIZE  43

#define NUM_WAITLIST_TASKS 9
#define NUM_FLAGWORDS  12
#define NUM_PHASES      6

/* Function pointer types for jobs and tasks */
typedef void (*agc_jobfunc_t)(void);
typedef void (*agc_taskfunc_t)(void);

#endif /* AGC_H */
