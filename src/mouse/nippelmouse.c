/*
	Agat Emulator version 1.14
	Copyright (c) NOP, nnop@newmail.ru
	nippelmouse module
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


struct MOUSE_STATE
{
	struct SLOT_RUN_STATE*st;
	int regs[2];
};

#define DIV_X 600
#define DIV_Y 600

static int mouse_term(struct SLOT_RUN_STATE*st)
{
	--st->sr->mouselock;
	free(st->data);
	return 0;
}

static int mouse_save(struct SLOT_RUN_STATE*st, OSTREAM*out)
{
	struct MOUSE_STATE*mcs = st->data;
	WRITE_ARRAY(out, mcs->regs);
	return 0;
}

static int mouse_load(struct SLOT_RUN_STATE*st, ISTREAM*in)
{
	struct MOUSE_STATE*mcs = st->data;
	READ_ARRAY(in, mcs->regs);
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
		mcs->regs[0] = mcs->regs[1] = 0;
		return 0;
	case SYS_COMMAND_MOUSE_EVENT:
		if (data & 0x80) {
//			puts("mouse move");
			mcs->regs[0] += mcs->st->sr->dxmouse;
			mcs->regs[1] -= mcs->st->sr->dymouse;
		}
		return 0;
	}
	return 0;
}


static void mouse_io_w(word adr, byte data, struct MOUSE_STATE*mcs) // C0X0-C0XF
{
//	printf("mouse: write reg %x: %02x\n", adr, data);
	switch (adr & 0x0F) {
	case 0x08:
		mcs->regs[0] = 0x22 * DIV_X;
		mcs->regs[1] = 0x22 * DIV_Y;
		break;
	case 0x0C:
		mcs->regs[0] = mcs->regs[1] = 0;
		break;
	}
}


static byte mouse_io_r(word adr, struct MOUSE_STATE*mcs) // C0X0-C0XF
{
	byte d;
	switch (adr & 0x0F) {
	case 0x08:
	case 0x0C:
		d = (mcs->regs[0] / DIV_X) & 0x0F;
		break;
	case 0x09:
		d = ((mcs->regs[0] / DIV_X) >> 4) & 0x07;
		if (mcs->st->sr->mousebtn & 2) d |= 0x08;
		break;
	case 0x0A:
		d = (mcs->regs[1] / DIV_Y) & 0x0F;
		break;
	case 0x0B:
		d = ((mcs->regs[1] / DIV_Y) >> 4) & 0x07;
		if (mcs->st->sr->mousebtn & 1) d |= 0x08;
		break;
	}
//	printf("read mouse reg %x = %x\n", adr, d);
	return d;
}


int  nippelmouse_init(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*st, struct SLOTCONFIG*cf)
{
	struct MOUSE_STATE*mcs;

	puts("in nippelmouse_init");

	mcs = calloc(1, sizeof(*mcs));
	if (!mcs) return -1;

	mcs->st = st;

	st->data = mcs;
	st->free = mouse_term;
	st->command = mouse_command;
	st->load = mouse_load;
	st->save = mouse_save;

	++sr->mouselock;

	fill_rw_proc(st->baseio_sel, 1, mouse_io_r, mouse_io_w, mcs);

	return 0;
}
