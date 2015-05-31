/*
	Agat Emulator version 1.4
	Copyright (c) NOP, nnop@newmail.ru
	printer9 - emulation of Agat-9's printer card
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

#include "printer_cable.h"
#include "printer_device.h"

struct PRINTER_STATE
{
	struct SLOT_RUN_STATE*st;

	byte rom[2048];
	byte rom_mode; // bit 1 -> C0X3, bit 2 -> CX00

	PPRINTER_CABLE pemu;
};

static int printer_term(struct SLOT_RUN_STATE*st)
{
	struct PRINTER_STATE*pcs = st->data;
	printer_cable_free(pcs->pemu);
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
	byte regs[3]; /* for compatibility with old saved states */
	struct PRINTER_STATE*pcs = st->data;

	WRITE_FIELD(out, pcs->rom_mode);
	WRITE_ARRAY(out, regs);

	return 0;
}

static int printer_load(struct SLOT_RUN_STATE*st, ISTREAM*in)
{
	byte regs[3]; /* for compatibility with old saved states */
	struct PRINTER_STATE*pcs = st->data;

	READ_FIELD(in, pcs->rom_mode);
	READ_ARRAY(in, regs);

	if ((pcs->rom_mode & 3) == 3) enable_printer_rom(pcs, 1);
	else enable_printer_rom(pcs, 0);

	return 0;
}

#define PRN_BASE_CMD 8000


static void init_menu(struct PRINTER_STATE*pcs, int s, HMENU menu)
{
	TCHAR buf1[1024];
	AppendMenu(menu, MF_STRING, PRN_BASE_CMD + s * 10,
			localize_str(LOC_PRINTER, 100, buf1, sizeof(buf1)));
}

static void update_menu(struct PRINTER_STATE*pcs, int s, HMENU menu)
{
	if (pcs->pemu) {
		EnableMenuItem(menu, PRN_BASE_CMD + s * 10,
			(printer_cable_is_printing(pcs->pemu)?MF_ENABLED:MF_GRAYED)|MF_BYCOMMAND);
	}
}

static void free_menu(struct PRINTER_STATE*pcs, int s, HMENU menu)
{
	DeleteMenu(menu, PRN_BASE_CMD + s * 10, MF_BYCOMMAND);
}


static void wincmd(HWND wnd, int cmd, int s, struct PRINTER_STATE*pcs)
{
	if (pcs->pemu && cmd == PRN_BASE_CMD + s * 10) {
		printer_cable_reset(pcs->pemu);
	}
}

static int printer_command(struct SLOT_RUN_STATE*st, int cmd, int data, long param)
{
	struct PRINTER_STATE*pcs = st->data;
	HMENU menu;
	switch (cmd) {
	case SYS_COMMAND_RESET:
		set_rom_mode(pcs, pcs->rom_mode & ~1);
		return 0;
	case SYS_COMMAND_HRESET:
		if (pcs->pemu) printer_cable_reset(pcs->pemu);
		set_rom_mode(pcs, 0);
		return 0;
	case SYS_COMMAND_INITMENU:
		menu = (HMENU) param;
		init_menu(pcs, st->sc->slot_no, menu);
		return 0;
	case SYS_COMMAND_UPDMENU:
		menu = (HMENU) param;
		update_menu(pcs, st->sc->slot_no, menu);
		break;
	case SYS_COMMAND_FREEMENU:
		menu = (HMENU) param;
		free_menu(pcs, st->sc->slot_no, menu);
		break;
	case SYS_COMMAND_WINCMD:
		wincmd((HWND)param, data, st->sc->slot_no, pcs);
		break;
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
	return pcs->rom[adr & (sizeof(pcs->rom)-1)];
}

static byte printer_rom_r(word adr, struct PRINTER_STATE*pcs) // CX00-CXFF
{
	set_rom_mode(pcs, pcs->rom_mode | 2);
	return pcs->rom[(adr & 0xFF) | 0x700];
}

static void printer_io_w(word adr, byte data, struct PRINTER_STATE*pcs) // C0X0-C0XF
{
	adr &= 0x03;
//	printf("printer: write reg %x = %02x\n", adr, data);
//	system_command(pcs->st->sr, SYS_COMMAND_DUMPCPUREGS, 0, 0);
	switch (adr) {
	case 0:
		printer_cable_write_data(pcs->pemu, data);
		break;
	case 1:
		printer_cable_write_control(pcs->pemu, data);
		break;
	case 3:
		set_rom_mode(pcs, pcs->rom_mode | 1);
		break;
	}
}

static byte printer_io_r(word adr, struct PRINTER_STATE*pcs) // C0X0-C0XF
{
	byte r;
	adr &= 0x03;
//	system_command(pcs->st->sr, SYS_COMMAND_DUMPCPUREGS, 0, 0);
	switch (adr) {
	case 2:
		return printer_cable_read_state(pcs->pemu);
	}
	return empty_read(adr, pcs);
}


#define POLAR_READY	0x40
#define POLAR_BUSY	0x00
#define CHAR_KOI8	0x00
#define CHAR_GOST	0x04
#define CHAR_CPA80	0x20
#define CHAR_FX85	0x24
#define DATA_INVERSE	0x10
#define DATA_NORMAL	0x00
#define READY_ACK	0x08
#define READY_BUSY	0x00
#define PRINTER_FX	0x00
#define PRINTER_D100	0x02



int  printer9_init(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*st, struct SLOTCONFIG*cf)
{
	ISTREAM*rom;
	struct PRINTER_STATE*pcs;
	int mode = cf->cfgint[CFG_INT_PRINT_MODE];

	puts("in printer9_init");

	pcs = calloc(1, sizeof(*pcs));
	if (!pcs) return -1;

	pcs->st = st;

	rom = isfopen(cf->cfgstr[CFG_STR_ROM]);
	if (!rom) { free(pcs); return -1; }
	isread(rom, pcs->rom, sizeof(pcs->rom));
	isclose(rom);

	pcs->pemu = printer_device_for_mode(sr, mode);
	if (!pcs->pemu) {
		free(pcs);
		return -3;
	}



	st->data = pcs;
	st->free = printer_term;
	st->command = printer_command;
	st->load = printer_load;
	st->save = printer_save;

	fill_rw_proc(st->io_sel, 1, printer_rom_r, empty_write, pcs);
	fill_rw_proc(st->baseio_sel, 1, printer_io_r, printer_io_w, pcs);
	fill_rw_proc(&st->xio_sel, 1, printer_xrom_r, empty_write, pcs);

	return 0;
}
