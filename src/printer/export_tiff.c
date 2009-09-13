#include "epson_emu.h"
#include "export.h"
#include "sysconf.h"
#include <io.h>
#include <stdio.h>
#include <assert.h>

#include "tiffwrite.h"

#define SRC_DPI 120
#define SRC_VERT_DPI 72
#define SRC_FONT_H 8
#define SRC_FONT_W 12

//#define TIFF_DEBUG
//#define PRINT_DEBUG

#ifdef TIFF_DEBUG
#define Tprintf(s) printf s
#define Tputs(s) puts(s)
#else
#define Tprintf(s)
#define Tputs(s)
#endif

#ifdef PRINT_DEBUG
#define Pprintf(s) printf s
#define Pputs(s) puts(s)
#else
#define Pprintf(s)
#define Pputs(s)
#endif


struct FONT_INFO
{
	int bold, dstrike;
	int italic;
	int underlined;
	int condensed, dwidth, dheight, cpi, proport;
	int subscript, superscript;
};


#define LINE_SIZE 256


struct EXPORT_TIFF
{
	FILE*out;
	struct TIFF_WRITER tiff;

	HWND wnd;
	unsigned flags;

	HDC	dc;
	HBITMAP bmp;

	HDC	bdc;
	HBITMAP bbmp;

	HFONT	fnt;
	struct FONT_INFO fi;
	int	fnt_dirty;
	int	pos_x, pos_y;
	int	char_w, char_h;
	int	line_h;
	int	page_cur_h, page_cur_lines;

	int	page_w, page_h;
	int	page_dirty;

	int	opened;

	int	res_x, res_y;
	int	x0, y0, y_set;

	double	margin[2];

	int	prev_char;
	int	was_cr, was_lf;
	int	auto_lf;
	int	auto_cr;

	int	vert_pos[LINE_SIZE];
	int	n_vert;


	void (*open_out)(struct EXPORT_TIFF*et);
	void (*close_out)(struct EXPORT_TIFF*et);
	void (*page_start)(struct EXPORT_TIFF*et);
	void (*page_finish)(struct EXPORT_TIFF*et);

	int  (*stretch)(struct EXPORT_TIFF*et, int w, int res);
};


static int get_name(HWND wnd, char*fname)
{
	int no;
	_mkdir(PRNOUT_DIR);
	for (no = 1; no; ++no) {
		sprintf(fname, PRNOUT_DIR"\\spool%03i.tiff", no);
		if (_access(fname, 0)) break;
	}	
	if (!select_save_tiff(wnd, fname)) {
		return 0;
	}
	return 1;
}

static void tiff_open_out(struct EXPORT_TIFF*et)
{
	char name[MAX_PATH];
	if (et->opened) return;
	et->opened = 1;
	if (!get_name(et->wnd, name)) {
		et->out = NULL;
		return;
	}
	et->out = fopen(name, "wb");
	if (!et->out) return;
	tiff_create(et->out, &et->tiff);

	et->dc = CreateCompatibleDC(NULL);
	assert(et->dc);
	et->bmp = CreateCompatibleBitmap(et->dc, et->page_w, et->page_h);
	assert(et->bmp);
	SelectObject(et->dc, et->bmp);
	SetStretchBltMode(et->dc, BLACKONWHITE);
	SetBkMode(et->dc, TRANSPARENT);
	SetTextColor(et->dc, RGB(0,0,0));
	SelectObject(et->dc, GetStockObject(WHITE_BRUSH));

	et->fnt_dirty = 1;
	et->page_dirty = 0;
	et->x0 = et->margin[0] * et->res_x;
	et->y0 = et->margin[1] * et->res_y;
	et->pos_x = et->x0;
	et->pos_y = et->y0;
	if (!et->line_h) {
		et->line_h = SRC_FONT_H * et->res_y / SRC_VERT_DPI;
		et->page_cur_h = et->page_h - et->line_h;
	}
}

static void tiff_close_out(struct EXPORT_TIFF*et)
{
	if (!et->opened) return;
	et->opened = 0;
	if (et->out) {
		tiff_finish(et->out, &et->tiff);
		fclose(et->out);
	}
	DeleteObject(et->bmp);
	DeleteDC(et->dc);
}


