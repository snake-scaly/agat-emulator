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

#define TIMEOUT_MSEC	50

#define FIRST_DELAY 300

#define N_BUFFERS 4

typedef unsigned char sample_t;
#define MIN_SAMPLE 0x00
#define MAX_SAMPLE 0xFF
#define XOR_SAMPLE (MAX_SAMPLE ^ MIN_SAMPLE)
#define MID_SAMPLE ((MIN_SAMPLE + MAX_SAMPLE) >> 1)

struct ACM_DATA
{
	struct SYS_RUN_STATE*sr;
	HWAVEOUT out;

	WAVEHDR hdrs[N_BUFFERS];
	sample_t *bufs[N_BUFFERS];
	int	maxlen;

	sample_t cur_val;
	long	prev_tick;
	int	freq;

	int	timer_id;

	double  flen, fsum; // fractional length and summ of weighted sample values
	int	pending; // samples pending to play

	int	curbuf; // current buffer number
	int	prevbuf; // previous buffer for reference
	int	curofs; // current buffer offset in samples

	int	bufrefs[N_BUFFERS];

	int	nbuf_playing;

	DWORD	last_append;

	CRITICAL_SECTION crit;
};

static void CALLBACK out_proc(
  HWAVEOUT hwo,      
  UINT uMsg,         
  DWORD dwInstance,  
  DWORD dwParam1,    
  DWORD dwParam2
);

static void post_cur_buffer(struct ACM_DATA*p);


#define fill_smps memset
/*
static void fill_smps(sample_t*buf, sample_t smp, int nsmp)
{
}
*/


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

	p->maxlen = par->buflen;

	p->freq = par->freq;

	printf("sound freq = %i; buflen = %i\n", p->freq, p->maxlen);

	InitializeCriticalSection(&p->crit);

	fmt.wFormatTag = WAVE_FORMAT_PCM;
	fmt.nChannels = 1;
	fmt.nSamplesPerSec = p->freq;
	fmt.nBlockAlign = sizeof(sample_t);
	fmt.wBitsPerSample = sizeof(sample_t) * 8;
	fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;

	if (ACM_FAILED(waveOutOpen(&p->out,WAVE_MAPPER,&fmt,(DWORD)out_proc,(DWORD_PTR)p,CALLBACK_FUNCTION),TEXT("opening wave out"))) {
		p->out = NULL;
		return p;
	}

	for (i = 0; i < N_BUFFERS; ++i) {
		p->bufs[i] = malloc(p->maxlen * sizeof(sample_t));
		if (!p->bufs[i]) {
			goto err1;
		}
		fill_smps(p->bufs[i], p->cur_val, p->maxlen);
		p->hdrs[i].lpData = p->bufs[i];
		p->hdrs[i].dwBufferLength = p->maxlen * sizeof(sample_t);
		p->hdrs[i].dwFlags = 0;
		p->hdrs[i].dwUser = i;
		if (ACM_FAILED(waveOutPrepareHeader(p->out, p->hdrs + i, sizeof(*p->hdrs)), TEXT("preparing waveout header"))) { ++i; goto err1; }
		p->hdrs[i].dwFlags |= WHDR_DONE;
	}

	Sprintf(("acm: sound initialized\n"));
	return p;
err1:
	for (j = 0; j < i; ++j) {
		waveOutUnprepareHeader(p->out, p->hdrs + j, sizeof(p->hdrs[j]));
		free(p->bufs[j]);
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
	if (!p) return;
	p->curbuf = -1;
	if (p->timer_id) timeKillEvent(p->timer_id);
	if (p->out) {
		int i;
		for (i = 0; i < N_BUFFERS; ++i) {
			p->bufrefs[i] = -1;
		}
		waveOutReset(p->out);
		for (i = 0; i < N_BUFFERS; ++i) {
			p->bufrefs[i] = -1;
			waveOutUnprepareHeader(p->out, p->hdrs + i, sizeof(p->hdrs[i]));
			if (p->bufs[i]) free(p->bufs[i]);
		}
		waveOutClose(p->out);
		p->out = NULL;
	}
	if (p->timer_id) timeKillEvent(p->timer_id);
	DeleteCriticalSection(&p->crit);
	free(p);
	Sprintf(("acm: sound uninitialized\n"));
}

static int allocate_buffer(struct ACM_DATA*p)
{
	int i;
	for (i = 0; i < N_BUFFERS; ++i) {
		if (!(p->hdrs[i].dwFlags & WHDR_DONE)) continue;
		p->hdrs[i].dwBufferLength = 0;
		return i;
	}
	return -1;
}

static void CALLBACK finish_timer(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	struct ACM_DATA*p = (struct ACM_DATA*)dwUser;
//	printf("acm: finish_timer\n");
	if (!p->out) return;
	EnterCriticalSection(&p->crit);
		if (p->pending) {
//			if (p->nbuf_playing < 2)
//			system_command(p->sr, SYS_COMMAND_BOOST, 10000, 0);
			Sprintf(("posting from timer\n"));
			post_cur_buffer(p);
		}
	LeaveCriticalSection(&p->crit);
	p->timer_id = 0;
}

