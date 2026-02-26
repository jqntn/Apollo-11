/*
 * navigation.c -- Conic subroutines, Kepler solver, R30 (V82)
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port
 *
 * Implements orbital mechanics calculations using fixed-point math.
 * The R30 routine (V82) computes apogee, perigee, and time to free fall
 * from the current CSM state vector.
 *
 * Uses a hardcoded Apollo 11 trans-lunar injection state vector as
 * initial data.
 */

#include <string.h>
#include "navigation.h"
#include "agc_cpu.h"
#include "pinball.h"

/* ----------------------------------------------------------------
 * State vectors
 * ---------------------------------------------------------------- */

agc_state_vector_t nav_csm_state;
agc_state_vector_t nav_lem_state;

/* ----------------------------------------------------------------
 * Integer square root (for long values)
 * ---------------------------------------------------------------- */

static long isqrt_long(long val)
{
    long x, prev;
    if (val <= 0) return 0;
    x = val;
    /* Rough initial guess */
    while (x > 32767) x >>= 1;
    if (x < 1) x = 1;
    prev = 0;
    while (x != prev) {
        prev = x;
        x = (x + val / x) / 2;
    }
    return x;
}

/* ----------------------------------------------------------------
 * Initialize navigation
 * ----------------------------------------------------------------
 * Hardcoded state vector approximating Apollo 11 shortly after
 * trans-lunar injection (TLI). This is in Earth-centered inertial
 * coordinates.
 *
 * Position: ~6563 km altitude (LEO parking orbit just after TLI)
 * Velocity: ~10.8 km/s (trans-lunar velocity)
 *
 * For demonstration purposes, we use a simpler low Earth orbit
 * state vector that produces nice apogee/perigee numbers:
 *   - 185 km circular orbit (typical Apollo parking orbit)
 *   - r = 6556 km, v = 7.79 km/s
 */

void nav_init(void)
{
    /* Position: X=6556 km, Y=0, Z=0 (simplified equatorial orbit)
     * Velocity: X=0, Y=7.79 km/s, Z=0 (circular orbit speed)
     *
     * Stored as DP fixed-point:
     * Position scaled at 2^(-14) km: value = km * 16384
     * Velocity scaled at 2^(6) km/s * 2^(-14): value = km/s * 16384 / 64
     *
     * For our integer math, we store in a simpler way:
     * We'll use the DP values directly for the orbital computation.
     */

    /* CSM state vector (simplified LEO parking orbit) */
    /* Position: (6556, 0, 0) km */
    agc_dp_t rx, vy;
    agc_word_t h, l;

    memset(&nav_csm_state, 0, sizeof(nav_csm_state));
    memset(&nav_lem_state, 0, sizeof(nav_lem_state));

    rx = 6556L * 16384L;  /* Position X in fixed-point */
    agc_dp_unpack(rx, &h, &l);
    nav_csm_state.r[0] = h;
    nav_csm_state.r[1] = l;

    vy = (long)(7.79 * 16384.0 / 64.0);  /* Velocity Y in fixed-point */
    agc_dp_unpack(vy, &h, &l);
    nav_csm_state.v[2] = h;
    nav_csm_state.v[3] = l;

    nav_csm_state.time = 0;
}

/* ----------------------------------------------------------------
 * Compute orbital parameters from state vector
 * ----------------------------------------------------------------
 * Given position r (km) and velocity v (km/s), compute:
 *   - Semi-major axis a
 *   - Eccentricity e
 *   - Apogee altitude (km above surface)
 *   - Perigee altitude (km above surface)
 *   - Orbital period (seconds)
 *
 * Uses the vis-viva equation: v^2 = mu * (2/r - 1/a)
 * So: 1/a = 2/r - v^2/mu
 *     a = 1 / (2/r - v^2/mu)
 *
 * Specific orbital energy: epsilon = v^2/2 - mu/r
 * Semi-major axis: a = -mu / (2 * epsilon)
 *
 * Eccentricity from: e = sqrt(1 + 2*epsilon*h^2/mu^2)
 * where h = |r x v| (specific angular momentum)
 */

