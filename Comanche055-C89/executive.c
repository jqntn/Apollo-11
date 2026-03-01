/*
 * executive.c -- Priority-based cooperative job scheduler
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port
 * Faithful reproduction of the AGC Executive (7 core sets)
 *
 * The Executive manages up to 7 concurrent jobs. Each job has a priority;
 * the highest-priority ready job runs until it voluntarily yields
 * (ENDOFJOB, CHANGEJOB, JOBSLEEP). This is cooperative multitasking.
 */

#include <string.h>
#include "executive.h"
#include "agc_cpu.h"

/* ----------------------------------------------------------------
 * State
 * ---------------------------------------------------------------- */

agc_coreset_t agc_coresets[NUM_CORE_SETS];
int agc_current_job = -1;
int agc_newjob = 0;

/* VAC area allocation flags (1 = in use) */
static int vac_inuse[NUM_VAC_AREAS];

/* Flag: set to 1 when exec_endofjob is called inside a job */
static volatile int job_ended;

/* ----------------------------------------------------------------
 * Init
 * ---------------------------------------------------------------- */

void exec_init(void)
{
    int i;
    memset(agc_coresets, 0, sizeof(agc_coresets));
    memset(vac_inuse, 0, sizeof(vac_inuse));
    for (i = 0; i < NUM_CORE_SETS; i++) {
        agc_coresets[i].vac_index = -1;
    }
    agc_current_job = -1;
    agc_newjob = 0;
    job_ended = 0;
}

/* ----------------------------------------------------------------
 * Find free core set
 * ---------------------------------------------------------------- */

static int find_free_coreset(void)
{
    int i;
    for (i = 0; i < NUM_CORE_SETS; i++) {
        if (agc_coresets[i].priority == 0 && agc_coresets[i].entry == NULL) {
            return i;
        }
    }
    return -1;
}

/* ----------------------------------------------------------------
 * Find free VAC area
 * ---------------------------------------------------------------- */

static int find_free_vac(void)
{
    int i;
    for (i = 0; i < NUM_VAC_AREAS; i++) {
        if (!vac_inuse[i]) {
            vac_inuse[i] = 1;
            return i;
        }
    }
    return -1;
}

/* ----------------------------------------------------------------
 * Find highest priority ready job
 * ---------------------------------------------------------------- */

static int find_highest_priority(void)
{
    int best = -1;
    int best_prio = 0;
    int i;
    for (i = 0; i < NUM_CORE_SETS; i++) {
        if (agc_coresets[i].priority > best_prio && agc_coresets[i].entry != NULL) {
            best_prio = agc_coresets[i].priority;
            best = i;
        }
    }
    return best;
}

/* ----------------------------------------------------------------
 * Schedule a basic job (NOVAC)
 * ---------------------------------------------------------------- */

int exec_novac(int priority, agc_jobfunc_t entry)
{
    int slot = find_free_coreset();
    if (slot < 0) return -1;

    agc_coresets[slot].priority = priority;
    agc_coresets[slot].entry = entry;
    agc_coresets[slot].vac_index = -1;

    /* Check if this new job is higher priority than current */
    if (agc_current_job >= 0 &&
        priority > agc_coresets[agc_current_job].priority) {
        agc_newjob = 1;
    }

    return slot;
}

/* ----------------------------------------------------------------
 * Schedule an interpretive job (FINDVAC)
 * ---------------------------------------------------------------- */

int exec_findvac(int priority, agc_jobfunc_t entry)
{
    int slot, vac;
    slot = find_free_coreset();
    if (slot < 0) return -1;

    vac = find_free_vac();
    if (vac < 0) {
        return -1;
    }

    agc_coresets[slot].priority = priority;
    agc_coresets[slot].entry = entry;
    agc_coresets[slot].vac_index = vac;

    if (agc_current_job >= 0 &&
        priority > agc_coresets[agc_current_job].priority) {
        agc_newjob = 1;
    }

    return slot;
}

/* ----------------------------------------------------------------
 * End current job (ENDOFJOB)
 * ---------------------------------------------------------------- */

void exec_endofjob(void)
{
    if (agc_current_job >= 0) {
        /* Free VAC area if allocated */
        if (agc_coresets[agc_current_job].vac_index >= 0) {
            vac_inuse[agc_coresets[agc_current_job].vac_index] = 0;
        }
        /* Clear the core set */
        agc_coresets[agc_current_job].priority = 0;
        agc_coresets[agc_current_job].entry = NULL;
        agc_coresets[agc_current_job].vac_index = -1;
        agc_current_job = -1;
    }
    job_ended = 1;
}

/* ----------------------------------------------------------------
 * Change to highest priority job (CHANG1/CHANG2)
 * ---------------------------------------------------------------- */

void exec_changejob(void)
{
    agc_newjob = 0;
    /* The main loop (exec_run) will pick the highest priority next */
    job_ended = 1;
}

/* ----------------------------------------------------------------
 * Job sleep (JOBSLEEP)
 * ---------------------------------------------------------------- */

void exec_jobsleep(void)
{
    if (agc_current_job >= 0) {
        /* Negate priority to mark as sleeping */
        agc_coresets[agc_current_job].priority =
            -agc_coresets[agc_current_job].priority;
    }
    job_ended = 1;
}

/* ----------------------------------------------------------------
 * Job wake (JOBWAKE)
 * ---------------------------------------------------------------- */

void exec_jobwake(int coreset_index)
{
    if (coreset_index < 0 || coreset_index >= NUM_CORE_SETS) return;
    if (agc_coresets[coreset_index].priority < 0) {
        /* Restore positive priority */
        agc_coresets[coreset_index].priority =
            -agc_coresets[coreset_index].priority;

        /* Check if woken job is higher priority than current */
        if (agc_current_job >= 0 &&
            agc_coresets[coreset_index].priority >
            agc_coresets[agc_current_job].priority) {
            agc_newjob = 1;
        }
    }
}

/* ----------------------------------------------------------------
 * Run one job quantum
 * ---------------------------------------------------------------- */

int exec_run(void)
{
    int best = find_highest_priority();

    if (best < 0) {
        /* No ready jobs: idle (DUMMYJOB equivalent) */
        agc_current_job = -1;
        return 0;
    }

    agc_current_job = best;
    agc_newjob = 0;
    job_ended = 0;

    /* Dispatch the job */
    if (agc_coresets[best].entry != NULL) {
        agc_coresets[best].entry();
    }

    /* If the job didn't call endofjob/changejob/sleep, auto-end it */
    if (!job_ended && agc_current_job == best) {
        exec_endofjob();
    }

    return 1;
}
