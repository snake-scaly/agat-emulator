/*
	Agat Emulator
	Copyright (c) NOP, nnop@newmail.ru
	prnprogressdlg_interop - show a print progress dialog from a CPU thread
*/

#ifndef PRNPROGRESSDLG_INTEROP_H
#define PRNPROGRESSDLG_INTEROP_H

#include <windows.h>

struct PRNPROGRESSDLG_INTEROP
{
	struct PRNPROGRESSDLG *progressdlg;
};

struct PRNPROGRESSDLG_INTEROP_CB
{
	void(*next)(void *param);    /* insert next page */
	void(*finish)(void *param);  /* finish printing */
	void(*free)(void *param);    /* free param */
};

struct PRNPROGRESSDLG_INFO
{
	size_t page_num;
	size_t printed_total;
	size_t printed_page;
};

void prnprogressdlg_interop_init(struct PRNPROGRESSDLG_INTEROP *interop);
void prnprogressdlg_interop_uninit(struct PRNPROGRESSDLG_INTEROP *interop);
int prnprogressdlg_create_sync(struct PRNPROGRESSDLG_INTEROP *interop, HWND parent,
	int enable_pages, struct PRNPROGRESSDLG_INTEROP_CB *cb, void *param);
int prnprogressdlg_update_async(struct PRNPROGRESSDLG_INTEROP *interop, struct PRNPROGRESSDLG_INFO *info);
void prnprogressdlg_destroy_async(struct PRNPROGRESSDLG_INTEROP *interop);

#endif // PRNPROGRESSDLG_INTEROP_H
