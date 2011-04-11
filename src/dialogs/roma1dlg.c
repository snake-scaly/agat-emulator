#include <windows.h>
#include <sys/stat.h>
#include "resize.h"
#include "dialog.h"
#include "sysconf.h"
#include "localize.h"
#include "resource.h"

static struct RESIZE_DIALOG resize=
{
	RESIZE_LIMIT_MIN,
	{{227,90},{0,0}},
	7,
	{
		{IDOK,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDCANCEL,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},

		{IDC_FW_NAME,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_NONE}},
		{IDC_CHOOSE_FW,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_NONE}},

		{IDC_SMALL,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_MED,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_LARGE,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
	}
};

static void sel_sz(HWND hwnd, int sz)
{
	CheckDlgButton(hwnd, IDC_SMALL, (sz == 256)?BST_CHECKED:BST_UNCHECKED);
	CheckDlgButton(hwnd, IDC_MED, (sz == 4096)?BST_CHECKED:BST_UNCHECKED);
	CheckDlgButton(hwnd, IDC_LARGE, (sz == 8192)?BST_CHECKED:BST_UNCHECKED);
}

static int dialog_init(HWND hwnd, struct SLOTCONFIG*conf)
{
	sel_sz(hwnd, conf->cfgint[CFG_INT_ROM_SIZE]);
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
			if (r!=-1) {
				struct stat st;
				r = stat(buf, &st);
				if (!r) sel_sz(hwnd, st.st_size);
				SetDlgItemText(hwnd, IDC_FW_NAME, buf);
			}
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
	int sz = conf->cfgint[CFG_INT_ROM_SIZE];
	GetDlgItemText(hwnd, IDC_FW_NAME, conf->cfgstr[CFG_STR_ROM], CFGSTRLEN);
	if (IsDlgButtonChecked(hwnd, IDC_SMALL) == BST_CHECKED) sz = 256;
	else if (IsDlgButtonChecked(hwnd, IDC_MED) == BST_CHECKED) sz = 4096;
	else if (IsDlgButtonChecked(hwnd, IDC_LARGE) == BST_CHECKED) sz = 8192;
	conf->cfgint[CFG_INT_ROM_SIZE] = sz;
	conf->cfgint[CFG_INT_ROM_OFS] = 0;
	conf->cfgint[CFG_INT_ROM_MASK] = sz - 1;
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

int roma1dlg_run(HWND hpar, struct SLOTCONFIG *conf)
{
	return dialog_run(&dialog, MAKEINTRESOURCE(IDD_ROMA1), hpar, conf);
}

