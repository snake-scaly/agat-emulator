/*
	Agat Emulator version 1.0
	Copyright (c) NOP, nnop@newmail.ru
	xram - extended memory module emulation for AGAT-7
*/
#include "sysconf.h"
#include "types.h"
#include "streams.h"
#include "debug.h"

#include "memory.h"
#include "runmgrint.h"

#define XRAM_BLOCK_SIZE 0x4000
#define XRAM_ADDR_MASK (XRAM_BLOCK_SIZE - 1)
#define XRAM_BLOCK_SHIFT 0x0E


struct XRAM_STATE
{
	int nslot;
	struct SYS_RUN_STATE*sr;
	dword ram_size;
	byte  ram_state;
	byte *ram;
};


static byte xram_read(word adr,struct XRAM_STATE*st);
static void xram_write(word adr,byte d, struct XRAM_STATE*st);

static byte xram_read_state(word adr,struct XRAM_STATE*st);
static void xram_write_state(word adr,byte d, struct XRAM_STATE*st);

static int xram_free(struct SLOT_RUN_STATE*ss)
{
	struct XRAM_STATE*st = ss->data;
	free(st->ram);
	free(st);
	return 0;
}

static void set_xram_procs(struct XRAM_STATE*st)
{
	if (st->ram_state & 0x10) { // write protected
		fill_rw_proc(st->sr->base_mem + (0x8000>>BASEMEM_BLOCK_SHIFT), 0x4000>>BASEMEM_BLOCK_SHIFT, xram_read, empty_write, st);
	} else {
		fill_rw_proc(st->sr->base_mem + (0x8000>>BASEMEM_BLOCK_SHIFT), 0x4000>>BASEMEM_BLOCK_SHIFT, xram_read, xram_write, st);
	}
}


static int xram_save(struct SLOT_RUN_STATE*ss, OSTREAM*out)
{
	struct XRAM_STATE*st = ss->data;
	WRITE_FIELD(out, st->ram_size);
	WRITE_FIELD(out, st->ram_state);
	oswrite(out, st->ram, st->ram_size);
	return 0;
}

static int xram_load(struct SLOT_RUN_STATE*ss, ISTREAM*in)
{
	struct XRAM_STATE*st = ss->data;
	READ_FIELD(in, st->ram_size);
	READ_FIELD(in, st->ram_state);
	st->ram = realloc(st->ram, st->ram_size);
	isread(in, st->ram, st->ram_size);
	set_xram_procs(st);
	return 0;
}


static int xram_command(struct SLOT_RUN_STATE*ss, int cmd, int data, long param)
{
	struct XRAM_STATE*st = ss->data;
	switch (cmd) {
	case SYS_COMMAND_HRESET:
		st->ram_state = 0;
		clear_block(st->ram, st->ram_size);
		break;
	case SYS_COMMAND_XRAM_RELEASE:
		if (data > st->nslot && (st->ram_state & 0x08)) {
//			puts("xram: restore ram");
			set_xram_procs(st);
			return 1;
		}
		break;
	}
	return 0;
}



int xram7_init(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*ss, struct SLOTCONFIG*sc)
{
	struct XRAM_STATE*st;

	st = calloc(1, sizeof(*st));
	if (!st) return -1;

	st->sr = sr;
	st->nslot = sc->slot_no;
	st->ram_size = memsizes_b[sc->cfgint[CFG_INT_MEM_SIZE]];
	st->ram_state = 0;
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

	fill_rw_proc(ss->io_sel, 1, xram_read_state, xram_write_state, st);

	return 0;
}


static __inline dword xram7_offset(struct XRAM_STATE*st, word adr)
{
	return (((st->ram_state & 0x07) << XRAM_BLOCK_SHIFT) + (adr & XRAM_ADDR_MASK)) & (st->ram_size - 1);
}

static byte xram_read(word adr,struct XRAM_STATE*st)
{
	return st->ram[xram7_offset(st, adr)];
}

static void xram_write(word adr, byte d, struct XRAM_STATE*st)
{
	st->ram[xram7_offset(st, adr)] = d;
}

static byte xram_read_state(word adr,struct XRAM_STATE*st)
{
	printf("xram_read_state: %x\n", st->ram_state & 0x7F);
	return st->ram_state & 0x7F;
}

static void xram_write_state(word adr, byte d, struct XRAM_STATE*st)
{
	d = adr & 0x7F;
	if (st->ram_state == d) return;
	printf("xram_write_state: %x\n", d);
	st->ram_state = d;
	if (st->ram_state & 0x08) { // module selected
		set_xram_procs(st);
	} else {
		system_command(st->sr, SYS_COMMAND_XRAM_RELEASE, st->nslot, (0x8000>>BASEMEM_BLOCK_SHIFT));
	}
}

