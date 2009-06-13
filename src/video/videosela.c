#include "videoint.h"

void update_video_ap(struct VIDEO_STATE*vs)
{
	struct APPLE_INFO*ai = &vs->ainf;
	if (ai->text_mode) {
		if (ai->videoterm) {
			set_video_active_range(vs, 0x10000, vs->vinf.ram_size, 1);
			set_video_type(vs, 11);
		} else {
			set_video_active_range(vs, (ai->page+1)*0x400, 0x400, 1);
			set_video_type(vs, 7);
		}
	} else if (ai->hgr) {
		if (basemem_n_blocks(vs->sr) < (ai->page+2) * 4) return;
		set_video_active_range(vs, (ai->page+1)*0x2000, 0x2000, 1);
		set_video_type(vs, 9);
	} else {
		set_video_active_range(vs, (ai->page+1)*0x400, 0x400, 1);
		set_video_type(vs, 8);
	}
}


void vsel_ap(struct VIDEO_STATE*vs, word adr)
{
	struct APPLE_INFO*ai = &vs->ainf;
	struct VTERM_INFO*vi = &vs->vinf;
	if (adr&8) {
		if (vi->ram) {
			switch (adr&0x0F) {
			case 8:
				if (ai->videoterm) puts("videoterm disabled");
				ai->videoterm = 0;
				break;
			case 9:
				if (!ai->videoterm) puts("videoterm enabled");
				ai->videoterm = 1;
				break;
			}
			video_update_mode(vs);
			update_video_ap(vs);
			return;
		} else {
			video_set_palette(vs, adr&7);
			return;
		}
	}
	video_set_mode(vs, VIDEO_MODE_APPLE);
//	printf("apple video select: %x\n", adr);
	adr&=7;
//	printf("vs->page = %i\n", vs->page);
	switch(adr) {
	case 0: ai->text_mode = 0; 
		break;
	case 1: ai->text_mode = 1; 
		break;
	case 2: ai->combined = 0; 
		break;
	case 3: ai->combined  = 1; 
		break;
	case 4: ai->page = 0; 
		break;
	case 5: ai->page = 1; 
		break;
	case 6: 
		if (vs->sr->cursystype == SYSTEM_A) {
			ai->hgr = 0;
		} else {
			ai->hgr = 1; // low-res emulation was not implemented in Agat
		}
		break;
	case 7: ai->hgr = 1; 
		break;
	}
	update_video_ap(vs);
	video_update_mode(vs);
}
