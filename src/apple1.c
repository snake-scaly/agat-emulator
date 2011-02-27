#include "runmgr.h"
#include "runmgrint.h"
#include "runstate.h"
#include "videow.h"

#include "resource.h"

#define TERM_WAIT 100

struct APPLE1_DATA
{
	int dsp_cr, kbd_cr;
	int remains;
	int pos[2];
	int lsz;
};

static void kbd_write(word adr, byte data, struct SYS_RUN_STATE*sr) // d010
{
}

static void kbdcr_write(word adr, byte data, struct SYS_RUN_STATE*sr) // d011
{
	struct APPLE1_DATA*a1 = sr->ptr;
	a1->kbd_cr = data;
}

static void scroll_bitmap(struct SYS_RUN_STATE*sr)
{
	int bmp_pitch = sr->bmp_pitch;
	byte*bmp_bits = sr->bmp_bits;
	memmove(bmp_bits, bmp_bits + bmp_pitch * CHAR_H, bmp_pitch * CHAR_H * 23);
	memset(bmp_bits + bmp_pitch * CHAR_H * 23, 0, bmp_pitch * CHAR_H);
	invalidate_video_window(sr, NULL);
}


static void output_bitmap(struct SYS_RUN_STATE*sr, int x, int y, byte data, int inv)
{
	struct APPLE1_DATA*a1 = sr->ptr;
	int bmp_pitch = sr->bmp_pitch;
	byte*bmp_bits = sr->bmp_bits;
	byte*ptr;
	const byte*fnt = video_get_font(sr);
	int tc = 15, bc = 0;
	int xi, yi, xn, yn;
	ptr =(byte*)bmp_bits+((y*bmp_pitch*CHAR_H+x*CHAR_W/2));
	fnt += data * 8;
	if (inv) { tc = 0; bc = 15; }
	for (yn=8;yn;yn--,fnt++) {
		byte*p=ptr, mask;
		for (xn=8,mask=0x01;xn;xn--,mask<<=1,p++) {
			byte cl;
#ifdef DOUBLE_X
			byte c=((*fnt)&mask)?tc:bc;
			cl=c|(c<<4);
#else
			byte c1, c2;
			c1=((*fnt)&mask)?tc:bc;
			mask>>=1;
			xn--;
			c2=((*fnt)&mask)?tc:bc;
			cl=c2|(c1<<4);
#endif
			p[0]=cl;
#ifdef DOUBLE_Y
			p[bmp_pitch]=cl;
#endif
		}
		ptr+=bmp_pitch;
#ifdef DOUBLE_Y
		ptr+=bmp_pitch;
#endif
	}
	invalidate_video_window(sr, NULL);
}

void flash_system_1(struct SYS_RUN_STATE*sr)
{
	struct APPLE1_DATA*a1 = sr->ptr;
	output_bitmap(sr, a1->pos[0], a1->pos[1], 0xC0, video_get_flash(sr));
}

static void video_clear(struct SYS_RUN_STATE*sr)
{
	struct APPLE1_DATA*a1 = sr->ptr;
	int bmp_pitch = sr->bmp_pitch;
	byte*bmp_bits = sr->bmp_bits;
	memset(bmp_bits, 0, bmp_pitch * CHAR_H * 24);
	a1->lsz = 0;
	a1->pos[0] = a1->pos[1] = 0;
}

static void video_write(byte data, struct SYS_RUN_STATE*sr)
{
	struct APPLE1_DATA*a1 = sr->ptr;
//	printf("output char: %i\n", data);
	switch (data) {
	case 0xDF: // backspace
		output_bitmap(sr, a1->pos[0], a1->pos[1], 0xA0, 0);
		if (a1->pos[0]) --a1->pos[0];
		else { a1->pos[0] = 39; if (a1->pos[1]) --a1->pos[1]; }
		if (a1->lsz) -- a1->lsz;
		break;
	case 0xFF: // right
		break;
	case 0x8C: // ctrl+L, extension
		video_clear(sr);
		break;
	case 0x8A:
	case 0x8D:
		if (a1->pos[0] || !a1->lsz) {
			output_bitmap(sr, a1->pos[0], a1->pos[1], 0xA0, 0);
			a1->pos[0] = 0;
			++ a1->pos[1];
		}	
		a1->lsz = 0;
		break;
	default:
		output_bitmap(sr, a1->pos[0], a1->pos[1], data, 0);
		++ a1->pos[0];
		++ a1->lsz;
	}
	if (a1->pos[0] >= 40) { a1->pos[0] = 0; ++ a1->pos[1]; }
	if (a1->pos[1] >= 24) {
		scroll_bitmap(sr);
		a1->pos[1] = 23;
	}
	output_bitmap(sr, a1->pos[0], a1->pos[1], 0xC0, video_get_flash(sr));
}

