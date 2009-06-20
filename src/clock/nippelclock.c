/*
	Agat Emulator version 1.6
	Copyright (c) NOP, nnop@newmail.ru
	nippelclock - emulation of Agat-9 Nippel Clock card
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

#define NREGS 64

#define REG_A 10
#define REG_B 11
#define REG_C 12
#define REG_D 13

struct NIPPELCLOCK_STATE
{
	struct SLOT_RUN_STATE*st;

	int	adjust;
	int	regno;
	byte	regs[NREGS];
	int	nints;
};

static int nippelclock_term(struct SLOT_RUN_STATE*st)
{
	struct NIPPELCLOCK_STATE*ncs = st->data;
	free(st->data);
	return 0;
}

static int nippelclock_save(struct SLOT_RUN_STATE*st, OSTREAM*out)
{
	struct NIPPELCLOCK_STATE*ncs = st->data;
	WRITE_FIELD(out, ncs->adjust);
	WRITE_FIELD(out, ncs->regno);
	WRITE_ARRAY(out, ncs->regs);
	WRITE_FIELD(out, ncs->nints);

	return 0;
}

static int nippelclock_load(struct SLOT_RUN_STATE*st, ISTREAM*in)
{
	struct NIPPELCLOCK_STATE*ncs = st->data;
	READ_FIELD(in, ncs->adjust);
	READ_FIELD(in, ncs->regno);
	READ_ARRAY(in, ncs->regs);
	READ_FIELD(in, ncs->nints);

	return 0;
}

static VOID CALLBACK ncs_callback(HWND wnd, UINT msg, struct NIPPELCLOCK_STATE*ncs, DWORD time);


static int nippelclock_command(struct SLOT_RUN_STATE*st, int cmd, int data, long param)
{
	struct NIPPELCLOCK_STATE*ncs = st->data;
	switch (cmd) {
	case SYS_COMMAND_RESET:
		ncs->regs[REG_B] &= ~(0x40|0x20|0x10);
		ncs->regs[REG_C] = 0;
		return 0;
	case SYS_COMMAND_HRESET:
		ncs->regno = 0;
		ncs->regs[REG_B] &= ~(0x40|0x20|0x10);
		ncs->regs[REG_C] = 0;
		return 0;
	case SYS_COMMAND_INIT_DONE:
		system_command(st->sr, SYS_COMMAND_SET_CPUTIMER, 1000000/8192, DEF_CPU_TIMER_ID(ncs->st));
		return 0;
	case SYS_COMMAND_CPUTIMER:
		if (param == DEF_CPU_TIMER_ID(ncs->st)) {
			ncs_callback(NULL, 0, ncs, 0);
			return 1;
		}
		return 0;
	}
	return 0;
}

static byte cvtnum(byte num, struct NIPPELCLOCK_STATE*ncs)
{
	if (ncs->regs[REG_B]&4) return num;
	else return ((num/10)<<4)+(num%10);
}

static byte cvtres(byte num, struct NIPPELCLOCK_STATE*ncs)
{
	if (ncs->regs[REG_B]&4) return num;
	else return (num>>4)*10 + (num & 15);
}

static void update_clock(struct NIPPELCLOCK_STATE*ncs)
{
	time_t timer;
	struct tm tm;
	time(&timer);
	timer -= ncs->adjust;
	tm = *localtime(&timer);
	ncs->regs[9] = cvtnum(tm.tm_year % 100, ncs);
	ncs->regs[8] = cvtnum(tm.tm_mon + 1, ncs);
	ncs->regs[7] = cvtnum(tm.tm_mday, ncs);
	ncs->regs[6] = cvtnum(((tm.tm_wday + 6) % 7) + 1, ncs);
	ncs->regs[4] = cvtnum(tm.tm_hour, ncs);
	ncs->regs[2] = cvtnum(tm.tm_min, ncs);
	ncs->regs[0] = cvtnum(tm.tm_sec, ncs);
	if (!(ncs->regs[REG_B]&2)) {
		if (ncs->regs[4] >= 12) {
			ncs->regs[4] -= 12;
			ncs->regs[4] |= 0x80;
		}
	}
}

static void write_clock(struct NIPPELCLOCK_STATE*ncs)
{
	time_t timer, t1;
	struct tm tm;
	time(&timer);
	tm = *localtime(&timer);
	tm.tm_year = cvtres(ncs->regs[9], ncs);
	if (tm.tm_year < 40) tm.tm_year += 100;
	tm.tm_mon = cvtres(ncs->regs[8], ncs) - 1;
	tm.tm_mday = cvtres(ncs->regs[7], ncs);
	tm.tm_wday = (cvtres(ncs->regs[6], ncs) + 7) % 7;
	if ((ncs->regs[REG_B]&2)) {
		tm.tm_hour = cvtres(ncs->regs[4], ncs);
	} else {
		if (ncs->regs[4]&0x80) {
			tm.tm_hour = cvtres(ncs->regs[4]&0x7F, ncs) + 12;
		} else {
			tm.tm_hour = cvtres(ncs->regs[4], ncs);
		}
	}
	tm.tm_hour = cvtres(ncs->regs[4], ncs);
	tm.tm_min = cvtres(ncs->regs[2], ncs);
	tm.tm_sec = cvtres(ncs->regs[0], ncs);
	t1 = mktime(&tm);
	ncs->adjust = timer - t1;
	printf("time adjust = %i\n", ncs->adjust);
}

static int compare_timer(struct NIPPELCLOCK_STATE*ncs)
{
	int i, j;
	for (i = 0, j = 0; i < 3; ++i, j += 2) {
		if ((ncs->regs[j + 1] & 0xC0) == 0xC0) continue;
		if (ncs->regs[j] != ncs->regs[j + 1]) return 0;
	}
	return 1;
}

static VOID CALLBACK ncs_callback(HWND wnd, UINT msg, struct NIPPELCLOCK_STATE*ncs, DWORD time)
{
	static int divs[16] = {0, 32, 64, 1,   2,   4,   8,    16,   32,  64, 128, 256, 512, 1024, 2048, 4096};
	/*		       0 256 128 8192 4096 2048  1024  512  256  128  64   32   16   8      4      2 */
	int div;
	div = divs[ncs->regs[REG_A]&0x0F];
	++ ncs->nints;

