#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include <commdlg.h>
#include "Shlwapi.h"
#include "resize.h"
#include "dialog.h"
#include "resource.h"
#include "sysconf.h"

#include "localize.h"

static struct RESIZE_DIALOG resize=
{
	RESIZE_LIMIT_MIN,
	{{210,161},{0,0}},
	4,
	{
		{IDC_SYSTYPE,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_NONE}},
		{IDOK,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDCANCEL,{RESIZE_ALIGN_CENTER,RESIZE_ALIGN_TOP}},
		{IDC_PERIPHERAL,{RESIZE_ALIGN_RIGHT,RESIZE_ALIGN_BOTTOM}},
	}
};


int select_rom(HWND hpar, TCHAR fname[CFGSTRLEN])
{
	OPENFILENAME ofn;
	TCHAR path[MAX_PATH], buf[2][256];
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hpar;
	ofn.lpstrFilter = localize_str(LOC_CFG, 0, buf[0], sizeof(buf[0])); //TEXT("Файлы ПЗУ (*.rom)\0*.rom\0Все файлы\0*.*\0");
	ofn.lpstrFile = fname;
	ofn.nMaxFile = CFGSTRLEN;
	ofn.lpstrTitle = localize_str(LOC_CFG, 1, buf[1], sizeof(buf[1])); //TEXT("Выбор файла ПЗУ");
	GetModuleFileName(NULL, path, MAX_PATH);
	PathRemoveFileSpec(path);
	ofn.lpstrInitialDir = path;
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
	if (!GetOpenFileName(&ofn)) return FALSE;
	{
		TCHAR buf[MAX_PATH];
		if (PathRelativePathTo(buf, path, FILE_ATTRIBUTE_DIRECTORY, fname, 0))
			lstrcpy(fname, buf);
	}
	return TRUE;
}

int select_save_config(HWND hpar, TCHAR fname[CFGSTRLEN])
{
	OPENFILENAME ofn;
	TCHAR path[MAX_PATH], buf[2][256];
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hpar;
	ofn.lpstrFilter = localize_str(LOC_CFG, 2, buf[0], sizeof(buf[0]));  //TEXT("Файлы конфигурации (*.cfg)\0*.cfg\0Все файлы\0*.*\0");
	ofn.lpstrFile = fname;
	ofn.nMaxFile = CFGSTRLEN;
	ofn.lpstrTitle = localize_str(LOC_CFG, 3, buf[1], sizeof(buf[1])); //TEXT("Выбор файла конфигурации");
	GetModuleFileName(NULL, path, MAX_PATH);
	PathRemoveFileSpec(path);
	_tcscat(path, TEXT("\\"SYSTEMS_DIR));
	ofn.lpstrInitialDir = path;
	ofn.lpstrDefExt = TEXT("cfg");
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
	if (!GetSaveFileName(&ofn)) return FALSE;
/*	{
		TCHAR buf[MAX_PATH];
		if (PathRelativePathTo(buf, path, FILE_ATTRIBUTE_DIRECTORY, fname, 0))
			lstrcpy(fname, buf);
	}*/
	return TRUE;
}


int select_font(HWND hpar, TCHAR fname[CFGSTRLEN])
{
	OPENFILENAME ofn;
	TCHAR path[MAX_PATH], buf[2][256];
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hpar;
	ofn.lpstrFilter = localize_str(LOC_CFG, 4, buf[0], sizeof(buf[0])); //TEXT("Файлы шрифтов (*.fnt)\0*.fnt\0Все файлы\0*.*\0");
	ofn.lpstrFile = fname;
	ofn.nMaxFile = CFGSTRLEN;
	ofn.lpstrTitle = localize_str(LOC_CFG, 5, buf[1], sizeof(buf[1])); //TEXT("Выбор файла знакогенератора");
	GetModuleFileName(NULL, path, MAX_PATH);
	PathRemoveFileSpec(path);
	ofn.lpstrInitialDir = path;
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
	if (!GetOpenFileName(&ofn)) return FALSE;
	{
		TCHAR buf[MAX_PATH];
		if (PathRelativePathTo(buf, path, FILE_ATTRIBUTE_DIRECTORY, fname, 0))
			lstrcpy(fname, buf);
	}
	return TRUE;
}


