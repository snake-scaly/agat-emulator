#include "videoint.h"

#include "video_renderer.h"

/* A table for quickly setting bits between two bit indices.
   The table is ng_bit_span[first][second] where both indices
   are inclusive for alignment. The table only contains valid
   values for first <= second. */
static const byte ng_bit_span[8][8] = {
	{0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF},
	{0x00, 0x40, 0x60, 0x70, 0x78, 0x7C, 0x7E, 0x7F},
	{0x00, 0x00, 0x20, 0x30, 0x38, 0x3C, 0x3E, 0x3F},
	{0x00, 0x00, 0x00, 0x10, 0x18, 0x1C, 0x1E, 0x1F},
	{0x00, 0x00, 0x00, 0x00, 0x08, 0x0C, 0x0E, 0x0F},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x06, 0x07},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01},
};

/* Mark a half-open range of scanlines as dirty.
   first - first dirty scanline
   last - one past last dirty scanline. The `last` itself is not marked */
void ng_video_mark_scanlines_dirty(struct VIDEO_STATE*vs, int first, int last)
{
	int first_bit, last_bit;
	int first_bucket, last_bucket;

	if (first >= last) return;

	first_bit = first & 7;
	last_bit = last & 7;
	first_bucket = first >> 3;
	last_bucket = last >> 3;

	if (first_bucket == last_bucket) {
		vs->ng_dirty_scanlines[first_bucket] |= ng_bit_span[first_bit][last_bit-1];
		return;
	}

	if (first_bit != 0) {
		vs->ng_dirty_scanlines[first_bucket] |= ng_bit_span[first_bit][7];
		first_bucket++;
	}

	memset(vs->ng_dirty_scanlines + first_bucket, 0xFF, last_bucket - first_bucket);

	if (last_bit != 0) vs->ng_dirty_scanlines[last_bucket] |= ng_bit_span[0][last_bit-1];
}

/* Check if a scanline is dirty. */
int ng_video_is_scanline_dirty(struct VIDEO_STATE*vs, int scanline)
{
	int bucket = scanline >> 3;
	int bit = ~scanline & 7;
	return vs->ng_dirty_scanlines[bucket] & (1 << bit);
}

/* Mark all scanlines as clean. */
void ng_video_clear_dirty_scanlines(struct VIDEO_STATE*vs)
{
	memset(vs->ng_dirty_scanlines, 0, sizeof(vs->ng_dirty_scanlines));
}

void ng_vid_invalidate_addr_2(struct VIDEO_STATE*vs, int addr)
{
	struct COMBINED_VIDEO_RENDERERS*cvm;
	struct VIDEO_RENDERER_BOUNDARY*mb1;
	int i, first, last;

	cvm = &vs->ng_curr_renderers;

	/* Always check all renderers. It is possible that a byte
	   is visible in multiple renderers at the same time.*/
	for (i = 0; i < cvm->size; i++) {
		mb1 = cvm->renderers + i;
		if (!mb1->renderer->map_address(mb1->page, addr, &first, &last)) continue; /* byte not visible */

		/* Check if the change is visible on the screen. */
		if (first < mb1->scanline) first = mb1->scanline;
		if (i < cvm->size - 1) {
			struct VIDEO_RENDERER_BOUNDARY*mb2 = mb1 + 1;
			if (last > mb2->scanline) last = mb2->scanline;
		}
		if (last <= first) continue; /* byte not visible */

		ng_video_mark_scanlines_dirty(vs, first, last);
	}
}

static void vid_invalidate_addr_comb(struct VIDEO_STATE*vs, dword adr)
{
	dword badr = (vs->ainf.page+1)*0x400;
	if (vs->ainf.text_mode||!vs->ainf.combined) return;
	if (!vs->sr->bmp_bits) return;
	if (adr>=badr&&adr<badr+0x400) 
	{
		RECT r;
		apaint_t40_addr_mix(vs, adr, &r);
		if (!vs->sr->sync_update) {
			invalidate_video_window(vs->sr, &r);
		} else {
			UnionRect(&vs->inv_area,&vs->inv_area,&r);
			vs->mem_access = 0;
		}
	}
}


void vid_invalidate_addr(struct SYS_RUN_STATE*sr, dword adr)
{
	struct VIDEO_STATE*vs = get_video_state(sr);
	struct RASTER_BLOCK*rb;
	dword sadr = adr;
	int i, j;
	if (sr->cursystype == SYSTEM_E) sadr &= 0xFFFF;
	if (!sr->bmp_bits) return;
//	printf("video_invalidate: adr = %x; n_rb = %i\n", adr, vs->n_rb);
	for (i = vs->n_rb, rb = vs->rb; i; --i, ++rb) {
//		printf("i = %i; n_ranges = %i\n", i, rb->n_ranges);
		for (j = rb->n_ranges - 1; j >= 0; --j) {
//			printf("j = %i; base_addr = %x; mem_size = %x\n", j, rb->base_addr[j], rb->mem_size[j]);
			if (sadr>=(dword)rb->base_addr[j]&&sadr<(dword)(rb->base_addr[j]+rb->mem_size[j])) {
				RECT r;
				if (vs->video_mode == VIDEO_MODE_AGAT && rb->vmode != vs->rb_cur.vmode) {
					rb->dirty = 1;
				} else {
//					printf("paint_addr[%i]: %x\n", rb->vtype, adr);
					paint_addr[rb->vtype](vs, adr, &r);
					if (sr->sync_update) {
						invalidate_video_window(sr, &r);
					} else {
						UnionRect(&vs->inv_area, &vs->inv_area, &r);
					}
				}
			}
		}
	}
	vs->mem_access = 0;
	if (vs->video_mode==VIDEO_MODE_APPLE)
		vid_invalidate_addr_comb(vs, sadr);
	ng_vid_invalidate_addr_2(vs, adr);
}
