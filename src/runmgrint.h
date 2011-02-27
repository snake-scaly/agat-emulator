#include "memory.h"
#include "keyb.h"

#define BASEMEM_BLOCK_SIZE	0x800
#define BASEMEM_BLOCK_SHIFT	11
#define BASEMEM_NBLOCKS		0x20


struct SYS_RUN_STATE
{
	LPCTSTR name;
	int   cursystype;
	struct SYSCONFIG*config;
	struct SLOT_RUN_STATE slots[NCONFTYPES];
	struct MEM_PROC base_mem[BASEMEM_NBLOCKS];
	struct MEM_PROC baseio_sel[16];
	struct MEM_PROC io_sel[8];
	struct MEM_PROC io6_sel[0x10];

	HWND base_w;
	SIZE v_size;
	struct KEYMAP keymap;
	int  fullscreen;
	HWND video_w;
	WINDOWPLACEMENT old_pl;
	LONG old_style;
	HMENU popup_menu;
	DWORD th;
	HANDLE h;
	HBITMAP old_bmp, mem_bmp;
	HDC mem_dc;
	int bmp_pitch;
	LPVOID bmp_bits;
	int mousechanged; // bit0 - btn1, bit1 - btn2, bit2 - btn3, bit 7 - pos
	int xmousepos, ymousepos; // restricted position
	int xmouse, ymouse; // unrestricted position
	int dxmouse, dymouse;
	int mouselock;
	int mouselocked;
	int mousebtn;
	int keyreg;
	int cur_key;

	int ints_enabled;

	int pause_inactive; // pause execution when window is inactive
	int apple_emu;

	void*ptr;
};

