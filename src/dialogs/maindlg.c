#include <windows.h>
#include <htmlhelp.h>
#include <commctrl.h>
#include "common.h"
#include "resize.h"
#include "dialog.h"
#include "sysconf.h"
#include "resource.h"
#include "streams.h"
#include "logo.h"
#include "runstate.h"
#include "systemstate.h"

#include "localize.h"


static struct RESIZE_DIALOG resize =
{
	RESIZE_LIMIT_MIN | RESIZE_NO_SIZEBOX,
	{{187,200},{0,0}},
	12,
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
		{IDC_CALLHELP,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_TOP}},
		{IDC_ABOUT,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_TOP}},
		{IDC_GCONFIG,{RESIZE_ALIGN_LEFT,RESIZE_ALIGN_TOP}},

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

static int main_cfg_get_sel(HWND wnd)
{
	return ListView_GetNextItem(GetDlgItem(wnd,IDC_CFGLIST),-1,LVNI_SELECTED);
}

static void main_cfg_set_sel(HWND wnd,int sel)
{
	if (sel!=-1) {
		HWND list=GetDlgItem(wnd,IDC_CFGLIST);
		ListView_SetItemState(list,sel,LVNI_SELECTED,LVNI_SELECTED);
	}
}

static int scan_configs(HWND hwnd)
{
	HWND hlist;
	HANDLE ff;
	WIN32_FIND_DATA fd;
	int sel;

	hlist = GetDlgItem(hwnd, IDC_CFGLIST);
	sel = main_cfg_get_sel(hwnd);
	SendMessage(hlist,WM_SETREDRAW,0,0);
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
	FindClose(ff);
	main_cfg_set_sel(hwnd, sel);
	SendMessage(hlist,WM_SETREDRAW,1,0);
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
	SetClassLong(hwnd, GCL_HICON, 
		(LONG)LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN)));
	return 0;
}

static void dialog_destroy(HWND hwnd, void*p)
{
//	ImageList_Destroy(him);
	free_run_states();
}