//	puts("tick");

	if (!(ncs->nints % 8192)) { // second update interval
		if (!(ncs->regs[REG_B]&0x80)) update_clock(ncs);
		ncs->regs[REG_C] |= 0x80;
		ncs->regs[REG_C] |= 0x10;
		if (ncs->regs[REG_B] & 0x10) {
			system_command(ncs->st->sr, SYS_COMMAND_IRQ, 0, 0);
		}
		if (compare_timer(ncs)) {
			ncs->regs[REG_C] |= 0x80;
			ncs->regs[REG_C] |= 0x20;
			if (ncs->regs[REG_B] & 0x20) {
				system_command(ncs->st->sr, SYS_COMMAND_IRQ, 0, 0);
			}
		}
	}

	if (div && !(ncs->nints % div)) { // interrupt
//		printf("div = %i\n", div);
		ncs->regs[REG_C] |= 0x80;
		ncs->regs[REG_C] |= 0x40;
		if (ncs->regs[REG_B] & 0x40) {
			system_command(ncs->st->sr, SYS_COMMAND_IRQ, 0, 0);
		}
	}
}

static void nc_write_addr(byte regno, struct NIPPELCLOCK_STATE*ncs)
{
	ncs->regno = regno % sizeof(ncs->regs);
}

static void nc_write_data(int regno, byte data, struct NIPPELCLOCK_STATE*ncs)
{
	ncs->regs[regno] = data;
	switch (regno) {
	case 0: case 2: case 4: case 6: case 7: case 8: case 9:
		write_clock(ncs);
		break;
	}
}


static byte nc_read_data(int regno, byte data, struct NIPPELCLOCK_STATE*ncs)
{
	byte r;
	switch (regno) {
	case REG_C:
		r = ncs->regs[REG_C];
		ncs->regs[REG_C] = 0;
		system_command(ncs->st->sr, SYS_COMMAND_NOIRQ, 0, 0);
		return r;
	}
	return ncs->regs[regno];
}


static void nippelclock_io_w(word adr, byte data, struct NIPPELCLOCK_STATE*ncs) // C0X0-C0XF
{
//	logprint(0, TEXT("nippelclock i/o write: %04X, %02X"), adr, data);
	adr &= 0x0F;
	switch (adr) {
	case 6:
		nc_write_addr(data, ncs);
		break;
	case 7:
		nc_write_data(ncs->regno, data, ncs);
		break;
	}
}

static byte nippelclock_io_r(word adr, struct NIPPELCLOCK_STATE*ncs) // C0X0-C0XF
{
//	logprint(0, TEXT("nippelclock i/o read: %04X"), adr);
	adr &= 0x0F;
	switch (adr) {
	case 6:
		return ncs->regno;
	case 7:
		return nc_read_data(ncs->regno, adr, ncs);
	}
	return empty_read(adr, ncs);
}


int  nippelclock_init(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*st, struct SLOTCONFIG*cf)
{
	int i;
	struct NIPPELCLOCK_STATE*ncs;

	puts("in nippelclock_init");

	ncs = calloc(1, sizeof(*ncs));
	if (!ncs) return -1;

	ncs->st = st;
	st->data = ncs;
	st->free = nippelclock_term;
	st->command = nippelclock_command;
	st->load = nippelclock_load;
	st->save = nippelclock_save;

	ncs->regs[REG_B] = 2 | 4;

	fill_rw_proc(st->baseio_sel, 1, nippelclock_io_r, nippelclock_io_w, ncs);

	return 0;
}
