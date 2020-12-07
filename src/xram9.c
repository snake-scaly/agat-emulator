/*
	Agat Emulator version 1.0
	Copyright (c) NOP, nnop@newmail.ru
	baseram - emulation of base ROM module for both models

	Saturn RAM 128K emulation is incomplete
*/

#include "sysconf.h"
#include "types.h"
#include "streams.h"
#include "debug.h"
#include "syslib.h"

#include "memory.h"
#include "runmgrint.h"


#define MEM7_BLOCK_SIZE 0x4000
#define MEM7_BLOCK_SHIFT 0x0E

#define RAM9_BANK_SHIFT	13
#define RAM9_BANK_SIZE	0x2000

static const int startup_ram9_mapping[8]={0,1,2,3,4,5,6,7};

struct XRAM_STATE
{
	int   nslot;
	struct SYS_RUN_STATE*sr;
	byte *ram;
	dword ram_size;
	int   ram9_mapping[8];
	int   ram9_enabled[8];
	byte  psrom9_mode;
	int   psrom9_ofs;
	int   last_write;
};


static byte xram9_read_mapping(word adr,struct XRAM_STATE*st);
static void xram9_write_mapping(word adr,byte d,struct XRAM_STATE*st);

static byte xram9_read_psrom_mode(word adr,struct XRAM_STATE*st);
static void xram9_write_psrom_mode(word adr,byte d,struct XRAM_STATE*st);

static byte xram9_apple_read_psrom_mode(word adr,struct XRAM_STATE*st);
static void xram9_apple_write_psrom_mode(word adr,byte d,struct XRAM_STATE*st);

static byte xram9_read(word adr,struct XRAM_STATE*st);
static void xram9_write(word adr,byte d,struct XRAM_STATE*st);

static byte xram9_read_psrom(word adr,struct XRAM_STATE*st);
static void xram9_write_psrom(word adr,byte d,struct XRAM_STATE*st);

static int xram_free(struct SLOT_RUN_STATE*ss)
{
	struct XRAM_STATE*st = ss->data;
	free(st->ram);
	free(st);
	return 0;
}

static int xram_save(struct SLOT_RUN_STATE*ss, OSTREAM*out)
{
	struct XRAM_STATE*st = ss->data;
	WRITE_FIELD(out, st->ram_size);
	WRITE_ARRAY(out, st->ram9_mapping);
	WRITE_ARRAY(out, st->ram9_enabled);
	WRITE_FIELD(out, st->psrom9_mode);
	WRITE_FIELD(out, st->psrom9_ofs);
	oswrite(out, st->ram, st->ram_size);
/*	{
		int i;
		for (i = 0; i < 8; ++i) {
			printf("xram9_save: %i: %i,%i\n", i, st->ram9_mapping[i], st->ram9_enabled[i]);
		}
	}*/
	return 0;
}

static int xram_load(struct SLOT_RUN_STATE*ss, ISTREAM*in)
{
	struct XRAM_STATE*st = ss->data;
	READ_FIELD(in, st->ram_size);
	READ_ARRAY(in, st->ram9_mapping);
	READ_ARRAY(in, st->ram9_enabled);
	READ_FIELD(in, st->psrom9_mode);
	READ_FIELD(in, st->psrom9_ofs);
	st->ram = realloc(st->ram, st->ram_size);
	isread(in, st->ram, st->ram_size);
/*	{
		int i;
		for (i = 0; i < 8; ++i) {
			printf("xram9_load: %i: %i,%i\n", i, st->ram9_mapping[i], st->ram9_enabled[i]);
		}
	}*/
	return 0;
}


static void xram_restore_segment(struct XRAM_STATE*st, int ind)
{
	printf("xram9 restore segment: %i\n", ind);
	if (ind<6) {
		fill_read_proc(st->sr->base_mem+ind*4, 4, xram9_read, st);
		fill_write_proc(st->sr->base_mem+ind*4, 4, xram9_write, st);
	} else {
		switch (ind) {
		case 6: // d000
			if (st->psrom9_mode&1) { // write access
				fill_write_proc(st->sr->base_mem+26,2,xram9_write_psrom, st);
			} else {
				fill_write_proc(st->sr->base_mem+26,2,empty_write, st);
			}
			if ((st->psrom9_mode&3)!=1) { // read_access
				fill_read_proc(st->sr->base_mem+26,2,xram9_read_psrom, st);
			} else {
				fill_read_proc(st->sr->base_mem+26,2,empty_read_addr, st);
				system_command(st->sr, SYS_COMMAND_PSROM_RELEASE, st->nslot, 6);
			}
			break;
		case 7: // e000
			if (st->psrom9_mode&1) { // write access
				fill_write_proc(st->sr->base_mem+28,4,xram9_write, st);
			} else {
				fill_write_proc(st->sr->base_mem+28,4,empty_write, st);
			}
			if ((st->psrom9_mode&3)!=1) { // read_access
				fill_read_proc(st->sr->base_mem+28,4,xram9_read, st);
			} else {
				fill_read_proc(st->sr->base_mem+28,3,empty_read_addr, st);
				system_command(st->sr, SYS_COMMAND_PSROM_RELEASE, st->nslot, 7);
			}
			break;
		}
	}
}



