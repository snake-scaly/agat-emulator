/*
	Agat Emulator version 1.4
	Copyright (c) NOP, nnop@newmail.ru
	mockingboard - emulation of Apple2's sound effects card
*/

#include "types.h"
#include "memory.h"
#include "streams.h"
#include "debug.h"
#include "sysconf.h"
#include "runmgr.h"
#include "runmgrint.h"

#include <dsound.h>
#include <assert.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define _USE_MATH_DEFINES
#include <math.h>

#include "syslib.h"
#include "debug.h"

#include "localize.h"

#pragma comment(lib,"dxguid")
#pragma comment(lib,"dsound")

#ifdef DEBUG_SOUND
#define Sprintf(x) printf x
#else
#define Sprintf(x)
#endif


#define NBUFS 6

#define INIT_STR(s) { memset(&(s),0,sizeof(s)); (s).dwSize=sizeof(s); }

#define NCHANS 2
#define NREGS 16


typedef short sample_t;


struct SOUND_STATE
{
	struct SLOT_RUN_STATE*st;
	IDirectSound*sound;
	IDirectSoundBuffer*buffers[NBUFS];

	int freq, freq0, amp0;
	int buflen, bufsmp;
	HWND video_w;

	byte psg_data[NCHANS];
	int  psg_curreg[NCHANS];
	byte psg_regs[NCHANS][NREGS];

	int cpu_freq_khz;
};





void fill_buf_sine(IDirectSoundBuffer*ds, int len, double a, double f)
{
	LPVOID pt[2];
	DWORD sz[2];
	if (IDirectSoundBuffer_Lock(ds, 0, len, pt + 0, sz + 0, pt + 1, sz + 1, 0)!=DS_OK) {
		Sprintf(("fill_sine: ds lock failed!\n"));
		return;
	}
	if (pt[1]) {
		puts("pt[1] should be NULL");
	} else {
		int ns = sz[0] / sizeof(sample_t);
		double v = 0;
		sample_t*s = pt[0];
		int nl = f * ns / (2 * M_PI);
		f = nl * 2 * M_PI / ns;
		for (; ns; --ns, ++s, v += f) {
			*s = a * sin(v);
		}
	}
	if (DS_FAILED(IDirectSoundBuffer_Unlock(ds, pt[0], sz[0], pt[1], sz[1]),TEXT("unlocking buffer"))) {
		Sprintf(("fill_sine: ds unlock failed!\n"));
		return;
	}
	
}



static int sound_init(struct SOUND_STATE*ss)
{
	WAVEFORMATEX fmt;
	int i, j;

	ss->buflen = 32768;
	ss->bufsmp = ss->buflen / sizeof(sample_t);
	ss->freq = 44100;
	ss->freq0 = ss->freq / 44;
	{
		int nt = ss->bufsmp * ss->freq0 / ss->freq;
		ss->freq0 = nt * ss->freq / ss->bufsmp;
	}
	ss->amp0 = 15000;
	ss->video_w = ss->st->sr->video_w;
	
	if (DS_FAILED(DirectSoundCreate(NULL,&ss->sound,NULL),TEXT("initializing Direct Sound"))) goto err0;
	if (DS_FAILED(IDirectSound_SetCooperativeLevel(ss->sound, ss->video_w,
		DSSCL_NORMAL),TEXT("setting cooperative level"))) {
		goto err1;
	}

	memset(&fmt, 0, sizeof(fmt));
	fmt.wFormatTag = WAVE_FORMAT_PCM;
	fmt.nChannels = 1;
	fmt.nSamplesPerSec = ss->freq;
	fmt.nAvgBytesPerSec = ss->freq * sizeof(sample_t) * fmt.nChannels;
	fmt.nBlockAlign = sizeof(sample_t) * fmt.nChannels;
	fmt.wBitsPerSample = sizeof(sample_t) * 8;

	for (i = 0; i < NBUFS; ++i) {
		DSBUFFERDESC de;
		INIT_STR(de);
	 	de.lpwfxFormat=&fmt;
		de.dwFlags=DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_STATIC;
		de.dwBufferBytes=ss->buflen;
		if (DS_FAILED(IDirectSound_CreateSoundBuffer(ss->sound,&de,ss->buffers + i,NULL),TEXT("Creating sound buffer"))) {
			goto err2;
		}
		fill_buf_sine(ss->buffers[i], ss->buflen, ss->amp0, 2.0*M_PI*ss->freq0/ss->freq);
/*		if (!i) {
			IDirectSoundBuffer_Play(ss->buffers[i], 0, 0, DSBPLAY_LOOPING);
		}*/
	}
	return 0;
err2:
	for (j = 0; j < i; ++j) {
		IDirectSoundBuffer_Release(ss->buffers[j]);
	}
err1:
	IDirectSound_Release(ss->sound);
err0:
	return -1;
}

