#include "videoint.h"
#include "video_renderer.h"
#include "video_apple_tv.h"
#include <cpu/cpuint.h>
#include <assert.h>

/* Add a video renderer change at the given scanline. */
static void ng_video_switch_mode_at_scanline(struct VIDEO_STATE*vs, const struct VIDEO_RENDERER*renderer, int page, int scanline)
{
	struct COMBINED_VIDEO_RENDERERS*cvr;
	struct VIDEO_RENDERER_BOUNDARY*last_rb;

	if (renderer->surface_type != SURFACE_BOOL_560_8 && renderer->surface_type != vs->sr->ng_render_surface.type) return;

	cvr = &vs->ng_next_renderers;
	assert(cvr->size >= 1);

	scanline = renderer->round_scanline(scanline);

	/* Replace any renderer switch that happened on the same scanline. */
	while (cvr->size > 0 && cvr->renderers[cvr->size - 1].scanline >= scanline)
		cvr->size--;

	assert(cvr->size <= MAX_RASTER_BLOCKS);
	if (cvr->size == MAX_RASTER_BLOCKS) return; /* too many renderer switches */

	last_rb = cvr->renderers + cvr->size - 1;
	if (last_rb->renderer == renderer && last_rb->page == page) return; /* same renderer, nothing to do */

	cvr->renderers[cvr->size].renderer = renderer;
	cvr->renderers[cvr->size].page = page;
	cvr->renderers[cvr->size].scanline = scanline;
	cvr->size++;
}

/* Add a video renderer change at the current scanline. */
static void ng_video_switch_mode(struct VIDEO_STATE*vs, const struct VIDEO_RENDERER*renderer, int page)
{
	if (vs->video_mode == VIDEO_MODE_AGAT) {
		int tick = (int)(cpu_get_tsc(vs->sr) - vs->ng_frame_start_tick);
		int scanline = (int)(tick / vs->ng_ticks_per_scanline);
		ng_video_switch_mode_at_scanline(vs, renderer, page, scanline);
	} else {
		ng_video_switch_mode_at_scanline(vs, renderer, page, 0);
	}
}

/* Apply video renderers gathered during the current frame and prepare for the next frame. */
static void ng_video_next_frame(struct VIDEO_STATE*vs)
{
	int i_curr_mode, i_next_mode;
	int scanline1, scanline2;
	int mode_differ1, mode_differ2;
	struct VIDEO_RENDERER_BOUNDARY last_vrb;

	assert(vs->ng_curr_renderers.size > 0 && vs->ng_next_renderers.size > 0);
	last_vrb = vs->ng_next_renderers.renderers[vs->ng_next_renderers.size - 1];

	if (vs->video_mode != VIDEO_MODE_AGAT && vs->ainf.combined) {
		/* "Hardware" support for combined text modes. */
		int page;
		const struct VIDEO_RENDERER*r = ng_apple_get_current_text_renderer(vs, &page);
		ng_video_switch_mode_at_scanline(vs, r, page, 20*8);
	}

	/* Compare the gathered renderer list with the current one.
	   Mark scanlines with different renderers as dirty. */
	i_curr_mode = i_next_mode = 0;

	scanline1 = 0;
	mode_differ1 = vs->ng_curr_renderers.renderers[0].renderer != vs->ng_next_renderers.renderers[0].renderer ||
		vs->ng_curr_renderers.renderers[0].page != vs->ng_next_renderers.renderers[0].page;

	for (;;) {
		if (i_curr_mode + 1 == vs->ng_curr_renderers.size && i_next_mode + 1 == vs->ng_next_renderers.size) {
			/* End of loop, handle the final stretch of scanlines. */
			if (mode_differ1) ng_video_mark_scanlines_dirty(vs, scanline1, vs->sr->ng_render_surface.height);
			break;
		}

		/* Find the closest scanline with a renderer switch. */
		if (i_curr_mode + 1 == vs->ng_curr_renderers.size) {
			i_next_mode++;
			scanline2 = vs->ng_next_renderers.renderers[i_next_mode].scanline;
		} else if (i_next_mode + 1 == vs->ng_next_renderers.size) {
			i_curr_mode++;
			scanline2 = vs->ng_curr_renderers.renderers[i_curr_mode].scanline;
		} else if (vs->ng_curr_renderers.renderers[i_curr_mode+1].scanline < vs->ng_next_renderers.renderers[i_next_mode+1].scanline) {
			i_curr_mode++;
			scanline2 = vs->ng_curr_renderers.renderers[i_curr_mode].scanline;
		} else {
			i_next_mode++;
			scanline2 = vs->ng_next_renderers.renderers[i_next_mode].scanline;
		}

		mode_differ2 = vs->ng_curr_renderers.renderers[i_curr_mode].renderer != vs->ng_next_renderers.renderers[i_next_mode].renderer ||
			vs->ng_curr_renderers.renderers[i_curr_mode].page != vs->ng_next_renderers.renderers[i_next_mode].page;

		/* Mark scanlines dirty if this is the end of differences. */
		if (mode_differ1 && !mode_differ2) ng_video_mark_scanlines_dirty(vs, scanline1, scanline2);

		/* Remember the first scanline where differences start. */
		if (!mode_differ1 && mode_differ2) scanline1 = scanline2;

		mode_differ1 = mode_differ2;
	}

	/* Transfer video renderers from next to current. */
	vs->ng_curr_renderers = vs->ng_next_renderers;

	vs->ng_next_renderers.renderers[0] = last_vrb;
	vs->ng_next_renderers.renderers[0].scanline = 0;
	vs->ng_next_renderers.size = 1;
}