struct VIDEO_STATE
{
	struct SYS_RUN_STATE*sr;
	// ...
};

void apaint_apple1(struct VIDEO_STATE*vs, dword addr, RECT*r)
{
//	printf("apaint_apple1(%X): %i,%i,%i,%i\n", addr, r->left, r->top, r->right, r->bottom);
}

static void dsp_write(word adr, byte data, struct SYS_RUN_STATE*sr) // d012
{
	struct APPLE1_DATA*a1 = sr->ptr;
	a1->dsp_cr |= 0x80;
	a1->remains = TERM_WAIT;
//	printf("video_write: %X: lsz = %i\n", data, a1->lsz);
	if (data & 0x80) video_write(data, sr);
}

static void dspcr_write(word adr, byte data, struct SYS_RUN_STATE*sr) // d013
{
	struct APPLE1_DATA*a1 = sr->ptr;
	a1->dsp_cr = data;
}

static byte kbd_read(word adr, struct SYS_RUN_STATE*sr) // d010
{
	byte r = keyb_read(adr, sr);
	keyb_clear(sr);
//	printf("r = %x\n", r);
	return r;
}

static byte kbdcr_read(word adr, struct SYS_RUN_STATE*sr) // d011
{
	struct APPLE1_DATA*a1 = sr->ptr;
	if (!keyb_preview(adr, sr)) a1->kbd_cr &= ~0x80;
	else a1->kbd_cr |= 0x80;
	return a1->kbd_cr;
}

static byte dsp_read(word adr, struct SYS_RUN_STATE*sr) // d012
{
	struct APPLE1_DATA*a1 = sr->ptr;
	byte r = a1->dsp_cr;
	if (!a1->remains) a1->dsp_cr &= ~0x80;
	else -- a1->remains;
	return r;
}

static byte dspcr_read(word adr, struct SYS_RUN_STATE*sr) // d013
{
	return 0;
}

static void d000_write(word adr, byte data, struct SYS_RUN_STATE*sr) // d000-d7ff
{
//	printf("d000_write: %X = %X\n", adr, data);
	switch (adr) {
	case 0xD010: // KBD
		kbd_write(adr, data, sr);
		return;
	case 0xD011: // KBD CR
		kbdcr_write(adr, data, sr);
		return;
	case 0xD012: // DSP
		dsp_write(adr, data, sr);
		return;
	case 0xD013: // DSP CR
		dspcr_write(adr, data, sr);
		return;
	}
	empty_write(adr, data, sr);
}

static byte d000_read(word adr, struct SYS_RUN_STATE*sr) // d000-d7ff
{
//	printf("d000_read: %X\n", adr);
	switch (adr) {
	case 0xD010: // KBD
		return kbd_read(adr, sr);
	case 0xD011: // KBD CR
		return kbdcr_read(adr, sr);
	case 0xD012: // DSP
		return dsp_read(adr, sr);
	case 0xD013: // DSP CR
		return dspcr_read(adr, sr);
	}
	return empty_read(adr, sr);
}

void io_write(word adr, byte data, struct MEM_PROC*io_sel); // C000-C7FF
byte io_read(word adr, struct MEM_PROC*io_sel); // C000-C7FF

