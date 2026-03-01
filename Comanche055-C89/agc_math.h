/*
 * agc_math.h -- Fixed-point arithmetic: SP, DP, vector, matrix, trig
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port
 * All operations replicate AGC bit-accurate scaling
 */

#ifndef AGC_MATH_H
#define AGC_MATH_H

#include "agc.h"

/* ----------------------------------------------------------------
 * Single precision (15-bit + sign)
 * ---------------------------------------------------------------- */

/* Multiply: (a * b) >> 14, upper 15 bits of 30-bit product */
agc_word_t agc_sp_multiply(agc_word_t a, agc_word_t b);

/* Single precision sine: argument scaled at pi, result scaled at 1 */
agc_word_t agc_sp_sin(agc_word_t angle);

/* Single precision cosine: argument scaled at pi, result scaled at 1 */
agc_word_t agc_sp_cos(agc_word_t angle);

/* ----------------------------------------------------------------
 * Double precision (30-bit: high word + low word)
 * ----------------------------------------------------------------
 * DP values are stored as two consecutive agc_word_t:
 *   word[0] = high word (sign + upper 14 bits)
 *   word[1] = low word (lower 14 bits, sign matches high)
 *
 * We also use agc_dp_t (int) for intermediate calculations.
 */

/* Pack two AGC words into a DP integer */
agc_dp_t agc_dp_pack(agc_word_t high, agc_word_t low);

/* Unpack a DP integer into two AGC words */
void agc_dp_unpack(agc_dp_t val, agc_word_t *high, agc_word_t *low);

/* DP addition */
agc_dp_t agc_dp_add(agc_dp_t a, agc_dp_t b);

/* DP subtraction */
agc_dp_t agc_dp_sub(agc_dp_t a, agc_dp_t b);

/* DP multiply: (a * b) >> 14, result is DP */
agc_dp_t agc_dp_multiply(agc_dp_t a, agc_dp_t b);

/* DP divide: (a << 14) / b, result is DP */
agc_dp_t agc_dp_divide(agc_dp_t a, agc_dp_t b);

/* DP absolute value */
agc_dp_t agc_dp_abs(agc_dp_t val);

/* DP negate */
agc_dp_t agc_dp_negate(agc_dp_t val);

/* DP sign: returns 1, 0, or -1 */
int agc_dp_sign(agc_dp_t val);

/* DP square root: result scaled so that sqrt(x) where x in [0,1) -> [0,1) */
agc_dp_t agc_dp_sqrt(agc_dp_t val);

/* DP sine: argument scaled at pi (semicircles), result scaled at 1 */
agc_dp_t agc_dp_sin(agc_dp_t angle);

/* DP cosine: argument scaled at pi, result scaled at 1 */
agc_dp_t agc_dp_cos(agc_dp_t angle);

/* DP arcsine: argument scaled at 1, result scaled at pi */
agc_dp_t agc_dp_asin(agc_dp_t val);

/* DP arccosine: argument scaled at 1, result scaled at pi */
agc_dp_t agc_dp_acos(agc_dp_t val);

/* DP arctan2: atan2(y, x), both scaled at 1, result scaled at pi */
agc_dp_t agc_dp_atan2(agc_dp_t y, agc_dp_t x);

/* ----------------------------------------------------------------
 * Vector operations (3D, 6 words: X_hi,X_lo, Y_hi,Y_lo, Z_hi,Z_lo)
 * ---------------------------------------------------------------- */

/* Vector add: result = a + b */
void agc_vec_add(const agc_word_t *a, const agc_word_t *b, agc_word_t *result);

/* Vector subtract: result = a - b */
void agc_vec_sub(const agc_word_t *a, const agc_word_t *b, agc_word_t *result);

/* Vector cross product: result = a x b */
void agc_vec_cross(const agc_word_t *a, const agc_word_t *b, agc_word_t *result);

/* Dot product: returns DP scalar */
agc_dp_t agc_vec_dot(const agc_word_t *a, const agc_word_t *b);

/* Unit vector: result = a / |a|, returns magnitude as DP */
agc_dp_t agc_vec_unit(const agc_word_t *a, agc_word_t *result);

/* Vector magnitude (absolute value): returns DP */
agc_dp_t agc_vec_mag(const agc_word_t *a);

/* Scalar-vector multiply: result = scalar * vec */
void agc_vec_scale(agc_dp_t scalar, const agc_word_t *vec, agc_word_t *result);

/* ----------------------------------------------------------------
 * Matrix operations (3x3, 18 words in row-major order)
 * ---------------------------------------------------------------- */

/* Matrix-vector multiply: result = M * v */
void agc_mat_vec_mul(const agc_word_t *mat, const agc_word_t *vec,
                     agc_word_t *result);

/* Vector-matrix multiply: result = v * M (transpose multiply) */
void agc_vec_mat_mul(const agc_word_t *vec, const agc_word_t *mat,
                     agc_word_t *result);

/* ----------------------------------------------------------------
 * Utility
 * ---------------------------------------------------------------- */

/* Convert DP to a decimal value for display (returns integer scaled per noun) */
int agc_dp_to_display(agc_dp_t val, int scale_exp);

/* Convert degrees (integer) to AGC angle (scaled at pi) */
agc_dp_t agc_degrees_to_angle(int degrees);

#endif /* AGC_MATH_H */
