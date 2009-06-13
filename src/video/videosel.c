#include "videoint.h"

void set_video_active_range(struct VIDEO_STATE*vs, dword adr, word len, int el)
{
//	printf("set_video_active_range start %x len %x\n", adr, len);
	vs->rb_cur.base_addr = adr;
	vs->rb_cur.mem_size = len;
	vs->rb_cur.el_size = el;
}

void set_video_type(struct VIDEO_STATE*vs, int t)
{
	vs->rb_cur.vtype = t;
}

int videosel(struct VIDEO_STATE*vs, int mode)
{
	int r=vs->rb_cur.vmode;
	video_set_mode(vs, VIDEO_MODE_AGAT);
	if (vs->rb_cur.vmode==mode) return 0xFF;
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
	if (!vs->rb_enabled) video_first_rb(vs);
	return r;
}

void video_update_rb(struct VIDEO_STATE*vs, int rbi)
{
	struct RASTER_BLOCK*rb = vs->rb + rbi;
	struct RASTER_BLOCK*crb = &vs->rb_cur;
	int lt;
	if (rb->vmode == crb->vmode) {
		if (rb->dirty) {
			video_repaint_block(vs, rbi);
			rb->dirty = 0;
		}
		return;
	}
	lt = rb->vtype;
	rb->vmode = crb->vmode;
	rb->vtype = crb->vtype;
	rb->prev_base = rb->base_addr;
	rb->mem_size = crb->mem_size / vs->n_rb;
	rb->base_addr = crb->base_addr + rb->mem_size * rbi;
	rb->el_size = crb->el_size;

	if (lt == crb->vtype) {
		video_switch_block_pages(vs, rbi);
	} else {
		video_repaint_block(vs, rbi);
	}
	rb->dirty = 0;
}

void video_first_rb(struct VIDEO_STATE*vs)
{
	vs->rbi = 0;
	video_update_rb(vs, vs->rbi);
}

void video_next_rb(struct VIDEO_STATE*vs)
{
	++ vs->rbi;
	if (vs->rbi >= vs->n_rb) vs->rbi = 0;
	video_update_rb(vs, vs->rbi);
}

void video_timer(struct VIDEO_STATE*vs, int t)
{
	printf("video_timer: t = %i\n", t);
	switch (t) {
	case 0:
		video_first_rb(vs);
		break;
	case 1:
		video_next_rb(vs);
		break;
	}
}

