/*
	Agat Emulator version 1.3
	Copyright (c) NOP, nnop@newmail.ru
	thunderclock - emulation of Apple ][ Thunderclock module
*/

#include "types.h"
#include "memory.h"
#include "streams.h"
#include "debug.h"
#include "sysconf.h"
#include "runmgr.h"
#include "runmgrint.h"
#include "common.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "syslib.h"
#include "debug.h"

#include "localize.h"

#define THUNDERCLOCK_ROM_SIZE 0x800
#define THUNDERCLOCK_ROM_OFFSET 0

struct THUNDERCLOCK_STATE
{
	struct SLOT_RUN_STATE*st;

	byte creg, cmode;
	int  ind;
	byte data[10], wdata[10];
	int  adjust;
	int  ints;
	int  was_int;
	int  nints;
	int  rate;

	int  rd, wr;


	byte rom[THUNDERCLOCK_ROM_SIZE];
	int  romsel;
};

static VOID CALLBACK tcs_callback(HWND wnd, UINT msg, struct THUNDERCLOCK_STATE*tcs, DWORD time);

static void thunderclock_rom_unselect(struct THUNDERCLOCK_STATE*tcs);
static void thunderclock_rom_select(struct THUNDERCLOCK_STATE*tcs);

static int thunderclock_term(struct SLOT_RUN_STATE*st)
{
	struct THUNDERCLOCK_STATE*tcs = st->data;
//	KillTimer(tcs->st->sr->video_w, (UINT)tcs);
	free(st->data);
	return 0;
}

static int thunderclock_save(struct SLOT_RUN_STATE*st, OSTREAM*out)
{
	struct THUNDERCLOCK_STATE*tcs = st->data;
	WRITE_FIELD(out, tcs->adjust);
	WRITE_FIELD(out, tcs->ints);
	WRITE_FIELD(out, tcs->romsel);

	return 0;
}

static int thunderclock_load(struct SLOT_RUN_STATE*st, ISTREAM*in)
{
	struct THUNDERCLOCK_STATE*tcs = st->data;
	READ_FIELD(in, tcs->adjust);
	READ_FIELD(in, tcs->ints);
	READ_FIELD(in, tcs->romsel);
	if (tcs->romsel) thunderclock_rom_select(tcs);

	return 0;
}

static int thunderclock_command(struct SLOT_RUN_STATE*st, int cmd, int data, long param)
{
	struct THUNDERCLOCK_STATE*tcs = st->data;
	switch (cmd) {
	case SYS_COMMAND_RESET:
		tcs->ints = 0;
		return 0;
	case SYS_COMMAND_HRESET:
		tcs->ints = 0;
		return 0;
	case SYS_COMMAND_FLASH:
		return 0;
	case SYS_COMMAND_REPAINT:
		return 0;
	case SYS_COMMAND_TOGGLE_MONO:
		return 0;
	case SYS_COMMAND_INIT_DONE:
		system_command(st->sr, SYS_COMMAND_SET_CPUTIMER, 1000000/64, DEF_CPU_TIMER_ID(tcs->st));
		return 0;
	case SYS_COMMAND_CPUTIMER:
		if (param == DEF_CPU_TIMER_ID(tcs->st)) {
//			puts("timer!");
			tcs_callback(NULL, 0, tcs, 0);
			return 1;
		}
		return 0;
	}
	return 0;
}

static byte thunderclock_xrom_r(word adr, struct THUNDERCLOCK_STATE*tcs); // C800-CFFF

static void thunderclock_rom_select(struct THUNDERCLOCK_STATE*tcs)
{
//	puts("thunderclock rom select");
	enable_slot_xio(tcs->st, 1);
}

static void thunderclock_rom_unselect(struct THUNDERCLOCK_STATE*tcs)
{
//	puts("thunderclock rom unselect");
	enable_slot_xio(tcs->st, 0);
}

static byte thunderclock_xrom_r(word adr, struct THUNDERCLOCK_STATE*tcs) // C800-CFFF
{
	if (adr == 0xCFFF) 
		thunderclock_rom_unselect(tcs);
	return tcs->rom[adr & (THUNDERCLOCK_ROM_SIZE - 1)];
}

static void thunderclock_rom_w(word adr, byte data, struct THUNDERCLOCK_STATE*tcs) // CX00-CXFF
{
	thunderclock_rom_select(tcs);
}

static byte thunderclock_rom_r(word adr, struct THUNDERCLOCK_STATE*tcs) // CX00-CXFF
{
	thunderclock_rom_select(tcs);
	return tcs->rom[(adr & 0xFF) + THUNDERCLOCK_ROM_OFFSET];
}

