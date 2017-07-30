/*
	Agat Emulator
	Copyright (c) NOP, nnop@newmail.ru
*/

#ifndef PRNPROGRESSDLG_H
#define PRNPROGRESSDLG_H

#include <windows.h>

/* Progress dialog callbacks. */
struct PRNPROGRESSDLG_CB
{
	void (*next)(void *param);    /* insert next page */
	void (*finish)(void *param);  /* finish printing */
	void (*free)(void *param);    /* free param */
};

struct PRNPROGRESSDLG* prnprogressdlg_create(HWND hpar, int enable_pages, struct PRNPROGRESSDLG_CB *cb, void *param);
void prnprogressdlg_destroy(struct PRNPROGRESSDLG *ppd);
int prnprogressdlg_set_bytes(struct PRNPROGRESSDLG *ppd, size_t pagenum, size_t total, size_t page);

#endif // PRNPROGRESSDLG_H
