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

static void repl_at(LPTSTR str)
{
	for (;*str; ++str) if (*str == TEXT('@')) *str = 0;
}

int select_rom(HWND hpar, TCHAR fname[CFGSTRLEN])
{
	OPENFILENAME ofn;
	TCHAR path[MAX_PATH], buf[2][256];
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hpar;
	ofn.lpstrFilter = localize_str(LOC_CFG, 0, buf[0], sizeof(buf[0])); //TEXT("����� ��� (*.rom)\0*.rom\0��� �����\0*.*\0");
	repl_at(buf[0]);
	ofn.lpstrFile = fname;
	ofn.nMaxFile = CFGSTRLEN;
	ofn.lpstrTitle = localize_str(LOC_CFG, 1, buf[1], sizeof(buf[1])); //TEXT("����� ����� ���");
	GetCurrentDirectory(MAX_PATH, path);
//	GetModuleFileName(NULL, path, MAX_PATH);
//	PathRemoveFileSpec(path);
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


int select_ram(HWND hpar, TCHAR fname[CFGSTRLEN])
{
	OPENFILENAME ofn;
	TCHAR path[MAX_PATH], buf[2][256];
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hpar;
	ofn.lpstrFilter = localize_str(LOC_CFG, 18, buf[0], sizeof(buf[0])); //TEXT("����� ��� (*.rom)\0*.rom\0��� �����\0*.*\0");
	repl_at(buf[0]);
	ofn.lpstrFile = fname;
	ofn.nMaxFile = CFGSTRLEN;
	ofn.lpstrTitle = localize_str(LOC_CFG, 19, buf[1], sizeof(buf[1])); //TEXT("����� ����� ���");
	GetCurrentDirectory(MAX_PATH, path);
//	GetModuleFileName(NULL, path, MAX_PATH);
//	PathRemoveFileSpec(path);
	ofn.lpstrInitialDir = path;
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
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
	ofn.lpstrFilter = localize_str(LOC_CFG, 2, buf[0], sizeof(buf[0]));  //TEXT("����� ������������ (*.cfg)\0*.cfg\0��� �����\0*.*\0");
	repl_at(buf[0]);
	ofn.lpstrFile = fname;
	ofn.nMaxFile = CFGSTRLEN;
	ofn.lpstrTitle = localize_str(LOC_CFG, 3, buf[1], sizeof(buf[1])); //TEXT("����� ����� ������������");
	GetCurrentDirectory(MAX_PATH, path);
//	GetModuleFileName(NULL, path, MAX_PATH);
//	PathRemoveFileSpec(path);
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


int select_save_text(HWND hpar, TCHAR fname[CFGSTRLEN])
{
	OPENFILENAME ofn;
	TCHAR path[MAX_PATH], buf[2][256];
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hpar;
	ofn.lpstrFilter = localize_str(LOC_CFG, 12, buf[0], sizeof(buf[0]));
	repl_at(buf[0]);
	ofn.lpstrFile = fname;
	ofn.nMaxFile = CFGSTRLEN;
	ofn.lpstrTitle = localize_str(LOC_CFG, 13, buf[1], sizeof(buf[1]));
	GetCurrentDirectory(MAX_PATH, path);
//	GetModuleFileName(NULL, path, MAX_PATH);
//	PathRemoveFileSpec(path);
	_tcscat(path, TEXT("\\"PRNOUT_DIR));
	ofn.lpstrInitialDir = path;
	ofn.lpstrDefExt = TEXT("txt");
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
	if (!GetSaveFileName(&ofn)) return FALSE;
	return TRUE;
}


int select_open_text(HWND hpar, TCHAR fname[CFGSTRLEN])
{
	OPENFILENAME ofn;
	TCHAR path[MAX_PATH], buf[2][256];
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hpar;
	ofn.lpstrFilter = localize_str(LOC_CFG, 12, buf[0], sizeof(buf[0]));
	repl_at(buf[0]);
	ofn.lpstrFile = fname;
	ofn.nMaxFile = CFGSTRLEN;
	ofn.lpstrTitle = localize_str(LOC_CFG, 24, buf[1], sizeof(buf[1]));
	GetCurrentDirectory(MAX_PATH, path);
//	GetModuleFileName(NULL, path, MAX_PATH);
//	PathRemoveFileSpec(path);
	_tcscat(path, TEXT("\\"TEXTS_DIR));
	ofn.lpstrInitialDir = path;
	ofn.lpstrDefExt = TEXT("txt");
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
	if (!GetOpenFileName(&ofn)) return FALSE;
	return TRUE;
}


int select_save_bin(HWND hpar, TCHAR fname[CFGSTRLEN])
{
	OPENFILENAME ofn;
	TCHAR path[MAX_PATH], buf[2][256];
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hpar;
	ofn.lpstrFilter = localize_str(LOC_CFG, 14, buf[0], sizeof(buf[0])); 
	repl_at(buf[0]);
	ofn.lpstrFile = fname;
	ofn.nMaxFile = CFGSTRLEN;
	ofn.lpstrTitle = localize_str(LOC_CFG, 15, buf[1], sizeof(buf[1]));
	GetCurrentDirectory(MAX_PATH, path);
//	GetModuleFileName(NULL, path, MAX_PATH);
//	PathRemoveFileSpec(path);
	_tcscat(path, TEXT("\\"PRNOUT_DIR));
	ofn.lpstrInitialDir = path;
	ofn.lpstrDefExt = TEXT("bin");
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
	if (!GetSaveFileName(&ofn)) return FALSE;
	return TRUE;
}


int select_save_dump(HWND hpar, TCHAR fname[CFGSTRLEN])
{
	OPENFILENAME ofn;
	TCHAR path[MAX_PATH], buf[2][256];
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hpar;
	ofn.lpstrFilter = localize_str(LOC_CFG, 14, buf[0], sizeof(buf[0])); 
	repl_at(buf[0]);
	ofn.lpstrFile = fname;
	ofn.nMaxFile = CFGSTRLEN;
	ofn.lpstrTitle = localize_str(LOC_CFG, 15, buf[1], sizeof(buf[1]));
	GetCurrentDirectory(MAX_PATH, path);
//	GetModuleFileName(NULL, path, MAX_PATH);
//	PathRemoveFileSpec(path);
	ofn.lpstrInitialDir = path;
	ofn.lpstrDefExt = TEXT("bin");
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
	if (!GetSaveFileName(&ofn)) return FALSE;
	return TRUE;
}

int select_open_dump(HWND hpar, TCHAR fname[CFGSTRLEN])
{
	OPENFILENAME ofn;
	TCHAR path[MAX_PATH], buf[2][256];
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hpar;
	ofn.lpstrFilter = localize_str(LOC_CFG, 14, buf[0], sizeof(buf[0])); 
	repl_at(buf[0]);
	ofn.lpstrFile = fname;
	ofn.nMaxFile = CFGSTRLEN;
	ofn.lpstrTitle = localize_str(LOC_CFG, 25, buf[1], sizeof(buf[1]));
	GetCurrentDirectory(MAX_PATH, path);
//	GetModuleFileName(NULL, path, MAX_PATH);
//	PathRemoveFileSpec(path);
	ofn.lpstrInitialDir = path;
	ofn.lpstrDefExt = TEXT("bin");
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
	if (!GetOpenFileName(&ofn)) return FALSE;
	return TRUE;
}


int select_save_tiff(HWND hpar, TCHAR fname[CFGSTRLEN])
{
	OPENFILENAME ofn;
	TCHAR path[MAX_PATH], buf[2][256];
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hpar;
	ofn.lpstrFilter = localize_str(LOC_CFG, 16, buf[0], sizeof(buf[0])); 
	repl_at(buf[0]);
	ofn.lpstrFile = fname;
	ofn.nMaxFile = CFGSTRLEN;
	ofn.lpstrTitle = localize_str(LOC_CFG, 17, buf[1], sizeof(buf[1]));
	GetCurrentDirectory(MAX_PATH, path);
//	GetModuleFileName(NULL, path, MAX_PATH);
//	PathRemoveFileSpec(path);
	_tcscat(path, TEXT("\\"PRNOUT_DIR));
	ofn.lpstrInitialDir = path;
	ofn.lpstrDefExt = TEXT("tiff");
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
	if (!GetSaveFileName(&ofn)) return FALSE;
	return TRUE;
}


int select_font(HWND hpar, TCHAR fname[CFGSTRLEN])
{
	OPENFILENAME ofn;
	TCHAR path[MAX_PATH], buf[2][256];
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hpar;
	ofn.lpstrFilter = localize_str(LOC_CFG, 4, buf[0], sizeof(buf[0])); //TEXT("����� ������� (*.fnt)\0*.fnt\0��� �����\0*.*\0");
	repl_at(buf[0]);
	ofn.lpstrFile = fname;
	ofn.nMaxFile = CFGSTRLEN;
	ofn.lpstrTitle = localize_str(LOC_CFG, 5, buf[1], sizeof(buf[1])); //TEXT("����� ����� ���������������");
	GetCurrentDirectory(MAX_PATH, path);
//	GetModuleFileName(NULL, path, MAX_PATH);
//	PathRemoveFileSpec(path);
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
	TCHAR path[MAX_PATH], buf[2][1024];
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hpar;
	ofn.lpstrFilter = localize_str(LOC_CFG, 6, buf[0], sizeof(buf[0])); //TEXT("����� ������� (*.dsk)\0*.dsk\0����� \"�����\" ������ (*.nib)\0*.nib\0��� �����\0*.*\0");
	repl_at(buf[0]);
	ofn.lpstrFile = fname;
	ofn.nMaxFile = CFGSTRLEN;
	ofn.lpstrTitle = localize_str(LOC_CFG, 7, buf[1], sizeof(buf[1])); //TEXT("����� ����� ������ �����");
	GetCurrentDirectory(MAX_PATH, path);
//	GetModuleFileName(NULL, path, MAX_PATH);
//	PathRemoveFileSpec(path);
	ofn.lpstrInitialDir = path;
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_READONLY | OFN_NOCHANGEDIR;
	if (!readonly) ofn.Flags |= OFN_HIDEREADONLY;
	if (!readonly||!*readonly) ofn.Flags &= ~OFN_READONLY;
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
	ofn.lpstrFilter = localize_str(LOC_CFG, 8, buf[0], sizeof(buf[0])); //TEXT("����� ����� (*.wav)\0*.wav\0��� �����\0*.*\0");
	repl_at(buf[0]);
	ofn.lpstrFile = fname;
	ofn.nMaxFile = CFGSTRLEN;
	GetCurrentDirectory(MAX_PATH, path);
//	GetModuleFileName(NULL, path, MAX_PATH);
//	PathRemoveFileSpec(path);
	_tcscat(path, TEXT("\\"TAPES_DIR));
	ofn.lpstrInitialDir = path;
	ofn.lpstrDefExt = TEXT("wav");
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
	switch (load) {
	case 1:
		ofn.lpstrTitle = localize_str(LOC_CFG, 9, buf[1], sizeof(buf[1])); //TEXT("����� ����� ����� ��� ������");
		ofn.Flags |= OFN_FILEMUSTEXIST;
		break;
	case 0:
		ofn.lpstrTitle = localize_str(LOC_CFG, 10, buf[1], sizeof(buf[1])); //TEXT("����� ����� ����� ��� ������");
		ofn.Flags |= OFN_OVERWRITEPROMPT;
		break;
	case -1:
		ofn.lpstrTitle = localize_str(LOC_CFG, 11, buf[1], sizeof(buf[1])); //TEXT("����� ����� �����");
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



int select_keyb(HWND hpar, TCHAR fname[CFGSTRLEN])
{
	OPENFILENAME ofn;
	TCHAR path[MAX_PATH], buf[2][256];
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hpar;
	ofn.lpstrFilter = localize_str(LOC_CFG, 20, buf[0], sizeof(buf[0]));
	repl_at(buf[0]);
	ofn.lpstrFile = fname;
	ofn.nMaxFile = CFGSTRLEN;
	ofn.lpstrTitle = localize_str(LOC_CFG, 21, buf[1], sizeof(buf[1]));
	GetCurrentDirectory(MAX_PATH, path);
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

int select_pal(HWND hpar, TCHAR fname[CFGSTRLEN])
{
	OPENFILENAME ofn;
	TCHAR path[MAX_PATH], buf[2][256];
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hpar;
	ofn.lpstrFilter = localize_str(LOC_CFG, 22, buf[0], sizeof(buf[0]));
	repl_at(buf[0]);
	ofn.lpstrFile = fname;
	ofn.nMaxFile = CFGSTRLEN;
	ofn.lpstrTitle = localize_str(LOC_CFG, 23, buf[1], sizeof(buf[1]));
	GetCurrentDirectory(MAX_PATH, path);
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

static int update_list(HWND hwnd, struct SYSCONFIG*conf, int systype)
{
	int i, id;
	HWND hlist;
	hlist = GetDlgItem(hwnd, IDC_PERIPHERAL);
	id = ListView_GetCurSel(hlist);
	ListView_DeleteAllItems(hlist);

	for (i = 0; i < NCONFTYPES; i++) {
		TCHAR buf[1024];
		if (!conf_present[systype][i]) continue;
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
	update_list(hwnd, conf, id);
	return 0;
}

static int slot_configure(HWND hwnd, struct SLOTCONFIG *slot, int initial)
{
	switch (slot->dev_type) {
	case DEV_FDD_TEAC:
		return teaccfgdlg_run(hwnd, slot);
	case DEV_FDD_SHUGART:
		return shugcfgdlg_run(hwnd, slot);
	case DEV_FDD_ATOM:
		return fddaacfgdlg_run(hwnd, slot);
	case DEV_MEMORY_PSROM7:
	case DEV_MEMORY_XRAM7:
	case DEV_MEMORY_XRAM9:
	case DEV_MEMORY_XRAMA:
	case DEV_MEMORY_SATURN: {
		int r;
		r = memdlg_run(hwnd, slot->cfgint[CFG_INT_MEM_MASK], 
				slot->cfgint[CFG_INT_MEM_SIZE]);
		if (r) {
			slot->cfgint[CFG_INT_MEM_SIZE] = r;
		} else return FALSE;
		return TRUE; }
	case DEV_A2RAMCARD:
	case DEV_RAMFACTOR: {
		int r;
		r = memdlg_run(hwnd, slot->cfgint[CFG_INT_MEM_MASK], 
				slot->cfgint[CFG_INT_MEM_SIZE]);
		if (r) {
			slot->cfgint[CFG_INT_MEM_SIZE] = r;
		} else return FALSE;
		select_ram(hwnd, slot->cfgstr[CFG_STR_RAM]);
		return TRUE; }
	
	case DEV_VIDEOTERM:
		return vtermdlg_run(hwnd, slot);
	case DEV_THUNDERCLOCK:
		return initial?TRUE:select_rom(hwnd, slot->cfgstr[CFG_STR_ROM]);
	case DEV_ACI:
		return initial?TRUE:select_rom(hwnd, slot->cfgstr[CFG_STR_ROM]);
	case DEV_FIRMWARE:
		return firmwdlg_run(hwnd, slot);
	case DEV_MOUSE_PAR:
	case DEV_PRINTER9:
		return prn9dlg_run(hwnd, slot);
	case DEV_PRINTERA:
		return prnadlg_run(hwnd, slot);
	case DEV_PRINTER_ATOM:
		return prnaadlg_run(hwnd, slot);
	case DEV_EXTROM_ATOM:
		return select_rom(hwnd, slot->cfgstr[CFG_STR_ROM]);
	case DEV_MOUSE_APPLE:
		return initial?TRUE:select_rom(hwnd, slot->cfgstr[CFG_STR_ROM]);
	case DEV_SCSI_CMS:
		return scsicfgdlg_run(hwnd, slot);
	case DEV_TTYA1:
		return ttya1dlg_run(hwnd, slot);
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
			if (slot->cfgint[CFG_INT_ROM_FLAGS] & CFG_INT_ROM_FLAG_A1)
				return roma1dlg_run(hwnd, slot);
			else
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
		case CONF_KEYBOARD:
			return select_keyb(hwnd, slot->cfgstr[CFG_STR_ROM]);
		case CONF_PALETTE:
			return select_pal(hwnd, slot->cfgstr[CFG_STR_ROM]);
	}
	return TRUE;
}

static int change_item(HWND hwnd, struct SYSCONFIG* conf, HWND hlist, int ind, int id, int sub)
{
	int syst = SendDlgItemMessage(hwnd, IDC_SYSTYPE, CB_GETCURSEL, 0, 0);

	if ((!sub && id < CONF_EXT) || conf->slots[id].dev_type == DEV_NULL) {
		int r;
		struct SLOTCONFIG newslot = conf->slots[id];
		r = devseldlg_run(hwnd, syst, id);
		if (r != -2) {
			r = reset_slot_config(&newslot, r, syst);
			r = slot_configure(hwnd, &newslot, 1);
			if (r != TRUE) {
				return 1;
			}
			conf->slots[id] = newslot;
			return 0;
		}
	} else {
		int r;
		r = slot_configure(hwnd, conf->slots + id, 0);
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
		col.pszText=(LPTSTR)localize_str(LOC_CFG, 100, buf, sizeof(buf)); //TEXT("������");
		col.cx=100;
		ListView_InsertColumn(hlist,0,&col);
		col.pszText=(LPTSTR)localize_str(LOC_CFG, 101, buf, sizeof(buf)); //TEXT("����������");
		col.cx=300;
		ListView_InsertColumn(hlist,1,&col);
		col.pszText=(LPTSTR)localize_str(LOC_CFG, 102, buf, sizeof(buf)); //TEXT("����������");
		col.cx=300;
		ListView_InsertColumn(hlist,2,&col);
	}


	ListView_SetExtendedListViewStyle(hlist, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	resize_loadlistviewcolumns(hlist, "devlist");
	update_list(hwnd, conf, conf->systype);
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
			if (!r) update_list(hwnd, conf, conf->systype);
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
