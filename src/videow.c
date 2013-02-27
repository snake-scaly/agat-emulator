/*
	Agat Emulator version 1.19
	Copyright (c) NOP, nnop@newmail.ru
*/

#include <windows.h>
#include "resource.h"

#include "common.h"
#include "runmgr.h"
#include "memory.h"
#include "runmgrint.h"
#include "videow.h"
#include "runstate.h"
#include "debug/debugwnd.h"

#include "localize.h"
#include <stdio.h>

#define VIDEO_CLASS TEXT("agat_video")

#define MAX_VIDEO_W (40*CHAR_W)
#define MAX_VIDEO_H (32*CHAR_H)


static LRESULT CALLBACK wnd_proc(HWND w,UINT msg,WPARAM wp,LPARAM lp);

static ATOM at;

static WORD get_keyb_language()
{
	return PRIMARYLANGID(LOWORD(GetKeyboardLayout(0)));
}

int is_keyb_english(struct SYS_RUN_STATE*sr)
{
	if (sr->gconfig->flags & EMUL_FLAGS_LANGSEL) {
		return !sr->cur_lang;
	} else {
		return get_keyb_language() == LANG_ENGLISH;
	}
}

int is_shift_pressed(struct SYS_RUN_STATE*sr)
{
	return keyb_is_pressed(sr, VK_SHIFT);
}

int is_ctrl_pressed(struct SYS_RUN_STATE*sr)
{
	return keyb_is_pressed(sr, VK_CONTROL);
}

int is_alt_pressed(struct SYS_RUN_STATE*sr)
{
	return keyb_is_pressed(sr, VK_MENU);
}


void update_alt_state(struct SYS_RUN_STATE*sr)
{
	if (keyb_is_pressed(sr, VK_LMENU)) sr->mousebtn|=0x10;
	else sr->mousebtn&=~0x10;
	if (keyb_is_pressed(sr, VK_RMENU)) sr->mousebtn|=0x20;
	else sr->mousebtn&=~0x20;
}

int register_video_window(struct SYS_RUN_STATE*sr)
{
	WNDCLASS cl;
	HDC dc;
	TCHAR cbuf[32];
	static int cno = 1;
	ZeroMemory(&cl,sizeof(cl));
	cl.hbrBackground=NULL;
	cl.lpfnWndProc=wnd_proc;
	wsprintf(cbuf, VIDEO_CLASS TEXT("_%04i"), cno++);
	cl.lpszClassName=cbuf;
	cl.hCursor=LoadCursor(NULL,IDC_ARROW);
	dc = GetDC(sr->base_w);
	sr->video_icon = cl.hIcon=sysicon_to_icon(dc, &sr->config->icon);
	ReleaseDC(sr->base_w, dc);
	if (!cl.hIcon) cl.hIcon = LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_MAIN));
	sr->video_at=RegisterClass(&cl);
	if (!sr->video_at) return -1;
	return 0;
}

void unregister_video_window(struct SYS_RUN_STATE*sr)
{
	UnregisterClass((LPCTSTR)sr->video_at, GetModuleHandle(NULL));
	if (sr->video_icon) DestroyIcon(sr->video_icon);
}


int load_video_palette(struct SYS_RUN_STATE*sr, RGBQUAD colors[16])
{
	int cl;
	FILE*in;
	for (cl=0;cl<16;cl++) {
		int v=(cl&8)?255:128;
		colors[cl].rgbRed=(cl&1)?v:0;
		colors[cl].rgbGreen=(cl&2)?v:0;
		colors[cl].rgbBlue=(cl&4)?v:0;
	}
	in = fopen(sr->config->slots[CONF_PALETTE].cfgstr[CFG_STR_ROM], "rt");
	if (!in) return 1;
	for (cl=0;cl<16;cl++) {
		char buf[1024];
		int n, r, g, b;
		fgets(buf, sizeof(buf), in);
		if (sscanf(buf, "%i %i %i", &r, &g, &b) == 3) {
			colors[cl].rgbRed = r;
			colors[cl].rgbGreen = g;
			colors[cl].rgbBlue = b;
		}
	}
	fclose(in);
	return 0;
}

int create_video_buffer(struct SYS_RUN_STATE*sr)
{
	struct {
		BITMAPINFOHEADER h;
		RGBQUAD colors[16];
	} bi;
	ZeroMemory(&bi,sizeof(bi));
	bi.h.biSize=sizeof(bi.h);
	bi.h.biWidth=MAX_VIDEO_W;
	bi.h.biHeight=-MAX_VIDEO_H;
	bi.h.biPlanes=1;
	bi.h.biBitCount=4;
	bi.h.biCompression=BI_RGB;
	bi.h.biSizeImage=0;
	sr->bmp_pitch=bi.h.biWidth>>1;
	load_video_palette(sr, bi.colors);
	sr->mem_dc=CreateCompatibleDC(NULL);
	if (!sr->mem_dc) return -3;
	sr->mem_bmp=CreateDIBSection(NULL,(BITMAPINFO*)&bi,DIB_RGB_COLORS,&sr->bmp_bits,NULL,0);
	if (!sr->mem_bmp) return -2;
	if (!sr->bmp_bits) return -4;
	sr->fullscreen = 0;
	GdiFlush();
	sr->old_bmp = SelectObject(sr->mem_dc, sr->mem_bmp);
	return 0;
}

int free_video_buffer(struct SYS_RUN_STATE*sr)
{
	if (sr->debug_ptr) debug_detach(sr);
	sr->bmp_bits = NULL;
	SelectObject(sr->mem_dc, sr->old_bmp);
	DeleteObject(sr->mem_bmp);
	DeleteDC(sr->mem_dc);
	return 0;
}

