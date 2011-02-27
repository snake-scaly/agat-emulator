#include "tapeint.h"
#define _USE_MATH_DEFINES
#include <math.h>

#define TAPE_TIMEOUT 500

#define PREFIX_MIN_DELAY 400
#define PREFIX_MAX_DELAY 800
#define PREFIX_NTEST 5

static void tape_in_finish(struct TAPE_STATE*st);
static void tape_out_finish(struct TAPE_STATE*st);

static VOID CALLBACK tape_timer_proc(HWND hwnd, UINT msg, UINT id, DWORD dwt);


static void start_timer(struct TAPE_STATE*st)
{
	if (!st->tm) {
		SetTimer(st->w, (DWORD)st, TAPE_TIMEOUT, tape_timer_proc);
		if (st->use_fast) system_command(st->sr, SYS_COMMAND_FAST, 1, 0);
	}
	++st->tm;
}

static void stop_timer(struct TAPE_STATE*st)
{
	if (!st->tm) return;
	--st->tm;
	if (!st->tm) {
		KillTimer(st->w, (DWORD)st);
		if (st->use_fast) system_command(st->sr, SYS_COMMAND_FAST, 0, 0);
	}
}


static VOID CALLBACK tape_timer_proc(HWND hwnd, UINT msg, UINT id, DWORD dwt)
{
	struct TAPE_STATE*st = (struct TAPE_STATE*)id;
	puts("tape_timer_proc");
	if (st->lastmsec + TAPE_TIMEOUT < get_n_msec()) {
		tape_in_finish(st);
		tape_out_finish(st);
	}
}


static int tape_out_init(struct TAPE_STATE*st, int tsc)
{
	struct S_AUDIO_FORMAT fmt;
	int r;
	r = select_tape(st->w, st->basename, 0);
	if (!r) {
		st->out_disabled = 1;
		start_timer(st);
		return -1; 
	}
	st->f = fopen(st->basename, "wb");
	if (!st->f) return -1;
	fmt.format_tag = 1;
	fmt.n_channels = 1;
	fmt.n_samples_per_sec = st->freq;
	fmt.block_align = 1;
	fmt.n_bytes_per_sec = fmt.block_align * fmt.n_samples_per_sec;
	fmt.bits_per_sample = fmt.block_align * 8;
	fmt.extra_size = 0;
	begin_wave_output(&st->wo, st->f, &fmt, 0x10);
	start_timer(st);
	st->val = 0;
	st->tw = tsc;
	return 0;
}


static int tape_in_init(struct TAPE_STATE*st, int tsc)
{
	struct S_AUDIO_FORMAT fmt;
	int r;
	puts("tape_in_init");
	r = select_tape(st->w, st->basename, 1);
	if (!r) {
		st->in_disabled = 1;
		start_timer(st);
		return -1;
	}
	st->in = fopen(st->basename, "rb");
	if (!st->in) return -1;
	r = wave_load(st->in, &st->wi);
	if (r < 0) { fclose(st->in); st->in = NULL; return -1; }
	if (st->wi.format->format_tag != 1) {
		wave_file_free(&st->wi);
		wave_file_clear(&st->wi);
		fclose(st->in);
		st->in = NULL;
		return -1;
	}
	start_timer(st);
	fseek(st->in, st->wi.data_start, SEEK_SET);
	st->insmp = -1;
	st->inlen = st->wi.data_size / st->wi.format->block_align;
	st->tr = tsc;
	st->inpsmp = -1;
	st->inprc = -1;
	return 0;
}

static void tape_out_finish(struct TAPE_STATE*st)
{
	puts("tape_out_finish");
	if (st->out_disabled) {
		st->out_disabled = 0;
		stop_timer(st);
	}
	if (!st->f) return;
	stop_timer(st);
	finish_wave_output(&st->wo, st->f);
	fclose(st->f);
	st->f = NULL;
}

static void tape_in_finish(struct TAPE_STATE*st)
{
	puts("tape_in_finish");
	if (st->in_disabled) {
		st->in_disabled = 0;
		stop_timer(st);
	}
	if (!st->in) return;
	stop_timer(st);
	wave_file_free(&st->wi);
	wave_file_clear(&st->wi);
	fclose(st->in);
	st->in = NULL;
}

static double xcos(double t)
{
	return 2 * pow(1 - pow(sin(t / 2), 8), 7) - 1;
}


static void tape_out_samples(struct TAPE_STATE*st, int pval, int val, int nsmp)
{
	if (!nsmp) {
		puts("Missing samples!!!");
	}
//	if (nsmp == 1 || st->freq < 22050) {
		for (;nsmp; --nsmp)
			fwrite(&val, 1, 1, st->f);
/*	} else {
		double dt = M_PI / (nsmp - 1), t = 0;
		for (;nsmp; --nsmp, t+=dt) {
			double c = xcos(t);
			int v = (1 + c)/2 * pval + (1 - c)/2 * val;
			fwrite(&v, 1, 1, st->f);
		}
	}*/
}

