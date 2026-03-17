/*
 * agc_math.c -- Fixed-point arithmetic: SP, DP, vector, matrix, trig.
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

#include "agc.h"
#include "agc_math.h"

/* ----------------------------------------------------------------
 * Single precision
 * ---------------------------------------------------------------- */

agc_word_t
agc_sp_multiply(agc_word_t a, agc_word_t b)
{
  /* (a * b) >> 14: upper 15 bits of 30-bit product */
  int product = (int)a * (int)b;
  return (agc_word_t)(product >> 14);
}

agc_word_t
agc_sp_sin(agc_word_t angle)
{
  /*
   * Replicates SPSIN from SINGLE_PRECISION_SUBROUTINES.agc.
   * Argument scaled at pi (semicircles), result scaled at 1.
   * Algorithm: reduce to [-0.5, 0.5], evaluate polynomial
   *   result = TEMK * (C1/2 + SQ*(C3/2 + SQ*C5/2))
   * then DDOUBL the result.
   */
  int temk, sq, acc;

  temk = (int)angle;

  /* TS TEMK -- overflow correction */
  if (temk > 16383) {
    temk = temk - 32767;
  } else if (temk < -16383) {
    temk = temk + 32767;
  }

  /* DOUBLE */
  temk = temk * 2;
  if (temk > 16383) {
    temk = temk - 32767;
    temk = 32767 - temk;
    if (temk > 16383) {
      return (angle >= 0) ? (agc_word_t)16383 : (agc_word_t)-16383;
    }
  } else if (temk < -16383) {
    temk = temk + 32767;
    temk = -32767 - temk;
    if (temk < -16383) {
      return (angle < 0) ? (agc_word_t)16383 : (agc_word_t)-16383;
    }
  }

  /* POLLEY: SQ = TEMK^2 >> 14 */
  sq = ((int)temk * (int)temk) >> 14;

  acc = (int)SP_C5_2;
  acc = (sq * acc) >> 14;
  acc = acc + (int)SP_C3_2;
  acc = (sq * acc) >> 14;
  acc = acc + (int)SP_C1_2;
  acc = ((int)temk * acc) >> 14;

  /* DDOUBL */
  acc = acc * 2;

  return agc_overflow_correct(acc);
}

agc_word_t
agc_sp_cos(agc_word_t angle)
{
  /* SPCOS: add HALF (pi/2 in semicircles) then call SPSIN */
  int shifted = (int)angle + (int)AGC_HALF;
  return agc_sp_sin(agc_overflow_correct(shifted));
}

/* ----------------------------------------------------------------
 * Double precision
 * ---------------------------------------------------------------- */

agc_dp_t
agc_dp_pack(agc_word_t high, agc_word_t low)
{
  agc_dp_t result = (agc_dp_t)high * 16384;
  if (high >= 0) {
    result += (agc_dp_t)(low & 0x3FFF);
  } else {
    result -= (agc_dp_t)((-low) & 0x3FFF);
  }
  return result;
}

void
agc_dp_unpack(agc_dp_t val, agc_word_t* high, agc_word_t* low)
{
  if (val >= 0) {
    *high = (agc_word_t)(val / 16384);
    *low = (agc_word_t)(val % 16384);
  } else {
    agc_dp_t pos = -val;
    *high = (agc_word_t) - (pos / 16384);
    *low = (agc_word_t) - (pos % 16384);
  }
}

agc_dp_t
agc_dp_add(agc_dp_t a, agc_dp_t b)
{
  return a + b;
}

agc_dp_t
agc_dp_sub(agc_dp_t a, agc_dp_t b)
{
  return a - b;
}

agc_dp_t
agc_dp_multiply(agc_dp_t a, agc_dp_t b)
{
  agc_int64_t product = (agc_int64_t)a * (agc_int64_t)b;
  product >>= 14;
  if (product > 0x1FFFFFFF) {
    product = 0x1FFFFFFF;
  }
  if (product < -0x1FFFFFFF) {
    product = -0x1FFFFFFF;
  }
  return (agc_dp_t)product;
}

agc_dp_t
agc_dp_divide(agc_dp_t a, agc_dp_t b)
{
  agc_int64_t al, result;
  if (b == 0) {
    return (a >= 0) ? 0x1FFFFFFF : -0x1FFFFFFF;
  }
  al = (agc_int64_t)a << 14;
  result = al / (agc_int64_t)b;
  if (result > 0x1FFFFFFF) {
    result = 0x1FFFFFFF;
  }
  if (result < -0x1FFFFFFF) {
    result = -0x1FFFFFFF;
  }
  return (agc_dp_t)result;
}

agc_dp_t
agc_dp_abs(agc_dp_t val)
{
  return (val < 0) ? -val : val;
}

agc_dp_t
agc_dp_negate(agc_dp_t val)
{
  return -val;
}

int
agc_dp_sign(agc_dp_t val)
{
  if (val > 0) {
    return 1;
  }
  if (val < 0) {
    return -1;
  }
  return 0;
}

