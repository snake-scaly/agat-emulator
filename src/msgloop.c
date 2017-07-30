/*
Agat Emulator
Copyright (c) NOP, nnop@newmail.ru
msgloop - main message loop
*/

#include "msgloop.h"
#include <assert.h>
#include <windows.h>
#include "dialog.h"
#include "list.h"

static LIST_NODE_DEF(modeless_head);

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

/* Run the main message loop. */
int msgloop_run()
{
	for (;;) {
		MSG msg;
		BOOL res = GetMessage(&msg, NULL, 0, 0);
		if (res == 0) return 0; /* WM_QUIT */
		if (res < 0) return -1; /* error */
		if (!modeless(&msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
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
