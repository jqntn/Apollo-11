/*
 * navigation.c -- Orbital mechanics: conic subroutines, R30 (V82).
 *
 * Uses a hardcoded LEO parking orbit state vector (185 km circular,
 * approximating the Apollo 11 parking orbit) for demonstration.
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

#include "agc.h"
#include "agc_cpu.h"
#include "agc_math.h"
#include "navigation.h"
#include "pinball.h"

#include <string.h>

/* ----------------------------------------------------------------
 * State vectors
 * ---------------------------------------------------------------- */

agc_state_vector_t nav_csm_state;
agc_state_vector_t nav_lem_state;

/* ----------------------------------------------------------------
 * Integer square root (Newton's method)
 * ---------------------------------------------------------------- */

static long
isqrt_long(long val)
{
  long x, prev;
  if (val <= 0)
    return 0;
  x = val;
  while (x > 32767)
    x >>= 1;
  if (x < 1)
    x = 1;
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
 * Hardcoded 185 km circular orbit: r = 6556 km, v = 7.79 km/s.
 * Position scaled at 2^(-14) km, velocity scaled at 2^(6-14) km/s.
 */

void
nav_init(void)
{
  agc_dp_t rx, vy;
  agc_word_t h, l;

  memset(&nav_csm_state, 0, sizeof(nav_csm_state));
  memset(&nav_lem_state, 0, sizeof(nav_lem_state));

  rx = 6556L * 16384L;
  agc_dp_unpack(rx, &h, &l);
  nav_csm_state.r[0] = h;
  nav_csm_state.r[1] = l;

  vy = (long)(7.79 * 16384.0 / 64.0);
  agc_dp_unpack(vy, &h, &l);
  nav_csm_state.v[2] = h;
  nav_csm_state.v[3] = l;

  nav_csm_state.time = 0;
}

/* ----------------------------------------------------------------
 * Compute orbital parameters from state vector
 * ----------------------------------------------------------------
 * Uses vis-viva equation: a = mu*r / (2*mu - v^2*r)
 * Eccentricity from angular momentum: e^2 = 1 - h^2/(mu*a)
 */

void
nav_compute_orbit(const agc_state_vector_t* sv,
                  long* apogee_km,
                  long* perigee_km,
                  long* period_sec)
{
  long rx, ry, rz, vx, vy, vz;
  long r_mag, v_sq, r_mag_sq;
  long mu, two_mu;
  long energy_num, a_km;
  agc_int64_t h_sq;
  long e_num, e_den;
  long apo, peri;

  rx = (long)agc_dp_pack(sv->r[0], sv->r[1]) / 16384L;
  ry = (long)agc_dp_pack(sv->r[2], sv->r[3]) / 16384L;
  rz = (long)agc_dp_pack(sv->r[4], sv->r[5]) / 16384L;

  vx = (long)agc_dp_pack(sv->v[0], sv->v[1]) * 64L / 16384L;
  vy = (long)agc_dp_pack(sv->v[2], sv->v[3]) * 64L / 16384L;
  vz = (long)agc_dp_pack(sv->v[4], sv->v[5]) * 64L / 16384L;

  r_mag_sq = rx * rx + ry * ry + rz * rz;
  r_mag = isqrt_long(r_mag_sq);
  if (r_mag == 0)
    r_mag = 1;

  v_sq = vx * vx + vy * vy + vz * vz;
  mu = MU_EARTH_KM3S2;

  /* Semi-major axis: a = mu*r / (2*mu - v^2*r) */
  two_mu = 2 * mu;
  energy_num = two_mu - v_sq * r_mag;

  if (energy_num <= 0) {
    *apogee_km = 99999;
    *perigee_km = r_mag - EARTH_RADIUS_KM;
    *period_sec = 0;
    return;
  }

  a_km = (long)(((agc_int64_t)mu * r_mag) / energy_num);

  /* Specific angular momentum squared */
  {
    agc_int64_t hx = (agc_int64_t)ry * vz - (agc_int64_t)rz * vy;
    agc_int64_t hy = (agc_int64_t)rz * vx - (agc_int64_t)rx * vz;
    agc_int64_t hz = (agc_int64_t)rx * vy - (agc_int64_t)ry * vx;
    h_sq = hx * hx + hy * hy + hz * hz;
  }

  /* Eccentricity: e^2 = 1 - h^2/(mu*a) */
  e_den = (long)((agc_int64_t)mu * a_km);
  if (e_den == 0)
    e_den = 1;
  e_num = (long)((agc_int64_t)e_den - h_sq / (mu > 0 ? mu : 1));

  /* Apogee = a*(1+e) - Re, Perigee = a*(1-e) - Re */
  {
    long ae_sq, ae;
    if (e_num < 0)
      e_num = 0;
    ae_sq = (long)(((agc_int64_t)a_km * a_km / e_den) * e_num);
    ae = isqrt_long(ae_sq > 0 ? ae_sq : 0);

    apo = a_km + ae - EARTH_RADIUS_KM;
    peri = a_km - ae - EARTH_RADIUS_KM;
  }

  if (apo < 0)
    apo = 0;
  if (peri < 0)
    peri = 0;

  *apogee_km = apo;
  *perigee_km = peri;

  /* Period: T ~ 2*pi*sqrt(a^3/mu) */
  {
    long a3_over_mu = (long)(((agc_int64_t)a_km * a_km / mu) * a_km);
    long sqrt_val = isqrt_long(a3_over_mu > 0 ? a3_over_mu : 0);
    *period_sec = 6 * sqrt_val + sqrt_val / 4; /* ~6.2832 */
  }
}

/* ----------------------------------------------------------------
 * R30 (V82): Orbit parameter display
 * ----------------------------------------------------------------
 * Displays via noun 44: R1=apogee NM, R2=perigee NM, R3=TFF min.
 */

void
program_r30_v82(void)
{
  long apogee_km, perigee_km, period_sec;
  int apo_nm, peri_nm, tff_min;

  nav_compute_orbit(&nav_csm_state, &apogee_km, &perigee_km, &period_sec);

  /* 1 km = 0.53996 NM ~ 54/100 */
  apo_nm = (int)((apogee_km * 54) / 100);
  peri_nm = (int)((perigee_km * 54) / 100);
  tff_min = (int)(period_sec / 60);

  agc_write_erasable(5, 0, (agc_word_t)apo_nm);
  agc_write_erasable(5, 1, (agc_word_t)peri_nm);
  agc_write_erasable(5, 2, (agc_word_t)tff_min);

  pinball_nvsub(6, 44);
}
