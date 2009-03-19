#include <windows.h>
#include <string.h>
#include <tchar.h>
#include "resource.h"
#include "sysconf.h"

#include "localize.h"

/*
const TCHAR*memsizes_s[NMEMSIZES] = {
	TEXT("———"), // 1
	TEXT("4 Кбайта"), // 2
	TEXT("8 Кбайт"), // 4
	TEXT("16 Кбайт"), // 8
	TEXT("32 Кбайта"), // 16
	TEXT("48 Кбайт"), // 32
	TEXT("64 Кбайта"), // 64
	TEXT("128 Кбайт"), // 128
	TEXT("12 Кбайт"),  //256
	TEXT("20 Кбайт"),  //512
	TEXT("24 Кбайта"), //1024
	TEXT("36 Кбайт"),  //2048
};
*/

const unsigned memsizes_b[NMEMSIZES] = {
	0,
	4 * 1024,
	8 * 1024,
	16 * 1024,
	32 * 1024,
	48 * 1024,
	64 * 1024,
	128 * 1024,
	12 * 1024,
	20 * 1024,
	24 * 1024,
	36 * 1024
};

/*
const TCHAR*devnames[] = {
	TEXT("———"),
	TEXT("Псевдо-ПЗУ"),
	TEXT("Доп. ОЗУ"),
	TEXT("Доп. ОЗУ"),
	TEXT("Language card"),
	TEXT("Контроллер Teac"),
	TEXT("Контроллер Shugart"),

	TEXT("— Система —"),
	TEXT("Отсутствует"),
	TEXT("Мышь"),
	TEXT("Джойстик"),
	TEXT("MMSYSTEM"),
	TEXT("DirectSound"),
	TEXT("Системный звук"),
	TEXT("Цветной"),
	TEXT("Монохромный"),
	TEXT("ЦП 6502"),
	TEXT("ЦП M6502"),
	TEXT("Отсутствует"),
	TEXT("Внешний файл"),
	TEXT("Без звука")
};

const TCHAR*confnames[] = {
	NULL,
	NULL,
	TEXT("Слот №2"),
	TEXT("Слот №3"),
	TEXT("Слот №4"),
	TEXT("Слот №5"),
	TEXT("Слот №6"),
	NULL,
	NULL,
	NULL,
	TEXT("(Процессор)"),
	TEXT("(Память)"),
	TEXT("(ПЗУ)"),
	TEXT("(Набор символов)"),
	TEXT("(Звук)"),
	TEXT("(Джойстик)"), 
	TEXT("(Монитор)"),
	TEXT("(Магнитофон)"),
};

const TCHAR*sysnames[] = {
	TEXT("Агат–7"),
	TEXT("Агат–9"),
	TEXT("Apple ][")
};

const TCHAR*drvtypenames[] = {
	TEXT("нет"),
	TEXT("ЕС5088"),
	TEXT("1S1D"),
	TEXT("1S2D"),
	TEXT("2S1D"),
	TEXT("2S2D"),
};
*/


int clear_config(struct SYSCONFIG*c)
{
	ZeroMemory(c, sizeof(*c));
	return 0;
}

int reset_config(struct SYSCONFIG*c, int systype)
{
	int i;
	if (systype == -1) systype = c->systype;

	memset(c, 0, sizeof(*c));
	c->systype = systype;
	for (i = 0; i < NCONFTYPES; i++) {
		c->slots[i].slot_no = i;
	}

	switch (systype) {
	case SYSTEM_7:
		reset_slot_config(c->slots+CONF_SLOT2, DEV_MEMORY_PSROM7, systype);
		reset_slot_config(c->slots+CONF_SLOT3, DEV_FDD_TEAC, systype);
		reset_slot_config(c->slots+CONF_SLOT4, DEV_MEMORY_XRAM7, systype);
		reset_slot_config(c->slots+CONF_SLOT6, DEV_FDD_SHUGART, systype);
		break;
	case SYSTEM_9:
		reset_slot_config(c->slots+CONF_SLOT2, DEV_MEMORY_XRAM9, systype);
		reset_slot_config(c->slots+CONF_SLOT5, DEV_FDD_TEAC, systype);
		reset_slot_config(c->slots+CONF_SLOT6, DEV_FDD_SHUGART, systype);
		break;
	case SYSTEM_A:
		reset_slot_config(c->slots+CONF_SLOT2, DEV_MEMORY_XRAMA, systype);
		reset_slot_config(c->slots+CONF_SLOT6, DEV_FDD_SHUGART, systype);
		break;
	}
	for (i = CONF_EXT; i < NCONFTYPES; i++) {
		reset_slot_config(c->slots+i, DEV_SYSTEM, systype);
	}

	reset_sysicon(c, systype);
	return 0;
}

