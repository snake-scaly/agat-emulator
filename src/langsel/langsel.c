#include <windows.h>
#include <tchar.h>
#include "resize.h"
#include "dialog.h"
#include "resource.h"
#include "localize.h"

#define LANG_DIR 	"lang"
#define INI_NAME	".\\emulator.ini"


static struct RESIZE_DIALOG resize=
{
	RESIZE_LIMIT_MIN | RESIZE_LIMIT_MAX,
	{{187,100},{300, 300}},
	3,
	{
		{IDOK,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_NONE}},
		{IDCANCEL,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_NONE}},
		{IDC_LANGS,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_BOTTOM}},
	}
};

static void update_controls(HWND hwnd)
{
	int n;
	BOOL b;
	n = ListBox_GetCurSel(GetDlgItem(hwnd, IDC_LANGS));
	b = n!=LB_ERR;
	EnableDlgItem(hwnd, IDOK, b);
}

static int scan_list(HWND hlist, LPCTSTR sel)
{
	HANDLE ff;
	WIN32_FIND_DATA fd;

	LockWindowUpdate(hlist);
	ListBox_ResetContent(hlist);

	ff = FindFirstFile(TEXT(LANG_DIR)TEXT("\\*.dll"), &fd);
	if (ff == INVALID_HANDLE_VALUE) {
		EnableWindow(hlist, FALSE);
		LockWindowUpdate(NULL);
		return -1;
	} else {
		EnableWindow(hlist, TRUE);
	}
	do {
		TCHAR buf[256];
		HMODULE mod;
		wsprintf(buf, TEXT(LANG_DIR)TEXT("\\%s"), fd.cFileName);
		mod = LoadLibrary(buf);
		if (mod) {
			if (LoadString(mod, BASE_LANG_STR_ID, buf, 256)) {
				int n;
				TCHAR *p;
				n = ListBox_AddString(hlist, buf);
				p = _tcsrchr(fd.cFileName, TEXT('.'));
				if (p) *p = 0;
				ListBox_SetItemData(hlist, n, _tcsdup(fd.cFileName));
				if (!_tcsicmp(fd.cFileName, sel)) {
					ListBox_SetCurSel(hlist, n);
				}
			}
			FreeLibrary(mod);
		}
	} while (FindNextFile(ff, &fd));
	FindClose(ff);
	LockWindowUpdate(NULL);
	return 0;
}

static int dialog_init(HWND hwnd, LPCTSTR lang)
{
	SetClassLong(hwnd, GCL_HICON, 
		(LONG)LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(1)));
	scan_list(GetDlgItem(hwnd, IDC_LANGS), lang);
	update_controls(hwnd);
	return 0;
}

static void dialog_destroy(HWND hwnd, LPCTSTR lang)
{
}

static int dialog_command(HWND hwnd, LPCSTR lang, int notify, int id, HWND ctl)
{
	if (id == IDC_LANGS && notify == LBN_DBLCLK) dialog_ok(hwnd, lang);
	update_controls(hwnd);
	return 0;
}


static int dialog_ok(HWND hwnd, LPCSTR lang)
{
	int n;
	HWND hlist = GetDlgItem(hwnd, IDC_LANGS);
	n = ListBox_GetCurSel(hlist);
	if (n != LB_ERR) {
		LPCTSTR lang = (LPCTSTR)ListBox_GetItemData(hlist, n);
		if (lang) {
			WritePrivateProfileString(TEXT("Environment"), TEXT("Lang"), lang, TEXT(INI_NAME));
		}
	}
	EndDialog(hwnd, TRUE);
	return 0;
}

static int dialog_cancel(HWND hwnd, LPCSTR lang)
{
	EndDialog(hwnd, FALSE);
	return 0;
}


static struct DIALOG_DATA dialog =
{
	&resize,

	dialog_init, 
	dialog_destroy,
	dialog_command,
	NULL,

	dialog_cancel,
	dialog_ok
};

int langdlg_run(HWND hpar, LPCTSTR lang)
{
	return dialog_run(&dialog, MAKEINTRESOURCE(IDD_LANGCFG), hpar, (LPTSTR)lang);
}


int CALLBACK WinMain(HINSTANCE hinst, HINSTANCE hprev, LPSTR cmdline, int cmdshow)
{
	TCHAR lang[256];
	resize_set_cfgname(TEXT(INI_NAME));
	GetPrivateProfileString(TEXT("Environment"), TEXT("Lang"), TEXT(""), lang, 256, TEXT(INI_NAME));
	localize_init();
	langdlg_run(NULL, lang);
	localize_term();
	return 0;
}
