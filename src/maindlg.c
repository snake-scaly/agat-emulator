#include <windows.h>
#include <commctrl.h>
#include "resize.h"
#include "dialog.h"
#include "sysconf.h"
#include "resource.h"
#include "streams.h"
#include "logo.h"
#include "runstate.h"
#include "systemstate.h"

extern HINSTANCE intface_inst;

static struct RESIZE_DIALOG resize =
{
	RESIZE_LIMIT_MIN | RESIZE_NO_SIZEBOX,
	{{187,179},{0,0}},
	10,
	{
		{IDC_CFGLIST,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_BOTTOM}},
		{IDOK,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_NONE}},
		{IDC_STOP,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_NONE}},
		{IDCANCEL,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_NONE}},
		{11,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_NONE}},
		{IDC_NEW,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_CENTER}},
		{IDC_CONFIG,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_CENTER}},
		{IDC_DELETE,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_CENTER}},
		{12,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_TOP}},
		{IDC_ABOUT,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_TOP}},

	}
};

static HIMAGELIST him;

static HBITMAP get_cfg_image(HWND wnd, LPCTSTR fname)
{
	ISTREAM*in;
	HBITMAP bm;
	struct SYSCONFIG cfg;
	HDC dc;
	TCHAR pathname[_MAX_PATH];
	wsprintf(pathname, TEXT(SYSTEMS_DIR"\\%s"), fname);
	in = isfopen(pathname);
	if (!in) return NULL;
	clear_config(&cfg);
	load_config(&cfg, in);
	dc = GetDC(wnd);
	bm = sysicon_to_bitmap(dc, &cfg.icon);
	ReleaseDC(wnd, dc);
	free_config(&cfg);
	isclose(in);
	return bm;
}


void update_save_state(LPCTSTR name)
{
	TCHAR buf[1024];
	wsprintf(buf, TEXT(SAVES_DIR"\\%s.sav"), name);
	if (!_taccess(buf, 04)) {
		or_run_state_flags(name, RUNSTATE_SAVED);
	} else {
		and_run_state_flags(name, ~RUNSTATE_SAVED);
	}
}

static int scan_configs(HWND hwnd)
{
	HWND hlist;
	HANDLE ff;
	WIN32_FIND_DATA fd;

	hlist = GetDlgItem(hwnd, IDC_CFGLIST);
	LockWindowUpdate(hlist);
	ImageList_RemoveAll(him);
	ListView_DeleteAllItems(hlist);

	ff = FindFirstFile(TEXT(SYSTEMS_DIR"\\*.cfg"), &fd);
	if (ff == INVALID_HANDLE_VALUE) {
		EnableWindow(hlist, FALSE);
		LockWindowUpdate(NULL);
		return -1;
	} else {
		EnableWindow(hlist, TRUE);
	}
	do {
		LVITEM it;
		int img;
		TCHAR*p;
		TCHAR buf[MAX_PATH];
		HBITMAP bmp;
		ZeroMemory(&it, sizeof(it));
		it.mask = LVIF_TEXT | LVIF_IMAGE;
		it.pszText = fd.cFileName;
		it.iItem = ListView_GetItemCount(hlist);
		bmp = get_cfg_image(hwnd, fd.cFileName);
		p = _tcsrchr(fd.cFileName, TEXT('.'));
		if (p) *p = 0;
		update_save_state(fd.cFileName);
		img = ImageList_Add(him, bmp, NULL);
		it.iImage = img;
		ListView_InsertItem(hlist, &it);
	} while (FindNextFile(ff, &fd));
	LockWindowUpdate(NULL);
	return 0;
}

static int update_configs(HWND hwnd)
{
	HWND hlist;
	int i, n;
	hlist = GetDlgItem(hwnd, IDC_CFGLIST);
	n = ListView_GetItemCount(hlist);
	for (i = 0; i < n; ++i) {
		TCHAR buf[1024];
		ListView_GetItemText(hlist, i, 0, buf, 1024);
		update_save_state(buf);
	}
	return 0;
}

