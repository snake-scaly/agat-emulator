#include <windows.h>
#include <commctrl.h>
#include "resize.h"
#include "dialog.h"
#include "common.h"
#include "localize.h"
#include "resource.h"

static struct RESIZE_DIALOG resize=
{
	RESIZE_LIMIT_MIN,
	{{278,170},{0,0}},
	12,
	{
		{IDOK,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDCANCEL,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDC_FULLSCREEN_DEFAULT,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_BACKGROUND_ACTIVE,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_SHUGART_SOUNDS,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_TEAC_SOUNDS,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_ENABLE_DEBUGGER,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_DEBUG_ILLEGAL_CMDS,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_DEBUG_NEW_CMDS,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{IDC_SYNC_UPDATE,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}},
		{100,{RESIZE_ALIGN_NONE,RESIZE_ALIGN_CENTER}},
		{IDC_LANGSEL,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_CENTER}}
	}
};

#define SET_FLAG(f)\
	if (conf->flags & EMUL_FLAGS_##f) \
		CheckDlgButton(hwnd, IDC_##f, BST_CHECKED)

#define CHECK_FLAG(f)\
	if (IsDlgButtonChecked(hwnd, IDC_##f) == BST_CHECKED)\
		conf->flags |= EMUL_FLAGS_##f; \
	else \
		conf->flags &= ~EMUL_FLAGS_##f


static void List_AddItem(HWND hlist, int id, int selid)
{
	int no;
	TCHAR buf[256];
	localize_str(LOC_MAIN, id, buf, sizeof(buf));
	no = ComboBox_AddStringData(hlist, buf, id);
	if (id == selid) {
		ComboBox_SetCurSel(hlist, no);
	}
}

static int dialog_init(HWND hwnd, struct GLOBAL_CONFIG*conf)
{
	SET_FLAG(FULLSCREEN_DEFAULT);
	SET_FLAG(BACKGROUND_ACTIVE);
	SET_FLAG(SHUGART_SOUNDS);
	SET_FLAG(TEAC_SOUNDS);
	SET_FLAG(ENABLE_DEBUGGER);
	SET_FLAG(DEBUG_ILLEGAL_CMDS);
	SET_FLAG(DEBUG_NEW_CMDS);
	SET_FLAG(SYNC_UPDATE);
	List_AddItem(GetDlgItem(hwnd, IDC_LANGSEL), 100, (conf->flags & EMUL_FLAGS_LANGSEL)?101:100);
	List_AddItem(GetDlgItem(hwnd, IDC_LANGSEL), 101, (conf->flags & EMUL_FLAGS_LANGSEL)?101:100);
	return 0;
}

static void dialog_destroy(HWND hwnd, void*p)
{
}

static int dialog_command(HWND hwnd, void*p, int notify, int id, HWND ctl)
{
	return 1;
}

static int dialog_ok(HWND hwnd, struct GLOBAL_CONFIG*conf)
{
	int n;
	CHECK_FLAG(FULLSCREEN_DEFAULT);
	CHECK_FLAG(BACKGROUND_ACTIVE);
	CHECK_FLAG(SHUGART_SOUNDS);
	CHECK_FLAG(TEAC_SOUNDS);
	CHECK_FLAG(ENABLE_DEBUGGER);
	CHECK_FLAG(DEBUG_ILLEGAL_CMDS);
	CHECK_FLAG(DEBUG_NEW_CMDS);
	CHECK_FLAG(SYNC_UPDATE);
	n = ComboBox_GetItemData(GetDlgItem(hwnd, IDC_LANGSEL), ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_LANGSEL)));
	if (n == 101) conf->flags |= EMUL_FLAGS_LANGSEL;
	else conf->flags &= ~EMUL_FLAGS_LANGSEL;
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

int global_config(HWND hpar, struct GLOBAL_CONFIG*conf)
{
	return dialog_run(&dialog, MAKEINTRESOURCE(IDD_GCONFIG), hpar, conf);
}
