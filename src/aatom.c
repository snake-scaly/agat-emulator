/*
	Agat Emulator version 1.21
	Copyright (c) NOP, nnop@newmail.ru
*/

#include "video/videoint.h"

/*#include "runmgr.h"
#include "runmgrint.h"
#include "runstate.h"
#include "videow.h"

#include "resource.h"*/

struct AATOM_DATA
{
	int keyrow;
	int lkeyrept;
	byte aa8255_data[3], aa8255_ctrl;
	int force_shift;

	byte aa6522_data[16];
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

	p->aa8255_ctrl = 0x9B;

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
	aa->aa8255_ctrl = 0x9B;
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

static int aa_table[60] = {
	0, 0, VK_UP, VK_RIGHT, VK_CAPITAL, VK_TAB, VK_OEM_6, VK_OEM_5, VK_OEM_4, VK_SPACE,
	'3', '2', '1', '0', VK_BACK, VK_INSERT, VK_RETURN, 0, 0, 0,
	VK_OEM_MINUS, VK_OEM_COMMA, VK_OEM_1, VK_OEM_7, '9', '8', '7', '6', '5', '4',
	'G', 'F', 'E', 'D', 'C', 'B', 'A', VK_OEM_3, VK_OEM_2, VK_OEM_PERIOD,
	'Q', 'P', 'O', 'N', 'M', 'L', 'K', 'J', 'I', 'H',
	VK_ESCAPE, 'Z', 'Y', 'X', 'W', 'V', 'U', 'T', 'S', 'R'
};


static int aa_keyb_is_pressed(struct SYS_RUN_STATE*sr, int k, int*sh)
{
	int s = aa_table[k];
	switch (s) {
	case VK_OEM_MINUS:
		if (keyb_is_pressed(sr, VK_OEM_PLUS)) { (*sh) = 10; return 1; }
		break;
	case VK_RIGHT:
		if (keyb_is_pressed(sr, VK_LEFT)) { (*sh) = 10; return 1; }
		break;
	case VK_UP:
		if (keyb_is_pressed(sr, VK_DOWN)) { (*sh) = 10; return 1; }
		break;
	case VK_BACK:
		if (keyb_is_pressed(sr, VK_DELETE)) return 1;
		break;
	}
	if (!s) return 0;
	return keyb_is_pressed(sr, s);
}


static byte aascan_key(int row, struct SYS_RUN_STATE*sr)
{
	byte r = 0, col, colmask;
	struct AATOM_DATA*aa = sr->sys.ptr;
//	int k = sr->key_down, krow, kcol;
//	if (!keyb_is_pressed(sr, k)) return 0x3F;

	if (!is_shift_pressed(sr)) r |= 0x80;
	if (!is_ctrl_pressed(sr)) r |= 0x40;

	r |= 0x3F;

	// scan columns
	for (col = 0, colmask = ~1; col < 6; ++col, colmask <<= 1, colmask |= 1) { 
		int key;
		key = col * 10 + row; //(9 - row);
		if (aa_keyb_is_pressed(sr, key, &aa->force_shift)) r &= colmask;
	}
	if (aa->force_shift) {
		-- aa->force_shift;
		r &= ~0x80;
	}

//	printf("k = %i\n", k);

/*	switch (k) {
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
*/
//	printf("key[%i]=%X\n", row, r);
	return r;
}

static int aa_get_sync(struct SYS_RUN_STATE*sr)
{
/*	int ndiv = 16667;
	int tsc = cpu_get_tsc(sr) % ndiv;
	return tsc > ndiv/100;*/
	struct VIDEO_STATE*vs;
	vs = get_video_state(sr);
	return vs->rbi >= 0;
}


static int aa_get_timer(struct SYS_RUN_STATE*sr)
{
	int ndiv = 416;
	int tsc = cpu_get_tsc(sr) % ndiv;
	return tsc < ndiv/2;
}

static int aa_get_tape(struct SYS_RUN_STATE*sr)
{
	return 0;
}

static byte aa8255_read(word adr, struct SYS_RUN_STATE*sr)
{
	struct AATOM_DATA*aa = sr->sys.ptr;
	byte r = 0;
	adr &= 3;
	switch (adr) {
	case 0: // port A
		if (aa->aa8255_ctrl & 0x10) {
			r = 0xFF;
		} else {
			r = aa->aa8255_data[adr];
		}
		break;
	case 1: // port B
		if (aa->aa8255_ctrl & 0x02) {
			r = aascan_key(aa->keyrow, sr);
		} else {
			r = aa->aa8255_data[adr];
		}
		break;
	case 2: // port C
		if (aa->aa8255_ctrl & 0x01) {
			r = aa->aa8255_data[adr] & 0x0F;
		} else {
			r = 0x0F;
		}
		if (aa->aa8255_ctrl & 0x08) {
			if (aa_get_timer(sr)) r |= 0x10;
			if (aa_get_tape(sr)) r |= 0x20;
			if (sr->key_rept == aa->lkeyrept) r |= 0x40;
			if (is_alt_pressed(sr)) r &= ~0x40; // rept
			if (!aa_get_sync(sr)) r |= 0x80;
			aa->lkeyrept = sr->key_rept;
		} else {
			r |= aa->aa8255_data[adr] & 0xF0;
		}
//		printf("port C read: %02X\n", r);
//		printf("r = %X\n", r);
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

static void aa8255_write_c(byte d, byte xd, struct SYS_RUN_STATE*sr)
{
	struct AATOM_DATA*aa = sr->sys.ptr;
	aabeep(sr, d & 4);
	if (xd & 3) {
		if (d & 1) {
			if (d & 2) {
				puts("tape output tone");
			} else {
				puts("tape output 0");
			}
		} else {
			puts("tape output 1");
		}
	}
}

static void aa8255_write(word adr, byte d, struct SYS_RUN_STATE*sr)
{
	struct AATOM_DATA*aa = sr->sys.ptr;
	adr &= 3;
	switch (adr) {
	case 0: // port A
		if (!(aa->aa8255_ctrl & 0x10)) {
			aa->aa8255_data[adr] = d;
			acorn_select_vmode(d >> 4, sr);
			acorn_select_keyrow(d & 15, aa);
		}
		break;
	case 1: // port B
		if (!(aa->aa8255_ctrl & 0x02)) {
			aa->aa8255_data[adr] = d;
		}
		break;
	case 2: // port C
//		printf("port C write: %02X\n", d);
		if (aa->aa8255_ctrl & 0x01) {           // lower: input
			d &= 0xF0;
			d |= aa->aa8255_data[adr] & 0x0F;
		}
		if (aa->aa8255_ctrl & 0x08) {           // upper: input
			d &= 0x0F;
			d |= aa->aa8255_data[adr] & 0xF0;
		}
		aa8255_write_c(d, aa->aa8255_data[adr] ^ d, sr);
		aa->aa8255_data[adr] = d;
		break;
	case 3: // Ctrl
		if (d & 0x80) {
//			printf("control write: %02X\n", d);
			aa->aa8255_ctrl = d;
		} else {
			int bit = (d >> 1) & 3;
			int set = d & 1;
			byte b = aa->aa8255_data[2];
			if ((aa->aa8255_ctrl & 0x01) && (bit < 4)) break;
			if ((aa->aa8255_ctrl & 0x08) && (bit > 3)) break;
			if (set) b |= (1<<bit);
			else b &= ~(1<<bit);
			aa8255_write_c(b, aa->aa8255_data[2] ^ b, sr);
			aa->aa8255_data[2] = b;
		}
		break;
	}
}

static void output_printer(struct SYS_RUN_STATE*sr, byte d)
{
	struct AATOM_DATA*aa = sr->sys.ptr;
	if (sr->config->slots[CONF_PRINTER].dev_type != DEV_PRINTER_ATOM) return;
	mem_proc_write(0, d, sr->slots[CONF_PRINTER].service_procs + 0);
//	printf("[%c]", d);
}


static byte aa6522_read(word adr, struct SYS_RUN_STATE*sr)
{
	struct AATOM_DATA*aa = sr->sys.ptr;
	adr &= 15;
//	printf("printer_read: %x: %x\n", adr, aa->aa6522_data[adr]);
	return aa->aa6522_data[adr];
}

static void aa6522_write(word adr, byte d, struct SYS_RUN_STATE*sr)
{
	struct AATOM_DATA*aa = sr->sys.ptr;
	adr &= 15;
//	printf("printer_write: %x, %x\n", adr, d);
	switch (adr) {
	case 1: // data register A (printer output)
		output_printer(sr, d);
		break;
	}
	aa->aa6522_data[adr] = d;
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



struct EXTROM_DATA
{
	byte rom[4096];
};

static byte extrom_rom_r(word adr, struct EXTROM_DATA*data) // A000-AFFF
{
	return data->rom[adr & 0xFFF];
}



static int extrom_term(struct SLOT_RUN_STATE*st)
{
	struct EXTROM_DATA*data = st->data;
	free(data);
	return 0;
}

int  extromaa_init(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*st, struct SLOTCONFIG*cf)
{
	struct EXTROM_DATA*data;
	ISTREAM*rom;
	const char_t*name;
	char_t buf[32];

	puts("in extromaa_init");
	data = calloc(1, sizeof(*data));
	if (!data) return -1;

	rom=isfopen(cf->cfgstr[CFG_STR_ROM]);
	if (!rom) {
		printf("no extension rom: %s", cf->cfgstr[CFG_STR_ROM]);
	} else {
		isread(rom, data->rom, sizeof(data->rom));
		isclose(rom);
	}

	fill_read_proc(sr->base_mem + (0xA000>>BASEMEM_BLOCK_SHIFT), 2, extrom_rom_r, data);

	st->data = data;
	st->free = extrom_term;

	return 0;
}

static void extram_rom_w(word adr, byte d, struct EXTROM_DATA*data) // A000-AFFF
{
	data->rom[adr & 0xFFF] = d;
}


static int save_extram(struct SLOT_RUN_STATE*st, OSTREAM*out)
{
	struct EXTROM_DATA*data = st->data;
	WRITE_ARRAY(out, data->rom);
	return 0;
}

static int load_extram(struct SLOT_RUN_STATE*st, ISTREAM*in)
{
	struct EXTROM_DATA*data = st->data;
	READ_ARRAY(in, data->rom);
	return 0;
}



int  extramaa_init(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*st, struct SLOTCONFIG*cf)
{
	struct EXTROM_DATA*data;
	ISTREAM*rom;
	const char_t*name;
	char_t buf[32];

	puts("in extramaa_init");
	data = calloc(1, sizeof(*data));
	if (!data) return -1;

	fill_rw_proc(sr->base_mem + (0xA000>>BASEMEM_BLOCK_SHIFT), 2, extrom_rom_r, extram_rom_w, data);

	st->data = data;
	st->free = extrom_term;

	return 0;
}
