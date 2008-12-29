#include "videoint.h"

void set_video_active_range(struct VIDEO_STATE*vs, dword adr, word len, int el)
{
//	printf("set_video_active_range start %x len %x\n", adr, len);
	vs->prev_base = vs->video_base_addr;
	vs->video_base_addr = adr;
	vs->video_mem_size = len;
	vs->video_el_size = el;
}

void set_video_type(struct VIDEO_STATE*vs, int t)
{
	int prev_type = vs->vid_type;
//	printf("set_video_type %i\n", t);
	vs->vid_type = t;
	if (prev_type==vs->vid_type) video_switch_screen_pages(vs);
	else video_repaint_screen(vs);
}



int videosel(struct VIDEO_STATE*vs, int mode)
{
	int prev_type=vs->vid_type;
	int r=vs->vid_mode;
	vs->prev_base=vs->video_base_addr;
	video_set_mode(vs, VIDEO_MODE_AGAT);
	if (vs->vid_mode==mode) return 0xFF;
//	printf("videosel: %x->%x\n",vs->vid_mode,mode);
	switch(vs->sr->cursystype) {
	case SYSTEM_7:
		videosel_7(vs, mode);
		r=0xFF;
		break;
	case SYSTEM_9:
		videosel_9(vs, mode);
		break;
	default:
		return 0xFF;
	}
	if (prev_type==vs->vid_type) video_switch_screen_pages(vs);
	else video_repaint_screen(vs);
	return r;
}