static int sound_term(struct SOUND_STATE*ss)
{
	int i;
	for (i = 0; i < NBUFS; ++i) {
		if (ss->buffers[i]) IDirectSoundBuffer_Release(ss->buffers[i]);
	}
	if (ss->sound) IDirectSound_Release(ss->sound);
	return 0;
}








                                                                                       	




static int mb_term(struct SLOT_RUN_STATE*st)
{
	struct SOUND_STATE*ss = st->data;
	sound_term(ss);
	free(ss);
	return 0;
}

static int mb_save(struct SLOT_RUN_STATE*st, OSTREAM*out)
{
	struct SOUND_STATE*ss = st->data;

	return 0;
}

static int mb_load(struct SLOT_RUN_STATE*st, ISTREAM*in)
{
	struct SOUND_STATE*ss = st->data;

	return 0;
}

static int mb_command(struct SLOT_RUN_STATE*st, int cmd, int data, long param)
{
	struct SOUND_STATE*ss = st->data;
	switch (cmd) {
	case SYS_COMMAND_RESET:
		return 0;
	case SYS_COMMAND_HRESET:
		return 0;
	case SYS_COMMAND_INIT_DONE:
		ss->cpu_freq_khz = cpu_get_freq(st->sr);
		return 0;
	}
	return 0;
}

static byte mb_rom_r(word adr, struct SOUND_STATE*ss) // CX00-CXFF
{
	printf("mockingboard: rom read %x\n", adr);
	return empty_read(adr, ss);
}


static void update_gen_enable(int channel, byte data, struct SOUND_STATE *ss);

static void psg_reset(int channel, struct SOUND_STATE*ss)
{
	printf("psg_reset on channel %i\n", channel);
	memset(ss->psg_regs[channel], 0, sizeof(ss->psg_regs[channel]));
	update_gen_enable(channel, 0xFF, ss);
}

static void psg_inactive(int channel, struct SOUND_STATE*ss)
{
//	printf("psg_inactive on channel %i\n", channel);
}


static void update_gen_freq(int channel, int gen, byte d1, byte d2, struct SOUND_STATE *ss)
{
	int div = (((unsigned)d2)<<12) | (((unsigned) d1)<<4);
	double f1;
	if (!div) return;
	f1 = ss->cpu_freq_khz * 1000.0 / div;
	printf("freq[%i,%i] = %g Hz\n", channel, gen, f1);
	IDirectSoundBuffer_SetFrequency(ss->buffers[gen*2+channel], f1*ss->freq/ss->freq0);
}

