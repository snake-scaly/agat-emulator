/*
	Agat Emulator version 1.14
	Copyright (c) NOP, nnop@newmail.ru
	6821 module
*/
#include "types.h"

struct MC6821_REGS
{
	byte data_pc, data_dev, dir, control;
};

struct MC6821_STATE
{
	struct MC6821_REGS regs[2];
};


byte mc6821_get_data(struct MC6821_STATE*st, int line);
void mc6821_set_data(struct MC6821_STATE*st, int line, byte data);
void mc6821_reset(struct MC6821_STATE*st);

byte mc6821_read(struct MC6821_STATE*st, int adr);
void mc6821_write(struct MC6821_STATE*st, int adr, byte data);
