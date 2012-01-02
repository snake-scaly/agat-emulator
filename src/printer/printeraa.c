/*
	Agat Emulator version 1.4
	Copyright (c) NOP, nnop@newmail.ru
	printeraa - emulation of Atom's printer interface
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
#include "export.h"

struct PRINTER_STATE
{
	struct SLOT_RUN_STATE*st;

	PEPSON_EMU pemu;
};

static int printer_term(struct SLOT_RUN_STATE*st)
{
	struct PRINTER_STATE*pcs = st->data;
	epson_free(pcs->pemu);
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
			(epson_hasdata(pcs->pemu)?MF_ENABLED:MF_GRAYED)|MF_BYCOMMAND);
	}
}

static void free_menu(struct PRINTER_STATE*pcs, int s, HMENU menu)
{
	DeleteMenu(menu, PRN_BASE_CMD + s * 10, MF_BYCOMMAND);
}


static void wincmd(HWND wnd, int cmd, int s, struct PRINTER_STATE*pcs)
{
	if (pcs->pemu && cmd == PRN_BASE_CMD + s * 10) {
		epson_flush(pcs->pemu);
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
		if (pcs->pemu) epson_flush(pcs->pemu);
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


static void printer_data(struct PRINTER_STATE*pcs, byte data)
{
//	printf("write printer data: %02x (%c)\n", data, data);
	epson_write(pcs->pemu, data & 0x7F);
}

static void printer_io_w(word adr, byte data, struct PRINTER_STATE*pcs)
{
	printer_data(pcs, data);
}


int  printeraa_init(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*st, struct SLOTCONFIG*cf)
{
	int i;
	ISTREAM*rom;
	struct PRINTER_STATE*pcs;
	struct EPSON_EXPORT exp;
	int mode = cf->cfgint[CFG_INT_PRINT_MODE];
	unsigned fl = 0;

	puts("in printeraa_init");

	pcs = calloc(1, sizeof(*pcs));
	if (!pcs) return -1;

	pcs->st = st;

	switch (mode) {
	case 0:
		i = export_raw_init(&exp, 0, sr->video_w);
		break;
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
	if (!pcs->pemu) {
		exp.free_data(exp.param);
		free(pcs);
		return -3;
	}


	fill_rw_proc(st->service_procs, 1, empty_read, printer_io_w, pcs);

	st->data = pcs;
	st->free = printer_term;
	st->command = printer_command;
	st->load = printer_load;
	st->save = printer_save;

	return 0;
}