static void update_gen_enable(int channel, byte data, struct SOUND_STATE *ss)
{
	if ((data & 4) == 0) {
		printf("enable C-%i\n", channel);
		IDirectSoundBuffer_Play(ss->buffers[4|channel], 0, 0, DSBPLAY_LOOPING);
	} else {
		printf("disable C-%i\n", channel);
		IDirectSoundBuffer_Stop(ss->buffers[4|channel]);
	}
	if ((data & 2) == 0) {
		printf("enable B-%i\n", channel);
		IDirectSoundBuffer_Play(ss->buffers[2|channel], 0, 0, DSBPLAY_LOOPING);
	} else {
		printf("disable B-%i\n", channel);
		IDirectSoundBuffer_Stop(ss->buffers[2|channel]);
	}
	if ((data & 1) == 0) {
		printf("enable A-%i\n", channel);
		IDirectSoundBuffer_Play(ss->buffers[0|channel], 0, 0, DSBPLAY_LOOPING);
	} else {
		printf("disable A-%i\n", channel);
		IDirectSoundBuffer_Stop(ss->buffers[0|channel]);
	}
}

static void psg_write_reg(int channel, byte reg, byte data, struct SOUND_STATE *ss)
{
	ss->psg_regs[channel][reg] = data;
	switch (reg) {
	case 0: case 1:
		update_gen_freq(channel, 0, ss->psg_regs[channel][0], ss->psg_regs[channel][1], ss);
		break;
	case 2: case 3:
		update_gen_freq(channel, 1, ss->psg_regs[channel][2], ss->psg_regs[channel][3], ss);
		break;
	case 4: case 5:
		update_gen_freq(channel, 2, ss->psg_regs[channel][4], ss->psg_regs[channel][5], ss);
		break;
	case 7:
		update_gen_enable(channel, data, ss);
		break;
	}
}

static void psg_write(int channel, struct SOUND_STATE*ss)
{
	printf("psg_write %02x to reg %i on channel %i\n", ss->psg_data[channel], ss->psg_curreg[channel], channel);
	psg_write_reg(channel, ss->psg_curreg[channel] % NREGS, ss->psg_data[channel], ss);
}

static void psg_select(int channel, struct SOUND_STATE*ss)
{
//	printf("psg_select %i on channel %i\n", ss->psg_data[channel], channel);
	ss->psg_curreg[channel] = ss->psg_data[channel];
}

static void psg_command(int channel, byte cmd, struct SOUND_STATE*ss)
{
	switch (cmd) {
	case 0: psg_reset(channel, ss); break;
	case 4: psg_inactive(channel, ss); break;
	case 6: psg_write(channel, ss); break;
	case 7: psg_select(channel, ss); break;
	}
}

static void psg_data(int channel, byte data, struct SOUND_STATE*ss)
{
//	printf("psg_data %x on channel %i\n", data, channel);
	ss->psg_data[channel] = data;
}

static void mb_rom_w(word adr, byte data, struct SOUND_STATE*ss) // CX00-CXFF
{
	switch (adr & 0xFF) {
	case 0x00: psg_command(0, data, ss); break;
	case 0x80: psg_command(1, data, ss); break;
	case 0x01: psg_data(0, data, ss); break;
	case 0x81: psg_data(1, data, ss); break;
	default:
		printf("mockingboard: rom write %x, %02x\n", adr, data);
	}
}


static void mb_io_w(word adr, byte data, struct SOUND_STATE*ss) // C0X0-C0XF
{
	printf("mockingboard: io write %x, %02x\n", adr, data);
}


static byte mb_io_r(word adr, struct SOUND_STATE*ss) // C0X0-C0XF
{
	printf("mockingboard: io read %x\n", adr);
	return empty_read(adr, ss);
}


int  mockingboard_init(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*st, struct SLOTCONFIG*cf)
{
	struct SOUND_STATE*ss;

	puts("in mockingboard_init");

	ss = calloc(1, sizeof(*ss));
	if (!ss) return -1;

	ss->st = st;

	if (sound_init(ss) < 0) { free(ss); return -1; }

	st->data = ss;
	st->free = mb_term;
	st->command = mb_command;
	st->load = mb_load;
	st->save = mb_save;

	fill_rw_proc(st->io_sel, 1, mb_rom_r, mb_rom_w, ss);
	fill_rw_proc(st->baseio_sel, 1, mb_io_r, mb_io_w, ss);

	return 0;
}
