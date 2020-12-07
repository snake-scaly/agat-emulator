#ifndef CPUINT_H
#define CPUINT_H

#include "types.h"
#include "memory.h"
#include "streams.h"
#include "debug.h"
#include "sysconf.h"
#include "runmgr.h"
#include "runmgrint.h"

struct CPU_TIMER
{
	long used;
	int  delay, remains;
	long param;
};

#define MAX_CPU_TIMERS	32


enum {
	CPU_INTR_RESET,
	CPU_INTR_IRQ,
	CPU_INTR_NMI,
	CPU_INTR_HRESET,
	CPU_INTR_NOIRQ,
	CPU_INTR_NONMI,

	CPU_INTR_MAX
};

struct CPU_STATE
{
	struct SYS_RUN_STATE*sr;

	HANDLE th;
	DWORD thid;
	void *state;

	int fast_mode, fast_boost;

	unsigned long long freq_6502;//=800; // 1MHz
	int undoc; // enable undocumented commands
	unsigned long long tsc_6502;
	int min_msleep; //=10;
	int lim_fetches; // =5000
	int need_cpusleep;
	volatile int term_req;
	volatile int sleep_req;
	HANDLE wakeup, response;

	int int_ticks[2];

	struct CPU_TIMER timers[MAX_CPU_TIMERS];


	int  (*hook_proc)(void*p);
	void *hook_data;

	int  (*exec_op)(struct CPU_STATE*st);
	void (*intr)(struct CPU_STATE*st, int t);
	void (*free)(struct CPU_STATE*st);
	int  (*save)(struct CPU_STATE*st, OSTREAM*out);
	int  (*load)(struct CPU_STATE*st, ISTREAM*in);
	int  (*cmd)(struct CPU_STATE*st, int cmd, int data, long param);
};


int  cpu_init(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*st, struct SLOTCONFIG*sc);

int cpu_hreset(struct CPU_STATE*st);
int cpu_intr(struct CPU_STATE*st, int t, int nticks); // nticks = 0 -> no auto disable interrupt
int cpu_pause(struct CPU_STATE*st, int p);
int cpu_step(struct CPU_STATE*st, int ncmd);
int cpu_cmd(struct CPU_STATE*cs, int cmd, int data, long param);

#endif //CPUINT_H
