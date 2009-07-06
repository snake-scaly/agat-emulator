/*
	Agat Emulator version 1.8
	Copyright (c) NOP, nnop@newmail.ru
	a2ramcard - emulation of Apple ][ Memory Expansion Card / RamFactor
*/

#include "types.h"
#include "memory.h"
#include "streams.h"
#include "debug.h"
#include "sysconf.h"
#include "runmgr.h"
#include "runmgrint.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "syslib.h"
#include "debug.h"

#include "localize.h"


#define MAX_ROM_SIZE	4096
#define MAX_ROM_PAGES	2


static void ramcard_rom_select(struct RAMCARD_STATE*rcs);
static void ramcard_rom_disable(struct RAMCARD_STATE*rcs);

static void ramcard_io_select(struct RAMCARD_STATE*rcs);
static void ramcard_io_disable(struct RAMCARD_STATE*rcs);

struct RAMCARD_STATE
{
	struct SLOT_RUN_STATE*st;
	int	cardtype;
	dword  	memsize;
	union {
		dword curaddr;
		byte  curaddr_b[4];
	};
	int	io_enabled;
	word	rom_offset;
	byte	rom[MAX_ROM_SIZE * MAX_ROM_PAGES];
	byte	*ram;
};



static int ramcard_term(struct SLOT_RUN_STATE*st)
{
	struct RAMCARD_STATE*rcs = st->data;
	if (st->sc->cfgstr[CFG_STR_RAM][0]) {
		OSTREAM*out;
		out = osfopen(st->sc->cfgstr[CFG_STR_RAM]);
		if (out) {
			oswrite(out, rcs->ram, rcs->memsize);
			osclose(out);
		}
	}
	free(rcs->ram);
	free(st->data);
	return 0;
}

static int ramcard_save(struct SLOT_RUN_STATE*st, OSTREAM*out)
{
	struct RAMCARD_STATE*rcs = st->data;

	WRITE_FIELD(out, rcs->curaddr);
	WRITE_FIELD(out, rcs->io_enabled);
	oswrite(out, rcs->ram, rcs->memsize);

	return 0;
}

static int ramcard_load(struct SLOT_RUN_STATE*st, ISTREAM*in)
{
	struct RAMCARD_STATE*rcs = st->data;
	READ_FIELD(in, rcs->curaddr);
	READ_FIELD(in, rcs->io_enabled);
	isread(in, rcs->ram, rcs->memsize);

	if (rcs->io_enabled) ramcard_io_select(rcs);
	else ramcard_io_disable(rcs);
	return 0;
}

static int ramcard_clear(struct RAMCARD_STATE*rcs)
{
	clear_block(rcs->ram, rcs->memsize);
	ramcard_rom_disable(rcs);
	ramcard_io_disable(rcs);
	return 0;
}

static int ramcard_command(struct SLOT_RUN_STATE*st, int cmd, int data, long param)
{
	struct RAMCARD_STATE*rcs = st->data;
	switch (cmd) {
	case SYS_COMMAND_RESET:
		ramcard_io_disable(rcs);
		ramcard_rom_disable(rcs);
		return 0;
	case SYS_COMMAND_HRESET:
		ramcard_clear(rcs);
		return 0;
	}
	return 0;
}


static byte ramcard_rom_r(word adr, struct RAMCARD_STATE*rcs) // CX00-CFFF
{
	adr &= (MAX_ROM_SIZE - 1);
	if (adr == 0xFFF) {
		ramcard_rom_disable(rcs);
	} else if (adr < 0x800) {
		ramcard_io_select(rcs);
		ramcard_rom_select(rcs);
	}
	return rcs->rom[adr + rcs->rom_offset];
}


static void write_data(struct RAMCARD_STATE*rcs, dword curaddr, byte data)
{
	curaddr &= (rcs->memsize - 1);
//	printf("write[%x]=%x\n", curaddr, data);
	rcs->ram[curaddr] = data;
}

static byte read_data(struct RAMCARD_STATE*rcs, dword curaddr)
{
	curaddr &= (rcs->memsize - 1);
//	printf("read[%x]=%x\n", curaddr, rcs->ram[curaddr]);
	return rcs->ram[curaddr];
}

