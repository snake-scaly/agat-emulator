/*
	Agat Emulator version 1.0
	Copyright (c) NOP, nnop@newmail.ru
	sound_acm - sound output via ACM library [incomplete]
*/

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include "soundint.h"
#include "debug.h"

#pragma comment(lib,"winmm")

//#define DEBUG_SOUND

#ifdef DEBUG_SOUND
#define Sprintf(x) printf x
#else
#define Sprintf(x)
#endif

//#define Qprintf(...) printf(__VA_ARGS__)
#define Qprintf(...)

#define FIRST_DELAY 300
#define POST_BUFFERS 2

#define N_BUFFERS 5

typedef unsigned char sample_t;
#define MIN_SAMPLE 0x00
#define MAX_SAMPLE 0xFF
#define XOR_SAMPLE (MAX_SAMPLE ^ MIN_SAMPLE)
#define MID_SAMPLE ((MIN_SAMPLE + MAX_SAMPLE) >> 1)

struct ACM_DATA;

struct ACM_BUFFER
{
	struct ACM_DATA*p;
	WAVEHDR hdr;
	sample_t *buf;
	int	next;
};

struct ACM_DATA
{
	struct SYS_RUN_STATE*sr;
	HWAVEOUT out;

	int	maxlen;

	sample_t cur_val;
	long	prev_tick;
	int	freq;
	int	timer_id;

	struct ACM_BUFFER buf[N_BUFFERS];


	double  flen, fsum; // fractional length and summ of weighted sample values
	int	pending; // samples pending to play

	int	firstbuf; // first of pending buffer number
	int	curbuf; // current buffer number
	int	prevbuf; // previous buffer for reference
	int	curofs; // current buffer offset in samples

	int	nbuf_playing, nbuf_ready;

	CRITICAL_SECTION crit;
};

static void CALLBACK out_proc(
	HWAVEOUT hwo,      
	UINT uMsg,         
	DWORD dwInstance,  
	DWORD dwParam1,    
	DWORD dwParam2
);

static void post_cur_buffer(struct ACM_DATA*p, int force);

#define LOCK(p) \
	EnterCriticalSection(&(p)->crit);

#define UNLOCK(p) \
	LeaveCriticalSection(&(p)->crit);

#define fill_smps memset
/*
static void fill_smps(sample_t*buf, sample_t smp, int nsmp)
{
}
*/

static int get_t()
{
	static int msec;
	int r = GetTickCount() - msec;
	if (!msec) { msec = r; return 0; }
	return r;
}

static int get_msec_from_len(struct ACM_DATA*p, int len)
{
	return len / sizeof(sample_t) * 1000 / p->freq;
}


static int get_len_from_msec(struct ACM_DATA*p, int msec)
{
	return msec * sizeof(sample_t) * p->freq / 1000;
}


static void* sound_init(struct SOUNDPARAMS*par)
{
	struct ACM_DATA*p;
	WAVEFORMATEX fmt;
	int i, j;

	p = calloc(1, sizeof(*p));
	if (!p) goto err;

	p->sr = par->sr;
	p->cur_val = MIN_SAMPLE;

	p->curbuf = -1;
	p->prevbuf = -1;
	p->firstbuf = -1;

	p->maxlen = par->buflen * par->freq / 11050;

	p->freq = par->freq;

	printf("sound freq = %i; buflen = %i\n", p->freq, p->maxlen);

	InitializeCriticalSection(&p->crit);

	fmt.wFormatTag = WAVE_FORMAT_PCM;
	fmt.nChannels = 1;
	fmt.nSamplesPerSec = p->freq;
	fmt.nBlockAlign = sizeof(sample_t);
	fmt.wBitsPerSample = sizeof(sample_t) * 8;
	fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;

	if (ACM_FAILED(waveOutOpen(&p->out,WAVE_MAPPER,&fmt,(DWORD)par->w,0,CALLBACK_WINDOW),TEXT("opening wave out"))) {
		p->out = NULL;
		return p;
	}

	for (i = 0; i < N_BUFFERS; ++i) {
		p->buf[i].p = p;
		p->buf[i].buf = malloc(p->maxlen * sizeof(sample_t));
		if (!p->buf[i].buf) {
			goto err1;
		}
		fill_smps(p->buf[i].buf, p->cur_val, p->maxlen);
		p->buf[i].hdr.lpData = p->buf[i].buf;
		p->buf[i].hdr.dwBufferLength = p->maxlen * sizeof(sample_t);
		p->buf[i].hdr.dwFlags = 0;
		p->buf[i].hdr.dwUser = i;
		if (ACM_FAILED(waveOutPrepareHeader(p->out, &p->buf[i].hdr, sizeof(p->buf[i].hdr)), TEXT("preparing waveout header"))) { ++i; goto err1; }
		p->buf[i].hdr.dwFlags |= WHDR_DONE;
	}

	Sprintf(("acm: sound initialized\n"));
	return p;
err1:
	for (j = 0; j < i; ++j) {
		waveOutUnprepareHeader(p->out, &p->buf[j].hdr, sizeof(p->buf[j].hdr));
		free(p->buf[j].buf);
	}
	waveOutClose(p->out);
err0:
	free(p);
err:
	Sprintf(("acm: sound initialization failed\n"));
	return NULL;
}