static void print_open_out(struct EXPORT_TIFF*et)
{
	char name[MAX_PATH];
	static PRINTDLG pd;
	DOCINFO di;
	if (et->opened) return;
	et->opened = 1;
	pd.lStructSize=sizeof(pd);
	pd.hwndOwner=et->wnd;
	pd.Flags|=PD_NOPAGENUMS|PD_NOSELECTION|PD_RETURNDC;
	if (!PrintDlg(&pd)) {
		et->dc = NULL;
		return;
	} else {
		et->dc = pd.hDC;
	}
	ZeroMemory(&di,sizeof(di));
	di.cbSize = sizeof(di);
	di.lpszDocName = TEXT("Agat printer output");
	StartDoc(et->dc, &di);

	SetStretchBltMode(et->dc, BLACKONWHITE);
	SetBkMode(et->dc, TRANSPARENT);
	SetTextColor(et->dc, RGB(0,0,0));
	SelectObject(et->dc, GetStockObject(WHITE_BRUSH));

	et->res_x = GetDeviceCaps(et->dc, LOGPIXELSX);
	et->res_y = GetDeviceCaps(et->dc, LOGPIXELSY);
	et->page_w = GetDeviceCaps(et->dc, HORZRES);
	et->page_h = GetDeviceCaps(et->dc, VERTRES);
	et->page_cur_h = et->page_h;
	et->fnt_dirty = 1;
	et->page_dirty = 0;
	et->x0 = et->margin[0] * et->res_x;
	et->y0 = et->margin[1] * et->res_y;
	et->pos_x = et->x0;
	et->pos_y = et->y0;
	if (!et->line_h) {
		et->line_h = SRC_FONT_H * et->res_y / SRC_VERT_DPI;
		et->page_cur_h = et->page_h - et->line_h;
	}
	Pprintf(("res: %ix%i\nsize: %ix%i\n", et->res_x, et->res_y, et->page_w, et->page_h));
}

static void print_close_out(struct EXPORT_TIFF*et)
{
	if (!et->opened) return;
	et->opened = 0;
	if (et->dc) {
		Pputs("printer end doc");
		EndDoc(et->dc);
		DeleteDC(et->dc);
	}
}


static void create_font(struct EXPORT_TIFF*et)
{
	int cw, ch;
	et->fnt_dirty = 0;
	et->char_w = et->res_x * SRC_FONT_W / SRC_DPI;
	et->char_h = et->res_y * SRC_FONT_H / SRC_VERT_DPI;
	if (et->fi.condensed) et->char_w /= 2;
	if (et->fi.dwidth) et->char_w *= 2;
	if (et->fi.dheight) et->char_h *= 2;
	cw = et->char_w;
	ch = et->char_h;
	if (et->fi.subscript || et->fi.superscript) {
		cw = cw * 2 / 3;
		ch = ch * 2 / 3;
	}
/*	if (!et->fi.cpi) et->fi.cpi = 10;
	et->char_w = (et->char_w * 10) / et->fi.cpi;*/
	Tprintf(("char_w = %i; char_h = %i; condensed = %i\n", et->char_w, et->char_h, et->fi.condensed));
	et->fnt = CreateFont(-ch, cw-1, 0, 0, et->fi.bold?FW_HEAVY:(et->fi.dstrike?FW_SEMIBOLD:FW_NORMAL), 
		et->fi.italic, et->fi.underlined, 0, RUSSIAN_CHARSET, OUT_TT_PRECIS,
		0, PROOF_QUALITY, FIXED_PITCH, TEXT("Courier New"));
	DeleteObject(SelectObject(et->dc, et->fnt));
}


static void graphout(struct EXPORT_TIFF*et, int res, int w, const unsigned char*data);

