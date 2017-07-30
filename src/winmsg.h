/*
Agat Emulator version 1.0
Copyright (c) NOP, nnop@newmail.ru
winmsg - custom message IDs
*/

#ifndef WINMSG_H
#define WINMSG_H

#include <windows.h>

enum USER_MESSAGES
{
	CWM_CLEAR = WM_USER,
	CWM_TERM,
#ifdef UNDER_CE
	WM_INPUTLANGCHANGE,
#endif

	UM_EXECUTE, // Execute a function on the message loop thread.
};

#endif // WINMSG_H