int reset_sysicon(struct SYSCONFIG*c, int systype)
{
	HBITMAP bmp;
	int codes[]={IDB_AGAT7_LOGO, IDB_AGAT9_LOGO, IDB_APPLE2_LOGO};
	HDC dc;
	int r;
	bmp = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(codes[systype]));
	if (!bmp) return -1;
	free_sysicon(&c->icon);
	dc = CreateCompatibleDC(NULL);
	r = bitmap_to_sysicon(dc, bmp, &c->icon);
	DeleteDC(dc);
	DeleteObject(bmp);
	return r;
}

int free_config(struct SYSCONFIG*c)
{
	free_sysicon(&c->icon);
	return 0;
}

int reset_slot_config(struct SLOTCONFIG*c, int devtype, int systype)
{
	c->dev_type = devtype;
	switch (c->dev_type) {
	case DEV_MEMORY_PSROM7:
		c->cfgint[CFG_INT_MEM_SIZE] = 4;
		c->cfgint[CFG_INT_MEM_MASK] = 8 | 16 | 32 | 64 | 128;
		return 0;
	case DEV_MEMORY_XRAM7:
		c->cfgint[CFG_INT_MEM_SIZE] = 4;
		c->cfgint[CFG_INT_MEM_MASK] = 8 | 16 | 32 | 64 | 128;
		return 0;
	case DEV_MEMORY_XRAM9:
		c->cfgint[CFG_INT_MEM_SIZE] = 7;
		c->cfgint[CFG_INT_MEM_MASK] = 128;
		return 0;
	case DEV_MEMORY_XRAMA:
		c->cfgint[CFG_INT_MEM_SIZE] = 3;
		c->cfgint[CFG_INT_MEM_MASK] = 8;
		return 0;
	case DEV_FDD_TEAC:
		c->cfgint[CFG_INT_DRV_TYPE1] = DRV_TYPE_2S2D;
		c->cfgint[CFG_INT_DRV_TYPE2] = DRV_TYPE_NONE;
		c->cfgint[CFG_INT_DRV_COUNT] = 1;
		c->cfgint[CFG_INT_DRV_RO_FLAGS] = 3;
		_tcscpy(c->cfgstr[CFG_STR_DRV_ROM], TEXT("ROMS\\TEAC.ROM"));
		c->cfgint[CFG_INT_DRV_ROM_RES] = 21;
		_tcscpy(c->cfgstr[CFG_STR_DRV_IMAGE1], TEXT("1.DSK"));
		_tcscpy(c->cfgstr[CFG_STR_DRV_IMAGE2], TEXT("2.DSK"));
		return 0;
	case DEV_FDD_SHUGART:
		c->cfgint[CFG_INT_DRV_TYPE1] = DRV_TYPE_SHUGART;
		c->cfgint[CFG_INT_DRV_TYPE2] = DRV_TYPE_NONE;
		c->cfgint[CFG_INT_DRV_COUNT] = 1;
		c->cfgint[CFG_INT_DRV_RO_FLAGS] = 3;
		switch (systype) {
		case SYSTEM_7:
			_tcscpy(c->cfgstr[CFG_STR_DRV_ROM], TEXT("ROMS\\SHUGART7.ROM"));
			c->cfgint[CFG_INT_DRV_ROM_RES] = 22;
			break;
		case SYSTEM_9:
			_tcscpy(c->cfgstr[CFG_STR_DRV_ROM], TEXT("ROMS\\SHUGART9.ROM"));
			c->cfgint[CFG_INT_DRV_ROM_RES] = 23;
			break;
		case SYSTEM_A:
			_tcscpy(c->cfgstr[CFG_STR_DRV_ROM], TEXT("ROMS\\SHUGARTA.ROM"));
			c->cfgint[CFG_INT_DRV_ROM_RES] = 24;
			break;
		}
		_tcscpy(c->cfgstr[CFG_STR_DRV_IMAGE1], TEXT("3.DSK"));
		_tcscpy(c->cfgstr[CFG_STR_DRV_IMAGE2], TEXT("4.DSK"));
		return 0;
	case DEV_VIDEOTERM:
		_tcscpy(c->cfgstr[CFG_STR_ROM], TEXT("ROMS\\VIDEOTERM.ROM"));
		_tcscpy(c->cfgstr[CFG_STR_ROM2], TEXT("FNTS\\VIDEOTERM_NORM.FNT"));
		_tcscpy(c->cfgstr[CFG_STR_ROM3], TEXT("FNTS\\VIDEOTERM_INV.FNT"));
		c->cfgint[CFG_INT_ROM_RES] = 200;
		return 0;
	case DEV_SYSTEM:
		switch (c->slot_no) {
		case CONF_MEMORY:
			switch (systype) {
			case SYSTEM_7:
				c->cfgint[CFG_INT_MEM_SIZE] = 4;
				c->cfgint[CFG_INT_MEM_MASK] = 16 | 64 | 128;
				return 0;
			case SYSTEM_9:
				c->cfgint[CFG_INT_MEM_SIZE] = 7;
				c->cfgint[CFG_INT_MEM_MASK] = 128;
				return 0;
			case SYSTEM_A:
				c->cfgint[CFG_INT_MEM_SIZE] = 5;
				c->cfgint[CFG_INT_MEM_MASK] = 2 | 4 | 8 | 16 | 32 | 256 | 512 | 1024 | 2048;
				return 0;
			}
			break;
		case CONF_MONITOR:
			switch (systype) {
			case SYSTEM_7:
			case SYSTEM_9:
				c->dev_type = DEV_NULL;
				return 0;
			case SYSTEM_A:
				c->dev_type = DEV_COLOR;
				return 0;
			}
			break;
		case CONF_CPU:
			c->dev_type = DEV_6502;
			c->cfgint[CFG_INT_CPU_SPEED] = 100;
			c->cfgint[CFG_INT_CPU_EXT] = 1;
			return 0;
		case CONF_SOUND:
			c->dev_type = DEV_MMSYSTEM;
			c->cfgint[CFG_INT_SOUND_FREQ] = 22050;
			c->cfgint[CFG_INT_SOUND_BUFSIZE] = 8192;
			return 0;
		case CONF_TAPE:
			switch (systype) {
			case SYSTEM_7:
			case SYSTEM_A:
				c->dev_type = DEV_TAPE_FILE;
				break;
			case SYSTEM_9:
				c->dev_type = DEV_TAPE_NONE;
				break;
			}
			_tcscpy(c->cfgstr[CFG_STR_TAPE], TEXT("tape.wav"));
			c->cfgint[CFG_INT_TAPE_FREQ] = 8000;
			return 0;
		case CONF_JOYSTICK:
			c->dev_type = DEV_MOUSE;
			return 0;
		case CONF_ROM:
			switch (systype) {
			case SYSTEM_7:
				_tcscpy(c->cfgstr[CFG_STR_ROM], TEXT("ROMS\\MONITOR7.ROM"));
				c->cfgint[CFG_INT_ROM_RES] = 11;
				c->cfgint[CFG_INT_ROM_SIZE] = 0x800;
				c->cfgint[CFG_INT_ROM_MASK] = 0x7FF;
				c->cfgint[CFG_INT_ROM_OFS] = 0;
				return 0;
			case SYSTEM_9:
				_tcscpy(c->cfgstr[CFG_STR_ROM], TEXT("ROMS\\MONITOR9.ROM"));
				c->cfgint[CFG_INT_ROM_RES] = 12;
				c->cfgint[CFG_INT_ROM_SIZE] = 0x800;
				c->cfgint[CFG_INT_ROM_MASK] = 0x7FF;
				c->cfgint[CFG_INT_ROM_OFS] = 0;
				return 0;
			case SYSTEM_A:
				_tcscpy(c->cfgstr[CFG_STR_ROM], TEXT("ROMS\\APPLE2.ROM"));
				c->cfgint[CFG_INT_ROM_RES] = 13;
				c->cfgint[CFG_INT_ROM_SIZE] = 0x3000;
				c->cfgint[CFG_INT_ROM_MASK] = 0x3FFF;
				c->cfgint[CFG_INT_ROM_OFS] = 0x1000;
				return 0;
			}
			break;
		case CONF_CHARSET:
			c->cfgint[CFG_INT_ROM_SIZE] = 0x800;
			switch (systype) {
			case SYSTEM_7:
				_tcscpy(c->cfgstr[CFG_STR_ROM], TEXT("FNTS\\AGATHE7.FNT"));
				c->cfgint[CFG_INT_ROM_RES] = 1;
				c->cfgint[CFG_INT_ROM_SIZE] = 0x800;
				return 0;
			case SYSTEM_9:
				_tcscpy(c->cfgstr[CFG_STR_ROM], TEXT("FNTS\\AGATHE9.FNT"));
				c->cfgint[CFG_INT_ROM_RES] = 2;
				return 0;
			case SYSTEM_A:
				_tcscpy(c->cfgstr[CFG_STR_ROM], TEXT("FNTS\\APPLESM.FNT"));
				c->cfgint[CFG_INT_ROM_RES] = 3;
				return 0;
			}
			break;
		}
	}
	return 0;
}

