/*
	Agat Emulator version 1.0
	Copyright (c) NOP, nnop@newmail.ru
	baseram - emulation of base ROM module for both models
*/

#include "sysconf.h"
#include "types.h"
#include "streams.h"
#include "debug.h"

#include "memory.h"
#include "runmgrint.h"

#define STDROM_SIZE 0x800
#define APPLEROM_SIZE 0x3000

struct BASEROM_STATE
{
	word rom_size, rom_mask, rom_ofs;
	byte *rom;
};



static byte rom_read(word adr,struct BASEROM_STATE*st);
static byte rom1_read(word adr,struct BASEROM_STATE*st);


static void rom_reset_procs(struct SLOT_RUN_STATE*ss)
{
	struct BASEROM_STATE*st = ss->data;
	if (ss->sr->cursystype != SYSTEM_1) {
		int sz = st->rom_size;
		int nb;
		if (sz > 0x3000) sz = 0x3000;
		nb = sz>>BASEMEM_BLOCK_SHIFT;
//		printf("rom_size = %i (%x); nb = %i\n", sz, sz, nb);
		fill_read_proc(ss->sr->base_mem + (0xD000>>BASEMEM_BLOCK_SHIFT), (0x3000 - sz) >> BASEMEM_BLOCK_SHIFT, empty_read_addr, NULL);
		fill_read_proc(ss->sr->base_mem+BASEMEM_NBLOCKS-nb, nb, rom_read, st);
	} else {
		fill_read_proc(ss->sr->base_mem+BASEMEM_NBLOCKS-1, 1, rom1_read, st);
	}
}


static int rom_command(struct SLOT_RUN_STATE*ss, int cmd, int data, long param)
{
	struct BASEROM_STATE*st = ss->data;
	switch (cmd) {
	case SYS_COMMAND_HRESET:
		rom_reset_procs(ss);
		break;
	case SYS_COMMAND_PSROM_RELEASE:
//		puts("baserom: restore rom");
		rom_reset_procs(ss);
		return 1;
	}
	return 0;
}

static int rom_free(struct SLOT_RUN_STATE*ss)
{
	struct BASEROM_STATE*st = ss->data;
	free(st->rom);
	free(st);
	return 0;
}

int rom_install(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*ss, struct SLOTCONFIG*sc)
{
	struct BASEROM_STATE*st;
	ISTREAM*in;

	st = malloc(sizeof(*st));
	if (!st) return -1;

	st->rom_size = sc->cfgint[CFG_INT_ROM_SIZE];
	st->rom_mask = sc->cfgint[CFG_INT_ROM_MASK];
	st->rom_ofs = sc->cfgint[CFG_INT_ROM_OFS];
	st->rom = malloc(st->rom_size);
	if (!st->rom) {
		free(st);
		return -1;
	}

	printf("rom size: %X (%i bytes), rom file: %s\n", st->rom_size, st->rom_size, sc->cfgstr[CFG_STR_ROM]);
	in=isfopen(sc->cfgstr[CFG_STR_ROM]);
	if (!in) {
		int r;
		r = load_buf_res(sc->cfgint[CFG_INT_ROM_RES], st->rom, st->rom_size);
		if (r < 0) {
			free(st->rom);
			free(st);
			return -2;
		}
	} else {
		if (isread(in,st->rom,st->rom_size)!=st->rom_size) {
			errprint(TEXT("can't read rom file %s"),sc->cfgstr[CFG_STR_ROM]);
			isclose(in);
			free(st->rom);
			free(st);
			return -3;
		}
		isclose(in);
	}

	ss->data = st;
	ss->free = rom_free;
	ss->command = rom_command;

	if (sr->cursystype == SYSTEM_9) { // to conform to the real Agat-9
		st->rom_size <<= 1;
	}

	rom_reset_procs(ss);
	return 0;
}


static byte rom_read(word adr,struct BASEROM_STATE*st)
{
//	printf("rom_read from addr %x(%x): %x\n", adr, (adr & st->rom_mask) - st->rom_ofs, st->rom[(adr & st->rom_mask) - st->rom_ofs]);
	return st->rom[(adr & st->rom_mask) - st->rom_ofs];
}


static byte rom1_read(word adr,struct BASEROM_STATE*st)
{
//	printf("rom1_read: %X\n", adr);
	if (adr >= 0xFF00) return st->rom[(adr & st->rom_mask) - st->rom_ofs];
	else return empty_read(adr, st);
}

byte system_read_rom(word adr,struct SYS_RUN_STATE*sr)
{
	return rom_read(adr, sr->slots[CONF_ROM].data);
}

