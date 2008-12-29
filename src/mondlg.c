#include <windows.h>
#include <commctrl.h>
#include "resize.h"
#include "dialog.h"
#include "sysconf.h"
#include "resource.h"

static struct RESIZE_DIALOG resize=
{
	RESIZE_LIMIT_MIN,
	{{187,91},{0,0}},
	4,
	{
		{IDOK,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDCANCEL,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDC_MONO,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_COLOR,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
	}
};

static int dialog_init(HWND hwnd, struct SLOTCONFIG*conf)
{
	switch (conf->dev_type) {
	case DEV_MONO:
		CheckDlgButton(hwnd, IDC_MONO, BST_CHECKED);
		break;
	case DEV_COLOR:
		CheckDlgButton(hwnd, IDC_COLOR, BST_CHECKED);
		break;
	}
	return 0;
}

static void dialog_destroy(HWND hwnd, void*p)
{
}

static int dialog_command(HWND hwnd, void*p, int notify, int id, HWND ctl)
{
	return 1;
}

static int dialog_ok(HWND hwnd, struct SLOTCONFIG*conf)
{
	if (IsDlgButtonChecked(hwnd, IDC_COLOR) == BST_CHECKED) conf->dev_type = DEV_COLOR;
	else if (IsDlgButtonChecked(hwnd, IDC_MONO) == BST_CHECKED) conf->dev_type = DEV_MONO;
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

int mondlg_run(HWND hpar, struct SLOTCONFIG *conf)
{
	return dialog_run(&dialog, MAKEINTRESOURCE(IDD_MONCFG), hpar, conf);
}
