#include "epson_emu.h"
#include "export.h"
#include "sysconf.h"
#include <io.h>
#include <stdio.h>
#include <assert.h>

#include "tiffwrite.h"

struct EXPORT_TIFF
{
	FILE*out;
	struct TIFF_WRITER tiff;

	HWND wnd;
	unsigned flags;

	HDC	dc;
	HBITMAP bmp;

	HFONT	fnt;
	int	fnt_dirty;
	int	pos_x, pos_y;
	int	char_w, char_h;

	int	page_w, page_h;
	int	page_dirty;

	int	opened;

	int  res;

};


static int get_name(HWND wnd, char*fname)
{
	int no;
	_mkdir(PRNOUT_DIR);
	for (no = 1; no; ++no) {
		sprintf(fname, PRNOUT_DIR"\\spool%03i.tiff", no);
		if (_access(fname, 0)) break;
	}	
	if (!select_save_text(wnd, fname)) {
		return 0;
	}
	return 1;
}

static void open_out(struct EXPORT_TIFF*et)
{
	char name[MAX_PATH];
	et->opened = 1;
	if (!get_name(et->wnd, name)) return;
	et->out = fopen(name, "wb");
	if (!et->out) return;
	tiff_create(et->out, &et->tiff);
}


static void create_font(struct EXPORT_TIFF*et)
{
	et->fnt_dirty = 0;
	if (et->fnt) DeleteObject(et->fnt);
	et->fnt = CreateFont(et->char_h, et->char_w, 0, 0, FW_NORMAL, 0, 0, 0, RUSSIAN_CHARSET, 0,
		0, 0, FIXED_PITCH, TEXT("Small"));
	SelectObject(et->dc, et->fnt);
}


static void printch(struct EXPORT_TIFF*et, int ch)
{
	if (et->fnt_dirty) {
		create_font(et);
	}
	et->page_dirty = 1;
	printf("text out at %i,%i\n", et->pos_x, et->pos_y);
	TextOut(et->dc, et->pos_x, et->pos_y, (LPCTSTR)&ch, 1);
	et->pos_x += et->char_w;
}

static void page_cr(struct EXPORT_TIFF*et)
{
	et->pos_x = 0;
}

static void page_lf(struct EXPORT_TIFF*et)
{
	et->pos_y += et->char_h;
}

static void page_bs(struct EXPORT_TIFF*et)
{
	if (et->pos_x > 0) et->pos_x -= et->char_w;
}

static void save_page(struct EXPORT_TIFF*et)
{
	FILE*out = et->out;
	struct TIFF_WRITER *tiff = &et->tiff;
	struct {
		BITMAPINFOHEADER bih;
		RGBQUAD pal[2];
	} bd;
	void*buf;


	tiff_new_page(out, tiff);
	tiff_set_strips_count(tiff, 1);
	tiff_new_chunk_short(out, tiff, 254, 1); // new subfile type
	tiff_new_chunk_long(out, tiff, 256, et->page_w); // width +
	tiff_new_chunk_long(out, tiff, 257, et->page_h); // height +
	tiff_new_chunk_short(out, tiff, 258, 1); // bits per sample
	tiff_new_chunk_short(out, tiff, 259, 1); // no compression +
	tiff_new_chunk_short(out, tiff, 262, 1); // black = 0 +
	tiff_new_chunk_string(out, tiff, 269, "Agat emulator document"); // document name
	tiff_new_chunk_long(out, tiff, 273, 0); // strip offsets +
	tiff_new_chunk_short(out, tiff, 274, 1); // orientation
	tiff_new_chunk_short(out, tiff, 277, 1); // components per pixel
	tiff_new_chunk_long(out, tiff, 278, et->page_h); // rows per strip + 
	tiff_new_chunk_long(out, tiff, 279, 0); // strip sizes +
	tiff_new_chunk_rat(out, tiff, 282, et->res, 1); // xres +
	tiff_new_chunk_rat(out, tiff, 283, et->res, 1); // yres +
	tiff_new_chunk_short(out, tiff, 296, 2); // unit, 2 = inch +

	bd.bih.biSize = sizeof(bd.bih);
	bd.bih.biWidth = et->page_w;
	bd.bih.biHeight = -et->page_h;
	bd.bih.biPlanes = 1;
	bd.bih.biBitCount = 1;
	bd.bih.biCompression = 0;
	BitBlt(GetDC(GetDesktopWindow()), 0, 0, et->page_w, et->page_h, et->dc, 0, 0, SRCCOPY);
	GetDIBits(et->dc, et->bmp, 0, et->page_h, NULL, (LPBITMAPINFO)&bd, DIB_RGB_COLORS);
	printf("buffer size = %i\n", bd.bih.biSizeImage);
	buf = malloc(bd.bih.biSizeImage);
	assert(buf);
	GetDIBits(et->dc, et->bmp, 0, et->page_h, buf, (LPBITMAPINFO)&bd, DIB_RGB_COLORS);
	tiff_add_strip_data(et->out, &et->tiff, 0, buf, bd.bih.biSizeImage);
	free(buf);
}