static void set_timer_for_buf(struct ACM_DATA*p, WAVEHDR*h)
{
	int msec;
	if (!h) msec = FIRST_DELAY;
	else msec = h->dwBufferLength / sizeof(sample_t) * 900 / p->freq - 20;
	if (msec <= 0) msec = 1;
//	printf("acm: set_timer = %i msec (buf = %i)\n", msec, h?h->dwUser:-1);
	p->timer_id = timeSetEvent(msec, 0, finish_timer, (DWORD)p, TIME_ONESHOT);
	if (!p->timer_id) {
		fprintf(stderr, "timer: set event failed\n");
	}
}

static void post_cur_buffer(struct ACM_DATA*p)
{
	int n = p->curbuf;
	if (p->curbuf == -1) return;
	if (p->prevbuf != -1) p->bufrefs[p->prevbuf] = p->curbuf;
	p->bufrefs[p->curbuf] = -1;
	p->prev_tick = 0;
//	printf("acm: posting current buffer %i with size %i samples (%i msec)\n", p->curbuf, p->curofs, p->curofs * 1000 / p->freq);
	p->pending -= p->curofs;
	p->hdrs[n].dwBufferLength = p->curofs * sizeof(sample_t);
	if (!p->nbuf_playing) set_timer_for_buf(p, p->hdrs + n);
	if (ACM_FAILED(waveOutWrite(p->out, p->hdrs + n, sizeof(*p->hdrs)), TEXT("waveout write"))) goto err;
	InterlockedIncrement(&p->nbuf_playing);
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
		if (p->curbuf == -1) {
			for (n = ntry; n; --n) {
				p->curbuf = allocate_buffer(p);
				if (p->curbuf != -1) break;
				msleep(30);
			}
			if (p->curbuf == -1) {
				Sprintf(("acm: no free buffers to allocate!\n"));
				return;
			}
			Sprintf(("acm: allocated new empty buffer %i\n", p->curbuf));
			p->curofs = 0;
		}
		if (sz > p->maxlen - p->curofs) sz = p->maxlen - p->curofs;
		fill_smps(p->bufs[p->curbuf] + p->curofs, val, sz);
		p->curofs += sz;
		nsmp -= sz;
		p->pending += sz;
		if (p->curofs == p->maxlen) post_cur_buffer(p);
	}
	Sprintf(("acm: end sound_write\n"));
}

static int sound_data(struct ACM_DATA*p, int val, long t, long f)
{
	int maxnsmp = p->maxlen * 2;
	double fsmp, mult = 1.00;
	if (!p || !p->out) return -1;
	if (val == SOUND_TOGGLE) val = (p->cur_val ^= XOR_SAMPLE);
	else p->cur_val = val;
	Sprintf(("acm: sound_data: %i\n", val));
	if (!p->pending && !p->nbuf_playing && !p->flen) {
//		system_command(p->sr, SYS_COMMAND_BOOST, 10000, 0);
	}
	if (t < p->prev_tick || !p->prev_tick || !p->pending) {
		p->prev_tick = t - 1;
		fsmp = 1;
	} else {
		fsmp = (t - p->prev_tick) * (double)p->freq/1000.0/(double)f * mult;
	}
//	printf("delta_t = %i, nsmp = %i\n", t - p->prev_tick, nsmp);
	if (fsmp > maxnsmp) { fsmp = 1; }
	EnterCriticalSection(&p->crit);
	if (p->flen) {
		if (p->flen + fsmp > 1) {
			double t = 1 - p->flen;
			p->fsum += ((double)val - MID_SAMPLE) / (double)(MAX_SAMPLE - MID_SAMPLE) * t;
			sound_write(p, p->fsum * (double)(MAX_SAMPLE - MID_SAMPLE) + MID_SAMPLE, 1);
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
		sound_write(p, val, (int)fsmp);
		fsmp -= (int)fsmp;
	}
	if (p->pending == 1 && !p->nbuf_playing) {
		set_timer_for_buf(p, NULL);
	}
	// at this point fsmp < 1 always
	p->fsum = ((double)val - MID_SAMPLE) / (double)(MAX_SAMPLE - MID_SAMPLE) * fsmp;
	p->flen = fsmp;
fin:
	p->prev_tick = t;
	LeaveCriticalSection(&p->crit);

	p->last_append = GetTickCount();

	return 0;
}

static void CALLBACK out_proc(HWAVEOUT hwo,UINT uMsg,DWORD dwInstance,  
				DWORD dwParam1,DWORD dwParam2)
{
	struct ACM_DATA*p = (struct ACM_DATA*)dwInstance;

	Sprintf(("acm: waveOut callback : %i\n",uMsg));
	switch (uMsg) {
	case WOM_DONE:
		{
			WAVEHDR*h = (WAVEHDR*)dwParam1;
			int n = p->bufrefs[h->dwUser];
			Sprintf(("acm: buffer done: buf = %i, next = %i\n", h->dwUser, n));
			InterlockedDecrement(&p->nbuf_playing);
			if (n != -1) set_timer_for_buf(p, p->hdrs + n);
		}
		break;
	}
	Sprintf(("acm: end callback\n"));
}



struct SOUNDPROC soundacm={ sound_init, sound_term, sound_data, NULL};