static void printch(struct EXPORT_TIFF*et, int ch)
{
	int y;
	if (et->fnt_dirty) {
		create_font(et);
	}
	if (!et->page_dirty) {
		et->page_start(et);
		et->page_dirty = 1;
	}
//	Tprintf(("text out at %i,%i\n", et->pos_x, et->pos_y));
	Tprintf(("%c", ch));
	y = et->pos_y;
	if (et->fi.subscript) y += et->char_h / 3;
/*	SelectObject(et->dc, GetStockObject(BLACK_PEN));
	Rectangle(et->dc, et->pos_x, et->pos_y, et->pos_x + et->char_w, et->pos_y + et->char_h);
*/
/*	if (ch == '|') {
		unsigned char bar_data[] = { 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 0, 0, 0, 0 };
		int nh = et->line_h  * SRC_VERT_DPI / et->res_y;
		unsigned char b = ~ ((1L << (8-nh)) - 1);
		printf("nh = %i, b = %x\n", nh, b);
		bar_data[6] = bar_data[7] = b;
		graphout(et, 120, 12, bar_data);
	} else*/ {
		if (ch != ' ') {
//			RECT r = { et->pos_x, et->pos_y, et->pos_x + et->char_w, et->pos_y + et->line_h };
//			DrawText(et->dc, (LPCTSTR)&ch, 1, &r, DT_LEFT | DT_TOP | DT_SINGLELINE);
			if (ch == '|') {
				if (et->n_vert < LINE_SIZE) {
					et->vert_pos[et->n_vert++] = et->pos_x;
				}
			} else {
				TextOut(et->dc, et->pos_x, y, (LPCTSTR)&ch, 1);
			}	
		}
		et->pos_x += et->char_w;
	}
}

static void page_cr(struct EXPORT_TIFF*et)
{
	et->pos_x = et->x0;
	et->was_cr = 1;
}

static void new_page(struct EXPORT_TIFF*et);

static void page_lf(struct EXPORT_TIFF*et)
{
	{
		int i;
		for (i = 0; i < et->n_vert; ++i) {
			RECT r = {et->vert_pos[i] + 6*et->res_x/SRC_DPI, et->pos_y, et->vert_pos[i] + 8*et->res_x/SRC_DPI, et->pos_y + et->line_h};
			FillRect(et->dc, &r, GetStockObject(BLACK_BRUSH));
		}
	}
//	printch(et, 0xB6);

	et->pos_y += et->line_h;
	if (et->page_cur_lines) {
		et->page_cur_h = et->page_cur_lines * 12 * et->res_y / SRC_VERT_DPI;
		et->page_cur_lines = 0;
	}

	Tprintf(("line feed: new pos_y = %i (line_h = %i); page_h = %i\n",et->pos_y, et->line_h, et->page_cur_h));
	if (et->fi.dwidth || et->fi.dheight || et->fi.condensed) et->fnt_dirty = 1;
	et->fi.dwidth = 0;
	et->fi.dheight = 0;
	et->fi.condensed = 0;
	et->was_lf = 1;
	et->n_vert = 0;
	if (et->pos_y - et->y0 + 2 * et->res_y / SRC_VERT_DPI - et->y_set > et->page_cur_h) new_page(et);
}

static void page_bs(struct EXPORT_TIFF*et)
{
	if (et->pos_x > et->x0) et->pos_x -= et->char_w;
}

static int count_rle(unsigned char*data, int len, int*ofs)
{
	int d0 = data[0];
	int cnt = 0;
	int oc = 0, o0 = 0;
	for ( ; len; --len, ++data, ++oc) {
		if (d0 == *data) {
			++cnt;
		} else {
			if (cnt > 3) break;
			cnt = 1;
			o0 = oc;
			d0 = *data;
		}
	}
	*ofs = o0;
	return cnt;
}

static int write_uncompr_rle(FILE*out, struct TIFF_WRITER*tiff, unsigned char*data, int len)
{
	while (len) {
		int n = len > 128? 128: len;
		unsigned char d0 = n - 1;
//		printf("tiff_literal %i\n", d0);
		tiff_add_strip_data(out, tiff, 0, &d0, 1);
		tiff_add_strip_data(out, tiff, 0, data, n);
		len -= n;
		data+= n;
	}
	return 0;
}

static int write_rle_rept(FILE*out, struct TIFF_WRITER*tiff, unsigned char val, int cnt)
{
	while (cnt) {
		int n = cnt > 128? 128: cnt;
		unsigned char d0 = 257 - n;
//		printf("tiff_rept %i(%i),%i\n", d0, n, val);
		tiff_add_strip_data(out, tiff, 0, &d0, 1);
		tiff_add_strip_data(out, tiff, 0, &val, 1);
		cnt -= n;
	}
	return 0;
}
/*
void dump_buf(unsigned char*data, int len)
{
	for(; len; --len, ++data) printf("%02X ", *data);
}
*/

