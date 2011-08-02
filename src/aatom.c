/*
	Agat Emulator version 1.21
	Copyright (c) NOP, nnop@newmail.ru
*/

#include "runmgr.h"
#include "runmgrint.h"
#include "runstate.h"
#include "videow.h"

#include "resource.h"

struct AATOM_DATA
{
	byte aa8255_ctrl;
	int keyrow;
	int lkeyrept;
};

void acorn_select_vmode(int mode, struct SYS_RUN_STATE*sr);
int acorn_get_vmode(struct SYS_RUN_STATE*sr);


static int free_system_aa(struct SYS_RUN_STATE*sr);
static int restart_system_aa(struct SYS_RUN_STATE*sr);
static int save_system_aa(struct SYS_RUN_STATE*sr, OSTREAM*out);
static int load_system_aa(struct SYS_RUN_STATE*sr, ISTREAM*in);
static int xio_control_aa(struct SYS_RUN_STATE*sr, int req);

static byte aaio_read(word adr, struct SYS_RUN_STATE*sr);
static void aaio_write(word adr,byte d, struct SYS_RUN_STATE*sr);


int init_system_aa(struct SYS_RUN_STATE*sr)
{
	struct AATOM_DATA*p;
	p = calloc(1, sizeof(*p));
	if (!p) return -1;
	sr->sys.ptr = p;
	sr->sys.free_system = free_system_aa;
	sr->sys.restart_system = restart_system_aa;
	sr->sys.save_system = save_system_aa;
	sr->sys.load_system = load_system_aa;
	sr->sys.xio_control = xio_control_aa;

	fill_rw_proc(sr->base_mem, BASEMEM_NBLOCKS, empty_read_zero, empty_write, sr);
	fill_rw_proc(sr->base_mem + (0xB000>>BASEMEM_BLOCK_SHIFT), 2, aaio_read, aaio_write, sr);

	puts("init_system_aa");

	return 0;
}


int free_system_aa(struct SYS_RUN_STATE*sr)
{
	if (sr->sys.ptr) free(sr->sys.ptr);
	sr->sys.ptr = NULL;
	return 0;
}

int restart_system_aa(struct SYS_RUN_STATE*sr)
{
	struct AATOM_DATA*aa = sr->sys.ptr;
	memset(aa, 0, sizeof(*aa));
	return 0;
}

int save_system_aa(struct SYS_RUN_STATE*sr, OSTREAM*out)
{
	struct AATOM_DATA*aa = sr->sys.ptr;
	puts("save_system_aa");
	oswrite(out, aa, sizeof(*aa));
	return 0;
}

int load_system_aa(struct SYS_RUN_STATE*sr, ISTREAM*in)
{
	struct AATOM_DATA*aa = sr->sys.ptr;
	puts("load_system_aa");
	isread(in, aa, sizeof(*aa));
	return 0;
}

int xio_control_aa(struct SYS_RUN_STATE*sr, int req)
{
	return 0;
}


static byte aascan_key(int row, struct SYS_RUN_STATE*sr)
{
	byte r = 0;
	int k = sr->key_down, krow, kcol;
//	if (!(k&0x80)) return 0x3F;
	if (!keyb_is_pressed(sr, k)) return 0x3F;

	if (!is_shift_pressed(sr)) r |= 0x80;
	if (!is_ctrl_pressed(sr)) r |= 0x40;

//	printf("k = %i\n", k);

	switch (k) {
	case VK_ESCAPE:
		k = 59;
		break;
	case VK_LEFT:
		r &= ~0x80;
	case VK_RIGHT:
		k = 6;
		break;
	case VK_DOWN:
		r &= ~0x80;
	case VK_UP:
		k = 7;
		break;
	case VK_CAPITAL: // caps lock
		k = 5;
		break;
	case VK_DELETE: // delete
	case VK_BACK:
		k = 15;
		break;
	case VK_INSERT: // copy
		k = 14;
		break;
	default:
		k &= 0x7F;
		if (k >= 0x20) k -= 0x20;
		break;
	}

	krow = 9 - (k % 10);
	kcol = k / 10;

	if (krow == row) r |= (~(1<<kcol)) & 0x3F;
	else r |= 0x3F;
//	printf("key[%i]=%X (%X) row = %i, col = %i\n", row, r, k, krow, kcol);
	return r;
}

