#include <windows.h>
#include <commctrl.h>
#include "resize.h"
#include "dialog.h"
#include "sysconf.h"
#include "resource.h"


extern HINSTANCE intface_inst;

static struct RESIZE_DIALOG resize=
{
	RESIZE_LIMIT_MIN,
	{{187,120},{0,0}},
	8,
	{
		{IDOK,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDC_RESET,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDCANCEL,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{12,{RESIZE_ALIGN_NONE,RESIZE_ALIGN_CENTER}},
		{IDC_CPUFREQ,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_UNDOC,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_TOP}},
		{IDC_CPU_TYPE,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_NONE}},
		{11,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_CENTER}},
	}
};

static void update_controls(HWND hwnd)
{
	TCHAR buf[32];
	int p = SendDlgItemMessage(hwnd, IDC_CPUFREQ, TBM_GETPOS, 0, 0);
	wsprintf(buf, TEXT("%3i%%"), p);
	SetDlgItemText(hwnd, 11, buf);
}

static int CpuList_AddItem(HWND hlist, int id, int sel)
{
	int n = ComboBox_AddStringData(hlist, devnames[id], id);
	if (id == sel)
		ComboBox_SetCurSel(hlist, n);
	return n;
}


static int dialog_init(HWND hwnd, struct SLOTCONFIG*conf)
{
	HWND hlist;
	hlist = GetDlgItem(hwnd, IDC_CPU_TYPE);
	CpuList_AddItem(hlist, DEV_6502, conf->dev_type);
	CpuList_AddItem(hlist, DEV_M6502, conf->dev_type);
	if (conf->cfgint[CFG_INT_CPU_EXT])
		CheckDlgButton(hwnd, IDC_UNDOC, BST_CHECKED);
	SendDlgItemMessage(hwnd, IDC_CPUFREQ, TBM_SETRANGE, FALSE, MAKELONG(1, 500));
	SendDlgItemMessage(hwnd, IDC_CPUFREQ, TBM_SETPOS, TRUE, conf->cfgint[CFG_INT_CPU_SPEED]);
	SendDlgItemMessage(hwnd, IDC_CPUFREQ, TBM_SETTICFREQ, 10, 0);
	EnableDlgItem(hwnd, IDC_UNDOC, (ComboBox_GetItemData(hlist, ComboBox_GetCurSel(hlist)) == DEV_6502));
	update_controls(hwnd);
	return 0;
}

static void dialog_destroy(HWND hwnd, void*p)
{
}

static int dialog_command(HWND hwnd, void*p, int notify, int id, HWND ctl)
{
	switch (id) {
	case IDC_RESET:
		SendDlgItemMessage(hwnd, IDC_CPUFREQ, TBM_SETPOS, TRUE, 100);
		CheckDlgButton(hwnd, IDC_UNDOC, BST_UNCHECKED);
		update_controls(hwnd);
		return 0;
	case IDC_CPU_TYPE:
		EnableDlgItem(hwnd, IDC_UNDOC, (ComboBox_GetItemData(ctl, ComboBox_GetCurSel(ctl)) == DEV_6502));
		return 0;
	}
	return 1;
}

static int dialog_message(HWND hwnd, void*p, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_VSCROLL:
	case WM_HSCROLL:
		update_controls(hwnd);
		break;
	}
	return 1;
}

static int dialog_ok(HWND hwnd, struct SLOTCONFIG*conf)
{
	HWND hlist;
	hlist = GetDlgItem(hwnd, IDC_CPU_TYPE);
	conf->dev_type = ComboBox_GetItemData(hlist, ComboBox_GetCurSel(hlist));
	conf->cfgint[CFG_INT_CPU_SPEED] = SendDlgItemMessage(hwnd, IDC_CPUFREQ, TBM_GETPOS, 0, 0);
	conf->cfgint[CFG_INT_CPU_EXT] = IsDlgButtonChecked(hwnd, IDC_UNDOC) == BST_CHECKED;
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
	dialog_ok,
	NULL,
	NULL,

	dialog_message
};

int cpudlg_run(HWND hpar, struct SLOTCONFIG *conf)
{
	return dialog_run(&dialog, MAKEINTRESOURCE(IDD_CPUCFG), hpar, conf);
}
