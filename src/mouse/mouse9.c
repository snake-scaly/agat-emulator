/*
	Agat Emulator version 1.4
	Copyright (c) NOP, nnop@newmail.ru
	mouse9 module
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

struct PRINTER_STATE
{
	struct SLOT_RUN_STATE*st;

	byte rom1[256], rom2[2048];
	byte rom_mode; // bit 1 -> C0X3, bit 2 -> CX00
	byte regs[3];
	int lastpos[2];
	int mousetype; // 0 - none, 1 - mm8031, 2 - mars
};

static int printer_term(struct SLOT_RUN_STATE*st)
{
	--st->sr->mouselock;
	free(st->data);
	return 0;
}

static void enable_printer_rom(struct PRINTER_STATE*pcs, int en);

static void set_rom_mode(struct PRINTER_STATE*pcs, byte mode)
{
	if ((mode ^ pcs->rom_mode) & 3) {
		if ((pcs->rom_mode & 3) == 3) enable_printer_rom(pcs, 0);
		if ((mode & 3) == 3) enable_printer_rom(pcs, 1);
		pcs->rom_mode = mode;
	}
}


static int printer_save(struct SLOT_RUN_STATE*st, OSTREAM*out)
{
	struct PRINTER_STATE*pcs = st->data;
	WRITE_FIELD(out, pcs->rom_mode);
	WRITE_ARRAY(out, pcs->regs);

	return 0;
}

static int printer_load(struct SLOT_RUN_STATE*st, ISTREAM*in)
{
	struct PRINTER_STATE*pcs = st->data;

	READ_FIELD(in, pcs->rom_mode);
	READ_ARRAY(in, pcs->regs);

	if ((pcs->rom_mode & 3) == 3) enable_printer_rom(pcs, 1);
	else enable_printer_rom(pcs, 0);

	return 0;
}

#define PRN_BASE_CMD 8000

static int printer_command(struct SLOT_RUN_STATE*st, int cmd, int data, long param)
{
	struct PRINTER_STATE*pcs = st->data;
	HMENU menu;
	switch (cmd) {
	case SYS_COMMAND_RESET:
		set_rom_mode(pcs, pcs->rom_mode & ~1);
		return 0;
	case SYS_COMMAND_HRESET:
		set_rom_mode(pcs, 0);
		return 0;
	}
	return 0;
}

static byte printer_xrom_r(word adr, struct PRINTER_STATE*pcs); // C800-CFFF

static void enable_printer_rom(struct PRINTER_STATE*pcs, int en)
{
	enable_slot_xio(pcs->st, en);
}

static byte printer_xrom_r(word adr, struct PRINTER_STATE*pcs) // C800-CFFF
{
	if ((adr & 0xF00) == 0xF00) {
		set_rom_mode(pcs, pcs->rom_mode & ~2);
	}
	return pcs->rom2[adr & (sizeof(pcs->rom2)-1)];
}

static byte printer_rom_r(word adr, struct PRINTER_STATE*pcs) // CX00-CXFF
{
	set_rom_mode(pcs, pcs->rom_mode | 2);
	return pcs->rom1[adr & 0xFF];
}

#define DIV_H 600
#define DIV_V 600
#define MAX_H 4
#define MAX_V 4

static int otable[8] = {0, 1, 3, 6, 15, 35, 70, 100};

static int odec(int ofs)
{
	int s = 1;
	int i;
	if (ofs < 0) { s = -1; ofs = -ofs; }
	for (i = 7; i >= 0; --i)
		if (otable[i] <= ofs) break;
	return i * s;
}


static int oenc(int ofs)
{
	int s = 1;
	int i;
	if (ofs < 0) { s = -1; ofs = -ofs; }
	return otable[ofs] * s;
}

static byte update_mm8031(struct PRINTER_STATE*pcs, byte res)
{
	if (pcs->st->sr->mousebtn & 1) res &= ~0x80;
	else res |= 0x80;
	if (pcs->st->sr->mousebtn & 2) res &= ~0x40;
	else res |= 0x40;
	return res;
}

static byte read_mm8031(struct PRINTER_STATE*pcs)
{
	int ofs = 0;
	byte res = 0x03;
	if (pcs->lastpos[0] == -1 && pcs->lastpos[1] == -1) {
		pcs->lastpos[0] = pcs->st->sr->xmouse/DIV_H;
		pcs->lastpos[1] = pcs->st->sr->ymouse/DIV_V;
	}
	if (pcs->regs[0] & 0x80) { // delta y
		ofs = odec(pcs->st->sr->ymouse/DIV_V - pcs->lastpos[1]);
		if (ofs > MAX_V) ofs = MAX_V;
		if (ofs < -MAX_V) ofs = -MAX_V;
		pcs->lastpos[1] += oenc(ofs);
		ofs = -ofs;
	} else { // delta x
		ofs = odec(pcs->st->sr->xmouse/DIV_H - pcs->lastpos[0]);
		if (ofs > MAX_H) ofs = MAX_H;
		if (ofs < -MAX_H) ofs = -MAX_H;
		pcs->lastpos[0] += oenc(ofs);
	}
//	printf("ofs = %i\n", ofs);
//	printf("(%i,%i)->(%i,%i); ofs = %i\n", pcs->lastpos[0], pcs->lastpos[1], pcs->st->sr->xmousepos/DIV_H, pcs->st->sr->ymousepos/DIV_V, ofs);
	ofs &= 15;
	ofs ^= 8;
	ofs <<= 2;
	res |= ofs;
	if (pcs->st->sr->mousebtn & 1) res &= ~0x80;
	else res |= 0x80;
	if (pcs->st->sr->mousebtn & 2) res &= ~0x40;
	else res |= 0x40;
	return res;
}

#undef DIV_H
#undef DIV_V
#define DIV_H 600
#define DIV_V 600

static byte update_mars(struct PRINTER_STATE*pcs, byte res)
{
	if (pcs->st->sr->mousebtn & 1) res &= ~0x80;
	else res |= 0x80;
	if (pcs->st->sr->mousebtn & 2) res &= ~0x40;
	else res |= 0x40;
	return res;
}

static byte read_mars(struct PRINTER_STATE*pcs)
{
	int ofs = 0;
	byte res = pcs->regs[2] | 0x0F;
	if (pcs->lastpos[0] == -1 && pcs->lastpos[1] == -1) {
		pcs->lastpos[0] = pcs->st->sr->xmouse/DIV_H;
		pcs->lastpos[1] = pcs->st->sr->ymouse/DIV_V;
	}
// delta y
	ofs = pcs->st->sr->ymouse/DIV_V - pcs->lastpos[1];
	if (ofs < 0) res &= ~2;
	if (ofs > 0) res &= ~1;
	if (ofs > 1) ofs = 1;
	if (ofs < -1) ofs = -1;
	pcs->lastpos[1] += ofs;
// delta x
	ofs = pcs->st->sr->xmouse/DIV_H - pcs->lastpos[0];
	if (ofs > 0) res &= ~8;
	if (ofs < 0) res &= ~4;
	if (ofs > 1) ofs = 1;
	if (ofs < -1) ofs = -1;
	pcs->lastpos[0] += ofs;
	if (pcs->st->sr->mousebtn & 1) res &= ~0x80;
	else res |= 0x80;
	if (pcs->st->sr->mousebtn & 2) res &= ~0x40;
	else res |= 0x40;
	return res;
}



static void printer_io_w(word adr, byte data, struct PRINTER_STATE*pcs) // C0X0-C0XF
{
//	printf("mouse: write reg %x: %02x\n", adr, data);
//	{ extern int cpu_debug; cpu_debug = 1; }
//	dump_mem(pcs->st->sr, 0, 0x10000, "memdump.bin");
	adr &= 0x03;
	switch (adr) {
	case 1:
		break;
	case 0:
		if (data&0x80 && pcs->mousetype == MOUSE_MARS) {
			pcs->regs[2] |= 0x0F; // reset motion bits
		}	
		pcs->regs[adr] = data;
		switch (pcs->mousetype) {
		case MOUSE_NONE:
			break;
		case MOUSE_MM8031:
			pcs->regs[2] = read_mm8031(pcs);
			break;
		case MOUSE_MARS:
			break;
		}
		break;
	case 3:
		set_rom_mode(pcs, pcs->rom_mode | 1);
		break;
	}
}

static byte printer_io_r(word adr, struct PRINTER_STATE*pcs) // C0X0-C0XF
{
//	printf("read mouse reg %x\n", adr);
	adr &= 0x03;
	switch (adr) {
	case 2:
		switch (pcs->mousetype) {
		case MOUSE_NONE:
			break;
		case MOUSE_MM8031:
			pcs->regs[2] = update_mm8031(pcs, pcs->regs[2]);
			break;
		case MOUSE_MARS:
			pcs->regs[2] = read_mars(pcs);
			break;
		}
//		printf("read mouse reg: %x\n", pcs->regs[2]);
		return pcs->regs[2];
	}
	return empty_read(adr, pcs);
}


int  mouse9_init(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*st, struct SLOTCONFIG*cf)
{
	int i;
	ISTREAM*rom;
	struct PRINTER_STATE*pcs;
	int mode = cf->cfgint[CFG_INT_PRINT_MODE];
	unsigned fl = 0;

	puts("in mouse9_init");

	pcs = calloc(1, sizeof(*pcs));
	if (!pcs) return -1;

	pcs->st = st;
	pcs->lastpos[0] = pcs->lastpos[1] = -1;

	pcs->regs[2] = 0x21 | 0x40 | 0x80;

	pcs->mousetype = cf->cfgint[CFG_INT_MOUSE_TYPE];
	switch (pcs->mousetype) {
	case MOUSE_MM8031:
		pcs->regs[2] = 0x23 | 0x40 | 0x80;
		break;
	case MOUSE_MARS:
		pcs->regs[2] = 0x40 | 0x80 | 0x20 | 0x10;
		break;
	}

	rom = isfopen(cf->cfgstr[CFG_STR_ROM]);
	if (!rom) { free(pcs); return -1; }
	isread(rom, pcs->rom1, sizeof(pcs->rom1));
	isclose(rom);

	rom = isfopen(cf->cfgstr[CFG_STR_ROM2]);
	if (!rom) { free(pcs); return -2; }
	isread(rom, pcs->rom2, sizeof(pcs->rom2));
	isclose(rom);

	st->data = pcs;
	st->free = printer_term;
	st->command = printer_command;
	st->load = printer_load;
	st->save = printer_save;

	++sr->mouselock;

	fill_rw_proc(st->io_sel, 1, printer_rom_r, empty_write, pcs);
	fill_rw_proc(st->baseio_sel, 1, printer_io_r, printer_io_w, pcs);
	fill_rw_proc(&st->xio_sel, 1, printer_xrom_r, empty_write, pcs);

	return 0;
}