int select_disk(HWND hpar, TCHAR fname[CFGSTRLEN], int*readonly)
{
	OPENFILENAME ofn;
	TCHAR path[MAX_PATH], buf[2][256];
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hpar;
	ofn.lpstrFilter = localize_str(LOC_CFG, 6, buf[0], sizeof(buf[0])); //TEXT("Файлы образов (*.dsk)\0*.dsk\0Файлы \"сырых\" данных (*.nib)\0*.nib\0Все файлы\0*.*\0");
	ofn.lpstrFile = fname;
	ofn.nMaxFile = CFGSTRLEN;
	ofn.lpstrTitle = localize_str(LOC_CFG, 7, buf[1], sizeof(buf[1])); //TEXT("Выбор файла образа диска");
	GetModuleFileName(NULL, path, MAX_PATH);
	PathRemoveFileSpec(path);
	ofn.lpstrInitialDir = path;
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_READONLY | OFN_NOCHANGEDIR;
	if (readonly&&!*readonly) ofn.Flags &= ~OFN_READONLY;
	if (!GetOpenFileName(&ofn)) return FALSE;
	if (readonly) {
		*readonly = (ofn.Flags & OFN_READONLY) ? 1: 0;
	}
	{
		TCHAR buf[MAX_PATH];
		if (PathRelativePathTo(buf, path, FILE_ATTRIBUTE_DIRECTORY, fname, 0))
			lstrcpy(fname, buf);
	}
	return TRUE;
}


int select_tape(HWND hpar, TCHAR fname[CFGSTRLEN], int load)
{
	OPENFILENAME ofn;
	TCHAR path[MAX_PATH], buf[2][256];
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hpar;
	ofn.lpstrFilter = localize_str(LOC_CFG, 8, buf[0], sizeof(buf[0])); //TEXT("Файлы ленты (*.wav)\0*.wav\0Все файлы\0*.*\0");
	ofn.lpstrFile = fname;
	ofn.nMaxFile = CFGSTRLEN;
	GetModuleFileName(NULL, path, MAX_PATH);
	PathRemoveFileSpec(path);
	_tcscat(path, TEXT("\\tapes"));
	ofn.lpstrInitialDir = path;
	ofn.lpstrDefExt = TEXT("wav");
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
	switch (load) {
	case 1:
		ofn.lpstrTitle = localize_str(LOC_CFG, 9, buf[1], sizeof(buf[1])); //TEXT("Выбор файла ленты для чтения");
		ofn.Flags |= OFN_FILEMUSTEXIST;
		break;
	case 0:
		ofn.lpstrTitle = localize_str(LOC_CFG, 10, buf[1], sizeof(buf[1])); //TEXT("Выбор файла ленты для записи");
		ofn.Flags |= OFN_OVERWRITEPROMPT;
		break;
	case -1:
		ofn.lpstrTitle = localize_str(LOC_CFG, 11, buf[1], sizeof(buf[1])); //TEXT("Выбор файла ленты");
		break;
	}
	if (!(load?GetOpenFileName(&ofn):GetSaveFileName(&ofn))) return FALSE;
	{
		TCHAR buf[MAX_PATH];
		GetModuleFileName(NULL, path, MAX_PATH);
		PathRemoveFileSpec(path);
		if (PathRelativePathTo(buf, path, FILE_ATTRIBUTE_DIRECTORY, fname, 0))
			lstrcpy(fname, buf);
	}
	return TRUE;
}



static int set_per_cfg(HWND hlist, int id, int type, LPTSTR comment)
{
	LVITEM item;
	int ind;
	ZeroMemory(&item,sizeof(item));
	ind = ListView_FindItemByData(hlist, -1, id);
	item.mask = LVIF_PARAM | LVIF_TEXT;
	item.pszText = (LPTSTR)get_confnames(id);
	item.iSubItem = 0;
	item.lParam = (LPARAM)id;
	if (ind == -1) {
		item.iItem = ListView_GetItemCount(hlist);
		item.iItem = ind = ListView_InsertItem(hlist, &item);
	} else {
		item.iItem = ind;
		ListView_SetItem(hlist, &item);
	}
	if (ind == -1) return ind;

	item.mask=LVIF_TEXT;
	item.pszText = (LPTSTR)get_devnames(type);
	item.iSubItem = 1;
	ListView_SetItem(hlist, &item);

	item.pszText = comment;
	item.iSubItem = 2;
	ListView_SetItem(hlist, &item);
	return ind;
}

