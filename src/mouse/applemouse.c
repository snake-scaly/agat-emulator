/*
	Agat Emulator version 1.14
	Copyright (c) NOP, nnop@newmail.ru
	applemouse module
*/

#include "types.h"
#include "memory.h"
#include "streams.h"
#include "debug.h"
#include "sysconf.h"
#include "runmgr.h"
#include "runmgrint.h"

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "syslib.h"
#include "debug.h"

#include "localize.h"
#include "mc6821.h"


#define MREG_DATA	0
#define MREG_CONTROL	1

struct MOUSE_STATE
{
	struct SLOT_RUN_STATE*st;

	byte rom[2048];
	byte last_control;
	byte cmdbuf[8];
	byte cmdlen, cmdofs;
	byte mode; // 1 - read, 2 - write, 0 - wait
	byte state, mmode;
	int  range[2][2];
	int  pos[2];
	int  blank_freq;
	struct MC6821_STATE mc6821;
};

#define DIV_X 256
#define DIV_Y 256

static int mouse_term(struct SLOT_RUN_STATE*st)
{
	--st->sr->mouselock;
	free(st->data);
	return 0;
}

static int mouse_save(struct SLOT_RUN_STATE*st, OSTREAM*out)
{
	struct MOUSE_STATE*mcs = st->data;
	WRITE_FIELD(out, mcs->mc6821);
	WRITE_FIELD(out, mcs->last_control);
	WRITE_ARRAY(out, mcs->cmdbuf);
	WRITE_FIELD(out, mcs->cmdlen);
	WRITE_FIELD(out, mcs->cmdofs);
	WRITE_FIELD(out, mcs->mode);
	WRITE_FIELD(out, mcs->state);
	WRITE_FIELD(out, mcs->mmode);
	WRITE_ARRAY(out, mcs->range);
	WRITE_ARRAY(out, mcs->pos);
	WRITE_FIELD(out, mcs->blank_freq);
	return 0;
}

static int mouse_load(struct SLOT_RUN_STATE*st, ISTREAM*in)
{
	struct MOUSE_STATE*mcs = st->data;

	READ_FIELD(in, mcs->mc6821);
	READ_FIELD(in, mcs->last_control);
	READ_ARRAY(in, mcs->cmdbuf);
	READ_FIELD(in, mcs->cmdlen);
	READ_FIELD(in, mcs->cmdofs);
	READ_FIELD(in, mcs->mode);
	READ_FIELD(in, mcs->state);
	READ_FIELD(in, mcs->mmode);
	READ_ARRAY(in, mcs->range);
	READ_ARRAY(in, mcs->pos);
	READ_FIELD(in, mcs->blank_freq);
	if (mcs->mmode & 0x08) { // VBE
		system_command(st->sr, SYS_COMMAND_SET_CPUTIMER, 1000000/mcs->blank_freq, DEF_CPU_TIMER_ID(mcs->st));
	}
	return 0;
}

static int mouse_command(struct SLOT_RUN_STATE*st, int cmd, int data, long param)
{
	struct MOUSE_STATE*mcs = st->data;
	HMENU menu;
	switch (cmd) {
	case SYS_COMMAND_RESET:
		return 0;
	case SYS_COMMAND_HRESET:
		mc6821_reset(&mcs->mc6821);
		mcs->blank_freq = 60;
		mcs->mode = 0;
		mcs->state = 0;
		mcs->mmode = 0;
		return 0;
	case SYS_COMMAND_MOUSE_EVENT:
//		printf("mouse event: mmode = %x\n", mcs->mmode);
		if (data & 3) mcs->state |= 0x04; // btn
		if (data & 0x80) {
//			puts("mouse move");
			mcs->state |= 0x22; // move
			mcs->pos[0] += mcs->st->sr->dxmouse;
			mcs->pos[1] += mcs->st->sr->dymouse;
			if (mcs->pos[0]/DIV_X > mcs->range[0][1]) mcs->pos[0] = mcs->range[0][1]*DIV_X;
			if (mcs->pos[0]/DIV_X < mcs->range[0][0]) mcs->pos[0] = mcs->range[0][0]*DIV_X;
			if (mcs->pos[1]/DIV_Y > mcs->range[1][1]) mcs->pos[1] = mcs->range[1][1]*DIV_Y;
			if (mcs->pos[1]/DIV_Y < mcs->range[1][0]) mcs->pos[1] = mcs->range[1][0]*DIV_Y;
		}
		if (mcs->mmode & 1) { // enabled
			if ((mcs->mmode&mcs->state)&0x06)
				system_command(mcs->st->sr, SYS_COMMAND_IRQ, 0, 0);
		}
		return 0;
	case SYS_COMMAND_CPUTIMER:
		if (param == DEF_CPU_TIMER_ID(mcs->st)) {
//			puts("vb event");
			if (mcs->mmode & 1) { // enabled
				mcs->state |= 0x08; // VBE
				if (mcs->mmode&0x08)
					system_command(mcs->st->sr, SYS_COMMAND_IRQ, 0, 0);
			}
			return 1;
		}
		return 0;
	}
	return 0;
}


