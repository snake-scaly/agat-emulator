#ifndef VIDEOINT_H
#define VIDEOINT_H

#include "types.h"
#include "memory.h"
#include "streams.h"
#include "debug.h"
#include "sysconf.h"
#include "runmgr.h"
#include "runmgrint.h"
#include "videow.h"

#define CHAR_W2 (CHAR_W>>1)
#define HGR_W (CHAR_W/8)
#define HGR_H (CHAR_H/8)
#define MGR_W (HGR_W*2)
#define MGR_H (HGR_H*2)
#define LGR_W (MGR_W*2)
#define LGR_H (MGR_H*2)

#define DGR_W (CHAR_W/16)
#define DGR_H (CHAR_H/8)
#define MCGR_W (CHAR_W/8)
#define MCGR_H (CHAR_H/8)


#define MAX_RASTER_BLOCKS 256


#define N_RB_7 16
#define N_RB_9 32

#define N_RBINT_7 20
#define N_RBINT_9 40

#define N_RBINT_DELAY_7 700
#define N_RBINT_DELAY_9 70

#define MAX_RASTER_RANGES 2

struct RASTER_BLOCK
{
	int vmode, vtype;
	int n_ranges;
	int base_addr[MAX_RASTER_RANGES], mem_size[MAX_RASTER_RANGES];
	int el_size, prev_base[MAX_RASTER_RANGES];
	int dirty;
	RECT r;
};

struct APPLE_INFO
{
	int text_mode;// = 0;
	int combined;// = 0;
	int page;// = 0;
	int hgr;// = 1;
	int videoterm; // = 0;
	byte hgr_flags[40][192]; // first and last color of byte
};

struct PAL_INFO
{
	int flash_mode; //=0;
	int cur_mono;  //= 0;
	int c1_palette[1]; //={0};
	int c2_palette[2]; //={0,15};
	int c4_palette[4]; //={0,1,2,4};
	int pal_regs[2];
	int prev_pal;
};

struct VTERM_INFO
{
	const byte*ram;
	word ram_size;
	word ram_ofs;
	word cur_ofs;
	byte cur_size[2];
	byte char_size[2];
	byte char_scl[2];
	byte scr_size[2];
	const byte*font;
};

#define MAX_N_FONTS 2

struct VIDEO_STATE
{
	struct SYS_RUN_STATE*sr;
	int video_mode; //= VIDEO_MODE_INVALID;

	struct RASTER_BLOCK rb[MAX_RASTER_BLOCKS], rb_cur;
	int n_rb, rbi;
	int rb_enabled;

	RECT inv_area;
	byte font[MAX_N_FONTS][256][8];
	int  cur_font;
	int  num_fonts;

	struct APPLE_INFO ainf;
	struct PAL_INFO pal;
	struct VTERM_INFO vinf;
};

enum {
	VIDEO_MODE_INVALID,
	VIDEO_MODE_AGAT,
	VIDEO_MODE_APPLE,
	VIDEO_MODE_APPLE_1
};


__inline struct VIDEO_STATE*get_video_state(struct SYS_RUN_STATE*sr)
{
	return sr->slots[CONF_CHARSET].data;
}

void video_set_mode(struct VIDEO_STATE*vs, int md);
void video_update_mode(struct VIDEO_STATE*vs);
void video_set_palette(struct VIDEO_STATE*vs, int mode);
int video_set_pal(struct PAL_INFO*pi, int mode); // explicit variant for internal use
int video_set_mono(struct VIDEO_STATE*vs, int a, int x); // and with a then xor with x

void videosel_7(struct VIDEO_STATE*vs, int mode);
void videosel_9(struct VIDEO_STATE*vs, int mode);
int  videosel(struct VIDEO_STATE*vs, int mode);

void update_video_ap(struct VIDEO_STATE*vs);
void vsel_ap(struct VIDEO_STATE*vs, word adr);

void video_switch_block_pages(struct VIDEO_STATE*vs, int rbi);

void video_repaint_block(struct VIDEO_STATE*vs, int rbi);
void video_repaint_screen(struct VIDEO_STATE*vs);

void set_video_active_range(struct VIDEO_STATE*vs, dword adr, word len, int el);
void set_video_type(struct VIDEO_STATE*vs, int t);

void video_first_rb(struct VIDEO_STATE*vs);
void video_next_rb(struct VIDEO_STATE*vs);
void video_update_rb(struct VIDEO_STATE*vs, int rbi);

void video_timer(struct VIDEO_STATE*vs, int t);

void (*paint_addr[])(struct VIDEO_STATE*vs, dword addr, RECT*r);

#endif //VIDEOINT_H
