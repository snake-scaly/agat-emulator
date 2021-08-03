/*
        Agat Emulator
        Copyright (c) NOP, nnop@newmail.ru
        NewGfx by Sergey 'SnakE' Gromov <snake.scaly@gmail.com>
*/

#include "ng_logging.h"

#include "logging/logging.h"

#include <windows.h>

void log_win_error(const char* file, int line, const char* msg)
{
	DWORD err = GetLastError();
	LPSTR err_msg;

	DWORD err_len = FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, (LPSTR)&err_msg, 0, NULL);

	if (err_len) {
		log_message(LOG_LEVEL_ERROR, file, line, "%s: %s", msg, err_msg);
	} else {
		DWORD fmt_err = GetLastError();
		log_message(LOG_LEVEL_ERROR, file, line, "%s: %lu", msg, err);
		log_message(LOG_LEVEL_ERROR, file, line, "Failed to decode message %lu: error code %lu", err, fmt_err);
	}
}
