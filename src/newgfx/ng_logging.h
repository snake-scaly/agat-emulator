/*
        Agat Emulator
        Copyright (c) NOP, nnop@newmail.ru
        NewGfx by Sergey 'SnakE' Gromov <snake.scaly@gmail.com>
*/

#ifndef NG_LOGGING_H
#define NG_LOGGING_H

#define LOG_WIN_ERROR(msg) log_win_error(__FILE__, __LINE__, msg)

void log_win_error(const char* file, int line, const char* msg);

#endif // NG_LOGGING_H
