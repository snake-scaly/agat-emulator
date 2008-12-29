#include <windows.h>
#include <commctrl.h>
#include "resize.h"
#include "resource.h"

#define DLG_ID IDD_SNDCFG

extern HINSTANCE intface_inst;

static struct RESIZE_DIALOG resize=
{
	RESIZE_LIMIT_MIN,
	{{192,129},{0,0}},
	12,
	{
		{IDOK,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDCANCEL,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDC_NONE,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_MMSYSTEM,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_DIRECTSOUND,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_FREQ,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_BUFSIZE,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{10,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{11,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_CENTER}},
		{12,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_CENTER}},
		{13,{RESIZE_ALIGN_NONE,RESIZE_ALIGN_CENTER}},
		{14,{RESIZE_ALIGN_NONE,RESIZE_ALIGN_CENTER}},
	}
};

static int dialog_init(HWND hwnd)
{
	return 0;
}

static void dialog_destroy(HWND hwnd)
{
}

static int dialog_command(HWND hwnd, int notify, int id, HWND ctl)
{
	switch (id) {
	case IDOK:
		EndDialog(hwnd, 0);
		return 0;
	case IDCANCEL:
		EndDialog(hwnd, 1);
		return 0;
	}
	return 1;
}

static int dialog_notify(HWND hwnd, int id, LPNMHDR hdr)
{
	return 1;
}

static BOOL CALLBACK dialog_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	int r;
	switch (msg) {
	case WM_INITDIALOG:
		r = dialog_init(hwnd);
		if (r < 0) {
			EndDialog(hwnd, -1);
			return FALSE;
		}
		resize_init(&resize);
		resize_attach(&resize,hwnd);
		resize_init_placement(hwnd, MAKEINTRESOURCE(DLG_ID));
		SetWindowLong(hwnd, DWL_USER, lp);
		return FALSE;
	case WM_DESTROY:
		resize_detach(&resize,hwnd);
		resize_free(&resize);
		resize_save_placement(hwnd, MAKEINTRESOURCE(DLG_ID));
		dialog_destroy(hwnd);
		return TRUE;
	case WM_CLOSE:
		EndDialog(hwnd, 1);
		break;
	case WM_SIZE:
		resize_realign(&resize,hwnd,LOWORD(lp),HIWORD(lp));
		return 0;
	case WM_SIZING:
		resize_sizing(&resize,hwnd,wp,(LPRECT)lp);
		return TRUE;
	case WM_COMMAND:
		r = dialog_command(hwnd, HIWORD(wp), LOWORD(wp), (HWND)lp);
		if (!r) break;
		return FALSE;
	case WM_NOTIFY:
		r = dialog_notify(hwnd, (int)wp, (LPNMHDR)lp);
		if (!r) break;
		return FALSE;
	default:
		return FALSE;
	}
	return TRUE;
}

int snddlg_run(HWND hpar)
{
	return DialogBox(intface_inst, MAKEINTRESOURCE(DLG_ID), hpar, dialog_proc);
}
