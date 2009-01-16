#include <windows.h>
#include <commctrl.h>
#include "resize.h"
#include "dialog.h"
#include "sysconf.h"
#include "resource.h"

#include "localize.h"

#define DLG_ID IDD_SNDCFG

static struct RESIZE_DIALOG resize=
{
	RESIZE_LIMIT_MIN,
	{{192,145},{0,0}},
	13,
	{
		{IDOK,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDCANCEL,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDC_NONE,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_MMSYSTEM,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_DIRECTSOUND,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_BEEPER,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_FREQ,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_BUFSIZE,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{10,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{11,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_CENTER}},
		{12,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_CENTER}},
		{13,{RESIZE_ALIGN_NONE,RESIZE_ALIGN_CENTER}},
		{14,{RESIZE_ALIGN_NONE,RESIZE_ALIGN_CENTER}},
	}
};
/*
static const TCHAR*freqnames[]={
	TEXT("8000 Гц"),
	TEXT("11025 Гц"),
	TEXT("22050 Гц"),
	TEXT("44100 Гц"),
	TEXT("48000 Гц")
};
*/

static int freqvals[]={8000, 11025, 22050, 44100, 48000};

/*
static const TCHAR*bufsizes[]={
	TEXT("Малый"),
	TEXT("Средний"),
	TEXT("Большой"),
};
*/

static int bufszvals[]={2048, 8192, 16384};


static void upd_controls(HWND hwnd)
{
	BOOL b = FALSE;
	if (IsDlgButtonChecked(hwnd, IDC_MMSYSTEM) == BST_CHECKED) b = TRUE;
	else if (IsDlgButtonChecked(hwnd, IDC_DIRECTSOUND) == BST_CHECKED) b = TRUE;
	EnableDlgItem(hwnd, IDC_FREQ, b);
	EnableDlgItem(hwnd, IDC_BUFSIZE, b);
	EnableDlgItem(hwnd, 13, b);
	EnableDlgItem(hwnd, 11, b);
	EnableDlgItem(hwnd, 14, b);
}

static int dialog_init(HWND hwnd, struct SLOTCONFIG*conf)
{
	int i;
	HWND hlist = GetDlgItem(hwnd, IDC_FREQ);
	HWND hblist = GetDlgItem(hwnd, IDC_BUFSIZE);
	TCHAR buf[256];

	switch (conf->dev_type) {
	case DEV_NOSOUND:
		CheckDlgButton(hwnd, IDC_NONE, BST_CHECKED);
		break;
	case DEV_BEEPER:
		CheckDlgButton(hwnd, IDC_BEEPER, BST_CHECKED);
		break;
	case DEV_MMSYSTEM:
		CheckDlgButton(hwnd, IDC_MMSYSTEM, BST_CHECKED);
		break;
	case DEV_DSOUND:
		CheckDlgButton(hwnd, IDC_DIRECTSOUND, BST_CHECKED);
		break;
	}
	for (i = 0; i < sizeof(freqvals)/sizeof(freqvals[0]); ++i) {
		int ind;
		ind = ComboBox_AddStringData(hlist, 
			localize_str(LOC_SOUND, i, buf, sizeof(buf)), //freqnames[i], 
			freqvals[i]);
		if (freqvals[i] == conf->cfgint[CFG_INT_SOUND_FREQ]) 
			ComboBox_SetCurSel(hlist, ind);
	}
	for (i = 0; i < sizeof(bufszvals)/sizeof(bufszvals[0]); ++i) {
		int ind;
		ind = ComboBox_AddStringData(hblist, 
			localize_str(LOC_SOUND, i + 10, buf, sizeof(buf)), //bufsizes[i], 
			bufszvals[i]);
		if (bufszvals[i] == conf->cfgint[CFG_INT_SOUND_BUFSIZE]) 
			ComboBox_SetCurSel(hblist, ind);
	}
	upd_controls(hwnd);
	return 0;
}

static void dialog_destroy(HWND hwnd, void*p)
{
}


static int dialog_command(HWND hwnd, void*p, int notify, int id, HWND ctl)
{
	switch (id) {
	case IDC_NONE:
	case IDC_BEEPER:
	case IDC_MMSYSTEM:
	case IDC_DIRECTSOUND:
		upd_controls(hwnd);
		break;
	}
	return 1;
}

static int dialog_ok(HWND hwnd, struct SLOTCONFIG*conf)
{
	HWND hlist = GetDlgItem(hwnd, IDC_FREQ);
	HWND hblist = GetDlgItem(hwnd, IDC_BUFSIZE);
	if (IsDlgButtonChecked(hwnd, IDC_NONE) == BST_CHECKED) conf->dev_type = DEV_NOSOUND;
	else if (IsDlgButtonChecked(hwnd, IDC_BEEPER) == BST_CHECKED) conf->dev_type = DEV_BEEPER;
	else if (IsDlgButtonChecked(hwnd, IDC_MMSYSTEM) == BST_CHECKED) conf->dev_type = DEV_MMSYSTEM;
	else if (IsDlgButtonChecked(hwnd, IDC_DIRECTSOUND) == BST_CHECKED) conf->dev_type = DEV_DSOUND;
	conf->cfgint[CFG_INT_SOUND_FREQ] = ComboBox_GetItemData(hlist, ComboBox_GetCurSel(hlist));
	conf->cfgint[CFG_INT_SOUND_BUFSIZE] = ComboBox_GetItemData(hblist, ComboBox_GetCurSel(hblist));
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

int snddlg_run(HWND hpar, struct SLOTCONFIG *conf)
{
	return dialog_run(&dialog, MAKEINTRESOURCE(DLG_ID), hpar, conf);
}

