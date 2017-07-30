/*
Agat Emulator
Copyright (c) NOP, nnop@newmail.ru
msgloop - main message loop
*/

#include "msgloop.h"

#include "dialog.h"
#include "list.h"
#include "winmsg.h"

#include <windows.h>

#include <assert.h>
#include <stdio.h>

static LIST_NODE_DEF(modeless_head);

/*
	32-bit integer access is atomic on all modern architectures. Also the compiler
	uses release and acquire semantics when writing and reading from volatile
	variables, respectively, starting Visual Studio 2005. Therefore
	loop_thread_id can be accessed without synchronization.
*/
static volatile DWORD loop_thread_id = 0;

/* Handle registered modeless dialog boxes. Return 1 if handled. */
static int modeless(MSG* msg)
{
	struct LIST_NODE *n;
	for (n = modeless_head.next; n != &modeless_head; n = n->next) {
		struct DIALOG_DATA *data;
		data = LIST_ENTRY(n, struct DIALOG_DATA, modeless_list);
		if (IsDialogMessage(data->modeless_hwnd, msg)) return 1;
	}
	return 0;
}

static int msgloop()
{
	for (;;) {
		MSG msg;
		BOOL res;

		res = GetMessage(&msg, NULL, 0, 0);
		if (res == 0) return 0; /* WM_QUIT */
		if (res < 0) return -1; /* error */

		/* execute callbacks on the message loop thread */
		if (msg.hwnd == NULL && msg.message == UM_EXECUTE) {
			MSGLOOP_RUNNABLE r = (MSGLOOP_RUNNABLE)msg.wParam;
			assert(r);
			r((void*)msg.lParam);
		} else if (!modeless(&msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

/* Run the main message loop. */
int msgloop_run()
{
       int result;
       loop_thread_id = GetCurrentThreadId();
       result = msgloop();
       loop_thread_id = 0;
       return result;
}

/* Register a modeless dialog for receiving keyboard events. */
void msgloop_register_modeless_dialog(struct DIALOG_DATA *dialog)
{
	list_insert(&dialog->modeless_list, &modeless_head);
}

/* Unregister a previously registered modeless dialog. */
void msgloop_unregister_modeless_dialog(struct DIALOG_DATA *dialog)
{
	list_remove(&dialog->modeless_list);
}

/* Execute a function on the message loop thread. */
void msgloop_execute(MSGLOOP_RUNNABLE r, void *p)
{
	if (!r) return;
	DWORD thread_id = loop_thread_id;
	if (thread_id != 0) PostThreadMessage(thread_id, UM_EXECUTE, (WPARAM)r, (LPARAM)p);
	else puts("Unable to execute on message loop thread: message loop not running");
}