static void read_clock(struct THUNDERCLOCK_STATE*tcs)
{
	time_t timer;
	struct tm tm;
	time(&timer);
	timer -= tcs->adjust;
	tm = *localtime(&timer);
	tcs->data[9] = tm.tm_mon + 1;
	tcs->data[8] = tm.tm_wday;
	tcs->data[7] = tm.tm_mday / 10;
	tcs->data[6] = tm.tm_mday % 10;
	tcs->data[5] = tm.tm_hour / 10;
	tcs->data[4] = tm.tm_hour % 10;
	tcs->data[3] = tm.tm_min / 10;
	tcs->data[2] = tm.tm_min % 10;
	tcs->data[1] = tm.tm_sec / 10;
	tcs->data[0] = tm.tm_sec % 10;
}

static void write_clock(struct THUNDERCLOCK_STATE*tcs)
{
	time_t timer, t1;
	struct tm tm;
	time(&timer);
	tm = *localtime(&timer);
	tm.tm_mon = tcs->wdata[9] - 1;
	tm.tm_wday = tcs->wdata[8];
	tm.tm_mday = tcs->wdata[7] * 10 + tcs->wdata[6];
	tm.tm_hour = tcs->wdata[5] * 10 + tcs->wdata[4];
	tm.tm_min = tcs->wdata[3] * 10 + tcs->wdata[2];
	tm.tm_sec = tcs->wdata[1] * 10 + tcs->wdata[0];
	t1 = mktime(&tm);
	tcs->adjust = (int)(timer - t1);
	printf("time adjust = %i\n", tcs->adjust);
}

static VOID CALLBACK tcs_callback(HWND wnd, UINT msg, struct THUNDERCLOCK_STATE*tcs, DWORD time)
{
	++ tcs->nints;
	if (tcs->ints == 1) {
//		puts("thunderclock tick");
		tcs->was_int |= 0x20;
		enable_slot_xio(tcs->st, 1);
		system_command(tcs->st->sr, SYS_COMMAND_IRQ, 0, 0);
	}
	if (tcs->ints == 2) tcs->ints = 1; // skip first int
}

static void tc_write_cmd(word adr, byte data, struct THUNDERCLOCK_STATE*tcs)
{
	byte x = tcs->creg ^ data;
	tcs->creg = data;
	if (data & 0x40 && !tcs->ints) { // ints enabled
//		system_command(tcs->st->sr, SYS_COMMAND_SET_CPU_HOOK, 1000000/64, (long)tcs);
//		SetTimer(tcs->st->sr->video_w, (UINT)tcs, 1000 / 64, (TIMERPROC)tcs_callback);
		tcs->ints = 1;
	} else if (!(data & 0x40) && tcs->ints) { // ints disabled
		tcs->ints = 0;
//		system_command(tcs->st->sr, SYS_COMMAND_SET_CPU_HOOK, 0, 0);
//		KillTimer(tcs->st->sr->video_w, (UINT)tcs);
	} 
	if (x & 4 && data & 4) { // set strobe
		tcs->cmode = tcs->creg; // write mode value
		switch (tcs->cmode) {
		case 0x24:
			tcs->rate = 0;
			logprint(0, TEXT("thunderclock rate: 64 HZ"));
			system_command(tcs->st->sr, SYS_COMMAND_SET_CPUTIMER, 1000000/64, (long)tcs);
			break;
		case 0x2C:
			tcs->rate = 1;
			logprint(0, TEXT("thunderclock rate: 256 HZ"));
			system_command(tcs->st->sr, SYS_COMMAND_SET_CPUTIMER, 1000000/256, (long)tcs);
			break;
		case 0x34:
			tcs->rate = 2;
			logprint(0, TEXT("thunderclock rate: 2048 HZ"));
			system_command(tcs->st->sr, SYS_COMMAND_SET_CPUTIMER, 1000000/2048, (long)tcs);
			break;
		}
//		logprint(0, TEXT("thunderclock: mode = %x"), tcs->cmode);
		switch (tcs->cmode & 0x30) {
		case 0x10: // read
			read_clock(tcs);
//			logprint(0, TEXT("thunderclock: get time %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x"), 
//				tcs->data[0], tcs->data[1], tcs->data[2], tcs->data[3], tcs->data[4], tcs->data[5], tcs->data[6], tcs->data[7], tcs->data[8], tcs->data[9]);
			tcs->ind = 0;
			tcs->rd = 1;
			tcs->wr = 0;
			break;
		case 0x20: // write
			tcs->ind = 4*sizeof(tcs->data) - 4;
			tcs->rd = 0;
			tcs->wr = 2;
			memset(tcs->wdata, 0, sizeof(tcs->wdata));
			break;
		case 0x30: // read/write
			read_clock(tcs);
//			logprint(0, TEXT("thunderclock: get time %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x"), 
//				tcs->data[0], tcs->data[1], tcs->data[2], tcs->data[3], tcs->data[4], tcs->data[5], tcs->data[6], tcs->data[7], tcs->data[8], tcs->data[9]);
			tcs->ind = 0;
			tcs->rd = 1;
			tcs->wr = 2;
			memset(tcs->wdata, 0, sizeof(tcs->wdata));
			break;
		}
	}
	if (x & 2 && data & 2) { // set clock
		if (tcs->wr) {
			int b, s;
			b = tcs->ind >> 2;
			s = tcs->ind & 3;
			if (tcs->creg & 1) {
				tcs->wdata[9-b] |= 1<<s;
			} else {
				tcs->wdata[9-b] &= ~(1<<s);
			}
		}
		if (tcs->cmode & 0x08) { // register shift
			tcs->ind ++;
			if (tcs->ind == 4*sizeof(tcs->data)) {
				if (tcs->wr) {
					-- tcs->wr;
					if (!tcs->wr) {
//						printf("thunderclock: set time %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", 
//							tcs->wdata[0], tcs->wdata[1], tcs->wdata[2], tcs->wdata[3], tcs->wdata[4], tcs->wdata[5], tcs->wdata[6], tcs->wdata[7], tcs->wdata[8], tcs->wdata[9]);
						write_clock(tcs);
					}
				}
				tcs->ind = 0;
			}
//			logprint(0, TEXT("thunderclock: index: %i"), tcs->ind);
		}
	}
}

