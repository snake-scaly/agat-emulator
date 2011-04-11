#include <windows.h>
#include <commctrl.h>
#include "resize.h"
#include "dialog.h"
#include "resource.h"
#include "sysconf.h"


static struct RESIZE_DIALOG resize=
{
	RESIZE_LIMIT_MIN,
	{{187,54},{0,0}},
	4,
	{
		{11,{RESIZE_ALIGN_NONE,RESIZE_ALIGN_CENTER}},
		{IDOK,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDCANCEL,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDC_MEMLIST,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
	}
};

static int dialog_init(HWND hwnd, void*p)
{
	HWND hlist;
	int i;
	unsigned long mp = (unsigned long)p, mask = 1;
	int def = mp >> NMEMSIZES;

	hlist = GetDlgItem(hwnd, IDC_MEMLIST);

	for (i = 0; i < NMEMSIZES; i++, mask <<= 1) {
		if (mp & mask) {
			int ind;
			ind = ComboBox_AddStringData(hlist, get_memsizes_s(i), i);
			if (i == def) ComboBox_SetCurSel(hlist, ind);
		}
	}
	return 0;
}

static int dialog_ok(HWND hwnd, void*p)
{
	HWND hlist;

	hlist = GetDlgItem(hwnd, IDC_MEMLIST);
	EndDialog(hwnd, ComboBox_GetItemData(hlist, ComboBox_GetCurSel(hlist)));
	return 0;
}



static struct DIALOG_DATA dialog =
{
	&resize,

	dialog_init, 
	NULL,
	NULL,
	NULL,

	NULL,
	dialog_ok
};

int memdlg_run(HWND hpar, unsigned mask, int def)
{
	return dialog_run(&dialog, MAKEINTRESOURCE(IDD_MEMORY), hpar, (void*)(mask&((1L<<NMEMSIZES)-1) | (def<<NMEMSIZES)));
}
