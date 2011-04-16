/*
	Agat Emulator version 1.0
	Copyright (c) NOP, nnop@newmail.ru
	fdd - emulation of Shugart floppy drive
*/

#include "common.h"
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

#include "syslib.h"
#include "debug.h"

#include "localize.h"

#ifndef W_OK
#define W_OK 02
#endif

#define FDD_TRACK_COUNT 34
#define FDD_SECTOR_COUNT 16
#define FDD_SECTOR_DATA_SIZE 256
#define NIBBLE_TRACK_LEN 6656

#define FDD_ROM_SIZE 256


static byte std_sig[]="Agathe emulator virtual disk\x0D\x0A\x1A""AD";

struct FDD_STATE
{
	int	CurrentPhase;
	int	ActiveDrive;
	int	MotorOn;
	int	ReadMode;
	byte 	WriteData;
	int	C0XD;
};

struct FDD_DRIVE_DATA
{
	int	Phase;
	int	Track;
	byte	TrackData[6656];
	int	TrackIndex, TrackLen;
	char_t	disk_name[1024];
	IOSTREAM *disk;
	int	rawfmt; // 1 if disk in raw (nibble) format
	int	prodos; // 1 if sectors in prodos order
	byte	disk_header[256];
	int	error;
	int	readonly;
	int	present;
	byte	volume;
	int	start_ofs;
	int	dirty;

	HMENU	submenu;
};

struct FDD_DATA
{
	struct FDD_DRIVE_DATA	drives[2];
	struct FDD_STATE	state;
	int	drv;
	int	initialized;
	int	time;
	int	type;
	byte	fdd_rom[FDD_ROM_SIZE];
	struct SYS_RUN_STATE	*sr;
	struct SLOT_RUN_STATE	*st;
	int	last_tsc;
	int	use_fast;
	int	ntries;
	byte	last_read;
};




static void fdd_io_write(unsigned short a,unsigned char d, struct FDD_DATA*xd);
static byte fdd_io_read(unsigned short a, struct FDD_DATA*xd);
static byte fdd_rom_read(unsigned short a, struct FDD_DATA*xd);
static byte get_cs(byte*data,int n);


static void fill_fdd_drive(struct FDD_DRIVE_DATA*data)
{
	data->disk=0;
	data->Phase=20;
	data->Track=0;
	data->volume=254;
	data->dirty = 1;
}

static void fill_fdd1(struct FDD_DATA*data)
{
	fill_fdd_drive(data->drives);
	fill_fdd_drive(data->drives+1);
	data->initialized=0;
	data->time=0;
	data->type=0;
	data->drv=0;

	data->state.CurrentPhase=0;
	data->state.ActiveDrive=0;
	data->state.MotorOn=0;
	data->state.ReadMode=1;
	data->state.C0XD=0;
}

int open_fdd1(struct FDD_DRIVE_DATA*drv,const char_t*name,int ro, int no)
{
	if (drv->disk) {
		isclose(drv->disk);
		drv->disk = NULL;
	}
	drv->dirty = 1;
	if (!name) return 1;
//	if (access(name,W_OK)) ro=1;
	drv->disk=ro?isfopen(name):iosfopen(name);
	if (!ro&&!drv->disk) {
		ro = 1;
		drv->disk=isfopen(name);
	}
	if (!drv->disk) {
		ro = 1;
		drv->disk = isfopen_res(MAKEINTRESOURCE(33+no));
	}
	if (!drv->disk) {
		errprint(TEXT("can't open disk file %s"),name);
		return -1;
	}
	if (isread(drv->disk,drv->disk_header,sizeof(drv->disk_header))!=sizeof(drv->disk_header)) {
		errprint(TEXT("can't read header from disk %s"),name);
		memset(drv->disk_header, 0, sizeof(drv->disk_header));
//		isclose(drv->disk);
//		drv->disk=0;
//		return -1;
	}
	drv->readonly=ro;
	if (!memcmp(std_sig,drv->disk_header,sizeof(std_sig)-1)) {
		if (drv->disk_header[48]) drv->readonly=1;
		drv->start_ofs=256;
	} else drv->start_ofs=0;
	if (strstr(name,"nib") || strstr(name,"NIB")) {
		puts("using nibble format");
		drv->rawfmt = 1;
	} else drv->rawfmt = 0;
	if (strstr(name,".po") || strstr(name,".PO")) {
		puts("using ProDOS order");
		drv->prodos = 1;
	} else drv->prodos = 0;
	drv->error = 0;
	return 0;
}