int get_slot_comment(struct SLOTCONFIG*c, TCHAR*buf)
{
	TCHAR lbuf[256];
	buf[0] = 0;

	switch (c->slot_no) {
	case CONF_SOUND:
		if (c->dev_type == DEV_MMSYSTEM || c->dev_type == DEV_DSOUND) {
			wsprintf(buf, 
				localize_str(LOC_SYSCONF, 500, lbuf, sizeof(lbuf)), //TEXT("%i Гц; %i сэмплов"), 
				c->cfgint[CFG_INT_SOUND_FREQ], c->cfgint[CFG_INT_SOUND_BUFSIZE]);
		}
		return 0;
	}

	switch (c->dev_type) {
	case DEV_MEMORY_PSROM7:
	case DEV_MEMORY_XRAM7:
	case DEV_MEMORY_XRAM9:
	case DEV_MEMORY_XRAMA:
		_tcscpy(buf, get_memsizes_s(c->cfgint[CFG_INT_MEM_SIZE]));
		return 0;
	case DEV_6502:
	case DEV_M6502:
		wsprintf(buf, 
			localize_str(LOC_SYSCONF, 501, lbuf, sizeof(lbuf)), //TEXT("Частота %i%%"), 
			c->cfgint[CFG_INT_CPU_SPEED]);
		return 0;
	case DEV_TAPE_FILE:
		if (c->cfgstr[CFG_STR_TAPE][0])
			_tcscpy(buf, c->cfgstr[CFG_STR_TAPE]);
		else
			localize_str(LOC_SYSCONF, 502, buf, sizeof(buf));
		wsprintf(buf + _tcslen(buf), 
			localize_str(LOC_SYSCONF, 503, lbuf, sizeof(lbuf)), //TEXT(", %i Гц"), 
			c->cfgint[CFG_INT_TAPE_FREQ]);
		return 0;
	case DEV_SYSTEM:
		switch (c->slot_no) {
		case CONF_MEMORY:
			_tcscpy(buf, get_memsizes_s(c->cfgint[CFG_INT_MEM_SIZE]));
			return 0;
		case CONF_ROM:
			_tcscpy(buf, c->cfgstr[CFG_STR_ROM]);
			return 0;
		case CONF_CHARSET:
			_tcscpy(buf, c->cfgstr[CFG_STR_ROM]);
			return 0;
		}
		break;
	case DEV_VIDEOTERM:
		_tcscpy(buf, c->cfgstr[CFG_STR_ROM]);
		_tcscat(buf, TEXT("; "));
		_tcscat(buf, c->cfgstr[CFG_STR_ROM2]);
		if (c->cfgstr[CFG_STR_ROM3]) {
			_tcscat(buf, TEXT("; "));
			_tcscat(buf, c->cfgstr[CFG_STR_ROM3]);
		}
		return 0;
	case DEV_FDD_TEAC:
	case DEV_FDD_SHUGART:
		switch (c->cfgint[CFG_INT_DRV_COUNT]) {
		case 0:
			localize_str(LOC_SYSCONF, 510, buf, sizeof(buf));
//			_tcscpy(buf, TEXT("Устройства отсутствуют"));
			break;
		case 1:
			if (c->cfgint[CFG_INT_DRV_TYPE1]!=DRV_TYPE_NONE) 
				wsprintf(buf, 
					localize_str(LOC_SYSCONF, 511, lbuf, sizeof(lbuf)), //TEXT("%s: %s"), 
					get_drvtypenames(c->cfgint[CFG_INT_DRV_TYPE1]),
					c->cfgstr[CFG_STR_DRV_IMAGE1]);
			else
				wsprintf(buf,
					localize_str(LOC_SYSCONF, 512, lbuf, sizeof(lbuf)), //TEXT("нет; %s: %s"), 
					get_drvtypenames(c->cfgint[CFG_INT_DRV_TYPE2]),
					c->cfgstr[CFG_STR_DRV_IMAGE2]);
			break;
		case 2:
			wsprintf(buf,
				localize_str(LOC_SYSCONF, 513, lbuf, sizeof(lbuf)), //TEXT("%s: %s; %s: %s"),
				get_drvtypenames(c->cfgint[CFG_INT_DRV_TYPE1]),
				c->cfgstr[CFG_STR_DRV_IMAGE1],
				get_drvtypenames(c->cfgint[CFG_INT_DRV_TYPE2]),
				c->cfgstr[CFG_STR_DRV_IMAGE2]);
			break;
		}
		return 0;
	}
	return 0;
}