static int set_per_comment(HWND hlist, int id, LPTSTR comment)
{
	LVITEM item;
	int ind;

	ZeroMemory(&item,sizeof(item));
	ind = ListView_FindItemByData(hlist, -1, id);
	if (ind == -1) return ind;

	item.mask=LVIF_TEXT;
	item.pszText = comment;
	item.iItem = ind;
	item.iSubItem = 2;
	ListView_SetItem(hlist, &item);
	return 0;
}

static int update_list(HWND hwnd, struct SYSCONFIG*conf)
{
	int i, id;
	HWND hlist;
	hlist = GetDlgItem(hwnd, IDC_PERIPHERAL);
	id = ListView_GetCurSel(hlist);
	ListView_DeleteAllItems(hlist);

	for (i = 0; i < NCONFTYPES; i++) {
		TCHAR buf[1024];
		if (!get_confnames(i)) continue;
		if (i >= CONF_EXT && conf->slots[i].dev_type == DEV_NULL) {
			continue;
		}
		get_slot_comment(conf->slots + i, buf);
		set_per_cfg(hlist, i, conf->slots[i].dev_type, buf);
	}
	if (id != -1) ListView_SetCurSel(hlist, id);
	return 0;
}

static int select_sys(HWND hwnd, struct SYSCONFIG*conf, int id)
{
	reset_config(conf, id);
	update_list(hwnd, conf);
	return 0;
}

static int slot_configure(HWND hwnd, struct SLOTCONFIG *slot)
{
	switch (slot->dev_type) {
	case DEV_FDD_TEAC:
		return teaccfgdlg_run(hwnd, slot);
	case DEV_FDD_SHUGART:
		return shugcfgdlg_run(hwnd, slot);
	case DEV_MEMORY_PSROM7:
	case DEV_MEMORY_XRAM7:
	case DEV_MEMORY_XRAM9:
	case DEV_MEMORY_XRAMA: {
		int r;
		r = memdlg_run(hwnd, slot->cfgint[CFG_INT_MEM_MASK], 
				slot->cfgint[CFG_INT_MEM_SIZE]);
		if (r) {
			slot->cfgint[CFG_INT_MEM_SIZE] = r;
		} else return FALSE;
		return TRUE; }
	
	case DEV_VIDEOTERM:
		return vtermdlg_run(hwnd, slot);
	case DEV_THUNDERCLOCK:
		return select_rom(hwnd, slot->cfgstr[CFG_STR_ROM]);
	}

	switch (slot->slot_no) {
		case CONF_CPU:
			return cpudlg_run(hwnd, slot);
		case CONF_MEMORY: {
			int r;
			r = memdlg_run(hwnd, slot->cfgint[CFG_INT_MEM_MASK], 
					slot->cfgint[CFG_INT_MEM_SIZE]);
			if (r) {
				slot->cfgint[CFG_INT_MEM_SIZE] = r;
			} else return FALSE;
			return TRUE; }
		case CONF_ROM:
			return select_rom(hwnd, slot->cfgstr[CFG_STR_ROM]);
		case CONF_CHARSET:
			return select_font(hwnd, slot->cfgstr[CFG_STR_ROM]);
		case CONF_SOUND:
			return snddlg_run(hwnd, slot);
		case CONF_JOYSTICK:
			return joydlg_run(hwnd, slot);
		case CONF_MONITOR:
			return mondlg_run(hwnd, slot);
		case CONF_TAPE:
			return tapedlg_run(hwnd, slot);
	}
	return TRUE;
}