LPCTSTR get_system_name(struct SYS_RUN_STATE *sr)
{
	return get_sysnames(sr->config->systype);
}


static DWORD CALLBACK cr_proc(LPVOID par)
{
	struct SYS_RUN_STATE*sr = par;
	MSG msg;
#ifdef UNDER_CE
	DWORD s=WS_VISIBLE|WS_POPUP;
	DWORD sex=WS_EX_TOPMOST;
#else
	DWORD s=WS_VISIBLE|WS_OVERLAPPED|WS_SYSMENU|WS_DLGFRAME|WS_MINIMIZEBOX|WS_CAPTION|WS_MAXIMIZEBOX;
	DWORD sex=0;
#endif

	RECT r={0,0,sr->v_size.cx,sr->v_size.cy};
	sr->pause_inactive = !(sr->gconfig->flags & EMUL_FLAGS_BACKGROUND_ACTIVE);
	sr->sync_update = (sr->gconfig->flags & EMUL_FLAGS_SYNC_UPDATE) != 0;
	AdjustWindowRectEx(&r,s,FALSE,0);
	puts("cr_thread");
	sr->video_w=CreateWindowEx(sex,(LPCTSTR)sr->video_at,get_system_name(sr),s,
		CW_USEDEFAULT,CW_USEDEFAULT,r.right-r.left,r.bottom-r.top,
		NULL/*sr->base_w*/,NULL,NULL,sr);
	if (!sr->video_w) {
		puts("createwindow error");
		return -1;
	}
/*	while (GetMessage(&msg,NULL,0,0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}*/
	return 0;
}

int init_video_window(struct SYS_RUN_STATE*sr)
{
	int r;
	sr->keymap = keymap_default;
	if (register_video_window(sr) < 0) return -1;
	keymap_load(sr->config->slots[CONF_KEYBOARD].cfgstr[CFG_STR_ROM], &sr->keymap);
	r = create_video_buffer(sr);
	switch (sr->cursystype) {
	case SYSTEM_7:
	case SYSTEM_9:
		sr->v_size.cx = 32 * CHAR_W;
		sr->v_size.cy = 32 * CHAR_H;
		break;
	default:
		sr->v_size.cx = 35 * CHAR_W;
		sr->v_size.cy = 24 * CHAR_H;
		break;
	}
/*	sr->h=CreateThread(NULL,0,cr_proc,sr,0,&sr->th);
	while (!sr->video_w) {
		if (WaitForSingleObject(sr->h, 100) != WAIT_TIMEOUT) {
			term_video_window(sr);
			return -1;
		}
	}
	puts("init_video_window ok");*/
	cr_proc(sr);
	return 0;
}


void toggle_debugger(struct SYS_RUN_STATE*sr)
{
	TCHAR lbuf[1024];
	MENUITEMINFO inf;

	if (!(sr->gconfig->flags & EMUL_FLAGS_ENABLE_DEBUGGER)) return;

	puts("toggle debugger");
	if (sr->debug_ptr) debug_detach(sr);
	else debug_attach(sr);

	localize_str(LOC_VIDEO, sr->debug_ptr?501:500, lbuf, sizeof(lbuf));
	inf.cbSize = sizeof(inf);
	inf.fMask = MIIM_TYPE;
	inf.fType = MF_STRING;
	inf.dwTypeData = lbuf;
	SetMenuItemInfo(sr->popup_menu,IDC_DEBUGGER,FALSE,&inf);
}

void set_video_size(struct SYS_RUN_STATE*sr, int w, int h)
{
	RECT r={0,0,w,h};
	if (sr->v_size.cx == w && sr->v_size.cy == h) return;
	sr->v_size.cx = w;
	sr->v_size.cy = h;
//	printf("set_video_size: %i %i\n", w, h);
	if (sr->fullscreen) {
		AdjustWindowRectEx(&r, sr->old_style,
			GetMenu(sr->video_w)?TRUE:FALSE,
			GetWindowLong(sr->video_w,GWL_EXSTYLE));
		sr->old_pl.rcNormalPosition.right = sr->old_pl.rcNormalPosition.left + r.right - r.left;
		sr->old_pl.rcNormalPosition.bottom = sr->old_pl.rcNormalPosition.top + r.bottom - r.top;
	} else {
		WINDOWPLACEMENT pl;
		pl.length = sizeof(pl);
		GetWindowPlacement(sr->video_w, &pl);
		AdjustWindowRectEx(&r,GetWindowLong(sr->video_w,GWL_STYLE),
			GetMenu(sr->video_w)?TRUE:FALSE,
			GetWindowLong(sr->video_w,GWL_EXSTYLE));
		pl.rcNormalPosition.right = pl.rcNormalPosition.left + r.right - r.left;
		pl.rcNormalPosition.bottom = pl.rcNormalPosition.top + r.bottom - r.top;
		SetWindowPlacement(sr->video_w, &pl);
	}
}

int term_video_window(struct SYS_RUN_STATE*sr)
{
	if (!sr->video_w) return 1;
	puts("free_video_window: entry");
	if (sr->input_data) {
		isclose(sr->input_data);
		sr->input_data = NULL;
		sr->input_size = sr->input_pos = 0;
	}
	DestroyWindow(sr->video_w);
//	WaitForSingleObject(sr->h, INFINITE);
//	CloseHandle(sr->h);
	free_video_buffer(sr);
	unregister_video_window(sr);
	puts("free_video_window: exit");
	return 0;
}


