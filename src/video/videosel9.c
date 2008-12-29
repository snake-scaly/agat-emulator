#include "videoint.h"

void videosel_9(struct VIDEO_STATE*vs, int mode)
{
	int page, subpage, type;
	page=(mode>>4)&0x0F;
	subpage=(mode>>2)&3;
	type=(mode&3);
	vs->vid_mode=mode;
	vs->vid_type=type;
	switch (type) {
	case 0:
		if (mode & 0x80) {
			page -= 8;
			vs->vid_type=6;
		} else {
			vs->vid_type=5;
		}
		if (mode&8) page+=8;
		page&=~1;
		vs->video_base_addr=(page<<0x0D);
		vs->video_mem_size=0x4000;
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
			vs->video_el_size=1;
		}
		break;
	case 3:
		if (mode&0x80) {
			vs->vid_type=6;
			page&=~1;
			vs->video_mem_size=0x4000;
		} else{
			vs->video_mem_size=0x2000;
		}
		vs->video_base_addr=(page<<0x0D);
		vs->video_el_size=1;
		break;
	}
//	printf("selector=%x, vid_type=%i, base_addr=%x, page=%i\n",mode,vs->vid_type,vs->video_base_addr,page);
}

