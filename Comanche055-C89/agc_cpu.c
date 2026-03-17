/*
 * agc_cpu.c -- AGC hardware state and 1's complement arithmetic.
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

#include "agc.h"
#include "agc_cpu.h"

#include <string.h>

/* ----------------------------------------------------------------
 * AGC global state
 * ---------------------------------------------------------------- */

agc_word_t agc_erasable[NUM_EBANKS][EBANK_SIZE];
agc_word_t agc_channels[NUM_CHANNELS];

int agc_ebank = 0;

agc_word_t agc_time1 = 0;
agc_word_t agc_time2 = 0;
agc_word_t agc_time3 = 0;
agc_word_t agc_time4 = 0;
agc_word_t agc_time5 = 0;
agc_word_t agc_time6 = 0;

int agc_inhint = 0;
agc_word_t agc_flagwords[NUM_FLAGWORDS];
int agc_current_program = 0;

/* ----------------------------------------------------------------
 * Erasable memory access
 * ---------------------------------------------------------------- */

agc_word_t
agc_read_erasable(int ebank, int addr)
{
  if (ebank < 0 || ebank >= NUM_EBANKS) {
    return 0;
  }
  if (addr < 0 || addr >= EBANK_SIZE) {
    return 0;
  }
  return agc_erasable[ebank][addr];
}

void
agc_write_erasable(int ebank, int addr, agc_word_t val)
{
  if (ebank < 0 || ebank >= NUM_EBANKS) {
    return;
  }
  if (addr < 0 || addr >= EBANK_SIZE) {
    return;
  }
  agc_erasable[ebank][addr] = val;
}

/* ----------------------------------------------------------------
 * I/O channel access
 * ---------------------------------------------------------------- */

agc_word_t
agc_read_channel(int chan)
{
  if (chan < 0 || chan >= NUM_CHANNELS) {
    return 0;
  }
  return agc_channels[chan];
}

void
agc_write_channel(int chan, agc_word_t val)
{
  if (chan < 0 || chan >= NUM_CHANNELS) {
    return;
  }
  agc_channels[chan] = val;
}

/* ----------------------------------------------------------------
 * Flag operations
 * ---------------------------------------------------------------- */

void
agc_flag_set(int flagword, agc_word_t bitmask)
{
  if (flagword < 0 || flagword >= NUM_FLAGWORDS) {
    return;
  }
  agc_flagwords[flagword] |= bitmask;
}

void
agc_flag_clear(int flagword, agc_word_t bitmask)
{
  if (flagword < 0 || flagword >= NUM_FLAGWORDS) {
    return;
  }
  agc_flagwords[flagword] &= ~bitmask;
}

int
agc_flag_test(int flagword, agc_word_t bitmask)
{
  if (flagword < 0 || flagword >= NUM_FLAGWORDS) {
    return 0;
  }
  return (agc_flagwords[flagword] & bitmask) != 0;
}

/* ----------------------------------------------------------------
 * Initialization
 * ---------------------------------------------------------------- */

void
agc_init(void)
{
  memset(agc_erasable, 0, sizeof(agc_erasable));
  memset(agc_channels, 0, sizeof(agc_channels));
  memset(agc_flagwords, 0, sizeof(agc_flagwords));

  agc_ebank = 0;
  agc_time1 = 0;
  agc_time2 = 0;
  agc_time3 = 0;
  agc_time4 = 0;
  agc_time5 = 0;
  agc_time6 = 0;
  agc_inhint = 0;
  agc_current_program = 0;

  /* Initial channel values (no warnings/standby active) */
  agc_channels[CHAN_CHAN30] = 037777;
  agc_channels[CHAN_CHAN31] = 037777;
  agc_channels[CHAN_CHAN32] = 037777;
  agc_channels[CHAN_CHAN33] = 037777;
}

/* ----------------------------------------------------------------
 * 1's complement arithmetic
 * ---------------------------------------------------------------- */

agc_word_t
agc_overflow_correct(int val)
{
  while (val > 16383) {
    val -= 32767;
  }
  while (val < -16383) {
    val += 32767;
  }
  return (agc_word_t)val;
}

agc_word_t
agc_add(agc_word_t a, agc_word_t b)
{
  int sum = (int)a + (int)b;
  return agc_overflow_correct(sum);
}

agc_word_t
agc_negate(agc_word_t val)
{
  return (agc_word_t)(-(int)val);
}

agc_word_t
agc_abs(agc_word_t val)
{
  return (val < 0) ? agc_negate(val) : val;
}

agc_word_t
agc_dabs(agc_word_t val)
{
  if (val > 0) {
    return (agc_word_t)(val - 1);
  }
  if (val < 0) {
    return (agc_word_t)(-(int)val - 1);
  }
  return 0;
}

int
agc_ccs_branch(agc_word_t val)
{
  if (val > 0) {
    return 0;
  }
  if (val == 0) {
    return 1;
  }
  return 2;
}
