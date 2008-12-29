#include "videoint.h"

static void video_repaint_screen_comb(struct VIDEO_STATE*vs)
{
	dword addr, n;
	dword badr = (vs->page+1)*0x400;
	if (vs->text_mode||!vs->combined) return;
	for (addr=badr,n=0x400;n;n--,addr++) {
		RECT r;
		apaint_t40_addr_mix(vs, addr,&r);
	}
	invalidate_video_window(vs->sr, NULL);
}

void video_repaint_screen(struct VIDEO_STATE*vs)
{
	dword addr, n;
//	printf("video_repaint_screen: type = %i, base = %x, size = %x\n", vs->vid_type, vs->video_base_addr, vs->video_mem_size);
	for (addr=vs->video_base_addr,n=vs->video_mem_size;n;n-=vs->video_el_size,addr+=vs->video_el_size) {
		RECT r;
		paint_addr[vs->vid_type](vs, addr, &r);
	}
	if (vs->video_mode==VIDEO_MODE_APPLE) video_repaint_screen_comb(vs);
	invalidate_video_window(vs->sr, NULL);
}
