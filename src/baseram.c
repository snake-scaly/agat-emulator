/*
	Agat Emulator version 1.0
	Copyright (c) NOP, nnop@newmail.ru
	baseram - emulation of base ROM module for both models
*/

#include "sysconf.h"
#include "types.h"
#include "streams.h"
#include "debug.h"

#include "memory.h"
#include "runmgrint.h"


#define MEM7_BLOCK_SIZE 0x4000
#define MEM7_BLOCK_SHIFT 0x0E

#define RAM9_BANK_SHIFT	13
#define RAM9_BANK_SIZE	0x2000

void system9_apple_mode(word adr, byte d, struct BASERAM_STATE*st);
void system9_cancel_apple_mode(word adr, byte d, struct BASERAM_STATE*st);

static const int startup_ram9_mapping[16]={0,1,2,3,4,5,6,7,0,0,0,0,0,0,0,0};

struct BASERAM_STATE
{
	struct SYS_RUN_STATE*sr;
	dword ram_size;
	dword basemem7_mapping[3];
	int   ram9_mapping[16];
	int   apple_emu;
	byte  psrom9_mode;
	int   psrom9_ofs;
	byte *ram;
	byte  apple_rom_mode;
};



static byte ram_read(word adr,struct BASERAM_STATE*st);
static void ram_write(word adr,byte d, struct BASERAM_STATE*st);

static byte ram7_read(word adr,struct BASERAM_STATE*st);
static void ram7_write(word adr,byte d, struct BASERAM_STATE*st);

static int ram_free(struct SLOT_RUN_STATE*ss)
{
	struct BASERAM_STATE*st = ss->data;
	free(st->ram);
	free(st);
	return 0;
}

static void apple_set_ext_rom_mode(int read, int write, struct BASERAM_STATE*st);
static byte apple_read_psrom_mode(word adr, struct BASERAM_STATE*st);
static void apple_write_psrom_mode(word adr, byte d, struct BASERAM_STATE*st);

static struct BASERAM_STATE*ram_init(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*ss, struct SLOTCONFIG*sc)
{
	struct BASERAM_STATE*st;

	st = calloc(1, sizeof(*st));
	if (!st) return NULL;

	st->sr = sr;
	st->ram_size = memsizes_b[sc->cfgint[CFG_INT_MEM_SIZE]];
	st->ram = malloc(st->ram_size);
	if (!st->ram) {
		free(st);
		return NULL;
	}
	clear_block(st->ram, st->ram_size);
	return st;
}

static int baseram_save(struct SLOT_RUN_STATE*ss, OSTREAM*out)
{
	struct BASERAM_STATE*st = ss->data;
	WRITE_FIELD(out, *st);
	oswrite(out, st->ram, st->ram_size);
	return 0;
}

static int baseram_load(struct SLOT_RUN_STATE*ss, ISTREAM*in)
{
	struct BASERAM_STATE*st = ss->data, st0;
	int lm = st->apple_emu;
	READ_FIELD(in, st0);
	st0.sr = st->sr;
	st0.ram = st->ram;
	*st = st0;
	isread(in, st->ram, st->ram_size);
	system_command(st->sr, SYS_COMMAND_REPAINT, 0, 0);
	if (st->apple_emu != lm) {
		if (st->apple_emu) {
			system9_apple_mode(0, 0, st);
		} else {
			system9_cancel_apple_mode(0, 0, st);
		}
	}
	switch (ss->sr->config->systype) {
	case SYSTEM_9:
		if (!st->apple_emu) break;
	case SYSTEM_A:
		{
			switch (st->apple_rom_mode&3) {
			case 2:
				apple_set_ext_rom_mode(1, 0, st);
				break;
			case 1:
				apple_set_ext_rom_mode(0, 1, st);
				break;
			case 0:
				apple_set_ext_rom_mode(0, 0, st);
				break;
			case 3:
				apple_set_ext_rom_mode(1, 1, st);
				break;
			}
		}
		break;
	}
	return 0;
}

static int rama_command(struct SLOT_RUN_STATE*ss, int cmd, int data, long param)
{
	struct BASERAM_STATE*st = ss->data;
	switch (cmd) {
	case SYS_COMMAND_HRESET:
		apple_set_ext_rom_mode(0, 1, st);
		clear_block(st->ram, st->ram_size);
	}
	return 0;
}


