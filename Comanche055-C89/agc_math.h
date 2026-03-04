/*
 * agc_math.h -- Fixed-point arithmetic: SP, DP, vector, matrix, trig.
 *
 * All operations replicate AGC bit-accurate scaling conventions.
 * SP values are 15-bit fractions scaled at 1 (value/16384).
 * DP values are 30-bit (high word + low word), stored either as
 * two agc_word_t or as agc_dp_t (int) intermediates.
 * Vectors are 3D with 6 words (Xhi,Xlo, Yhi,Ylo, Zhi,Zlo).
 * Matrices are 3x3 in row-major order (18 words).
 *
 * Maps to SINGLE_PRECISION_SUBROUTINES.agc and the interpreter
 * math routines.
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

#ifndef AGC_MATH_H
#define AGC_MATH_H

#include "agc.h"

/* ----------------------------------------------------------------
 * Single precision (15-bit + sign)
 * ---------------------------------------------------------------- */

agc_word_t agc_sp_multiply(agc_word_t a, agc_word_t b);
agc_word_t agc_sp_sin(agc_word_t angle);
agc_word_t agc_sp_cos(agc_word_t angle);

/* ----------------------------------------------------------------
 * Double precision (30-bit: high word + low word)
 * ---------------------------------------------------------------- */

agc_dp_t agc_dp_pack(agc_word_t high, agc_word_t low);
void agc_dp_unpack(agc_dp_t val, agc_word_t *high, agc_word_t *low);

agc_dp_t agc_dp_add(agc_dp_t a, agc_dp_t b);
agc_dp_t agc_dp_sub(agc_dp_t a, agc_dp_t b);
agc_dp_t agc_dp_multiply(agc_dp_t a, agc_dp_t b);
agc_dp_t agc_dp_divide(agc_dp_t a, agc_dp_t b);
agc_dp_t agc_dp_abs(agc_dp_t val);
agc_dp_t agc_dp_negate(agc_dp_t val);
int agc_dp_sign(agc_dp_t val);
agc_dp_t agc_dp_sqrt(agc_dp_t val);

agc_dp_t agc_dp_sin(agc_dp_t angle);
agc_dp_t agc_dp_cos(agc_dp_t angle);
agc_dp_t agc_dp_asin(agc_dp_t val);
agc_dp_t agc_dp_acos(agc_dp_t val);
agc_dp_t agc_dp_atan2(agc_dp_t y, agc_dp_t x);

/* ----------------------------------------------------------------
 * Vector operations (3D, 6 words)
 * ---------------------------------------------------------------- */

void agc_vec_add(const agc_word_t *a, const agc_word_t *b,
                 agc_word_t *result);
void agc_vec_sub(const agc_word_t *a, const agc_word_t *b,
                 agc_word_t *result);
void agc_vec_cross(const agc_word_t *a, const agc_word_t *b,
                   agc_word_t *result);
agc_dp_t agc_vec_dot(const agc_word_t *a, const agc_word_t *b);
agc_dp_t agc_vec_unit(const agc_word_t *a, agc_word_t *result);
agc_dp_t agc_vec_mag(const agc_word_t *a);
void agc_vec_scale(agc_dp_t scalar, const agc_word_t *vec,
                   agc_word_t *result);

/* ----------------------------------------------------------------
 * Matrix operations (3x3, 18 words, row-major)
 * ---------------------------------------------------------------- */

void agc_mat_vec_mul(const agc_word_t *mat, const agc_word_t *vec,
                     agc_word_t *result);
void agc_vec_mat_mul(const agc_word_t *vec, const agc_word_t *mat,
                     agc_word_t *result);

/* ----------------------------------------------------------------
 * Utility
 * ---------------------------------------------------------------- */

int agc_dp_to_display(agc_dp_t val, int scale_exp);
agc_dp_t agc_degrees_to_angle(int degrees);

#endif /* AGC_MATH_H */