static int remove_fdd1(struct SLOT_RUN_STATE*sr)
{
	struct FDD_DATA*data = sr->data;
	open_fdd1(data->drives+0, NULL, 0, 0);
	open_fdd1(data->drives+1, NULL, 0, 1);
	return 0;
}

#define FDD1_BASE_CMD 7000

static int init_menu(struct FDD_DRIVE_DATA*drv, int s, int d, HMENU menu)
{
	TCHAR buf[1024], buf1[1024];
	if (drv->submenu) DeleteMenu(menu, (UINT)drv->submenu, MF_BYCOMMAND);
	if (drv->present) {
		drv->submenu = CreatePopupMenu();
		AppendMenu(drv->submenu, MF_STRING, FDD1_BASE_CMD + (s*4 + d*2 + 0), 
			localize_str(LOC_FDD, 2, buf1, sizeof(buf1)));
//			TEXT("Вставить диск..."));
		AppendMenu(drv->submenu, MF_STRING, FDD1_BASE_CMD + (s*4 + d*2 + 1), 
			localize_str(LOC_FDD, 3, buf1, sizeof(buf1)));
//			TEXT("Извлечь диск"));

		//TEXT("Дисковод 140Кб (S%i,D%i)"),
		wsprintf(buf, localize_str(LOC_FDD, 1, buf1, sizeof(buf1)), s, d + 1);
		AppendMenu(menu, MF_POPUP, (UINT_PTR)drv->submenu, buf);
	} else drv->submenu = NULL;
	return 0;
}

static int free_menu(struct FDD_DRIVE_DATA*drv, int s, int d, HMENU menu)
{
	if (drv->submenu) {
		DeleteMenu(menu, (UINT)drv->submenu, MF_BYCOMMAND);
	}
	return 0;
}

static int update_menu(struct FDD_DRIVE_DATA*drv, int s, int d, HMENU menu)
{
	if (drv->present) {
		EnableMenuItem(drv->submenu, FDD1_BASE_CMD + (s*4 + d*2 + 1), (drv->disk?MF_ENABLED:MF_GRAYED)|MF_BYCOMMAND);
	}
	return 0;
}

static int fdd1_drvcmd(HWND wnd, struct FDD_DRIVE_DATA*drv, int cmd, int s, int d)
{
	if (drv->present) {
		if (cmd == FDD1_BASE_CMD + (s*4 + d*2 + 0)) {
			if (select_disk(wnd, drv->disk_name, &drv->readonly)) {
				open_fdd1(drv, drv->disk_name, drv->readonly, d);
			}
		} else if (cmd == FDD1_BASE_CMD + (s*4 + d*2 + 1)) {
			open_fdd1(drv, NULL, 0, d);
			drv->disk_name[0] = 0;
		}
	}
	return 0;
}

static int fdd1_wincmd(HWND wnd, struct FDD_DATA*data, int cmd, int s)
{
	fdd1_drvcmd(wnd, data->drives + 0, cmd, s, 0);
	fdd1_drvcmd(wnd, data->drives + 1, cmd, s, 1);
	return 0;
}

static int fdd1_command(struct SLOT_RUN_STATE*st, int cmd, int cdata, long param)
{
	struct FDD_DATA*data = st->data;
	HMENU menu;
	switch (cmd) {
	case SYS_COMMAND_RESET:
	case SYS_COMMAND_HRESET:
		if (data->state.MotorOn && data->use_fast) 
			system_command(st->sr, SYS_COMMAND_FAST, 0, 0);
		data->state.MotorOn = 0;
		data->state.ReadMode = 1;
		break;
	case SYS_COMMAND_INITMENU:
		menu = (HMENU) param;
		init_menu(data->drives + 0, st->sc->slot_no, 0, menu);
		init_menu(data->drives + 1, st->sc->slot_no, 1, menu);
		break;
	case SYS_COMMAND_UPDMENU:
		menu = (HMENU) param;
		update_menu(data->drives + 0, st->sc->slot_no, 0, menu);
		update_menu(data->drives + 1, st->sc->slot_no, 1, menu);
		break;
	case SYS_COMMAND_FREEMENU:
		menu = (HMENU) param;
		free_menu(data->drives + 0, st->sc->slot_no, 0, menu);
		free_menu(data->drives + 1, st->sc->slot_no, 1, menu);
		break;
	case SYS_COMMAND_WINCMD:
		fdd1_wincmd((HWND)param, data, cdata, st->sc->slot_no);
		break;
	}
	return 0;
}