static byte mouse_rom_r(word adr, struct MOUSE_STATE*mcs) // CX00-CXFF
{
	word ofs = (mc6821_get_data(&mcs->mc6821, MREG_CONTROL)<<7)&0x700;
	return mcs->rom[(adr & 0xFF) + ofs];
}

static void mouse_cmd(struct MOUSE_STATE*mcs, const byte*buf, int len)
{
//	printf("mouse_write: %x, %i bytes\n", buf[0], len);
	switch (buf[0] >> 4) {
	case 0:
//		printf("set mouse state: %x\n", buf[0]&0x15);
		mcs->mmode = buf[0] & 15;
		if (mcs->mmode & 0x08) {
			system_command(mcs->st->sr, SYS_COMMAND_SET_CPUTIMER, 1000000/mcs->blank_freq, DEF_CPU_TIMER_ID(mcs->st));
		}
		break;
	case 1: {
		int mx, my;
		mcs->mode = 1;
		mcs->cmdlen = 5;
		mx = mcs->pos[0] / DIV_X;
		my = mcs->pos[1] / DIV_Y;
//		printf("read mouse pos: %x, %x\n", mx, my);
		mcs->cmdbuf[0] = mx & 0xFF;
		mcs->cmdbuf[1] = mx >> 8;
		mcs->cmdbuf[2] = my & 0xFF;
		mcs->cmdbuf[3] = my >> 8;
		mcs->cmdbuf[4] = mcs->state & 0x20; // move only
		if (mcs->state & 0x80) mcs->cmdbuf[4] |= 0x40;
		if (mcs->st->sr->mousebtn&0x1) {
			mcs->cmdbuf[4] |= 0x80;
			mcs->state |= 0x80;
		} else mcs->state &= ~0x80;

		if (mcs->state & 0x08) mcs->cmdbuf[4] |= 0x01;
		if (mcs->st->sr->mousebtn&0x2) {
			mcs->cmdbuf[4] |= 0x08;
			mcs->state &= 0x08;
		} else mcs->state &=~0x08;
		mcs->state &= ~0x20;
		return; }
	case 2:
//		puts("acknowledge int");
		mcs->mode = 1;
		mcs->cmdlen = 1;
		mcs->cmdbuf[0] = mcs->state & ~0x20;
		mcs->state &= ~0x0E;
		system_command(mcs->st->sr, SYS_COMMAND_NOIRQ, 0, 0);
		return;
	case 3:
//		puts("clear mouse pos");
		mcs->pos[0] = 0;
		mcs->pos[1] = 0;
		break;
	case 4:
//		printf("set mouse pos %x %x %x %x\n", buf[1], buf[2], buf[3], buf[4]);
		mcs->pos[0] = (buf[1] | (buf[2]<<8)) * DIV_X;
		mcs->pos[1] = (buf[3] | (buf[4]<<8)) * DIV_Y;
		break;
	case 5:
//		puts("init mouse");
		mcs->mode = 1;
		mcs->cmdlen = 2;
		mcs->cmdbuf[0] = 0xFF;
		mcs->cmdbuf[1] = 0xFF;
		mcs->pos[0] = 0;
		mcs->pos[1] = 0;
		mcs->range[0][0] = mcs->range[1][0] = 0;
		mcs->range[0][1] = mcs->range[1][1] = 1024;
		return;
	case 6:
/*		printf("set mouse clamp %c: %x %x %x %x\n", 
			(buf[0]&1)?'Y':'X',
			buf[1], buf[2], buf[3], buf[4]);
*/			
		mcs->range[buf[0]&1][0] = buf[1] | (buf[3] << 8);
		mcs->range[buf[0]&1][1] = buf[2] | (buf[4] << 8);
		if (mcs->pos[0]/DIV_X > mcs->range[0][1]) mcs->pos[0] = mcs->range[0][1]*DIV_X;
		if (mcs->pos[0]/DIV_X < mcs->range[0][0]) mcs->pos[0] = mcs->range[0][0]*DIV_X;
		if (mcs->pos[1]/DIV_Y > mcs->range[1][1]) mcs->pos[1] = mcs->range[1][1]*DIV_Y;
		if (mcs->pos[1]/DIV_Y < mcs->range[1][0]) mcs->pos[1] = mcs->range[1][0]*DIV_Y;
		break;
	case 7:
//		puts("home mouse pos");
		mcs->pos[0] = mcs->range[0][0];
		mcs->pos[1] = mcs->range[1][0];
		break;
	case 9:
//		puts("set mouse params");
		mcs->blank_freq = (buf[0]&1)?50:60; // Hz
		if (mcs->mmode & 0x08) {
			system_command(mcs->st->sr, SYS_COMMAND_SET_CPUTIMER, 1000000/mcs->blank_freq, DEF_CPU_TIMER_ID(mcs->st));
		}
		break;
	default:
//		printf("unknown mouse command %x\n", buf[0]>>4);
		break;
	}
	mcs->mode = 0;
}


