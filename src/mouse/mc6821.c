/*
	Agat Emulator
	Copyright (c) NOP, nnop@newmail.ru
	6821 module
*/

#include <string.h>
#include "mc6821.h"

byte mc6821_get_data(struct MC6821_STATE*st, int line)
{
	return (st->regs[line].data_pc & st->regs[line].dir) | 
		(st->regs[line].data_dev & ~st->regs[line].dir);
}

void mc6821_set_data(struct MC6821_STATE*st, int line, byte data)
{
	st->regs[line].data_dev = data;
}

void mc6821_reset(struct MC6821_STATE*st)
{
	memset(st, 0, sizeof(*st));
}


byte mc6821_read(struct MC6821_STATE*st, int adr)
{
	struct MC6821_REGS*regs = st->regs + ((adr&2)?1:0);
	if (adr & 1) return regs->control;
	if (regs->control & 4) {
		regs->control &= 0x3F; // clear ints
		return (regs->data_dev & ~regs->dir) | (regs->data_pc & regs->dir);
	} else return regs->dir;
}

void mc6821_write(struct MC6821_STATE*st, int adr, byte data)
{
	struct MC6821_REGS*regs = st->regs + ((adr&2)?1:0);
	switch (adr & 1) {
	case 0: if (regs->control & 4) regs->data_pc = data;
		else regs->dir = data;
		break;
	case 1:	regs->control = data;
		break;
	}
}