static int fdd1_save(struct SLOT_RUN_STATE*st, OSTREAM*out)
{
	struct FDD_DATA*data = st->data;

	WRITE_FIELD(out, data->drives[0].present);
	WRITE_FIELD(out, data->drives[0].readonly);
	WRITE_ARRAY(out, data->drives[0].disk_name);

	WRITE_FIELD(out, data->drives[1].present);
	WRITE_FIELD(out, data->drives[1].readonly);
	WRITE_ARRAY(out, data->drives[1].disk_name);
	return 0;
}

static int fdd1_load(struct SLOT_RUN_STATE*st, ISTREAM*in)
{
	struct FDD_DATA*data = st->data;

	READ_FIELD(in, data->drives[0].present);
	READ_FIELD(in, data->drives[0].readonly);
	READ_ARRAY(in, data->drives[0].disk_name);

	READ_FIELD(in, data->drives[1].present);
	READ_FIELD(in, data->drives[1].readonly);
	READ_ARRAY(in, data->drives[1].disk_name);

	if (data->drives[0].present) {
		open_fdd1(data->drives, data->drives[0].disk_name, data->drives[0].readonly & 1, 0);
	}
	if (data->drives[1].present) {
		open_fdd1(data->drives + 1, data->drives[1].disk_name, data->drives[1].readonly & 1, 1);
	}
	data->initialized = 1;
	return 0;
}


int  fdd1_init(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*st, struct SLOTCONFIG*cf)
{
	struct FDD_DATA*data;
	ISTREAM*rom;
	const char_t*name;
	char_t buf[32];

	data = calloc(1, sizeof(*data));
	if (!data) return -1;

	data->sr = sr;
	data->st = st;

	rom=isfopen(cf->cfgstr[CFG_STR_DRV_ROM]);
	if (!rom) {
		load_buf_res(cf->cfgint[CFG_INT_DRV_ROM_RES], data->fdd_rom, FDD_ROM_SIZE);
	} else {
		isread(rom, data->fdd_rom, sizeof(data->fdd_rom));
		isclose(rom);
	}

	fill_fdd1(data);
	if (cf->cfgint[CFG_INT_DRV_TYPE1] == DRV_TYPE_SHUGART) {
		data->drives[0].present = 1;
		sprintf(buf, "s%id1", st->sc->slot_no);
		name = sys_get_parameter(buf);
		if (!name) name = cf->cfgstr[CFG_STR_DRV_IMAGE1];
		strcpy(data->drives[0].disk_name, name);
		open_fdd1(data->drives, name, cf->cfgint[CFG_INT_DRV_RO_FLAGS] & 1, 0);
	}
	if (cf->cfgint[CFG_INT_DRV_TYPE2] == DRV_TYPE_SHUGART) {
		data->drives[1].present = 1;
		sprintf(buf, "s%id2", st->sc->slot_no);
		name = sys_get_parameter(buf);
		if (!name) name = cf->cfgstr[CFG_STR_DRV_IMAGE2];
		strcpy(data->drives[1].disk_name, name);
		open_fdd1(data->drives+1, name, cf->cfgint[CFG_INT_DRV_RO_FLAGS] & 2, 1);
	}
	data->initialized = 1;
	fill_read_proc(st->baseio_sel, 1, fdd_io_read, data);
	fill_write_proc(st->baseio_sel, 1, fdd_io_write, data);
	fill_read_proc(st->io_sel, 1, fdd_rom_read, data);
	fill_write_proc(st->io_sel, 1, empty_write, data);

	st->data = data;
	st->free = remove_fdd1;
	st->command = fdd1_command;
	st->save = fdd1_save;
	st->load = fdd1_load;

	data->use_fast = cf->cfgint[CFG_INT_DRV_FAST];

	return 0;
}