static int run_config(HWND hwnd, LPCTSTR fname)
{
	struct SYSCONFIG *cfg;
	TCHAR pathname[_MAX_PATH];
	ISTREAM*in;
	struct SYS_RUN_STATE*sr;
	int r;
	int use_save = 0;
	TCHAR buf[2][256];

	printf("running config %s\n", fname);

	{
		unsigned fl = get_run_state_flags(fname);
		if (fl & RUNSTATE_RUNNING && (sr = get_run_state_ptr(fname))) {
			int r;
			r = MessageBox(hwnd, 
				localize_str(LOC_MAIN, 0, buf[0], sizeof(buf[0])), //TEXT("Система уже запущена. Переключиться на неё?\n(Нет - запустить заново)"), 
				localize_str(LOC_MAIN, 1, buf[1], sizeof(buf[1])), //TEXT("Запущенная система"),
				MB_ICONQUESTION|MB_YESNOCANCEL);
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
			r = MessageBox(hwnd,
				localize_str(LOC_MAIN, 2, buf[0], sizeof(buf[0])), //TEXT("Имеется сохранённое состояние системы. Загрузить его?"),
				localize_str(LOC_MAIN, 3, buf[1], sizeof(buf[1])), //TEXT("Сохранённые состояния"), 
				MB_ICONQUESTION|MB_YESNOCANCEL);
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
	if (!sr) {
		MessageBox(hwnd, 
			localize_str(LOC_MAIN, 10, buf[0], sizeof(buf[0])), //TEXT("При инициализации системы возникла ошибка"),
			localize_str(LOC_MAIN, 11, buf[1], sizeof(buf[1])), //TEXT("Инициализация системы"), 
			MB_ICONEXCLAMATION);
		return -4;
	}
	{
		const char*fs = sys_get_parameter("fs");
		if (!fs && (g_config.flags & EMUL_FLAGS_FULLSCREEN_DEFAULT)) fs = "1";
		if (fs) set_fullscreen(sr, atoi(fs));
	}
	if (use_save) {
		if (load_system_state_file(sr) < 0) {
			MessageBox(hwnd,
				localize_str(LOC_MAIN, 12, buf[0], sizeof(buf[0])), //TEXT("При загрузке состояния возникла ошибка"),
				localize_str(LOC_MAIN, 13, buf[1], sizeof(buf[1])), //TEXT("Сохранённые состояния"),
				MB_ICONEXCLAMATION);
			free_system_state(sr);
			return -1;
		}
	}
	system_command(sr, SYS_COMMAND_START, 0, 0);
	return 0;
}

static int run_cur_config(HWND hwnd)
{
	HWND  hlist;
	int n;
	TCHAR fname[_MAX_PATH];

	hlist = GetDlgItem(hwnd, IDC_CFGLIST);

	n = ListView_GetNextItem(hlist, -1, LVNI_SELECTED);
	if (n == -1) return -1;
	ListView_GetItemText(hlist, n, 0, fname, _MAX_PATH);
	return run_config(hwnd, fname);
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
			TCHAR buf[1024], buf1[256];
			wsprintf(buf, 
				localize_str(LOC_MAIN, 20, buf1, sizeof(buf1)), //TEXT("Остановить выполнение системы «%s»?"), 
				fname);
			r = MessageBox(hwnd, buf, 
				localize_str(LOC_MAIN, 21, buf1, sizeof(buf1)), //TEXT("Запрос"), 
				MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2);
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
		TCHAR buf1[256];
		DeleteFile(fname);
		MessageBox(hwnd,
			localize_str(LOC_MAIN, 30, buf1, sizeof(buf1)), //TEXT("Ошибка создания системы"),
			NULL, MB_ICONEXCLAMATION | MB_OK);
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
			TCHAR buf[2][256];
			r = MessageBox(hwnd,
				localize_str(LOC_MAIN, 40, buf[0], sizeof(buf[0])), //TEXT("Модифицируемая система сейчас запущена. Вносимые изменения вступят в действие только после её повторного запуска."),
				localize_str(LOC_MAIN, 41, buf[1], sizeof(buf[1])), //TEXT("Напоминание"), 
				MB_ICONINFORMATION|MB_OKCANCEL);
			switch (r) {
			case IDOK:
				break;
			default:
				return -1;
			}
				
		}
		if (fl & RUNSTATE_SAVED) {
			int r;
			TCHAR buf[2][256];
			r = MessageBox(hwnd,
				localize_str(LOC_MAIN, 50, buf[0], sizeof(buf[0])), //TEXT("Модифицируемая система имеет сохранённое состояние. После изменения конфигурации системы это состояние будет потеряно. Продолжить?"), 
				localize_str(LOC_MAIN, 51, buf[1], sizeof(buf[1])), //TEXT("Изменение сохраненной системы"),
				MB_ICONQUESTION|MB_YESNO);
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
		TCHAR buf[256];
		n = ListView_GetNextItem(hlist, n, LVNI_SELECTED);
		if (n == -1) break;
		ListView_GetItemText(hlist, n, 0, fname, _MAX_PATH);
		wsprintf(msg, 
			localize_str(LOC_MAIN, 60, buf, sizeof(buf)), //TEXT("Удалить систему \xAB%s\xBB?"), 
			fname);
		r = MessageBox(hwnd, msg, 
			localize_str(LOC_MAIN, 61, buf, sizeof(buf)), //TEXT("Подтверждение"), 
			MB_ICONQUESTION | MB_YESNOCANCEL);
		if (r == IDYES) {
			wsprintf(msg, TEXT(SYSTEMS_DIR"\\%s.cfg"), fname);
			if (!DeleteFile(msg)) {
				MessageBox(hwnd, 
					localize_str(LOC_MAIN, 62, buf, sizeof(buf)), //TEXT("Ошибка удаления системы"), 
					NULL, MB_ICONEXCLAMATION | MB_OK);
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
		TCHAR buf[2][256];
		r = MessageBox(hwnd,
			localize_str(LOC_MAIN, 70, buf[0], sizeof(buf[0])), //TEXT("Имеются запущенные системы. Завершить работу?"),
			localize_str(LOC_MAIN, 71, buf[1], sizeof(buf[1])), //TEXT("Завершение работы"), 
			MB_ICONQUESTION | MB_YESNO);
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

static void on_help(HWND hwnd)
{
	extern TCHAR lang[256];
	TCHAR path[MAX_PATH];
	wsprintf(path, TEXT("%s\\%s.chm"), HELP_DIR, lang);
	HtmlHelp(hwnd, path, HH_DISPLAY_TOC, 0);
}

static int dialog_command(HWND hwnd, void*p, int notify, int id, HWND ctl)
{
	switch (id) {
	case IDOK:
		run_cur_config(hwnd);
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
	case IDC_CALLHELP:
		on_help(hwnd);
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
	case IDC_GCONFIG:
		global_config(hwnd, &g_config);
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
			run_cur_config(hwnd);
			return 0;
		}
		break;
	}
	return 1;
}

static int  dialog_message(HWND hwnd, void*param, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_HELP:
		on_help(hwnd);
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
	NULL,
	dialog_close,
	NULL,
	dialog_message
};

int maindlg_run(HWND hpar)
{
	_mkdir(SYSTEMS_DIR);
	_mkdir(SAVES_DIR);
	return dialog_run(&dialog, MAKEINTRESOURCE(IDD_MAIN), hpar, NULL);
}


int maindlg_run_config(HWND hpar, LPCTSTR name)
{
	int r;
	MSG msg;
	_mkdir(SYSTEMS_DIR);
	_mkdir(SAVES_DIR);
	update_save_state(name);
	r = run_config(hpar, name);
	if (r < 0) return r;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}