int rama_install(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*ss, struct SLOTCONFIG*sc)
{

	struct BASERAM_STATE*st;

	st = ram_init(sr, ss, sc);
	if (!st) return -1;

	ss->data = st;
	ss->free = ram_free;
	ss->command = rama_command;

	{
		int nb = (st->ram_size>>BASEMEM_BLOCK_SHIFT);
//		printf("ram_size = %i; nb = %i\n", st->ram_size, nb);
		fill_rw_proc(sr->base_mem, nb, ram_read, ram_write, st);
	}
	if (sr->config->slots[CONF_SLOT0].dev_type == DEV_MEMORY_XRAMA) { // language card installed
		st->ram_size = 0x10000;
		st->ram = realloc(st->ram, st->ram_size);
		if (!st->ram) {
			free(st);
			return -1;
		}
		fill_rw_proc(st->sr->baseio_sel+8, 1, apple_read_psrom_mode, apple_write_psrom_mode, st);
		st->apple_rom_mode = 0xC1;
		apple_set_ext_rom_mode(0, 1, st);
	}
	return 0;
}

static void ram7_set_state(struct BASERAM_STATE*st, int state)
{
//	printf("basemem7: state=%i\n", state&15);
	switch (st->ram_size) {
	case 0x10000: // 64K
		if (state&1) {
			st->basemem7_mapping[2]=3 << MEM7_BLOCK_SHIFT;
		} else {
			st->basemem7_mapping[2]=2 << MEM7_BLOCK_SHIFT;
		}
		break;
	case 0x20000: // 128K
		if (state&8) {
			static const int no1[8]={1,1,4,5,1,1,4,5};
			static const int no2[8]={2,3,6,7,2,3,6,7};
			st->basemem7_mapping[0]=0;
			st->basemem7_mapping[1]=no1[state&7]<<MEM7_BLOCK_SHIFT;
			st->basemem7_mapping[2]=no2[state&7]<<MEM7_BLOCK_SHIFT;
		} else {
			static const int no[8]={2,3,6,7,2,3,4,5};
			st->basemem7_mapping[0]=0;
			st->basemem7_mapping[1]=1<<MEM7_BLOCK_SHIFT;
			st->basemem7_mapping[2]=no[state&7]<<MEM7_BLOCK_SHIFT;
		}
		break;
	}
}


void basemem7_state_w(word adr,byte data, struct BASERAM_STATE*st) // C0F0-C0FF
{
//	printf("basemem7_state_w @ %04X: %02X\n", adr, data);
	ram7_set_state(st, adr&0x0F);
}

byte basemem7_state_r(word adr, struct BASERAM_STATE*st) // C0F0-C0FF
{
//	printf("basemem7_state_r @ %04X\n", adr);
	ram7_set_state(st, adr&0x0F);
	return empty_read(adr, st);
}

static int ram7_command(struct SLOT_RUN_STATE*ss, int cmd, int data, long param)
{
	struct BASERAM_STATE*st = ss->data;
	switch (cmd) {
	case SYS_COMMAND_HRESET:
		st->basemem7_mapping[0] = 0;
		st->basemem7_mapping[1] = 1<<MEM7_BLOCK_SHIFT;
		st->basemem7_mapping[2] = 2<<MEM7_BLOCK_SHIFT;
		clear_block(st->ram, st->ram_size);
		if (st->ram_size > 0x8000) {
			fill_rw_proc(st->sr->base_mem + (0x8000>>BASEMEM_BLOCK_SHIFT), 0x4000>>BASEMEM_BLOCK_SHIFT, ram7_read, ram7_write, st);
		} else {
			fill_rw_proc(st->sr->base_mem + (0x8000>>BASEMEM_BLOCK_SHIFT), 0x4000>>BASEMEM_BLOCK_SHIFT, empty_read, empty_write, NULL);
		}
		break;
	case SYS_COMMAND_XRAM_RELEASE:
		puts("baseram: restore ram");
		if (st->ram_size > 0x8000) {
			fill_rw_proc(st->sr->base_mem + (0x8000>>BASEMEM_BLOCK_SHIFT), 0x4000>>BASEMEM_BLOCK_SHIFT, ram7_read, ram7_write, st);
		} else {
			fill_rw_proc(st->sr->base_mem + (0x8000>>BASEMEM_BLOCK_SHIFT), 0x4000>>BASEMEM_BLOCK_SHIFT, empty_read, empty_write, NULL);
		}
		return 1;
	}
	return 0;
}

