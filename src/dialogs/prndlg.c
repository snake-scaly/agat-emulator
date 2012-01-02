#include <windows.h>
#include <commctrl.h>
#include "resize.h"
#include "dialog.h"
#include "sysconf.h"
#include "localize.h"
#include "resource.h"

static struct RESIZE_DIALOG resize9=
{
	RESIZE_LIMIT_MIN,
	{{206,110},{0,0}},
	10,
	{
		{IDOK,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDCANCEL,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},

		{IDC_FW1_NAME,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_FW2_NAME,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},

		{IDC_CHOOSE_FW1,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_CENTER}},
		{IDC_CHOOSE_FW2,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_CENTER}},

		{IDC_PRINT_MODE,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},

		{100,{RESIZE_ALIGN_NONE,RESIZE_ALIGN_CENTER}},
		{101,{RESIZE_ALIGN_NONE,RESIZE_ALIGN_CENTER}},
		{102,{RESIZE_ALIGN_NONE,RESIZE_ALIGN_CENTER}},
	}
};



static struct RESIZE_DIALOG resizea=
{
	RESIZE_LIMIT_MIN,
	{{206,79},{0,0}},
	7,
	{
		{IDOK,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDCANCEL,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},

		{IDC_FW1_NAME,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},

		{IDC_CHOOSE_FW1,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_CENTER}},

		{IDC_PRINT_MODE,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},

		{100,{RESIZE_ALIGN_NONE,RESIZE_ALIGN_CENTER}},
		{102,{RESIZE_ALIGN_NONE,RESIZE_ALIGN_CENTER}},
	}
};



static struct RESIZE_DIALOG resizeaa=
{
	RESIZE_LIMIT_MIN,
	{{206,51},{0,0}},
	4,
	{
		{IDOK,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDCANCEL,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},

		{IDC_PRINT_MODE,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},

		{102,{RESIZE_ALIGN_NONE,RESIZE_ALIGN_CENTER}},
	}
};


static void PrnList_AddItem(HWND hlist, int id, int selid)
{
	int no;
	TCHAR buf[1024];
	no = ComboBox_AddStringData(hlist, localize_str(LOC_PRINTER, id, buf, sizeof(buf)), id);
	if (id == selid) {
		ComboBox_SetCurSel(hlist, no);
	}
}

static void MouseList_AddItem(HWND hlist, int id, int selid)
{
	int no;
	TCHAR buf[1024];
	no = ComboBox_AddStringData(hlist, localize_str(LOC_MOUSE, 10 + id, buf, sizeof(buf)), id);
	if (id == selid) {
		ComboBox_SetCurSel(hlist, no);
	}
}

static int dialog_init(HWND hwnd, struct SLOTCONFIG*conf)
{
	HWND hlist;
	hlist = GetDlgItem(hwnd, IDC_PRINT_MODE);
	if (conf->dev_type == DEV_MOUSE_PAR) {
		TCHAR buf[1024];
		SetWindowText(hwnd, localize_str(LOC_MOUSE, 0, buf, sizeof(buf)));
		SetDlgItemText(hwnd, 102, localize_str(LOC_MOUSE, 1, buf, sizeof(buf)));
		MouseList_AddItem(hlist, MOUSE_NONE, conf->cfgint[CFG_INT_MOUSE_TYPE]);
		MouseList_AddItem(hlist, MOUSE_MM8031, conf->cfgint[CFG_INT_MOUSE_TYPE]);
		MouseList_AddItem(hlist, MOUSE_MARS, conf->cfgint[CFG_INT_MOUSE_TYPE]);
	} else {
		PrnList_AddItem(hlist, PRINT_RAW, conf->cfgint[CFG_INT_PRINT_MODE]);
		PrnList_AddItem(hlist, PRINT_TEXT, conf->cfgint[CFG_INT_PRINT_MODE]);
		PrnList_AddItem(hlist, PRINT_TIFF, conf->cfgint[CFG_INT_PRINT_MODE]);
		PrnList_AddItem(hlist, PRINT_PRINT, conf->cfgint[CFG_INT_PRINT_MODE]);
	}
	SetDlgItemText(hwnd, IDC_FW1_NAME, conf->cfgstr[CFG_STR_ROM]);
	SetDlgItemText(hwnd, IDC_FW2_NAME, conf->cfgstr[CFG_STR_ROM2]);

	return 0;
}

static void dialog_destroy(HWND hwnd, void*p)
{
}

static int dialog_command(HWND hwnd, void*p, int notify, int id, HWND ctl)
{
	switch (id) {
	case IDC_CHOOSE_FW1:
		{
			TCHAR buf[CFGSTRLEN];
			int r;
			GetDlgItemText(hwnd, IDC_FW1_NAME, buf, CFGSTRLEN);
			r = select_rom(hwnd, buf, -1);
			if (r!=-1) SetDlgItemText(hwnd, IDC_FW1_NAME, buf);
		}
		break;
	case IDC_CHOOSE_FW2:
		{
			TCHAR buf[CFGSTRLEN];
			int r;
			GetDlgItemText(hwnd, IDC_FW2_NAME, buf, CFGSTRLEN);
			r = select_font(hwnd, buf, -1);
			if (r!=-1) SetDlgItemText(hwnd, IDC_FW2_NAME, buf);
		}
		break;
	}
	return 1;
}

static int dialog_ok(HWND hwnd, struct SLOTCONFIG*conf)
{
	HWND hlist;

	hlist = GetDlgItem(hwnd, IDC_PRINT_MODE);
	conf->cfgint[CFG_INT_PRINT_MODE] = ComboBox_GetItemData(hlist, ComboBox_GetCurSel(hlist));

	GetDlgItemText(hwnd, IDC_FW1_NAME, conf->cfgstr[CFG_STR_ROM], CFGSTRLEN);
	GetDlgItemText(hwnd, IDC_FW2_NAME, conf->cfgstr[CFG_STR_ROM2], CFGSTRLEN);
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

static struct DIALOG_DATA dialog9 =
{
	&resize9,

	dialog_init, 
	dialog_destroy,
	dialog_command,
	dialog_notify,

	dialog_close,
	dialog_ok
};

static struct DIALOG_DATA dialoga =
{
	&resizea,

	dialog_init, 
	dialog_destroy,
	dialog_command,
	dialog_notify,

	dialog_close,
	dialog_ok
};

static struct DIALOG_DATA dialogaa =
{
	&resizeaa,

	dialog_init, 
	dialog_destroy,
	dialog_command,
	dialog_notify,

	dialog_close,
	dialog_ok
};

int prn9dlg_run(HWND hpar, struct SLOTCONFIG *conf)
{
	return dialog_run(&dialog9, MAKEINTRESOURCE(IDD_PRN9CFG), hpar, conf);
}

int prnadlg_run(HWND hpar, struct SLOTCONFIG *conf)
{
	return dialog_run(&dialoga, MAKEINTRESOURCE(IDD_PRNACFG), hpar, conf);
}

int prnaadlg_run(HWND hpar, struct SLOTCONFIG *conf)
{
	return dialog_run(&dialogaa, MAKEINTRESOURCE(IDD_PRNAACFG), hpar, conf);
}
