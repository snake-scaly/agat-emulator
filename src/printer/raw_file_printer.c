/*
	A printer device that writes all received bytes to a file unchanged.
	Author: Sergey "SnakE" Gromov, snake.scaly@gmail.com
*/

#include <stdio.h>
#include <windows.h>
#include <errno.h>

#include "printer_cable.h"
#include "printer_emu.h"
#include "raw_file_printer.h"
#include "sysconf.h"

#define RESET_BIT 0x40
#define DATA_BIT  0x80
#define BUSY_BIT 0x80
#define BYTE_MASK 0xFF

struct RAW_FILE_PRINTER
{
	struct PRINTER_EMU emu;
	FILE*out;
	HWND wnd;
	int ignore_data;
};

static int get_name(HWND wnd, char*fname)
{
	int no;
	_mkdir(PRNOUT_DIR);
	for (no = 1; no; ++no) {
		sprintf(fname, PRNOUT_DIR"\\spool%03i.bin", no);
		if (_access(fname, 0)) break;
	}
	if (!select_save_bin(wnd, fname)) {
		return 0;
	}
	return 1;
}

static void open_out(struct RAW_FILE_PRINTER*rfp)
{
	char name[MAX_PATH];
	if (!get_name(rfp->wnd, name)) return;
	rfp->out = fopen(name, "wb");
	if (!rfp->out) return;
}

static void close_out(struct RAW_FILE_PRINTER*rfp)
{
	if (rfp->out) {
		fclose(rfp->out);
		rfp->out = NULL;
	}
}

static int reset(struct PRINTER_EMU*emu)
{
	struct RAW_FILE_PRINTER*rfp = (struct RAW_FILE_PRINTER*)emu;
	close_out(rfp);
	rfp->ignore_data = 0;
	return 0;
}

static int consume_byte(struct PRINTER_EMU*emu, int data)
{
	struct RAW_FILE_PRINTER*rfp = (struct RAW_FILE_PRINTER*)emu;
	int err = 0;

	if (rfp->ignore_data) return 0;
	if (!rfp->out) open_out(rfp);
	if (!rfp->out) {
		rfp->ignore_data = 1;
		return 0;
	}

	if (fputc(data, rfp->out) == EOF) err = errno;

	return err;
}

static int is_printing(struct PRINTER_EMU*emu)
{
	struct RAW_FILE_PRINTER*rfp = (struct RAW_FILE_PRINTER*)emu;
	return rfp->out != NULL;
}

static int slot_command(struct PRINTER_EMU*emu, int id, long param)
{
	return 0;
}

static int free_h(struct PRINTER_EMU*emu)
{
	struct RAW_FILE_PRINTER*rfp = (struct RAW_FILE_PRINTER*)emu;
	close_out(rfp);
	free(rfp);
	return 0;
}

static const struct PRINTER_EMU_OPERATIONS ops =
{
	consume_byte,
	is_printing,
	slot_command,
	reset,
	free_h,
};

struct PRINTER_CABLE* raw_file_printer_create(HWND wnd)
{
	int err;

	struct RAW_FILE_PRINTER*rfp;

	rfp = calloc(1, sizeof *rfp);
	if (!rfp) return NULL;

	rfp->wnd = wnd;

	err = printer_emu_init(&rfp->emu, &ops);
	if (err) {
		free(rfp);
		rfp = NULL;
		errno = err;
	}

	return (struct PRINTER_CABLE*)rfp;
}