static void on_save_state(HWND w, struct SYS_RUN_STATE*sr)
{
	BOOL b = get_run_state_flags_by_ptr(sr) & RUNSTATE_SAVED;
	int r = 1;
	if (b) {
		TCHAR buf[1024], lbuf[256];
		wsprintf(buf, 
			localize_str(LOC_VIDEO, 0, lbuf, sizeof(lbuf)), //TEXT("Перезаписать сохранённое состояние системы «%s»?"), 
			sr->name);
		if (MessageBox(w, buf, 
			localize_str(LOC_VIDEO, 1, lbuf, sizeof(lbuf)), //TEXT("Перезапись состояния"),
			MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) != IDYES) return;
	}
	r = save_system_state_file(sr);
	if (r) {
		TCHAR buf[2][256];
		MessageBox(w, 
			localize_str(LOC_VIDEO, 2, buf[0], sizeof(buf[0])), //TEXT("При сохранении состояния системы возникла ошибка"), 
			localize_str(LOC_VIDEO, 3, buf[1], sizeof(buf[1])), //TEXT("Сохранение состояния"), 
			MB_ICONERROR);
	}
}

static void on_load_state(HWND w, struct SYS_RUN_STATE*sr)
{
	int r = 1;
	{
		TCHAR buf[1024], lbuf[256];
		wsprintf(buf, 
			localize_str(LOC_VIDEO, 10, lbuf, sizeof(lbuf)), //TEXT("Загрузить сохранённое состояние системы <%s>?"), 
			sr->name);
		if (MessageBox(w, buf, 
			localize_str(LOC_VIDEO, 11, lbuf, sizeof(lbuf)), //TEXT("Загрузка состояния"), 
			MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON1) != IDYES) return;
	}
	r = load_system_state_file(sr);
	if (r) {
		TCHAR buf[2][256];
		MessageBox(w,
			localize_str(LOC_VIDEO, 12, buf[0], sizeof(buf[0])), //TEXT("При загрузке состояния системы возникла ошибка"),
			localize_str(LOC_VIDEO, 13, buf[1], sizeof(buf[1])), //TEXT("Загрузка состояния"), 
			MB_ICONERROR);
	}
}

static void on_clear_state(HWND w, struct SYS_RUN_STATE*sr)
{
	TCHAR buf[1024], lbuf[256];
	int r;
	sprintf(buf, 
		localize_str(LOC_VIDEO, 20, lbuf, sizeof(lbuf)), //TEXT("Сбросить сохранённое состояние системы <%s>?"), 
		sr->name);
	if (MessageBox(w, buf, 
		localize_str(LOC_VIDEO, 21, lbuf, sizeof(lbuf)), //TEXT("Сброс состояния"), 
		MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) != IDYES) return;
	r = clear_system_state_file(sr);
	if (r) {
		TCHAR buf[2][256];
		MessageBox(w, 
			localize_str(LOC_VIDEO, 22, buf[0], sizeof(buf[0])), //TEXT("При сбросе состояния системы возникла ошибка"), 
			localize_str(LOC_VIDEO, 23, buf[1], sizeof(buf[1])), //TEXT("Сброс состояния"), 
			MB_ICONERROR);
	}
}

void set_fullscreen(struct SYS_RUN_STATE*sr, int fs)
{
	if (sr->fullscreen == fs) return;
//	printf("set fullscreen %i\n", fs);
	sr->fullscreen = fs;
	if (fs) {
		RECT r;
		sr->old_pl.length = sizeof(sr->old_pl);
		GetWindowPlacement(sr->video_w, &sr->old_pl);
		GetClientRect(GetDesktopWindow(), &r);
		sr->old_style = SetWindowLong(sr->video_w, GWL_STYLE, WS_OVERLAPPED | WS_VISIBLE);
		MoveWindow(sr->video_w, r.left, r.top, r.right-r.left, r.bottom-r.top, TRUE);
	} else {
		SetWindowLong(sr->video_w, GWL_STYLE, sr->old_style);
		SetWindowPlacement(sr->video_w, &sr->old_pl);
	}
	InvalidateRect(sr->video_w, NULL, TRUE);
}

static void calc_fullscr_params(struct SYS_RUN_STATE*sr, LPRECT d)
{
	RECT r, rb;
	int wl, hl, ww, wh;
	GetClientRect(sr->video_w, &r);
	ww = r.right-r.left;
	wh = r.bottom-r.top;
	wl = wh * 4 / 3;
//	printf("ww = %i, wh = %i, wl = %i\n", ww, wh, wl);
	if (wl > ww) {
		hl = ww * 3 / 4;
		wl = 0;
		hl = (wh - hl) / 2;
	} else {
		wl = (ww - wl) / 2;
		hl = 0;
	}
	d->left = r.left + wl;
	d->right = r.right - wl;
	d->top = r.top + hl;
	d->bottom = r.bottom - hl;
//	printf("d = %i,%i,%i,%i\n", d->left, d->top, d->right, d->bottom);
}

static void scr_to_full(struct SYS_RUN_STATE*sr, const LPRECT s, LPRECT d)
{
	RECT rw;
	double k1, k2;
	calc_fullscr_params(sr, &rw);
	k1 = (rw.right - rw.left) / (double)sr->v_size.cx;
	k2 = (rw.bottom - rw.top) / (double)sr->v_size.cy;
	d->left = rw.left + s->left * k1;
	d->top = rw.top + s->top * k2;
	d->right = rw.left + (s->right + 1) * k1 - 1;
	d->bottom = rw.top + (s->bottom + 1) * k2 - 1;
}

