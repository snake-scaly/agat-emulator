#include "videoint.h"
#include "video_renderer.h"

static RGBQUAD*fill_pixels(RGBQUAD*dst, RGBQUAD color, int length)
{
	RGBQUAD*end = dst + length;
	for (; dst < end; dst++) {
		*dst = color;
	}
	return dst;
}

static int agat_lgr_render_line(struct VIDEO_STATE*vs, int page, int scanline, struct RENDER_SURFACE*target, RECT*rect)
{
	const int mem_bpl = 32;
	const int vid_spl = 4;

	int line = scanline / vid_spl;
	byte*mem = ramptr(vs->sr) + page * 0x800 + line * mem_bpl;
	byte*lptr, *lend, *vid, *vend;
	RGBQUAD*vline;
	int stride = target->stride;

	scanline = line * vid_spl;
	vid = target->pixels + scanline * stride;
	vline = (RGBQUAD*)vid;
	for (lptr = mem, lend = mem + mem_bpl; lptr < lend; lptr++) {
		vline = fill_pixels(vline, vs->sr->ng_palette[*lptr >> 4], 8);
		vline = fill_pixels(vline, vs->sr->ng_palette[*lptr & 15], 8);
	}

	vend = vid + stride * (vid_spl - 1);
	while (vid < vend) {
		byte*vnext = vid + stride;
		memcpy(vnext, vid, 512 * sizeof(RGBQUAD));
		vid = vnext;
	}

	rect->left = 0;
	rect->right = 512;
	rect->top = scanline;
	rect->bottom = scanline + vid_spl;

	return rect->bottom;
}

static int agat_lgr_map_address(int page, int addr, int*first_scanline, int*last_scanline)
{
	if (addr < page * 0x800 || addr >= (page + 1) * 0x800) return 0;
	*first_scanline = (addr & 0x7E0) >> 3;
	*last_scanline = *first_scanline + 4;
	return 1;
}

static int agat_lgr_round_scanline(int scanline)
{
	return (scanline + 3) & ~3;
}

static int agat_mgr_render_line(struct VIDEO_STATE*vs, int page, int scanline, struct RENDER_SURFACE*target, RECT*rect)
{
	const int mem_bpl = 64;
	const int vid_spl = 2;

	int line = scanline / vid_spl;
	byte*mem = ramptr(vs->sr) + page * 0x2000 + line * mem_bpl;
	byte*lptr, *lend, *vid;
	RGBQUAD*vline;
	int stride = target->stride;

	scanline = line * vid_spl;
	vid = target->pixels + scanline * stride;
	vline = (RGBQUAD*)vid;
	for (lptr = mem, lend = mem + mem_bpl; lptr < lend; lptr++) {
		vline = fill_pixels(vline, vs->sr->ng_palette[*lptr >> 4], 4);
		vline = fill_pixels(vline, vs->sr->ng_palette[*lptr & 15], 4);
	}

	memcpy(vid + stride, vid, 512 * sizeof(RGBQUAD));

	rect->left = 0;
	rect->right = 512;
	rect->top = scanline;
	rect->bottom = scanline + vid_spl;

	return rect->bottom;
}

static int agat_mgr_map_address(int page, int addr, int*first_scanline, int*last_scanline)
{
	if (addr < page * 0x2000 || addr >= (page + 1) * 0x2000) return 0;
	*first_scanline = (addr & 0x1FC0) >> 5;
	*last_scanline = *first_scanline + 2;
	return 1;
}

static int agat_mgr_round_scanline(int scanline)
{
	return (scanline + 1) & ~1;
}

static int agat_hgr_render_line(struct VIDEO_STATE*vs, int page, int scanline, struct RENDER_SURFACE*target, RECT*rect)
{
	const int mem_bpl = 32;

	byte*mem = ramptr(vs->sr) + page * 0x2000 + scanline * mem_bpl;
	byte*lptr, *lend, *vid;
	RGBQUAD*vline;
	RGBQUAD colors[] = {vs->sr->ng_palette[0], vs->sr->ng_palette[15]};
	int stride = target->stride;
	RGBQUAD white = {255,255,255,255};

	vid = target->pixels + scanline * stride;
	vline = (RGBQUAD*)vid;
	for (lptr = mem, lend = mem + mem_bpl; lptr < lend; lptr++) {
		int i, b = *lptr;
		for (i = 0; i < 8; i++, b <<= 1) {
			vline = fill_pixels(vline, colors[(b & 0x80) >> 7], 2);
		}
	}

	rect->left = 0;
	rect->right = 512;
	rect->top = scanline;
	rect->bottom = scanline + 1;

	return rect->bottom;
}