static void sound_term(struct ACM_DATA*p)
{
	HWAVEOUT out;
	if (!p) return;
	p->curbuf = -1;
	LOCK(p);
	out = p->out;
	p->out = NULL;
	if (p->timer_id) timeKillEvent(p->timer_id);
	UNLOCK(p);
	if (out) {
		int i;
		LOCK(p);
		for (i = 0; i < N_BUFFERS; ++i) {
			p->buf[i].next = -1;
		}
		UNLOCK(p);
		waveOutReset(out);
		for (i = 0; i < N_BUFFERS; ++i) {
			p->buf[i].next = -1;
			waveOutUnprepareHeader(out, &p->buf[i].hdr, sizeof(p->buf[i].hdr));
			if (p->buf[i].buf) free(p->buf[i].buf);
		}
		waveOutClose(out);
	}
	DeleteCriticalSection(&p->crit);
	free(p);
	Sprintf(("acm: sound uninitialized\n"));
}

static int allocate_buffer(struct ACM_DATA*p)
{
	int i;
	for (i = 0; i < N_BUFFERS; ++i) {
		if (!(p->buf[i].hdr.dwFlags & WHDR_DONE)) continue;
		p->buf[i].hdr.dwBufferLength = 0;
		p->buf[i].hdr.dwFlags &= ~WHDR_DONE;
		return i;
	}
	return -1;
}

static void on_push_timeout(struct ACM_DATA*p)
{
	LOCK(p);
	if (!p->out) {
		UNLOCK(p);
		return;
	}
	p->timer_id = 0;
	Qprintf("%i:  push_timeout, pending = %i, nbuf_playing = %i, nbuf_ready = %i\n",
		get_t(),
		p->pending, p->nbuf_playing, p->nbuf_ready);
	if ((p->pending || p->nbuf_ready) && !p->nbuf_playing) {
		Qprintf("posting from timer\n");
		post_cur_buffer(p, 1);
	}
	UNLOCK(p);
}

static void on_end_buffer(struct ACM_DATA*p, int bufno)
{
	LOCK(p);
	if (!p->out) {
		UNLOCK(p);
		return;
	}
	Qprintf("%i: buffer %i: end_buffer, pending = %i, nbuf_playing = %i, nbuf_ready = %i\n",
		get_t(),
		bufno, p->pending, p->nbuf_playing, p->nbuf_ready);
	if ((p->pending || p->nbuf_ready) && p->nbuf_playing < 2) {
		Qprintf("posting from end_buffer\n");
		post_cur_buffer(p, 1);
	}
	UNLOCK(p);
}


static void CALLBACK finish_timer(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	struct ACM_DATA*p = (struct ACM_DATA*)dwUser;
//	printf("acm: finish_timer\n");
	on_push_timeout(p);
}

static int set_timer(struct ACM_DATA*p)
{
	Qprintf("%i: set timer for %i msec\n", get_t(), FIRST_DELAY);
	p->timer_id = timeSetEvent(FIRST_DELAY, 0, finish_timer, 
		(DWORD)p, TIME_ONESHOT);
	if (!p->timer_id) {
		fprintf(stderr, "timer: set event failed\n");
	}
	return 0;
}

static void post_queued_buffers(struct ACM_DATA*p)
{
	if (p->timer_id) {
		Qprintf("killing timer...\n");
		timeKillEvent(p->timer_id);
		p->timer_id = 0;
	}
	for (; p->firstbuf != -1; p->firstbuf = p->buf[p->firstbuf].next) {
		int n = p->firstbuf;
//		printf("post_queued_buffers: firstbuf = %i, curbuf = %i, prevbuf = %i, ready = %i, playing = %i\n", p->firstbuf, p->curbuf, p->prevbuf, p->nbuf_ready, p->nbuf_playing);
		Qprintf("%i: buffer %i: posted for %i msec\n", get_t(), n, get_msec_from_len(p, p->buf[n].hdr.dwBufferLength));
		if (!p->buf[n].hdr.dwBufferLength) continue;
		if (ACM_FAILED(waveOutWrite(p->out, &p->buf[n].hdr, sizeof(p->buf[n].hdr)), TEXT("waveout write"))) break;
		InterlockedIncrement(&p->nbuf_playing);
		InterlockedDecrement(&p->nbuf_ready);
	}
}

