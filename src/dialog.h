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