static int update_controls(HWND hwnd)
{
	HWND hlist;
	int n;
	hlist = GetDlgItem(hwnd, IDC_CFGLIST);
	n = ListView_GetSelectedCount(hlist);
//	printf("n selected = %i\n", n);
	EnableDlgItem(hwnd, IDOK, n == 1);
	EnableDlgItem(hwnd, IDC_CONFIG, n == 1);
	EnableDlgItem(hwnd, IDC_DELETE, n);
	if (n == 1) {
		TCHAR fname[_MAX_PATH];
		unsigned fl;
		n = ListView_GetNextItem(hlist, -1, LVNI_SELECTED);
		if (n == -1) return -1;
		ListView_GetItemText(hlist, n, 0, fname, _MAX_PATH);
		fl = get_run_state_flags(fname);
		if (fl & RUNSTATE_RUNNING && get_run_state_ptr(fname)) {
			EnableDlgItem(hwnd, IDC_STOP, TRUE);
		} else {
			EnableDlgItem(hwnd, IDC_STOP, FALSE);
		}
	}
	return 0;
}

static int dialog_init(HWND hwnd, void*p)
{
	HWND hlist;
	init_run_states();
	CreateLogoWindow(hwnd);

	him = ImageList_Create(64, 64, ILC_COLOR24, 0, 5);

	hlist = GetDlgItem(hwnd, IDC_CFGLIST);

	ListView_SetImageList(hlist, him, LVSIL_NORMAL);

	ListView_SetExtendedListViewStyle(hlist, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	scan_configs(hwnd);
	update_controls(hwnd);
	SetClassLong(hwnd, GCL_HICON, (LONG)LoadIcon(intface_inst, MAKEINTRESOURCE(IDI_MAIN)));
	return 0;
}

static void dialog_destroy(HWND hwnd, void*p)
{
//	ImageList_Destroy(him);
	free_run_states();
}


static int run_config(HWND hwnd)
{
	struct SYSCONFIG *cfg;
	TCHAR fname[_MAX_PATH];
	TCHAR pathname[_MAX_PATH];
	HWND  hlist;
	ISTREAM*in;
	struct SYS_RUN_STATE*sr;
	int r, n;
	int use_save = 0;

	hlist = GetDlgItem(hwnd, IDC_CFGLIST);

	n = ListView_GetNextItem(hlist, -1, LVNI_SELECTED);
	if (n == -1) return -1;
	ListView_GetItemText(hlist, n, 0, fname, _MAX_PATH);

	{
		unsigned fl = get_run_state_flags(fname);
		if (fl & RUNSTATE_RUNNING && (sr = get_run_state_ptr(fname))) {
			int r;
			r = MessageBox(hwnd, TEXT("������� ��� ��������. ������������� �� ��?\n(��� - ��������� ������)"), TEXT("���������� �������"), MB_ICONQUESTION|MB_YESNOCANCEL);
			switch (r) {
			case IDYES:
				system_command(sr, SYS_COMMAND_ACTIVATE, 0, 0);
				return 0;
			case IDNO:
				free_system_state(sr);
				break;
			default:
				return -1;
			}
		}
		if (fl & RUNSTATE_SAVED) {
			int r;
			r = MessageBox(hwnd, TEXT("������� ����������� ��������� �������. ��������� ���?"), TEXT("����������� ���������"), MB_ICONQUESTION|MB_YESNOCANCEL);
			switch (r) {
			case IDYES:
				use_save = 1;
				break;
			case IDNO:
				break;
			default:
				return -1;
			}
		}
	}

	wsprintf(pathname, TEXT(SYSTEMS_DIR"\\%s.cfg"), fname);

	cfg = malloc( sizeof(*cfg));

	clear_config(cfg);
	in = isfopen(pathname);
	if (!in) {
		return -2;
	}
	r = load_config(cfg, in);
	isclose(in);
	if (r) return -3;

	sr = init_system_state(cfg, hwnd, fname);
	if (!sr) return -4;
	if (use_save) {
		if (load_system_state_file(sr) < 0) {
			MessageBox(hwnd, TEXT("��� �������� ��������� �������� ������"), TEXT("����������� ���������"), MB_ICONEXCLAMATION);
			free_system_state(sr);
			return -1;
		}
	}
	system_command(sr, SYS_COMMAND_START, 0, 0);
	return 0;
}

static int stop_config(HWND hwnd)
{
	TCHAR fname[_MAX_PATH];
	HWND  hlist;
	struct SYS_RUN_STATE*sr;
	int r, n;

	hlist = GetDlgItem(hwnd, IDC_CFGLIST);

	n = ListView_GetNextItem(hlist, -1, LVNI_SELECTED);
	if (n == -1) return -1;
	ListView_GetItemText(hlist, n, 0, fname, _MAX_PATH);

	{
		unsigned fl = get_run_state_flags(fname);
		if (fl & RUNSTATE_RUNNING && (sr = get_run_state_ptr(fname))) {
			int r;
			TCHAR buf[1024];
			wsprintf(buf, TEXT("���������� ���������� ������� �%s�?"), fname);
			r = MessageBox(hwnd, buf, TEXT("������"), MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2);
			switch (r) {
			case IDYES:
				free_system_state(sr);
				return 0;
			default:
				return -1;
			}
		}
	}
	return -1;
}

static int delete_save(LPCTSTR fname)
{
	TCHAR pathname[_MAX_PATH];
	wsprintf(pathname, TEXT(SAVES_DIR"\\%s.sav"), fname);
	return DeleteFile(pathname)?0:-1;
}


static int new_config(HWND hwnd)
{
	struct SYSCONFIG cfg;
	TCHAR fname[CFGSTRLEN]="", *p;
	int r;
	OSTREAM*out;
	r = syschoosedlg_run(hwnd);
	if (r == FALSE) return 1;
	clear_config(&cfg);
	reset_config(&cfg, r - TRUE);
	r = cfgdlg_run(hwnd, &cfg);
	if (!r) return 1;
	r = select_save_config(hwnd, fname);
	if (!r) return 1;
	out = osfopen(fname);
	if (!out) {
		return -2;
	}
	r = save_config(&cfg, out);
	free_config(&cfg);
	osclose(out);
	if (r) {
		DeleteFile(fname);
		MessageBox(hwnd, TEXT("������ �������� �������"), NULL, MB_ICONEXCLAMATION | MB_OK);
		return -1;
	}
	p = _tcsrchr(fname, TEXT('.'));
	if (p) *p = 0;
	p = _tcsrchr(fname, TEXT('\\'));
	if (p) *p++ = 0; else p = fname;
	delete_save(p);
	scan_configs(hwnd);
	return 0;
}


static int change_config(HWND hwnd)
{
	int n;
	struct SYSCONFIG cfg;
	TCHAR fname[_MAX_PATH];
	TCHAR pathname[_MAX_PATH];
	HWND  hlist;
	ISTREAM*in;
	OSTREAM*out;
	int r;

	hlist = GetDlgItem(hwnd, IDC_CFGLIST);

	n = ListView_GetNextItem(hlist, -1, LVNI_SELECTED);
	if (n == -1) return -1;
	ListView_GetItemText(hlist, n, 0, fname, _MAX_PATH);

	{
		unsigned fl = get_run_state_flags(fname);
		if (fl & RUNSTATE_RUNNING && get_run_state_ptr(fname)) {
			int r;
			r = MessageBox(hwnd, TEXT("�������������� ������� ������ ��������. �������� ��������� ������� � �������� ������ ����� � ���������� �������."), TEXT("�����������"), MB_ICONINFORMATION|MB_OKCANCEL);
			switch (r) {
			case IDOK:
				break;
			default:
				return -1;
			}
				
		}
		if (fl & RUNSTATE_SAVED) {
			int r;
			r = MessageBox(hwnd, TEXT("�������������� ������� ����� ����������� ���������. ����� ��������� ������������ ������� ��� ��������� ����� ��������. ����������?"), TEXT("��������� ����������� �������"), MB_ICONQUESTION|MB_YESNO);
			switch (r) {
			case IDYES:
				break;
			default:
				return -1;
			}
		}
	}
	
	wsprintf(pathname, TEXT(SYSTEMS_DIR"\\%s.cfg"), fname);


	clear_config(&cfg);
	in = isfopen(pathname);
	if (!in) {
		return -2;
	}
	r = load_config(&cfg, in);
	isclose(in);
	if (r) return -3;
	r = cfgdlg_run(hwnd, &cfg);
	if (!r) { free_config(&cfg); return 1; }

	out = osfopen(pathname);
	if (!out) {
		free_config(&cfg);
		return -5;
	}
	r = save_config(&cfg, out);
	free_config(&cfg);
	osclose(out);

	delete_save(fname);

	scan_configs(hwnd);
	return 0;
}

static int delete_config(HWND hwnd)
{
	int n = -1;
	HWND hlist;
	TCHAR fname[_MAX_PATH];
	TCHAR msg[_MAX_PATH];
	int nd = 0;

	hlist = GetDlgItem(hwnd, IDC_CFGLIST);
	do {
		int r;
		n = ListView_GetNextItem(hlist, n, LVNI_SELECTED);
		if (n == -1) break;
		ListView_GetItemText(hlist, n, 0, fname, _MAX_PATH);
		wsprintf(msg, TEXT("������� ������� \xAB%s\xBB?"), fname);
		r = MessageBox(hwnd, msg, TEXT("�������������"), MB_ICONQUESTION | MB_YESNOCANCEL);
		if (r == IDYES) {
			wsprintf(msg, TEXT(SYSTEMS_DIR"\\%s.cfg"), fname);
			if (!DeleteFile(msg)) {
				MessageBox(hwnd, TEXT("������ �������� �������"), NULL, MB_ICONEXCLAMATION | MB_OK);
			} else {
				delete_save(fname);
				++nd;
			}
		}
		if (r == IDCANCEL) break;
	} while (1);
	if (nd) scan_configs(hwnd);
	return 0;
}


static void on_quit(HWND hwnd)
{
	int n;
	n = get_n_running_systems();
	if (n) {
		int r;
		r = MessageBox(hwnd, TEXT("������� ���������� �������. ��������� ������?"), TEXT("���������� ������"), MB_ICONQUESTION | MB_YESNO);
		switch (r) {
		case IDYES:
			free_all_running_systems();
			break;
		default:
			return;
		}
	}
	PostQuitMessage(0);
	EndDialog(hwnd, 1);
}

static int dialog_close(HWND hwnd, void*p)
{
	on_quit(hwnd);
	return 0;
}

static int dialog_command(HWND hwnd, void*p, int notify, int id, HWND ctl)
{
	switch (id) {
	case IDOK:
		run_config(hwnd);
		return 0;
	case IDC_STOP:
		stop_config(hwnd);
		return 0;
	case IDC_UPDATE:
		update_controls(hwnd);
		update_configs(hwnd);
		return 0;
	case IDCANCEL:
		on_quit(hwnd);
		return 0;
	case IDC_ABOUT:
		aboutdlg_run(hwnd);
		return 0;
	case IDC_NEW:
		if (new_config(hwnd) < 0) MessageBeep(MB_ICONEXCLAMATION);
		return 0;
	case IDC_CONFIG:
		if (change_config(hwnd) < 0) MessageBeep(MB_ICONEXCLAMATION);
		return 0;
	case IDC_DELETE:
		if (delete_config(hwnd) < 0) MessageBeep(MB_ICONEXCLAMATION);
		return 0;
	}
	return 1;
}

static int dialog_notify(HWND hwnd, void*p, int id, LPNMHDR hdr)
{
	switch (id) {
	case IDC_CFGLIST: 
		switch (hdr->code) {
		case LVN_ITEMCHANGED:
			update_controls(hwnd);
			return 0;
		case NM_DBLCLK:
			run_config(hwnd);
			return 0;
		}
		break;
	}
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
};

int maindlg_run(HWND hpar)
{
	register_video_window();
	_mkdir(SYSTEMS_DIR);
	_mkdir(SAVES_DIR);
	return dialog_run(&dialog, MAKEINTRESOURCE(IDD_MAIN), hpar, NULL);
}