static int xram_command(struct SLOT_RUN_STATE*ss, int cmd, int data, long param)
{
	struct XRAM_STATE*st = ss->data;
//	printf("xram_command(%i,%i)\n", st->nslot, cmd);
	switch (cmd) {
	case SYS_COMMAND_HRESET:
		clear_block(st->ram, st->ram_size);
		memset(st->ram9_enabled, 0, sizeof(st->ram9_enabled));
		memcpy(st->ram9_mapping, startup_ram9_mapping, sizeof(st->ram9_mapping));
		xram9_write_psrom_mode(1, 0, st);
		break;
	case SYS_COMMAND_BASEMEM9_RESTORE:
//		printf("xram_restore_segment(%i,%i)\n", st->nslot, param);
		if (data > st->nslot && st->ram9_enabled[param]) {
//			printf("*** xram_restore_segment(%i,%i)\n", st->nslot, param);
			xram_restore_segment(st, param);
			return 1;
		}
		break;
	case SYS_COMMAND_APPLEMODE:
		st->psrom9_mode = 1;
		st->psrom9_ofs=-RAM9_BANK_SIZE/2;
		if (data) {
			st->last_write = 0;
			memzero(st->ram9_enabled, sizeof(st->ram9_enabled));
			fill_rw_proc(ss->io_sel, 1, empty_read_addr, xram9_write_mapping, st);
			fill_rw_proc(ss->baseio_sel, 1, xram9_apple_read_psrom_mode, xram9_apple_write_psrom_mode, st);
//			system_command(st->sr, SYS_COMMAND_BASEMEM9_RESTORE, st->nslot, 6);
//			system_command(st->sr, SYS_COMMAND_BASEMEM9_RESTORE, st->nslot, 7);
		} else {
			memzero(st->ram9_enabled, sizeof(st->ram9_enabled));
			fill_rw_proc(ss->io_sel, 1, xram9_read_mapping, xram9_write_mapping, st);
			fill_rw_proc(ss->baseio_sel, 1, xram9_read_psrom_mode, xram9_write_psrom_mode, st);
		}
	}
	return 0;
}


int xram9_init(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*ss, struct SLOTCONFIG*sc)
{
	struct XRAM_STATE*st;

	st = calloc(1, sizeof(*st));
	if (!st) return -1;

	st->sr = sr;
	st->nslot = sc->slot_no;
//	printf("slot_no = %i\n", sc->slot_no);

	st->ram_size = memsizes_b[sc->cfgint[CFG_INT_MEM_SIZE]];
	st->ram = malloc(st->ram_size);
	if (!st->ram) {
		free(st);
		return -1;
	}
	clear_block(st->ram, st->ram_size);

	ss->data = st;
	ss->command = xram_command;
	ss->free = xram_free;
	ss->load = xram_load;
	ss->save = xram_save;

	memcpy(st->ram9_mapping, startup_ram9_mapping, sizeof(st->ram9_mapping));
	xram9_write_psrom_mode(1, 0, st);

	fill_rw_proc(ss->io_sel, 1, xram9_read_mapping, xram9_write_mapping, st);
	fill_rw_proc(ss->baseio_sel, 1, xram9_read_psrom_mode, xram9_write_psrom_mode, st);

	return 0;
}

static byte xram9_read_mapping(word adr,struct XRAM_STATE*st)
{
	int ind=(adr&0x70)>>4;
	return (adr&0x70)|st->ram9_mapping[ind];
}

static void xram9_write_mapping(word adr,byte d,struct XRAM_STATE*st)
{
	int ind=(adr&0x70)>>4;
	st->ram9_mapping[ind]=adr&0x8F;
//	printf("%x: setting xram mapping %i to %i\n",adr,ind,adr&0x0F);
	if (adr&0x80) {
		st->ram9_enabled[ind] = 1;
		xram_restore_segment(st, ind);
	} else {
		st->ram9_enabled[ind] = 0;
		system_command(st->sr, SYS_COMMAND_BASEMEM9_RESTORE, st->nslot, ind);
	}
}