static int aa_get_sync(struct SYS_RUN_STATE*sr)
{
	int ndiv = 16667;
	int tsc = cpu_get_tsc(sr) % ndiv;
	return tsc > ndiv/300;
}


static int aa_get_timer(struct SYS_RUN_STATE*sr)
{
	int ndiv = 416;
	int tsc = cpu_get_tsc(sr) % ndiv;
	return tsc < ndiv/2;
}

static byte aa8255_read(word adr, struct SYS_RUN_STATE*sr)
{
	struct AATOM_DATA*aa = sr->sys.ptr;
	byte r = 0;
	adr &= 3;
	switch (adr) {
	case 0: // port A
		r = (acorn_get_vmode(sr)<<4) | aa->keyrow;
		break;
	case 1: // port B
		r |= aascan_key(aa->keyrow, sr);
		break;
	case 2: // port C
		if (!aa_get_sync(sr)) r |= 0x80;
		if (aa_get_timer(sr)) r |= 0x10;
		if (sr->key_rept == aa->lkeyrept) r |= 0x40;
		aa->lkeyrept = sr->key_rept;
//		printf("r = %X\n", r);
//		if (!is_alt_pressed(sr)) r |= 0x40; // rept
		break;
	case 3: // Ctrl
		r = aa->aa8255_ctrl;
		break;
	}
	return r;
}

int sound_write_bit(struct SYS_RUN_STATE*sr, int v);

static void aabeep(struct SYS_RUN_STATE*sr, int v)
{
//	printf("beep %i\n", v);
	sound_write_bit(sr, v?0xFFFF:0x00);
}

static void acorn_select_keyrow(int row, struct AATOM_DATA*aa)
{
	aa->keyrow = row;
}

static void aa8255_write(word adr, byte d, struct SYS_RUN_STATE*sr)
{
	struct AATOM_DATA*aa = sr->sys.ptr;
	adr &= 3;
	switch (adr) {
	case 0: // port A
		acorn_select_vmode(d >> 4, sr);
		acorn_select_keyrow(d & 15, aa);
		break;
	case 1: // port B
		break;
	case 2: // port C
		aabeep(sr, d & 4);
		if (d & 2) {
//			puts("tape output");
		}
//		if (d & 4) 
//		aabeep(sr, (d & 4)>>2);
		break;
	case 3: // Ctrl
		if ((d == 5) || (d == 4)) { // speaker to input
			aabeep(sr, d & 1);
		}
		aa->aa8255_ctrl = d;
		break;
	}
}


static byte aa6522_read(word adr, struct SYS_RUN_STATE*sr)
{
	adr &= 15;
	return 0;
}

static void aa6522_write(word adr, byte d, struct SYS_RUN_STATE*sr)
{
	adr &= 15;
}


static byte aaio_read(word adr, struct SYS_RUN_STATE*sr)
{
	word adr0 = adr;
	byte r = 0;
	adr &= 0xFFF;
	switch (adr >> 10) {
	case 0: // B000..B3FF
		r = aa8255_read(adr, sr);
		break;
	case 1: // B400..B7FF
		// Econet
		break;
	case 2: // B800..BBFF
		r = aa6522_read(adr, sr);
		break;
	case 3: // BC00..BFFF
		break;
	}
//	printf("atom io read:  %04X: %02X\t", adr0, r);
//	system_command(sr, SYS_COMMAND_DUMPCPUREGS, 0, 0);
	return r;
}


static void aaio_write(word adr, byte d, struct SYS_RUN_STATE*sr)
{
//	printf("atom io write: %04X, %02X\t", adr, d);
//	system_command(sr, SYS_COMMAND_DUMPCPUREGS, 0, 0);
	adr &= 0xFFF;
	switch (adr >> 10) {
	case 0: // B000..B3FF
		aa8255_write(adr, d, sr);
		break;
	case 1: // B400..B7FF
		// Econet
		break;
	case 2: // B800..BBFF
		aa6522_write(adr, d, sr);
		break;
	case 3: // BC00..BFFF
		break;
	}
}


