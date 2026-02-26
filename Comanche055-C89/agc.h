/*
 * agc.h -- Core AGC types, constants, and macros
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port
 * Faithful reproduction of the AGC's 15-bit + sign, 1's complement arithmetic
 */

#ifndef AGC_H
#define AGC_H

/* AGC word: 16-bit signed, represents 15 data bits + sign (1's complement) */
typedef short agc_word_t;

/* Double precision: 32-bit signed, holds 30-bit DP values (two AGC words) */
typedef int agc_dp_t;

/* 64-bit integer for intermediate calculations (avoids 32-bit overflow).
 * long long is a C99/GNU extension; suppressed via -Wno-long-long. */
typedef long long agc_int64_t;

/* ----------------------------------------------------------------
 * AGC 1's complement constants
 * ---------------------------------------------------------------- */

#define AGC_POSMAX   ((agc_word_t)16383)   /* 037777 octal = 16383 decimal */
#define AGC_NEGMAX   ((agc_word_t)-16383)
#define AGC_POS_ZERO ((agc_word_t)0)
#define AGC_NEG_ZERO ((agc_word_t)(-0x7FFF)) /* -0 in 1's complement: all 1s */

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

/* Named numeric constants */
#define AGC_ZERO     0
#define AGC_ONE      1
#define AGC_TWO      2
#define AGC_THREE    3
#define AGC_FOUR     4
#define AGC_FIVE     5
#define AGC_SIX      6
#define AGC_SEVEN    7
#define AGC_EIGHT    8
#define AGC_NINE     9
#define AGC_TEN      10
#define AGC_ELEVEN   11

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

/* Priority constants (octal from FIXED_FIXED_CONSTANT_POOL) */
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
#define HALF_SEC  50
#define ONE_SEC   100
#define TWO_SECS  200
#define THREE_SECS 300
#define FOUR_SECS 400

/* Fixed-point math constants scaled at 1 (15-bit fraction) */
/* HALF = 0.5, QUARTER = 0.25 */
#define AGC_HALF     ((agc_word_t)0x4000)
#define AGC_QUARTER  ((agc_word_t)0x2000)
#define AGC_NEG_HALF ((agc_word_t)-0x4000)

/* Single precision sin/cos polynomial coefficients (scaled at 1) */
/* C1/2 = 0.7853134 decimal */
/* C3/2 = -0.3216147 decimal */
/* C5/2 = 0.0363551 decimal */
#define SP_C1_2  ((agc_word_t)0x6488)  /* octal 31103 -> C1/2 */
#define SP_C3_2  ((agc_word_t)-0x2969) /* octal 65552 (neg) -> C3/2 */
#define SP_C5_2  ((agc_word_t)0x04A9)  /* octal 01124 -> C5/2 */

/* ----------------------------------------------------------------
 * 1's complement arithmetic macros
 * ----------------------------------------------------------------
 *
 * The AGC uses 1's complement where:
 *   +0 = 0x0000, -0 = 0x7FFF (all 15 data bits set, sign=0 in our model)
 *
 * We model AGC words in 2's complement C, mapping:
 *   AGC +0 = C 0
 *   AGC -0 = C -0 (which is just 0 in C, but we track it separately)
 *   AGC positive: 1..16383 (0x0001..0x3FFF)
 *   AGC negative: -1..-16383
 *
 * For bit-accurate fidelity, we use a 16-bit range to allow overflow:
 *   positive overflow: +16384 (one above POSMAX)
 *   negative overflow: -16384 (one below NEGMAX)
 */

/* Clamp/overflow correction: AGC overflow wraps around through +-0 */
static agc_word_t agc_overflow_correct(int val)
{
    while (val > 16383)  val -= 32767;
    while (val < -16383) val += 32767;
    return (agc_word_t)val;
}

/* 1's complement addition: a + b with overflow correction */
static agc_word_t agc_add(agc_word_t a, agc_word_t b)
{
    int sum = (int)a + (int)b;
    return agc_overflow_correct(sum);
}

/* 1's complement negation (complement): -0 maps to +0 and vice versa */
static agc_word_t agc_negate(agc_word_t val)
{
    return (agc_word_t)(-(int)val);
}

/* Absolute value for AGC 1's complement */
static agc_word_t agc_abs(agc_word_t val)
{
    return (val < 0) ? agc_negate(val) : val;
}

/* Diminished absolute value (CCS behavior):
 * if val > 0, return val-1
 * if val == +0, return 0
 * if val < 0, return |val|-1
 * if val == -0, return 0 */
static agc_word_t agc_dabs(agc_word_t val)
{
    if (val > 0) return (agc_word_t)(val - 1);
    if (val < 0) return (agc_word_t)(-(int)val - 1);
    return 0;
}

/* CCS 4-way branch index:
 * val > 0: return 0
 * val == +0: return 1
 * val < 0: return 2
 * val == -0: return 3
 * (In C, we can't distinguish +0/-0 in 2's complement, so -0 path unused) */
static int agc_ccs_branch(agc_word_t val)
{
    if (val > 0) return 0;
    if (val == 0) return 1;  /* +0 */
    return 2;                /* < 0 */
}

/* Sign test */
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

/* Number of core sets (job slots) in the Executive */
#define NUM_CORE_SETS   7
/* Number of VAC areas */
#define NUM_VAC_AREAS   5
/* VAC area size (words) */
#define VAC_AREA_SIZE  43

/* Waitlist task slots */
#define NUM_WAITLIST_TASKS 9

/* Number of flagwords */
#define NUM_FLAGWORDS  12

/* Maximum number of phase table entries */
#define NUM_PHASES      6

/* Function pointer types for jobs and tasks */
typedef void (*agc_jobfunc_t)(void);
typedef void (*agc_taskfunc_t)(void);

#endif /* AGC_H */
