/*
Agat Emulator
Copyright (c) NOP, nnop@newmail.ru
msgloop - main message loop
*/

#ifndef MSGLOOP_H
#define MSGLOOP_H

typedef void (*MSGLOOP_RUNNABLE)(void *p);

int msgloop_run();
void msgloop_register_modeless_dialog(struct DIALOG_DATA *dialog);
void msgloop_unregister_modeless_dialog(struct DIALOG_DATA *dialog);
void msgloop_execute(MSGLOOP_RUNNABLE r, void *p);

#endif // MSGLOOP_H
