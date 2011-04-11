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
	{{227,95},{0,0}},
	7,
	{
		{IDOK,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDCANCEL,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},

		{IDC_FW_NAME,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_NONE}},
		{IDC_CHOOSE_FW,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_NONE}},

		{IDC_ACTIVE,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_F8MOD,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_F8ACTIVE,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
	}
};


static int dialog_init(HWND hwnd, struct SLOTCONFIG*conf)
{
	CheckDlgButton(hwnd, IDC_ACTIVE, (conf->cfgint[CFG_INT_ROM_FLAGS] & CFG_INT_ROM_FLAG_ACTIVE)?BST_CHECKED:BST_UNCHECKED);
	CheckDlgButton(hwnd, IDC_F8MOD, (conf->cfgint[CFG_INT_ROM_FLAGS] & CFG_INT_ROM_FLAG_F8MOD)?BST_CHECKED:BST_UNCHECKED);
	CheckDlgButton(hwnd, IDC_F8ACTIVE, (conf->cfgint[CFG_INT_ROM_FLAGS] & CFG_INT_ROM_FLAG_F8ACTIVE)?BST_CHECKED:BST_UNCHECKED);
	EnableDlgItem(hwnd, IDC_F8ACTIVE, IsDlgButtonChecked(hwnd, IDC_F8MOD) == BST_CHECKED);
	SetDlgItemText(hwnd, IDC_FW_NAME, conf->cfgstr[CFG_STR_ROM]);
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
	case IDC_F8MOD:
		EnableDlgItem(hwnd, IDC_F8ACTIVE, IsDlgButtonChecked(hwnd, IDC_F8MOD) == BST_CHECKED);
		return 0;
	}
	return 1;
}

static int dialog_ok(HWND hwnd, struct SLOTCONFIG*conf)
{
	int r = 0;
	GetDlgItemText(hwnd, IDC_FW_NAME, conf->cfgstr[CFG_STR_ROM], CFGSTRLEN);
	if (IsDlgButtonChecked(hwnd, IDC_ACTIVE) == BST_CHECKED) r |= CFG_INT_ROM_FLAG_ACTIVE;
	if (IsDlgButtonChecked(hwnd, IDC_F8MOD) == BST_CHECKED) r |= CFG_INT_ROM_FLAG_F8MOD;
	if (IsDlgButtonChecked(hwnd, IDC_F8ACTIVE) == BST_CHECKED) r |= CFG_INT_ROM_FLAG_F8ACTIVE;
	conf->cfgint[CFG_INT_ROM_FLAGS] = r;
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

int firmwdlg_run(HWND hpar, struct SLOTCONFIG *conf)
{
	return dialog_run(&dialog, MAKEINTRESOURCE(IDD_FIRMWARE), hpar, conf);
}

