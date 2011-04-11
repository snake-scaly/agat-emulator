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
	5,
	{
		{IDOK,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDCANCEL,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDC_NONE,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_MOUSE,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_JOYSTICK,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
	}
};

static int dialog_init(HWND hwnd, struct SLOTCONFIG*conf)
{
	switch (conf->dev_type) {
	case DEV_NOJOY:
		CheckDlgButton(hwnd, IDC_NONE, BST_CHECKED);
		break;
	case DEV_MOUSE:
		CheckDlgButton(hwnd, IDC_MOUSE, BST_CHECKED);
		break;
	case DEV_JOYSTICK:
		CheckDlgButton(hwnd, IDC_JOYSTICK, BST_CHECKED);
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
	if (IsDlgButtonChecked(hwnd, IDC_NONE) == BST_CHECKED) conf->dev_type = DEV_NOJOY;
	else if (IsDlgButtonChecked(hwnd, IDC_MOUSE) == BST_CHECKED) conf->dev_type = DEV_MOUSE;
	else if (IsDlgButtonChecked(hwnd, IDC_JOYSTICK) == BST_CHECKED) conf->dev_type = DEV_JOYSTICK;
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

int joydlg_run(HWND hpar, struct SLOTCONFIG *conf)
{
	return dialog_run(&dialog, MAKEINTRESOURCE(IDD_JOYCFG), hpar, conf);
}