static int compress_rle_line(FILE*out, struct TIFF_WRITER*tiff, unsigned char*data, int len)
{
//	printf("compress_rle_line: len = %i\n", len);
//	dump_buf(data, len);
	while (len) {
		int ofs, cnt, n;
		cnt = count_rle(data, len, &ofs);
//		printf("count_rle(%i): ofs = %i, cnt = %i\n", len, ofs, cnt);
		if (cnt > 3) {
			if (ofs) write_uncompr_rle(out, tiff, data, ofs);
			write_rle_rept(out, tiff, data[ofs], cnt);
			n = ofs + cnt;
		} else {
			write_uncompr_rle(out, tiff, data, len);
			n = len;
		}
		len -= n;
		data += n;
	}
	return 0;
}

static void tiff_start_page(struct EXPORT_TIFF*et)
{
	RECT r = {0, 0, et->page_w, et->page_h};
	FillRect(et->dc, &r, GetStockObject(WHITE_BRUSH));
}

static void tiff_finish_page(struct EXPORT_TIFF*et)
{
	FILE*out = et->out;
	struct TIFF_WRITER *tiff = &et->tiff;
	struct {
		BITMAPINFOHEADER bih;
		RGBQUAD pal[2];
	} bd;
	void*buf;
	int compr = et->flags & EXPORT_TIFF_COMPRESS_RLE?32773 : 1;

	if (!out) return;

	tiff_new_page(out, tiff);
	tiff_set_strips_count(tiff, 1);
	tiff_new_chunk_short(out, tiff, 254, 2); // new subfile type
	tiff_new_chunk_long(out, tiff, 256, et->page_w); // width +
	tiff_new_chunk_long(out, tiff, 257, et->page_h); // height +
	tiff_new_chunk_short(out, tiff, 258, 1); // bits per sample
	tiff_new_chunk_short(out, tiff, 259, compr); // compression +
	tiff_new_chunk_short(out, tiff, 262, 1); // black = 0 +
	tiff_new_chunk_string(out, tiff, 269, "Agat emulator document"); // document name
	tiff_new_chunk_long(out, tiff, 273, 0); // strip offsets +
	tiff_new_chunk_short(out, tiff, 274, 1); // orientation
	tiff_new_chunk_short(out, tiff, 277, 1); // components per pixel
	tiff_new_chunk_long(out, tiff, 278, et->page_h); // rows per strip + 
	tiff_new_chunk_long(out, tiff, 279, 0); // strip sizes +
	tiff_new_chunk_rat(out, tiff, 282, et->res_x, 1); // xres +
	tiff_new_chunk_rat(out, tiff, 283, et->res_y, 1); // yres +
	tiff_new_chunk_short(out, tiff, 296, 2); // unit, 2 = inch +
	tiff_new_chunk_string(out, tiff, 305, "Agat emulator"); // software

	ZeroMemory(&bd, sizeof(bd));
	bd.bih.biSize = sizeof(bd.bih);
	bd.bih.biWidth = et->page_w;
	bd.bih.biHeight = -et->page_h;
	bd.bih.biPlanes = 1;
	bd.bih.biBitCount = 1;
	bd.bih.biCompression = 0;
	GetDIBits(et->dc, et->bmp, 0, et->page_h, NULL, (LPBITMAPINFO)&bd, DIB_RGB_COLORS);
	Tprintf(("buffer size = %i\n", bd.bih.biSizeImage));
	buf = malloc(bd.bih.biSizeImage);
	assert(buf);
	GetDIBits(et->dc, et->bmp, 0, et->page_h, buf, (LPBITMAPINFO)&bd, DIB_RGB_COLORS);
	if (compr == 1) {
		tiff_add_strip_data(et->out, &et->tiff, 0, buf, bd.bih.biSizeImage);
	} else {
		int ls = et->page_w / 8;
		int n = et->page_h;
		unsigned char *d = buf;
		for (; n; --n, d+=ls) {
			compress_rle_line(et->out, &et->tiff, d, ls);
		}
	}
	free(buf);

}

static void print_start_page(struct EXPORT_TIFF*et)
{
	if (!et->dc) return;
	StartPage(et->dc);
	Pputs("print start page");
}

static void print_finish_page(struct EXPORT_TIFF*et)
{
	if (!et->dc) return;
	EndPage(et->dc);
	Pputs("print end page");
}


static void new_page(struct EXPORT_TIFF*et)
{
	if (et->page_dirty) {
		et->page_finish(et);
	}
	et->pos_x = et->x0;
	et->pos_y = et->y0;
	et->y_set = 0;
	et->page_dirty = 0;
	et->prev_char = 0;
	et->was_cr = et->was_lf = 1;
}

