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
	{{239,155},{0,0}},
	4,
	{
		{IDOK,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDCANCEL,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDC_SYSVIEW,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_CENTER}},
		{IDC_SYSLIST,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
	}
};

static int dialog_init(HWND hwnd, struct SLOTCONFIG*conf)
{
	HWND hlist;
	int i;

	hlist = GetDlgItem(hwnd, IDC_SYSLIST);
	for (i = 0; i < NSYSTYPES; i++) {
		ListBox_AddString(hlist, get_sysnames(i));
	}
	return 0;
}

static void dialog_destroy(HWND hwnd, void*p)
{
}

static int dialog_command(HWND hwnd, void*p, int notify, int id, HWND ctl)
{
	int bmps[] = {
		IDB_AGAT7,
		IDB_AGAT9,
		IDB_APPLE2,
		IDB_APPLE2P,
		IDB_APPLE2E,
		IDB_APPLE1,
		IDB_APPLE2EE,
		IDB_PRAVETZ82,
		IDB_PRAVETZ8A,
	};

	if (id == IDC_SYSLIST && notify == LBN_SELCHANGE) {
		HBITMAP bm, lbm;
		LPCTSTR res_id;
		res_id = MAKEINTRESOURCE(bmps[ListBox_GetCurSel(ctl)]);
		bm = LoadBitmap(localize_get_lib(), res_id);
		if (!bm) bm = LoadBitmap(localize_get_def_lib(), res_id);
		if (!bm) bm = LoadBitmap(localize_get_def_lib(), MAKEINTRESOURCE(IDB_UNKNOWN));
		lbm = (HANDLE)SendDlgItemMessage(hwnd, IDC_SYSVIEW, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bm);
		if (lbm) DeleteObject(lbm);
		EnableDlgItem(hwnd, IDOK, TRUE);
	}
//	printf("%i %i\n", notify, id);
	return 1;
}

static int dialog_ok(HWND hwnd, struct SLOTCONFIG*conf)
{
	int r;
	r = ListBox_GetCurSel(GetDlgItem(hwnd, IDC_SYSLIST));
	EndDialog(hwnd, r + TRUE);
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

int syschoosedlg_run(HWND hpar)
{
	return dialog_run(&dialog, MAKEINTRESOURCE(IDD_SYSCHOOSE), hpar, NULL);
}
