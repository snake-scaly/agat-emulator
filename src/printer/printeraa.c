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

#include "printer_cable.h"
#include "printer_device.h"

struct PRINTER_STATE
{
	struct SLOT_RUN_STATE*st;

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


static int printer_command(struct SLOT_RUN_STATE*st, int cmd, int data, long param)
{
	struct PRINTER_STATE*pcs = st->data;

	if (pcs->pcab) pcs->pcab->ops->slot_command(pcs->pcab, cmd, param);

	return 0;
}


static void printer_data(struct PRINTER_STATE*pcs, byte data)
{
	if (pcs->pcab) {
//	printf("write printer data: %02x (%c)\n", data, data);
		pcs->pcab->ops->write_data(pcs->pcab, data & 0x7F);
		pcs->pcab->ops->write_control(pcs->pcab, 0x40);
		pcs->pcab->ops->write_control(pcs->pcab, 0xC0);
	}
}

static void printer_io_w(word adr, byte data, struct PRINTER_STATE*pcs)
{
	printer_data(pcs, data);
}


int  printeraa_init(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*st, struct SLOTCONFIG*cf)
{
	ISTREAM*rom;
	struct PRINTER_STATE*pcs;
	int mode = cf->cfgint[CFG_INT_PRINT_MODE];

	puts("in printeraa_init");

	pcs = calloc(1, sizeof(*pcs));
	if (!pcs) return -1;

	pcs->st = st;

	pcs->pcab = printer_device_for_mode(sr, st, mode);
	if (!pcs->pcab) {
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