static void tape_out(struct TAPE_STATE*st, int tsc, int freq)
{
	int cursmp;
	int nsmp;
	int val0;
	if (st->out_disabled || st->in) {
		st->lastmsec = get_n_msec();
		return;
	}
//	puts("tape_out");
	if (!st->f) {
		int d = tsc - st->tw;
		st->tw = tsc;
		if (d < PREFIX_MIN_DELAY || d > PREFIX_MAX_DELAY) { // block header detect
			st->ntest = 0;
			st->tw = tsc;
			return;
		} else {
			++ st->ntest;
			if (st->ntest > PREFIX_NTEST) {
				int r = tape_out_init(st, tsc);
				st->ntest = 0;
				if (r < 0) return;
			} else return;
		}
	}
	cursmp = (tsc-st->tw)*(double)st->freq/1000.0/(double)freq;
	if (cursmp < st->prevsmp || !st->prevsmp) { st->prevsmp=cursmp-1; }
	nsmp=cursmp - st->prevsmp;
	st->prevsmp = cursmp;
	val0 = st->val;
	st->val = st->val ^ 0xFF;
	st->lastmsec = get_n_msec();
//	printf("tape: out: %i samples %x\n", nsmp, ((st->val > 0x7F)?0xFF:0x7F));
	tape_out_samples(st, val0, st->val, nsmp);
}

static byte get_sample(struct TAPE_STATE*st)
{
	byte r = 0;
	switch (st->wi.format->bits_per_sample) {
	case 8:
		fread(&r, 1, 1, st->in);
		fseek(st->in, st->wi.format->block_align - 1, SEEK_CUR);
		break;
	case 16:
		fseek(st->in, 1, SEEK_CUR);
		fread(&r, 1, 1, st->in);
		fseek(st->in, st->wi.format->block_align - 2, SEEK_CUR);
		r += 0x80;
		break;
	}
	return r;
}

static byte tape_in(struct TAPE_STATE*st, int tsc, int freq)
{
	int nsmp, cursmp;
	byte r = 0;
	int prc;
//	int i;
//	int nn[2]={0, 0};

	if (st->in_disabled || (st->sr->cursystype == SYSTEM_1 && st->f)) {
		st->lastmsec = get_n_msec();
		return st->val;
	}
	if (!st->in) {
		int r = tape_in_init(st, tsc);
		if (r < 0) return st->val;
	}
//	puts("tape_in");
	cursmp = (tsc - st->tr)*(double)st->wi.format->n_samples_per_sec/1000.0/(double)freq;
	if (cursmp < st->insmp || st->inpsmp==-1) { st->inpsmp=cursmp-1; }
	nsmp = cursmp - st->inpsmp;
	st->inpsmp = cursmp;
//	printf("nsmp = %i, insmp = %i, inlen = %i\n", nsmp, st->insmp, st->inlen);
	if (nsmp > st->inlen - st->insmp)
		nsmp = st->inlen - st->insmp;
	if (nsmp <= 0) return st->val;
	fseek(st->in, (nsmp - 1) * st->wi.format->block_align, SEEK_CUR);
	r = get_sample(st);
/*	for (i = 0; i < nsmp; ++i ) {
		int id, n;
		n = get_sample(st);
		if (n > 0x7F) id = 1; else id = 0;
		++ nn[id];
	}*/
	st->insmp += nsmp;
	prc = st->insmp * 100 / st->inlen;
	if (st->inprc!=prc) {
		printf("read %i%%    \r", prc);
		st->inprc = prc;
	}
	st->lastmsec = get_n_msec();
//	return st->val = ((nn[1] > nn[0])?0xFF:0x7F);
//	printf("tape: in: %i samples %x\n", nsmp, ((r > 0x7F)?0xFF:0x7F));
	return st->val = ((r > 0x7F)?0xFF:0x7F);
}

void tape_toggle(struct SLOT_RUN_STATE*st)
{
	if (!st->data) return;
	tape_out(st->data, cpu_get_tsc(st->sr), cpu_get_freq(st->sr));
}

byte tape_read(struct SLOT_RUN_STATE*st)
{
	if (!st->data) return 0xFF;
	return tape_in(st->data, cpu_get_tsc(st->sr), cpu_get_freq(st->sr));
}


static void tape_out_w(word adr, byte data, struct SLOT_RUN_STATE*st) // C020-C02F
{
	tape_toggle(st);
}

static byte tape_out_r(word adr, struct SLOT_RUN_STATE*st) // C020-C02F
{
	tape_toggle(st);
	return empty_read(adr, st);
}

static byte tape_in_r(word adr, struct SLOT_RUN_STATE*st) // C060
{
	return tape_read(st);
}

static int tape_command(struct SLOT_RUN_STATE*st, int cmd, int data, long param)
{
	struct TAPE_STATE*tst = st->data;
	switch (cmd) {
	case SYS_COMMAND_HRESET:
	case SYS_COMMAND_RESET:
	case SYS_COMMAND_STOP:
		tape_in_finish(tst);
		tape_out_finish(tst);
		return 0;
	}
	return 0;
}

static int tape_term(struct SLOT_RUN_STATE*st)
{
	struct TAPE_STATE*tst = st->data;
	tape_in_finish(tst);
	tape_out_finish(tst);
	free(tst);
	return 0;
}


int  tape_init(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*st, struct SLOTCONFIG*sc)
{
	struct TAPE_STATE*tst;
	if (sc->dev_type != DEV_TAPE_FILE) {
		return 0;
	}
	tst = calloc(1, sizeof(*tst));
	if (!tst) return -1;
	tst->w = sr->video_w;
	tst->sr = sr;
	tst->freq = sc->cfgint[CFG_INT_TAPE_FREQ];
	tst->use_fast = sc->cfgint[CFG_INT_TAPE_FAST];
	_tcscpy(tst->basename, sc->cfgstr[CFG_STR_TAPE]);
	fill_read_proc(sr->baseio_sel + 2, 1, tape_out_r, st);
	fill_write_proc(sr->baseio_sel + 2, 1, tape_out_w, st);
	fill_read_proc(sr->io6_sel + 0, 1, tape_in_r, st);

	st->data = tst;
	st->free = tape_term;
	st->command = tape_command;

	return 0;
}
