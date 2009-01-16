#include <windows.h>
#include <windowsx.h>

#ifndef IDAPPLY
#define IDAPPLY IDYES
#endif

struct DIALOG_DATA
{
	const struct RESIZE_DIALOG*resize;

	int  (*init)(HWND hwnd, void*param);
	void (*free)(HWND hwnd, void*param);
	int  (*command)(HWND hwnd, void*param, int notify, int id, HWND hctl);
	int  (*notify)(HWND hwnd, void*param, int id, LPNMHDR hdr);

// convenience subroutines
	int  (*close)(HWND hwnd, void*param);
	int  (*ok)(HWND hwnd, void*param);
	int  (*cancel)(HWND hwnd, void*param);
	int  (*apply)(HWND hwnd, void*param);

// low-level subroutine
	int  (*message)(HWND hwnd, void*param, UINT msg, WPARAM wp, LPARAM lp);
	
// internal fields
	void* param;
	LPCTSTR res_id;
};

extern HINSTANCE res_instance;

int dialog_run(struct DIALOG_DATA*data, LPCTSTR res_id, HWND hpar, void* param);


// WinAPI extension
BOOL EnableDlgItem(HWND hpar, int id, BOOL enable);
int ListBox_AddStringData(HWND hlist, LPCTSTR str, LPARAM data);
int ComboBox_AddStringData(HWND hlist, LPCTSTR str, LPARAM data);
int ListView_FindItemByData(HWND hlist, int ind, LPARAM data);
LPARAM ListView_GetItemLParam(HWND hlist, int id);
int ListView_GetCurSel(HWND hlist);
void ListView_SetCurSel(HWND hlist, int no);
