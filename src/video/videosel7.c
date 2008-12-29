#include "videoint.h"

void videosel_7(struct VIDEO_STATE*vs, int mode)
{
	int page, subpage, type;
	int basemem_blocks = basemem_n_blocks(vs->sr)>>3;
	printf("basemem_blocks = %i\n", basemem_blocks);
	page=(mode>>4)&(basemem_blocks*2-1);
	subpage=(mode>>2)&3;
	type=(mode&3);
	printf("video page = %i, subpage = %i, type = %i\n",
		page, subpage, type);
	vs->vid_mode=mode;
	vs->vid_type=type;
	switch (type) {
	case 0:
		vs->video_base_addr=(page<<0x0D)+(subpage<<0x0B);
		vs->video_mem_size=0x800;
		vs->video_el_size=1;
		break;
	case 1:
		vs->video_base_addr=(page<<0x0D);
		vs->video_mem_size=0x2000;
		vs->video_el_size=1;
		break;
	case 2:
		page&=7;
		vs->video_base_addr=(page<<0x0D)+(subpage<<0x0B);
		vs->video_mem_size=0x800;
		vs->video_el_size=2;
		if (mode&0x80) {
			vs->vid_type=4;
			if (mode&4) { vs->vid_type = 10; }
			vs->video_el_size=1;
		}
		break;
	case 3:
		vs->video_base_addr=(page<<0x0D);
		vs->video_mem_size=0x2000;
		vs->video_el_size=1;
		break;
	}
}

