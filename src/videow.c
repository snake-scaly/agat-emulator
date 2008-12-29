#include <windows.h>
#include "resource.h"

#include "runmgr.h"
#include "memory.h"
#include "runmgrint.h"
#include "videow.h"
#include "keyb.h"
#include "runstate.h"

#define VIDEO_CLASS TEXT("agat_video")

#define MAX_VIDEO_W (40*CHAR_W)
#define MAX_VIDEO_H (32*CHAR_H)


static LRESULT CALLBACK wnd_proc(HWND w,UINT msg,WPARAM wp,LPARAM lp);

static ATOM at;


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

int create_video_buffer(struct SYS_RUN_STATE*sr)
{
	struct {
		BITMAPINFOHEADER h;
		RGBQUAD colors[16];
	} bi;
	int cl;
	ZeroMemory(&bi,sizeof(bi));
	bi.h.biSize=sizeof(bi.h);
	bi.h.biWidth=MAX_VIDEO_W;
	bi.h.biHeight=-MAX_VIDEO_H;
	bi.h.biPlanes=1;
	bi.h.biBitCount=4;
	bi.h.biCompression=BI_RGB;
	bi.h.biSizeImage=0;
	sr->bmp_pitch=bi.h.biWidth>>1;
	for (cl=0;cl<16;cl++) {
	int v=(cl&8)?255:128;
		bi.colors[cl].rgbRed=(cl&1)?v:0;
		bi.colors[cl].rgbGreen=(cl&2)?v:0;
		bi.colors[cl].rgbBlue=(cl&4)?v:0;
	}
	sr->mem_dc=CreateCompatibleDC(NULL);
	if (!sr->mem_dc) return -3;
	sr->mem_bmp=CreateDIBSection(NULL,(BITMAPINFO*)&bi,DIB_RGB_COLORS,&sr->bmp_bits,NULL,0);
	if (!sr->mem_bmp) return -2;
	if (!sr->bmp_bits) return -4;
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
	return sysnames[sr->config->systype];
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
		sr->base_w,NULL,NULL,sr);
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
	r = create_video_buffer(sr);
	sr->v_size.cx = 32 * CHAR_W;
	sr->v_size.cy = 32 * CHAR_H;

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
	WINDOWPLACEMENT pl;
	RECT r={0,0,w,h};
	sr->v_size.cx = w;
	sr->v_size.cy = h;
	pl.length = sizeof(pl);
	GetWindowPlacement(sr->video_w, &pl);
	AdjustWindowRectEx(&r,GetWindowLong(sr->video_w,GWL_STYLE),
		GetMenu(sr->video_w)?TRUE:FALSE,
		GetWindowLong(sr->video_w,GWL_EXSTYLE));
	pl.rcNormalPosition.right = pl.rcNormalPosition.left + r.right - r.left;
	pl.rcNormalPosition.bottom = pl.rcNormalPosition.top + r.bottom - r.top;
	SetWindowPlacement(sr->video_w, &pl);
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
		TCHAR buf[1024];
		wsprintf(buf, TEXT("Перезаписать сохранённое состояние системы «%s»?"), sr->name);
		if (MessageBox(w, buf, TEXT("Перезапись состояния"), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) != IDYES) return;
	}
	r = save_system_state_file(sr);
	if (r) {
		MessageBox(w, TEXT("При сохранении состояния системы возникла ошибка"), 
			TEXT("Сохранение состояния"), MB_ICONERROR);
	}
}

static void on_load_state(HWND w, struct SYS_RUN_STATE*sr)
{
	int r = 1;
	{
		TCHAR buf[1024];
		wsprintf(buf, TEXT("Загрузить сохранённое состояние системы <%s>?"), sr->name);
		if (MessageBox(w, buf, TEXT("Загрузка состояния"), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON1) != IDYES) return;
	}
	r = load_system_state_file(sr);
	if (r) {
		MessageBox(w, TEXT("При загрузке состояния системы возникла ошибка"),
			TEXT("Загрузка состояния"), MB_ICONERROR);
	}
}

static void on_clear_state(HWND w, struct SYS_RUN_STATE*sr)
{
	TCHAR buf[1024];
	int r;
	sprintf(buf, TEXT("Сбросить сохранённое состояние системы <%s>?"), sr->name);
	if (MessageBox(w, buf, TEXT("Сброс состояния"), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) != IDYES) return;
	r = clear_system_state_file(sr);
	if (r) {
		MessageBox(w, TEXT("При сбросе состояния системы возникла ошибка"), 
			TEXT("Сброс состояния"), MB_ICONERROR);
	}
}