static void tiff_write_char(struct EXPORT_TIFF*et, int ch)
{
	int lch = et->prev_char;
	if (ch && !et->opened) {
		et->open_out(et);
	}
	et->prev_char = ch;
	if (!et->opened) return;
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
		if (et->auto_cr) page_cr(et);
		page_lf(et);
		break;
	case EPS_SI:
		et->fnt_dirty = 1;
		et->fi.condensed = 1;
		break;
	case EPS_SO:
		et->fnt_dirty = 1;
		et->fi.dwidth = 1;
		break;
	case EPS_DC2:
		et->fnt_dirty = 1;
		et->fi.condensed = 0;
		break;
	case EPS_DC4:
		et->fnt_dirty = 1;
		et->fi.dwidth = 0;
		break;
	}
	if (ch < ' ') return;
	if (et->was_cr && !et->was_lf && et->auto_lf) {
		page_lf(et);
	}
	et->was_cr = et->was_lf = 0;
	printch(et, ch);
}


static int raster_stretch(struct EXPORT_TIFF*et, int w, int res)
{
	int w1;
	w1 = w * et->res_x / res;
	StretchBlt(et->dc, et->pos_x, et->pos_y, w1, 8 * et->res_y / SRC_VERT_DPI,
		et->bdc, 0, 0, w, 8, SRCAND);
	return w1;
}


static HRGN get_black_rgn(struct EXPORT_TIFF*et, int w, int res)
{
	int x, y;
	HRGN r;
	r = CreateRectRgn(0, 0, 0, 0);
	for (x = 0; x < w; ++x) {
		for (y = 0; y < 8; ++y) {
			HRGN r1;
			COLORREF cl = GetPixel(et->bdc, x, y);
			if (GetRValue(cl) || GetGValue(cl) || GetBValue(cl)) continue;
			r1 = CreateRectRgn(et->pos_x + x * et->res_x / res,
				et->pos_y + y * et->res_y / SRC_VERT_DPI,
				et->pos_x + (x + 1) * et->res_x / res,
				et->pos_y + (y + 1) * et->res_y / SRC_VERT_DPI);
			CombineRgn(r, r, r1, RGN_OR);
			DeleteObject(r1);
		}
	}
	return r;
}

static int vector_stretch(struct EXPORT_TIFF*et, int w, int res)
{
	int w1;
	HRGN rgn = get_black_rgn(et, w, res);
	w1 = w * et->res_x / res;
	FillRgn(et->dc, rgn, GetStockObject(BLACK_BRUSH));
	DeleteObject(rgn);
	return w1;
}

static  void graphout(struct EXPORT_TIFF*et, int res, int w, const unsigned char*data)
{
	int x, y, m;
	if (!et->page_dirty) {
		et->page_start(et);
		et->page_dirty = 1;
	}
	et->prev_char = 0;
	et->was_cr = et->was_lf = 0;
	Tprintf(("graphics output at %i,%i, width = %i\n",et->pos_x,et->pos_y,w));
	Tprintf(("\ngraphics data:\n"));
	for (y = 0, m = 0x80; y < 8; ++y, m>>=1) {
		for (x = 0; x < w; ++x) {
			Tprintf(("%c",(data[x]&m)?'*':'\xB7'));
		}
		Tprintf(("\n"));
	}
	for (x = 0; x < w; ++x, ++data) {
		for (y = 0, m = 0x80; y < 8; ++y, m>>=1) {
			SetPixelV(et->bdc, x, y, ((*data)&m)?RGB(0,0,0):RGB(255,255,255));
		}
	}
	Tprintf(("line height = %i; graphics height = %i\n", et->line_h, 8 * et->res_y / SRC_VERT_DPI));
	et->pos_x += et->stretch(et, w, res);
}

