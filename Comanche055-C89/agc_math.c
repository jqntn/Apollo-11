/*
 * agc_math.c -- Fixed-point arithmetic: SP, DP, vector, matrix, trig
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port
 * All operations replicate AGC bit-accurate scaling
 */

#include "agc_math.h"

/* ----------------------------------------------------------------
 * Single precision
 * ---------------------------------------------------------------- */

agc_word_t agc_sp_multiply(agc_word_t a, agc_word_t b)
{
    /* AGC SP multiply: 15-bit x 15-bit, return upper 15 bits of 30-bit product
     * Both operands scaled at 1 (i.e., value/16384)
     * Product: (a * b) / 16384, effectively >> 14 */
    int product = (int)a * (int)b;
    return (agc_word_t)(product >> 14);
}

agc_word_t agc_sp_sin(agc_word_t angle)
{
    /* Replicate AGC SPSIN from SINGLE_PRECISION_SUBROUTINES.agc
     * Argument scaled at pi (semicircles): 0x4000 = pi
     * Result scaled at 1
     *
     * Algorithm:
     *   1. Reduce angle to range [-0.5, 0.5] (in semicircles)
     *   2. Evaluate polynomial: result = TEMK * (C1/2 + SQ*(C3/2 + SQ*C5/2))
     *      where SQ = TEMK^2
     *   3. Double the result (DDOUBL) */

    int temk, sq, acc;

    /* Reduce: bring into first period */
    temk = (int)angle;
    /* TS TEMK -- if overflow, negate */
    if (temk > 16383) {
        temk = temk - 32767;
    } else if (temk < -16383) {
        temk = temk + 32767;
    }

    /* DOUBLE */
    temk = temk * 2;
    if (temk > 16383) {
        /* Overflow: angle is in second quadrant */
        temk = temk - 32767;
        /* XCH TEMK; INDEX TEMK; AD LIMITS; COM; AD TEMK */
        /* This maps the angle for quadrant correction */
        temk = 32767 - temk;
        if (temk > 16383) {
            /* ARG90: return +-1 */
            return (angle >= 0) ? (agc_word_t)16383 : (agc_word_t)-16383;
        }
    } else if (temk < -16383) {
        temk = temk + 32767;
        temk = -32767 - temk;
        if (temk < -16383) {
            return (angle < 0) ? (agc_word_t)16383 : (agc_word_t)-16383;
        }
    }

    /* POLLEY: evaluate polynomial
     * SQ = TEMK * TEMK >> 14 */
    sq = ((int)temk * (int)temk) >> 14;

    /* acc = C5/2 */
    acc = (int)SP_C5_2;
    /* acc = SQ * C5/2 >> 14, then + C3/2 */
    acc = (sq * acc) >> 14;
    acc = acc + (int)SP_C3_2;
    /* acc = SQ * (C3/2 + SQ*C5/2) >> 14, then + C1/2 */
    acc = (sq * acc) >> 14;
    acc = acc + (int)SP_C1_2;
    /* acc = TEMK * polynomial >> 14 */
    acc = ((int)temk * acc) >> 14;
    /* DDOUBL: double the result */
    acc = acc * 2;

    return agc_overflow_correct(acc);
}

agc_word_t agc_sp_cos(agc_word_t angle)
{
    /* SPCOS: add HALF (pi/2 in semicircles) then call SPSIN */
    int shifted = (int)angle + (int)AGC_HALF;
    return agc_sp_sin(agc_overflow_correct(shifted));
}

/* ----------------------------------------------------------------
 * Double precision
 * ---------------------------------------------------------------- */

agc_dp_t agc_dp_pack(agc_word_t high, agc_word_t low)
{
    /* Pack: DP = high * 16384 + (low & 0x3FFF)
     * Low word carries lower 14 bits, sign matches high */
    agc_dp_t result = (agc_dp_t)high * 16384;
    if (high >= 0) {
        result += (agc_dp_t)(low & 0x3FFF);
    } else {
        result -= (agc_dp_t)((-low) & 0x3FFF);
    }
    return result;
}

