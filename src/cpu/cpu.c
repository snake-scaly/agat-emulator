#include "cpuint.h"

#include "localize.h"


static DWORD CALLBACK cpu_thread(struct CPU_STATE*cs);

static struct CPU_TIMER* find_timer(struct CPU_STATE*cs, long param)
{
	struct CPU_TIMER*r;
	int i;
	for (r = cs->timers, i = MAX_CPU_TIMERS; i; --i, ++r)
		if (r->used && r->param == param) return r;
	return NULL;
}

static struct CPU_TIMER* new_timer(struct CPU_STATE*cs, int delay, long param)
{
	struct CPU_TIMER*r;
	int i;
	for (r = cs->timers, i = MAX_CPU_TIMERS; i; --i, ++r)
		if (!InterlockedExchange(&r->used, 1)) {
			r->delay = delay;
			r->remains = 0;
			r->param = param;
			return r;
		}
	return NULL;
}

static int cpu_command(struct SLOT_RUN_STATE*st, int cmd, int data, long param)
{
	struct CPU_STATE*cs = st->data;
	int r;
	switch (cmd) {
	case SYS_COMMAND_FAST:
		r = cs->fast_mode;
//		if (data) cs->fast_mode ++;
//		else cs->fast_mode --;
		cs->fast_mode = data;
//		printf("fast mode: %i\n", data);
		return r;
	case SYS_COMMAND_BOOST:
//		printf("cpu: boost (%i) += %i\n", cs->fast_boost, data);
		cs->fast_boost += data;
		return cs->fast_boost;
	case SYS_COMMAND_HRESET:
		return cpu_hreset(cs);
	case SYS_COMMAND_RESET:
		return cpu_intr(cs, CPU_INTR_HRESET, 0);
	case SYS_COMMAND_IRQ:
		return cpu_intr(cs, CPU_INTR_IRQ, data);
	case SYS_COMMAND_NMI:
		return cpu_intr(cs, CPU_INTR_NMI, data);
	case SYS_COMMAND_NOIRQ:
		return cpu_intr(cs, CPU_INTR_NOIRQ, 0);
	case SYS_COMMAND_NONMI:
		return cpu_intr(cs, CPU_INTR_NONMI, 0);
	case SYS_COMMAND_START:
		cpu_pause(cs, 0);
		return 0;
	case SYS_COMMAND_STOP:
		cpu_pause(cs, 1);
		return 0;
	case SYS_COMMAND_SET_CPU_HOOK:
		cs->hook_proc = (void*)data;
		cs->hook_data = (void*)param;
		return 1;
	case SYS_COMMAND_SET_CPUTIMER: {
		struct CPU_TIMER*r;
		r = find_timer(cs, param);
		if (r) {
			r->delay = data;
			r->remains = 0;
			r->used = data?1:0;
		} else {
			r = new_timer(cs, data, param);
		}
		return r?1:0; }
	default:
		return cpu_cmd(cs, cmd, data, param);
	}
	return 0;
}

static int cpu_term(struct SLOT_RUN_STATE*st)
{
	struct CPU_STATE*cs = st->data;
	DWORD res;
	cs->term_req = 1;
	cs->sleep_req = 0;
	SetEvent(cs->wakeup);
	if (GetExitCodeThread(cs->th, &res) && res != STILL_ACTIVE) {
		WaitForSingleObject(cs->th, 1000);
	}
	TerminateThread(cs->th, 0);
	CloseHandle(cs->th);
	CloseHandle(cs->wakeup);
	CloseHandle(cs->response);
	cs->free(cs);
	free(cs);
	return 0;
}

int  cpu_save(struct SLOT_RUN_STATE*st, OSTREAM*out)
{
	struct CPU_STATE*cs = st->data;
	WRITE_FIELD(out, cs->fast_mode);
	WRITE_FIELD(out, cs->freq_6502);
	WRITE_FIELD(out, cs->undoc);
	WRITE_FIELD(out, cs->tsc_6502);
	WRITE_FIELD(out, cs->min_msleep);
	WRITE_FIELD(out, cs->lim_fetches);
	WRITE_ARRAY(out, cs->int_ticks);
	WRITE_ARRAY(out, cs->timers);
	if (cs->save) return cs->save(cs, out);
	return 0;
}

