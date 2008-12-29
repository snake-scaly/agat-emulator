#include "sysconf.h"
#include "runstate.h"
#include <windows.h>
#include <tchar.h>

struct RS_DATA
{
	LPCTSTR name;
	struct SYS_RUN_STATE*st;
	unsigned flags;
};

static int n_runs;
static struct RS_DATA*runs;

int init_run_states()
{
	n_runs = 0;
	runs = NULL;
	return 0;
}

void free_run_states()
{
	int i;
	for (i = 0; i < n_runs; ++ i) {
		if (runs[i].name) free((void*)runs[i].name);
	}
	if (runs) free(runs);
}

int find_run_by_name(LPCTSTR name)
{
	int i;
//	printf("find_run by name: %s\n", name);
	for (i = 0; i < n_runs; ++ i) {
		if (runs[i].name && !lstrcmpi(name, runs[i].name)) return i;
	}
	return -1;
}

int find_run_by_ptr(struct SYS_RUN_STATE*st)
{
	int i;
//	printf("find_run by ptr: %p\n", st);
	for (i = 0; i < n_runs; ++ i) {
		if (runs[i].st == st) return i;
	}
	return -1;
}

int append_run(LPCTSTR name, unsigned flags, struct SYS_RUN_STATE*st)
{
	int n = n_runs ++;
//	printf("append_run %s: %x, %p\n", name, flags, st);
	runs = realloc(runs, n_runs * sizeof(runs[0]));
	if (!runs) return -1;
	runs[n].name = name?_tcsdup(name):NULL;
	runs[n].flags = flags;
	runs[n].st = st;
	return n;
}

int set_run_state_flags(LPCTSTR name, unsigned flags)
{
	int i;
	i = find_run_by_name(name);
	if (i == -1) i = append_run(name, flags, NULL);
	else runs[i].flags = flags;
	return i;
}

unsigned get_run_state_flags(LPCTSTR name)
{
	int i;
	i = find_run_by_name(name);
	if (i == -1) return 0;
	return runs[i].flags;
}

unsigned or_run_state_flags(LPCTSTR name, unsigned flags)
{
	int i;
	i = find_run_by_name(name);
	if (i == -1) { append_run(name, flags, NULL); return flags; }
	return runs[i].flags |= flags;
}

unsigned and_run_state_flags(LPCTSTR name, unsigned flags)
{
	int i;
	i = find_run_by_name(name);
	if (i == -1) { append_run(name, 0, NULL); return 0; }
	return runs[i].flags &= flags;
}

unsigned get_run_state_flags_by_ptr(struct SYS_RUN_STATE*st)
{
	int i;
	i = find_run_by_ptr(st);
	if (i == -1) return 0;
	return runs[i].flags;
}

LPCTSTR get_run_state_name(struct SYS_RUN_STATE*st)
{
	int i;
	i = find_run_by_ptr(st);
	if (i == -1) return NULL;
	return runs[i].name;
}


int set_run_state_ptr(LPCTSTR name, struct SYS_RUN_STATE*st)
{
	int i;
	i = find_run_by_name(name);
	if (i == -1) i = append_run(name, 0, st);
	else runs[i].st = st;
	return i;
}

struct SYS_RUN_STATE*get_run_state_ptr(LPCTSTR name)
{
	int i;
	i = find_run_by_name(name);
	if (i == -1) return NULL;
	return runs[i].st;
}


int get_n_running_systems()
{
	int i, r = 0;
	for (i = 0; i < n_runs; ++ i) {
		if (runs[i].name && runs[i].st && (runs[i].flags & RUNSTATE_RUNNING)) ++r;
	}
	return r;
}

int free_all_running_systems()
{
	int i, r = 0;
	for (i = 0; i < n_runs; ++ i) {
		if (runs[i].name && runs[i].st && (runs[i].flags & RUNSTATE_RUNNING)) {
			free_system_state(runs[i].st);
			runs[i].st = NULL;
			runs[i].flags &= ~(RUNSTATE_RUNNING | RUNSTATE_PAUSED);
		}
	}
	return r;
}

