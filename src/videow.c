#include <windows.h>
#include "resource.h"

#include "runmgr.h"
#include "memory.h"
#include "runmgrint.h"
#include "videow.h"
#include "runstate.h"

#include "localize.h"
#include <stdio.h>

#define VIDEO_CLASS TEXT("agat_video")

#define MAX_VIDEO_W (40*CHAR_W)
#define MAX_VIDEO_H (32*CHAR_H)


static LRESULT CALLBACK wnd_proc(HWND w,UINT msg,WPARAM wp,LPARAM lp);

static ATOM at;

WORD get_keyb_language()
{
	return PRIMARYLANGID(LOWORD(GetKeyboardLayout(0)));
}

int register_video_window()
{
	WNDCLASS cl;
	ZeroMemory(&cl,sizeof(cl));
	cl.hbrBackground=NULL;//CreateSolidBrush(RGB(0,0,0));
	cl.lpfnWndProc=wnd_proc;
	cl.lpszClassName=VIDEO_CLASS;
	cl.hCursor=LoadCursor(NULL,IDC_ARROW);
	cl.hIcon=LoadIcon(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_MAIN));
	at=RegisterClass(&cl);
	if (!at) return -1;
	return 0;
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
	SelectObject(sr->mem_dc, sr->old_bmp);
	DeleteObject(sr->mem_bmp);
	DeleteDC(sr->mem_dc);
	sr->bmp_bits = NULL;
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
	DWORD s=WS_VISIBLE|WS_OVERLAPPED|WS_SYSMENU|WS_DLGFRAME|WS_MINIMIZEBOX|WS_CAPTION;
	DWORD sex=0;
