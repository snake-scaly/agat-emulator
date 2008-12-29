#ifndef SYSCONF_H
#define SYSCONF_H

#define SYSTEMS_DIR	"systems"
#define SAVES_DIR 	"saves"

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

	NDEVTYPES
};


enum {
	CONF_LIST_START = 2,
	CONF_SLOT2 = CONF_LIST_START,
	CONF_SLOT3,
	CONF_SLOT4,
	CONF_SLOT5,
	CONF_SLOT6,

	CONF_EXT = 10,
	CONF_CPU = 10,
	CONF_MEMORY,
	CONF_ROM,
	CONF_CHARSET,
	CONF_SOUND,
	CONF_JOYSTICK,
	CONF_MONITOR,
	CONF_TAPE,

	NCONFTYPES
};

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


#define NMEMSIZES 8
extern const TCHAR*memsizes_s[];
const unsigned memsizes_b[];

extern const TCHAR*devnames[];


extern const TCHAR*confnames[];
extern const TCHAR*sysnames[];
extern const TCHAR*drvtypenames[];


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
	DRV_TYPE_2S2D
};

enum {
	CFG_INT_DRV_TYPE1,
	CFG_INT_DRV_TYPE2,
	CFG_INT_DRV_COUNT,
	CFG_INT_DRV_RO_FLAGS,
	CFG_INT_DRV_ROM_RES,
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
	CFG_STR_TAPE,
};


#endif //SYSCONF_H