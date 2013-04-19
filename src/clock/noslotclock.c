/*
	Agat Emulator version 1.6
	Copyright (c) NOP, nnop@newmail.ru
	noslotclock - emulation of Dallas Clock chip
*/

#include "types.h"
#include "memory.h"
#include "streams.h"
#include "debug.h"
#include "sysconf.h"
#include "runmgr.h"
#include "runmgrint.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "syslib.h"
#include "debug.h"

#include "localize.h"

#define NREGS 8


static const byte match[8] = {
	0xC5, 0x3A, 0xA3, 0x5C, 0xC5, 0x3A, 0xA3, 0x5C
};

struct NOSLOTCLOCK_STATE
{
	struct SLOT_RUN_STATE*st;
	struct MEM_PROC sys_rom;

	int	adjust;
	byte	regs[NREGS];
	byte	val;
	int	b_ind;
	int	r_ind;
	int	active;
};

static int noslotclock_term(struct SLOT_RUN_STATE*st)
{
	struct NIPPELCLOCK_STATE*ncs = st->data;
	free(st->data);
	return 0;
}

static int noslotclock_save(struct SLOT_RUN_STATE*st, OSTREAM*out)
{
	struct NOSLOTCLOCK_STATE*ncs = st->data;
	WRITE_FIELD(out, ncs->adjust);
	WRITE_ARRAY(out, ncs->regs);

	return 0;
}

static int noslotclock_load(struct SLOT_RUN_STATE*st, ISTREAM*in)
{
	struct NOSLOTCLOCK_STATE*ncs = st->data;
	READ_FIELD(in, ncs->adjust);
	READ_ARRAY(in, ncs->regs);

	return 0;
}

static void update_clock(struct NOSLOTCLOCK_STATE*ncs);
static void write_clock(struct NOSLOTCLOCK_STATE*ncs);

static byte clock_bit_read(struct NOSLOTCLOCK_STATE*ncs)
{
	int v;
	v = (ncs->regs[ncs->r_ind] >> ncs->b_ind) & 1;
	++ ncs->b_ind;
	if (ncs->b_ind == 8) {
		++ ncs->r_ind;
		ncs->b_ind = 0;
		if (ncs->r_ind == 8) {
			ncs->r_ind = 0;
			ncs->active = 0;
		}
	}
//	printf("clock bit read: r_ind = %i, b_ind = %i, active = %i, v = %i\n", ncs->r_ind, ncs->b_ind, ncs->active, v);
	return v;
}

static void clock_bit_write(struct NOSLOTCLOCK_STATE*ncs, int v)
{
//	printf("clock bit write: r_ind = %i, b_ind = %i, active = %i, v = %i\n", ncs->r_ind, ncs->b_ind, ncs->active, v);
	if (!ncs->active) {
		if (((match[ncs->r_ind] >> ncs->b_ind) & 1) != v) {
			ncs->b_ind = ncs->r_ind = 0;
		} else {
			++ ncs->b_ind;
			if (ncs->b_ind == 8) {
				++ ncs->r_ind;
				ncs->b_ind = 0;
				if (ncs->r_ind == 8) {
					ncs->active = 1;
					ncs->r_ind = 0;
					update_clock(ncs);
				}
			}
		}
	} else {
		if (v) ncs->regs[ncs->r_ind] |= (1<<ncs->b_ind);
		else ncs->regs[ncs->r_ind] &= ~(1<<ncs->b_ind);
		++ ncs->b_ind;
		if (ncs->b_ind == 8) {
			++ ncs->r_ind;
			ncs->b_ind = 0;
			if (ncs->r_ind == 8) {
				ncs->active = 0;
				ncs->r_ind = 0;
				write_clock(ncs);
			}
		}
	}
}


static byte clock_rom_read(word adr, struct NOSLOTCLOCK_STATE*ncs)
{
	switch (adr) {
	case 0xC800: // write 0
	case 0xC801: // write 1
		clock_bit_write(ncs, adr & 1);
		break;
	case 0xC804: // read
		if (ncs->active) return clock_bit_read(ncs);
		else { ncs->b_ind = ncs->r_ind = 0; }
		break;
	}
	return ncs->sys_rom.read(adr, ncs->sys_rom.pr);
}

static int noslotclock_command(struct SLOT_RUN_STATE*st, int cmd, int data, long param)
{
	struct NOSLOTCLOCK_STATE*ncs = st->data;
	switch (cmd) {
	case SYS_COMMAND_RESET:
		return 0;
	case SYS_COMMAND_HRESET:
		return 0;
	case SYS_COMMAND_INIT_DONE:
		ncs->sys_rom = st->sr->rom_c800;
		st->sr->rom_c800.read = clock_rom_read;
		st->sr->rom_c800.pr = ncs;
		return 0;
	}
	return 0;
}

static byte cvtnum(byte num, struct NOSLOTCLOCK_STATE*ncs)
{
	return ((num/10)<<4)+(num%10);
}

static byte cvtres(byte num, struct NOSLOTCLOCK_STATE*ncs)
{
	return (num>>4)*10 + (num & 15);
}

static void update_clock(struct NOSLOTCLOCK_STATE*ncs)
{
	time_t timer;
	struct tm tm;
	time(&timer);
	timer -= ncs->adjust;
	tm = *localtime(&timer);
	ncs->regs[7] = cvtnum(tm.tm_year % 100, ncs);
	ncs->regs[6] = cvtnum(tm.tm_mon + 1, ncs);
	ncs->regs[5] = cvtnum(tm.tm_mday, ncs);
	ncs->regs[4] = cvtnum(((tm.tm_wday + 6) % 7) + 1, ncs);
	ncs->regs[3] = cvtnum(tm.tm_hour, ncs);
	ncs->regs[2] = cvtnum(tm.tm_min, ncs);
	ncs->regs[1] = cvtnum(tm.tm_sec, ncs);
	ncs->regs[0] = cvtnum(GetTickCount()%100, ncs);
}

static void write_clock(struct NOSLOTCLOCK_STATE*ncs)
{
	time_t timer, t1;
	struct tm tm;
	time(&timer);
	tm = *localtime(&timer);
	tm.tm_year = cvtres(ncs->regs[7], ncs);
	if (tm.tm_year < 40) tm.tm_year += 100;
	tm.tm_mon = cvtres(ncs->regs[6], ncs) - 1;
	tm.tm_mday = cvtres(ncs->regs[5], ncs);
	tm.tm_wday = (cvtres(ncs->regs[4]&7, ncs) + 7) % 7;
	tm.tm_hour = cvtres(ncs->regs[3]&0x3F, ncs);
	if (ncs->regs[3]&0x80) {
		if (ncs->regs[3]&0x20) {
			tm.tm_hour += 11;
		} else {
			tm.tm_hour -= 1;
		}
	}
	tm.tm_min = cvtres(ncs->regs[2], ncs);
	tm.tm_sec = cvtres(ncs->regs[1], ncs);
	t1 = mktime(&tm);
	ncs->adjust = timer - t1;
	printf("time adjust = %i\n", ncs->adjust);
}



int  noslotclock_init(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*st, struct SLOTCONFIG*cf)
{
	int i;
	struct NOSLOTCLOCK_STATE*ncs;

	puts("in noslotclock_init");

	ncs = calloc(1, sizeof(*ncs));
	if (!ncs) return -1;

	ncs->st = st;
	st->data = ncs;
	st->free = noslotclock_term;
	st->command = noslotclock_command;
	st->load = noslotclock_load;
	st->save = noslotclock_save;

	return 0;
}