int save_config(struct SYSCONFIG*c, OSTREAM*out)
{
	int r;
	DWORD h = MAKELONG(sizeof(*c), MAKEWORD(0,8));
	r = oswrite(out, &h, sizeof(h));
	if (r != sizeof(h)) return -1;
	r = oswrite(out, c, sizeof(*c));
	if (r!=sizeof(*c)) return -1;
	if (c->icon.image && c->icon.imglen) {
		r = oswrite(out, c->icon.image, c->icon.imglen);
		if (r!=c->icon.imglen) return -1;
	}
	return 0;
}


int load_config(struct SYSCONFIG*c, ISTREAM*in)
{
	int r;
	DWORD h = MAKELONG(sizeof(*c), MAKEWORD(0,8)), h1;
	r = isread(in, &h1, sizeof(h1));
	if (r != sizeof(h1)) return -1;
	if (h != h1) return -2; // invalid version
	r = isread(in, c, sizeof(*c));
	if (r!=sizeof(*c)) return -1;
	if (c->icon.image && c->icon.imglen) {
		c->icon.image = malloc(c->icon.imglen);
		if (!c->icon.image) return -1;
		r = isread(in, c->icon.image, c->icon.imglen);
		if (r!=c->icon.imglen) return -1;
	}
	return 0;
}


