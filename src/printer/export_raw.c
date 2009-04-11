#include "epson_emu.h"
#include "export.h"
#include "sysconf.h"
#include <io.h>
#include <stdio.h>

#define BUF_SIZE 256

struct EXPORT_RAW
{
	FILE*out;
	HWND wnd;
	unsigned flags;

	int  opened;
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

static void open_out(struct EXPORT_RAW*er)
{
	char name[MAX_PATH];
	er->opened = 1;
	if (!get_name(er->wnd, name)) return;
	er->out = fopen(name, "wb");
	if (!er->out) return;
}


static void raw_write_char(struct EXPORT_RAW*er, int ch)
{
	if (ch && !er->opened) {
		open_out(er);
	}
	if (er->out) putc(ch, er->out);
}

static void raw_write_command(struct EXPORT_RAW*er, int cmd, int nparams, unsigned char*params)
{
	if (!er->opened) {
		open_out(er);
	}
	if (!er->out) return;
	putc('\x1B', er->out);
	putc(cmd, er->out);
	fwrite(params, 1, nparams, er->out);
}

static void raw_close(struct EXPORT_RAW*er)
{
	if (!er->opened) return;
	er->opened = 0;
	if (er->out) {
		fclose(er->out);
	}
}

static void raw_free_data(struct EXPORT_RAW*er)
{
	if (er) {
		raw_close(er);
		free(er);
	}
}

static int raw_opened(struct EXPORT_RAW*er)
{
	return er->opened;
}


int  export_raw_init(struct EPSON_EXPORT*exp, unsigned flags, HWND wnd)
{
	struct EXPORT_RAW*er;

	er = calloc(sizeof(*er), 1);
	if (!er) return -1;

	er->flags = flags;
	er->wnd = wnd;

	exp->param = er;
	exp->write_char = raw_write_char;
	exp->write_command = raw_write_command;
	exp->close = raw_close;
	exp->free_data = raw_free_data;
	exp->opened = raw_opened;
	return 0;
}

