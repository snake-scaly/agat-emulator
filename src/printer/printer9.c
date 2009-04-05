/*
	Agat Emulator version 1.4
	Copyright (c) NOP, nnop@newmail.ru
	thunderclock - emulation of Agat-9's printer card
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

#include "epson_emu.h"

struct PRINTER_STATE
{
	struct SLOT_RUN_STATE*st;

	byte rom1[256], rom2[2048];
	byte rom_mode; // bit 1 -> C0X3, bit 2 -> CX00
	byte regs[3];

	PEPSON_EMU pemu;
};

static int printer_term(struct SLOT_RUN_STATE*st)
{
	struct PRINTER_STATE*pcs = st->data;
	epson_free(pcs->pemu);
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

static int printer_command(struct SLOT_RUN_STATE*st, int cmd, int data, long param)
{
	struct PRINTER_STATE*pcs = st->data;
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
	int ind = (0xC800 >> BASEMEM_BLOCK_SHIFT);
	if (en) {
		fill_rw_proc(pcs->st->sr->base_mem + ind, 
			1, printer_xrom_r, empty_write, pcs);
	} else {
		if (pcs->st->sr->base_mem[ind].read == printer_xrom_r)
			fill_rw_proc(pcs->st->sr->base_mem + ind, 
				1, empty_read_addr, empty_write, pcs);
	}	
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

static void printer_data(struct PRINTER_STATE*pcs, byte data)
{
//	printf("write printer data: %02x (%c)\n", data, data);
	epson_write(pcs->pemu, data);
}

static void printer_control(struct PRINTER_STATE*pcs, byte data)
{
//	printf("printer_control %02X\n", data);
	if ((data ^ pcs->regs[1]) & 0x80) {
		if (data & 0x80) printer_data(pcs, pcs->regs[0]);
	}
}

static void printer_io_w(word adr, byte data, struct PRINTER_STATE*pcs) // C0X0-C0XF
{
	adr &= 0x03;
	switch (adr) {
	case 1:
		printer_control(pcs, data);
	case 0:
//		printf("printer: write reg %x: %02x\n", adr, data);
		pcs->regs[adr] = data;
		break;
	case 3:
		set_rom_mode(pcs, pcs->rom_mode | 1);
		break;
	}
}


static byte printer_io_r(word adr, struct PRINTER_STATE*pcs) // C0X0-C0XF
{
	adr &= 0x03;
	switch (adr) {
	case 2:
//		printf("printer: read reg %x = %02x\n", adr, pcs->regs[adr]);
		pcs->regs[2]^=0x80;
		return pcs->regs[2];
	}
	return empty_read(adr, pcs);
}


int  printer9_init(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*st, struct SLOTCONFIG*cf)
{
	int i;
	ISTREAM*rom;
	struct PRINTER_STATE*pcs;

	puts("in printer9_init");

	pcs = calloc(1, sizeof(*pcs));
	if (!pcs) return -1;

	pcs->st = st;
	pcs->pemu = epson_create(0, sr->video_w);
	pcs->regs[2] = 0x0;

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

	fill_rw_proc(st->io_sel, 1, printer_rom_r, empty_write, pcs);
	fill_rw_proc(st->baseio_sel, 1, printer_io_r, printer_io_w, pcs);

	return 0;
}