static byte mouse_read(struct MOUSE_STATE*mcs)
{
	byte d;
	if (mcs->mode != 1 || !mcs->cmdlen) return 0xFF;
	d = mcs->cmdbuf[mcs->cmdofs++];
	if (mcs->cmdofs == mcs->cmdlen) {
		mcs->mode = 0;
	}
//	printf("mouse_read: %x\n", d);
	return d;
}

static void mouse_write(struct MOUSE_STATE*mcs, byte data)
{
	static byte lens[16] = {
		1, 1, 1, 1, 5, 1, 5, 1,
		1, 1, 2, 1, 1, 1, 1, 1
	};
//	printf("mouse_write: %x\n", data);
	if (mcs->mode != 2 || !mcs->cmdlen) { //
		mcs->mode = 2; // write block
		mcs->cmdofs = 0;
		mcs->cmdlen = lens[data>>4];
	}
	if (mcs->mode == 2 && mcs->cmdofs < mcs->cmdlen) {
		mcs->cmdbuf[mcs->cmdofs++] = data;
		if (mcs->cmdofs == mcs->cmdlen) {
			mcs->mode = 0;
			mcs->cmdofs = 0;
			mc6821_set_data(&mcs->mc6821, MREG_DATA, mcs->cmdbuf[0]);
			mouse_cmd(mcs, mcs->cmdbuf, mcs->cmdlen);
			mc6821_set_data(&mcs->mc6821, MREG_DATA, mouse_read(mcs));
		}
	}
}

static void check_mouse(struct MOUSE_STATE*mcs)
{
	byte control = mc6821_get_data(&mcs->mc6821, MREG_CONTROL);
	byte dif = control ^ mcs->last_control;
//	printf("control = %x; last_control = %x; dif = %x\n", control, mcs->last_control, dif);
	if (dif & 0x20) { // write request
		if (control & 0x20) {
			control |= 0x80; // ready to read data being written
		} else {
			control &= ~0x80;
			mouse_write(mcs, mc6821_get_data(&mcs->mc6821, MREG_DATA));
		}
	}
	if (mcs->mode == 1 && mcs->cmdlen) control |= 0x40;
	if (dif & 0x10) { // read request
		if (control & 0x10) {
			control &= ~0x40;
		} else {
			control |= 0x40;
			mc6821_set_data(&mcs->mc6821, MREG_DATA, mouse_read(mcs));
		}
	}
	mc6821_set_data(&mcs->mc6821, MREG_CONTROL, control);
	mcs->last_control = control;
}

static void mouse_io_w(word adr, byte data, struct MOUSE_STATE*mcs) // C0X0-C0XF
{
//	printf("mouse: write reg %x: %02x\n", adr, data);
//	system_command(mcs->st->sr, SYS_COMMAND_DUMPCPUREGS, 0, 0);
	mc6821_write(&mcs->mc6821, adr, data);
	check_mouse(mcs);
}


static byte mouse_io_r(word adr, struct MOUSE_STATE*mcs) // C0X0-C0XF
{
	byte d;
	d = mc6821_read(&mcs->mc6821, adr);
//	printf("read mouse reg %x = %x\n", adr, d);
//	system_command(mcs->st->sr, SYS_COMMAND_DUMPCPUREGS, 0, 0);
	return d;
}


int  applemouse_init(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*st, struct SLOTCONFIG*cf)
{
	ISTREAM*rom;
	struct MOUSE_STATE*mcs;

	puts("in applemouse_init");

	mcs = calloc(1, sizeof(*mcs));
	if (!mcs) return -1;

	mcs->st = st;
	mcs->blank_freq = 60; // hz
	mcs->range[0][1] = mcs->range[1][1] = 1024;

	mc6821_reset(&mcs->mc6821);

	rom = isfopen(cf->cfgstr[CFG_STR_ROM]);
	if (!rom) { free(mcs); return -1; }
	isread(rom, mcs->rom, sizeof(mcs->rom));
	isclose(rom);

	st->data = mcs;
	st->free = mouse_term;
	st->command = mouse_command;
	st->load = mouse_load;
	st->save = mouse_save;

	++sr->mouselock;

	fill_rw_proc(st->io_sel, 1, mouse_rom_r, empty_write, mcs);
	fill_rw_proc(st->baseio_sel, 1, mouse_io_r, mouse_io_w, mcs);

	return 0;
}
