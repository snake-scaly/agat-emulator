#include "epson_emu.h"
#include "export_text.h"
#include "sysconf.h"
#include <io.h>
#include <stdio.h>


static unsigned char koi2win[128] = {
	128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
	144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 218, 155, 176, 157, 183, 159,
	160, 161, 162, 184, 186, 165, 166, 191, 168, 169, 170, 171, 172, 173, 174, 175,
	156, 177, 178, 168, 170, 181, 182, 175, 184, 185, 186, 187, 188, 189, 190, 185,
	254, 224, 225, 246, 228, 229, 244, 227, 245, 232, 233, 234, 235, 236, 237, 238,
	239, 255, 240, 241, 242, 243, 230, 226, 252, 251, 231, 248, 253, 249, 247, 250,
	222, 192, 193, 214, 196, 197, 212, 195, 213, 200, 201, 202, 203, 204, 205, 206,
	207, 223, 208, 209, 210, 211, 198, 194, 220, 219, 199, 216, 221, 217, 215, 218
};

static int charkoi2win(unsigned char c)
{
	if (c < 0x80) return c;
	return koi2win[c&0x7F];
}

struct EXPORT_TEXT
{
	FILE*out;
	HWND wnd;
	int  opened;
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

static void open_out(struct EXPORT_TEXT*et)
{
	char name[MAX_PATH];
	et->opened = 1;
	if (!get_name(et->wnd, name)) return;
	et->out = fopen(name, "wt");
	if (!et->out) return;
}

static void txt_write_char(struct EXPORT_TEXT*et, int ch)
{
	if (ch && !et->opened) {
		open_out(et);
	}
	if (!et->out) return;
	if (ch == 10) {
		fputc(13, et->out);
	}
	if (ch < ' ') return;
	fputc(charkoi2win(ch), et->out);
}

static void txt_write_command(struct EXPORT_TEXT*et, int cmd, int nparams, unsigned char*params)
{
	switch (cmd) {
	case 'L': case 'K': case 'Y': case 'Z': case '*':
		if (et->out) {
			int nchars = nparams / 12;
			for (;nchars;--nchars) fputc(' ', et->out);
		}
		break;
	}
}

static void txt_free_data(struct EXPORT_TEXT*et)
{
	if (et) {
		if (et->out) fclose(et->out);
		free(et);
	}
}


int  export_text_init(struct EPSON_EXPORT*exp, unsigned flags, HWND wnd)
{
	struct EXPORT_TEXT*et;

	et = calloc(sizeof(*et), 1);
	if (!et) return -1;

	et->wnd = wnd;

	exp->param = et;
	exp->write_char = txt_write_char;
	exp->write_command = txt_write_command;
	exp->free_data = txt_free_data;
	return 0;
}

