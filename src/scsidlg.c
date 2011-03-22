#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include "resize.h"
#include "dialog.h"
#include "sysconf.h"
#include "localize.h"
#include "resource.h"

static struct RESIZE_DIALOG resize=
{
	RESIZE_LIMIT_MIN,
	{{227,195},{0,0}},
	14,
	{
		{IDOK,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDCANCEL,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDC_SIZE1,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_NONE}},
		{IDC_SIZE2,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_NONE}},
		{IDC_SIZE3,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_NONE}},
		{IDC_DEVNAME1,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_NONE}},
		{IDC_DEVNAME2,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_NONE}},
		{IDC_DEVNAME3,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_NONE}},
		{IDC_DEVSEL1,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_NONE}},
		{IDC_DEVSEL2,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_NONE}},
		{IDC_DEVSEL3,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_NONE}},
		{17,{RESIZE_ALIGN_NONE,RESIZE_ALIGN_TOP}},
		{IDC_IMGROM,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_TOP}},
		{IDC_IMGROMSEL,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_TOP}},
	}
};

static void DrvList_AddItem(HWND hlist, int id, int selid, LPCTSTR name)
{
	int no;
	no = ComboBox_AddStringData(hlist, name, id);
	if (id == selid) {
		ComboBox_SetCurSel(hlist, no);
	}
}

static void upd_controls(HWND hwnd)
{
	BOOL b;
	HWND hlist;

	hlist = GetDlgItem(hwnd, IDC_DEVNO1);
	b = ComboBox_GetItemData(hlist, ComboBox_GetCurSel(hlist)) != 0;
	EnableDlgItem(hwnd, IDC_SIZE1, b);
	EnableDlgItem(hwnd, IDC_DEVNAME1, b);
	EnableDlgItem(hwnd, IDC_DEVSEL1, b);

	hlist = GetDlgItem(hwnd, IDC_DEVNO2);
	b = ComboBox_GetItemData(hlist, ComboBox_GetCurSel(hlist)) != 0;
	EnableDlgItem(hwnd, IDC_SIZE2, b);
	EnableDlgItem(hwnd, IDC_DEVNAME2, b);
	EnableDlgItem(hwnd, IDC_DEVSEL2, b);

	hlist = GetDlgItem(hwnd, IDC_DEVNO3);
	b = ComboBox_GetItemData(hlist, ComboBox_GetCurSel(hlist)) != 0;
	EnableDlgItem(hwnd, IDC_SIZE3, b);
	EnableDlgItem(hwnd, IDC_DEVNAME3, b);
	EnableDlgItem(hwnd, IDC_DEVSEL3, b);
}

static void init_devlist(HWND hlist, int val)
{
	TCHAR buf[256];
	DrvList_AddItem(hlist, 0, val, localize_str(LOC_SCSI, 10, buf, sizeof(buf)));
	DrvList_AddItem(hlist, 1, val, TEXT("#0"));
	DrvList_AddItem(hlist, 2, val, TEXT("#1"));
	DrvList_AddItem(hlist, 3, val, TEXT("#2"));
	DrvList_AddItem(hlist, 4, val, TEXT("#3"));
	DrvList_AddItem(hlist, 5, val, TEXT("#4"));
	DrvList_AddItem(hlist, 6, val, TEXT("#5"));
	DrvList_AddItem(hlist, 7, val, TEXT("#7"));
}