int init_system_1(struct SYS_RUN_STATE*sr)
{
	struct APPLE1_DATA*p;
	p = calloc(1, sizeof(*p));
	if (!p) return -1;
	sr->ptr = p;
	fill_read_proc(sr->base_mem, BASEMEM_NBLOCKS, empty_read, NULL);
	fill_write_proc(sr->base_mem, BASEMEM_NBLOCKS, empty_write, NULL);
	fill_rw_proc(sr->base_mem + (0xD000>>BASEMEM_BLOCK_SHIFT), 1, d000_read, d000_write, sr);

	fill_read_proc(sr->io_sel, 8, empty_read, NULL);
	fill_write_proc(sr->io_sel, 8, empty_write, NULL);

	fill_read_proc(sr->base_mem + 24, 1, io_read, sr->io_sel);
	fill_write_proc(sr->base_mem + 24, 1, io_write, sr->io_sel);
	puts("init_system_1");
	return 0;
}


int free_system_1(struct SYS_RUN_STATE*sr)
{
	if (sr->ptr) free(sr->ptr);
	sr->ptr = NULL;
	return 0;
}

int restart_system_1(struct SYS_RUN_STATE*sr)
{
	struct APPLE1_DATA*a1 = sr->ptr;
	int bmp_pitch = sr->bmp_pitch;
	byte*bmp_bits = sr->bmp_bits;
	memset(a1, 0, sizeof(*a1));
	memset(bmp_bits, 0, bmp_pitch * CHAR_H * 24);
	return 0;
}

int save_system_1(struct SYS_RUN_STATE*sr, OSTREAM*out)
{
	struct APPLE1_DATA*a1 = sr->ptr;
	int bmp_pitch = sr->bmp_pitch;
	byte*bmp_bits = sr->bmp_bits;
	oswrite(out, a1, sizeof(*a1));
	oswrite(out, bmp_bits, bmp_pitch * 24 * CHAR_H);
	puts("save_system_1");
//	printf("pos: %i %i\n", a1->pos[0], a1->pos[1]);
	return 0;
}

int load_system_1(struct SYS_RUN_STATE*sr, ISTREAM*in)
{
	struct APPLE1_DATA*a1 = sr->ptr;
	int bmp_pitch = sr->bmp_pitch;
	byte*bmp_bits = sr->bmp_bits;
	isread(in, a1, sizeof(*a1));
	isread(in, bmp_bits, bmp_pitch * 24 * CHAR_H);
	puts("load_system_1");
//	printf("pos: %i %i\n", a1->pos[0], a1->pos[1]);
	invalidate_video_window(sr, NULL);
	return 0;
}


struct ACI_DATA
{
	struct SLOT_RUN_STATE*st;
	byte rom[256];
};

static byte aci_rom_r(word adr, struct ACI_DATA*aci) // CX00-CXFF
{
	return aci->rom[adr & 0xFF];
}

static void aci_io_w(word adr, byte data, struct ACI_DATA*aci) // c000-c0ff
{
//	printf("aci_io_w: %x, %x\n", adr, data);
	tape_toggle(aci->st->sr->slots + CONF_TAPE);
}

static byte aci_io_r(word adr, struct ACI_DATA*aci) // c000-c0ff
{
//	printf("aci_io_r: %x\n", adr);
	if (adr >= 0xC080) {
		byte v = tape_read(aci->st->sr->slots + CONF_TAPE);
		if (v & 0x80) adr ^= 1;
	}
	tape_toggle(aci->st->sr->slots + CONF_TAPE);
	return aci->rom[adr & 0xFF];
}


static int aci_term(struct SLOT_RUN_STATE*st)
{
	free(st->data);
	return 0;
}

int aci_init(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*st, struct SLOTCONFIG*cf)
{
	struct ACI_DATA*aci;
	ISTREAM*rom;

	aci = calloc(1, sizeof(*aci));
	if (!aci) return -1;

	aci->st = st;

	rom = isfopen(cf->cfgstr[CFG_STR_ROM]);
	if (!rom) {
		load_buf_res(cf->cfgint[CFG_INT_ROM_RES], aci->rom, 256);
	} else {
		isread(rom, aci->rom, 256);
		isclose(rom);
	}

	st->data = aci;
	st->free = aci_term;
	st->command = NULL;
	st->load = NULL;
	st->save = NULL;

	fill_rw_proc(st->io_sel, 1, aci_rom_r, empty_write, aci);
	fill_rw_proc(sr->io_sel + 0, 1, aci_io_r, aci_io_w, aci);
	return 0;
}
