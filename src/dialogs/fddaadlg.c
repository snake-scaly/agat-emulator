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
	{{203,104},{0,0}},
	9,
	{
		{IDOK,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDCANCEL,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{14,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{17,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_TOP}},
		{IDC_IMG1,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_RO1,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_IMGSEL1,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_CENTER}},
		{IDC_IMGROM,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_TOP}},
		{IDC_IMGROMSEL,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_TOP}},
	}
};


static int dialog_init(HWND hwnd, struct SLOTCONFIG*sc)
{
	HWND hlist;

	SetDlgItemText(hwnd, IDC_IMGROM, sc->cfgstr[CFG_STR_DRV_ROM]);
	SetDlgItemText(hwnd, IDC_IMG1, sc->cfgstr[CFG_STR_DRV_IMAGE1]);

	if (sc->cfgint[CFG_INT_DRV_RO_FLAGS] & 1)
		CheckDlgButton(hwnd, IDC_RO1, BST_CHECKED);
	return 0;
}

static void dialog_destroy(HWND hwnd, struct SLOTCONFIG*sc)
{
}

static int dialog_command(HWND hwnd, struct SLOTCONFIG*sc, int notify, int id, HWND ctl)
{
	TCHAR buf[CFGSTRLEN];
	int ro;
	int r;

	switch (id) {
	case IDC_IMGSEL1:
		GetDlgItemText(hwnd, IDC_IMG1, buf, CFGSTRLEN);
		ro = IsDlgButtonChecked(hwnd, IDC_RO1) == BST_CHECKED;
		r = select_disk(hwnd, buf, &ro);
		if (r == TRUE) {
			SetDlgItemText(hwnd, IDC_IMG1, buf);
			CheckDlgButton(hwnd, IDC_RO1, ro?BST_CHECKED:BST_UNCHECKED);
		}
		break;
	case IDC_IMGROMSEL:
		GetDlgItemText(hwnd, IDC_IMGROM, buf, CFGSTRLEN);
		r = select_rom(hwnd, buf);
		if (r == TRUE) {
			SetDlgItemText(hwnd, IDC_IMGROM, buf);
		}
		break;
	}
	return 1;
}

static int dialog_ok(HWND hwnd, struct SLOTCONFIG*sc)
{
	HWND hlist;
	int nd;

	GetDlgItemText(hwnd, IDC_IMGROM, sc->cfgstr[CFG_STR_DRV_ROM], CFGSTRLEN);
	GetDlgItemText(hwnd, IDC_IMG1, sc->cfgstr[CFG_STR_DRV_IMAGE1], CFGSTRLEN);

	if (IsDlgButtonChecked(hwnd, IDC_RO1) == BST_CHECKED)
		sc->cfgint[CFG_INT_DRV_RO_FLAGS] |= 1;
	else 
		sc->cfgint[CFG_INT_DRV_RO_FLAGS] &= ~1;

	EndDialog(hwnd, 1);
	return 0;
}

static int dialog_close(HWND hwnd, struct SLOTCONFIG*sc)
{
	EndDialog(hwnd, 0);
	return 0;
}


static int dialog_notify(HWND hwnd, struct SLOTCONFIG*sc, int id, LPNMHDR hdr)
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


int fddaacfgdlg_run(HWND hpar, struct SLOTCONFIG*sc)
{
	return dialog_run(&dialog, MAKEINTRESOURCE(IDD_FDDAACFG), hpar, sc);
}
