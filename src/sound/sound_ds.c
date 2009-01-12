/*
	Agat Emulator version 1.0
	Copyright (c) NOP, nnop@newmail.ru
	sound_ds - sound output via DirectSound library [incomplete]
*/

#include "soundint.h"
#include "debug.h"
#include <dsound.h>
#include <assert.h>

//#define DEBUG_SOUND

#pragma comment(lib,"dxguid")
#pragma comment(lib,"dsound")

//#define SFREQ 22050
//#define BUF_NSMP 300
//#define SDELAY (BUF_NSMP*1000/SFREQ)
#define INIT_STR(s) { memset(&(s),0,sizeof(s)); (s).dwSize=sizeof(s); }

#define SOUND_BUFFER_SIZE BUF_NSMP

typedef short sample_t;

#ifdef DEBUG_SOUND
#define Sprintf(x) printf x
#else
#define Sprintf(x)
#endif

struct DS_DATA
{
	IDirectSound*sound;
	IDirectSoundBuffer*buffer;
	IDirectSoundNotify*notify;
	DSBPOSITIONNOTIFY  positions[2];
	int freq;
	int buflen, bufsmp;

	sample_t lastval;
	int cursmp, prevsmp;

	HWND video_w;

	int playing;
	int part;
	HANDLE pipe[2];
	HANDLE hsem;
	HANDLE hthread;
	DWORD tid;
	int term;
};


struct SNDPACK
{
	sample_t val;
	short count;
};

static void fill_buffer(struct DS_DATA*p, int ofs, int nb, sample_t val)
{
	LPVOID pt[2];
	DWORD sz[2];
	if (!nb) return;
	if (IDirectSoundBuffer_Lock(p->buffer,ofs,nb,pt,sz,pt+1,sz+1,0)!=DS_OK) {
		Sprintf(("fill: ds lock failed!\n"));
		return;
	}
	if (pt[0]) memset(pt[0], val, sz[0]);
	if (pt[1]) memset(pt[1], val, sz[1]);
	if (DS_FAILED(IDirectSoundBuffer_Unlock(p->buffer,pt[0],sz[0],pt[1],sz[1]),TEXT("unlocking buffer"))) {
		Sprintf(("fill: ds unlock failed!\n"));
		return;
	}
}

static void clear_buffer(struct DS_DATA*p)
{
	static LPVOID pt[2];
	static DWORD sz[2];
	if (DS_FAILED(IDirectSoundBuffer_Lock(p->buffer,0,0,pt,sz,pt+1,sz+1,DSBLOCK_ENTIREBUFFER),TEXT("locking buffer"))) {
		Sprintf(("clear: ds lock failed!\n"));
		return;
	}
	if (pt[0]) memset(pt[0],p->lastval,sz[0]);
	if (pt[1]) memset(pt[1],p->lastval,sz[1]);
	if (DS_FAILED(IDirectSoundBuffer_Unlock(p->buffer,pt[0],sz[0],pt[1],sz[1]),TEXT("unlocking buffer"))) {
		Sprintf(("clear: ds unlock failed!\n"));
		return;
	}
}


