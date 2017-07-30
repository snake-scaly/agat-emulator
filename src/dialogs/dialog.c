#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include "dialog.h"
#include "localize.h"
#include "msgloop.h"

HINSTANCE res_instance = NULL;

static BOOL CALLBACK dialog_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	int r = -1;
	BOOL res = TRUE;
	struct DIALOG_DATA*data = (struct DIALOG_DATA*)GetWindowLong(hwnd, DWL_USER);
	if (msg == WM_INITDIALOG) {
		SetWindowLong(hwnd, DWL_USER, lp);
		data = (struct DIALOG_DATA*)lp;
	}
//	printf("msg = %i\n", msg);
	if (!data) return FALSE;
	switch (msg) {
	case WM_SIZE:
//		puts("size");
		resize_realign(data->resize,hwnd,LOWORD(lp),HIWORD(lp));
		res = FALSE;
		break;
	case WM_GETMINMAXINFO:
		resize_minmaxinfo(data->resize,hwnd,(LPMINMAXINFO)lp);
		res = FALSE;
		break;
	}
	if (data&&data->message) {
		r = data->message(hwnd, data->param, msg, wp, lp);
		if (!r) return TRUE;
	}
	switch (msg) {
	case WM_INITDIALOG:
//		puts("init");
		r = data->init ? data->init(hwnd, data->param) : 0;
		if (r < 0) {
			EndDialog(hwnd, -2);
			return FALSE;
		}
		resize_init(data->resize);
		resize_attach(data->resize,hwnd);
		resize_init_placement(hwnd, data->res_id);
		if (data&&data->message) data->message(hwnd, data->param, WM_SIZE, 0, 0);
		return FALSE;
	case WM_DESTROY:
//		puts("destroy");
		resize_detach(data->resize,hwnd);
		resize_free(data->resize);
		resize_save_placement(hwnd, data->res_id);
		if (data->free) data->free(hwnd, data->param);
		/* Automatically unregister modeless dialogs when destroyed. */
		if (data->modeless_hwnd)
		{
			msgloop_unregister_modeless_dialog(data);
			data->modeless_hwnd = NULL;
		}
		return TRUE;
	case WM_CLOSE:
//		puts("close");
		if (data->close) data->close(hwnd, data->param);
		else if (data->cancel) data->cancel(hwnd, data->param);
		/* EndDialog() is only for modal dialogs. Modeless dialogs
		   need to be simply destroyed. */
		else if (data->modeless_hwnd) DestroyWindow(hwnd);
		else EndDialog(hwnd, FALSE);
		break;
	case WM_COMMAND:
//		puts("command");
		if (HIWORD(wp) == BN_CLICKED) {
			switch (LOWORD(wp)) {
			case IDOK:
				if (data->ok) r = data->ok(hwnd, data->param);
				else if (data->command) r = -1;
				else if (data->modeless_hwnd) r = DestroyWindow(hwnd), 0;
				else r = EndDialog(hwnd, TRUE), 0;
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
	return res;
}


int dialog_run(struct DIALOG_DATA*data, LPCTSTR res_id, HWND hpar, void* param)
{
	int r;
	HINSTANCE inst = res_instance;
	data->param = param;
	data->res_id = res_id;
	if (!inst) {
		inst = localize_get_lib();
		r = DialogBoxParam(inst, res_id, hpar, dialog_proc, (LPARAM)data);
		if (r != -1) return r;
		inst = localize_get_def_lib();
	}
	r = DialogBoxParam(inst, res_id, hpar, dialog_proc, (LPARAM)data);
	return r;
}

void dialog_init_modeless(struct DIALOG_DATA*data)
{
	list_init(&data->modeless_list);
	data->modeless_hwnd = NULL;
}

static int dialog_show_modeless_inst(struct DIALOG_DATA*data, HWND hpar, int cmd_show, HINSTANCE inst)
{
	data->modeless_hwnd = CreateDialogParam(inst, data->res_id, hpar, dialog_proc, (LPARAM)data);
	if (!data->modeless_hwnd) return -1;
	msgloop_register_modeless_dialog(data);
	if (cmd_show != -1) ShowWindow(data->modeless_hwnd, cmd_show);
	return 0;
}

/*
	Show a modeless dialog box.

	data - dialog data instance. This DIALOG_DATA must have been initialized
	       either using dialog_init_modeless() or by statically initializing
	       the data->modeless_list member using the LIST_NODE_INIT() macro
	res_id - dialog resource identifier
	hpar - handle of the parent window. Can be NULL
	cmd_show - command for ShowWindow. Pass -1 to use the resource default
	param - user parameter for dialog callbacks
*/
int dialog_show_modeless(struct DIALOG_DATA*data, LPCTSTR res_id, HWND hpar, int cmd_show, void*param)
{
	HINSTANCE inst = res_instance;
	data->param = param;
	data->res_id = res_id;
	if (!inst) {
		inst = localize_get_lib();
		if (dialog_show_modeless_inst(data, hpar, cmd_show, inst) == 0) return 0;
		inst = localize_get_def_lib();
	}
	return dialog_show_modeless_inst(data, hpar, cmd_show, inst);
}

int dialog_hide_modeless(struct DIALOG_DATA*data)
{
	return DestroyWindow(data->modeless_hwnd) ? 0 : -1;
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

