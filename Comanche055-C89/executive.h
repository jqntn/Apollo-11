/*
 * executive.h -- Priority-based cooperative job scheduler
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port
 * Faithful reproduction of the AGC Executive (7 core sets)
 */

#ifndef EXECUTIVE_H
#define EXECUTIVE_H

#include "agc.h"

/* ----------------------------------------------------------------
 * Core set structure
 * ----------------------------------------------------------------
 * Each core set holds the state of one job:
 *   - priority (0 = empty, >0 = active, <0 = asleep)
 *   - entry function pointer
 *   - VAC area index (-1 if no VAC)
 */

typedef struct {
    int             priority;   /* >0 active, <0 sleeping, 0 empty */
    agc_jobfunc_t   entry;      /* Job entry point */
    int             vac_index;  /* VAC area index, or -1 */
} agc_coreset_t;

/* Core sets */
extern agc_coreset_t agc_coresets[NUM_CORE_SETS];

/* Index of currently running job (-1 if none) */
extern int agc_current_job;

/* NEWJOB flag: set when a higher-priority job is waiting */
extern int agc_newjob;

/* ----------------------------------------------------------------
 * Executive API
 * ---------------------------------------------------------------- */

/* Initialize the executive (clear all core sets) */
void exec_init(void);

/* Schedule a basic job (no VAC area needed).
 * Returns core set index, or -1 if no free slot. */
int exec_novac(int priority, agc_jobfunc_t entry);

/* Schedule an interpretive job (allocates a VAC area).
 * Returns core set index, or -1 if no free slot. */
int exec_findvac(int priority, agc_jobfunc_t entry);

/* Yield: end current job, scan for highest priority, dispatch it.
 * Called at the end of every job. */
void exec_endofjob(void);

/* Change to highest priority job if one is waiting.
 * Called voluntarily by a running job to yield cooperatively. */
void exec_changejob(void);

/* Put current job to sleep (negate its priority). */
void exec_jobsleep(void);

/* Wake a sleeping job identified by its core set index. */
void exec_jobwake(int coreset_index);

/* Run one job quantum: find highest priority job and call it.
 * Returns 1 if a job ran, 0 if only idle. */
int exec_run(void);

#endif /* EXECUTIVE_H */