static DWORD CALLBACK sound_proc(struct DS_DATA*p)
{
	int timeremains = INFINITE;
	int cur_ofs = 0;
	struct SNDPACK pack;
	while (!p->term) {
		DWORD nb;
		int r;
		if ((timeremains != INFINITE) && timeremains < 10) timeremains = 10;
		Sprintf(("snd: waiting for packet for %i msecs\n", timeremains));
		r = WaitForSingleObject(p->hsem, timeremains);
		if (p->term) break;
		Sprintf(("snd: wait result: %s\n", (r == WAIT_TIMEOUT)?"timeout":"done"));
		if (r == WAIT_TIMEOUT) {
			if (!p->playing && cur_ofs <= p->buflen / 4) {
				if (timeremains < 300) {
					timeremains *= 2;
					continue;
				}
			}
			if (!p->playing && cur_ofs) {
				timeremains = cur_ofs * 1000 / p->freq / sizeof(sample_t);
				IDirectSoundBuffer_SetCurrentPosition(p->buffer, 0);
				Sprintf(("snd: playing remaining buffer for %i msecs after timeout\n", timeremains));
				IDirectSoundBuffer_Play(p->buffer, 0, 0, DSBPLAY_LOOPING);
				p->playing = 1;
				continue;
			}
			Sprintf(("snd: stopping playing\n"));
			IDirectSoundBuffer_Stop(p->buffer);
			clear_buffer(p);
			timeremains = INFINITE;
			cur_ofs = 0;
			p->playing = 0;
			p->prevsmp = -1;
		} else {
			int ofs;
			int nb;
			DWORD pl_cur, wr_cur;
			ReadFile(p->pipe[0], &pack, sizeof(pack), &nb, NULL);
			nb = pack.count * sizeof(sample_t);
			Sprintf(("snd: pack: count = %i, sample = %x\n", pack.count, pack.val));
			ofs = cur_ofs;
			if (!p->playing) {
				pl_cur = 0;
				if (!cur_ofs) {
					clear_buffer(p);
					Sprintf(("snd: clearing whole stopped buffer with value %x\n", p->lastval));
				}	
			} else {
				IDirectSoundBuffer_GetCurrentPosition(p->buffer, &pl_cur, &wr_cur);
				Sprintf(("snd: current buffer position: pl %x, wr %x\n", pl_cur, wr_cur));
				pl_cur = wr_cur;
				if (pl_cur < p->buflen/2 && (ofs < pl_cur || ofs + nb > p->buflen)) {
//					fill_buffer(p, pl_cur, p->buflen/2 - pl_cur, pack.val);
					ResetEvent(p->positions[1].hEventNotify);
					Sprintf(("snd: waiting second event\n"));
			                WaitForSingleObject(p->positions[1].hEventNotify, INFINITE);
					Sprintf(("snd: after waiting second event\n"));
					ResetEvent(p->positions[0].hEventNotify);
					ResetEvent(p->positions[1].hEventNotify);
				}
				if (p->term) break;
				if (pl_cur > p->buflen/2 && (ofs > p->buflen/2 || ofs + nb > p->buflen/2)) {
//					fill_buffer(p, pl_cur, p->buflen - pl_cur, pack.val);
					ResetEvent(p->positions[0].hEventNotify);
					Sprintf(("snd: waiting first event\n"));
			                WaitForSingleObject(p->positions[0].hEventNotify, INFINITE);
					Sprintf(("snd: after waiting first event\n"));
					ResetEvent(p->positions[0].hEventNotify);
					ResetEvent(p->positions[1].hEventNotify);
				}
				if (p->term) break;
				timeremains = 1000;
			}
			Sprintf(("snd: filling buffer: ofs = %x, nb = %i, val = %x\n", ofs, nb, pack.val));
			fill_buffer(p, ofs, nb, pack.val);
			cur_ofs = ofs + nb;
			if (cur_ofs > p->buflen) cur_ofs -= p->buflen;
			Sprintf(("snd: increased curofs = %x\n", cur_ofs));
			{
				int n = (cur_ofs - pl_cur);
				if (n < 0) n += p->buflen;
				timeremains = n * 1000 / p->freq / sizeof(sample_t);
			}
			if (!p->playing && cur_ofs > p->buflen / 4) {
				IDirectSoundBuffer_SetCurrentPosition(p->buffer, 0);
				Sprintf(("snd: starting play from start of buffer\n"));
				IDirectSoundBuffer_Play(p->buffer, 0, 0, DSBPLAY_LOOPING);
				p->playing = 1;
			}
		}
	}
	return 0;
}



static struct DS_DATA* sound_init(struct SOUNDPARAMS*par)
{
	struct DS_DATA*p;
	DWORD tid;
	DSBUFFERDESC de;
	static WAVEFORMATEX fmt;

	p = calloc(1, sizeof(*p));
	if (!p) return NULL;

	p->prevsmp = -1;
	p->cursmp = 0;
	p->lastval = 0x7F7F;
	p->buflen = par->buflen;
	p->bufsmp = p->buflen / sizeof(sample_t);
	p->freq = par->freq;
	p->video_w = par->w;

	INIT_STR(de);
	memset(&fmt,0,sizeof(fmt));
	fmt.wFormatTag=WAVE_FORMAT_PCM;
	fmt.nChannels=1;
	fmt.nSamplesPerSec=par->freq;
	fmt.nAvgBytesPerSec=par->freq * sizeof(sample_t);
	fmt.nBlockAlign=sizeof(sample_t);
	fmt.wBitsPerSample=sizeof(sample_t)*8;