agc_dp_t
agc_dp_sqrt(agc_dp_t val)
{
  /* Newton's method for fixed-point sqrt */
  agc_dp_t x, prev;
  int i;

  if (val <= 0) {
    return 0;
  }

  x = val >> 1;
  if (x == 0) {
    x = 1;
  }

  for (i = 0; i < 20; i++) {
    agc_int64_t div_result;
    if (x == 0) {
      break;
    }
    div_result = ((agc_int64_t)val << 14) / (agc_int64_t)x;
    prev = x;
    x = (agc_dp_t)(((agc_int64_t)x + div_result) >> 1);
    if (x == prev) {
      break;
    }
  }
  return x;
}

agc_dp_t
agc_dp_sin(agc_dp_t angle)
{
  /* DP sine using SP sine on high word with DP correction */
  agc_word_t high, low;
  agc_word_t sin_h, cos_h;
  agc_dp_t result;

  agc_dp_unpack(angle, &high, &low);
  sin_h = agc_sp_sin(high);
  cos_h = agc_sp_cos(high);

  /* sin(h+l) ~ sin(h) + l*cos(h) */
  result = agc_dp_pack(sin_h, 0);
  result += ((agc_dp_t)low * (agc_dp_t)cos_h) >> 14;

  return result;
}

agc_dp_t
agc_dp_cos(agc_dp_t angle)
{
  return agc_dp_sin(angle + agc_dp_pack(AGC_HALF, 0));
}

agc_dp_t
agc_dp_asin(agc_dp_t val)
{
  /* Newton iteration: solve sin(x) = val for x */
  agc_dp_t x, sinx, cosx, dx;
  int i;

  if (val >= (agc_dp_t)16383 * 16384) {
    return agc_dp_pack(AGC_HALF, 0);
  }
  if (val <= -(agc_dp_t)16383 * 16384) {
    return agc_dp_pack(AGC_NEG_HALF, 0);
  }

  x = val >> 1;

  for (i = 0; i < 15; i++) {
    sinx = agc_dp_sin(x);
    cosx = agc_dp_cos(x);
    if (cosx == 0) {
      break;
    }
    dx = agc_dp_divide(val - sinx, cosx);
    dx = dx >> 1;
    x = x + dx;
    if (agc_dp_abs(dx) < 2) {
      break;
    }
  }
  return x;
}

agc_dp_t
agc_dp_acos(agc_dp_t val)
{
  return agc_dp_pack(AGC_HALF, 0) - agc_dp_asin(val);
}

agc_dp_t
agc_dp_atan2(agc_dp_t y, agc_dp_t x)
{
  agc_dp_t mag, ratio, result;

  if (x == 0 && y == 0) {
    return 0;
  }

  /* asin(y / sqrt(x^2 + y^2)) */
  mag = agc_dp_sqrt(agc_dp_multiply(x, x) + agc_dp_multiply(y, y));
  if (mag == 0) {
    return 0;
  }
  ratio = agc_dp_divide(y, mag);
  result = agc_dp_asin(ratio);

  /* Quadrant correction */
  if (x < 0) {
    if (y >= 0) {
      result = agc_dp_pack((agc_word_t)16383, 0) - result;
    } else {
      result = -agc_dp_pack((agc_word_t)16383, 0) - result;
    }
  }
  return result;
}

/* ----------------------------------------------------------------
 * Vector operations (3D, 6 words: Xhi,Xlo, Yhi,Ylo, Zhi,Zlo)
 * ---------------------------------------------------------------- */

void
agc_vec_add(const agc_word_t* a, const agc_word_t* b, agc_word_t* result)
{
  int i;
  for (i = 0; i < 3; i++) {
    agc_dp_t va = agc_dp_pack(a[i * 2], a[i * 2 + 1]);
    agc_dp_t vb = agc_dp_pack(b[i * 2], b[i * 2 + 1]);
    agc_dp_t vr = va + vb;
    agc_dp_unpack(vr, &result[i * 2], &result[i * 2 + 1]);
  }
}

void
agc_vec_sub(const agc_word_t* a, const agc_word_t* b, agc_word_t* result)
{
  int i;
  for (i = 0; i < 3; i++) {
    agc_dp_t va = agc_dp_pack(a[i * 2], a[i * 2 + 1]);
    agc_dp_t vb = agc_dp_pack(b[i * 2], b[i * 2 + 1]);
    agc_dp_t vr = va - vb;
    agc_dp_unpack(vr, &result[i * 2], &result[i * 2 + 1]);
  }
}

