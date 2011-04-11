#include <windows.h>
#include <commctrl.h>
#include "resize.h"
#include "dialog.h"
#include "sysconf.h"
#include "resource.h"


#include "localize.h"

static struct RESIZE_DIALOG resize=
{
	RESIZE_LIMIT_MIN,
	{{187,84},{0,0}},
	5,
	{
		{IDOK,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDCANCEL,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDC_TTYSPEED,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_NONE}},
		{IDC_TTYCLEAR,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_TOP}},
		{11,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_NONE}},
	}
};

static void update_controls(HWND hwnd)
{
	TCHAR buf[32];
	int p = SendDlgItemMessage(hwnd, IDC_TTYSPEED, TBM_GETPOS, 0, 0);
	if (p == 500) lstrcpy(buf, TEXT("inf"));
	else wsprintf(buf, TEXT("%3i%%"), p);
	SetDlgItemText(hwnd, 11, buf);
}


static int dialog_init(HWND hwnd, struct SLOTCONFIG*conf)
{
	HWND hlist;
	hlist = GetDlgItem(hwnd, IDC_CPU_TYPE);
	if (conf->cfgint[CFG_INT_TTY_FLAGS] & CFG_INT_TTY_FLAG_CLEAR)
		CheckDlgButton(hwnd, IDC_TTYCLEAR, BST_CHECKED);
	SendDlgItemMessage(hwnd, IDC_TTYSPEED, TBM_SETRANGE, FALSE, MAKELONG(1, 500));
	SendDlgItemMessage(hwnd, IDC_TTYSPEED, TBM_SETPOS, TRUE, conf->cfgint[CFG_INT_TTY_SPEED]);
	SendDlgItemMessage(hwnd, IDC_TTYSPEED, TBM_SETTICFREQ, 10, 0);
	update_controls(hwnd);
	return 0;
}

static void dialog_destroy(HWND hwnd, void*p)
{
}

static int dialog_command(HWND hwnd, void*p, int notify, int id, HWND ctl)
{
	return 1;
}

static int dialog_message(HWND hwnd, void*p, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_VSCROLL:
	case WM_HSCROLL:
		update_controls(hwnd);
		break;
	}
	return 1;
}

static int dialog_ok(HWND hwnd, struct SLOTCONFIG*conf)
{
	int r = 0;
	conf->cfgint[CFG_INT_TTY_SPEED] = SendDlgItemMessage(hwnd, IDC_TTYSPEED, TBM_GETPOS, 0, 0);
	if (IsDlgButtonChecked(hwnd, IDC_TTYCLEAR) == BST_CHECKED) r |= CFG_INT_TTY_FLAG_CLEAR;
	conf->cfgint[CFG_INT_TTY_FLAGS] = r;
	EndDialog(hwnd, TRUE);
	return 0;
}

static int dialog_close(HWND hwnd, void*p)
{
	EndDialog(hwnd, FALSE);
	return 0;
}


static int dialog_notify(HWND hwnd, void*p, int id, LPNMHDR hdr)
{
	return 1;
}

static struct DIALOG_DATA dialog =
{
	&resize,

	dialog_init, 
	dialog_destroy,
	dialog_command,
	dialog_notify,

	dialog_close,
	dialog_ok,
	NULL,
	NULL,

	dialog_message
};

int ttya1dlg_run(HWND hpar, struct SLOTCONFIG *conf)
{
	return dialog_run(&dialog, MAKEINTRESOURCE(IDD_TTYA1), hpar, conf);
}