static byte get_state(struct XRAM_STATE*st)
{
	byte res = 0xC0;
	int wr = st->psrom9_mode & 1;
	int rd = (st->ram9_mapping[6] && (st->psrom9_mode&3)!=1) ? 1: 0;
	if (!st->psrom9_ofs) res |= 8;
	if (wr) res |= 1;
	if (rd == wr) res |= 2;
	return res;
}

static byte xram9_select_apple_psrom_mode(word adr,struct XRAM_STATE*st)
{
	int hi = adr & 8;
	int sb = adr & 4;
	int mde = adr & 3;
	byte last_mode = st->psrom9_mode;
	byte last_state = get_state(st);
	printf("xram9 apple read: %x\n", adr);
	if (hi) st->psrom9_ofs=0;
	else st->psrom9_ofs=-RAM9_BANK_SIZE/2;
	if (sb) {
		int bank = adr & 3;
		if (adr & 8) bank |= 4;
		printf("select bank %i\n", bank);
		bank <<= 1;
		st->ram9_mapping[6] = bank;
		st->ram9_mapping[7] = bank + 1;
	} else {
		switch (mde) {
		case 0:
			st->ram9_enabled[6] = st->ram9_enabled[7] = 1;
			st->psrom9_mode = 2 | hi | (adr & 0xF0);
			st->last_write = 0;
			break;
		case 1:
			st->ram9_enabled[6] = st->ram9_enabled[7] = 0;
			st->psrom9_mode = st->last_write | hi | (adr & 0xF0);
			st->last_write = 1;
			break;
		case 2:
			st->ram9_enabled[6] = st->ram9_enabled[7] = 0;
			st->psrom9_mode = 0 | hi | (adr & 0xF0);
			st->last_write = 0;
			break;
		case 3: {
			st->ram9_enabled[6] = st->ram9_enabled[7] = 1;
			st->psrom9_mode = 2 | st->last_write | hi | (adr & 0xF0);
			st->last_write = 1;
			break; }
		}
	}
	if (st->ram9_enabled[6]) {
		xram_restore_segment(st, 6);
	} else {
		system_command(st->sr, SYS_COMMAND_BASEMEM9_RESTORE, st->nslot, 6);
	}
	if (st->ram9_enabled[7]) {
		xram_restore_segment(st, 7);
	} else {
		system_command(st->sr, SYS_COMMAND_BASEMEM9_RESTORE, st->nslot, 7);
	}
	return last_state;
}

static byte xram9_apple_read_psrom_mode(word adr,struct XRAM_STATE*st)
{
	return xram9_select_apple_psrom_mode(adr, st);
}

static void xram9_apple_write_psrom_mode(word adr,byte d,struct XRAM_STATE*st)
{
	xram9_select_apple_psrom_mode(adr, st);
}


static byte xram9_read_psrom_mode(word adr,struct XRAM_STATE*st)
{
	byte res = st->psrom9_mode | (adr & 0xF0);
	if (!(res & 3)) res |= 2;
	return res;
}

static void xram9_write_psrom_mode(word adr,byte d,struct XRAM_STATE*st)
{
	st->psrom9_mode=adr&0x0B;
//	printf("setting xrom mode to %x\n",psrom_mode);
	if (st->psrom9_mode&8) st->psrom9_ofs=0;
	else st->psrom9_ofs=-RAM9_BANK_SIZE/2;

	if (st->ram9_enabled[6]) xram_restore_segment(st, 6);
	if (st->ram9_enabled[7]) xram_restore_segment(st, 7);
}

static dword get_addr(word adr, struct XRAM_STATE*st)
{
	int bank=adr>>RAM9_BANK_SHIFT;
	return ((st->ram9_mapping[bank]&0x0F)<<RAM9_BANK_SHIFT)|(adr&(RAM9_BANK_SIZE-1));
}


static byte xram9_read(word adr, struct XRAM_STATE*st)
{
	return st->ram[get_addr(adr, st)];
}

static void xram9_write(word adr, byte d, struct XRAM_STATE*st)
{
	st->ram[get_addr(adr, st)] = d;
}


static byte xram9_read_psrom(word adr,struct XRAM_STATE*st)
{
	return xram9_read((word)(adr+st->psrom9_ofs), st);
}

static void xram9_write_psrom(word adr,byte d,struct XRAM_STATE*st)
{
	xram9_write((word)(adr+st->psrom9_ofs), d, st);
}