static void ng_video_render_frame(struct VIDEO_STATE*vs)
{
	struct COMBINED_VIDEO_RENDERERS*cvm;
	struct VIDEO_RENDERER_BOUNDARY*mb;
	int scanline = 0;
	int mode_idx = 0;
	RECT rect;
	RECT invalid = {0, 0, 0, 0};

	cvm = &vs->ng_curr_renderers;

	/* FIXME init combined_modes with a sensible default */
	assert(cvm->size != 0);

	for (;;) {
		/* Skip clean scanlines. */
		while (scanline < vs->sr->ng_render_surface.height && !ng_video_is_scanline_dirty(vs, scanline)) scanline++;
		if (scanline >= vs->sr->ng_render_surface.height) break;

		/* Find the responsible renderer. */
		while (mode_idx < cvm->size - 1 && cvm->renderers[mode_idx+1].scanline <= scanline) mode_idx++;

		/* Render. */
		mb = cvm->renderers + mode_idx;
		if (mb->renderer->surface_type == vs->sr->ng_render_surface.type) {
			scanline = mb->renderer->render_line(vs, mb->page, scanline, &vs->sr->ng_render_surface, &rect);
		} else if (mb->renderer->surface_type == SURFACE_BOOL_560_8 && vs->sr->ng_render_surface.type == SURFACE_RGBA_560_192) {
			scanline = apple_tv_filter(mb->renderer->render_line, vs, mb->page, scanline, &vs->sr->ng_render_surface, &rect);
		} else {
			/* Mode is incompatible with the current surface, skip. */
			mode_idx++;
			if (mode_idx < cvm->size) scanline = cvm->renderers[mode_idx].scanline;
			else scanline = vs->sr->ng_render_surface.height;
		}

		UnionRect(&invalid, &invalid, &rect);
	}

	/* The screen is rendered. Mark all scanlines clean. */
	ng_video_clear_dirty_scanlines(vs);

	ng_adjust_video_rect(vs->sr, &invalid);
	InvalidateRect(vs->sr->ng_window, &invalid, FALSE);
}

void set_video_active_range(struct VIDEO_STATE*vs, dword adr, word len, int el)
{
//	printf("set_video_active_range start %x len %x\n", adr, len);
	vs->rb_cur.n_ranges = 1;
	vs->rb_cur.base_addr[0] = adr;
	vs->rb_cur.mem_size[0] = len;
	vs->rb_cur.el_size = el;
}

void set_video_type(struct VIDEO_STATE*vs, int t)
{
//	printf("set_video_type %i\n", t);
	vs->rb_cur.vtype = t;
}