static void fdd_rot(struct FDD_DRIVE_DATA *d)
{
	d->TrackIndex++;
	if (d->TrackIndex>=d->TrackLen)
		d->TrackIndex = 0;
}


static byte fdd_read_data(struct FDD_DATA*data)
{
	byte r;
	struct FDD_DRIVE_DATA *d = data->drives+data->drv;

	if (!d->present) {
		return 0xFF;
	}	
	if (!data->state.MotorOn) {
		if (!(rand()%2)) data->last_read = (rand()&0x7F) | 0x80;
		return data->last_read;
	}	

	if (!d->disk) { d->error = 1; return data->last_read = rand()%0x100; }

	if (!(data->time&3)) r = rand()&0x7F;
	else {
		r = d->TrackData[d->TrackIndex];
//		printf("TrackData[%i] = %X\n", d->TrackIndex, r);
		fdd_rot(d);
	}
	data->last_read = r;
	data->time++;
	return r;
}

static void fdd_write_data(struct FDD_DATA*data)
{
	struct FDD_DRIVE_DATA *d = data->drives+data->drv;
	int tsc;
	if (d->readonly) return;
	tsc = cpu_get_tsc(data->sr);

//	printf("Writeing: tsc = %i\n", tsc-data->last_tsc);
	data->last_tsc = tsc;

//	printf("Writing[%i]: %02X (%02X)\n", d->TrackIndex, data->state.WriteData, d->TrackData[d->TrackIndex]);
	d->TrackData[d->TrackIndex] = data->state.WriteData;
	fdd_rot(d);
}

static void fdd_nowrite(struct FDD_DATA*data,byte d)
{
}

static byte fdd_noread(struct FDD_DATA*data)
{
	return 0xFF;
}

void fdd_io_write(unsigned short a,unsigned char d,struct FDD_DATA*data)
{
	a&=0x0F;
	switch (a) {
	case 0x0D:
		data->state.WriteData = d;
		data->state.C0XD = 1;
		break;
	default:
		fdd_io_read(a, data);
		break;
	}
}


static void fdd_code_fm(byte b, byte*r)
{
	r[0] = (b >> 1) | 0xAA;
	r[1] = b | 0xAA;
}

static void fdd_decode_fm(const byte*r, byte*b)
{
	*b = (r[1] & 0x55) | ((r[0] &0x55) << 1);
}

static byte fdd_check_sum(const byte*b, int n)
{
	register byte r = 0;
	for (;n; n--, b++) r^=*b;
	return r;
}

static int next_sector(struct FDD_DATA*data, int ofs, int *nrot)
{
	struct FDD_DRIVE_DATA *d = data->drives+data->drv;
	static const byte hdr[] = {0xD5, 0xAA, 0x96};
	int n = 0, i, off0 = -1, i0;
	for (i = 0; i < d->TrackLen; ++i, ++ofs) {
		if (ofs == d->TrackLen) { ofs = 0; ++*nrot; }
		if (d->TrackData[ofs] != hdr[n]) {
			if (n) { ofs = off0; i = i0; }
			n = 0;
		} else {
			if (!n) { off0 = ofs; i0 = i; }
			++n;
			if (n == sizeof(hdr)) return off0;
		}
	}
	return -1;
}

static int next_data(struct FDD_DATA*data, int maxl, int ofs, int *nrot)
{
	struct FDD_DRIVE_DATA *d = data->drives+data->drv;
	static const byte hdr[] = {0xD5, 0xAA, 0xAD};
	int n = 0, i, off0 = -1, i0;
	for (i = 0; i < d->TrackLen; ++i, ++ofs) {
		if (ofs == d->TrackLen) { ofs = 0; ++*nrot; }
		if (d->TrackData[ofs] != hdr[n]) {
			if (n) { ofs = off0; i = i0; }
			n = 0;
		} else {
			if (!n) { off0 = ofs; i0 = i; }
			++n;
			if (n == sizeof(hdr)) return off0;
		}
	}
	return -1;
}


