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


struct VIDEO_STATE
{
	struct SYS_RUN_STATE*sr;
	int video_mode; //= VIDEO_MODE_INVALID;
	int video_base_addr; //=0x7800;
	int prev_base; // = -1
	int video_mem_size; //=0x800;
	int video_el_size; //=1;

	RECT inv_area;

	byte font[256][8];

	int vid_mode;
	int vid_type;

	int flash_mode; //=0;
	int cur_mono;  //= 0;
	int c1_palette[1]; //={0};
	int c2_palette[2]; //={0,15};
	int c4_palette[4]; //={0,1,2,4};

	int text_mode;// = 0;
	int combined;// = 0;
	int page;// = 0;
	int hgr;// = 1;
	byte hgr_flags[40][192]; // first and last color of byte
	int prev_pal;
};

enum {
	VIDEO_MODE_INVALID,
	VIDEO_MODE_AGAT,
	VIDEO_MODE_APPLE
};


__inline struct VIDEO_STATE*get_video_state(struct SYS_RUN_STATE*sr)
{
	return sr->slots[CONF_CHARSET].data;
}

void video_set_mode(struct VIDEO_STATE*vs, int md);
void video_update_mode(struct VIDEO_STATE*vs);
void video_set_palette(struct VIDEO_STATE*vs, int mode);
int video_set_pal(struct VIDEO_STATE*vs, int d1,int d2); // explicit variant for internal use
int video_set_mono(struct VIDEO_STATE*vs, int a, int x); // and with a then xor with x

void videosel_7(struct VIDEO_STATE*vs, int mode);
void videosel_9(struct VIDEO_STATE*vs, int mode);
int  videosel(struct VIDEO_STATE*vs, int mode);

void update_video_ap(struct VIDEO_STATE*vs);
void vsel_ap(struct VIDEO_STATE*vs, word adr);

void video_switch_screen_pages(struct VIDEO_STATE*vs);

void video_repaint_screen(struct VIDEO_STATE*vs);

void set_video_active_range(struct VIDEO_STATE*vs, dword adr, word len, int el);
void set_video_type(struct VIDEO_STATE*vs, int t);

void (*paint_addr[])(struct VIDEO_STATE*vs, dword addr, RECT*r);

#endif //VIDEOINT_H