void agc_dp_unpack(agc_dp_t val, agc_word_t *high, agc_word_t *low)
{
    /* Unpack DP into high and low words */
    if (val >= 0) {
        *high = (agc_word_t)(val / 16384);
        *low  = (agc_word_t)(val % 16384);
    } else {
        agc_dp_t pos = -val;
        *high = (agc_word_t)-(pos / 16384);
        *low  = (agc_word_t)-(pos % 16384);
    }
}

agc_dp_t agc_dp_add(agc_dp_t a, agc_dp_t b)
{
    return a + b;
}

agc_dp_t agc_dp_sub(agc_dp_t a, agc_dp_t b)
{
    return a - b;
}

agc_dp_t agc_dp_multiply(agc_dp_t a, agc_dp_t b)
{
    /* DP multiply: (a * b) >> 14
     * Both operands scaled at 1 in 28-bit fraction
     * Use 64-bit intermediate to avoid overflow */
    long al = (long)a;
    long bl = (long)b;
    long product = (al * bl) >> 14;
    /* Clamp to 30-bit range */
    if (product > 0x1FFFFFFFL) product = 0x1FFFFFFFL;
    if (product < -0x1FFFFFFFL) product = -0x1FFFFFFFL;
    return (agc_dp_t)product;
}

agc_dp_t agc_dp_divide(agc_dp_t a, agc_dp_t b)
{
    /* DP divide: (a << 14) / b */
    long al, result;
    if (b == 0) return (a >= 0) ? 0x1FFFFFFF : -0x1FFFFFFF;
    al = (long)a << 14;
    result = al / (long)b;
    if (result > 0x1FFFFFFFL) result = 0x1FFFFFFFL;
    if (result < -0x1FFFFFFFL) result = -0x1FFFFFFFL;
    return (agc_dp_t)result;
}

agc_dp_t agc_dp_abs(agc_dp_t val)
{
    return (val < 0) ? -val : val;
}

agc_dp_t agc_dp_negate(agc_dp_t val)
{
    return -val;
}

int agc_dp_sign(agc_dp_t val)
{
    if (val > 0) return 1;
    if (val < 0) return -1;
    return 0;
}

agc_dp_t agc_dp_sqrt(agc_dp_t val)
{
    /* Newton's method for fixed-point square root
     * Input and output scaled at 1 (i.e., val in [0, 1) as 28-bit fraction)
     * sqrt(x) where x = val/2^28, result = sqrt(x) * 2^14 */
    agc_dp_t x, prev;
    int i;

    if (val <= 0) return 0;

    /* Initial guess: half the value or a reasonable starting point */
    x = val >> 1;
    if (x == 0) x = 1;

    for (i = 0; i < 20; i++) {
        /* x_new = (x + val/x) / 2
         * In fixed point: x_new = (x + (val << 14) / x) / 2 */
        long div_result;
        if (x == 0) break;
        div_result = ((long)val << 14) / (long)x;
        prev = x;
        x = (agc_dp_t)((long)x + div_result) >> 1;
        if (x == prev) break;
    }
    return x;
}

agc_dp_t agc_dp_sin(agc_dp_t angle)
{
    /* DP sine using SP sine on high word, with DP correction
     * Argument scaled at pi (semicircles), result scaled at 1 */
    agc_word_t high, low;
    agc_word_t sin_h, cos_h;
    agc_dp_t result;

    agc_dp_unpack(angle, &high, &low);
    sin_h = agc_sp_sin(high);
    cos_h = agc_sp_cos(high);

    /* DP correction: sin(h+l) ~ sin(h) + l*cos(h)
     * where l is the low-order correction scaled at pi */
    result = agc_dp_pack(sin_h, 0);
    result += ((agc_dp_t)low * (agc_dp_t)cos_h) >> 14;

    return result;
}

agc_dp_t agc_dp_cos(agc_dp_t angle)
{
    /* cos(x) = sin(x + pi/2) */
    return agc_dp_sin(angle + agc_dp_pack(AGC_HALF, 0));
}

