#include <windows.h>
#include <commctrl.h>
#include "resize.h"
#include "dialog.h"
#include "sysconf.h"
#include "resource.h"

static struct RESIZE_DIALOG resize=
{
	RESIZE_LIMIT_MIN,
	{{237,142},{0,0}},
	8,
	{
		{IDOK,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDCANCEL,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{11,{RESIZE_ALIGN_NONE,RESIZE_ALIGN_CENTER}},
		{12,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_CENTER}},
		{13,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_CENTER}},
		{IDC_AGAT_7,{RESIZE_ALIGN_NONE,RESIZE_ALIGN_CENTER}},
		{IDC_AGAT_9,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_CENTER}},
		{IDC_APPLE_2,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_CENTER}},
	}
};

static int dialog_init(HWND hwnd, struct SLOTCONFIG*conf)
{
	return 0;
}

static void dialog_destroy(HWND hwnd, void*p)
{
}

static int dialog_command(HWND hwnd, void*p, int notify, int id, HWND ctl)
{
//	printf("%i %i\n", notify, id);
	EnableDlgItem(hwnd, IDOK, TRUE);
	SetFocus(GetDlgItem(hwnd, IDOK));
	switch (id) {
	case 11:
		CheckRadioButton(hwnd, IDC_AGAT_7, IDC_APPLE_2, IDC_AGAT_7);
		break;
	case 12:
		CheckRadioButton(hwnd, IDC_AGAT_7, IDC_APPLE_2, IDC_AGAT_9);
		break;
	case 13:
		CheckRadioButton(hwnd, IDC_AGAT_7, IDC_APPLE_2, IDC_APPLE_2);
		break;
	case IDC_AGAT_7:
	case IDC_AGAT_9:
	case IDC_APPLE_2:
//		EnableDlgItem(hwnd, IDOK, TRUE);
		return 0;
	}
	return 1;
}

static int dialog_ok(HWND hwnd, struct SLOTCONFIG*conf)
{
	int r = -1;
	if (IsDlgButtonChecked(hwnd, IDC_AGAT_7) == BST_CHECKED) r = SYSTEM_7;
	else if (IsDlgButtonChecked(hwnd, IDC_AGAT_9) == BST_CHECKED) r = SYSTEM_9;
	else if (IsDlgButtonChecked(hwnd, IDC_APPLE_2) == BST_CHECKED) r = SYSTEM_A;
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
