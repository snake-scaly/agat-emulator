/*
	Agat Emulator version 1.4
	Copyright (c) NOP, nnop@newmail.ru
	printera - emulation of Apple's printer card
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
#include "epson_emu.h"
#include "export.h"
#include "raw_file_printer.h"

struct PRINTER_STATE
{
	struct SLOT_RUN_STATE*st;

	byte rom1[256];
	PPRINTER_CABLE pemu;
};

static int printer_term(struct SLOT_RUN_STATE*st)
{
	struct PRINTER_STATE*pcs = st->data;
	printer_cable_free(pcs->pemu);
	free(st->data);
	return 0;
}

static int printer_save(struct SLOT_RUN_STATE*st, OSTREAM*out)
{
	struct PRINTER_STATE*pcs = st->data;

	return 0;
}

static int printer_load(struct SLOT_RUN_STATE*st, ISTREAM*in)
{
	struct PRINTER_STATE*pcs = st->data;

	return 0;
}

#define PRN_BASE_CMD 9000


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
		return 0;
	case SYS_COMMAND_HRESET:
		if (pcs->pemu) printer_cable_reset(pcs->pemu);
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


static byte printer_rom_r(word adr, struct PRINTER_STATE*pcs) // CX00-CXFF
{
	return pcs->rom1[adr & 0xFF];
}

static void printer_data(struct PRINTER_STATE*pcs, byte data)
{
//	printf("write printer data: %02x (%c)\n", data, data);
	printer_cable_write_data(pcs->pemu, data & 0x7F);
	printer_cable_write_control(pcs->pemu, 0x40);
	printer_cable_write_control(pcs->pemu, 0xC0);
}

static void printer_io_w(word adr, byte data, struct PRINTER_STATE*pcs) // C0X0-C0XF
{
	adr &= 0x0F;
	switch (adr) {
	case 0:
		printer_data(pcs, data);
		break;
	}
}

int  printera_init(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*st, struct SLOTCONFIG*cf)
{
	int i;
	ISTREAM*rom;
	struct PRINTER_STATE*pcs;
	struct EPSON_EXPORT exp = {0};
	int mode = cf->cfgint[CFG_INT_PRINT_MODE];
	unsigned fl = 0;

	puts("in printera_init");

	pcs = calloc(1, sizeof(*pcs));
	if (!pcs) return -1;

	pcs->st = st;

	rom = isfopen(cf->cfgstr[CFG_STR_ROM]);
	if (!rom) { free(pcs); return -1; }
	isread(rom, pcs->rom1, sizeof(pcs->rom1));
	isclose(rom);

	if (mode == 0) {
		pcs->pemu = raw_file_printer_create(sr->video_w);
	} else {
		switch (mode) {
		case 1:
			i = export_text_init(&exp, 0, sr->video_w);
			fl = EPSON_TEXT_RECODE_FX;
			break;
		case 2:
			i = export_tiff_init(&exp, EXPORT_TIFF_COMPRESS_RLE, sr->video_w);
			fl = EPSON_TEXT_RECODE_FX;
			break;
		case 3:
			i = export_print_init(&exp, 0, sr->video_w);
			fl = EPSON_TEXT_RECODE_FX;
			break;
		default:
			i = -1;
			break;
		}

		if (i < 0)  {
			free(pcs);
			return -2;
		}

		pcs->pemu = epson_create(fl, &exp);
	}
	if (!pcs->pemu) {
		if (exp.free_data) exp.free_data(exp.param);
		free(pcs);
		return -3;
	}



	st->data = pcs;
	st->free = printer_term;
	st->command = printer_command;
	st->load = printer_load;
	st->save = printer_save;

	fill_rw_proc(st->io_sel, 1, printer_rom_r, empty_write, pcs);
	fill_rw_proc(st->baseio_sel, 1, empty_read, printer_io_w, pcs);

	return 0;
}
