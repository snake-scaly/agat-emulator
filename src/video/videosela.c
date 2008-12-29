#include "videoint.h"

void update_video_ap(struct VIDEO_STATE*vs)
{
	if (vs->text_mode) {
		set_video_active_range(vs, (vs->page+1)*0x400, 0x400, 1);
		set_video_type(vs, 7);
	} else if (vs->hgr) {
		set_video_active_range(vs, (vs->page+1)*0x2000, 0x2000, 1);
		set_video_type(vs, 9);
	} else {
		set_video_active_range(vs, (vs->page+1)*0x400, 0x400, 1);
		set_video_type(vs, 8);
	}
}


void vsel_ap(struct VIDEO_STATE*vs, word adr)
{
	if (adr&8) {
		video_set_palette(vs, adr&7);
		return;
	}
	video_set_mode(vs, VIDEO_MODE_APPLE);
//	printf("apple video select: %x\n", adr);
	adr&=7;
//	printf("vs->page = %i\n", vs->page);
	switch(adr) {
	case 0: vs->text_mode = 0; 
		break;
	case 1: vs->text_mode = 1; 
		break;
	case 2: vs->combined = 0; 
		break;
	case 3: vs->combined  = 1; 
		break;
	case 4: vs->page = 0; 
		break;
	case 5: vs->page = 1; 
		break;
	case 6: vs->hgr = 0; 
		break;
	case 7: vs->hgr = 1; 
		break;
	}
	update_video_ap(vs);
}