static void full_to_scr(struct SYS_RUN_STATE*sr, const LPRECT s, LPRECT d)
{
	RECT rw;
	double k1, k2;
	calc_fullscr_params(sr, &rw);
	k1 = (rw.right - rw.left) / (double)sr->v_size.cx;
	k2 = (rw.bottom - rw.top) / (double)sr->v_size.cy;
	d->left = (s->left - rw.left) / k1;
	d->top = (s->top - rw.top) / k2;
	d->right = (s->right - rw.left + 1) / k1 - 1;
	d->bottom = (s->bottom - rw.top + 1) / k2 - 1;
}


static byte xmem_read(word a, struct SYS_RUN_STATE*sr)
{
	if (a >= 0xC000 && a < 0xD000) return 0xFF;
	return mem_read(a, sr);
}

static void xmem_write(word a, byte b, struct SYS_RUN_STATE*sr)
{
	if (a >= 0xC000 && a < 0xD000) return;
	mem_write(a, b, sr);
}

int lock_mouse(struct SYS_RUN_STATE*sr)
{
	RECT r;
	if (sr->mouselocked) return 1;
	sr->mouselocked = 1;
	SetClassLongPtr(sr->video_w, GCL_HCURSOR, 0);
	ShowCursor(FALSE);
	GetClientRect(sr->video_w, &r);
	{
		POINT pt = {(r.right+r.left)/2, (r.bottom-r.top)/2};
		ClientToScreen(sr->video_w, &pt);
		SetCursorPos(pt.x, pt.y);
	}
	return 0;
}

int unlock_mouse(struct SYS_RUN_STATE*sr)
{
	RECT r;
	if (!sr->mouselocked) return 1;
	sr->mouselocked = 0;
	SetClassLongPtr(sr->video_w, GCL_HCURSOR, (LONG)LoadCursor(NULL,IDC_ARROW));
	ShowCursor(TRUE);
	GetClientRect(sr->video_w, &r);
	{
		POINT pt = {
			sr->xmousepos * (r.right - r.left) / 0xFFFF,
			sr->ymousepos * (r.bottom - r.top) / 0xFFFF
		};
		ClientToScreen(sr->video_w, &pt);
		SetCursorPos(pt.x, pt.y);
	}
	return 0;
}


int dump_mem(struct SYS_RUN_STATE*sr, int start, int size, const char*fname)
{
	FILE*f = fopen(fname, "wb");
	if (!f) return -1;
	for (;size; --size, ++start) {
		byte b = xmem_read(start, sr);
		if (fwrite(&b, 1, 1, f) != 1) {
			fclose(f);
			return -2;
		}
	}
	fclose(f);
	return 0;
}


int read_dump_mem(struct SYS_RUN_STATE*sr, int start, int size, const char*fname)
{
	FILE*f = fopen(fname, "rb");
	int n = 0;
	if (!f) return -1;
	if (!size) {
		for (; !feof(f); ++start) {
			byte b;
			int r;
			r = fread(&b, 1, 1, f);
			if (r < 0) {
				fclose(f);
				return -2;
			}
			if (!r) break;
			xmem_write(start, b, sr);
			++ n;
		}
	} else {
		for (;size; --size, ++start) {
			byte b;
			if (fread(&b, 1, 1, f) != 1) {
				fclose(f);
				return -2;
			}
			xmem_write(start, b, sr);
			++ n;
		}
	}
	fclose(f);
	return n;
}

int on_input_file(HWND w, struct SYS_RUN_STATE*sr)
{
	TCHAR bufs[2][1024];
	TCHAR fname[CFGSTRLEN] = "";
	ISTREAM*in;
	int r;
	if (sr->input_data) {
		isclose(sr->input_data);
		sr->input_data = NULL;
		sr->input_size = sr->input_pos = 0;
	}
	switch (MessageBox(w,
		localize_str(LOC_VIDEO, 202, bufs[0], sizeof(bufs[0])),
		localize_str(LOC_VIDEO, 201, bufs[1], sizeof(bufs[1])),
		MB_ICONQUESTION | MB_YESNOCANCEL)) {
	case IDYES:
		sr->input_recode = 1;
		break;
	case IDNO:
		sr->input_recode = 0;
		break;
	case IDCANCEL:
		return 1;
	}
	r = select_open_text(w, fname);
	if (!r) return 2;
	in = isfopen(fname);
	if (!in) {
		MessageBox(w, localize_str(LOC_GENERIC, 2, bufs[0], sizeof(bufs[0])), NULL, 0);
		return -1;
	}
	isseek(in, 0, SSEEK_END);
	sr->input_size = istell(in);
	isseek(in, 0, SSEEK_SET);
	sr->input_data = in;
	sr->input_cntr = 0;
	return 0;
}

int cancel_input_file(struct SYS_RUN_STATE*sr)
{
	if (!sr->input_data) return 0;
	isclose(sr->input_data);
	sr->input_data = NULL;
	sr->input_size = sr->input_pos = 0;
	system_command(sr, SYS_COMMAND_SET_STATUS_TEXT, -1, 0);
	return 0;
}

static byte translate_capslock(struct SYS_RUN_STATE*sr, byte k)
{
	if (sr->config->systype == SYSTEM_8A) {
		int sh = sr->caps_lock;
		k |= 0x80;
		if (is_keyb_english(sr)) {
			if (k >= 0xE0 && k < 0xFF) { k -= 0x20; }
			else sh = !sh;
		} else {
			if (k >= 0xC0 && k < 0xE0) { k += 0x20; sh = !sh; }
		}
		if (sh) k |= 0x80;
		else if (k >= 0xC0) k &= 0x7F;
		return k;
	}
	if (!sr->caps_lock || sr->cursystype != SYSTEM_E) return k;
	if ((k >> 5) == 2) k |= 0x20;
	else if ((k >> 5) == 3) k &= ~0x20;
	return k;
}

