/*
 * navigation.h -- Orbital mechanics: conic subroutines, R30 (V82).
 *
 * Implements orbit parameter computation from a state vector
 * (position + velocity in Earth-centered inertial coordinates).
 * The R30 routine computes apogee, perigee, and orbital period,
 * storing results in erasable for noun 44 display.
 *
 * Maps to CONIC_SUBROUTINES.agc and R30.agc.
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

#ifndef NAVIGATION_H
#define NAVIGATION_H

#include "agc.h"

typedef struct
{
  agc_word_t r[6]; /* Position vector (DP words) */
  agc_word_t v[6]; /* Velocity vector (DP words) */
  agc_dp_t time;   /* Time tag (centiseconds) */
} agc_state_vector_t;

extern agc_state_vector_t nav_csm_state;
extern agc_state_vector_t nav_lem_state;

#define MU_EARTH_KM3S2 398600L
#define EARTH_RADIUS_KM 6371L

void
nav_init(void);
void
program_r30_v82(void);
void
nav_compute_orbit(const agc_state_vector_t* sv,
                  long* apogee_km,
                  long* perigee_km,
                  long* period_sec);

#endif /* NAVIGATION_H */