agc_dp_t agc_dp_asin(agc_dp_t val)
{
    /* Arcsine via Newton iteration or polynomial
     * Input scaled at 1, output scaled at pi
     * Use iterative approach: start from sin(x)=val, solve for x */
    agc_dp_t x, sinx, cosx, dx;
    int i;

    if (val >= (agc_dp_t)16383 * 16384) return agc_dp_pack(AGC_HALF, 0);
    if (val <= -(agc_dp_t)16383 * 16384) return agc_dp_pack(AGC_NEG_HALF, 0);

    /* Initial guess: x = val (small angle approx, val ~= sin(val) for small) */
    x = val >> 1;  /* rough initial guess scaled at pi */

    for (i = 0; i < 15; i++) {
        sinx = agc_dp_sin(x);
        cosx = agc_dp_cos(x);
        if (cosx == 0) break;
        /* Newton: x_new = x + (val - sin(x)) / cos(x)
         * All in fixed point with appropriate scaling */
        dx = agc_dp_divide(val - sinx, cosx);
        /* Scale adjustment: divide result is in fraction, need pi scaling */
        dx = dx >> 1;
        x = x + dx;
        if (agc_dp_abs(dx) < 2) break;
    }
    return x;
}

agc_dp_t agc_dp_acos(agc_dp_t val)
{
    /* acos(x) = pi/2 - asin(x) */
    return agc_dp_pack(AGC_HALF, 0) - agc_dp_asin(val);
}

agc_dp_t agc_dp_atan2(agc_dp_t y, agc_dp_t x)
{
    /* atan2(y, x) result scaled at pi */
    agc_dp_t mag, ratio, result;

    if (x == 0 && y == 0) return 0;

    /* Compute atan(y/x) using asin(y / sqrt(x^2 + y^2)) */
    mag = agc_dp_sqrt(agc_dp_multiply(x, x) + agc_dp_multiply(y, y));
    if (mag == 0) return 0;
    ratio = agc_dp_divide(y, mag);
    result = agc_dp_asin(ratio);

    /* Quadrant correction */
    if (x < 0) {
        if (y >= 0)
            result = agc_dp_pack((agc_word_t)16383, 0) - result;
        else
            result = -agc_dp_pack((agc_word_t)16383, 0) - result;
    }
    return result;
}

/* ----------------------------------------------------------------
 * Vector operations (3D, 6 words: Xhi,Xlo, Yhi,Ylo, Zhi,Zlo)
 * ---------------------------------------------------------------- */

void agc_vec_add(const agc_word_t *a, const agc_word_t *b, agc_word_t *result)
{
    int i;
    for (i = 0; i < 3; i++) {
        agc_dp_t va = agc_dp_pack(a[i*2], a[i*2+1]);
        agc_dp_t vb = agc_dp_pack(b[i*2], b[i*2+1]);
        agc_dp_t vr = va + vb;
        agc_dp_unpack(vr, &result[i*2], &result[i*2+1]);
    }
}

void agc_vec_sub(const agc_word_t *a, const agc_word_t *b, agc_word_t *result)
{
    int i;
    for (i = 0; i < 3; i++) {
        agc_dp_t va = agc_dp_pack(a[i*2], a[i*2+1]);
        agc_dp_t vb = agc_dp_pack(b[i*2], b[i*2+1]);
        agc_dp_t vr = va - vb;
        agc_dp_unpack(vr, &result[i*2], &result[i*2+1]);
    }
}

void agc_vec_cross(const agc_word_t *a, const agc_word_t *b, agc_word_t *result)
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

agc_dp_t agc_vec_dot(const agc_word_t *a, const agc_word_t *b)
{
    agc_dp_t sum = 0;
    int i;
    for (i = 0; i < 3; i++) {
        agc_dp_t va = agc_dp_pack(a[i*2], a[i*2+1]);
        agc_dp_t vb = agc_dp_pack(b[i*2], b[i*2+1]);
        sum += agc_dp_multiply(va, vb);
    }
    return sum;
}