	if (DS_FAILED(DirectSoundCreate(NULL,&p->sound,NULL),TEXT("initializing Direct Sound"))) goto err0;
	if (DS_FAILED(IDirectSound_SetCooperativeLevel(p->sound,par->w,
		DSSCL_NORMAL),TEXT("setting cooperative level"))) {
		goto err1;
	}
//	de.dwFlags=DSBCAPS_PRIMARYBUFFER|DSBCAPS_CTRLVOLUME|DSBCAPS_GETCURRENTPOSITION2;
 	de.lpwfxFormat=&fmt;
	de.dwFlags=DSBCAPS_CTRLPOSITIONNOTIFY|DSBCAPS_GETCURRENTPOSITION2;
	de.dwBufferBytes=p->buflen;
	if (DS_FAILED(IDirectSound_CreateSoundBuffer(p->sound,&de,&p->buffer,NULL),TEXT("Creating sound buffer"))) {
		goto err1;
	}
	if (DS_FAILED(IDirectSoundBuffer_QueryInterface(p->buffer, &IID_IDirectSoundNotify, &p->notify),TEXT("Creating notification interface"))) {
		goto err2;
	}
	if (!CreatePipe(p->pipe+0, p->pipe+1, NULL, 0)) {
		goto err3;
	}
	p->positions[0].hEventNotify = CreateEvent(NULL, TRUE, FALSE, NULL);
	p->positions[0].dwOffset = 0;
	p->positions[1].hEventNotify = CreateEvent(NULL, TRUE, FALSE, NULL);
	p->positions[1].dwOffset = p->buflen >> 1;
	IDirectSoundNotify_SetNotificationPositions(p->notify, 2, p->positions);
	p->playing = 0;
	p->part = 0;
	p->hsem = CreateSemaphore(NULL, 0, 100 * sizeof(struct SNDPACK), NULL);
	p->hthread = CreateThread(NULL, 0, sound_proc, p, 0, &p->tid);
	SetThreadPriority(p->hthread, THREAD_PRIORITY_ABOVE_NORMAL);
	return p;
err3:
	IDirectSoundNotify_Release(p->notify);
err2:
	IDirectSoundBuffer_Release(p->buffer);
err1:
	IDirectSound_Release(p->sound);
err0:
	free(p);
	return NULL;
}

static void sound_term(struct DS_DATA*p)
{
	if (!p) return;
	p->term = 1;
	ReleaseSemaphore(p->hsem, 1, NULL);
	CloseHandle(p->pipe+0);
	CloseHandle(p->pipe+1);
	WaitForSingleObject(p->hthread, 1000);
	TerminateThread(p->hthread, 0);
	CloseHandle(p->hthread);
	CloseHandle(p->hsem);
	CloseHandle(p->positions[0].hEventNotify);
	CloseHandle(p->positions[1].hEventNotify);
	if (p->notify) IDirectSoundNotify_Release(p->notify);
	if (p->buffer) IDirectSoundBuffer_Release(p->buffer);
	if (p->sound) IDirectSound_Release(p->sound);
	free(p);
}             
static int sound_data(struct DS_DATA*p, int val, long t, long f)
{
	struct SNDPACK pack;
	int maxnsmp=p->bufsmp;
	int nsmp;
	DWORD nb;
	int cursmp=t*(double)p->freq/1000.0/(double)f * 1.1;
	if (val == SOUND_TOGGLE) val = p->lastval ^ 0xFFFF;
	else val = val?0x7FFF:0x8000;
//	Sprintf(("sound_data: val = %x; t = %i; f = %i; cursmp = %i\n", val, t, f, cursmp));
	if (p->prevsmp != -1) {
		if (cursmp<p->prevsmp) { p->prevsmp=cursmp-1; }
		nsmp=cursmp-p->prevsmp;
		if (nsmp>maxnsmp) nsmp=1;
	} else {
		nsmp = 1;
	}
	p->prevsmp=cursmp;
	pack.val = val;
	pack.count = nsmp;
	do {
		if (!WriteFile(p->pipe[1], &pack, sizeof(pack), &nb, NULL)) return -1;
	} while (nb != sizeof(pack));
	ReleaseSemaphore(p->hsem, 1, NULL);
	p->lastval = val;
	return 0;
}

struct SOUNDPROC soundds={ sound_init, sound_term, sound_data };