int ram7_install(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*ss, struct SLOTCONFIG*sc)
{
	struct BASERAM_STATE*st;
	st = ram_init(sr, ss, sc);
	if (!st) return -1;

	ss->data = st;
	ss->command = ram7_command;
	ss->free = ram_free;

	st->basemem7_mapping[0] = 0;
	st->basemem7_mapping[1] = 1<<MEM7_BLOCK_SHIFT;
	st->basemem7_mapping[2] = 2<<MEM7_BLOCK_SHIFT;

	switch (st->ram_size) {
	case 0x8000: // 32K
		fill_rw_proc(sr->base_mem, 0x8000>>BASEMEM_BLOCK_SHIFT, ram_read, ram_write, st);
		break;
	case 0x10000: // 64K
		fill_rw_proc(sr->base_mem, 0x8000>>BASEMEM_BLOCK_SHIFT, ram_read, ram_write, st);
		fill_rw_proc(sr->base_mem + (0x8000>>BASEMEM_BLOCK_SHIFT), 0x4000>>BASEMEM_BLOCK_SHIFT, ram7_read, ram7_write, st);
		break;
	case 0x20000: // 128K
		fill_rw_proc(sr->base_mem, 0x4000>>BASEMEM_BLOCK_SHIFT, ram_read, ram_write, st);
		fill_rw_proc(sr->base_mem + (0x4000>>BASEMEM_BLOCK_SHIFT), 0x8000>>BASEMEM_BLOCK_SHIFT, ram7_read, ram7_write, st);
		break;
	default:
		abort();
	}
	if (st->ram_size > 0x8000)
		fill_rw_proc(sr->baseio_sel + 15, 1, basemem7_state_r, basemem7_state_w, st);
	return 0;
}

static __inline dword ram7_offset(struct BASERAM_STATE*st, word adr)
{
	int ind=adr>>MEM7_BLOCK_SHIFT;
	return st->basemem7_mapping[ind]+(adr&(MEM7_BLOCK_SIZE-1));
}

static byte ram7_read(word adr,struct BASERAM_STATE*st)
{
	return st->ram[ram7_offset(st, adr)];
}

static void ram7_write(word adr, byte d,struct BASERAM_STATE*st)
{
	byte ld;
	dword radr;
	radr = ram7_offset(st, adr);
	ld = st->ram[radr];
	if (ld == d) return;
	st->ram[radr] = d;
	vid_invalidate_addr(st->sr, radr);
}

static byte ram_read(word adr,struct BASERAM_STATE*st)
{
	return st->ram[adr];
}

static void ram_write(word adr,byte d,struct BASERAM_STATE*st)
{
	byte ld = st->ram[adr];
	if (ld == d) return;
	st->ram[adr] = d;
	vid_invalidate_addr(st->sr, adr);
}


static byte baseram9_read(word adr,struct BASERAM_STATE*st);
static void baseram9_write(word adr,byte d,struct BASERAM_STATE*st);

static byte baseram9_read_psrom(word adr,struct BASERAM_STATE*st);
static void baseram9_write_psrom(word adr,byte d,struct BASERAM_STATE*st);

static byte baseram9_read_mapping(word adr,struct BASERAM_STATE*st);
static void baseram9_write_mapping(word adr,byte d,struct BASERAM_STATE*st);

static byte baseram9_read_psrom_mode(word adr,struct BASERAM_STATE*st);
static void baseram9_write_psrom_mode(word adr,byte d,struct BASERAM_STATE*st);

static void baseram9_psrom_setmodeprocs(struct BASERAM_STATE*st, int rd, int wr, int flags)
{
//	printf("baseram9_psrom_mode: %i %i\n", rd, wr);
	if (wr) { // write access
		if (flags&1) fill_write_proc(st->sr->base_mem+26,2,baseram9_write_psrom, st);
		if (flags&2) fill_write_proc(st->sr->base_mem+28,4,baseram9_write, st);
	} else {
		if (flags&1) fill_write_proc(st->sr->base_mem+26,2,empty_write, NULL);
		if (flags&2) fill_write_proc(st->sr->base_mem+28,4,empty_write, NULL);
	}
	if (rd) { // read_access
		if (flags&1) fill_read_proc(st->sr->base_mem+26,2,baseram9_read_psrom, st);
		if (flags&2) fill_read_proc(st->sr->base_mem+28,4,baseram9_read, st);
	} else {
			if (flags&1) fill_read_proc(st->sr->base_mem+26,2,empty_read_addr, NULL);
			if (flags&2) fill_read_proc(st->sr->base_mem+28,3,empty_read_addr, NULL);
			if (flags&2) system_command(st->sr, SYS_COMMAND_PSROM_RELEASE, 0, 0);
	}
}

