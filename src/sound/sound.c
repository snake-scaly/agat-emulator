#include "soundint.h"

extern struct SOUNDPROC soundnone, soundacm, soundds, soundbeep;

static struct SOUNDPROC* procs[4]={
	&soundnone, &soundacm, &soundds, &soundbeep
};


#define TIMER_INTERVAL_MS 100

struct SOUNDDATA
{
	void	*data;
	struct SOUNDPROC*proc;
	int	timer_id;
	struct SYS_RUN_STATE*sr;

	DWORD	last_tick;
	long	last_tsc;
	long	sync_freq;
};


static void CALLBACK acm_timer(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2);



static struct SOUNDPROC* getproc(struct SLOT_RUN_STATE*st)
{
	struct SOUNDPROC*p = procs[0];
	switch (st->sc->dev_type) {
	case DEV_BEEPER:
		p = procs[3];
		break;
	case DEV_MMSYSTEM:
		p = procs[1];
		break;
	case DEV_DSOUND:
		p = procs[2];
		break;
	}
	return p;
}


static int sound_command(struct SLOT_RUN_STATE*st, int cmd, int data, long param)
{
	return 0;
}


static int sound_term(struct SLOT_RUN_STATE*st)
{
	struct SOUNDDATA* d0 = st->data;
	struct SOUNDPROC*p = d0->proc;
	if (!p) return -1;
	if (d0->timer_id) timeKillEvent(d0->timer_id);
	p->sound_term(d0->data);
	return 0;
}


static void sound_write(struct SLOT_RUN_STATE*st, int d)
{
	struct SOUNDDATA* d0 = st->data;
	struct SOUNDPROC*p = d0->proc;
	if (!p) return;
	p->sound_data(d0->data, d, 
		cpu_get_tsc(d0->sr), d0->sync_freq?d0->sync_freq:cpu_get_freq(d0->sr));
}

static void sound_toggle(struct SLOT_RUN_STATE*st)
{
	sound_write(st, SOUND_TOGGLE);
}

static void sound_w(word adr, byte data, struct SLOT_RUN_STATE*st) // C030-C03F
{
	sound_toggle(st);
}

static byte sound_r(word adr, struct SLOT_RUN_STATE*st) // C030-C03F
{
	sound_toggle(st);
	return empty_read(adr, st);
}

int sound_write_bit(struct SYS_RUN_STATE*sr, int v)
{
	sound_write(sr->slots + CONF_SOUND, v);
	return 0;
}

int  sound_init(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*st, struct SLOTCONFIG*cf)
{
	struct SOUNDPROC*p = getproc(st);
	struct SOUNDPARAMS par;
	struct SOUNDDATA *d0;
	void*data;
	par.w = sr->video_w;
	par.freq = cf->cfgint[CFG_INT_SOUND_FREQ]?cf->cfgint[CFG_INT_SOUND_FREQ]:44100;
	par.buflen = cf->cfgint[CFG_INT_SOUND_BUFSIZE]?cf->cfgint[CFG_INT_SOUND_BUFSIZE]:par.freq / 2;
	par.sr = sr;
	if (!p) return -1;
	d0 = calloc(sizeof(*d0), 1);
	if (!d0) return -3;
	data = p->sound_init(&par);
	if (!data) { free(d0); return -2; }

	if (p->sound_timer) {
		d0->timer_id = timeSetEvent(TIMER_INTERVAL_MS, 0, (LPTIMECALLBACK)acm_timer, (DWORD_PTR)d0, TIME_PERIODIC);
	        if (!d0->timer_id) {
	        	p->sound_term(data);
	        	free(d0);
	        	return -4;
	        }
	} else d0->timer_id = 0;

	puts("sound init ok");

	fill_read_proc(sr->baseio_sel + 3, 1, sound_r, st);
	fill_write_proc(sr->baseio_sel + 3, 1, sound_w, st);
	
	d0->data = data;
	d0->proc = p;
	d0->sr = sr;

	st->data = d0;
	st->free = sound_term;
	st->command = sound_command;
	return 0;
}

#define ABS(a) (((a) < 0) ? -(a): (a))

static void CALLBACK acm_timer(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	struct SOUNDDATA* d0 = (struct SOUNDDATA*)dwUser;
	DWORD tick = GetTickCount();
	long tsc = cpu_get_tsc(d0->sr);

/*	if (!d0->sync_freq) d0->sync_freq = cpu_get_freq(d0->sr);

	if (d0->last_tick) {
		long cpu_tick = (tsc - d0->last_tsc) / cpu_get_freq(d0->sr);
		long timer_tick = tick - d0->last_tick;
		long freq;
		freq = (tsc - d0->last_tsc) / timer_tick;
		if (freq < d0->sync_freq || ABS(freq - d0->sync_freq) > 50) d0->sync_freq = freq - 30;
//		printf("cpu_tick = %i; timer_tick = %i; freq = %i\n", cpu_tick, timer_tick, d0->sync_freq);
	}*/
	d0->last_tick = tick;
	d0->last_tsc = tsc;
	if (d0->proc->sound_timer) d0->proc->sound_timer(d0->data);
}
