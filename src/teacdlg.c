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
	{{203,194},{0,0}},
	19,
	{
		{IDOK,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDCANCEL,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{11,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_NONE}},
		{12,{RESIZE_ALIGN_NONE,RESIZE_ALIGN_NONE}},
		{13,{RESIZE_ALIGN_NONE,RESIZE_ALIGN_NONE}},
		{14,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{15,{RESIZE_ALIGN_NONE,RESIZE_ALIGN_CENTER}},
		{16,{RESIZE_ALIGN_NONE,RESIZE_ALIGN_CENTER}},
		{17,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_TOP}},
		{IDC_DRV1,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_NONE}},
		{IDC_DRV2,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_NONE}},
		{IDC_IMG1,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_IMG2,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_RO1,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_RO2,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_IMGSEL1,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_CENTER}},
		{IDC_IMGSEL2,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_CENTER}},
		{IDC_IMGROM,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_TOP}},
		{IDC_IMGROMSEL,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_TOP}},
	}
};

static void DrvList_AddItem(HWND hlist, int id, int selid)
{
	int no;
	no = ComboBox_AddStringData(hlist, get_drvtypenames(id), id);
	if (id == selid) {
		ComboBox_SetCurSel(hlist, no);
	}
}

static void upd_controls(HWND hwnd)
{
	BOOL b;
	HWND hlist;

	hlist = GetDlgItem(hwnd, IDC_DRV1);
	b = ComboBox_GetItemData(hlist, ComboBox_GetCurSel(hlist)) != DRV_TYPE_NONE;
	EnableDlgItem(hwnd, IDC_IMG1, b);
	EnableDlgItem(hwnd, IDC_IMGSEL1, b);
	EnableDlgItem(hwnd, IDC_RO1, b);
	EnableDlgItem(hwnd, 15, b);

	hlist = GetDlgItem(hwnd, IDC_DRV2);
	b = ComboBox_GetItemData(hlist, ComboBox_GetCurSel(hlist)) != DRV_TYPE_NONE;
	EnableDlgItem(hwnd, IDC_IMG2, b);
	EnableDlgItem(hwnd, IDC_IMGSEL2, b);
	EnableDlgItem(hwnd, IDC_RO2, b);
	EnableDlgItem(hwnd, 16, b);
}


static int dialog_init(HWND hwnd, struct SLOTCONFIG*sc)
{
	HWND hlist;

	SetDlgItemText(hwnd, IDC_IMGROM, sc->cfgstr[CFG_STR_DRV_ROM]);
	SetDlgItemText(hwnd, IDC_IMG1, sc->cfgstr[CFG_STR_DRV_IMAGE1]);
	SetDlgItemText(hwnd, IDC_IMG2, sc->cfgstr[CFG_STR_DRV_IMAGE2]);

	hlist = GetDlgItem(hwnd, IDC_DRV1);
	DrvList_AddItem(hlist, DRV_TYPE_NONE, sc->cfgint[CFG_INT_DRV_TYPE1]);
	DrvList_AddItem(hlist, DRV_TYPE_1S1D, sc->cfgint[CFG_INT_DRV_TYPE1]);
	DrvList_AddItem(hlist, DRV_TYPE_1S2D, sc->cfgint[CFG_INT_DRV_TYPE1]);
	DrvList_AddItem(hlist, DRV_TYPE_2S1D, sc->cfgint[CFG_INT_DRV_TYPE1]);
	DrvList_AddItem(hlist, DRV_TYPE_2S2D, sc->cfgint[CFG_INT_DRV_TYPE1]);

	hlist = GetDlgItem(hwnd, IDC_DRV2);
	DrvList_AddItem(hlist, DRV_TYPE_NONE, sc->cfgint[CFG_INT_DRV_TYPE2]);
	DrvList_AddItem(hlist, DRV_TYPE_1S1D, sc->cfgint[CFG_INT_DRV_TYPE2]);
	DrvList_AddItem(hlist, DRV_TYPE_1S2D, sc->cfgint[CFG_INT_DRV_TYPE2]);
	DrvList_AddItem(hlist, DRV_TYPE_2S1D, sc->cfgint[CFG_INT_DRV_TYPE2]);
	DrvList_AddItem(hlist, DRV_TYPE_2S2D, sc->cfgint[CFG_INT_DRV_TYPE2]);

	if (sc->cfgint[CFG_INT_DRV_RO_FLAGS] & 1)
		CheckDlgButton(hwnd, IDC_RO1, BST_CHECKED);
	if (sc->cfgint[CFG_INT_DRV_RO_FLAGS] & 2)
		CheckDlgButton(hwnd, IDC_RO2, BST_CHECKED);

	upd_controls(hwnd);
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
	case IDC_DRV1:
	case IDC_DRV2:
		upd_controls(hwnd);
		break;
	case IDC_IMGSEL1:
		GetDlgItemText(hwnd, IDC_IMG1, buf, CFGSTRLEN);
		ro = IsDlgButtonChecked(hwnd, IDC_RO1) == BST_CHECKED;
		r = select_disk(hwnd, buf, &ro);
		if (r == TRUE) {
			SetDlgItemText(hwnd, IDC_IMG1, buf);
			CheckDlgButton(hwnd, IDC_RO1, ro?BST_CHECKED:BST_UNCHECKED);
		}
		break;
	case IDC_IMGSEL2:
		GetDlgItemText(hwnd, IDC_IMG2, buf, CFGSTRLEN);
		ro = IsDlgButtonChecked(hwnd, IDC_RO2) == BST_CHECKED;
		r = select_disk(hwnd, buf, &ro);
		if (r == TRUE) {
			SetDlgItemText(hwnd, IDC_IMG2, buf);
			CheckDlgButton(hwnd, IDC_RO2, ro?BST_CHECKED:BST_UNCHECKED);
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
	GetDlgItemText(hwnd, IDC_IMG2, sc->cfgstr[CFG_STR_DRV_IMAGE2], CFGSTRLEN);

	hlist = GetDlgItem(hwnd, IDC_DRV1);
	sc->cfgint[CFG_INT_DRV_TYPE1] = ComboBox_GetItemData(hlist, ComboBox_GetCurSel(hlist));

	hlist = GetDlgItem(hwnd, IDC_DRV2);
	sc->cfgint[CFG_INT_DRV_TYPE2] = ComboBox_GetItemData(hlist, ComboBox_GetCurSel(hlist));

	nd = 0;
	if (sc->cfgint[CFG_INT_DRV_TYPE1] != DRV_TYPE_NONE) nd ++;
	if (sc->cfgint[CFG_INT_DRV_TYPE2] != DRV_TYPE_NONE) nd ++;
	sc->cfgint[CFG_INT_DRV_COUNT] = nd;

	if (IsDlgButtonChecked(hwnd, IDC_RO1) == BST_CHECKED)
		sc->cfgint[CFG_INT_DRV_RO_FLAGS] |= 1;
	else 
		sc->cfgint[CFG_INT_DRV_RO_FLAGS] &= ~1;

	if (IsDlgButtonChecked(hwnd, IDC_RO2) == BST_CHECKED)
		sc->cfgint[CFG_INT_DRV_RO_FLAGS] |= 2;
	else 
		sc->cfgint[CFG_INT_DRV_RO_FLAGS] &= ~2;

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


int teaccfgdlg_run(HWND hpar, struct SLOTCONFIG*sc)
{
	return dialog_run(&dialog, MAKEINTRESOURCE(IDD_TEACCFG), hpar, sc);
}
