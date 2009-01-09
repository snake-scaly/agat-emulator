#include <windows.h>
#include <commctrl.h>
#include "resize.h"
#include "dialog.h"
#include "resource.h"

#define DLG_ID IDD_ABOUT

static struct RESIZE_DIALOG resize=
{
	RESIZE_LIMIT_MIN,
	{{187,150},{0,0}},
	6,
	{
		{IDOK,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{11,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_CENTER}},
		{12,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_CENTER}},
		{13,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_CENTER}},
		{14,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_CENTER}},
		{15,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_CENTER}},

	}
};

static int dialog_init(HWND hwnd, void*p)
{
	return 0;
}

static void dialog_destroy(HWND hwnd, void*p)
{
}

static int dialog_command(HWND hwnd, void*p, int notify, int id, HWND ctl)
{
	return 1;
}

static int dialog_ok(HWND hwnd, void*p)
{
	EndDialog(hwnd, 0);
	return 0;
}

static int dialog_close(HWND hwnd, void*p)
{
	EndDialog(hwnd, 1);
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

int aboutdlg_run(HWND hpar)
{
	return dialog_run(&dialog, MAKEINTRESOURCE(IDD_ABOUT), hpar, NULL);
}