void
agc_vec_cross(const agc_word_t* a, const agc_word_t* b, agc_word_t* result)
{
  agc_dp_t ax = agc_dp_pack(a[0], a[1]);
  agc_dp_t ay = agc_dp_pack(a[2], a[3]);
  agc_dp_t az = agc_dp_pack(a[4], a[5]);
  agc_dp_t bx = agc_dp_pack(b[0], b[1]);
  agc_dp_t by = agc_dp_pack(b[2], b[3]);
  agc_dp_t bz = agc_dp_pack(b[4], b[5]);
  agc_dp_t rx, ry, rz;

  rx = agc_dp_sub(agc_dp_multiply(ay, bz), agc_dp_multiply(az, by));
  ry = agc_dp_sub(agc_dp_multiply(az, bx), agc_dp_multiply(ax, bz));
  rz = agc_dp_sub(agc_dp_multiply(ax, by), agc_dp_multiply(ay, bx));

  agc_dp_unpack(rx, &result[0], &result[1]);
  agc_dp_unpack(ry, &result[2], &result[3]);
  agc_dp_unpack(rz, &result[4], &result[5]);
}

agc_dp_t
agc_vec_dot(const agc_word_t* a, const agc_word_t* b)
{
  agc_dp_t sum = 0;
  int i;
  for (i = 0; i < 3; i++) {
    agc_dp_t va = agc_dp_pack(a[i * 2], a[i * 2 + 1]);
    agc_dp_t vb = agc_dp_pack(b[i * 2], b[i * 2 + 1]);
    sum += agc_dp_multiply(va, vb);
  }
  return sum;
}

agc_dp_t
agc_vec_mag(const agc_word_t* a)
{
  agc_dp_t sum_sq = 0;
  int i;
  for (i = 0; i < 3; i++) {
    agc_dp_t v = agc_dp_pack(a[i * 2], a[i * 2 + 1]);
    sum_sq += agc_dp_multiply(v, v);
  }
  return agc_dp_sqrt(sum_sq);
}

agc_dp_t
agc_vec_unit(const agc_word_t* a, agc_word_t* result)
{
  agc_dp_t mag = agc_vec_mag(a);
  int i;
  if (mag == 0) {
    for (i = 0; i < 6; i++) {
      result[i] = 0;
    }
    return 0;
  }
  for (i = 0; i < 3; i++) {
    agc_dp_t v = agc_dp_pack(a[i * 2], a[i * 2 + 1]);
    agc_dp_t u = agc_dp_divide(v, mag);
    agc_dp_unpack(u, &result[i * 2], &result[i * 2 + 1]);
  }
  return mag;
}

void
agc_vec_scale(agc_dp_t scalar, const agc_word_t* vec, agc_word_t* result)
{
  int i;
  for (i = 0; i < 3; i++) {
    agc_dp_t v = agc_dp_pack(vec[i * 2], vec[i * 2 + 1]);
    agc_dp_t r = agc_dp_multiply(scalar, v);
    agc_dp_unpack(r, &result[i * 2], &result[i * 2 + 1]);
  }
}

/* ----------------------------------------------------------------
 * Matrix operations (3x3, 18 words, row-major, each element DP)
 * ---------------------------------------------------------------- */

void
agc_mat_vec_mul(const agc_word_t* mat,
                const agc_word_t* vec,
                agc_word_t* result)
{
  int row;
  for (row = 0; row < 3; row++) {
    agc_dp_t sum = 0;
    int col;
    for (col = 0; col < 3; col++) {
      agc_dp_t m =
        agc_dp_pack(mat[(row * 3 + col) * 2], mat[(row * 3 + col) * 2 + 1]);
      agc_dp_t v = agc_dp_pack(vec[col * 2], vec[col * 2 + 1]);
      sum += agc_dp_multiply(m, v);
    }
    agc_dp_unpack(sum, &result[row * 2], &result[row * 2 + 1]);
  }
}

void
agc_vec_mat_mul(const agc_word_t* vec,
                const agc_word_t* mat,
                agc_word_t* result)
{
  int col;
  for (col = 0; col < 3; col++) {
    agc_dp_t sum = 0;
    int row;
    for (row = 0; row < 3; row++) {
      agc_dp_t v = agc_dp_pack(vec[row * 2], vec[row * 2 + 1]);
      agc_dp_t m =
        agc_dp_pack(mat[(row * 3 + col) * 2], mat[(row * 3 + col) * 2 + 1]);
      sum += agc_dp_multiply(v, m);
    }
    agc_dp_unpack(sum, &result[col * 2], &result[col * 2 + 1]);
  }
}

/* ----------------------------------------------------------------
 * Utility
 * ---------------------------------------------------------------- */

int
agc_dp_to_display(agc_dp_t val, int scale_exp)
{
  long scaled;
  if (scale_exp >= 0) {
    scaled = (long)val << scale_exp;
  } else {
    scaled = (long)val >> (-scale_exp);
  }
  return (int)(scaled >> 14);
}

agc_dp_t
agc_degrees_to_angle(int degrees)
{
  /* 180 degrees = 1.0 semicircles = 16384 in DP high word */
  long val = (long)degrees * 16384L;
  return (agc_dp_t)(val / 180);
}
