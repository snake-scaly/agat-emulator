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

static void open_out(struct RAW_FILE_PRINTER*emu)
{
	char name[MAX_PATH];
	if (!get_name(emu->wnd, name)) return;
	emu->out = fopen(name, "wb");
	if (!emu->out) return;
}

static void close_out(struct RAW_FILE_PRINTER*emu)
{
	if (emu->out) {
		fclose(emu->out);
		emu->out = NULL;
	}
}

static int reset(struct RAW_FILE_PRINTER*emu)
{
	if (!emu) return EINVAL;
	close_out(emu);
	emu->ignore_data = 0;
	return 0;
}

static int consume_byte(struct RAW_FILE_PRINTER*emu, int data)
{
	int err = 0;

	if (!emu) return EINVAL;
	if (emu->ignore_data) return 0;
	if (!emu->out) open_out(emu);
	if (!emu->out) {
		emu->ignore_data = 1;
		return 0;
	}

	if (fputc(data, emu->out) == EOF) err = errno;

	return err;
}

static int is_printing(struct RAW_FILE_PRINTER*emu)
{
	if (!emu) { errno = EINVAL; return -1; }
	return emu->out != NULL;
}

static int free_h(struct RAW_FILE_PRINTER*emu)
{
	if (!emu) return EINVAL;
	close_out(emu);
	free(emu);
	return 0;
}

PPRINTER_CABLE raw_file_printer_create(HWND wnd)
{
	struct RAW_FILE_PRINTER*emu;
	PPRINTER_CABLE pemu;

	emu = calloc(1, sizeof *emu);
	if (!emu) return NULL;

	emu->wnd = wnd;

	pemu = printer_emu_create(emu, consume_byte, is_printing, reset, free_h);
	if (!pemu) free(emu);

	return pemu;
}
