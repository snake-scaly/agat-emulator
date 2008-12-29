/*
	Agat Emulator version 1.0
	Copyright (c) NOP, nnop@newmail.ru
	fdd - emulation of TEAC floppy drive
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

#include "syslib.h"
#include "debug.h"


//#include "fdd.h"

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
	byte	TrackData[6671];
	int	TrackIndex, TrackLen;
	char_t	disk_name[1024];
	IOSTREAM *disk;
	int	rawfmt; // 1 if disk in raw (nibble) format
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
		isclose(drv->disk);
		drv->disk=0;
		return -1;
	}
	drv->readonly=ro;
	if (!memcmp(std_sig,drv->disk_header,sizeof(std_sig)-1)) {
		if (drv->disk_header[48]) drv->readonly=1;
		drv->start_ofs=256;
	} else drv->start_ofs=0;
	if (strstr(name,"nib")) {
		puts("using nibble format");
		drv->rawfmt = 1;
	}
	drv->error = 0;
	return 0;
}


static int remove_fdd1(struct SLOT_RUN_STATE*sr)
{
	return 0;
}

#define FDD1_BASE_CMD 7000

static int init_menu(struct FDD_DRIVE_DATA*drv, int s, int d, HMENU menu)
{
	TCHAR buf[1024];
	if (drv->submenu) DeleteMenu(menu, (UINT)drv->submenu, MF_BYCOMMAND);
	if (drv->present) {
		drv->submenu = CreatePopupMenu();
		AppendMenu(drv->submenu, MF_STRING, FDD1_BASE_CMD + (s*4 + d*2 + 0), TEXT("Вставить диск..."));
		AppendMenu(drv->submenu, MF_STRING, FDD1_BASE_CMD + (s*4 + d*2 + 1), TEXT("Извлечь диск"));

		wsprintf(buf, TEXT("Дисковод 140Кб (S%i,D%i)"), s, d + 1);
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
		EnableMenuItem(drv->submenu, FDD1_BASE_CMD + (s*4 + d*2 + 1), drv->disk?MF_ENABLED:MF_GRAYED|MF_BYCOMMAND);
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
	data = calloc(1, sizeof(*data));
	if (!data) return -1;

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
		strcpy(data->drives[0].disk_name, cf->cfgstr[CFG_STR_DRV_IMAGE1]);
		open_fdd1(data->drives, data->drives[0].disk_name, cf->cfgint[CFG_INT_DRV_RO_FLAGS] & 1, 0);
	}
	if (cf->cfgint[CFG_INT_DRV_TYPE2] == DRV_TYPE_SHUGART) {
		data->drives[1].present = 1;
		strcpy(data->drives[1].disk_name, cf->cfgstr[CFG_STR_DRV_IMAGE2]);
		open_fdd1(data->drives+1, data->drives[1].disk_name, cf->cfgint[CFG_INT_DRV_RO_FLAGS] & 2, 1);
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

	return 0;
}


static byte fdd_read_data(struct FDD_DATA*data)
{
	byte r;
	struct FDD_DRIVE_DATA *d = data->drives+data->drv;

	if (!d->disk) { d->error = 1; return rand()%0xFF; }

	if (!(data->time&3)) r = 0;
	else {
		r = d->TrackData[d->TrackIndex];
		d->TrackIndex++;
		if (d->TrackIndex>=d->TrackLen)
			d->TrackIndex = 0;
	}
	data->time++;
	return r;
}

static void fdd_write_data(struct FDD_DATA*data)
{
	struct FDD_DRIVE_DATA *d = data->drives+data->drv;
	d->TrackData[d->TrackIndex] = data->state.WriteData;
	d->TrackIndex++;
	if (d->TrackIndex>=d->TrackLen)
		d->TrackIndex = 0;
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


static void fdd_save_track(struct FDD_DATA*data)
{
	puts("save_track");
}

static byte fdd_check_sum(const byte*b, int n)
{
	register byte r = 0;
	for (;n; n--, b++) r^=*b;
	return r;
}

static void fdd_code_fm(byte b, byte*r)
{
	r[0] = (b >> 1) | 0xAA;
	r[1] = b | 0xAA;
}

static void fdd_decode_fm(const byte*r, byte*b)
{
	*b = (r[0] & 0x55) | ((r[1] &0x55) << 1);
}

static void fdd_code_mfm(const byte*b, byte*d)
{
	static const byte CodeTabl[0x40]={
		0x96,0x97,0x9A,0x9B,0x9D,0x9E,0x9F,0xA6,0xA7,0xAB,0xAC,0xAD,0xAE,0xAF,0xB2,
		  0xB3,0xB4,0xB5,0xB6,0xB7,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,0xCB,
		  0xCD,0xCE,0xCF,0xD3,0xD6,0xD7,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
		  0xE5,0xE6,0xE7,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,0xF2,0xF3,0xF4,
		  0xF5,0xF6,0xF7,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF
	};

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

		mov	ecx, 55h
	l3:	and	byte ptr [edi+ecx], 3fh
		dec	ecx
		jns	l3


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


static void sound_phase()
{
	PlaySound("shugart.wav", GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC | SND_NOWAIT);
//	puts("shugart sound");
}


static void fdd_load_track(struct FDD_DATA*data)
{
	struct FDD_DRIVE_DATA *drv = data->drives+data->drv;
	int i, j, d, dl;
	byte a2[2];
	byte buf[257];
	static const byte ren[]={
		0x00,0x07,0x0E,0x06,0x0D,0x05,0x0C,0x04,
		0x0B,0x03,0x0A,0x02,0x09,0x01,0x08,0x0F
	};
	static const byte ren1[]={
		0x00,0x0D,0x0B,0x09,0x07,0x05,0x03,0x01,
		0x0E,0x0C,0x0A,0x08,0x06,0x04,0x02,0x0F
	};

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
		buf[2] = ren1[i];
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
		dl = d+22;
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


static void fdd_select_phase(struct FDD_DATA*data, int p)
{
	if (data->state.CurrentPhase==p) return;
	if (((data->state.CurrentPhase+1)&3)==p) {
		struct FDD_DRIVE_DATA *d = data->drives+data->drv;
		if (d->Phase<110) {
			d->Phase++;
			if (d->Track!=d->Phase/2) {
        			d->Track=d->Phase/2;
        			printf("fdd1: track = %i\n",d->Track);
				sound_phase();
				fdd_load_track(data);
			}	
		}
	}
	if (((data->state.CurrentPhase-1)&3)==p) {
		struct FDD_DRIVE_DATA *d = data->drives+data->drv;
		if (d->Phase>0) {
			d->Phase--;
			if (d->Track!=d->Phase/2) {
        			d->Track=d->Phase/2;
        			printf("fdd1: track = %i\n",d->Track);
				sound_phase();
				fdd_load_track(data);
			}	
		}
	}
	data->state.CurrentPhase = p;
}

byte fdd_io_read(unsigned short a,struct FDD_DATA*data)
{
	byte r = 0xFF;
	a&=0x0F;
	if (!data->initialized) return 0;
	switch (a) {
	case 1: case 3: case 5: case 7:
		fdd_select_phase(data, a>>1);
		break;
	case 8:
		data->state.MotorOn = 0;
//		printf("fdd1: motor off\n");
		break;
	case 9:
		data->state.MotorOn = 1;
//		printf("fdd1: motor on\n");
		break;
	case 10: case 11:
		data->drv = a - 10;
//		printf("fdd1: select drive = %i\n", data->drv);
		if (data->drives[data->drv].dirty) {
			fdd_load_track(data);
		}
		break;
	case 12:
		if (data->state.ReadMode) {
			r = fdd_read_data(data);
		} else {
			fdd_write_data(data);
		}
		break;
	case 13:
		data->state.C0XD = 1;
		return r;
	case 14:
		if (!data->state.ReadMode) {
			data->state.ReadMode = 1;
			fdd_save_track(data);
		}
		if (data->state.C0XD) {
			r = data->drives[data->drv].readonly?0x80:0x00;
		}
		break;
	case 15:
		data->state.ReadMode = 0;
		break;
	}
	data->state.C0XD = 0;
//	printf("fdd1 read[%x]=%x\n", a, r);
	return r;
}

byte fdd_rom_read(unsigned short a, struct FDD_DATA*xd)
{
	return xd->fdd_rom[a & (FDD_ROM_SIZE - 1)];
}