int  cpu_load(struct SLOT_RUN_STATE*st, ISTREAM*in)
{
	struct CPU_STATE*cs = st->data;
	READ_FIELD(in, cs->fast_mode);
	READ_FIELD(in, cs->freq_6502);
	READ_FIELD(in, cs->undoc);
	READ_FIELD(in, cs->tsc_6502);
	READ_FIELD(in, cs->min_msleep);
	READ_FIELD(in, cs->lim_fetches);
	READ_ARRAY(in, cs->int_ticks);
	READ_ARRAY(in, cs->timers);
	if (cs->load) return cs->load(cs, in);
	return 0;
}

int cpu_cmd(struct CPU_STATE*cs, int cmd, int data, long param)
{
	if (cs->cmd) return cs->cmd(cs, cmd, data, param);
	return 0;
}

int  cpu_init(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*st, struct SLOTCONFIG*cf)
{
	struct CPU_STATE*cs;

	cs = calloc(1, sizeof(*cs));
	if (!cs) return -1;
	cs->sr = sr;
	cs->min_msleep = 30;
	cs->lim_fetches = 1000;
	cs->freq_6502 = cf->cfgint[CFG_INT_CPU_SPEED]*10;
	cs->undoc = cf->cfgint[CFG_INT_CPU_EXT];

	cs->wakeup = CreateEvent(NULL, FALSE, FALSE, NULL);
	cs->response = CreateEvent(NULL, FALSE, FALSE, NULL);

	switch (cf->dev_type) {
	case DEV_6502:
		init_cpu_6502(cs);
		break;
	case DEV_M6502:
		init_cpu_M6502(cs);
		break;
	case DEV_65C02:
		init_cpu_65c02(cs);
		break;
	}

	cs->sleep_req = 1;
	cs->th = CreateThread(NULL, 0, cpu_thread, cs, 0, &cs->thid);
	if (cs->th == NULL) {
		CloseHandle(cs->wakeup);
		cs->free(cs);
		free(cs);
		return -1;
	}
	WaitForSingleObject(cs->response, 5000);

	st->data = cs;
	st->free = cpu_term;
	st->save = cpu_save;
	st->load = cpu_load;
	st->command = cpu_command;
	return 0;
}


int cpu_hreset(struct CPU_STATE*st)
{
	st->intr(st, CPU_INTR_HRESET);
	return 0;
}

int cpu_intr(struct CPU_STATE*st, int t, int nticks)
{
	switch (t) {
	case CPU_INTR_IRQ:
		st->int_ticks[0] = nticks;
		break;
	case CPU_INTR_NMI:
		st->int_ticks[1] = nticks;
		break;
	case CPU_INTR_NOIRQ:
		st->int_ticks[0] = 0;
		break;
	case CPU_INTR_NONMI:
		st->int_ticks[1] = 0;
		break;
	case CPU_INTR_RESET:
		st->int_ticks[0] = 0;
		st->int_ticks[1] = 0;
		break;
	}
	st->intr(st, t);
	return 0;
}

int cpu_pause(struct CPU_STATE*st, int p)
{
	if (p) {
		if (st->sleep_req) return 1;
		ResetEvent(st->wakeup);
		st->sleep_req = 1;
		WaitForSingleObject(st->response, 1000);
		return 0;
	} else {
		if (!st->sleep_req) return 1;
		st->sleep_req = 0;
		SetEvent(st->wakeup);
	}
	return 0;
}

int cpu_step(struct CPU_STATE*st, int ncmd)
{
	int l = st->sleep_req;
	st->sleep_req = 1;
	ResetEvent(st->wakeup);
	for (;ncmd; --ncmd) {
		PulseEvent(st->wakeup);
	}
	st->sleep_req = l;
	return 0;
}


