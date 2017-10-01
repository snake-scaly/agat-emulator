#include "printer_cable.h"
#include "epson_emu.h"
#include "export.h"
#include "prnprogressdlg_interop.h"
#include "sysconf.h"
#include <direct.h>
#include <io.h>
#include <stdio.h>



#define BUF_SIZE 256

struct EXPORT_TEXT
{
	FILE*out;
	HWND wnd;
	unsigned flags;

	unsigned char buf[BUF_SIZE];
	int buf_pos;

	int  opened;

	struct PRNPROGRESSDLG_INTEROP progress_interop;
	struct PRNPROGRESSDLG_INFO progress_info;
};


static int get_name(HWND wnd, char*fname)
{
	int no;
	_mkdir(PRNOUT_DIR);
	for (no = 1; no; ++no) {
		sprintf(fname, PRNOUT_DIR"\\spool%03i.txt", no);
		if (_access(fname, 0)) break;
	}	
	if (!select_save_text(wnd, fname)) {
		return 0;
	}
	return 1;
}

static void txt_close(struct EXPORT_TEXT*et);

static struct PRNPROGRESSDLG_INTEROP_CB progress_cb =
{
	NULL, txt_close, NULL
};

static void open_out(struct EXPORT_TEXT*et)
{
	char name[MAX_PATH];

	if (prnprogressdlg_create_sync(&et->progress_interop,
		et->wnd, 0, &progress_cb, et)) goto fail;
	memset(&et->progress_info, 0, sizeof(et->progress_info));
	et->opened = 1;

	if (!get_name(et->wnd, name)) return;
	et->out = fopen(name, "wt");
	if (!et->out) goto fail;
	return;

fail:
	puts(__FUNCTION__ " failed");
}


static void buf_back(struct EXPORT_TEXT*et)
{
	if (et->buf_pos) -- et->buf_pos;
}

static void buf_flush(struct EXPORT_TEXT*et)
{
	fwrite(et->buf, 1, et->buf_pos, et->out);
	et->buf_pos = 0;
}

static void buf_add(struct EXPORT_TEXT*et, int ch)
{
	if (et->buf_pos == BUF_SIZE) {
		fwrite(et->buf, 1, 1, et->out);
		memmove(et->buf, et->buf + 1, BUF_SIZE - 1);
		et->buf[et->buf_pos - 1] = ch;
	} else {
		et->buf[et->buf_pos++] = ch;
	}
}

static void txt_write_char(struct EXPORT_TEXT*et, int ch)
{
	if (ch && !et->opened) {
		open_out(et);
	}
	if (!et->out) return;
	et->progress_info.printed_total++;
	prnprogressdlg_update_async(&et->progress_interop, &et->progress_info);
	switch (ch) {
	case 8:
		buf_back(et);
		break;
	case 10:
	case 13:
		buf_add(et, 10);
		break;
	}
	if (ch < ' ') return;
	buf_add(et, ch);
}

static void txt_write_command(struct EXPORT_TEXT*et, int cmd, int nparams, unsigned char*params)
{
	buf_flush(et);
	switch (cmd) {
	case 'L': case 'K': case 'Y': case 'Z': case '*':
		if (et->out) {
			int nchars = nparams / 12;
			for (;nchars;--nchars) fputc(' ', et->out);
		}
		break;
	}
}

static void txt_close(struct EXPORT_TEXT*et)
{
	if (!et->opened) return;
	et->opened = 0;
	if (et->out) {
		buf_flush(et);
		fclose(et->out);
	}
	prnprogressdlg_destroy_async(&et->progress_interop);
}

static void txt_free_data(struct EXPORT_TEXT*et)
{
	if (et) {
		txt_close(et);
		prnprogressdlg_interop_uninit(&et->progress_interop);
		free(et);
	}
}

static int txt_opened(struct EXPORT_TEXT*et)
{
	return et->opened;
}



int  export_text_init(struct EPSON_EXPORT*exp, unsigned flags, HWND wnd)
{
	struct EXPORT_TEXT*et;

	et = calloc(sizeof(*et), 1);
	if (!et) return -1;

	et->flags = flags;
	et->wnd = wnd;

	exp->param = et;
	exp->write_char = txt_write_char;
	exp->write_command = txt_write_command;
	exp->close = txt_close;
	exp->free_data = txt_free_data;
	exp->opened = txt_opened;

	prnprogressdlg_interop_init(&et->progress_interop);
	return 0;
}

