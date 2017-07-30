/*
	Agat Emulator
	Copyright (c) NOP, nnop@newmail.ru
	prnprogressdlg_interop - show a print progress dialog from a CPU thread
*/

#include "prnprogressdlg_interop.h"

#include <prnprogressdlg.h>
#include <cpu/cpuint.h>
#include <msgloop.h>

#include <windows.h>

/*
	Initialize an interop structure.
*/
void prnprogressdlg_interop_init(struct PRNPROGRESSDLG_INTEROP *interop)
{
	memset(interop, 0, sizeof(*interop));
}

/*
	Destroy a progress dialog owned by this interop asynchronously.
*/
void prnprogressdlg_destroy_async(struct PRNPROGRESSDLG_INTEROP *interop)
{
	if (interop->progressdlg) {
		msgloop_execute(prnprogressdlg_destroy, interop->progressdlg);
		interop->progressdlg = NULL;
	}
}

/*
	Uninitialize this interop structure. If a progress dialog is currently
	owned by this interop, it will be destroyed asynchronously.
*/
void prnprogressdlg_interop_uninit(struct PRNPROGRESSDLG_INTEROP *interop)
{
	prnprogressdlg_destroy_async(interop);
}

struct PRNPROGRESSDLG_CALLBACK_PARAM
{
	struct CPU_STATE *cpu;
	struct PRNPROGRESSDLG_INTEROP_CB *cb;
	void *param;
};

static void prnprogressdlg_next_cb(struct PRNPROGRESSDLG_CALLBACK_PARAM *param)
{
	if (param->cb && param->cb->next) cpu_exec_async(param->cpu, param->cb->next, param->param);
}

static void prnprogressdlg_finish_cb(struct PRNPROGRESSDLG_CALLBACK_PARAM *param)
{
	if (param->cb && param->cb->finish) cpu_exec_async(param->cpu, param->cb->finish, param->param);
}

static void prnprogressdlg_free_cb(struct PRNPROGRESSDLG_CALLBACK_PARAM *param)
{
	if (param->cb && param->cb->free) cpu_exec_async(param->cpu, param->cb->free, param->param);
	free(param);
}

static struct PRNPROGRESSDLG_CB prnprogressdlg_interop_cb =
{
	.next = prnprogressdlg_next_cb,
	.finish = prnprogressdlg_finish_cb,
	.free = prnprogressdlg_free_cb,
};

struct PRNPROGRESSDLG_CREATE_PARAM
{
	HWND parent;
	int enable_pages;
	struct PRNPROGRESSDLG_CALLBACK_PARAM cb_param;
	HANDLE completion_event;
	struct PRNPROGRESSDLG *progressdlg;
};

static void prnprogressdlg_create_cb(struct PRNPROGRESSDLG_CREATE_PARAM *param)
{
	struct PRNPROGRESSDLG_CALLBACK_PARAM *cb_param = calloc(1, sizeof(*cb_param));
	if (cb_param) {
		*cb_param = param->cb_param;
		param->progressdlg = prnprogressdlg_create(param->parent,
			param->enable_pages, &prnprogressdlg_interop_cb, cb_param);
		// progress dialog owns cb_param after successful creation
		if (!param->progressdlg) free(cb_param);
	}
	SetEvent(param->completion_event);
}

/*
	Create a print progress dialog synchronously.
	This function schedules the dialog creation on the GUI thread
	and waits until it's done.

	cb must point to a static list of callbacks. Moreover these callbacks
	must be prepared to receive calls even after the dialog has been destroyed,
	because destruction happens asynchronously. This is especially true for
	the cb->free callback: it will *always* be called after the
	prnprogressdlg_destroy_async() or prnprogressdlg_interop_uninit() will
	have returned.

	interop - an initialized interop structure.
	parent - HWND of the parent window. Can be NULL.
	cb - progress dialog callbacks. Will be called on the GUI thread.
	     Can be NULL. Individual callbacks can be NULL.
	param - will be passed to callbacks. Can be NULL.

	Returns zero on success, non-zero on failure.
*/
int prnprogressdlg_create_sync(
	struct PRNPROGRESSDLG_INTEROP *interop,
	HWND parent,
	int enable_pages,
	struct PRNPROGRESSDLG_INTEROP_CB *cb,
	void *param)
{
	struct PRNPROGRESSDLG_CREATE_PARAM create_param = {0};
	create_param.parent = parent;
	create_param.enable_pages = enable_pages;
	create_param.cb_param.cpu = get_cpu();
	create_param.cb_param.cb = cb;
	create_param.cb_param.param = param;

	create_param.completion_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (!create_param.completion_event) return 1;

	msgloop_execute(prnprogressdlg_create_cb, &create_param);

	WaitForSingleObject(create_param.completion_event, INFINITE);
	CloseHandle(create_param.completion_event);

	interop->progressdlg = create_param.progressdlg;
	return !interop->progressdlg;
}

struct PRNPROGRESSDLG_UPDATE_PARAM
{
	struct PRNPROGRESSDLG *ppd;
	struct PRNPROGRESSDLG_INFO info;
};

static void prnprogressdlg_update_cb(struct PRNPROGRESSDLG_UPDATE_PARAM *param)
{
	prnprogressdlg_set_bytes(
		param->ppd,
		param->info.page_num,
		param->info.printed_total,
		param->info.printed_page);
	free(param);
}

/*
	Request a print progress dialog to update statistics.
	This function is asynchronous. It schedules the update on the GUI
	thread and returns immediately.

	interop - an interop structure that owns the dialog.
	info - data to display. Contents are copied and are not accessed after this function returns.
*/
int prnprogressdlg_update_async(struct PRNPROGRESSDLG_INTEROP *interop, struct PRNPROGRESSDLG_INFO *info)
{
	struct PRNPROGRESSDLG_UPDATE_PARAM *param = calloc(1, sizeof(*param));
	if (!param) return 1;
	param->ppd = interop->progressdlg;
	param->info = *info;
	msgloop_execute(prnprogressdlg_update_cb, param);
	return 0;
}