HBITMAP sysicon_to_bitmap(HDC dc, struct SYSICON*icon)
{
	BITMAPINFOHEADER hdr;
	BITMAPINFO bi;

	if (!icon->image || !icon->imglen) return NULL;

	ZeroMemory(&hdr, sizeof(hdr));
	hdr.biSize = sizeof(hdr);
	hdr.biWidth = icon->w;
	hdr.biHeight = icon->h;
	hdr.biPlanes = 1;
	hdr.biBitCount = 24;
	hdr.biCompression = BI_RGB;

	bi.bmiHeader = hdr;
	return CreateDIBitmap(dc, &hdr, CBM_INIT, icon->image, &bi, DIB_RGB_COLORS);
}

int bitmap_to_sysicon(HDC dc, HBITMAP hbm, struct SYSICON*icon)
{
	BITMAPINFO bi;
	BITMAP bmp;
	int r;

	r = GetObject(hbm, sizeof(bmp), &bmp);

	if (r!=sizeof(bmp)) return -1;
	icon->w = bmp.bmWidth;
	icon->h = bmp.bmHeight;
	icon->imglen = icon->h * ((icon->w * 3 + 3) &~3);
	icon->image = malloc(icon->imglen);
	if (!icon->image) return -1;

	ZeroMemory(&bi, sizeof(bi));
	bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
	bi.bmiHeader.biWidth = icon->w;
	bi.bmiHeader.biHeight = icon->h;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 24;
	bi.bmiHeader.biCompression = BI_RGB;

	r = GetDIBits(dc, hbm, 0, icon->h, icon->image, &bi, DIB_RGB_COLORS);
	if (!r) {
		free(icon->image);
		icon->image = NULL;
		return -1;
	}

	return 0;
}

int free_sysicon(struct SYSICON*icon)
{
	if (icon->image) {
		free(icon->image);
		icon->image = NULL;
	}
	return 0;
}

LPCTSTR get_memsizes_s(int n)
{
	static TCHAR bufs[NMEMSIZES][256];
	if (n < 0 || n >= NMEMSIZES) return NULL;
	return localize_str(LOC_SYSCONF, n, bufs[n], sizeof(bufs[n]));
}


LPCTSTR get_devnames(int n)
{
	static TCHAR bufs[NDEVTYPES][256];
	if (n < 0 || n >= NDEVTYPES) return NULL;
	return localize_str(LOC_SYSCONF, n + 100, bufs[n], sizeof(bufs[n]));
}



LPCTSTR get_confnames(int n)
{
	static TCHAR bufs[NCONFTYPES][256];
	if (n < 0 || n >= NCONFTYPES) return NULL;
	return localize_str(LOC_SYSCONF, n + 200, bufs[n], sizeof(bufs[n]));
}



LPCTSTR get_sysnames(int n)
{
	static TCHAR bufs[NSYSTYPES][256];
	if (n < 0 || n >= NSYSTYPES) return NULL;
	return localize_str(LOC_SYSCONF, n + 300, bufs[n], sizeof(bufs[n]));
}


LPCTSTR get_drvtypenames(int n)
{
	static TCHAR bufs[DRV_N_TYPES][256];
	if (n < 0 || n >= DRV_N_TYPES) return NULL;
	return localize_str(LOC_SYSCONF, n + 400, bufs[n], sizeof(bufs[n]));
}

