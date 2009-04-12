#include "memory.h"

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
	int xmousepos, ymousepos;
	int mousebtn;
	int keyreg;
	int cur_key;

	int ints_enabled;
};

