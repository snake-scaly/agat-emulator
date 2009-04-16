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



#define INIT_STR(s) { memset(&(s),0,sizeof(s)); (s).dwSize=sizeof(s); }

#define NCHANS 2
#define NGENS 3
#define NBUFS (2*NCHANS*NGENS)


#define NREGS 16

#define MB_ORB		0x00
#define MB_ORA		0x01
#define MB_DDRB		0x02
#define MB_DDRA		0x03
#define MB_CNT1L	0x04
#define MB_CNT1H	0x05
#define MB_LATCH1L 	0x06
#define MB_LATCH1H 	0x07
#define MB_CNT2L	0x08
#define MB_CNT2H	0x09
#define MB_SR		0x0A
#define MB_ACR 		0x0B
#define MB_PCR		0x0C
#define MB_IFR 		0x0D
#define MB_IER 		0x0E
#define MB_ORAX		0x0F



typedef short sample_t;


struct SOUND_STATE
{
	struct SLOT_RUN_STATE*st;
	IDirectSound*sound;
	IDirectSoundBuffer*buffers[NBUFS];

	int freq, freq0, amp0, ampn;
	int buflen, bufsmp;
	HWND video_w;

	byte ddrs[NCHANS][2];
	byte psg_data[NCHANS][15];
	int  psg_curreg[NCHANS];
	byte psg_regs[NCHANS][NREGS];

	int cpu_freq_khz;
};


static void mb_callback(struct SOUND_STATE*ss, int channel);



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