static void apply_capslock(struct SYS_RUN_STATE*sr)
{
}

static void check_capslock(HWND w, struct SYS_RUN_STATE*sr)
{
	if (GetKeyState(VK_CAPITAL) & 1) {
		sr->caps_lock = 1;
	} else {
		sr->caps_lock = 0;
	}
	apply_capslock(sr);
}

static void copy_clipboard(struct SYS_RUN_STATE*sr)
{
	HDC enh, src;
	HBITMAP bmp, lbm;

	src = GetDC(sr->video_w);

	enh = CreateCompatibleDC(src);
	if (!enh) {
		ReleaseDC(sr->video_w, src);
		return;
	}
	bmp = CreateCompatibleBitmap(src, sr->v_size.cx, sr->v_size.cy);
	if (!bmp) {
		ReleaseDC(sr->video_w, src);
		return;
	}
	ReleaseDC(sr->video_w, src);
	lbm = SelectObject(enh, bmp);
	if (!BitBlt(enh, 0, 0, sr->v_size.cx, sr->v_size.cy, sr->mem_dc, 0, 0, SRCCOPY)) {
		SelectObject(enh, lbm);
		DeleteObject(bmp);
		DeleteDC(enh);
		return;
	}
	SelectObject(enh, lbm);
	DeleteDC(enh);

	if (!OpenClipboard(sr->video_w)) { MessageBeep(MB_ICONEXCLAMATION); return; }
	if (!EmptyClipboard()) {
		CloseClipboard();
		MessageBeep(MB_ICONEXCLAMATION);
		return;
	}
	if (!SetClipboardData(CF_BITMAP, bmp)) {
		CloseClipboard();
		MessageBeep(MB_ICONEXCLAMATION);
		return;
	}
	CloseClipboard();
	puts("copy ok");
}