static void new_page(struct EXPORT_TIFF*et)
{
	RECT r = {0, 0, et->page_w, et->page_h};
	if (et->page_dirty && et->out) {
		save_page(et);
	}
	et->pos_x = 0;
	et->pos_y = 0;
	FillRect(et->dc, &r, GetStockObject(WHITE_BRUSH));
	et->page_dirty = 0;
}

static void tiff_write_char(struct EXPORT_TIFF*et, int ch)
{
	if (ch && !et->opened) {
		open_out(et);
	}
	if (!et->out) return;
	switch (ch) {
	case EPS_BS:
		page_bs(et);
		break;
	case EPS_FF:
		new_page(et);
		break;
	case EPS_CR:
		page_cr(et);
		break;
	case EPS_LF:
		page_cr(et);
		page_lf(et);
		break;
	}
	if (ch < ' ') return;
	printch(et, ch);
}

static void tiff_write_command(struct EXPORT_TIFF*et, int cmd, int nparams, unsigned char*params)
{
	switch (cmd) {
	case 'L': case 'K': case 'Y': case 'Z': case '*':
		if (et->out) {
			int nchars = nparams / 12;
//			for (;nchars;--nchars) fputc(' ', et->out);
		}
		break;
	}
}

static void tiff_free_data(struct EXPORT_TIFF*et)
{
	if (et) {
		if (et->out) {
			if (et->page_dirty && et->out) {
				save_page(et);
			}
			tiff_finish(et->out, &et->tiff);
			fclose(et->out);
		}
		free(et);
	}
}


int  export_tiff_init(struct EPSON_EXPORT*exp, unsigned flags, HWND wnd)
{
	struct EXPORT_TIFF*et;

	et = calloc(sizeof(*et), 1);
	if (!et) return -1;

	et->flags = flags;
	et->wnd = wnd;
	et->res = 300; // dpi
	et->page_w = 1024;
	et->page_h = 3000;
	et->char_w = 12;
	et->char_h = 14;

	et->dc = CreateCompatibleDC(GetDC(GetDesktopWindow()));
	assert(et->dc);
	et->bmp = CreateCompatibleBitmap(et->dc, et->page_w, et->page_h);
	assert(et->bmp);
	SelectObject(et->dc, et->bmp);
	SetBkMode(et->dc, TRANSPARENT);
	SetTextColor(et->dc, RGB(0,0,0));
	BitBlt(GetDC(GetDesktopWindow()), 0, 0, et->page_w, et->page_h, et->dc, 0, 0, SRCCOPY);

	new_page(et);

	et->fnt_dirty = 1;

	exp->param = et;
	exp->write_char = tiff_write_char;
	exp->write_command = tiff_write_command;
	exp->free_data = tiff_free_data;
	return 0;
}

