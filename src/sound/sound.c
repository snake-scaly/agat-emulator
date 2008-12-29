#include "soundint.h"

extern struct SOUNDPROC soundnone, soundacm, soundds;

static struct SOUNDPROC* procs[3]={
	&soundnone, &soundacm, &soundds
};


static struct SOUNDPROC* getproc(struct SLOT_RUN_STATE*st)
{
	struct SOUNDPROC*p = procs[0];
	switch (st->sc->dev_type) {
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
	struct SOUNDPROC*p = getproc(st);
	if (!p) return -1;
	p->sound_term(st->data);
	return 0;
}

static void sound_toggle(struct SLOT_RUN_STATE*st)
{
	struct SOUNDPROC*p = getproc(st);
//	puts("sound_toggle");
	if (!p) return;
	p->sound_data(st->data, SOUND_TOGGLE, cpu_get_tsc(st->sr), cpu_get_freq(st->sr));
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


int  sound_init(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*st, struct SLOTCONFIG*cf)
{
	struct SOUNDPROC*p = getproc(st);
	struct SOUNDPARAMS par;
	void*data;
	par.w = sr->video_w;
	par.freq = 44100;
	par.buflen = 20000;
	if (!p) return -1;
	data = p->sound_init(&par);
	if (!data) return -2;
	puts("sound init ok");

	fill_read_proc(sr->baseio_sel + 3, 1, sound_r, st);
	fill_write_proc(sr->baseio_sel + 3, 1, sound_w, st);
	
	st->data = data;
	st->free = sound_term;
	st->command = sound_command;
	return 0;
}