agc_dp_t agc_vec_mag(const agc_word_t *a)
{
    agc_dp_t sum_sq = 0;
    int i;
    for (i = 0; i < 3; i++) {
        agc_dp_t v = agc_dp_pack(a[i*2], a[i*2+1]);
        sum_sq += agc_dp_multiply(v, v);
    }
    return agc_dp_sqrt(sum_sq);
}

agc_dp_t agc_vec_unit(const agc_word_t *a, agc_word_t *result)
{
    agc_dp_t mag = agc_vec_mag(a);
    int i;
    if (mag == 0) {
        for (i = 0; i < 6; i++) result[i] = 0;
        return 0;
    }
    for (i = 0; i < 3; i++) {
        agc_dp_t v = agc_dp_pack(a[i*2], a[i*2+1]);
        agc_dp_t u = agc_dp_divide(v, mag);
        agc_dp_unpack(u, &result[i*2], &result[i*2+1]);
    }
    return mag;
}

void agc_vec_scale(agc_dp_t scalar, const agc_word_t *vec, agc_word_t *result)
{
    int i;
    for (i = 0; i < 3; i++) {
        agc_dp_t v = agc_dp_pack(vec[i*2], vec[i*2+1]);
        agc_dp_t r = agc_dp_multiply(scalar, v);
        agc_dp_unpack(r, &result[i*2], &result[i*2+1]);
    }
}

/* ----------------------------------------------------------------
 * Matrix operations (3x3, 18 words, row-major, each element is DP)
 * ---------------------------------------------------------------- */

void agc_mat_vec_mul(const agc_word_t *mat, const agc_word_t *vec,
                     agc_word_t *result)
{
    int row;
    for (row = 0; row < 3; row++) {
        agc_dp_t sum = 0;
        int col;
        for (col = 0; col < 3; col++) {
            agc_dp_t m = agc_dp_pack(mat[(row*3+col)*2],
                                     mat[(row*3+col)*2+1]);
            agc_dp_t v = agc_dp_pack(vec[col*2], vec[col*2+1]);
            sum += agc_dp_multiply(m, v);
        }
        agc_dp_unpack(sum, &result[row*2], &result[row*2+1]);
    }
}

void agc_vec_mat_mul(const agc_word_t *vec, const agc_word_t *mat,
                     agc_word_t *result)
{
    int col;
    for (col = 0; col < 3; col++) {
        agc_dp_t sum = 0;
        int row;
        for (row = 0; row < 3; row++) {
            agc_dp_t v = agc_dp_pack(vec[row*2], vec[row*2+1]);
            agc_dp_t m = agc_dp_pack(mat[(row*3+col)*2],
                                     mat[(row*3+col)*2+1]);
            sum += agc_dp_multiply(v, m);
        }
        agc_dp_unpack(sum, &result[col*2], &result[col*2+1]);
    }
}

/* ----------------------------------------------------------------
 * Utility
 * ---------------------------------------------------------------- */

int agc_dp_to_display(agc_dp_t val, int scale_exp)
{
    /* Convert DP fixed-point to integer for DSKY display
     * scale_exp: power of 2 scaling (e.g., val * 2^scale_exp = real value)
     * Returns integer suitable for 5-digit display */
    long scaled;
    if (scale_exp >= 0) {
        scaled = (long)val << scale_exp;
    } else {
        scaled = (long)val >> (-scale_exp);
    }
    /* Convert from 14-bit fraction to integer */
    return (int)(scaled >> 14);
}

agc_dp_t agc_degrees_to_angle(int degrees)
{
    /* Convert integer degrees to AGC angle scaled at pi (semicircles)
     * 180 degrees = 1.0 in semicircles = 16384 in AGC DP high word
     * So 1 degree = 16384/180 ~= 91.02 */
    long val = (long)degrees * 16384L;
    return (agc_dp_t)(val / 180);
}