static int agat_hgr_map_address(int page, int addr, int*first_scanline, int*last_scanline)
{
	if (addr < page * 0x2000 || addr >= (page + 1) * 0x2000) return 0;
	*first_scanline = (addr & 0x1FE0) >> 5;
	*last_scanline = *first_scanline + 1;
	return 1;
}

static int agat_hgr_round_scanline(int scanline)
{
	return scanline;
}

static int agat_mcgr_render_line(struct VIDEO_STATE*vs, int page, int scanline, struct RENDER_SURFACE*target, RECT*rect)
{
	const int mem_bpl = 64;

	byte*mem = ramptr(vs->sr) + page * 0x4000 + (scanline & 1) * 0x2000 + (scanline >> 1) * mem_bpl;
	byte*lptr, *lend, *vid;
	RGBQUAD*vline;
	RGBQUAD colors[] = {
		vs->sr->ng_palette[vs->pal.c4_palette[0]],
		vs->sr->ng_palette[vs->pal.c4_palette[1]],
		vs->sr->ng_palette[vs->pal.c4_palette[2]],
		vs->sr->ng_palette[vs->pal.c4_palette[3]],
	};
	int stride = target->stride;

	vid = target->pixels + scanline * stride;
	vline = (RGBQUAD*)vid;
	for (lptr = mem, lend = mem + mem_bpl; lptr < lend; lptr++) {
		int i, b = *lptr;
		for (i = 0; i < 4; i++, b <<= 2) {
			vline = fill_pixels(vline, colors[(b & 0xC0) >> 6], 2);
		}
	}

	rect->left = 0;
	rect->right = 512;
	rect->top = scanline;
	rect->bottom = scanline + 1;

	return rect->bottom;
}

static int agat_mcgr_map_address(int page, int addr, int*first_scanline, int*last_scanline)
{
	if (addr < page * 0x4000 || addr >= (page + 1) * 0x4000) return 0;
	*first_scanline = ((addr & 0x1FC0) >> 5) | ((addr & 0x2000) >> 13);
	*last_scanline = *first_scanline + 1;
	return 1;
}

static int agat_mcgr_round_scanline(int scanline)
{
	return scanline;
}

static int agat_dgr_render_line(struct VIDEO_STATE*vs, int page, int scanline, struct RENDER_SURFACE*target, RECT*rect)
{
	const int mem_bpl = 64;

	byte*mem = ramptr(vs->sr) + page * 0x4000 + (scanline & 1) * 0x2000 + (scanline >> 1) * mem_bpl;
	byte*lptr, *lend, *vid;
	RGBQUAD*vline;
	RGBQUAD colors[] = {vs->sr->ng_palette[0], vs->sr->ng_palette[15]};
	int stride = target->stride;

	vid = target->pixels + scanline * stride;
	vline = (RGBQUAD*)vid;
	for (lptr = mem, lend = mem + mem_bpl; lptr < lend; lptr++) {
		int i, b = *lptr;
		for (i = 0; i < 8; i++, b <<= 1, vline++) {
			*vline = colors[(b & 0x80) >> 7];
		}
	}

	rect->left = 0;
	rect->right = 512;
	rect->top = scanline;
	rect->bottom = scanline + 1;

	return rect->bottom;
}

static int agat_dgr_map_address(int page, int addr, int*first_scanline, int*last_scanline)
{
	if (addr < page * 0x4000 || addr >= (page + 1) * 0x4000) return 0;
	*first_scanline = ((addr & 0x1FC0) >> 5) | ((addr & 0x2000) >> 13);
	*last_scanline = *first_scanline + 1;
	return 1;
}

static int agat_dgr_round_scanline(int scanline)
{
	return scanline;
}

struct VIDEO_RENDERER vmode_agat_lgr = {
	SURFACE_RGBA_512_256,
	0,
	agat_lgr_render_line,
	agat_lgr_map_address,
	agat_lgr_round_scanline,
};

struct VIDEO_RENDERER vmode_agat_mgr = {
	SURFACE_RGBA_512_256,
	0,
	agat_mgr_render_line,
	agat_mgr_map_address,
	agat_mgr_round_scanline,
};

struct VIDEO_RENDERER vmode_agat_hgr = {
	SURFACE_RGBA_512_256,
	0,
	agat_hgr_render_line,
	agat_hgr_map_address,
	agat_hgr_round_scanline,
};

struct VIDEO_RENDERER vmode_agat_mcgr = {
	SURFACE_RGBA_512_256,
	0,
	agat_mcgr_render_line,
	agat_mcgr_map_address,
	agat_mcgr_round_scanline,
};

struct VIDEO_RENDERER vmode_agat_dgr = {
	SURFACE_RGBA_512_256,
	0,
	agat_dgr_render_line,
	agat_dgr_map_address,
	agat_dgr_round_scanline,
};
