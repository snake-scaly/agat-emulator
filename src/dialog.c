#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include "dialog.h"
#include "localize.h"

HINSTANCE res_instance;



static BOOL CALLBACK dialog_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	int r;
	struct DIALOG_DATA*data = (struct DIALOG_DATA*)GetWindowLong(hwnd, DWL_USER);
	if (msg == WM_INITDIALOG) {
		SetWindowLong(hwnd, DWL_USER, lp);
		data = (struct DIALOG_DATA*)lp;
	}
//	printf("msg = %i\n", msg);
	if (!data) return FALSE;
	if (data&&data->message) {
		r = data->message(hwnd, data->param, msg, wp, lp);
		if (!r) return TRUE;
	}
	switch (msg) {
	case WM_INITDIALOG:
//		puts("init");
		r = data->init ? data->init(hwnd, data->param) : 0;
		if (r < 0) {
			EndDialog(hwnd, -1);
			return FALSE;
		}
		resize_init(data->resize);
		resize_attach(data->resize,hwnd);
		resize_init_placement(hwnd, data->res_id);
		return FALSE;
	case WM_DESTROY:
//		puts("destroy");
		resize_detach(data->resize,hwnd);
		resize_free(data->resize);
		resize_save_placement(hwnd, data->res_id);
		if (data->free) data->free(hwnd, data->param);
		return TRUE;
	case WM_CLOSE:
//		puts("close");
		if (data->close) data->close(hwnd, data->param);
		else if (data->cancel) data->cancel(hwnd, data->param);
		else EndDialog(hwnd, FALSE);
		break;
	case WM_SIZE:
//		puts("size");
		resize_realign(data->resize,hwnd,LOWORD(lp),HIWORD(lp));
		return 0;
	case WM_SIZING:
//		puts("sizing");
		resize_sizing(data->resize,hwnd,wp,(LPRECT)lp);
		return TRUE;
	case WM_COMMAND:
//		puts("command");
		if (HIWORD(wp) == BN_CLICKED) {
			switch (LOWORD(wp)) {
			case IDOK:
				r = data->ok ? data->ok(hwnd, data->param) :
					(data->command ? -1: (EndDialog(hwnd, TRUE),0));
				break;
			case IDCANCEL:
				r = data->cancel ? data->cancel(hwnd, data->param) : 
					(data->close ? data->close(hwnd, data->param) : 
						(data->command ? -1: (EndDialog(hwnd, FALSE),0)));
				break;
			case IDCLOSE:
				r = data->close ? data->close(hwnd, data->param) :
					(data->command ? -1: (EndDialog(hwnd, FALSE),0));
				break;
			case IDAPPLY:
				r = data->apply ? data->apply(hwnd, data->param) : -1;
				break;
			}
		}
		if (!r) break;
		r = data->command ? data->command(hwnd, data->param, HIWORD(wp), LOWORD(wp), (HWND)lp) : -1;
		if (!r) break;
		return FALSE;
	case WM_NOTIFY:
//		puts("notify");
		r = data->notify ? data->notify(hwnd, data->param, (int)wp, (LPNMHDR)lp): -1;
		if (!r) break;
		return FALSE;
	default:
		return FALSE;
	}
	return TRUE;
}


int dialog_run(struct DIALOG_DATA*data, LPCTSTR res_id, HWND hpar, void* param)
{
	if (!res_instance) res_instance = localize_get_lib();
	data->param = param;
	data->res_id = res_id;
	return DialogBoxParam(res_instance, res_id, hpar, dialog_proc, (LPARAM)data);
}

BOOL EnableDlgItem(HWND hpar, int id, BOOL enable)
{
	HWND hchild;
	hchild = GetDlgItem(hpar, id);
	if (!hchild) return FALSE;
	return EnableWindow(hchild, enable);
}

int ListBox_AddStringData(HWND hlist, LPCTSTR str, LPARAM data)
{
	int ind;
	ind = ListBox_AddString(hlist, str);
	if (ind == LB_ERR) return ind;
	ListBox_SetItemData(hlist, ind, data);
	return ind;
}

int ComboBox_AddStringData(HWND hlist, LPCTSTR str, LPARAM data)
{
	int ind;
	ind = ComboBox_AddString(hlist, str);
	if (ind == CB_ERR) return ind;
	ComboBox_SetItemData(hlist, ind, data);
	return ind;
}

int ListView_FindItemByData(HWND hlist, int ind, LPARAM data)
{
	LVFINDINFO fi;
	fi.flags = LVFI_PARAM;
	fi.lParam = data;
	return ListView_FindItem(hlist, ind, &fi);
}


LPARAM ListView_GetItemLParam(HWND hlist, int id)
{
	LVITEM it;
	ZeroMemory(&it, sizeof(it));
	it.iItem = id;
	it.mask = LVIF_PARAM;
	if (!ListView_GetItem(hlist, &it)) return 0;
	return it.lParam;
}


int ListView_GetCurSel(HWND hlist)
{
	return ListView_GetNextItem(hlist, -1, LVNI_FOCUSED);
}

void ListView_SetCurSel(HWND hlist, int no)
{
	ListView_SetItemState(hlist, no, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
}