static int dialog_init(HWND hwnd, struct SLOTCONFIG*sc)
{
	HWND hlist;

	SetDlgItemText(hwnd, IDC_IMGROM, sc->cfgstr[CFG_STR_DRV_ROM]);
	SetDlgItemText(hwnd, IDC_DEVNAME1, sc->cfgstr[CFG_STR_SCSI_NAME1]);
	SetDlgItemText(hwnd, IDC_DEVNAME2, sc->cfgstr[CFG_STR_SCSI_NAME2]);
	SetDlgItemText(hwnd, IDC_DEVNAME3, sc->cfgstr[CFG_STR_SCSI_NAME3]);
	SetDlgItemInt(hwnd, IDC_SIZE1, sc->cfgint[CFG_INT_SCSI_SZ1], FALSE);
	SetDlgItemInt(hwnd, IDC_SIZE2, sc->cfgint[CFG_INT_SCSI_SZ2], FALSE);
	SetDlgItemInt(hwnd, IDC_SIZE3, sc->cfgint[CFG_INT_SCSI_SZ3], FALSE);

	hlist = GetDlgItem(hwnd, IDC_DEVNO1);
	init_devlist(hlist, sc->cfgint[CFG_INT_SCSI_NO1]);
	hlist = GetDlgItem(hwnd, IDC_DEVNO2);
	init_devlist(hlist, sc->cfgint[CFG_INT_SCSI_NO2]);
	hlist = GetDlgItem(hwnd, IDC_DEVNO3);
	init_devlist(hlist, sc->cfgint[CFG_INT_SCSI_NO3]);

	upd_controls(hwnd);
	return 0;
}

static void dialog_destroy(HWND hwnd, struct SLOTCONFIG*sc)
{
}

