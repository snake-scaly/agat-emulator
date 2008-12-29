#include <windows.h>
#include <commctrl.h>
#include "resize.h"
#include "dialog.h"
#include "sysconf.h"
#include "resource.h"

static struct RESIZE_DIALOG resize=
{
	RESIZE_LIMIT_MIN,
	{{187,135},{0,0}},
	11,
	{
		{10,{RESIZE_ALIGN_NONE,RESIZE_ALIGN_CENTER}},
		{IDOK,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDCANCEL,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDC_NONE,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_FILE,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{11,{RESIZE_ALIGN_NONE,RESIZE_ALIGN_CENTER}},
		{IDC_FREQ,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{12,{RESIZE_ALIGN_NONE,RESIZE_ALIGN_CENTER}},
		{IDC_FNAME,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_CHOOSE,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_CENTER}},
		{IDC_FAST,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_CENTER}},
	}
};

static const TCHAR*freqnames[]={
	"8000 關",
	"11025 關",
	"22050 關",
	"44100 關",
	"48000 關"
};

static int freqvals[]={8000, 11025, 22050, 44100, 48000};

static void update_controls(HWND hwnd)
{
	BOOL b;
	b = IsDlgButtonChecked(hwnd, IDC_FILE) == BST_CHECKED;
	EnableDlgItem(hwnd, 11, b);
	EnableDlgItem(hwnd, 12, b);
	EnableDlgItem(hwnd, IDC_FREQ, b);
	EnableDlgItem(hwnd, IDC_FNAME, b);
	EnableDlgItem(hwnd, IDC_CHOOSE, b);
	EnableDlgItem(hwnd, IDC_FAST, b);
}

static int dialog_init(HWND hwnd, struct SLOTCONFIG*conf)
{
	int i;
	HWND hlist = GetDlgItem(hwnd, IDC_FREQ);
	switch (conf->dev_type) {
	case DEV_TAPE_NONE:
		CheckDlgButton(hwnd, IDC_NONE, BST_CHECKED);
		break;
	case DEV_TAPE_FILE:
		CheckDlgButton(hwnd, IDC_FILE, BST_CHECKED);
		break;
	}
	for (i = 0; i < sizeof(freqvals)/sizeof(freqvals[0]); ++i) {
		int ind;
		ind = ComboBox_AddStringData(hlist, (LPARAM)freqnames[i], freqvals[i]);
		if (freqvals[i] == conf->cfgint[CFG_INT_TAPE_FREQ]) 
			ComboBox_SetCurSel(hlist, ind);
	}
	SetDlgItemText(hwnd, IDC_FNAME, conf->cfgstr[CFG_STR_TAPE]);
	CheckDlgButton(hwnd, IDC_FAST, conf->cfgint[CFG_INT_TAPE_FAST]?BST_CHECKED:BST_UNCHECKED);
	update_controls(hwnd);
	return 0;
}

static void dialog_destroy(HWND hwnd, void*p)
{
}

static int dialog_command(HWND hwnd, void*p, int notify, int id, HWND ctl)
{
	switch (id) {
	case IDC_CHOOSE:
		{
			TCHAR buf[CFGSTRLEN];
			int r;
			GetDlgItemText(hwnd, IDC_FNAME, buf, CFGSTRLEN);
			r = select_tape(hwnd, buf, -1);
			if (r!=-1) SetDlgItemText(hwnd, IDC_FNAME, buf);
		}
		break;
	}
	update_controls(hwnd);
	return 1;
}

static int dialog_ok(HWND hwnd, struct SLOTCONFIG*conf)
{
	HWND hlist = GetDlgItem(hwnd, IDC_FREQ);
	if (IsDlgButtonChecked(hwnd, IDC_NONE) == BST_CHECKED) conf->dev_type = DEV_TAPE_NONE;
	else if (IsDlgButtonChecked(hwnd, IDC_FILE) == BST_CHECKED) conf->dev_type = DEV_TAPE_FILE;
	conf->cfgint[CFG_INT_TAPE_FREQ] = ComboBox_GetItemData(hlist, ComboBox_GetCurSel(hlist));
	conf->cfgint[CFG_INT_TAPE_FAST] = IsDlgButtonChecked(hwnd, IDC_FAST) == BST_CHECKED;
	GetDlgItemText(hwnd, IDC_FNAME, conf->cfgstr[CFG_STR_TAPE], CFGSTRLEN);
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

int tapedlg_run(HWND hpar, struct SLOTCONFIG *conf)
{
	return dialog_run(&dialog, MAKEINTRESOURCE(IDD_TAPECFG), hpar, conf);
}