LRESULT CALLBACK wnd_proc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	struct SYS_RUN_STATE*sr = (struct SYS_RUN_STATE*)GetWindowLongPtr(w, GWL_USERDATA);
	switch (msg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT cs = (LPCREATESTRUCT)lp;
			HMENU m;
			TCHAR lbuf[256];
			sr = (struct SYS_RUN_STATE*)cs->lpCreateParams;
			SetWindowLongPtr(w, GWL_USERDATA, (LONG)sr);
			puts("wnd_proc createwindow");
#ifdef UNDER_CE
			m=sr->popup_menu=CreatePopupMenu();
#else
			m=sr->popup_menu=GetSystemMenu(w,FALSE);
			if (!m) 
				m=sr->popup_menu=CreatePopupMenu();
			else
				AppendMenu(m,MF_SEPARATOR,0,0);
#endif
			AppendMenu(m,MF_STRING,IDC_RESET,
				localize_str(LOC_VIDEO, 100, lbuf, sizeof(lbuf))); //TEXT("&Сброс\tCtrl+Break"));
			AppendMenu(m,MF_STRING,IDC_HRESET,
				localize_str(LOC_VIDEO, 101, lbuf, sizeof(lbuf))); //TEXT("&Полный сброс"));
//			AppendMenu(m,MF_STRING,IDC_IRQ,
//				localize_str(LOC_VIDEO, 102, lbuf, sizeof(lbuf))); //TEXT("IRQ"));
//			AppendMenu(m,MF_STRING,IDC_NMI,
//				localize_str(LOC_VIDEO, 103, lbuf, sizeof(lbuf))); //TEXT("NMI"));
			AppendMenu(m,MF_SEPARATOR,0,0);
			AppendMenu(m,MF_STRING,IDC_SAVESTATE,
				localize_str(LOC_VIDEO, 110, lbuf, sizeof(lbuf))); //TEXT("Сохранить состояние..."));
			AppendMenu(m,MF_STRING,IDC_LOADSTATE,
				localize_str(LOC_VIDEO, 111, lbuf, sizeof(lbuf))); //TEXT("Загрузить состояние..."));
			AppendMenu(m,MF_STRING,IDC_CLEARSTATE,
				localize_str(LOC_VIDEO, 112, lbuf, sizeof(lbuf))); //TEXT("Сбросить состояние..."));
			AppendMenu(m,MF_SEPARATOR,0,0);

			if (sr->cursystype != SYSTEM_AA) {
				AppendMenu(m,MF_STRING,IDC_INPUT_FILE,
					localize_str(LOC_VIDEO, 200, lbuf, sizeof(lbuf))); //TEXT("Ввод из текстового файла...\tF6"));
			}
			AppendMenu(m,MF_STRING,IDC_COPY,
				localize_str(LOC_VIDEO, 300, lbuf, sizeof(lbuf))); //TEXT("Копирование в буфер обмена\tF7"));
			AppendMenu(m,MF_SEPARATOR,0,0);
		if (sr->gconfig->flags & EMUL_FLAGS_ENABLE_DEBUGGER) {
			AppendMenu(m,MF_STRING,IDC_DEBUGGER,
				localize_str(LOC_VIDEO, 500, lbuf, sizeof(lbuf))); //TEXT("Запустить отладчик\tF8"));
			AppendMenu(m,MF_SEPARATOR,0,0);
		}
#ifdef UNDER_CE
			AppendMenu(m,MF_SEPARATOR,0,0);
			AppendMenu(m,MF_STRING,IDCLOSE,
				localize_str(LOC_VIDEO, 120, lbuf, sizeof(lbuf))); //TEXT("&Закрыть"));
#endif
			SetTimer(w,TID_FLASH,FLASH_INTERVAL,NULL);
			check_capslock(w, sr);
		}
		break;
	case WM_TIMER:
		if (sr->cursystype == SYSTEM_E) update_alt_state(sr);
		switch (wp) {
		case TID_FLASH:
			system_command(sr, SYS_COMMAND_FLASH, 0, 0);
			break;
		case TID_IRQ:
			system_command(sr, SYS_COMMAND_IRQ, 0, 0);
			break;
		case TID_NMI:
			system_command(sr, SYS_COMMAND_NMI, 0, 0);
			break;
		}
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC dc;
			dc=BeginPaint(w,&ps);
			if (sr->fullscreen) {
				RECT r, rb, rw;
				HBRUSH bb = GetStockObject(BLACK_BRUSH);
				GetClientRect(w, &rw);
				calc_fullscr_params(sr, &r);
				if (r.left) {
					rb.left = rw.left;
					rb.top = rw.top;
					rb.right = r.left;
					rb.bottom = rw.bottom;
					FillRect(dc, &rb, bb);
					rb.left = r.right;
					rb.right = rw.right;
					FillRect(dc, &rb, bb);
				}
				if (r.top) {
					rb.left = r.left;
					rb.top = rw.top;
					rb.right = r.right;
					rb.bottom = r.top;
					FillRect(dc, &rb, bb);
					rb.top = r.bottom;
					rb.bottom = rw.bottom;
					FillRect(dc, &rb, bb);
				}
				SetStretchBltMode(dc, COLORONCOLOR);
				StretchBlt(dc,r.left,r.top,r.right-r.left,r.bottom-r.top,
					sr->mem_dc,0,0,sr->v_size.cx,sr->v_size.cy,SRCCOPY);
			} else {
				BitBlt(dc,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right-ps.rcPaint.left,ps.rcPaint.bottom-ps.rcPaint.top,sr->mem_dc,ps.rcPaint.left,ps.rcPaint.top,SRCCOPY);
			}
			EndPaint(w,&ps);
			return 0;
		}
		break;
	case WM_CLOSE:
		{
			struct SYSCONFIG*c = sr->config;
			if (sr->fullscreen) set_fullscreen(sr, 0);
			UpdateWindow(w);
			free_system_state(sr);
			free_config(c);
		}	
		break;
	case WM_MOUSEACTIVATE:
		if (LOWORD(lp) == HTCLIENT && sr->mouselock) lock_mouse(sr);
		break;
	case WM_ACTIVATE:
		check_capslock(w, sr);
		switch (wp) {
		case WA_CLICKACTIVE:
			break;
		case WA_INACTIVE:
			unlock_mouse(sr);
			break;
		}
		if (sr->pause_inactive) {
			switch (wp) {
			case WA_CLICKACTIVE:
			case WA_ACTIVE:
				system_command(sr, SYS_COMMAND_START, 0, 0);
				break;
			case WA_INACTIVE:
				system_command(sr, SYS_COMMAND_STOP, 0, 0);
				break;
			}
		}
		if (wp == WA_INACTIVE && sr->fullscreen) {
			set_fullscreen(sr, 0);
		}
		break;
	case WM_HELP:
		return TRUE;
	case WM_SYSKEYDOWN:
		switch (wp) {
		case VK_F12: {
			extern int cpu_debug;
			cpu_debug = !cpu_debug;
			dump_mem(sr, 0, 0x10000, "debug.bin");
			} break;
		case VK_F11:
			dump_mem(sr, 0, 0x10000, "memdump.bin");
			break;
		case VK_F10:
			if (GetKeyState(VK_MENU)&0x8000) {
			RECT r;
			GetWindowRect(w, &r);
			TrackPopupMenu(sr->popup_menu,TPM_CENTERALIGN,
				(r.left + r.right) / 2, (r.bottom + r.top) / 2,
				0,w,NULL);
			} break;
		case VK_F4:
			goto def;
		case VK_RETURN:
			if (GetKeyState(VK_MENU)&0x8000) {
				set_fullscreen(sr, !sr->fullscreen);
				return 0;
			}
			break;
		case VK_PAUSE:
/*			if (GetKeyState(VK_CONTROL)&0x8000) {
				system_command(sr, SYS_COMMAND_RESET, 0, 0);
			} else*/ {
				cancel_input_file(sr);
				system_command(sr, SYS_COMMAND_STOP, 0, 0);
				system_command(sr, SYS_COMMAND_HRESET, 0, 0);
				system_command(sr, SYS_COMMAND_START, 0, 0);
			}
			break;
		}
	case WM_KEYDOWN:
		if (lp & 0x40000000) ++sr->key_rept;
		system_command(sr, SYS_COMMAND_WAKEUP, 0, 0);