static int decode_ts(const byte*data, byte vtscs[4])
{
	int i;
	for (i = 0; i < 4; ++i, data += 2) {
		fdd_decode_fm(data, vtscs + i);
	}
	if (fdd_check_sum(vtscs, 3) != vtscs[3]) return -1;
	return 0;
}

static int fdd_save_track_nibble(struct FDD_DATA*data)
{
	struct FDD_DRIVE_DATA *d = data->drives+data->drv;
	if (!d->disk) return -1;
	osseek(d->disk, d->start_ofs + d->Track*NIBBLE_TRACK_LEN, SSEEK_SET);
	if (oswrite(d->disk, d->TrackData, NIBBLE_TRACK_LEN) != NIBBLE_TRACK_LEN) {
		errprint(TEXT("can't write track"));
		d->error=1;
		return -1;
	}
	return 0;
}

static const byte CodeTabl[0x40]={
	0x96,0x97,0x9A,0x9B,0x9D,0x9E,0x9F,0xA6,0xA7,0xAB,0xAC,0xAD,0xAE,0xAF,0xB2,0xB3,
	0xB4,0xB5,0xB6,0xB7,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,0xCB,0xCD,0xCE,0xCF,0xD3,
	0xD6,0xD7,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,0xE5,0xE6,0xE7,0xE9,0xEA,0xEB,0xEC,
	0xED,0xEE,0xEF,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF
};

static const byte DecodeTabl[0x80]={
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x02,0x03,0x00,0x04,0x05,0x06,
	0x00,0x00,0x00,0x00,0x00,0x00,0x07,0x08,0x00,0x00,0x00,0x09,0x0A,0x0B,0x0C,0x0D,
	0x00,0x00,0x0E,0x0F,0x10,0x11,0x12,0x13,0x00,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1B,0x00,0x1C,0x1D,0x1E,
	0x00,0x00,0x00,0x1F,0x00,0x00,0x20,0x21,0x00,0x22,0x23,0x24,0x25,0x26,0x27,0x28,
	0x00,0x00,0x00,0x00,0x00,0x29,0x2A,0x2B,0x00,0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,
	0x00,0x00,0x33,0x34,0x35,0x36,0x37,0x38,0x00,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F
};

static const byte ren[]={ // dos sector -> file order
	0x00,0x07,0x0E,0x06,0x0D,0x05,0x0C,0x04,
	0x0B,0x03,0x0A,0x02,0x09,0x01,0x08,0x0F
};

static const byte ren1[]={ // file order -> dos sector
	0x00,0x0D,0x0B,0x09,0x07,0x05,0x03,0x01,
	0x0E,0x0C,0x0A,0x08,0x06,0x04,0x02,0x0F
};


static const byte pren[]={// dos sector -> file order
	0x00,0x08,0x01,0x09,0x02,0x0A,0x03,0x0B,
	0x04,0x0C,0x05,0x0D,0x06,0x0E,0x07,0x0F
};

static const byte pren1[]={
	0x00,0x02,0x04,0x06,0x08,0x0A,0x0C,0x0E,
	0x01,0x03,0x05,0x07,0x09,0x0B,0x0D,0x0F
};


static void fdd_code_mfm(const byte*b, byte*d)
{

	memset(d, 0, 0x157);

	__asm {
		pushad
		mov	esi, b
		mov	edi, d
		mov	ebx, 2h
	l2:	mov	ecx, 55h
	l1:	dec	bl
		mov	al, [esi+ebx]
		shr	al, 1
		rcl	byte ptr [edi+ecx], 1
		shr	al, 1
		rcl	byte ptr [edi+ecx], 1
		mov	byte ptr [edi+56h+ebx], al
		dec	ecx
		jns	l1
		or	ebx, ebx
		jne	l2


		xor	al, al
		xor	ecx, ecx
		xor	ebx, ebx
	l4:	mov	ah, [edi+ecx]
		mov	bl, ah
		xor	bl, al
		mov	al, ah
		mov	bl, CodeTabl[ebx]
		mov	[edi+ecx], bl
		inc	ecx
		cmp	ecx, 156h
		jne	l4
		mov	bl, al
		mov	bl, CodeTabl[ebx]
		mov	[edi+ecx], bl
		popad
	}
}


