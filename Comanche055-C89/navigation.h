/*
 * navigation.h -- Conic subroutines, Kepler solver, R30 (V82)
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port
 */

#ifndef NAVIGATION_H
#define NAVIGATION_H

#include "agc.h"
#include "agc_math.h"

/* ----------------------------------------------------------------
 * Orbital state vector (position + velocity in Earth-centered inertial)
 * ----------------------------------------------------------------
 * Position in kilometers, velocity in km/s
 * Stored as DP fixed-point scaled per AGC conventions
 */

typedef struct {
    agc_word_t r[6];    /* Position vector (X,Y,Z as DP words) */
    agc_word_t v[6];    /* Velocity vector (X,Y,Z as DP words) */
    agc_dp_t   time;    /* Time tag (centiseconds) */
} agc_state_vector_t;

/* CSM and LEM state vectors */
extern agc_state_vector_t nav_csm_state;
extern agc_state_vector_t nav_lem_state;

/* ----------------------------------------------------------------
 * Constants
 * ---------------------------------------------------------------- */

/* Earth gravitational parameter mu (km^3/s^2) as DP fixed-point
 * mu_earth = 398600.4418 km^3/s^2 */
#define MU_EARTH_KM3S2  398600L

/* Earth radius (km) */
#define EARTH_RADIUS_KM  6371L

/* Nautical miles per kilometer */
/* 1 NM = 1.852 km, so 1 km = 0.53996 NM */

/* ----------------------------------------------------------------
 * Navigation API
 * ---------------------------------------------------------------- */

/* Initialize navigation with hardcoded Apollo 11 state vector */
void nav_init(void);

/* R30 (V82): Compute and display orbital parameters
 * Calculates apogee, perigee, and time to free fall (TFF)
 * Results stored in erasable for noun 44 display */
void program_r30_v82(void);

/* Compute orbital elements from state vector
 * Returns: semi-major axis (km), eccentricity, period (seconds) */
void nav_compute_orbit(const agc_state_vector_t *sv,
                       long *apogee_km, long *perigee_km,
                       long *period_sec);

#endif /* NAVIGATION_H */
