#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include "resize.h"
#include "dialog.h"
#include "sysconf.h"
#include "resource.h"

static struct RESIZE_DIALOG resize=
{
	RESIZE_LIMIT_MIN,
	{{164,153},{0,0}},
	4,
	{
		{IDOK,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDCANCEL,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{11,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_BOTTOM}},
		{IDC_DEVLIST,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_BOTTOM}},
	}
};

struct SELINFO {
	int systype;
	int slotid;
};

static int DevList_AddItem(HWND hlist, int id)
{
	return ListBox_AddStringData(hlist, get_devnames(id), id);
}

static int dialog_init(HWND hwnd, void*p)
{
	struct SELINFO*inf = p;
	HWND hlist = GetDlgItem(hwnd, IDC_DEVLIST);
	DevList_AddItem(hlist, DEV_NULL);
	switch (inf->systype) {
	case SYSTEM_7: // agathe 7
		DevList_AddItem(hlist, DEV_MEMORY_XRAM7);
		DevList_AddItem(hlist, DEV_MEMORY_PSROM7);
		DevList_AddItem(hlist, DEV_FDD_TEAC);
		DevList_AddItem(hlist, DEV_FDD_SHUGART);
		DevList_AddItem(hlist, DEV_PRINTER9);
		DevList_AddItem(hlist, DEV_NIPPELCLOCK);
		DevList_AddItem(hlist, DEV_MOUSE_PAR);
		DevList_AddItem(hlist, DEV_MOUSE_NIPPEL);
		return 0;
	case SYSTEM_9: // agathe 9
		if (inf->slotid != CONF_SLOT1) {
			DevList_AddItem(hlist, DEV_MEMORY_XRAM9);
			DevList_AddItem(hlist, DEV_FDD_TEAC);
			DevList_AddItem(hlist, DEV_FDD_SHUGART);
			DevList_AddItem(hlist, DEV_PRINTER9);
			DevList_AddItem(hlist, DEV_MOUSE_PAR);
			DevList_AddItem(hlist, DEV_MOUSE_NIPPEL);
		}
		DevList_AddItem(hlist, DEV_NIPPELCLOCK);
		return 0;
	case SYSTEM_A: // apple ][
	case SYSTEM_P: // apple ][
	case SYSTEM_E: // apple ][
		DevList_AddItem(hlist, DEV_MEMORY_XRAMA);
		DevList_AddItem(hlist, DEV_FDD_SHUGART);
		DevList_AddItem(hlist, DEV_PRINTERA);
		DevList_AddItem(hlist, DEV_SOFTCARD);
		if (inf->slotid == CONF_SLOT3) {
			DevList_AddItem(hlist, DEV_VIDEOTERM);
		}
		DevList_AddItem(hlist, DEV_THUNDERCLOCK);
		DevList_AddItem(hlist, DEV_MOCKINGBOARD);
		DevList_AddItem(hlist, DEV_A2RAMCARD);
		DevList_AddItem(hlist, DEV_RAMFACTOR);
		DevList_AddItem(hlist, DEV_MEMORY_SATURN);
		DevList_AddItem(hlist, DEV_MOUSE_APPLE);
		return 0;
	case SYSTEM_1: // apple I
		DevList_AddItem(hlist, DEV_ACI);
		return 0;
	}
	return -1;
}

static void dialog_destroy(HWND hwnd, void*p)
{
}

static int dialog_command(HWND hwnd, void*p, int notify, int id, HWND ctl)
{
	switch (id) {
	case IDC_DEVLIST:
		switch (notify) {
		case LBN_SELCHANGE:
			EnableDlgItem(hwnd, IDOK, SendDlgItemMessage(hwnd, IDC_DEVLIST, LB_GETCURSEL, 0, 0) != LB_ERR);
			return 0;
		}
		break;
	}
	return 1;
}

static int dialog_ok(HWND hwnd, void*p)
{
	HWND hlist = GetDlgItem(hwnd, IDC_DEVLIST);
	EndDialog(hwnd, ListBox_GetItemData(hlist, ListBox_GetCurSel(hlist)));
	return 0;
}

static int dialog_close(HWND hwnd, void*p)
{
	EndDialog(hwnd, -1);
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


int devseldlg_run(HWND hpar, int systype, int slotid)
{
	struct SELINFO inf = { systype, slotid };
	return dialog_run(&dialog, MAKEINTRESOURCE(IDD_DEVSEL), hpar, &inf);
}