static int fdd_decode_mfm(byte*s, byte*d)
{
	int res = 0;
	memset(d, 0, 0x100);
	__asm {
		pushad
		mov	esi, s
		mov	edi, d
		xor	eax, eax
		xor	ebx, ebx
		xor	ecx, ecx
l1:		mov	bl, [esi + ecx]
		and	bl, 7Fh
		mov	bl, DecodeTabl[ebx]
		mov	ah, bl
		xor	ah, al
		mov	[esi + ecx], ah
		mov	al, ah
		inc	ecx
		cmp	ecx, 156h
		jne	l1
		mov	bl, al
		mov	bl, CodeTabl[ebx]
		cmp	[esi + ecx], bl
		je	cs_ok
		mov	res, -1
		jmp	fin

cs_ok:		xor	ebx, ebx
l2:		xor	ecx, ecx
l3:		mov	al, [esi + 56h + ebx]
		shr	byte ptr [esi + ecx], 1
		rcl	al, 1
		shr	byte ptr [esi + ecx], 1
		rcl	al, 1
		mov	[edi + ebx], al
		inc	bl
		jz	fin
		inc	cl
		cmp	cl, 56h
		jne	l3
		jmp	short l2
fin:
		popad
	}
	return res;
}


static int copy_sect(struct FDD_DRIVE_DATA *d, byte*buf, int ofs, int len, int *nrot)
{
	for (;len; --len, ++ofs, ++buf) {
		if (ofs == d->TrackLen) { ofs = 0; ++*nrot; }
		*buf = d->TrackData[ofs];
	}
	return ofs;
}

static void dump_buf(byte*data, int len)
{
	char buf[1024];
	char *p;
	const unsigned char *d = data;
	int n, ofs = 0;
	printf("dump(%i):\n",len);
	while (len) {
		const unsigned char *d1 = d;
		int m, m1;
		p = buf;
		n = 16;
		m = n;
		for (; n && len; --n, --len, ++d) {
			p += sprintf(p, "%02X ", *d);
		}
		m1 = n;
		for (; n; --n) {
			p += sprintf(p, "__ ");
		}
		for (; m > m1; --m, ++d1) {
			int c = *d1;
			if (c <' ') c = '.';
			p += sprintf(p, "%c", c);
		}
		for (; m; --m) {
			p += sprintf(p, " ");
		}
		*p = 0;
		printf("%04x: %s\n", ofs, buf);
		ofs += 16;
	}
}

static void fdd_save_track(struct FDD_DATA*data)
{
	struct FDD_DRIVE_DATA *d = data->drives+data->drv;
	int ofs, nrot = 0;
//	puts("save_track");
	if (!d->disk) return;
	if (d->readonly) return;
	if (d->rawfmt) {
		fdd_save_track_nibble(data);
		return;
	}
/*	{
		char buf[128];
		FILE*out;
		sprintf(buf,"track%02i.bin", d->Track);
		out = fopen(buf, "wb");
		fwrite(d->TrackData, 1, sizeof(d->TrackData), out);
		fclose(out);
	}*/
	for (ofs = next_sector(data, 0, &nrot); ofs != -1 && nrot < 2; ofs = next_sector(data, ofs + 1, &nrot)) {
		byte vtscs[4];
		byte shdr[14];
		byte sbuf[343+6], buf[FDD_SECTOR_DATA_SIZE];
		int ofs1, r;
//		printf("ofs = %i\n", ofs);
		ofs = copy_sect(d, shdr, ofs, 14, &nrot);
		if (decode_ts(shdr + 3, vtscs)) {
			printf("invalid VTS checksum!\n");
			continue;
		}
		if (shdr[11] != 0xDE || shdr[12] != 0xAA) {
			printf("invalid VTS tail!\n");
			continue;
		}
//		printf("vts: %i %i %i\n", vtscs[0], vtscs[1], vtscs[2]);
		ofs1 = next_data(data, 20, ofs, &nrot);
		if (ofs1 == -1) {
			printf("no sector data prefix!\n");
			continue;
		}
		ofs = ofs1;
		ofs = copy_sect(d, sbuf, ofs, 343+6, &nrot);
		if (sbuf[0] != 0xD5 || sbuf[1] != 0xAA || sbuf[2] != 0xAD) {
			printf("invalid sector data prefix!\n");
			continue;
		}
		if (sbuf[343+3] != 0xDE || sbuf[343+4] != 0xAA) {
			printf("invalid sector data tail!\n");
			continue;
		}
//		dump_buf(sbuf + 3, 0x157);
		r = fdd_decode_mfm(sbuf + 3, buf);
		if (r) {
			printf("decode_mfm = %i\n", r);
			continue;
		}
//		dump_buf(sbuf + 3, 0x157);
//		dump_buf(buf, 256);
//		printf("buf: %x %x %x %x %x (%s)\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf);
		osseek(d->disk, d->start_ofs + (d->Track*FDD_SECTOR_COUNT + (d->prodos?pren[vtscs[2]]:ren[vtscs[2]])) *
			FDD_SECTOR_DATA_SIZE, SSEEK_SET);
		if (oswrite(d->disk, buf, FDD_SECTOR_DATA_SIZE)!=FDD_SECTOR_DATA_SIZE) {
			errprint(TEXT("can't write sector"));
			d->error=1;
		}
	}
	osseek(d->disk, 0, SSEEK_CUR);
}