static void baseram9_restore_segment(struct BASERAM_STATE*st, int ind)
{
	if (ind<6) {
		fill_read_proc(st->sr->base_mem+ind*4, 4, baseram9_read, st);
		fill_write_proc(st->sr->base_mem+ind*4, 4, baseram9_write, st);
	} else {
		switch (ind) {
		case 6: // d000
			switch (st->psrom9_mode&3) {
			case 0: baseram9_psrom_setmodeprocs(st, 1, 0, 1); break;
			case 1: baseram9_psrom_setmodeprocs(st, 0, 1, 1); break;
			case 2: baseram9_psrom_setmodeprocs(st, 1, 0, 1); break;
			case 3: baseram9_psrom_setmodeprocs(st, 1, 1, 1); break;
			}
			break;
		case 7: // e000
			switch (st->psrom9_mode&3) {
			case 0: baseram9_psrom_setmodeprocs(st, 1, 0, 2); break;
			case 1: baseram9_psrom_setmodeprocs(st, 0, 1, 2); break;
			case 2: baseram9_psrom_setmodeprocs(st, 1, 0, 2); break;
			case 3: baseram9_psrom_setmodeprocs(st, 1, 1, 2); break;
			}
			break;
		}
	}
}

void baseram9_write_psrom_mode(word adr,byte d, struct BASERAM_STATE*st)
{
	st->psrom9_mode=(adr&0x0F);
//	printf("baseram9_read_psrom_mode(%x) = %x\n",adr, st->psrom9_mode);
	if (st->psrom9_mode&8) st->psrom9_ofs=RAM9_BANK_SIZE/2;
	else st->psrom9_ofs=0;
	switch (st->psrom9_mode&3) {
	case 0: baseram9_psrom_setmodeprocs(st, 1, 0, 3); break;
	case 1: baseram9_psrom_setmodeprocs(st, 0, 1, 3); break;
	case 2: baseram9_psrom_setmodeprocs(st, 1, 0, 3); break;
	case 3: baseram9_psrom_setmodeprocs(st, 1, 1, 3); break;
	}
}


static int ram9_command(struct SLOT_RUN_STATE*ss, int cmd, int data, long param)
{
	struct BASERAM_STATE*st = ss->data;
	switch (cmd) {
	case SYS_COMMAND_HRESET:
		clear_block(st->ram, st->ram_size);
		system9_cancel_apple_mode(0, 0, st);
		memcpy(st->ram9_mapping, startup_ram9_mapping, sizeof(st->ram9_mapping));
		st->apple_rom_mode = 0xC0;
		baseram9_write_psrom_mode(1, 0, st);
		break;
	case SYS_COMMAND_BASEMEM9_RESTORE:
//		printf("*** baseram9_restore_segment(%i)\n", param);
		baseram9_restore_segment(st, param);
		return 1;
	}
	return 0;
}


static int get_addr_bank9(struct BASERAM_STATE*st, word adr, int bank)
{
	return (st->ram9_mapping[bank]<<RAM9_BANK_SHIFT)|(adr&(RAM9_BANK_SIZE-1));
}

static int get_addr9(struct BASERAM_STATE*st, word adr)
{
	return get_addr_bank9(st, adr, adr>>RAM9_BANK_SHIFT);
}

byte baseram9_read(word adr, struct BASERAM_STATE*st)
{
	return st->ram[get_addr9(st,adr)];
}

void baseram9_write(word adr, byte d, struct BASERAM_STATE*st)
{
	int ad1=get_addr9(st, adr);
	if (st->ram[ad1]==d) return;
	st->ram[ad1]=d;
	vid_invalidate_addr(st->sr, ad1);
}

byte baseram9_read_psrom(word adr,struct BASERAM_STATE*st)
{
	return baseram9_read((word)(adr+st->psrom9_ofs-RAM9_BANK_SIZE/2), st);
}

void baseram9_write_psrom(word adr,byte d, struct BASERAM_STATE*st)
{
	baseram9_write((word)(adr + st->psrom9_ofs-RAM9_BANK_SIZE/2), d, st);
}