static void thunderclock_io_w(word adr, byte data, struct THUNDERCLOCK_STATE*tcs) // C0X0-C0XF
{
//	logprint(0, TEXT("thunderclock i/o write: %04X, %02X"), adr, data);
	adr &= 0x0F;
	switch (adr) {
	case 0:
		tc_write_cmd(adr, data, tcs);
		break;
	case 8:
		tcs->was_int = 0;
		system_command(tcs->st->sr, SYS_COMMAND_NOIRQ, 0, 0);
		break;
	}
}

static byte tc_read_cmd(word adr, struct THUNDERCLOCK_STATE*tcs)
{
	if (1) {
		int b, s, r;
		b = tcs->ind >> 2;
		s = tcs->ind & 3;
//		system_command(tcs->st->sr, SYS_COMMAND_DUMPCPUREGS, 0, 0);
		r = ((tcs->data[b]>>s)&1)?0x80:0x00;
		if (tcs->nints&1) r |= 0x40;
		switch (tcs->rate) {
		case 0: r |= 0x20; break;
		case 1: r |= 0x28; break;
		case 2: r |= 0x30; break;
		}
		r |= tcs->was_int;
//		logprint(0, TEXT("thunderclock: read bit: %i: %i: %x"), tcs->ind, (tcs->data[b]>>s)&1, r);
		return r;
	} else return empty_read(adr, tcs);
}

static byte thunderclock_io_r(word adr, struct THUNDERCLOCK_STATE*tcs) // C0X0-C0XF
{
//	logprint(0, TEXT("thunderclock i/o read: %04X"), adr);
	adr &= 0x0F;
	switch (adr) {
	case 0:
		return tc_read_cmd(adr, tcs);
	case 8:
		tcs->was_int = 0;
		system_command(tcs->st->sr, SYS_COMMAND_NOIRQ, 0, 0);
		break;
	}
	return empty_read(adr, tcs);
}


int  thunderclock_init(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*st, struct SLOTCONFIG*cf)
{
	ISTREAM*rom;
	struct THUNDERCLOCK_STATE*tcs;

	puts("in thunderclock_init");

	tcs = calloc(1, sizeof(*tcs));
	if (!tcs) return -1;

	tcs->st = st;

	rom = isfopen(cf->cfgstr[CFG_STR_ROM]);
	if (!rom) {
		load_buf_res(cf->cfgint[CFG_INT_ROM_RES], tcs->rom, THUNDERCLOCK_ROM_SIZE);
	} else {
		isread(rom, tcs->rom, THUNDERCLOCK_ROM_SIZE);
		isclose(rom);
	}

	st->data = tcs;
	st->free = thunderclock_term;
	st->command = thunderclock_command;
	st->load = thunderclock_load;
	st->save = thunderclock_save;

	fill_rw_proc(st->io_sel, 1, thunderclock_rom_r, thunderclock_rom_w, tcs);
	fill_rw_proc(st->baseio_sel, 1, thunderclock_io_r, thunderclock_io_w, tcs);
	fill_rw_proc(&st->xio_sel, 1, thunderclock_xrom_r, empty_write, tcs);

	return 0;
}