#endif
	RECT r={0,0,sr->v_size.cx,sr->v_size.cy};
	AdjustWindowRectEx(&r,s,FALSE,0);
	puts("cr_thread");
	sr->video_w=CreateWindowEx(sex,(LPCTSTR)at,get_system_name(sr),s,
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
	keymap_load(sr->config->slots[CONF_KEYBOARD].cfgstr[CFG_STR_ROM], &sr->keymap);
	r = create_video_buffer(sr);
	switch (sr->cursystype) {
	case SYSTEM_7:
	case SYSTEM_9:
		sr->v_size.cx = 32 * CHAR_W;
		sr->v_size.cy = 32 * CHAR_H;
		break;
	case SYSTEM_A:
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
	DestroyWindow(sr->video_w);
	WaitForSingleObject(sr->h, INFINITE);
	CloseHandle(sr->h);
	free_video_buffer(sr);
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

static void set_fullscreen(struct SYS_RUN_STATE*sr, int fs)
{
	if (sr->fullscreen == fs) return;
	printf("set fullscreen %i\n", fs);
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


static void scr_to_full(struct SYS_RUN_STATE*sr, const LPRECT s, LPRECT d)
{
	RECT rw;
	double k1, k2;
	GetClientRect(sr->video_w, &rw);
	k1 = (rw.right - rw.left) / (double)sr->v_size.cx;
	k2 = (rw.bottom - rw.top) / (double)sr->v_size.cy;
	d->left = s->left * k1;
	d->top = s->top * k2;
	d->right = (s->right + 1) * k1 - 1;
	d->bottom = (s->bottom + 1) * k2 - 1;
}

static void full_to_scr(struct SYS_RUN_STATE*sr, const LPRECT s, LPRECT d)
{
	RECT rw;
	double k1, k2;
	GetClientRect(sr->video_w, &rw);
	k1 = (rw.right - rw.left) / (double)sr->v_size.cx;
	k2 = (rw.bottom - rw.top) / (double)sr->v_size.cy;
	d->left = s->left / k1;
	d->top = s->top / k2;
	d->right = (s->right + 1) / k1 - 1;
	d->bottom = (s->bottom + 1) / k2 - 1;
}


static byte xmem_read(word a, struct SYS_RUN_STATE*sr)
{
	if (a >= 0xC000 && a < 0xD000) return 0xFF;
	return mem_read(a, sr);
}

int dump_mem(struct SYS_RUN_STATE*sr, int start, int size, const char*fname)
{
	FILE*f = fopen(fname, "wb");
	if (!f) return -1;
	for (;size; --size, ++start) {
		byte b = xmem_read(start, sr);
		fwrite(&b, 1, 1, f);
	}
	fclose(f);
	return 0;
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
#ifdef UNDER_CE
			AppendMenu(m,MF_SEPARATOR,0,0);
			AppendMenu(m,MF_STRING,IDCLOSE,
				localize_str(LOC_VIDEO, 120, lbuf, sizeof(lbuf))); //TEXT("&Закрыть"));
#endif
			SetTimer(w,TID_FLASH,FLASH_INTERVAL,NULL);
		}
		break;
	case WM_TIMER:
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
				RECT r;
				GetClientRect(w, &r);
				SetStretchBltMode(dc, COLORONCOLOR);
//				printf("paint rect %i,%i -> %i,%i\n", ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom);
//				full_to_scr(sr, &ps.rcPaint, &r);
//				StretchBlt(dc,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right-ps.rcPaint.left,ps.rcPaint.bottom-ps.rcPaint.top,
//					sr->mem_dc,r.left,r.top,r.right-r.left,r.bottom-r.top,SRCCOPY);
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
		if (sr->fullscreen) set_fullscreen(sr, 0);
		UpdateWindow(w);
		system_command(sr, SYS_COMMAND_STOP, 0, 0);
		free_config(sr->config);
		free_system_state(sr);
		break;
	case WM_ACTIVATE:
		if (sr->pause_inactive) {
			switch (wp) {
			case WA_ACTIVE:
			case WA_CLICKACTIVE:
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
		case VK_F12: { extern int cpu_debug; cpu_debug = !cpu_debug; } break;
		case VK_F11:
			dump_mem(sr, 0, 0x10000, "memdump.bin");
			break;
		case VK_RETURN:
			if (GetKeyState(VK_MENU)&0x8000) {
				set_fullscreen(sr, !sr->fullscreen);
				return 0;
			}
			break;
		}
		break;
	case WM_KEYDOWN:
		{
			int d;
#ifdef KEY_SCANCODES
			d=decode_key(lp, &sr->keymap, get_keyb_language() == LANG_RUSSIAN);
#else
			d=decode_key(wp, &sr->keymap, get_keyb_language() == LANG_RUSSIAN);
#endif

		switch (wp) {
		case VK_F4:
			system_command(sr, SYS_COMMAND_TOGGLE_MONO, 0, 0);
			break;
		case VK_APPS:
			system_command(sr, SYS_COMMAND_FAST, 1, 0);
			break;
		}
//			printf("d=%x\n",d);
			if (d) {
				sr->cur_key=d|0x80;
#ifndef SYNC_SCREEN_UPDATE
				InvalidateRect(w,NULL,FALSE);
#endif
			}
			if (d==-1) {
				sr->cur_key = 0;
				system_command(sr, SYS_COMMAND_RESET, 0, 0);
			}
		}
		break;
	case WM_KEYUP:
		switch (wp) {
		case VK_APPS:
			system_command(sr, SYS_COMMAND_FAST, 0, 0);
			break;
		}
		break;
	case WM_LBUTTONDOWN:
		SetCapture(w);
		ShowCursor(FALSE);
		sr->mousebtn|=1;
/*		if (GetKeyState(VK_MENU)) 
			TrackPopupMenu(popup_menu,0,
				(short)LOWORD(lp),(short)HIWORD(lp),
				0,w,NULL);*/
		break;
	case WM_RBUTTONDOWN:
		SetCapture(w);
		ShowCursor(FALSE);
		sr->mousebtn|=2;
		break;
	case WM_LBUTTONUP:
		ShowCursor(TRUE);
		ReleaseCapture();
		sr->mousebtn&=~1;
		break;
	case WM_RBUTTONUP:
		ShowCursor(TRUE);
		ReleaseCapture();
		sr->mousebtn&=~2;
		break;
	case WM_DESTROY:
		if (sr->base_w) {
			ShowWindow(sr->base_w, SW_SHOWNORMAL);
			SetActiveWindow(sr->base_w);
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
			system_command(sr, SYS_COMMAND_RESET, 0, 0);
			break;
		case IDC_IRQ:
			system_command(sr, SYS_COMMAND_IRQ, 0, 0);
			break;
		case IDC_NMI:
			system_command(sr, SYS_COMMAND_NMI, 0, 0);
			break;
		case IDC_HRESET:
			system_command(sr, SYS_COMMAND_HRESET, 0, 0);
			break;
		case IDCLOSE:
			PostMessage(w,WM_CLOSE,0,0);
			break;
		case IDC_SAVESTATE:
			on_save_state(w, sr);
			break;
		case IDC_LOADSTATE:
			on_load_state(w, sr);
			break;
		case IDC_CLEARSTATE:
			on_clear_state(w, sr);
			break;
		default:
			system_command(sr, SYS_COMMAND_WINCMD, LOWORD(wp), (long)w);
			break;
		}
		break;
	case WM_MOUSEMOVE:
		{
			RECT r;
			GetClientRect(w, &r);
			sr->xmousepos=((long)(short)LOWORD(lp))*0xFFFF/(r.right-r.left);
			sr->ymousepos=((long)(short)HIWORD(lp))*0xFFFF/(r.bottom-r.top);
//			printf("x=%i, y=%i\n", xmousepos, ymousepos);
		}
		break;
	case WM_INPUTLANGCHANGE:
//		printf("%x\n",lp);
		sr->keyreg = (PRIMARYLANGID(LOWORD(lp))==LANG_RUSSIAN) ? 0x7F: 0xFF;
		break;
	}
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

