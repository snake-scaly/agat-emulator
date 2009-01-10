#include "cpuint.h"


static DWORD CALLBACK cpu_thread(struct CPU_STATE*cs);

static int cpu_command(struct SLOT_RUN_STATE*st, int cmd, int data, long param)
{
	struct CPU_STATE*cs = st->data;
	int r;
	switch (cmd) {
	case SYS_COMMAND_FAST:
		r = cs->fast_mode;
		cs->fast_mode = data;
		printf("fast mode: %i\n", data);
		return r;
	case SYS_COMMAND_HRESET:
		return cpu_hreset(cs);
	case SYS_COMMAND_RESET:
		return cpu_intr(cs, CPU_INTR_HRESET);
	case SYS_COMMAND_IRQ:
		return cpu_intr(cs, CPU_INTR_IRQ);
	case SYS_COMMAND_NMI:
		return cpu_intr(cs, CPU_INTR_NMI);
	case SYS_COMMAND_START:
		cpu_pause(cs, 0);
		return 0;
	case SYS_COMMAND_STOP:
		cpu_pause(cs, 1);
		return 0;
	default:
		return cpu_cmd(cs, cmd, data, param);
	}
	return 0;
}

static int cpu_term(struct SLOT_RUN_STATE*st)
{
	struct CPU_STATE*cs = st->data;
	cs->term_req = 1;
	cs->sleep_req = 0;
	SetEvent(cs->wakeup);
	WaitForSingleObject(cs->th, 1000);
	TerminateThread(cs->th, 0);
	CloseHandle(cs->th);
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
	cs->min_msleep = 10;
	cs->lim_fetches = 1000;
	cs->freq_6502 = cf->cfgint[CFG_INT_CPU_SPEED]*10;
	cs->undoc = cf->cfgint[CFG_INT_CPU_EXT];

	cs->wakeup = CreateEvent(NULL, FALSE, FALSE, NULL);

	switch (cf->dev_type) {
	case DEV_6502:
		init_cpu_6502(cs);
		break;
	case DEV_M6502:
		init_cpu_M6502(cs);
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
	Sleep(100);

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

int cpu_intr(struct CPU_STATE*st, int t)
{
	st->intr(st, t);
	return 0;
}

int cpu_pause(struct CPU_STATE*st, int p)
{
	if (p) {
		if (st->sleep_req) return 1;
		ResetEvent(st->wakeup);
		st->sleep_req = 1;
		Sleep(100);
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


static DWORD CALLBACK cpu_thread(struct CPU_STATE*cs)
{
	unsigned t0=get_n_msec();
	int n_ticks = 0;
	while (!cs->term_req) {
		int r, t, rt;
		while (cs->sleep_req) {
			unsigned ta = get_n_msec();
			WaitForSingleObject(cs->wakeup, INFINITE);
			t0 += get_n_msec() - ta;
			if (cs->term_req) return 0;
		}
//		puts("exec_op");
		r = cs->exec_op(cs);
		if (r > 0) {
			cs->tsc_6502 += r;
			n_ticks += r;
		}
		if (r < 0) {
			if (MessageBox(cs->sr->video_w, 
				TEXT("Ошибка исполнения команды процессора. Продолжить?"),
				TEXT("Процессор"), MB_ICONEXCLAMATION | MB_YESNO) == IDNO) {
					TCHAR title[1024];
					GetWindowText(cs->sr->video_w, title, 1024);
					lstrcat(title, TEXT(" (неактивен)"));
					SetWindowText(cs->sr->video_w, title);
					cs->term_req = 1;
					break;
				}
		}
		if (n_ticks>=cs->lim_fetches||cs->need_cpusleep) {
			unsigned t=get_n_msec();
			unsigned rt=n_ticks/cs->freq_6502;
			if (rt-t+t0>cs->min_msleep) {
				if (rt>t-t0 && !cs->fast_mode) {
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
