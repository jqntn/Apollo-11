/*
 * executive.c -- Priority-based cooperative job scheduler.
 *
 * Comanche055 (Apollo 11 CM) ANSI C89 port.
 */

#include "agc.h"
#include "executive.h"

#include <string.h>

/* ----------------------------------------------------------------
 * State
 * ---------------------------------------------------------------- */

agc_coreset_t agc_coresets[NUM_CORE_SETS];
int agc_current_job = -1;
int agc_newjob = 0;

static int vac_inuse[NUM_VAC_AREAS];
static volatile int job_ended;

/* ----------------------------------------------------------------
 * Init
 * ---------------------------------------------------------------- */

void
exec_init(void)
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
 * Internal helpers
 * ---------------------------------------------------------------- */

static int
find_free_coreset(void)
{
  int i;
  for (i = 0; i < NUM_CORE_SETS; i++) {
    if (agc_coresets[i].priority == 0 && agc_coresets[i].entry == NULL) {
      return i;
    }
  }
  return -1;
}

static int
find_free_vac(void)
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

static int
find_highest_priority(void)
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
 * NOVAC / FINDVAC
 * ---------------------------------------------------------------- */

int
exec_novac(int priority, agc_jobfunc_t entry)
{
  int slot = find_free_coreset();
  if (slot < 0) {
    return -1;
  }

  agc_coresets[slot].priority = priority;
  agc_coresets[slot].entry = entry;
  agc_coresets[slot].vac_index = -1;

  if (agc_current_job >= 0 &&
      priority > agc_coresets[agc_current_job].priority) {
    agc_newjob = 1;
  }
  return slot;
}

int
exec_findvac(int priority, agc_jobfunc_t entry)
{
  int slot, vac;
  slot = find_free_coreset();
  if (slot < 0) {
    return -1;
  }

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
 * ENDOFJOB / CHANGEJOB / JOBSLEEP / JOBWAKE
 * ---------------------------------------------------------------- */

void
exec_endofjob(void)
{
  if (agc_current_job >= 0) {
    if (agc_coresets[agc_current_job].vac_index >= 0) {
      vac_inuse[agc_coresets[agc_current_job].vac_index] = 0;
    }
    agc_coresets[agc_current_job].priority = 0;
    agc_coresets[agc_current_job].entry = NULL;
    agc_coresets[agc_current_job].vac_index = -1;
    agc_current_job = -1;
  }
  job_ended = 1;
}

void
exec_changejob(void)
{
  agc_newjob = 0;
  job_ended = 1;
}

void
exec_jobsleep(void)
{
  if (agc_current_job >= 0) {
    agc_coresets[agc_current_job].priority =
      -agc_coresets[agc_current_job].priority;
  }
  job_ended = 1;
}

void
exec_jobwake(int coreset_index)
{
  if (coreset_index < 0 || coreset_index >= NUM_CORE_SETS) {
    return;
  }
  if (agc_coresets[coreset_index].priority < 0) {
    agc_coresets[coreset_index].priority =
      -agc_coresets[coreset_index].priority;
    if (agc_current_job >= 0 && agc_coresets[coreset_index].priority >
                                  agc_coresets[agc_current_job].priority) {
      agc_newjob = 1;
    }
  }
}

/* ----------------------------------------------------------------
 * Run one job quantum
 * ---------------------------------------------------------------- */

int
exec_run(void)
{
  int best = find_highest_priority();

  if (best < 0) {
    agc_current_job = -1;
    return 0;
  }

  agc_current_job = best;
  agc_newjob = 0;
  job_ended = 0;

  if (agc_coresets[best].entry != NULL) {
    agc_coresets[best].entry();
  }

  /* Auto-end if the job didn't yield explicitly */
  if (!job_ended && agc_current_job == best) {
    exec_endofjob();
  }

  return 1;
}