static void tiff_write_command(struct EXPORT_TIFF*et, int cmd, int nparams, unsigned char*params)
{
	switch (cmd) {
	case '0':
		et->line_h = et->res_y / 8;
		break;
	case '-':
		if (nparams == 1) {
			switch (*params) {
			case 0: case '0':
				if (et->fi.underlined) et->fnt_dirty = 1;
				et->fi.underlined = 0;
				break;
			case 1: case '1':
				if (!et->fi.underlined) et->fnt_dirty = 1;
				et->fi.underlined = 1;
				break;
			}
		}
		break;
	case '+':
		if (nparams == 1) {
			et->line_h = *params * et->res_y / 360;
		}
		break;
	case 'w':
		if (nparams == 1) {
			switch (*params) {
			case 0: case '0':
				if (et->fi.dheight) et->fnt_dirty = 1;
				et->fi.dheight = 0;
				break;
			case 1: case '1':
				if (!et->fi.dheight) et->fnt_dirty = 1;
				et->fi.dheight = 1;
				break;
			}
		}
		break;
	case '!':
		if (nparams == 1) {
			unsigned char f = *params;
			et->fnt_dirty = 1;
			if (f & 1) {
				et->fi.cpi = 10;
			} else {
				et->fi.cpi = 12;
			}
			if (f & 2) {
				et->fi.proport = 1;
			} else {
				et->fi.proport = 0;
			}
			if (f & 4) {
				et->fi.condensed = 1;
			} else {
				et->fi.condensed = 0;
			}
			if (f & 8) {
				et->fi.bold = 1;
			} else {
				et->fi.bold = 0;
			}
			if (f & 16) {
				et->fi.dstrike = 1;
			} else {
				et->fi.dstrike = 0;
			}
			if (f & 32) {
				et->fi.dwidth = 1;
			} else {
				et->fi.dwidth = 0;
			}
			if (f & 64) {
				et->fi.italic = 1;
			} else {
				et->fi.italic = 0;
			}
			if (f & 128) {
				et->fi.underlined = 1;
			} else {
				et->fi.underlined = 0;
			}
		}
	case '1':
		et->line_h = et->res_y * 7 / 72;
		break;
	case '2':
		et->line_h = et->res_y / 6;
		break;
	case '3':
		if (nparams == 1) {
			et->line_h = *params * et->res_y / 216;
		}
		break;
	case '4':
		if (!et->fi.italic) et->fnt_dirty = 1;
		et->fi.italic = 1;
		break;
	case '5':
		if (et->fi.italic) et->fnt_dirty = 1;
		et->fi.italic = 0;
		break;
	case 'A':
//		printch(et, 0xA4);
		if (nparams == 1) {
			et->line_h = (*params) * et->res_y / 72;
		}
		break;
	case 'C':
//		et->y_set = et->pos_y - et->y0;
//		printch(et, 0xA7);
		switch (nparams) {
		case 1:
			et->page_cur_lines = params[0];
			break;
		case 2:
			et->page_cur_h = (params[1])*et->res_y;
			break;
		}
		Tprintf(("et->page_cur_h = %i\n", et->page_cur_h));
		break;
	case 'E':
		if (!et->fi.bold) et->fnt_dirty = 1;
		et->fi.bold = 1;
		break;
	case 'F':
		if (et->fi.bold) et->fnt_dirty = 1;
		et->fi.bold = 0;
		break;
	case 'G':
		if (!et->fi.dstrike) et->fnt_dirty = 1;
		et->fi.dstrike = 1;
		break;
	case 'H':
		if (et->fi.dstrike) et->fnt_dirty = 1;
		et->fi.dstrike = 0;
		break;
	case 'J':
		if (nparams == 1) {
			et->pos_y += (*params) * et->res_y / 216;
		}
		break;
	case 'M':
		et->fnt_dirty = 1;
		et->fi.cpi = 12;
		break;
	case 'P':
		et->fnt_dirty = 1;
		et->fi.cpi = 10;
		break;
	case 'g':
		et->fnt_dirty = 1;
		et->fi.cpi = 15;
		break;
	case EPS_SI:
		et->fnt_dirty = 1;
		et->fi.condensed = 1;
		break;
	case EPS_SO:
		et->fnt_dirty = 1;
		et->fi.dwidth = 2;
		break;
	case 'L':
	case 'Y':
		graphout(et, 120, nparams, params);
		break;
	case 'K':
		graphout(et, 60, nparams, params);
		break;
	case 'S':
		if (nparams == 1) {
		switch (*params) {
		case 0: case 48:
			if (et->fi.subscript || !et->fi.superscript) et->fnt_dirty = 1;
			et->fi.subscript = 0;
			et->fi.superscript = 1;
			break;
		case 1: case 49:
			if (!et->fi.subscript || et->fi.superscript) et->fnt_dirty = 1;
			et->fi.subscript = 1;
			et->fi.superscript = 0;
			break;
		} }
		break;
	case 'T':
		if (et->fi.subscript || et->fi.superscript) et->fnt_dirty = 1;
		et->fi.subscript = et->fi.superscript = 0;
		break;
	case 'Z':
		graphout(et, 240, nparams, params);
		break;
	case '*': // unsupported yet
		break;
	default:
		Tprintf(("Tiff: unsupported command %c (%i args)\n", cmd, nparams));
	}
}


