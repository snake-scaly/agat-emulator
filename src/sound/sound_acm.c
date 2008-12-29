/*
	Agat Emulator version 1.0
	Copyright (c) NOP, nnop@newmail.ru
	sound_acm - sound output via ACM library [incomplete]
*/

#include <windows.h>
#include <mmsystem.h>
#include "soundint.h"
#include "debug.h"

#pragma comment(lib,"winmm")


#define SOUND_BUFFER_SIZE 256

struct ACM_DATA
{
	char *sound_buffer;
	int  buflen;
	HWAVEOUT out;
	WAVEHDR hdr;
	int cur_val;
};

static void CALLBACK out_proc(
  HWAVEOUT hwo,      
  UINT uMsg,         
  DWORD dwInstance,  
  DWORD dwParam1,    
  DWORD dwParam2     
);


static void* sound_init(struct SOUNDPARAMS*par)
{
	struct ACM_DATA*p;
	WAVEFORMATEX fmt;

	p = calloc(1, sizeof(*p));
	if (!p) return NULL;
	p->buflen = par->buflen;
	p->sound_buffer = malloc(p->buflen);
	p->cur_val = 0;
	if (!p->sound_buffer) goto err0;

	fmt.wFormatTag=WAVE_FORMAT_PCM;
	fmt.nChannels=1;
	fmt.nSamplesPerSec=par->freq;
	fmt.nAvgBytesPerSec=par->freq;
	fmt.nBlockAlign=1;
	fmt.wBitsPerSample=8;
	if (ACM_FAILED(waveOutOpen(&p->out,WAVE_MAPPER,&fmt,(DWORD)out_proc,0,CALLBACK_FUNCTION),TEXT("opening wave out"))) goto err1;
	ZeroMemory(&p->hdr,sizeof(p->hdr));
	p->hdr.lpData=p->sound_buffer;
	p->hdr.dwBufferLength=p->buflen;
	p->hdr.dwFlags=WHDR_BEGINLOOP|WHDR_ENDLOOP;
	p->hdr.dwLoops=INFINITE;
	if (ACM_FAILED(waveOutPrepareHeader(p->out,&p->hdr,sizeof(p->hdr)),TEXT("preparing sound header"))) goto err2;
	if (ACM_FAILED(waveOutWrite(p->out,&p->hdr,sizeof(p->hdr)),TEXT("starting playing sound"))) goto err3;
	return p;
err3:
	waveOutUnprepareHeader(p->out,&p->hdr,sizeof(p->hdr));
err2:
	waveOutClose(p->out);
err1:
	free(p->sound_buffer);
err0:
	free(p);
	return NULL;
}

static void sound_term(struct ACM_DATA*p)
{
	if (!p) return;
	if (p->out) {
		waveOutReset(p->out);
		waveOutUnprepareHeader(p->out,&p->hdr,sizeof(p->hdr));
		waveOutClose(p->out);
	}
	if (p->sound_buffer) free(p->sound_buffer);
	free(p);
}

static int sound_data(struct ACM_DATA*p, int val, long t, long f)
{
	if (!p||!p->sound_buffer) return -1;
	if (val == SOUND_TOGGLE) val = (p->cur_val = ~p->cur_val);
//	waveOutSetVolume(p->out, (p->cur_val == 0x7F)?0xFFFF: 0x0000);
	memset(p->sound_buffer, val, p->buflen);
	return 0;
}


static void CALLBACK out_proc(HWAVEOUT hwo,UINT uMsg,DWORD dwInstance,  
				DWORD dwParam1,DWORD dwParam2)
{
//	printf("callback : %i\n",uMsg);
}

struct SOUNDPROC soundacm={ sound_init, sound_term, sound_data };
