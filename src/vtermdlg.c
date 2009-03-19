#include <windows.h>
#include <commctrl.h>
#include "resize.h"
#include "dialog.h"
#include "sysconf.h"
#include "localize.h"
#include "resource.h"

static struct RESIZE_DIALOG resize=
{
	RESIZE_LIMIT_MIN,
	{{206,95},{0,0}},
	11,
	{
		{IDOK,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDCANCEL,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},

		{IDC_FW_NAME,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_FNT_NAME,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_XFNT_NAME,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},

		{IDC_CHOOSE_FW,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_CENTER}},
		{IDC_CHOOSE_FNT,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_CENTER}},
		{IDC_CHOOSE_XFNT,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_CENTER}},

		{100,{RESIZE_ALIGN_NONE,RESIZE_ALIGN_CENTER}},
		{101,{RESIZE_ALIGN_NONE,RESIZE_ALIGN_CENTER}},
		{102,{RESIZE_ALIGN_NONE,RESIZE_ALIGN_CENTER}},
	}
};


static int dialog_init(HWND hwnd, struct SLOTCONFIG*conf)
{
	SetDlgItemText(hwnd, IDC_FW_NAME, conf->cfgstr[CFG_STR_ROM]);
	SetDlgItemText(hwnd, IDC_FNT_NAME, conf->cfgstr[CFG_STR_ROM2]);
	SetDlgItemText(hwnd, IDC_XFNT_NAME, conf->cfgstr[CFG_STR_ROM3]);
	return 0;
}

static void dialog_destroy(HWND hwnd, void*p)
{
}

static int dialog_command(HWND hwnd, void*p, int notify, int id, HWND ctl)
{
	switch (id) {
	case IDC_CHOOSE_FW:
		{
			TCHAR buf[CFGSTRLEN];
			int r;
			GetDlgItemText(hwnd, IDC_FW_NAME, buf, CFGSTRLEN);
			r = select_rom(hwnd, buf, -1);
			if (r!=-1) SetDlgItemText(hwnd, IDC_FW_NAME, buf);
		}
		break;
	case IDC_CHOOSE_FNT:
		{
			TCHAR buf[CFGSTRLEN];
			int r;
			GetDlgItemText(hwnd, IDC_FNT_NAME, buf, CFGSTRLEN);
			r = select_font(hwnd, buf, -1);
			if (r!=-1) SetDlgItemText(hwnd, IDC_FNT_NAME, buf);
		}
		break;
	case IDC_CHOOSE_XFNT:
		{
			TCHAR buf[CFGSTRLEN];
			int r;
			GetDlgItemText(hwnd, IDC_XFNT_NAME, buf, CFGSTRLEN);
			r = select_font(hwnd, buf, -1);
			if (r!=-1) SetDlgItemText(hwnd, IDC_XFNT_NAME, buf);
		}
		break;
	}
	return 1;
}

static int dialog_ok(HWND hwnd, struct SLOTCONFIG*conf)
{
	GetDlgItemText(hwnd, IDC_FW_NAME, conf->cfgstr[CFG_STR_ROM], CFGSTRLEN);
	GetDlgItemText(hwnd, IDC_FNT_NAME, conf->cfgstr[CFG_STR_ROM2], CFGSTRLEN);
	GetDlgItemText(hwnd, IDC_XFNT_NAME, conf->cfgstr[CFG_STR_ROM3], CFGSTRLEN);
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
	dialog_ok
};

int vtermdlg_run(HWND hpar, struct SLOTCONFIG *conf)
{
	return dialog_run(&dialog, MAKEINTRESOURCE(IDD_VTCFG), hpar, conf);
}