int videosel(struct VIDEO_STATE*vs, int mode)
{
	int r=vs->rb_cur.vmode;
	if (vs->sr->apple_emu) return 0xFF;
	video_set_mode(vs, VIDEO_MODE_AGAT);
//	printf("videosel: %x->%x\n",vs->rb_cur.vmode,mode);
	switch(vs->sr->cursystype) {
	case SYSTEM_7:
		r=0xFF;
		if (vs->rb_cur.vmode==mode) return r;
		videosel_7(vs, mode);
		break;
	case SYSTEM_9:
//		if (vs->rb_cur.vmode == 10 && mode == 10) mode = 0x82;
		if (vs->rb_cur.vmode==mode) return r;
		videosel_9(vs, mode);
		break;
	default:
		return 0xFF;
	}
	if (!vs->rb_enabled) video_repaint_screen(vs);
	return r;
}

void ng_videosel2(struct VIDEO_STATE*vs, int mode)
{
	int page;
	const struct VIDEO_RENDERER*vmode = ng_get_renderer(vs, mode, &page);
	ng_video_switch_mode(vs, vmode, page);
}

void video_update_rb(struct VIDEO_STATE*vs, int rbi)
{
	struct RASTER_BLOCK*rb = vs->rb + rbi;
	struct RASTER_BLOCK*crb = &vs->rb_cur;
	int lt;

	if (rb->vmode == crb->vmode) {
		if (rb->dirty) {
//			puts("video_repaint_block");
			video_repaint_block(vs, rbi);
			rb->dirty = 0;
		}
		return;
	}

	lt = rb->vtype;
	rb->vmode = crb->vmode;
	rb->vtype = crb->vtype;
	rb->prev_base[0] = rb->base_addr[0];
	rb->mem_size[0] = crb->mem_size[0] / vs->n_rb;
	rb->base_addr[0] = crb->base_addr[0] + rb->mem_size[0] * rbi;
	rb->n_ranges = crb->n_ranges;

	if (crb->n_ranges > 1) {
		rb->prev_base[1] = rb->base_addr[1];
		rb->mem_size[1] = crb->mem_size[1] / vs->n_rb;
		rb->base_addr[1] = crb->base_addr[1] + rb->mem_size[1] * rbi;
	}	

	rb->el_size = crb->el_size;

	if (lt == crb->vtype && !rb->dirty) {
//		puts("video_switch_block_pages");
		video_switch_block_pages(vs, rbi);
	} else {
//		puts("video_repaint_block");
		video_repaint_block(vs, rbi);
	}
	rb->dirty = 0;
}

void video_first_rb(struct VIDEO_STATE*vs)
{
	vs->rbi = -2;
//	if (vs->rbi < 0) vs->rbi = 0;
//	video_update_rb(vs, vs->rbi);
}

void video_next_rb(struct VIDEO_STATE*vs)
{
	++ vs->rbi;
	if (vs->rbi >= 0 && vs->rbi < vs->n_rb) video_update_rb(vs, vs->rbi);
}

void video_timer(struct VIDEO_STATE*vs, int t)
{
//	printf("video_timer: t = %i\n", t);
	switch (t) {
	case 0:
		if (vs->sr->ints_enabled) {
			system_command(vs->sr, SYS_COMMAND_NMI, 1, 0);
		}
		if (vs->rb_enabled) video_first_rb(vs);
		break;
	case 1:
		if (vs->sr->ints_enabled) {
			int del = 0;
			switch(vs->sr->cursystype) {
			case SYSTEM_7: del = N_RBINT_DELAY_7; break;
			case SYSTEM_9: del = N_RBINT_DELAY_9; break;
			}
			system_command(vs->sr, SYS_COMMAND_IRQ, del, 0);
		}
		if (vs->rb_enabled) video_next_rb(vs);
		break;
	case 2:
		ng_video_next_frame(vs);
		ng_video_render_frame(vs);
		/* Remember the frame start tick for subsequent switches. */
		vs->ng_frame_start_tick = cpu_get_tsc(vs->sr);
		break;
	}
}