void fill_buf_noise(IDirectSoundBuffer*ds, int len, double a, double f)
{
	LPVOID pt[2];
	DWORD sz[2];
	if (IDirectSoundBuffer_Lock(ds, 0, len, pt + 0, sz + 0, pt + 1, sz + 1, 0)!=DS_OK) {
		Sprintf(("fill_noise: ds lock failed!\n"));
		return;
	}
	if (pt[1]) {
		puts("pt[1] should be NULL");
	} else {
		int ns = sz[0] / sizeof(sample_t);
		double v = 0;
		sample_t*s = pt[0];
		for (; ns; --ns, ++s, v += f) {
			*s = a * rand() / RAND_MAX;
		}
	}
	if (DS_FAILED(IDirectSoundBuffer_Unlock(ds, pt[0], sz[0], pt[1], sz[1]),TEXT("unlocking buffer"))) {
		Sprintf(("fill_noise: ds unlock failed!\n"));
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
	ss->freq0 = 1000;
	{
		int nt = ss->bufsmp * ss->freq0 / ss->freq;
		ss->freq0 = nt * ss->freq / ss->bufsmp;
	}
	ss->amp0 = 32767;
	ss->ampn = 3000;
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
		if (i < NCHANS*NGENS) {
			fill_buf_sine(ss->buffers[i], ss->buflen, ss->amp0, 2.0*M_PI*ss->freq0/ss->freq);
		} else {
			fill_buf_noise(ss->buffers[i], ss->buflen, ss->ampn, 2.0*M_PI*ss->freq0/ss->freq);
		}	
		IDirectSoundBuffer_SetVolume(ss->buffers[i], -10000);
		IDirectSoundBuffer_SetPan(ss->buffers[i], (i&1)?-10000:10000);
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
	WRITE_ARRAY(out, ss->ddrs);
	WRITE_ARRAY(out, ss->psg_data);
	WRITE_ARRAY(out, ss->psg_curreg);
	WRITE_ARRAY(out, ss->psg_regs);

	return 0;
}

static void start_timer(int channel, int no, struct SOUND_STATE*ss);
static void psg_write_reg(int channel, byte reg, byte data, struct SOUND_STATE *ss);

static int mb_load(struct SLOT_RUN_STATE*st, ISTREAM*in)
{
	struct SOUND_STATE*ss = st->data;
	int i, j;

	READ_ARRAY(in, ss->ddrs);
	READ_ARRAY(in, ss->psg_data);
	READ_ARRAY(in, ss->psg_curreg);
	READ_ARRAY(in, ss->psg_regs);

	for (j = 0; j < NCHANS; ++j) {
		for (i = 0; i < NREGS; ++i) {
			psg_write_reg(j, i, ss->psg_regs[j][i], ss);
		}
	}
	start_timer(0, 0, ss);
	start_timer(1, 0, ss);
	return 0;
}

static void psg_reset(int channel, struct SOUND_STATE*ss);


static int mb_command(struct SLOT_RUN_STATE*st, int cmd, int data, long param)
{
	struct SOUND_STATE*ss = st->data;
	switch (cmd) {
	case SYS_COMMAND_RESET:
		return 0;
	case SYS_COMMAND_HRESET:
		system_command(ss->st->sr, SYS_COMMAND_SET_CPUTIMER, 0, (long)ss);
		psg_reset(0, ss);
		psg_reset(1, ss);
		return 0;
	case SYS_COMMAND_INIT_DONE:
		ss->cpu_freq_khz = cpu_get_freq(st->sr);
		return 0;
	case SYS_COMMAND_CPUTIMER:
		if ((param&~1L) == (long)ss) {
			mb_callback(ss, param & 1);
			return 1;
		}
		return 0;
	}
	return 0;
}


static void update_gen_enable(int channel, byte data, struct SOUND_STATE *ss);

static void psg_reset(int channel, struct SOUND_STATE*ss)
{
	int i;
//	printf("psg_reset on channel %i\n", channel);
	memset(ss->psg_regs[channel], 0, sizeof(ss->psg_regs[channel]));
	for (i = 0; i < NBUFS; ++i) {
		IDirectSoundBuffer_Stop(ss->buffers[i]);
		IDirectSoundBuffer_SetVolume(ss->buffers[i], 0);
		IDirectSoundBuffer_SetFrequency(ss->buffers[i], ss->freq);
	}
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
//	printf("freq[%i,%i] = %g Hz\n", channel, gen, f1);
	IDirectSoundBuffer_SetFrequency(ss->buffers[gen*2+channel], f1*ss->freq/ss->freq0);
}


static void update_ng_freq(int channel, byte d1, struct SOUND_STATE *ss)
{
	int div = ((unsigned) d1)<<4;
	double f1;
	int i;
	if (!div) return;
	f1 = ss->cpu_freq_khz * 1000.0 / div;
//	printf("freq_ng[%i] = %g Hz\n", channel, f1);
	for (i = 0; i < NGENS; ++i) {
		IDirectSoundBuffer_SetFrequency(ss->buffers[(i+NGENS)*2+channel], f1*ss->freq/ss->freq0);
	}
}

static void update_gen_enable(int channel, byte data, struct SOUND_STATE *ss)
{
	if ((data & 4) == 0) {
//		printf("enable C-%i\n", channel);
		IDirectSoundBuffer_Play(ss->buffers[4|channel], 0, 0, DSBPLAY_LOOPING);
	} else {
//		printf("disable C-%i\n", channel);
		IDirectSoundBuffer_Stop(ss->buffers[4|channel]);
	}
	if ((data & 2) == 0) {
//		printf("enable B-%i\n", channel);
		IDirectSoundBuffer_Play(ss->buffers[2|channel], 0, 0, DSBPLAY_LOOPING);
	} else {
//		printf("disable B-%i\n", channel);
		IDirectSoundBuffer_Stop(ss->buffers[2|channel]);
	}
	if ((data & 1) == 0) {
//		printf("enable A-%i\n", channel);
		IDirectSoundBuffer_Play(ss->buffers[0|channel], 0, 0, DSBPLAY_LOOPING);
	} else {
//		printf("disable A-%i\n", channel);
		IDirectSoundBuffer_Stop(ss->buffers[0|channel]);
	}
	if ((data & 32) == 0) {
//		printf("enable C-noise %i\n", channel);
		IDirectSoundBuffer_Play(ss->buffers[6|channel], 0, 0, DSBPLAY_LOOPING);
	} else {
//		printf("disable C-noise %i\n", channel);
		IDirectSoundBuffer_Stop(ss->buffers[6|channel]);
	}
	if ((data & 16) == 0) {
//		printf("enable B-noise %i\n", channel);
		IDirectSoundBuffer_Play(ss->buffers[8|channel], 0, 0, DSBPLAY_LOOPING);
	} else {
//		printf("disable B-noise %i\n", channel);
		IDirectSoundBuffer_Stop(ss->buffers[8|channel]);
	}
	if ((data & 8) == 0) {
//		printf("enable A-noise %i\n", channel);
		IDirectSoundBuffer_Play(ss->buffers[10|channel], 0, 0, DSBPLAY_LOOPING);
	} else {
//		printf("disable A-noise %i\n", channel);
		IDirectSoundBuffer_Stop(ss->buffers[10|channel]);
	}
}

void update_gen_amp(int channel, int gen, byte data, struct SOUND_STATE *ss)
{
	if (data & 16) { // envelope
		data = 0;
	}
	{
		int lev;
		data &= 15;
		lev = ((15 - data) * -10000)/15;
//		printf("new sound level %i on channel %i:%i\n", lev, channel, gen);
		IDirectSoundBuffer_SetVolume(ss->buffers[channel + gen*2], lev);
		IDirectSoundBuffer_SetVolume(ss->buffers[channel + (gen + NGENS)*2], lev);
	}
}

void update_env_freq(int channel, byte d1, byte d2, struct SOUND_STATE *ss)
{
	int div = (((unsigned)d2)<<16) | (((unsigned) d1)<<8);
	double f1;
	int i;
	if (!div) return;
	f1 = ss->cpu_freq_khz * 1000.0 / div;
//	printf("freq_env[%i] = %g Hz\n", channel, f1);
}

void update_env_shape(int channel, byte data, struct SOUND_STATE *ss)
{
}

static void psg_write_reg(int channel, byte reg, byte data, struct SOUND_STATE *ss)
{
	ss->psg_regs[channel][reg] = data;
	switch (reg) {
	case 0: //case 1:
		update_gen_freq(channel, 0, ss->psg_regs[channel][0], ss->psg_regs[channel][1]&15, ss);
		break;
	case 2: //case 3:
		update_gen_freq(channel, 1, ss->psg_regs[channel][2], ss->psg_regs[channel][3]&15, ss);
		break;
	case 4: //case 5:
		update_gen_freq(channel, 2, ss->psg_regs[channel][4], ss->psg_regs[channel][5]&15, ss);
		break;
	case 6:
		update_ng_freq(channel, data&0x1F, ss);
		break;
	case 7:
		update_gen_enable(channel, data, ss);
		break;
	case 8:
		update_gen_amp(channel, 0, data, ss);
		break;
	case 9:
		update_gen_amp(channel, 1, data, ss);
		break;
	case 10:
		update_gen_amp(channel, 2, data, ss);
		break;
	case 11: case 12:
		update_env_freq(channel, ss->psg_regs[channel][11], ss->psg_regs[channel][12], ss);
		break;
	case 13:
		update_env_shape(channel, data, ss);
		break;
	}
}

static void psg_write(int channel, struct SOUND_STATE*ss)
{
//	printf("psg_write %02x to reg %i on channel %i\n", ss->psg_data[channel], ss->psg_curreg[channel], channel);
	psg_write_reg(channel, ss->psg_curreg[channel] % NREGS, ss->psg_data[channel][1], ss);
}

static void psg_select(int channel, struct SOUND_STATE*ss)
{
//	printf("psg_select %i on channel %i\n", ss->psg_data[channel], channel);
	ss->psg_curreg[channel] = ss->psg_data[channel][1];
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
}


static void fix_ifr(int channel, struct SOUND_STATE*ss)
{
	if ((ss->psg_data[channel][MB_IFR] &= 0x7F) && ss->psg_data[channel][MB_IER] & 0x7F) {
		ss->psg_data[channel][MB_IFR] |= 0x80;
	}
}


static void start_timer(int channel, int no, struct SOUND_STATE*ss)
{
	int div;
	if (no) return;
	if (!(ss->psg_data[channel][MB_IER] & 0x40)) return;
	div = ss->psg_data[channel][MB_LATCH1L] | (ss->psg_data[channel][MB_LATCH1H]<<8);
//	printf("T1 latch: %02X%02X; div = %i\n", ss->psg_data[channel][MB_LATCH1H], ss->psg_data[channel][MB_LATCH1L], div);
//	div /= 4;
	system_command(ss->st->sr, SYS_COMMAND_SET_CPUTIMER, div, ((long)ss) | channel);
}

static void stop_timer(int channel, int no, struct SOUND_STATE*ss)
{
	if (no) return;
	system_command(ss->st->sr, SYS_COMMAND_SET_CPUTIMER, 0, ((long)ss) | channel);
}


static void psg_ifr(int channel, byte data, struct SOUND_STATE*ss)
{
//	printf("psg_ifr %x on channel %i\n", data, channel);
	data = (~data) | 0x80;
	ss->psg_data[channel][MB_IFR] &= data;
	fix_ifr(channel, ss);
}

static void psg_ier(int channel, byte data, struct SOUND_STATE*ss)
{
//	printf("psg_ier %x on channel %i\n", data, channel);
	if (data & 0x80) {
		data &= 0x7F;
		ss->psg_data[channel][MB_IER] |= data;
		fix_ifr(channel, ss);
		start_timer(channel, 0, ss);
		start_timer(channel, 1, ss);
	} else {
		data ^= 0x7F;
		ss->psg_data[channel][MB_IER] &= data;
		stop_timer(channel, 0, ss);
		stop_timer(channel, 1, ss);
	}
}

static void psg_pcr(int channel, byte data, struct SOUND_STATE*ss)
{
}


static void mb_callback(struct SOUND_STATE*ss, int channel)
{
//	printf("mockingboard interrupt (%i, %i)\n", GetTickCount(), channel);
	ss->psg_data[channel][MB_IFR] |= 0x40;
	fix_ifr(channel, ss);
	system_command(ss->st->sr, SYS_COMMAND_IRQ, 0, 0);
	ss->psg_data[channel][MB_CNT1L] = ss->psg_data[channel][MB_LATCH1L];
	ss->psg_data[channel][MB_CNT1H] = ss->psg_data[channel][MB_LATCH1H];
	if (ss->psg_data[channel][MB_ACR]&0x40) { // free running
//		int div = ss->psg_data[channel][MB_LATCH1L] | (ss->psg_data[channel][MB_LATCH1H]<<8);
//		system_command(ss->st->sr, SYS_COMMAND_SET_CPUTIMER, div, (long)ss);
	} else { // one shot
		system_command(ss->st->sr, SYS_COMMAND_SET_CPUTIMER, 0, (long)ss);
	}
}

static void mb_rom_w(word adr, byte data, struct SOUND_STATE*ss) // CX00-CXFF
{
	int chan = 0;
	if (adr & 0x80) chan = 1;

	switch (adr & 0x0F) {
	case MB_IFR: psg_ifr(chan, data, ss); break;
	case MB_IER: psg_ier(chan, data, ss); break;
	case MB_CNT1L: // timer 1 counter l
		break;
	case MB_ORAX: adr = MB_ORA; // no break
	default:
//		printf("mockingboard data write[%i][%i] = %02x\n", chan, adr&0x0F, data);
		ss->psg_data[chan][adr&0x0F] = data;
	}

	switch (adr & 0x0F) {
	case MB_ORB: psg_command(chan, data & ss->psg_data[chan][MB_DDRB], ss); break;
	case MB_ORA: psg_data(chan, data & ss->psg_data[chan][MB_DDRA], ss); break;
	case MB_DDRB: break;
	case MB_DDRA: break;
	case MB_CNT1H: // timer 1 counter h
		ss->psg_data[chan][MB_CNT1L] = ss->psg_data[chan][MB_LATCH1L];
		ss->psg_data[chan][MB_LATCH1H] = data;
		ss->psg_data[chan][MB_IFR] &= ~0x40;
		fix_ifr(chan, ss);
		start_timer(chan, 0, ss);
		break;
	case MB_LATCH1L: // timer 1 latch l
		break;
	case MB_LATCH1H: // timer 1 latch h
		ss->psg_data[chan][MB_IFR] &= ~0x40;
		fix_ifr(chan, ss);
		break;
	case MB_CNT2L: // timer 2 counter l
		break;
	case MB_CNT2H: // timer 2 counter h
		ss->psg_data[chan][MB_IFR] &= ~0x20;
		fix_ifr(chan, ss);
		start_timer(chan, 1, ss);
		break;
	case MB_ACR: // ACR
		break;
	case MB_PCR: psg_pcr(chan, data, ss); break;
	case MB_IFR: break;
	case MB_IER: break;
	default:
		printf("mockingboard: rom write %x, %02x\n", adr, data);
	}
}


static byte mb_rom_r(word adr, struct SOUND_STATE*ss) // CX00-CXFF
{
	int chan = 0;
	if (adr & 0x80) chan = 1;
	switch (adr & 0x0F) {
	case MB_CNT1L: 
		ss->psg_data[chan][MB_IFR] &= ~0x40;
		break;
	case MB_ORAX: adr = MB_ORA; break;
	default:
		printf("mockingboard: rom read %x\n", adr);
	}
	return ss->psg_data[chan][adr&0x0F];
}



static void mb_io_w(word adr, byte data, struct SOUND_STATE*ss) // C0X0-C0XF
{
//	printf("mockingboard: io write %x, %02x\n", adr, data);
}


static byte mb_io_r(word adr, struct SOUND_STATE*ss) // C0X0-C0XF
{
//	printf("mockingboard: io read %x\n", adr);
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
