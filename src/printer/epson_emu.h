/*
	Epson printer command emulation for Agat emulator
	Copyright (c) NOP, nnop@newmail.ru
*/

#include <windows.h>

typedef struct EPSON_EMU *PEPSON_EMU;

PEPSON_EMU epson_create(unsigned flags, HWND wnd);
void epson_free(PEPSON_EMU emu);

int epson_open(PEPSON_EMU emu, unsigned flags);
int epson_close(PEPSON_EMU emu);


int epson_write(PEPSON_EMU emu, unsigned char data);

