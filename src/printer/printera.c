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
#include "printer_device.h"

struct PRINTER_STATE
{
	struct SLOT_RUN_STATE*st;

	byte rom1[256];
	struct PRINTER_CABLE*pcab;
};

static int printer_term(struct SLOT_RUN_STATE*st)
{
	struct PRINTER_STATE*pcs = st->data;
	if (pcs->pcab) pcs->pcab->ops->free(pcs->pcab);
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
	if (pcs->pcab) {
		int printing = pcs->pcab->ops->is_printing(pcs->pcab);
		EnableMenuItem(menu, PRN_BASE_CMD + s * 10,
			(printing?MF_ENABLED:MF_GRAYED)|MF_BYCOMMAND);
	}
}

static void free_menu(struct PRINTER_STATE*pcs, int s, HMENU menu)
{
	DeleteMenu(menu, PRN_BASE_CMD + s * 10, MF_BYCOMMAND);
}


static void wincmd(HWND wnd, int cmd, int s, struct PRINTER_STATE*pcs)
{
	if (pcs->pcab && cmd == PRN_BASE_CMD + s * 10) {
		pcs->pcab->ops->reset(pcs->pcab);
	}
}

static int printer_command(struct SLOT_RUN_STATE*st, int cmd, int data, long param)
{
	struct PRINTER_STATE*pcs = st->data;
	HMENU menu;

	if (pcs->pcab) pcs->pcab->ops->slot_command(pcs->pcab, cmd, param);

	switch (cmd) {
	case SYS_COMMAND_RESET:
		return 0;
	case SYS_COMMAND_HRESET:
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
	if (pcs->pcab) {
		pcs->pcab->ops->write_data(pcs->pcab, data & 0x7F);
		pcs->pcab->ops->write_control(pcs->pcab, 0x40);
		pcs->pcab->ops->write_control(pcs->pcab, 0xC0);
	}
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
	ISTREAM*rom;
	struct PRINTER_STATE*pcs;
	int mode = cf->cfgint[CFG_INT_PRINT_MODE];

	puts("in printera_init");

	pcs = calloc(1, sizeof(*pcs));
	if (!pcs) return -1;

	pcs->st = st;

	rom = isfopen(cf->cfgstr[CFG_STR_ROM]);
	if (!rom) { free(pcs); return -1; }
	isread(rom, pcs->rom1, sizeof(pcs->rom1));
	isclose(rom);

	pcs->pcab = printer_device_for_mode(sr, st, mode);
	if (!pcs->pcab) {
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
