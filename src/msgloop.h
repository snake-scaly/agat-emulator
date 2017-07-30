/*
Agat Emulator
Copyright (c) NOP, nnop@newmail.ru
msgloop - main message loop
*/

#ifndef MSGLOOP_H
#define MSGLOOP_H

int msgloop_run();
void msgloop_register_modeless_dialog(struct DIALOG_DATA *dialog);
void msgloop_unregister_modeless_dialog(struct DIALOG_DATA *dialog);

#endif // MSGLOOP_H
