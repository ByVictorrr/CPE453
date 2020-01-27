#ifndef SCHEDULER_H_
#define SCHEDULER_H_
#include "lwp.h"
#define st_next sched_two
#define st_prev sched_one

void rr_admit(thread new);
void rr_remove(thread victim);
thread rr_next();

#endif