void nav_compute_orbit(const agc_state_vector_t *sv,
                       long *apogee_km, long *perigee_km,
                       long *period_sec)
{
    long rx, ry, rz, vx, vy, vz;
    long r_mag, v_sq, r_mag_sq;
    long mu, two_mu;
    long energy_num, a_km;
    long h_sq, e_num, e_den;
    long apo, peri;

    /* Extract position (km, integer) and velocity (km/s * 100 for precision) */
    /* Simplified: we use integer km and integer m/s */
    rx = (long)agc_dp_pack(sv->r[0], sv->r[1]) / 16384L;
    ry = (long)agc_dp_pack(sv->r[2], sv->r[3]) / 16384L;
    rz = (long)agc_dp_pack(sv->r[4], sv->r[5]) / 16384L;

    /* Velocity: stored with different scaling, extract as m/s */
    vx = (long)agc_dp_pack(sv->v[0], sv->v[1]) * 64L / 16384L;
    vy = (long)agc_dp_pack(sv->v[2], sv->v[3]) * 64L / 16384L;
    vz = (long)agc_dp_pack(sv->v[4], sv->v[5]) * 64L / 16384L;

    /* |r| in km */
    r_mag_sq = rx*rx + ry*ry + rz*rz;
    r_mag = isqrt_long(r_mag_sq);
    if (r_mag == 0) r_mag = 1;

    /* |v|^2 in (km/s)^2 * 10000 for precision */
    /* Actually let's work in km and km/s directly */
    /* v stored as integer km/s approximation */
    v_sq = vx*vx + vy*vy + vz*vz;

    mu = MU_EARTH_KM3S2;  /* 398600 km^3/s^2 */

    /* Specific orbital energy: eps = v^2/2 - mu/r (km^2/s^2) */
    /* eps * r = v^2*r/2 - mu */
    /* Semi-major axis: a = -mu / (2*eps) = mu*r / (2*mu - v^2*r) */

    two_mu = 2 * mu;
    energy_num = two_mu - v_sq * r_mag;

    if (energy_num <= 0) {
        /* Hyperbolic or parabolic orbit */
        *apogee_km = 99999;
        *perigee_km = r_mag - EARTH_RADIUS_KM;
        *period_sec = 0;
        return;
    }

    a_km = (mu * r_mag) / energy_num;

    /* Specific angular momentum: h = r x v */
    /* |h|^2 = (ry*vz - rz*vy)^2 + (rz*vx - rx*vz)^2 + (rx*vy - ry*vx)^2 */
    {
        long hx = ry*vz - rz*vy;
        long hy = rz*vx - rx*vz;
        long hz = rx*vy - ry*vx;
        h_sq = hx*hx + hy*hy + hz*hz;
    }

    /* Eccentricity: e^2 = 1 - h^2 / (mu * a)
     * => e^2 = 1 - h^2 / (mu * a)
     * => e = sqrt(1 - p/a) where p = h^2/mu */
    e_den = mu * a_km;
    if (e_den == 0) e_den = 1;
    e_num = e_den - h_sq / (mu > 0 ? mu : 1);

    /* Apogee = a * (1 + e) - R_earth, Perigee = a * (1 - e) - R_earth */
    /* Since e^2 = e_num / e_den: */
    /* a*(1+e) = a + a*e, a*(1-e) = a - a*e */
    /* a*e = a * sqrt(e_num/e_den) = sqrt(a^2 * e_num / e_den) */
    {
        long ae_sq, ae;
        if (e_num < 0) e_num = 0;  /* Circular orbit */
        ae_sq = (a_km * a_km * e_num) / e_den;
        ae = isqrt_long(ae_sq > 0 ? ae_sq : 0);

        apo  = a_km + ae - EARTH_RADIUS_KM;
        peri = a_km - ae - EARTH_RADIUS_KM;
    }

    if (apo < 0) apo = 0;
    if (peri < 0) peri = 0;

    *apogee_km = apo;
    *perigee_km = peri;

    /* Period: T = 2*pi*sqrt(a^3/mu) seconds
     * Approximate: T ~ 6.2832 * sqrt(a^3 / mu)
     * For a in km and mu in km^3/s^2 */
    {
        /* a^3/mu: if a=6556, a^3=2.816e11, mu=3.986e5, ratio=7.066e5
         * sqrt(7.066e5) = 840.6, * 2pi = 5283 sec = 88 min */
        long a3_over_mu = (a_km * a_km / mu) * a_km;
        long sqrt_val = isqrt_long(a3_over_mu > 0 ? a3_over_mu : 0);
        *period_sec = 6 * sqrt_val + sqrt_val / 4;  /* ~6.2832 * sqrt */
    }
}

/* ----------------------------------------------------------------
 * R30 (V82): Orbit parameter display
 * ----------------------------------------------------------------
 * Computes orbital parameters and displays them via noun 44:
 *   R1: Apogee altitude (nautical miles)
 *   R2: Perigee altitude (nautical miles)
 *   R3: Time to free fall (minutes, or TFF to perigee)
 */

void program_r30_v82(void)
{
    long apogee_km, perigee_km, period_sec;
    int apo_nm, peri_nm, tff_min;

    /* Compute orbital parameters from CSM state vector */
    nav_compute_orbit(&nav_csm_state, &apogee_km, &perigee_km, &period_sec);

    /* Convert km to nautical miles (1 km = 0.53996 NM, approx 54/100) */
    apo_nm  = (int)((apogee_km * 54) / 100);
    peri_nm = (int)((perigee_km * 54) / 100);
    tff_min = (int)(period_sec / 60);

    /* Store in erasable for noun 44 access */
    agc_write_erasable(5, 0, (agc_word_t)apo_nm);
    agc_write_erasable(5, 1, (agc_word_t)peri_nm);
    agc_write_erasable(5, 2, (agc_word_t)tff_min);

    /* Display via V06 N44 */
    pinball_nvsub(6, 44);
}
