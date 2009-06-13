#include "videoint.h"

static void vid_invalidate_addr_comb(struct VIDEO_STATE*vs, dword adr)
{
	dword badr = (vs->ainf.page+1)*0x400;
	if (vs->ainf.text_mode||!vs->ainf.combined) return;
	if (adr>=badr&&adr<badr+0x400) 
	{
		RECT r;
		apaint_t40_addr_mix(vs, adr, &r);
#ifdef SYNC_SCREEN_UPDATE
		invalidate_video_window(vs->sr, &r);
#else
		UnionRect(&vs->inv_area,&vs->inv_area,&r);
#endif //SYNC_SCREEN_UPDATE
	}
}


void vid_invalidate_addr(struct SYS_RUN_STATE*sr, dword adr)
{
	struct VIDEO_STATE*vs = get_video_state(sr);
	struct RASTER_BLOCK*rb;
	int i;
	for (i = vs->n_rb, rb = vs->rb; i; --i, ++rb) {
		if (adr>=rb->base_addr&&adr<rb->base_addr+rb->mem_size) {
			RECT r;
			paint_addr[rb->vtype](vs, adr, &r);
#ifdef SYNC_SCREEN_UPDATE
			invalidate_video_window(sr, &r);
#else
			UnionRect(&vs->inv_area, &vs->inv_area, &r);
#endif //SYNC_SCREEN_UPDATE
		}
	}
	if (vs->video_mode==VIDEO_MODE_APPLE)
		vid_invalidate_addr_comb(vs, adr);
}