//		printf("key_down: %X %X\n", wp, lp);
		if (sr->gconfig->flags & EMUL_FLAGS_LANGSEL) {
			if (wp == VK_SHIFT && !(lp&0x40000000)) {
				if (GetKeyState(VK_CONTROL)&0x8000) sr->cur_lang = !sr->cur_lang;
			} else if (wp == VK_CONTROL && !(lp&0x40000000)) {
				if (GetKeyState(VK_SHIFT)&0x8000) sr->cur_lang = !sr->cur_lang;
			}
			sr->keyreg = is_keyb_english(sr)?0xFF:0x7F;
		}
		{
			int d;
			if (sr->cursystype == SYSTEM_E) update_alt_state(sr);
#ifdef KEY_SCANCODES
			d=decode_key(lp, &sr->keymap, !is_keyb_english(sr));
#else
			d=decode_key(wp, &sr->keymap, !is_keyb_english(sr));
#endif
			if (d && d != -1) d = translate_capslock(sr, d);

		switch (wp) {
		case VK_CAPITAL:
			check_capslock(w, sr);
			break;
		case VK_SCROLL:
			if (GetKeyState(wp)&1) {
				system_command(sr, SYS_COMMAND_STOP, 0, 0);
			} else {
				system_command(sr, SYS_COMMAND_START, 0, 0);
			}	
			break;
		case VK_ESCAPE:
			cancel_input_file(sr);
			break;
		case VK_F4:
			system_command(sr, SYS_COMMAND_TOGGLE_MONO, 0, 0);
			break;
		case VK_F5:
			unlock_mouse(sr);
			break;
		case VK_F6:
			on_input_file(w, sr);
			break;
		case VK_F7:
			copy_clipboard(sr);
			break;
		case VK_F8:
			toggle_debugger(sr);
			break;
		case VK_APPS:
			system_command(sr, SYS_COMMAND_FAST, 1, 0);
			break;
		}
//			printf("d=%x\n",d);
			if (wp == VK_CAPITAL || wp == VK_INSERT || wp == VK_DELETE || wp == VK_BACK)
				sr->key_down = wp;
			if (d) {
				sr->input_hbit = d>>7;
				sr->cur_key = d | 0x80;
				sr->key_down = wp;
				if (!sr->sync_update) {
					InvalidateRect(w,NULL,FALSE);
				}
			}
			if (d==-1) {
				sr->cur_key = 0;
				system_command(sr, SYS_COMMAND_RESET, 0, 0);
				if (GetKeyState(VK_MENU)&0x8000) {
					cancel_input_file(sr);
					system_command(sr, SYS_COMMAND_STOP, 0, 0);
					system_command(sr, SYS_COMMAND_HRESET, 0, 0);
					system_command(sr, SYS_COMMAND_START, 0, 0);
				} else {
					cancel_input_file(sr);
					system_command(sr, SYS_COMMAND_RESET, 0, 0);
				}
			}
		}
		break;
	case WM_KEYUP:
		system_command(sr, SYS_COMMAND_WAKEUP, 0, 0);
		sr->key_rept = 0;
		if (sr->cursystype == SYSTEM_E) update_alt_state(sr);
		switch (wp) {
		case VK_APPS:
			system_command(sr, SYS_COMMAND_FAST, 0, 0);
			break;
		}
		break;
	case WM_LBUTTONDOWN:
		if (sr->mouselock) lock_mouse(sr);
		sr->mousebtn|=1;
		sr->mousechanged|=1;
		system_command(sr, SYS_COMMAND_WAKEUP, 0, 0);
		system_command(sr, SYS_COMMAND_MOUSE_EVENT, 1, 0);
		