byte apple_read_psrom_d(word adr, struct BASERAM_STATE*st)
{
	return st->ram[get_addr_bank9(st, adr, 15) + 
			st->psrom9_ofs - RAM9_BANK_SIZE/2];
}

byte apple_read_psrom_e(word adr, struct BASERAM_STATE*st)
{
	return st->ram[get_addr_bank9(st, adr, 14)];
}

byte apple_read_lang_d(word adr, struct BASERAM_STATE*st)
{
	return st->ram[adr + st->psrom9_ofs - RAM9_BANK_SIZE/2];
}

byte apple_read_lang_e(word adr, struct BASERAM_STATE*st)
{
	return st->ram[adr];
}

void apple_write_lang_d(word adr, byte d, struct BASERAM_STATE*st)
{
	st->ram[adr + st->psrom9_ofs - RAM9_BANK_SIZE/2] = d;
}

void apple_write_lang_e(word adr, byte d, struct BASERAM_STATE*st)
{
	st->ram[adr] = d;
}


void apple_write_psrom_d(word adr, byte d, struct BASERAM_STATE*st)
{
	st->ram[get_addr_bank9(st, adr, 15) + 
			st->psrom9_ofs - RAM9_BANK_SIZE/2] = d;
}

void apple_write_psrom_e(word adr, byte d, struct BASERAM_STATE*st)
{
	st->ram[get_addr_bank9(st, adr, 14)] = d;
}


byte baseram9_read_mapping(word adr, struct BASERAM_STATE*st)
{
	int ind=(adr&0xF0)>>4;
//	printf("baseram9_read_mapping[%x] = %i\n", adr, st->ram9_mapping[ind]);
	return (adr&0xF0)|st->ram9_mapping[ind];
}

void baseram9_write_mapping(word adr, byte d, struct BASERAM_STATE*st)
{
	int ind=(adr&0xF0)>>4;
//	printf("ram9_mapping[%i]=%x (%x)\n",ind, adr&0x0F, adr);
	st->ram9_mapping[ind]=adr&0x0F;
}

byte baseram9_read_psrom_mode(word adr, struct BASERAM_STATE*st)
{
	byte res = st->psrom9_mode|0xF0;
	if (!(res & 3)) res |= 2;
//	printf("baseram9_read_psrom_mode(%x) = %x\n",adr, st->psrom9_mode|(adr&0xF0)|4);
	return res;
}

void apple_set_ext_rom_mode(int read, int write, struct BASERAM_STATE*st)
{
//	printf("apple language card emulation: read=%i, write=%i\n", read, write);
	if (st->apple_emu) {
		fill_read_proc(st->sr->base_mem+26,2,read?apple_read_psrom_d:baseram9_read_psrom,st);
		fill_read_proc(st->sr->base_mem+28,4,read?apple_read_psrom_e:baseram9_read,st);
		fill_write_proc(st->sr->base_mem+26,2,write?apple_write_psrom_d:empty_write,st);
		fill_write_proc(st->sr->base_mem+28,4,write?apple_write_psrom_e:empty_write,st);
	} else {
		if (!read) {
			system_command(st->sr, SYS_COMMAND_PSROM_RELEASE, 0, 0);
		} else {
			fill_read_proc(st->sr->base_mem+26,2,apple_read_lang_d,st);
			fill_read_proc(st->sr->base_mem+28,4,apple_read_lang_e,st);
		}
		fill_write_proc(st->sr->base_mem+26,2,write?apple_write_lang_d:empty_write,st);
		fill_write_proc(st->sr->base_mem+28,4,write?apple_write_lang_e:empty_write,st);
	}
}

byte apple_read_psrom_mode(word adr, struct BASERAM_STATE*st)
{
	int hi = adr & 8;
	int mde = adr & 3;
	byte last_mode = st->apple_rom_mode;
//	printf("apple_read_psrom_mode(%x)\n", adr);
	if (hi) st->psrom9_ofs=RAM9_BANK_SIZE/2;
	else st->psrom9_ofs=0;
	switch (mde) {
	case 0:
		st->apple_rom_mode = 0xC2 | hi;
		apple_set_ext_rom_mode(1, 0, st);
		break;
	case 1:
		st->apple_rom_mode = 0xC1 | hi;
		apple_set_ext_rom_mode(0, 1, st);
		break;
	case 2:
		st->apple_rom_mode = 0xC0 | hi;
		apple_set_ext_rom_mode(0, 0, st);
		break;
	case 3: { int dr;
		dr = (((st->apple_rom_mode&3)==1) || ((st->apple_rom_mode&3)==3));
		st->apple_rom_mode = 0xC3 | hi;
		apple_set_ext_rom_mode(1, dr, st);
		break; }
	}
	return last_mode;
}