static void post_cur_buffer(struct ACM_DATA*p, int force)
{
	int n = p->curbuf;
	if (p->curbuf == -1) return;
	if (p->firstbuf == -1) p->firstbuf = p->curbuf;
	if (p->prevbuf != -1) p->buf[p->prevbuf].next = p->curbuf;
	p->buf[p->curbuf].next = -1;
	p->prev_tick = 0;
//	printf("acm: posting current buffer %i with size %i samples (%i msec)\n", p->curbuf, p->curofs, p->curofs * 1000 / p->freq);
	p->pending -= p->curofs;
	p->buf[n].hdr.dwBufferLength = p->curofs * sizeof(sample_t);
	Qprintf("%i: buffer %i: enqueued for %i msec\n", get_t(), n, get_msec_from_len(p, p->buf[n].hdr.dwBufferLength));
	InterlockedIncrement(&p->nbuf_ready);
	if (!force && !p->nbuf_playing) {
		if (p->nbuf_ready >= POST_BUFFERS) post_queued_buffers(p);
	} else {
		post_queued_buffers(p);
	}
	p->prevbuf = p->curbuf;
	p->curbuf = -1;
	return;
err:
	p->curbuf = -1;
	return;
}



static void sound_write(struct ACM_DATA*p, sample_t val, int nsmp)
{
	int ntry = 10, n;
	Sprintf(("acm: sound_write %x, count = %i\n", val, nsmp));
	Sprintf(("acm: pending = %i, curofs = %i\n", p->pending, p->curofs));
	while (nsmp > 0) {
		int sz = nsmp;
		int maxlen;
		if (p->curbuf == -1) {
			for (n = ntry; n; --n) {
				p->curbuf = allocate_buffer(p);
				if (p->curbuf != -1) break;
				if (cpu_get_fast(p->sr)) return;
				UNLOCK(p);
				msleep(30);
				Qprintf("acm: waiting for buffer to free...\n");
				LOCK(p);
				if (!p->out) return;
			}
			if (p->curbuf == -1) {
				printf("acm: no free buffers to allocate!\n");
				return;
			}
//			printf("acm: allocated new empty buffer %i\n", p->curbuf);
			if (!p->pending && !p->nbuf_ready && !p->nbuf_playing)
				set_timer(p);
			p->curofs = 0;
		}
		maxlen = p->maxlen;
		if (!p->nbuf_playing) {
			maxlen = (FIRST_DELAY / POST_BUFFERS - FIRST_DELAY / 50) * p->freq / 1000 * sizeof(sample_t);
			if (maxlen > p->maxlen) maxlen = p->maxlen;
		}
		if (maxlen < p->curofs) maxlen = p->curofs;
		if (sz > maxlen - p->curofs) sz = maxlen - p->curofs;
		fill_smps(p->buf[p->curbuf].buf + p->curofs, val, sz);
		p->curofs += sz;
		nsmp -= sz;
		p->pending += sz;
		if (p->curofs == maxlen) post_cur_buffer(p, 0);
	}
	Sprintf(("acm: end sound_write\n"));
}

static int sound_data(struct ACM_DATA*p, int val, long t, long f)
{
	int maxnsmp = p->maxlen * 2;
	int lpending = p->pending;
	double fsmp, mult = 1.0;
	if (!p || !p->out) return -1;
	if (val == SOUND_TOGGLE) val = (p->cur_val ^= XOR_SAMPLE);
	else p->cur_val = val;
	Sprintf(("acm: sound_data: %i\n", val));
	if (t < p->prev_tick || !p->prev_tick || !p->pending) {
		p->prev_tick = t - 1;
		fsmp = 1;
	} else {
		fsmp = (t - p->prev_tick) * (double)p->freq/1000.0/(double)f * mult;
	}
//	printf("delta_t = %i, nsmp = %i\n", t - p->prev_tick, nsmp);
	if (fsmp > maxnsmp) { fsmp = 1; }
	if (p->flen) {
		if (p->flen + fsmp > 1) {
			double t = 1 - p->flen;
			p->fsum += ((double)val - MID_SAMPLE) / (double)(MAX_SAMPLE - MID_SAMPLE) * t;
			LOCK(p);
			sound_write(p, p->fsum * (double)(MAX_SAMPLE - MID_SAMPLE) + MID_SAMPLE, 1);
			UNLOCK(p);
			fsmp -= t;
			p->fsum = 0;
			p->flen = 0;
		} else {
			p->fsum += ((double)val - MID_SAMPLE) / (double)(MAX_SAMPLE - MID_SAMPLE) * fsmp;
			p->flen += fsmp;
			fsmp = 0;
			goto fin;
		}
	}
	
	// at this point flen = 0 always
	if (fsmp > 1) {
		LOCK(p);
		sound_write(p, val, (int)fsmp);
		UNLOCK(p);
		fsmp -= (int)fsmp;
	}
	// at this point fsmp < 1 always
	p->fsum = ((double)val - MID_SAMPLE) / (double)(MAX_SAMPLE - MID_SAMPLE) * fsmp;
	p->flen = fsmp;
fin:
	p->prev_tick = t;

	return 0;
}

static int sound_event(struct ACM_DATA*p, HWAVEOUT out, WAVEHDR*hdr)
{
	if (out != p->out) return 0;
	InterlockedDecrement(&p->nbuf_playing);
	on_end_buffer(p, hdr->dwUser);
	return 1;
}



struct SOUNDPROC soundacm={ sound_init, sound_term, sound_data, sound_event};