/*		if (GetKeyState(VK_MENU)) 
			TrackPopupMenu(popup_menu,0,
				(short)LOWORD(lp),(short)HIWORD(lp),
				0,w,NULL);*/
		break;
	case WM_RBUTTONDOWN:
		if (sr->mouselock) lock_mouse(sr);
		sr->mousebtn|=2;
		sr->mousechanged|=2;
		system_command(sr, SYS_COMMAND_WAKEUP, 0, 0);
		system_command(sr, SYS_COMMAND_MOUSE_EVENT, 2, 0);
		break;
	case WM_LBUTTONUP:
		sr->mousebtn&=~1;
		sr->mousechanged|=1;
		system_command(sr, SYS_COMMAND_WAKEUP, 0, 0);
		system_command(sr, SYS_COMMAND_MOUSE_EVENT, 1, 0);
		break;
	case WM_NCLBUTTONDBLCLK:
		if (wp == HTCAPTION) {
			set_fullscreen(sr, 1);
			return 0;
		}
		break;
	case WM_RBUTTONUP:
		sr->mousebtn&=~2;
		sr->mousechanged|=2;
		system_command(sr, SYS_COMMAND_WAKEUP, 0, 0);
		system_command(sr, SYS_COMMAND_MOUSE_EVENT, 2, 0);
		break;
	case WM_DESTROY:
		if (sr->base_w) {
			WINDOWPLACEMENT wp;
			wp.length = sizeof(wp);
			GetWindowPlacement(sr->base_w, &wp);
			if (wp.showCmd == SW_SHOWMINIMIZED) ShowWindow(sr->base_w, SW_RESTORE);
//			ShowWindow(sr->base_w, SW_SHOWNORMAL);
			BringWindowToTop(sr->base_w);
//			SetForegroundWindow(sr->base_w);
//			SetActiveWindow(sr->base_w);
		} else PostQuitMessage(0);
		break;
	case WM_ENTERMENULOOP:
		{
			BOOL b = get_run_state_flags_by_ptr(sr) & RUNSTATE_SAVED;
			EnableMenuItem(sr->popup_menu, IDC_LOADSTATE, (b?MF_ENABLED:MF_GRAYED) | MF_BYCOMMAND);
			EnableMenuItem(sr->popup_menu, IDC_CLEARSTATE, (b?MF_ENABLED:MF_GRAYED) | MF_BYCOMMAND);
			system_command(sr, SYS_COMMAND_UPDMENU, 0, (long)sr->popup_menu);
		}
		break;
	case WM_COMMAND: case WM_SYSCOMMAND:
		switch (LOWORD(wp)) {
		case IDC_RESET:
			cancel_input_file(sr);
			system_command(sr, SYS_COMMAND_RESET, 0, 0);
			break;
		case IDC_IRQ:
			system_command(sr, SYS_COMMAND_IRQ, 0, 0);
			break;
		case IDC_NMI:
			system_command(sr, SYS_COMMAND_NMI, 0, 0);
			break;
		case IDC_HRESET:
			cancel_input_file(sr);
			system_command(sr, SYS_COMMAND_STOP, 0, 0);
			system_command(sr, SYS_COMMAND_HRESET, 0, 0);
			system_command(sr, SYS_COMMAND_START, 0, 0);
			break;
		case IDCLOSE:
			PostMessage(w,WM_CLOSE,0,0);
			break;
		case IDC_SAVESTATE:
			on_save_state(w, sr);
			break;
		case IDC_DEBUGGER:
			toggle_debugger(sr);
			break;
		case IDC_LOADSTATE:
			on_load_state(w, sr);
			break;
		case IDC_CLEARSTATE:
			on_clear_state(w, sr);
			break;
		case SC_MAXIMIZE:
			set_fullscreen(sr, 1);
			return 0;
		case IDC_INPUT_FILE:
			on_input_file(w, sr);
			break;
		case IDC_COPY:
			copy_clipboard(sr);
			break;
		default:
			system_command(sr, SYS_COMMAND_WINCMD, LOWORD(wp), (long)w);
			break;
		}
		break;
	case WM_MOUSEMOVE:
		{
			RECT r;
			int x, y;
			GetClientRect(w, &r);
			x=((long)(short)LOWORD(lp))*0xFFFF/(r.right-r.left);
			y=((long)(short)HIWORD(lp))*0xFFFF/(r.bottom-r.top);
			if (sr->mouselocked) {
				POINT pt = {(r.right+r.left)/2, (r.bottom-r.top)/2};
				int lx = pt.x*0xFFFF/(r.right-r.left);
				int ly = pt.y*0xFFFF/(r.bottom-r.top);
				sr->dxmouse = x - lx;
				sr->dymouse = y - ly;
				sr->xmousepos += sr->dxmouse;
				sr->ymousepos += sr->dymouse;
				if (sr->xmousepos > 0xFFFF) sr->xmousepos = 0xFFFF;
				if (sr->ymousepos > 0xFFFF) sr->ymousepos = 0xFFFF;
				if (sr->xmousepos < 0) sr->xmousepos = 0;
				if (sr->ymousepos < 0) sr->ymousepos = 0;
				if ((pt.x != (short)LOWORD(lp)) || (pt.y != (short)HIWORD(lp))) {
					ClientToScreen(w, &pt);
					SetCursorPos(pt.x, pt.y);
				}
			} else {
				sr->dxmouse = x - sr->xmousepos;
				sr->dymouse = y - sr->ymousepos;
				sr->xmousepos = x;
				sr->ymousepos = y;
			}
			sr->xmouse += sr->dxmouse;
			sr->ymouse += sr->dymouse;
			sr->mousechanged|=0x80;
//			system_command(sr, SYS_COMMAND_WAKEUP, 0, 0);
			system_command(sr, SYS_COMMAND_MOUSE_EVENT, 0x80, 0);
//			printf("x=%i, y=%i\n", sr->xmousepos, sr->ymousepos);
		}
		break;
	case MM_WOM_DONE:
		system_command(sr, SYS_COMMAND_SOUND_DONE, wp, lp);
		return 0;
	case WM_INPUTLANGCHANGE:
//		printf("%x\n",lp);
		if (!(sr->gconfig->flags & EMUL_FLAGS_LANGSEL)) {
			sr->keyreg = is_keyb_english(sr)?0xFF:0x7F;
		}	
		break;
	}
def:
	return DefWindowProc(w,msg,wp,lp);
}

int invalidate_video_window(struct SYS_RUN_STATE*sr, RECT *r)
{
	if (sr->fullscreen && r) {
		RECT r1;
		scr_to_full(sr, r, &r1);
		InvalidateRect(sr->video_w, &r1, FALSE);
	} else {
		InvalidateRect(sr->video_w, r, FALSE);
	}	
	return 0;
}

void enable_ints(struct SYS_RUN_STATE*sr)
{
	if (sr->ints_enabled) return;
//	system_command(sr, SYS_COMMAND_DUMPCPUREGS, 0, 0);
	sr->ints_enabled = 1;
//	puts("enable ints");
//	SetTimer(sr->video_w,TID_IRQ,2,NULL);
//	SetTimer(sr->video_w,TID_NMI,20,NULL);
//	system_command(sr, SYS_COMMAND_IRQ, 0, 0);
//	system_command(sr, SYS_COMMAND_NMI, 0, 0);
}

void disable_ints(struct SYS_RUN_STATE*sr)
{
	if (!sr->ints_enabled) return;
	sr->ints_enabled = 0;
//	puts("disable ints");
//	KillTimer(sr->video_w,TID_IRQ);
//	KillTimer(sr->video_w,TID_NMI);
}

void toggle_ints(struct SYS_RUN_STATE*sr)
{
	if (sr->ints_enabled) 
		disable_ints(sr);
	else
		enable_ints(sr);
}


byte keyb_is_pressed(struct SYS_RUN_STATE*sr, int vk)
{
	return (GetKeyState(vk)&0x8000)?0x80:0x00;
}

byte keyb_apple_state(struct SYS_RUN_STATE*sr, int lr)
{
	int keys[2] = {VK_LMENU, VK_RMENU};
	printf("apple_state[%i]\n", lr);
	return keyb_is_pressed(sr, keys[lr]);
}