static void sound_phase()
{
	PlaySound(NULL, NULL, SND_NODEFAULT);
	PlaySound("shugart.wav", GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC | SND_NOWAIT);
}


static void fdd_load_track(struct FDD_DATA*data)
{
	struct FDD_DRIVE_DATA *drv = data->drives+data->drv;
	int i, j, d, dl;
	byte a2[2];
	byte buf[257];

	if (!drv->disk) {
		drv->error=1;
		return;
	}	

	if (drv->Track>34) drv->Track=34;
	if (drv->rawfmt) {
//		puts("fdd_load_track: nibble");
		isseek(drv->disk, drv->start_ofs + drv->Track*
			NIBBLE_TRACK_LEN, SSEEK_SET);
		if (isread(drv->disk,drv->TrackData,NIBBLE_TRACK_LEN)!=NIBBLE_TRACK_LEN) {
			errprint(TEXT("can't read track"));
			drv->error=1;
		}
		drv->TrackIndex = 0;
		drv->TrackLen = NIBBLE_TRACK_LEN;
		return;
	}
//	puts("fdd_load_track");
	isseek(drv->disk, drv->start_ofs + drv->Track*
		FDD_SECTOR_DATA_SIZE*FDD_SECTOR_COUNT, SSEEK_SET);
	memset(drv->TrackData,0xFF,128);
	d = 128;
	for (i=0; i<FDD_SECTOR_COUNT; i++) {
		dl = d;
		drv->TrackData[dl] = 0xD5; dl++;
		drv->TrackData[dl] = 0xAA; dl++;
		drv->TrackData[dl] = 0x96; dl++;
		buf[0] = drv->volume;
		buf[1] = drv->Track;
		buf[2] = drv->prodos?pren1[i]:ren1[i];
		buf[3] = fdd_check_sum(buf,3);
		for (j=0; j<4; j++) {
			fdd_code_fm(buf[j], a2);
			drv->TrackData[dl] = a2[0];
			drv->TrackData[dl+1] = a2[1];
			dl+=2;
		}
		drv->TrackData[d+11] = 0xDE;
		drv->TrackData[d+12] = 0xAA;
		drv->TrackData[d+13] = 0xEB;
		memset(drv->TrackData+d+14,0xFF,5);
		drv->TrackData[d+19] = 0xD5;
		drv->TrackData[d+20] = 0xAA;
		drv->TrackData[d+21] = 0xAD;
		dl = d + 22;
		if (isread(drv->disk,buf,FDD_SECTOR_DATA_SIZE)!=FDD_SECTOR_DATA_SIZE) {
			errprint(TEXT("can't read sector"));
			drv->error=1;
		}
		fdd_code_mfm(buf,drv->TrackData+d+22);
		drv->TrackData[d+365] = 0xDE;
		drv->TrackData[d+366] = 0xAA;
		drv->TrackData[d+367] = 0xEB;
		memset(drv->TrackData+d+368,0xFF,40);
		d+=408;
	}
/*	{
		char buf[128];
		FILE*out;
		sprintf(buf,"track%02i.bin", drv->Track);
		out = fopen(buf, "wb");
		fwrite(drv->TrackData, 1, sizeof(drv->TrackData), out);
		fclose(out);
	}*/
	drv->TrackIndex = 0;
	drv->TrackLen = sizeof(drv->TrackData);
}