LRESULT CALLBACK wnd_proc(HWND w,UINT msg,WPARAM wp,LPARAM lp)
{
	struct SYS_RUN_STATE*sr = (struct SYS_RUN_STATE*)GetWindowLongPtr(w, GWL_USERDATA);
	switch (msg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT cs = (LPCREATESTRUCT)lp;
			HMENU m;
			sr = (struct SYS_RUN_STATE*)cs->lpCreateParams;
			SetWindowLongPtr(w, GWL_USERDATA, (LONG)sr);
			puts("wnd_proc createwindow");
#ifdef UNDER_CE
			m=sr->popup_menu=CreatePopupMenu();
#else
			m=sr->popup_menu=GetSystemMenu(w,FALSE);
			AppendMenu(m,MF_SEPARATOR,0,0);
#endif
			AppendMenu(m,MF_STRING,IDC_RESET,TEXT("&Сброс\tCtrl+Break"));
			AppendMenu(m,MF_STRING,IDC_HRESET,TEXT("&Полный сброс"));
//			AppendMenu(m,MF_STRING,IDC_IRQ,TEXT("IRQ"));
//			AppendMenu(m,MF_STRING,IDC_NMI,TEXT("NMI"));
			AppendMenu(m,MF_SEPARATOR,0,0);
			AppendMenu(m,MF_STRING,IDC_SAVESTATE,TEXT("Сохранить состояние..."));
			AppendMenu(m,MF_STRING,IDC_LOADSTATE,TEXT("Загрузить состояние..."));
			AppendMenu(m,MF_STRING,IDC_CLEARSTATE,TEXT("Сбросить состояние..."));
			AppendMenu(m,MF_SEPARATOR,0,0);
#ifdef UNDER_CE
			AppendMenu(m,MF_SEPARATOR,0,0);
			AppendMenu(m,MF_STRING,IDCLOSE,TEXT("&Закрыть"));
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
			if (!BitBlt(dc,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right-ps.rcPaint.left,ps.rcPaint.bottom-ps.rcPaint.top,sr->mem_dc,ps.rcPaint.left,ps.rcPaint.top,SRCCOPY)) {
//				WIN_ERROR(GetLastError(),TEXT("blitter failure"));
			}
			EndPaint(w,&ps);
			return 0;
		}
		break;
	case WM_CLOSE:
		system_command(sr, SYS_COMMAND_STOP, 0, 0);
		free_config(sr->config);
		free_system_state(sr);
		break;
	case WM_KEYDOWN:
		{
#ifdef KEY_SCANCODES
			int d=decode_key(lp);
#else
			int d=decode_key(wp);
#endif
		switch (wp) {
		case VK_F4:
			system_command(sr, SYS_COMMAND_TOGGLE_MONO, 0, 0);
			break;
		}
//			printf("d=%x\n",d);
			if (d) {
				sr->cur_key=d|0x80;
#ifndef SYNC_SCREEN_UPDATE
				InvalidateRect(w,NULL,FALSE);
#endif
			}
			if (d==-1)
				system_command(sr, SYS_COMMAND_RESET, 0, 0);
		}
		break;
	case WM_LBUTTONDOWN:
		SetCapture(w);
		sr->mousebtn|=1;
/*		if (GetKeyState(VK_MENU)) 
			TrackPopupMenu(popup_menu,0,
				(short)LOWORD(lp),(short)HIWORD(lp),
				0,w,NULL);*/
		break;
	case WM_RBUTTONDOWN:
		SetCapture(w);
		sr->mousebtn|=2;
		break;
	case WM_LBUTTONUP:
		ReleaseCapture();
		sr->mousebtn&=~1;
		break;
	case WM_RBUTTONUP:
		ReleaseCapture();
		sr->mousebtn&=~2;
		break;
	case WM_DESTROY:
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
		if ((lp&0xFFFF)==0x419) {
			sr->keyreg=0x7F;
		} else sr->keyreg=0xFF;
		break;
	}
	return DefWindowProc(w,msg,wp,lp);
}

int invalidate_video_window(struct SYS_RUN_STATE*sr, RECT *r)
{
	InvalidateRect(sr->video_w, r, FALSE);
	return 0;
}

void enable_ints(struct SYS_RUN_STATE*sr)
{
	if (sr->ints_enabled) return;
	sr->ints_enabled = 1;
	puts("enable ints");
	SetTimer(sr->video_w,TID_IRQ,2,NULL);
	SetTimer(sr->video_w,TID_NMI,20,NULL);
//	system_command(sr, SYS_COMMAND_IRQ, 0, 0);
//	system_command(sr, SYS_COMMAND_NMI, 0, 0);
}

void disable_ints(struct SYS_RUN_STATE*sr)
{
	if (!sr->ints_enabled) return;
	sr->ints_enabled = 0;
	puts("disable ints");
	KillTimer(sr->video_w,TID_IRQ);
	KillTimer(sr->video_w,TID_NMI);
}

void toggle_ints(struct SYS_RUN_STATE*sr)
{
	if (sr->ints_enabled) 
		disable_ints(sr);
	else
		enable_ints(sr);
}
