/*
	Agat Emulator version 1.8
	Copyright (c) NOP, nnop@newmail.ru
	saturnmem - emulation of Saturn Ram Board
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

#define BANK_SIZE 0x4000

struct SATURN_STATE
{
	struct SLOT_RUN_STATE*st;
	int	card_32;
	dword  	memsize;
	byte	read, write, last_write;
	dword	bank_ofs, page_ofs;
	byte	*ram;
};

static byte read_mem_d000(word adr, struct SATURN_STATE*scs);
static byte read_mem_e000(word adr, struct SATURN_STATE*scs);
static void write_mem_d000(word adr, byte d, struct SATURN_STATE*scs);
static void write_mem_e000(word adr, byte d, struct SATURN_STATE*scs);


static void select_bank(byte state, struct SATURN_STATE*scs)
{
	int bank = state & 3;
	if (state & 8) bank |= 4;
	scs->bank_ofs = (bank * BANK_SIZE) & (scs->memsize - 1);
}

static void select_access(int r, int w, int h, struct SATURN_STATE*scs)
{
//	printf("saturn: select_access(%i, %i, %i)\n", r, w, h);
	scs->read = r;
	scs->write = w;
	scs->page_ofs = h?0x1000:0;
	if (r) {
		fill_read_proc(scs->st->sr->base_mem+26,2,read_mem_d000,scs);
		fill_read_proc(scs->st->sr->base_mem+28,4,read_mem_e000,scs);
	} else {
		system_command(scs->st->sr, SYS_COMMAND_PSROM_RELEASE, scs->st->sc->slot_no, 0);
	}
	fill_write_proc(scs->st->sr->base_mem+26,2,w?write_mem_d000:empty_write,scs);
	fill_write_proc(scs->st->sr->base_mem+28,4,w?write_mem_e000:empty_write,scs);
}

static int saturn_term(struct SLOT_RUN_STATE*st)
{
	struct SATURN_STATE*scs = st->data;
	free(scs->ram);
	free(st->data);
	return 0;
}

static int saturn_save(struct SLOT_RUN_STATE*st, OSTREAM*out)
{
	struct SATURN_STATE*scs = st->data;

	WRITE_FIELD(out, scs->read);
	WRITE_FIELD(out, scs->write);
	WRITE_FIELD(out, scs->last_write);
	WRITE_FIELD(out, scs->bank_ofs);
	WRITE_FIELD(out, scs->page_ofs);
	oswrite(out, scs->ram, scs->memsize);

	return 0;
}

static int saturn_load(struct SLOT_RUN_STATE*st, ISTREAM*in)
{
	struct SATURN_STATE*scs = st->data;
	READ_FIELD(in, scs->read);
	READ_FIELD(in, scs->write);
	READ_FIELD(in, scs->last_write);
	READ_FIELD(in, scs->bank_ofs);
	READ_FIELD(in, scs->page_ofs);
	isread(in, scs->ram, scs->memsize);
	return 0;
}

static int saturn_clear(struct SATURN_STATE*scs)
{
	clear_block(scs->ram, scs->memsize);
	scs->bank_ofs = 0;
	scs->last_write = 0;
	select_access(0, 0, 0, scs);
	return 0;
}

static int saturn_command(struct SLOT_RUN_STATE*st, int cmd, int data, long param)
{
	struct SATURN_STATE*scs = st->data;
	switch (cmd) {
	case SYS_COMMAND_RESET:
		return 0;
	case SYS_COMMAND_HRESET:
		saturn_clear(scs);
		return 0;
	case SYS_COMMAND_LOAD_DONE:
		select_access(scs->read, scs->write, scs->page_ofs, scs);
		return 0;
	}
	return 0;
}


static byte read_mem_d000(word adr, struct SATURN_STATE*scs)
{
	dword memadr = scs->bank_ofs + scs->page_ofs + (adr & 0xFFF);
	return scs->ram[memadr];
}

static byte read_mem_e000(word adr, struct SATURN_STATE*scs)
{
	dword memadr = scs->bank_ofs + 0x2000 + (adr & 0x1FFF);
	return scs->ram[memadr];
}

static void write_mem_d000(word adr, byte d, struct SATURN_STATE*scs)
{
	dword memadr = scs->bank_ofs + scs->page_ofs + (adr & 0xFFF);
	scs->ram[memadr] = d;
}

static void write_mem_e000(word adr, byte d, struct SATURN_STATE*scs)
{
	dword memadr = scs->bank_ofs + 0x2000 + (adr & 0x1FFF);
	scs->ram[memadr] = d;
}

static byte get_state(struct SATURN_STATE*scs)
{
	byte res = 0xC0;
	if (scs->page_ofs) res |= 8;
	if (scs->write) res |= 1;
	if (scs->read == scs->write) res |= 2;
	return res;
}

static byte read_state(word adr, struct SATURN_STATE*scs)
{
	byte old_state = get_state(scs);
//	printf("saturn read state %x\n", adr);
	if (adr & 4) {
		if (scs->card_32) select_bank(1, scs);
		else {
			select_bank((byte)adr, scs);
			return old_state;
		}	
	} else {
		if (scs->card_32) select_bank(0, scs);
	}
	switch (adr & 3) {
		case 0: select_access(1, 0, adr & 8, scs); scs->last_write = 0; break;
		case 1: select_access(0, scs->last_write, adr & 8, scs); scs->last_write = 1; break;
		case 2: select_access(0, 0, adr & 8, scs); scs->last_write = 0; break;
		case 3: select_access(1, scs->last_write, adr & 8, scs); scs->last_write = 1; break;
	}
	return old_state;
}

static void write_state(word adr, byte d, struct SATURN_STATE*scs)
{
	read_state(adr, scs);
}


int  saturnmem_init(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*st, struct SLOTCONFIG*cf)
{
	struct SATURN_STATE*scs;

	puts("in saturnmem_init");

	scs = calloc(1, sizeof(*scs));
	if (!scs) return -1;

	scs->st = st;

	scs->memsize = memsizes_b[cf->cfgint[CFG_INT_MEM_SIZE]];
	if (scs->memsize < 64*1024) scs->card_32 = 1;

	printf("saturn memsize: %i bytes\n", scs->memsize);

	scs->ram = malloc(scs->memsize);
	if (!scs->ram) { free(scs); return -1; }
	clear_block(scs->ram, scs->memsize);

	st->data = scs;
	st->free = saturn_term;
	st->command = saturn_command;
	st->load = saturn_load;
	st->save = saturn_save;

	fill_rw_proc(st->baseio_sel, 1, read_state, write_state, scs);

	puts("saturn initialized");
	return 0;
}