static void fdd_select_phase(struct FDD_DATA*data, int p, int en)
{
	struct FDD_DRIVE_DATA *d = data->drives+data->drv;
//	printf("fdd1: %s phase %i, cur_phase %i, phase %i, track %i\n", en?"enable":"disable", p, data->state.CurrentPhase, data->drives[data->drv].Phase, data->drives[data->drv].Track);
	if (!d->present) return;
	if (!en) return;
	if (data->state.CurrentPhase==p) return;
	if (((d->Phase+1)&3)==p) {
		if (d->Phase&1) {
			if (data->st->sr->gconfig->flags & EMUL_FLAGS_SHUGART_SOUNDS)
				sound_phase();
		}
		if (d->Phase<110) {
			d->Phase++;
			if (d->Track!=d->Phase/2) {
        			d->Track=d->Phase/2;
//        			printf("fdd1: track = %i\n",d->Track);
				fdd_load_track(data);
			}	
		}
	}
	if (((d->Phase-1)&3)==p) {
		if (d->Phase&1) {
			if (data->st->sr->gconfig->flags & EMUL_FLAGS_SHUGART_SOUNDS)
				sound_phase();
		}
		if (d->Phase>0) {
			d->Phase--;
			if (d->Track!=d->Phase/2) {
        			d->Track=d->Phase/2;
//        			printf("fdd1: track = %i\n",d->Track);
				fdd_load_track(data);
			}	
		}
	}
	data->state.CurrentPhase = p;
}

static void fdd_begin_write(struct FDD_DATA*data)
{
	struct FDD_DRIVE_DATA *d = data->drives+data->drv;
	data->time = 1;
	data->state.ReadMode = 0;
	data->last_tsc = 0;
	fdd_rot(d);
}

byte fdd_io_read(unsigned short a,struct FDD_DATA*data)
{
	byte r = 0xFF;
	a&=0x0F;
	if (!data->initialized) return 0;
	switch (a) {
	case 0: case 2: case 4: case 6:
	case 1: case 3: case 5: case 7:
		fdd_select_phase(data, a >> 1, a & 1);
		break;
	case 8:
		if (data->use_fast) system_command(data->sr, SYS_COMMAND_FAST, 0, 0);
		data->state.MotorOn = 0;
//		printf("fdd1: motor off\n");
		break;
	case 9:
		if (data->use_fast) system_command(data->sr, SYS_COMMAND_FAST, 1, 0);
		data->state.MotorOn = 1;
//		printf("fdd1: motor on\n");
		break;
	case 10: case 11:
		data->ntries = 0;
		data->drv = a - 10;
//		printf("fdd1: select drive = %i\n", data->drv);
		if (data->drives[data->drv].dirty) {
			fdd_load_track(data);
		}
		break;
	case 12:
		if (data->state.ReadMode) {
			r = fdd_read_data(data);
//			printf("read_data: %X\n", r);
		} else {
			fdd_write_data(data);
		}
		break;
	case 13:
		data->state.C0XD = 1;
		return r;
	case 14:
		if (!data->drives[data->drv].present) {
			r = 0xFF;
			break;
		}
		if (!data->state.ReadMode) {
			data->state.ReadMode = 1;
			fdd_save_track(data);
		}
		if (data->state.C0XD) {
			r = data->drives[data->drv].readonly?0x80:0x00;
		}
		break;
	case 15:
		fdd_begin_write(data);
		break;
	}
	data->state.C0XD = 0;
//	printf("fdd1 read[%x]=%x\n", a, r);
//	system_command(data->st->sr, SYS_COMMAND_DUMPCPUREGS, 0, 0);
	return r;
}

byte fdd_rom_read(unsigned short a, struct FDD_DATA*xd)
{
	return xd->fdd_rom[a & (FDD_ROM_SIZE - 1)];
}