static int dialog_command(HWND hwnd, struct SLOTCONFIG*sc, int notify, int id, HWND ctl)
{
	TCHAR buf[CFGSTRLEN];
	int r;

	switch (id) {
	case IDC_DEVNO1:
	case IDC_DEVNO2:
	case IDC_DEVNO3:
		upd_controls(hwnd);
		break;
	case IDC_DEVSEL1:
		GetDlgItemText(hwnd, IDC_DEVNAME1, buf, CFGSTRLEN);
		r = select_disk(hwnd, buf, NULL);
		if (r == TRUE) {
			SetDlgItemText(hwnd, IDC_DEVNAME1, buf);
		}
		break;
	case IDC_DEVSEL2:
		GetDlgItemText(hwnd, IDC_DEVNAME2, buf, CFGSTRLEN);
		r = select_disk(hwnd, buf, NULL);
		if (r == TRUE) {
			SetDlgItemText(hwnd, IDC_DEVNAME2, buf);
		}
		break;
	case IDC_DEVSEL3:
		GetDlgItemText(hwnd, IDC_DEVNAME3, buf, CFGSTRLEN);
		r = select_disk(hwnd, buf, NULL);
		if (r == TRUE) {
			SetDlgItemText(hwnd, IDC_DEVNAME3, buf);
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
	BOOL ok;
	TCHAR buf[2][256];

	sc->cfgint[CFG_INT_SCSI_SZ1] = GetDlgItemInt(hwnd, IDC_SIZE1, &ok, FALSE);
	if (!ok) goto fail2;
	sc->cfgint[CFG_INT_SCSI_SZ2] = GetDlgItemInt(hwnd, IDC_SIZE2, &ok, FALSE);
	if (!ok) goto fail2;
	sc->cfgint[CFG_INT_SCSI_SZ3] = GetDlgItemInt(hwnd, IDC_SIZE3, &ok, FALSE);
	if (!ok) goto fail2;

	GetDlgItemText(hwnd, IDC_IMGROM, sc->cfgstr[CFG_STR_DRV_ROM], CFGSTRLEN);
	GetDlgItemText(hwnd, IDC_DEVNAME1, sc->cfgstr[CFG_STR_SCSI_NAME1], CFGSTRLEN);
	GetDlgItemText(hwnd, IDC_DEVNAME2, sc->cfgstr[CFG_STR_SCSI_NAME2], CFGSTRLEN);
	GetDlgItemText(hwnd, IDC_DEVNAME3, sc->cfgstr[CFG_STR_SCSI_NAME3], CFGSTRLEN);

	hlist = GetDlgItem(hwnd, IDC_DEVNO1);
	sc->cfgint[CFG_INT_SCSI_NO1] = ComboBox_GetItemData(hlist, ComboBox_GetCurSel(hlist));
	hlist = GetDlgItem(hwnd, IDC_DEVNO2);
	sc->cfgint[CFG_INT_SCSI_NO2] = ComboBox_GetItemData(hlist, ComboBox_GetCurSel(hlist));
	hlist = GetDlgItem(hwnd, IDC_DEVNO3);
	sc->cfgint[CFG_INT_SCSI_NO3] = ComboBox_GetItemData(hlist, ComboBox_GetCurSel(hlist));

	if ((sc->cfgint[CFG_INT_SCSI_NO1] && sc->cfgint[CFG_INT_SCSI_NO1] == sc->cfgint[CFG_INT_SCSI_NO2]) || 
		(sc->cfgint[CFG_INT_SCSI_NO1] && sc->cfgint[CFG_INT_SCSI_NO1] == sc->cfgint[CFG_INT_SCSI_NO3]) ||
		(sc->cfgint[CFG_INT_SCSI_NO2] && sc->cfgint[CFG_INT_SCSI_NO2] == sc->cfgint[CFG_INT_SCSI_NO3])) goto fail1;

	if (sc->cfgint[CFG_INT_SCSI_NO1] && !sc->cfgstr[CFG_STR_SCSI_NAME1][0]) goto fail3;
	if (sc->cfgint[CFG_INT_SCSI_NO2] && !sc->cfgstr[CFG_STR_SCSI_NAME2][0]) goto fail3;
	if (sc->cfgint[CFG_INT_SCSI_NO3] && !sc->cfgstr[CFG_STR_SCSI_NAME3][0]) goto fail3;

	if (!sc->cfgstr[CFG_STR_DRV_ROM][0]) goto fail4;

	if (sc->cfgint[CFG_INT_SCSI_NO1] && !sc->cfgint[CFG_INT_SCSI_SZ1]) goto fail5;
	if (sc->cfgint[CFG_INT_SCSI_NO2] && !sc->cfgint[CFG_INT_SCSI_SZ2]) goto fail5;
	if (sc->cfgint[CFG_INT_SCSI_NO3] && !sc->cfgint[CFG_INT_SCSI_SZ3]) goto fail5;

	EndDialog(hwnd, 1);
	return 0;
fail1:
	MessageBox(hwnd, 
		localize_str(LOC_SCSI, 1, buf[0], sizeof(buf[0])), 
		localize_str(LOC_SCSI, 0, buf[0], sizeof(buf[0])), MB_ICONEXCLAMATION | MB_OK);
	return 0;
fail2:
	MessageBox(hwnd, 
		localize_str(LOC_SCSI, 2, buf[0], sizeof(buf[0])), 
		localize_str(LOC_SCSI, 0, buf[0], sizeof(buf[0])), MB_ICONEXCLAMATION | MB_OK);
	return 0;
fail3:
	MessageBox(hwnd, 
		localize_str(LOC_SCSI, 3, buf[0], sizeof(buf[0])), 
		localize_str(LOC_SCSI, 0, buf[0], sizeof(buf[0])), MB_ICONEXCLAMATION | MB_OK);
	return 0;
fail4:
	MessageBox(hwnd, 
		localize_str(LOC_SCSI, 4, buf[0], sizeof(buf[0])), 
		localize_str(LOC_SCSI, 0, buf[0], sizeof(buf[0])), MB_ICONEXCLAMATION | MB_OK);
	return 0;
fail5:
	MessageBox(hwnd, 
		localize_str(LOC_SCSI, 5, buf[0], sizeof(buf[0])), 
		localize_str(LOC_SCSI, 0, buf[0], sizeof(buf[0])), MB_ICONEXCLAMATION | MB_OK);
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


int scsicfgdlg_run(HWND hpar, struct SLOTCONFIG*sc)
{
	return dialog_run(&dialog, MAKEINTRESOURCE(IDD_SCSIDLG), hpar, sc);
}

