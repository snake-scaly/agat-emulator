/*
	Agat Emulator
	Copyright (c) NOP, nnop@newmail.ru
*/

#include "prnprogressdlg.h"

#include "dialog.h"
#include "msgloop.h"
#include "resize.h"
#include "resource.h"

#include <assert.h>
#include <windows.h>
#include <strsafe.h>

enum
{
	RECV_TEXT_BUF_SIZE = 32,

	BYTES_TIMER_ID = 1,
	BYTES_UPDATE_DELAY = 250,
	BYTES_FLAG_BUSY = 1, /* timer is running */
	BYTES_FLAG_MODIFIED = 2,

	ICON_TIMER_ID = 2,
	ICON_OFF_DELAY = 150,
	ICON_ON_DELAY = 350,
	ICON_FLAG_BUSY = 1,
	ICON_FLAG_MODIFIED = 2,
	ICON_FLAG_ON = 4,
};

struct PRNPROGRESSDLG
{
	struct DIALOG_DATA dialog_data;
	struct RESIZE_DIALOG resize;
	TCHAR num_prefix[RECV_TEXT_BUF_SIZE];
	TCHAR total_prefix[RECV_TEXT_BUF_SIZE];
	TCHAR page_prefix[RECV_TEXT_BUF_SIZE];
	size_t page_num;
	size_t printed_total;
	size_t printed_page;
	int bytes_printed_flags;
	int icon_flags;
	struct PRNPROGRESSDLG_CB *cb;
	void *param;
};

static struct RESIZE_DIALOG resize =
{
	RESIZE_LIMIT_MIN | RESIZE_NO_SIZEBOX,
	{ { 185, 99 }, { 0, 0 } },
	6,
	{
		{ IDC_SIZE1, { RESIZE_ALIGN_RIGHT, RESIZE_ALIGN_NONE } },
		{ IDC_PAGENUM, { RESIZE_ALIGN_RIGHT, RESIZE_ALIGN_NONE } },
		{ IDC_BYTESREC, { RESIZE_ALIGN_RIGHT, RESIZE_ALIGN_NONE } },
		{ IDC_BYTESPAGE, { RESIZE_ALIGN_RIGHT, RESIZE_ALIGN_NONE } },
		{ IDC_NEWPAGE, { RESIZE_ALIGN_RIGHT, RESIZE_ALIGN_TOP } },
		{ IDC_FINISH, { RESIZE_ALIGN_RIGHT, RESIZE_ALIGN_TOP } },
	}
};

static struct RESIZE_DIALOG resize_nopage =
{
	RESIZE_LIMIT_MIN | RESIZE_NO_SIZEBOX,
	{ { 185, 58 }, { 0, 0 } },
	3,
	{
		{ IDC_SIZE1, { RESIZE_ALIGN_RIGHT, RESIZE_ALIGN_NONE } },
		{ IDC_BYTESPAGE, { RESIZE_ALIGN_RIGHT, RESIZE_ALIGN_NONE } },
		{ IDC_FINISH, { RESIZE_ALIGN_RIGHT, RESIZE_ALIGN_TOP } },
	}
};

static void update_bytes_printed(HWND hwnd, struct PRNPROGRESSDLG *ppd)
{
	TCHAR text[RECV_TEXT_BUF_SIZE];
	StringCchPrintf(text, RECV_TEXT_BUF_SIZE, TEXT("%s %Iu"), ppd->num_prefix, ppd->page_num + 1);
	SetDlgItemText(hwnd, IDC_PAGENUM, text);
	StringCchPrintf(text, RECV_TEXT_BUF_SIZE, TEXT("%s %Iu"), ppd->total_prefix, ppd->printed_total);
	SetDlgItemText(hwnd, IDC_BYTESREC, text);
	StringCchPrintf(text, RECV_TEXT_BUF_SIZE, TEXT("%s %Iu"), ppd->page_prefix, ppd->printed_page);
	SetDlgItemText(hwnd, IDC_BYTESPAGE, text);
	ppd->bytes_printed_flags = BYTES_FLAG_BUSY; /* and not modified */
	SetTimer(hwnd, BYTES_TIMER_ID, BYTES_UPDATE_DELAY, NULL);
}