static void tiff_close(struct EXPORT_TIFF*et)
{
	if (!et->opened) return;
	if (et->page_dirty) {
		et->page_finish(et);
	}
	et->close_out(et);
}

static void tiff_free_data(struct EXPORT_TIFF*et)
{
	if (et) {
		tiff_close(et);
		DeleteObject(et->fnt);
		DeleteObject(et->bbmp);
		DeleteDC(et->bdc);
		free(et);
	}
}

static int tiff_opened(struct EXPORT_TIFF*et)
{
	return et->opened;
}


int export_comm_init(struct EXPORT_TIFF*et)
{
	et->margin[0] = 0;
	et->margin[1] = 0.3; // inches
	return 0;
}


int  export_tiff_init(struct EPSON_EXPORT*exp, unsigned flags, HWND wnd)
{
	struct EXPORT_TIFF*et;

	et = calloc(sizeof(*et), 1);
	if (!et) return -1;

	export_comm_init(et);

	et->flags = flags;
	et->wnd = wnd;
	et->res_x = et->res_y = 300; // dpi
	et->page_w = 8.25 * et->res_x;
	et->page_w = (et->page_w + 31) & ~31;
	et->page_h = 11.75 * et->res_y;
	et->page_cur_h = et->page_h;
	et->x0 = et->y0 = 40;
	et->pos_x = et->x0;
	et->pos_y = et->y0;
	et->auto_lf = et->auto_cr = 1;
	et->y_set = 0;
	et->page_cur_lines = 0;

	et->page_finish = tiff_finish_page;
	et->page_start = tiff_start_page;
	et->open_out = tiff_open_out;
	et->close_out = tiff_close_out;
	et->stretch = raster_stretch;

	et->bdc = CreateCompatibleDC(NULL);
	assert(et->bdc);
//	et->bbmp = CreateCompatibleBitmap(et->bdc, et->page_w, et->page_h/66);
	et->bbmp = CreateBitmap(et->page_w, et->page_h/66, 1, 1, NULL);
	assert(et->bbmp);
	SelectObject(et->bdc, et->bbmp);

	new_page(et);

	et->fnt_dirty = 1;

	exp->param = et;
	exp->write_char = tiff_write_char;
	exp->write_command = tiff_write_command;
	exp->close = tiff_close;
	exp->free_data = tiff_free_data;
	exp->opened = tiff_opened;
	return 0;
}



int  export_print_init(struct EPSON_EXPORT*exp, unsigned flags, HWND wnd)
{
	struct EXPORT_TIFF*et;

	et = calloc(sizeof(*et), 1);
	if (!et) return -1;

	export_comm_init(et);

	et->flags = flags;
	et->wnd = wnd;
	et->res_x = et->res_y = 300; // dpi
	et->page_w = 8.25 * et->res_x;
	et->page_w = (et->page_w + 31) & ~31;
	et->page_cur_h = et->page_h;
	et->page_h = 11.75 * et->res_y;
	et->x0 = et->y0 = 40;
	et->auto_lf = et->auto_cr = 1;
	et->y_set = 0;
	et->page_cur_lines = 0;

	et->page_finish = print_finish_page;
	et->page_start = print_start_page;
	et->open_out = print_open_out;
	et->close_out = print_close_out;
	et->stretch = vector_stretch;

	et->bdc = CreateCompatibleDC(NULL);
	assert(et->bdc);
	et->bbmp = CreateBitmap(et->page_w, et->page_h/66, 1, 1, NULL);
	assert(et->bbmp);
	SelectObject(et->bdc, et->bbmp);

	new_page(et);

	et->fnt_dirty = 1;

	exp->param = et;
	exp->write_char = tiff_write_char;
	exp->write_command = tiff_write_command;
	exp->close = tiff_close;
	exp->free_data = tiff_free_data;
	exp->opened = tiff_opened;
	return 0;
}

