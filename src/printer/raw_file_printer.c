/*
	A printer device that writes all received bytes to a file unchanged.
	Author: Sergey "SnakE" Gromov, snake.scaly@gmail.com
*/

#include <direct.h>
#include <errno.h>
#include <io.h>
#include <stdio.h>
#include <windows.h>

#include "printer_cable.h"
#include "printer_emu.h"
#include "prnprogressdlg_interop.h"
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
	struct PRNPROGRESSDLG_INTEROP progress_interop;
	struct PRNPROGRESSDLG_INFO progress_info;
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

static int flush(struct PRINTER_EMU*emu);

static struct PRNPROGRESSDLG_INTEROP_CB prnprogressdlg_interop_cb =
{
	.finish = flush,
};

static void open_out(struct RAW_FILE_PRINTER*rfp)
{
	if (prnprogressdlg_create_sync(&rfp->progress_interop,
		rfp->wnd, 0, &prnprogressdlg_interop_cb, rfp)) goto fail;
	memset(&rfp->progress_info, 0, sizeof(rfp->progress_info));

	char name[MAX_PATH];
	if (!get_name(rfp->wnd, name)) return;
	rfp->out = fopen(name, "wb");
	if (!rfp->out) goto fail;
	return;

fail:
	puts(__FUNCTION__ " failed");
}

static void close_out(struct RAW_FILE_PRINTER*rfp)
{
	if (rfp->out) {
		fclose(rfp->out);
		rfp->out = NULL;
	}
	prnprogressdlg_destroy_async(&rfp->progress_interop);
}

static int reset(struct PRINTER_EMU*emu)
{
	return 0;
}

static int flush(struct PRINTER_EMU*emu)
{
	struct RAW_FILE_PRINTER*rfp = (struct RAW_FILE_PRINTER*)emu;
	close_out(rfp);
	rfp->ignore_data = 0;
	return 0;
}

static int consume_byte(struct PRINTER_EMU*emu, int data)
{
	struct RAW_FILE_PRINTER*rfp = (struct RAW_FILE_PRINTER*)emu;

	if (rfp->ignore_data) return 0;
	if (!rfp->out) open_out(rfp);
	if (!rfp->out) {
		rfp->ignore_data = 1;
		return 0;
	}

	if (fputc(data, rfp->out) == EOF) return errno;

	rfp->progress_info.printed_total++;
	prnprogressdlg_update_async(&rfp->progress_interop, &rfp->progress_info);
	return 0;
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
	prnprogressdlg_interop_uninit(&rfp->progress_interop);
	free(rfp);
	return 0;
}

static const struct PRINTER_EMU_OPERATIONS ops =
{
	consume_byte,
	reset,
	is_printing,
	slot_command,
	flush,
	free_h,
};

struct PRINTER_CABLE* raw_file_printer_create(HWND wnd)
{
	int err = 0;

	struct RAW_FILE_PRINTER*rfp;

	rfp = calloc(1, sizeof *rfp);
	if (!rfp) goto fail;

	rfp->wnd = wnd;
	prnprogressdlg_interop_init(&rfp->progress_interop);

	err = printer_emu_init(&rfp->emu, &ops);
	if (err) goto fail_rfp;

	return (struct PRINTER_CABLE*)rfp;

fail_rfp:
	free(rfp);
fail:
	puts(__FUNCTION__ " failed");
	errno = err;
	return NULL;
}
