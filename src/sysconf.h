#ifndef SYSCONF_H
#define SYSCONF_H

#define SYSTEMS_DIR	"systems"
#define SAVES_DIR 	"saves"
#define PRNOUT_DIR 	"print"
#define HELP_DIR 	"help"
#define TAPES_DIR 	"tapes"

#include <windows.h>
#include <tchar.h>


#include "streams.h"
#include "runmgr.h"

#define DEFSYSICON_W 32
#define DEFSYSICON_H 32

enum {
	SYSTEM_7,
	SYSTEM_9,
	SYSTEM_A,

	NSYSTYPES
};


enum {
	DEV_NULL,
	DEV_MEMORY_PSROM7,
	DEV_MEMORY_XRAM7,
	DEV_MEMORY_XRAM9,
	DEV_MEMORY_XRAMA,
	DEV_FDD_TEAC,
	DEV_FDD_SHUGART,

	DEV_SYSTEM,
	DEV_NOJOY,
	DEV_MOUSE,
	DEV_JOYSTICK,
	DEV_MMSYSTEM,
	DEV_DSOUND,
	DEV_BEEPER,

	DEV_COLOR,
	DEV_MONO,

	DEV_6502,
	DEV_M6502,

	DEV_TAPE_NONE,
	DEV_TAPE_FILE,

	DEV_NOSOUND,

	DEV_SOFTCARD,

	DEV_65C02,
	DEV_VIDEOTERM,
	DEV_THUNDERCLOCK,
	
	DEV_PRINTER9,
	DEV_MOCKINGBOARD,

	DEV_NIPPELCLOCK,
	DEV_PRINTERA,

	DEV_A2RAMCARD,
	DEV_RAMFACTOR,
	DEV_MEMORY_SATURN,

	DEV_MOUSE_PAR,
	DEV_MOUSE_NIPPEL,
	DEV_MOUSE_APPLE,

	NDEVTYPES
};


enum {
	CONF_LIST_START = 0,
	CONF_SLOT0 = CONF_LIST_START,
	CONF_SLOT1,
	CONF_SLOT2,
	CONF_SLOT3,
	CONF_SLOT4,
	CONF_SLOT5,
	CONF_SLOT6,
	CONF_SLOT7,

	CONF_EXT = 10,
	CONF_CPU = 10,
	CONF_MEMORY,
	CONF_ROM,
	CONF_CHARSET,
	CONF_SOUND,
	CONF_JOYSTICK,
	CONF_MONITOR,
	CONF_TAPE,
	CONF_KEYBOARD,
	CONF_PALETTE,

	NCONFTYPES
};


char conf_present[NSYSTYPES][NCONFTYPES];

#define CFGSTRLEN 256
#define CFGNSTRINGS 5
#define CFGNINT 10

struct SLOTCONFIG
{
	int  slot_no;
	int  dev_type;
	char cfgstr[CFGNSTRINGS][CFGSTRLEN];
	int  cfgint[CFGNINT];
};

struct SYSICON
{
	void*image;
	int imglen;
	int w, h;
};

struct SYSCONFIG
{
	int systype;
	struct SLOTCONFIG slots[NCONFTYPES];

	struct SYSICON icon;
};


#define NMEMSIZES 18
LPCTSTR get_memsizes_s(int n);
const unsigned memsizes_b[];

LPCTSTR get_devnames(int n);
LPCTSTR get_confnames(int n);
LPCTSTR get_sysnames(int n);
LPCTSTR get_drvtypenames(int n);

int clear_config(struct SYSCONFIG*c);
int reset_config(struct SYSCONFIG*c, int systype);
int reset_slot_config(struct SLOTCONFIG*c, int devtype, int systype);
int free_config(struct SYSCONFIG*c);


int get_slot_comment(struct SLOTCONFIG*c, TCHAR*buf);

int save_config(struct SYSCONFIG*c, OSTREAM*out);
int load_config(struct SYSCONFIG*c, ISTREAM*in);

HBITMAP sysicon_to_bitmap(HDC dc, struct SYSICON*icon);
int bitmap_to_sysicon(HDC dc, HBITMAP hbm, struct SYSICON*icon);
int free_sysicon(struct SYSICON*icon);

enum {
	CFG_INT_MEM_SIZE,
	CFG_INT_MEM_MASK
};

enum {
	DRV_TYPE_NONE,
	DRV_TYPE_SHUGART,
	DRV_TYPE_1S1D,
	DRV_TYPE_1S2D,
	DRV_TYPE_2S1D,
	DRV_TYPE_2S2D,

	DRV_N_TYPES
};

enum {
	PRINT_RAW,
	PRINT_TEXT,
	PRINT_TIFF,
	PRINT_PRINT
};

enum {
	MOUSE_NONE,
	MOUSE_MM8031,
	MOUSE_MARS
};

enum {
	CFG_INT_DRV_TYPE1,
	CFG_INT_DRV_TYPE2,
	CFG_INT_DRV_COUNT,
	CFG_INT_DRV_RO_FLAGS,
	CFG_INT_DRV_ROM_RES,
	CFG_INT_DRV_FAST,
};

enum {
	CFG_STR_DRV_ROM,
	CFG_STR_DRV_IMAGE1,
	CFG_STR_DRV_IMAGE2,
};

enum {
	CFG_INT_CPU_SPEED,
	CFG_INT_CPU_EXT
};

enum {
	CFG_STR_ROM,
	CFG_STR_ROM2,
	CFG_STR_ROM3,
	CFG_STR_RAM
};

enum {
	CFG_INT_ROM_RES,
	CFG_INT_ROM_SIZE,
	CFG_INT_ROM_MASK,
	CFG_INT_ROM_OFS,
};

enum {
	CFG_INT_TAPE_FREQ,
	CFG_INT_TAPE_FAST,
};


enum {
	CFG_INT_SOUND_FREQ,
	CFG_INT_SOUND_BUFSIZE,
};

enum {
	CFG_STR_TAPE,
};


enum {
	CFG_INT_PRINT_MODE,
};

enum {
	CFG_INT_MOUSE_TYPE,
};

#endif //SYSCONF_H