static void blink_icon(struct PRNPROGRESSDLG *ppd)
{
	HWND icon = GetDlgItem(ppd->dialog_data.modeless_hwnd, IDC_PRINT_ICON);
	ppd->icon_flags |= ICON_FLAG_BUSY;

	if (ppd->icon_flags & ICON_FLAG_ON) {
		ShowWindow(icon, SW_HIDE);
		ppd->icon_flags &= ~(ICON_FLAG_ON | ICON_FLAG_MODIFIED);
		SetTimer(ppd->dialog_data.modeless_hwnd, ICON_TIMER_ID, ICON_OFF_DELAY, NULL);
	} else {
		ShowWindow(icon, SW_SHOWNA);
		ppd->icon_flags |= ICON_FLAG_ON;
		SetTimer(ppd->dialog_data.modeless_hwnd, ICON_TIMER_ID, ICON_ON_DELAY, NULL);
	}
}

static int dialog_init(HWND hwnd, struct PRNPROGRESSDLG *ppd)
{
	GetDlgItemText(hwnd, IDC_PAGENUM, ppd->num_prefix, RECV_TEXT_BUF_SIZE);
	GetDlgItemText(hwnd, IDC_BYTESREC, ppd->total_prefix, RECV_TEXT_BUF_SIZE);
	GetDlgItemText(hwnd, IDC_BYTESPAGE, ppd->page_prefix, RECV_TEXT_BUF_SIZE);
	update_bytes_printed(hwnd, ppd);
	return 0;
}

static int dialog_message(HWND hwnd, struct PRNPROGRESSDLG *ppd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch(msg) {
	case WM_TIMER:
		switch(wp) {
		case BYTES_TIMER_ID:
			if (ppd->bytes_printed_flags & BYTES_FLAG_MODIFIED) update_bytes_printed(hwnd, ppd);
			else ppd->bytes_printed_flags &= ~BYTES_FLAG_BUSY;
			break;
		case ICON_TIMER_ID:
			if (ppd->icon_flags & ICON_FLAG_MODIFIED || !(ppd->icon_flags & ICON_FLAG_ON)) blink_icon(ppd);
			else ppd->icon_flags &= ~ICON_FLAG_BUSY;
			break;
		}
		return 0;
	}

	return -1;
}

static int dialog_command(HWND hwnd, struct PRNPROGRESSDLG *ppd, int notify, int id, HWND hctl)
{
	if (notify == BN_CLICKED) {
		switch (id) {
		case IDC_NEWPAGE:
			if (ppd->cb && ppd->cb->next) ppd->cb->next(ppd->param);
			return 0;
		case IDC_FINISH:
			if (ppd->cb && ppd->cb->finish) ppd->cb->finish(ppd->param);
			return 0;
		}
	}
	return -1;
}

struct PRNPROGRESSDLG* prnprogressdlg_create(HWND hpar, int enable_pages, struct PRNPROGRESSDLG_CB *cb, void *param)
{
	struct PRNPROGRESSDLG *ppd = calloc(1, sizeof(*ppd));
	if (!ppd) return NULL;
	dialog_init_modeless(&ppd->dialog_data);
	ppd->resize = enable_pages? resize : resize_nopage;
	ppd->dialog_data.resize = &ppd->resize;
	ppd->dialog_data.init = dialog_init;
	ppd->dialog_data.message = dialog_message;
	ppd->dialog_data.command = dialog_command;
	ppd->icon_flags = ICON_FLAG_ON;
	ppd->cb = cb;
	ppd->param = param;
	LPCTSTR resource = MAKEINTRESOURCE(enable_pages? IDD_PRNPROGRESS : IDD_PRNPROGRESS_NOPAGE);
	if (dialog_show_modeless(&ppd->dialog_data, resource, hpar, SW_SHOWNA, ppd) == 0) {
		return ppd;
	}
	prnprogressdlg_destroy(ppd);
	return NULL;
}

void prnprogressdlg_destroy(struct PRNPROGRESSDLG *ppd)
{
	if (ppd) {
		dialog_hide_modeless(&ppd->dialog_data);
		if (ppd->cb && ppd->cb->free) ppd->cb->free(ppd->param);
		free(ppd);
	}
}

int prnprogressdlg_set_bytes(struct PRNPROGRESSDLG *ppd, size_t pagenum, size_t total, size_t page)
{
	if (!ppd) return -1;
	ppd->page_num = pagenum;
	ppd->printed_total = total;
	ppd->printed_page = page;
	ppd->bytes_printed_flags |= BYTES_FLAG_MODIFIED;
	ppd->icon_flags |= ICON_FLAG_MODIFIED;
	if (!(ppd->bytes_printed_flags & BYTES_FLAG_BUSY)) {
		update_bytes_printed(ppd->dialog_data.modeless_hwnd, ppd);
	}
	if (!(ppd->icon_flags & ICON_FLAG_BUSY)) {
		assert(ppd->icon_flags & ICON_FLAG_ON);
		blink_icon(ppd);
	}
	return 0;
}
