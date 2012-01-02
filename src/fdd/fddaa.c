/*
	Agat Emulator
	Copyright (c) NOP, nnop@newmail.ru
	fddaa - emulation of Acorn atom floppy controller
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

#define FDD_ROM_SIZE 0x1000


struct FDD_DRIVE_DATA
{
	int	error;
	int	readonly;
	int	present;

	HMENU	submenu;

	char_t	disk_name[1024];
	IOSTREAM *disk;
};

struct FDD_DATA
{
	struct FDD_DRIVE_DATA	drives[1];
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
}

static void fill_fdd1(struct FDD_DATA*data)
{
	fill_fdd_drive(data->drives);
	data->initialized=0;
	data->time=0;
	data->type=0;
	data->drv=0;
}

static int open_fdd1(struct FDD_DRIVE_DATA*drv,const char_t*name,int ro, int no)
{
	if (drv->disk) {
		isclose(drv->disk);
		drv->disk = NULL;
	}
	if (!name) return 1;
//	if (access(name,W_OK)) ro=1;
	drv->disk=ro?isfopen(name):iosfopen(name);
	if (!ro&&!drv->disk) {
		ro = 1;
//		puts("set ro to 1");
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
//	printf("ro: %i\n", ro);
	drv->readonly=ro;
//	printf("readonly: %i\n", drv->readonly);
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
		wsprintf(buf, localize_str(LOC_FDD, 4, buf1, sizeof(buf1)), s, d + 1);
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
		break;
	case SYS_COMMAND_INITMENU:
		menu = (HMENU) param;
		init_menu(data->drives + 0, st->sc->slot_no, 0, menu);
		break;
	case SYS_COMMAND_UPDMENU:
		menu = (HMENU) param;
		update_menu(data->drives + 0, st->sc->slot_no, 0, menu);
		break;
	case SYS_COMMAND_FREEMENU:
		menu = (HMENU) param;
		free_menu(data->drives + 0, st->sc->slot_no, 0, menu);
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


int  fddaa_init(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*st, struct SLOTCONFIG*cf)
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
		sprintf(buf, "fdd");
		name = sys_get_parameter(buf);
		if (!name) name = cf->cfgstr[CFG_STR_DRV_IMAGE1];
		strcpy(data->drives[0].disk_name, name);
		open_fdd1(data->drives, name, cf->cfgint[CFG_INT_DRV_RO_FLAGS] & 1, 0);
	}
	data->initialized = 1;

	st->data = data;
	st->free = remove_fdd1;
	st->command = fdd1_command;
	st->save = fdd1_save;
	st->load = fdd1_load;

	return 0;
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



static void sound_phase()
{
	PlaySound(NULL, NULL, SND_NODEFAULT);
	PlaySound("shugart.wav", GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC | SND_NOWAIT);
}





byte fdd_io_read(unsigned short a,struct FDD_DATA*data)
{
	byte r = 0xFF;
	a&=0x0F;
	if (!data->initialized) return 0;
	return r;
}

byte fdd_rom_read(unsigned short a, struct FDD_DATA*xd)
{
	return xd->fdd_rom[a & (FDD_ROM_SIZE - 1)];
}