static void select_rom_bank(struct RAMCARD_STATE*rcs, int bank)
{
	if (rcs->cardtype == 1) {
//		printf("select rom bank %i\n", bank);
		rcs->rom_offset = MAX_ROM_SIZE * bank;
	}
}

static void select_addr(struct RAMCARD_STATE*rcs, int ind, byte data)
{
	if (ind < 2 && (rcs->curaddr_b[ind] & 0x80) && !(data&0x80)) {
		select_addr(rcs, ind + 1, rcs->curaddr_b[ind + 1] + 1);
	}
	rcs->curaddr_b[ind] = data;
}

static void ramcard_io_w(word adr, byte data, struct RAMCARD_STATE*rcs) // C0X0-C0XF
{
//		printf("ramcard: io[%04X] = %02X\n", adr, data);
	adr &= 0x0F;
	switch (adr) {
	case 0: select_addr(rcs, 0, data); break;
	case 1: select_addr(rcs, 1, data); break;
	case 2: select_addr(rcs, 2, data); break;
	case 3: write_data(rcs, rcs->curaddr++, data); break;
	case 15: select_rom_bank(rcs, data & 1); break;
	default:;
		printf("ramcard: io[%04X] = %02X\n", adr, data);
	}
}

static byte ramcard_io_r(word adr, struct RAMCARD_STATE*rcs) // C0X0-C0XF
{
//		printf("ramcard: read io[%04X]\n", adr);
	adr &= 0x0F;
	switch (adr) {
	case 0: return rcs->curaddr_b[0];
	case 1: return rcs->curaddr_b[1];
	case 2: return rcs->curaddr_b[2] | ((~(rcs->memsize - 1) >> 16) & 0xF0);
	case 3: return read_data(rcs, rcs->curaddr++);
	case 15: return rcs->rom_offset?1:0;
	default:;
		printf("ramcard: read io[%04X]\n", adr);
	}
	return 0;
}

int  a2ramcard_init(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*st, struct SLOTCONFIG*cf)
{
	struct RAMCARD_STATE*rcs;
	ISTREAM*rom;

	puts("in a2ramcard_init");

	rcs = calloc(1, sizeof(*rcs));
	if (!rcs) return -1;

	rcs->st = st;

	rom = isfopen(cf->cfgstr[CFG_STR_ROM]);
	if (!rom) { free(rcs); return -1; }
	isread(rom, rcs->rom, sizeof(rcs->rom));
	isclose(rom);

	rcs->cardtype = cf->dev_type == DEV_RAMFACTOR;
	rcs->memsize = memsizes_b[cf->cfgint[CFG_INT_MEM_SIZE]];

	printf("ramcard memsize: %i bytes\n", rcs->memsize);

	rcs->ram = malloc(rcs->memsize);
	if (!rcs->ram) { free(rcs); return -1; }
	clear_block(rcs->ram, rcs->memsize);

	if (cf->cfgstr[CFG_STR_RAM][0]) {
		ISTREAM*in;
		in = isfopen(cf->cfgstr[CFG_STR_RAM]);
		if (in) {
			isread(in, rcs->ram, rcs->memsize);
			isclose(in);
		}
	}

	st->data = rcs;
	st->free = ramcard_term;
	st->command = ramcard_command;
	st->load = ramcard_load;
	st->save = ramcard_save;

	fill_rw_proc(st->io_sel, 1, ramcard_rom_r, empty_write, rcs);
	fill_rw_proc(&st->xio_sel, 1, ramcard_rom_r, empty_write, rcs);
	puts("ramcard initialized");
	return 0;
}

static void ramcard_rom_select(struct RAMCARD_STATE*rcs)
{
	enable_slot_xio(rcs->st, 1);
}


static void ramcard_rom_disable(struct RAMCARD_STATE*rcs)
{
	enable_slot_xio(rcs->st, 0);
}


static void ramcard_io_select(struct RAMCARD_STATE*rcs)
{
	if (rcs->io_enabled) return;
	rcs->io_enabled = 1;
	fill_rw_proc(rcs->st->baseio_sel, 1, ramcard_io_r, ramcard_io_w, rcs);
}


static void ramcard_io_disable(struct RAMCARD_STATE*rcs)
{
	if (!rcs->io_enabled) return;
	rcs->io_enabled = 0;
	fill_rw_proc(rcs->st->baseio_sel, 1, empty_read, empty_write, rcs);
}