void apple_write_psrom_mode(word adr, byte d, struct BASERAM_STATE*st)
{
	apple_read_psrom_mode(adr, st);
}


void system9_apple_mode(word adr, byte d, struct BASERAM_STATE*st)
{
	puts("apple mode");
	st->sr->apple_emu = st->apple_emu = 1;
	fill_rw_proc(st->sr->baseio_sel+8, 1, apple_read_psrom_mode, apple_write_psrom_mode, st);
	fill_rw_proc(st->sr->io_sel+1, 1, empty_read_addr, empty_write, st);
	fill_read_proc(st->sr->base_mem,24,baseram9_read, st);
	fill_write_proc(st->sr->base_mem,24,baseram9_write, st);
	st->apple_rom_mode = 0xC1;
	apple_set_ext_rom_mode(0, 1, st);
	system_command(st->sr, SYS_COMMAND_APPLEMODE, 1, 0);
}

void system9_cancel_apple_mode(word adr, byte d, struct BASERAM_STATE*st)
{
	puts("cancel apple mode");
	st->sr->apple_emu = st->apple_emu = 0;
	fill_rw_proc(st->sr->base_mem, 24, baseram9_read, baseram9_write, st);
	fill_read_proc(st->sr->base_mem+26, 5, empty_read_addr, NULL);
	fill_write_proc(st->sr->base_mem+26, 2, baseram9_write_psrom, st);
	fill_write_proc(st->sr->base_mem+28,4, baseram9_write, st);
	fill_rw_proc(st->sr->baseio_sel+8, 1, baseram9_read_psrom_mode, baseram9_write_psrom_mode, st);
	fill_rw_proc(st->sr->io_sel+1, 1, baseram9_read_mapping, baseram9_write_mapping, st);
	fill_write_proc(st->sr->baseio_sel + 15, 1, system9_apple_mode, st);
	system_command(st->sr, SYS_COMMAND_APPLEMODE, 0, 0);
}

int ram9_install(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*ss, struct SLOTCONFIG*sc)
{
	struct BASERAM_STATE*st;
	st = ram_init(sr, ss, sc);
	if (!st) return -1;

	ss->data = st;
	ss->command = ram9_command;
	ss->free = ram_free;

	clear_block(st->ram, st->ram_size);
	memcpy(st->ram9_mapping, startup_ram9_mapping, sizeof(st->ram9_mapping));
	baseram9_write_psrom_mode(1, 0, st);

	fill_rw_proc(sr->base_mem, 24, baseram9_read, baseram9_write, st);
	fill_read_proc(sr->base_mem+26, 5, empty_read_addr, NULL);
	fill_write_proc(sr->base_mem+26, 2, baseram9_write_psrom, st);
	fill_write_proc(sr->base_mem+28,4, baseram9_write, st);
	fill_rw_proc(sr->baseio_sel+8, 1, baseram9_read_psrom_mode, baseram9_write_psrom_mode, st);
	fill_rw_proc(sr->io_sel+1, 1, baseram9_read_mapping, baseram9_write_mapping, st);
	fill_write_proc(sr->baseio_sel + 15, 1, system9_apple_mode, st);
	return 0;
}

int ram_install(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*ss, struct SLOTCONFIG*sc)
{
	ss->save = baseram_save;
	ss->load = baseram_load;
	switch (sr->config->systype) {
	case SYSTEM_7:
		return ram7_install(sr, ss, sc);
	case SYSTEM_9:
		return ram9_install(sr, ss, sc);
	case SYSTEM_A:
		return rama_install(sr, ss, sc);
	}
	return -1;
}

byte* ramptr(struct SYS_RUN_STATE*sr)
{
	struct BASERAM_STATE*st = sr->slots[CONF_MEMORY].data;
	return st->ram;
}

int basemem_n_blocks(struct SYS_RUN_STATE*sr)
{
	struct BASERAM_STATE*st = sr->slots[CONF_MEMORY].data;
	return st->ram_size>>BASEMEM_BLOCK_SHIFT;
}