static int change_item(HWND hwnd, struct SYSCONFIG* conf, HWND hlist, int ind, int id, int sub)
{
	int syst = SendDlgItemMessage(hwnd, IDC_SYSTYPE, CB_GETCURSEL, 0, 0);

	if ((!sub && id < CONF_EXT) || conf->slots[id].dev_type == DEV_NULL) {
		int r;
		struct SLOTCONFIG newslot = conf->slots[id];
		r = devseldlg_run(hwnd, syst);
		if (r != -1) {
			r = reset_slot_config(&newslot, r, syst);
			r = slot_configure(hwnd, &newslot);
			if (r != TRUE) {
				return 1;
			}
			conf->slots[id] = newslot;
			return 0;
		}
	} else {
		int r;
		r = slot_configure(hwnd, conf->slots + id);
		if (r != TRUE) {
			return 1;
		}
		return 0;
	}
	return -1;
}

static int dialog_init(HWND hwnd, struct SYSCONFIG*conf)
{
	HWND hlist;
	int i;

	hlist = GetDlgItem(hwnd, IDC_SYSTYPE);
	for (i = 0; i < NSYSTYPES; i++) {
		ComboBox_AddString(hlist, get_sysnames(i));
	}
	ComboBox_SetCurSel(hlist, conf->systype);

	hlist = GetDlgItem(hwnd, IDC_PERIPHERAL);
	{
		LVCOLUMN col;
		TCHAR buf[256];
		memset(&col,0,sizeof(col));
		col.mask=LVCF_TEXT|LVCF_WIDTH;
		col.pszText=(LPTSTR)localize_str(LOC_CFG, 20, buf, sizeof(buf)); //TEXT("Ресурс");
		col.cx=100;
		ListView_InsertColumn(hlist,0,&col);
		col.pszText=(LPTSTR)localize_str(LOC_CFG, 21, buf, sizeof(buf)); //TEXT("Устройство");
		col.cx=300;
		ListView_InsertColumn(hlist,1,&col);
		col.pszText=(LPTSTR)localize_str(LOC_CFG, 22, buf, sizeof(buf)); //TEXT("Информация");
		col.cx=300;
		ListView_InsertColumn(hlist,2,&col);
	}


	ListView_SetExtendedListViewStyle(hlist, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	resize_loadlistviewcolumns(hlist, "devlist");
	update_list(hwnd, conf);
	return 0;
}

static void dialog_destroy(HWND hwnd, struct SYSCONFIG*conf)
{
	HWND hlist;
	hlist = GetDlgItem(hwnd, IDC_PERIPHERAL);
	resize_savelistviewcolumns(hlist, "devlist");
}

static int dialog_command(HWND hwnd, struct SYSCONFIG*conf, int notify, int id, HWND ctl)
{
	switch (id) {
	case IDOK:
		EndDialog(hwnd, TRUE);
		return 0;
	case IDCANCEL:
		EndDialog(hwnd, FALSE);
		return 0;
	case IDC_SYSTYPE:
		switch (notify) {
		case CBN_SELCHANGE:
			select_sys(hwnd, conf, SendMessage(ctl, CB_GETCURSEL, 0, 0));
			return 0;
		}
		break;
	}
	return 1;
}

static int dialog_notify(HWND hwnd, struct SYSCONFIG*conf, int id, LPNMHDR hdr)
{
	int r;
	switch (id) {
	case IDC_PERIPHERAL:
		switch (hdr->code) {
		case LVN_ITEMACTIVATE:
			r = change_item(hwnd, conf, hdr->hwndFrom, ((LPNMITEMACTIVATE)hdr) -> iItem, 
					ListView_GetItemLParam(hdr->hwndFrom, ((LPNMITEMACTIVATE)hdr) -> iItem),
					((LPNMITEMACTIVATE)hdr) -> iSubItem);
			if (!r) update_list(hwnd, conf);
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

};


int cfgdlg_run(HWND hpar, struct SYSCONFIG*conf)
{
	struct SYSCONFIG cfg1;
	int r;
	if (conf) {
		cfg1 = *conf;
	} else {
		clear_config(&cfg1);
		reset_config(&cfg1, SYSTEM_7);
	}
	r = dialog_run(&dialog, MAKEINTRESOURCE(IDD_CONFIG), hpar, &cfg1);
	if (r) {
		if (conf) *conf = cfg1;
		else free_config(&cfg1);
	}
	return r;
}
