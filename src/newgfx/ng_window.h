/*
	Agat Emulator
	Copyright (c) NOP, nnop@newmail.ru
	NewGfx by Sergey 'SnakE' Gromov <snake.scaly@gmail.com>
*/

#ifndef NG_WINDOW_H
#define NG_WINDOW_H

typedef struct NG_WINDOW NG_WINDOW;

NG_WINDOW* ng_window_create();
void ng_window_free(NG_WINDOW* ngw);

#endif // NG_WINDOW_H
