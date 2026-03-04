/*
 * executive.h -- Priority-based cooperative job scheduler (EXECUTIVE.agc).
 *
 * Manages up to 7 concurrent jobs in core sets.  Each job has a
 * priority; the highest-priority ready job runs until it voluntarily
 * yields via ENDOFJOB, CHANGEJOB, or JOBSLEEP.  Jobs are scheduled
 * with NOVAC (basic) or FINDVAC (interpretive, allocates a VAC area).
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

#ifndef EXECUTIVE_H
#define EXECUTIVE_H

#include "agc.h"

typedef struct {
    int             priority;   /* >0 active, <0 sleeping, 0 empty */
    agc_jobfunc_t   entry;
    int             vac_index;  /* VAC area index, or -1 */
} agc_coreset_t;

extern agc_coreset_t agc_coresets[NUM_CORE_SETS];
extern int agc_current_job;
extern int agc_newjob;

void exec_init(void);
int exec_novac(int priority, agc_jobfunc_t entry);
int exec_findvac(int priority, agc_jobfunc_t entry);
void exec_endofjob(void);
void exec_changejob(void);
void exec_jobsleep(void);
void exec_jobwake(int coreset_index);
int exec_run(void);

#endif /* EXECUTIVE_H */