static void decrement_int(struct CPU_STATE*cs, int n, int id, int cmd)
{
	if (cs->int_ticks[id]) {
		if (cs->int_ticks[id] > n) cs->int_ticks[id] -= n;
		else {
			cpu_intr(cs, cmd, 0);
		}
	}
}

static void decrement_timers(struct CPU_STATE*cs, int n)
{
	struct CPU_TIMER*r;
	int i;

//	printf("decrement_timers: %i\n", n);

	decrement_int(cs, n, 0, CPU_INTR_NOIRQ);
	decrement_int(cs, n, 1, CPU_INTR_NONMI);

	for (r = cs->timers, i = MAX_CPU_TIMERS; i; --i, ++r)
//		printf("timer[%i]: used = %i; delay = %i; remains = %i\n", i, r->used, r->delay, r->remains);
		if (r->used && r->delay) {
			if (r->remains<=0) r->remains += r->delay;
			r->remains -= n;
			if (r->remains<=0) {
				system_command(cs->sr, SYS_COMMAND_CPUTIMER, 0, r->param);
			}
		}
}

static DWORD CALLBACK cpu_thread(struct CPU_STATE*cs)
{
	TCHAR bufs[2][256];
	unsigned t0=get_n_msec();
	long long n_ticks = 0;
	PulseEvent(cs->response);
	while (!cs->term_req) {
		int r, t, rt;
		while (cs->sleep_req) {
			unsigned ta = get_n_msec();
			PulseEvent(cs->response);
			WaitForSingleObject(cs->wakeup, INFINITE);
			t0 += get_n_msec() - ta;
			if (cs->term_req) return 0;
		}
//		puts("exec_op");
		if  (cs->hook_proc) {
			r = cs->hook_proc(cs->hook_data);
		} else r = 0;
		if (!r) r = cs->exec_op(cs);
		if (r > 0) {
			cs->tsc_6502 += r;
			n_ticks += r;
			if (cs->fast_boost > r) cs->fast_boost -= r;
			else cs->fast_boost = 0;
		}
		if (r < 0) {
			if (MessageBox(cs->sr->video_w, 
				localize_str(LOC_CPU, 1, bufs[0], sizeof(bufs[0])),
//				TEXT("Ошибка исполнения команды процессора. Продолжить?"),
				localize_str(LOC_CPU, 0, bufs[1], sizeof(bufs[1])),
//				TEXT("Процессор"), 
				MB_ICONEXCLAMATION | MB_YESNO) == IDNO) {
//					lstrcat(title, TEXT(" (неактивен)"));
					system_command(cs->sr, SYS_COMMAND_SET_STATUS_TEXT, -1, (long)localize_str(LOC_CPU, 2, bufs[0], sizeof(bufs[0])));
					cs->term_req = 1;
					break;
				}
		}
		decrement_timers(cs, r);
		if (n_ticks>=cs->lim_fetches||cs->need_cpusleep) {
			unsigned t=get_n_msec();
			unsigned rt=n_ticks/cs->freq_6502;
			if (rt-t+t0>cs->min_msleep) {
				if (rt>t-t0 && !cs->fast_mode && ! cs->fast_boost) {
 					msleep(rt-t+t0);
				}
				t0=t;
				n_ticks=0;
				cs->need_cpusleep=0;
			}
		}
	}
	return 0;
}


int cpu_get_tsc(struct SYS_RUN_STATE*sr)
{
	struct CPU_STATE*cs;
	cs = sr->slots[CONF_CPU].data;
	return cs->tsc_6502;
}

int cpu_get_freq(struct SYS_RUN_STATE*sr)
{
	struct CPU_STATE*cs;
	cs = sr->slots[CONF_CPU].data;
	return cs->freq_6502;
}

int cpu_get_fast(struct SYS_RUN_STATE*sr)
{
	struct CPU_STATE*cs;
	cs = sr->slots[CONF_CPU].data;
	return cs->fast_mode;
}
