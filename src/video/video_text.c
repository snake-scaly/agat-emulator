#include "videoint.h"
#include "video_renderer.h"

static const byte empty_char[8] = {0};

/* Using A7 ROM convention: bits 0-6 are drawn from left to right, 0 is foreground, 1 is background. */
static byte*agat_draw_char_a7(byte*vid, int stride, const byte*glyph, const RGBQUAD*colors, int double_width)
{
	byte*line = vid;
	int i, j;

	for (i = 0; i < 8; i++, glyph++, line += stride) {
		int b = *glyph;
		RGBQUAD*v = (RGBQUAD*)line;
		for (j = 0; j < 7; j++, b >>= 1) {
			int c = !(b & 1);
			*v++ = colors[c];
			if (double_width) *v++ = colors[c];
		}
	}

	return vid + (double_width ? 7*4*2 : 7*4);
}

/* Using A9 ROM convention: bits 7-1 are drawn from left to right, 1 is foreground, 0 is background. */
static byte*agat_draw_char_a9(byte*vid, int stride, const byte*glyph, const RGBQUAD*colors, int double_width)
{
	byte*line = vid;
	int i, j;

	for (i = 0; i < 8; i++, glyph++, line += stride) {
		int b = *glyph;
		RGBQUAD*v = (RGBQUAD*)line;
		for (j = 0; j < 7; j++, b <<= 1) {
			int c = (b & 0x80) >> 7;
			*v++ = colors[c];
			if (double_width) *v++ = colors[c];
		}
	}

	return vid + (double_width ? 7*4*2 : 7*4);
}

static byte*agat_draw_char(struct VIDEO_STATE*vs, byte*vid, int stride, int ch, const RGBQUAD*colors, int double_width)
{
	const byte*glyph = vs->cur_font >= 0 ? vs->font[0][ch] : empty_char;
	if (vs->sr->cursystype == SYSTEM_9) return agat_draw_char_a9(vid, stride, glyph, colors, double_width);
	return agat_draw_char_a7(vid, stride, glyph, colors, double_width);
}

/* Using Apple ROM convention: bits 0-6 are drawn from left to right, 0 is white, 1 is black. */
static byte*apple_apple_draw_char_bitstream(byte*vid, int stride, const byte*glyph, int invert, int double_width)
{
	byte*line = vid;
	int i, j;

	for (i = 0; i < 8; i++, glyph++, line += stride) {
		int b = invert ? *glyph : ~*glyph;
		byte*v = line;
		for (j = 0; j < 7; j++, b >>= 1) {
			int c = b & 1;
			*v++ = c;
			if (double_width) *v++ = c;
		}
	}

	return vid + 7 + 7 * double_width;
}

/* Using Agat 9 ROM convention: bits 7-1 are drawn from left to right, 1 is white, 0 is black. */
static byte*apple_agat_draw_char_bitstream(byte*vid, int stride, const byte*glyph, int invert, int double_width)
{
	byte*line = vid;
	int i, j;

	for (i = 0; i < 8; i++, glyph++, line += stride) {
		int b = invert ? ~*glyph : *glyph;
		byte*v = line;
		for (j = 0; j < 7; j++, b <<= 1) {
			int c = (b & 0x80) >> 7;
			*v++ = c;
			if (double_width) *v++ = c;
		}
	}

	return vid + 7 + 7 * double_width;
}

/* Calculates the glyph and color inversion flag for a character based on the current system state. */
const byte*apple_get_glyph_inversion(struct VIDEO_STATE*vs, int ch, int*invert)
{
	if (vs->sr->cursystype == SYSTEM_9) {
		*invert = ((ch & 0xC0) == 0x00) || ((ch & 0xC0) == 0x40 && vs->pal.flash_mode);
		return vs->cur_font >= 0 ? vs->font[vs->cur_font][ch | 0x80] : empty_char;
	} else {
		*invert = vs->cur_font == 0 && (ch & 0xC0) == 0x40 && vs->pal.flash_mode;
		return vs->cur_font >= 0 ? vs->font[0][ch] : empty_char;
	}
}

static byte*apple_draw_char_bitstream(struct VIDEO_STATE*vs, byte*vid, int stride, int ch, int double_width)
{
	int invert;
	const byte*glyph = apple_get_glyph_inversion(vs, ch, &invert);
	if (vs->sr->cursystype == SYSTEM_9) {
		return apple_agat_draw_char_bitstream(vid, stride, glyph, invert, double_width);
	} else {
		return apple_apple_draw_char_bitstream(vid, stride, glyph, invert, double_width);
	}
}

static byte*apple_draw_char_bw(struct VIDEO_STATE*vs, byte*vid, int stride, int ch, int double_width)
{
	int invert;
	const byte*glyph = apple_get_glyph_inversion(vs, ch, &invert);
	const RGBQUAD colors[] = {
		vs->sr->ng_palette[vs->pal.c2_palette[0]],
		vs->sr->ng_palette[vs->pal.c2_palette[1]],
		vs->sr->ng_palette[vs->pal.c2_palette[0]],
	};

	if (vs->sr->cursystype == SYSTEM_9) {
		return agat_draw_char_a9(vid, stride, glyph, colors + invert, double_width);
	} else {
		return agat_draw_char_a7(vid, stride, glyph, colors + invert, double_width);
	}
}

static int t32_render_line(struct VIDEO_STATE*vs, int page, int scanline, struct RENDER_SURFACE*target, RECT*rect)
{
	int chars_per_line = 32;
	int mem_bytes_per_line = chars_per_line * 2;
	int mem_page_size = mem_bytes_per_line * 32;
	int vid_pixels_per_line = chars_per_line * 7 * 2; /* each pixel is doubled horizontally */
	int vid_width = 512;
	int vid_bytes_per_pixel = 4;
	int vid_width_bytes = vid_width * vid_bytes_per_pixel;
	int vid_bytes_per_border = (vid_width_bytes - vid_pixels_per_line * vid_bytes_per_pixel) / 2;

	int line_number, line_offset;
	const byte*mem;
	byte*vid;
	int stride;
	int i;

	line_number = scanline / 8;
	line_offset = line_number * mem_bytes_per_line;
	mem = ramptr(vs->sr) + page * mem_page_size + line_offset;

	scanline = line_number * 8;
	stride = target->stride;
	vid = target->pixels + scanline * stride;

	/* Text modes are narrow. Fill sides of the screen with black. */
	for (i = 0; i < 8; i++) {
		byte*p = vid + i * stride;
		memset(p, 0, vid_bytes_per_border);
		memset(p + vid_width_bytes - vid_bytes_per_border, 0 , vid_bytes_per_border);
	}

	vid += vid_bytes_per_border;

	for (i = 0; i < chars_per_line; i++, mem += 2) {
		int ch = mem[0];
		int attr = mem[1];
		int color_attr = (attr & 7) | ((attr & 16) >> 1);
		int flash_attr = attr & 8;
		int normal_attr = attr & 32;

		int swap = !normal_attr && (!flash_attr || vs->pal.flash_mode);
		RGBQUAD colors[3] = { /* bg, fg, bg for easy swapping */
			vs->sr->ng_palette[vs->pal.c1_palette[0]],
			vs->sr->ng_palette[color_attr],
			vs->sr->ng_palette[vs->pal.c1_palette[0]],
		};

		vid = agat_draw_char(vs, vid, stride, ch, colors + swap, 1);
	}

	rect->left = 0;
	rect->right = vid_width;
	rect->top = scanline;
	rect->bottom = scanline + 8;

	return rect->bottom;
}

static int t32_map_address(int page, int addr, int*first_scanline, int*last_scanline)
{
	int chars_per_line = 32;
	int mem_bytes_per_line = chars_per_line * 2;
	int mem_page_size = mem_bytes_per_line * 32;

	if (addr < page * mem_page_size || addr >= (page + 1) * mem_page_size) return 0;

	*first_scanline = ((addr % mem_page_size) / mem_bytes_per_line) * 8;
	*last_scanline = *first_scanline + 8;
	return 1;
}

static int t32_round_scanline(int scanline)
{
	return (scanline + 7) & ~7;
}

static int t64_render_line_pal(struct VIDEO_STATE*vs, int page, int scanline, struct RENDER_SURFACE*target, RECT*rect, RGBQUAD*colors)
{
	int chars_per_line = 64;
	int mem_bytes_per_line = chars_per_line;
	int mem_page_size = mem_bytes_per_line * 32;
	int vid_pixels_per_line = chars_per_line * 7;
	int vid_width = 512;
	int vid_bytes_per_pixel = 4;
	int vid_width_bytes = vid_width * vid_bytes_per_pixel;
	int vid_bytes_per_border = (vid_width_bytes - vid_pixels_per_line * vid_bytes_per_pixel) / 2;

	int line_number, line_offset;
	const byte*mem;
	byte*vid;
	int stride;
	int i;

	line_number = scanline / 8;
	line_offset = line_number * mem_bytes_per_line;
	mem = ramptr(vs->sr) + page * mem_page_size + line_offset;

	scanline = line_number * 8;
	stride = target->stride;
	vid = target->pixels + scanline * stride;

	/* Text modes are narrow. Fill sides of the screen with black. */
	for (i = 0; i < 8; i++) {
		byte*p = vid + i * stride;
		memset(p, 0, vid_bytes_per_border);
		memset(p + vid_width_bytes - vid_bytes_per_border, 0 , vid_bytes_per_border);
	}

	vid += vid_bytes_per_border;

	for (i = 0; i < chars_per_line; i++, mem++) vid = agat_draw_char(vs, vid, stride, mem[0], colors, 0);

	rect->left = 0;
	rect->right = vid_width;
	rect->top = scanline;
	rect->bottom = scanline + 8;

	return rect->bottom;
}

static int t64_render_line(struct VIDEO_STATE*vs, int page, int scanline, struct RENDER_SURFACE*target, RECT*rect)
{
	RGBQUAD colors[2] = {
		vs->sr->ng_palette[vs->pal.c2_palette[0]],
		vs->sr->ng_palette[vs->pal.c2_palette[1]],
	};
	return t64_render_line_pal(vs, page, scanline, target, rect, colors);
}

static int t64_render_line_inv(struct VIDEO_STATE*vs, int page, int scanline, struct RENDER_SURFACE*target, RECT*rect)
{
	RGBQUAD colors[2] = {
		vs->sr->ng_palette[vs->pal.c2_palette[1]],
		vs->sr->ng_palette[vs->pal.c2_palette[0]],
	};
	return t64_render_line_pal(vs, page, scanline, target, rect, colors);
}

static int t64_map_address(int page, int addr, int*first_scanline, int*last_scanline)
{
	int chars_per_line = 64;
	int mem_bytes_per_line = chars_per_line;
	int mem_page_size = mem_bytes_per_line * 32;

	if (addr < page * mem_page_size || addr >= (page + 1) * mem_page_size) return 0;

	*first_scanline = ((addr % mem_page_size) / mem_bytes_per_line) * 8;
	*last_scanline = *first_scanline + 8;
	return 1;
}

static int t64_round_scanline(int scanline)
{
	return (scanline + 7) & ~7;
}

static int apple_t40_render_line_bitstream(struct VIDEO_STATE*vs, int page, int scanline, struct RENDER_SURFACE*target, RECT*rect)
{
	byte*base_mem = ramptr(vs->sr) + (page + 1) * 0x400;
	int block = (scanline & 0x38) >> 3;
	int sub_block = (scanline & 0xC0) >> 6;
	byte*mem = base_mem + block * 0x80 + sub_block * 40;
	int stride = target->stride;

	byte*vmem = target->pixels - 1;
	int x;

	for (x = 0; x < 40; x++, mem++) vmem = apple_draw_char_bitstream(vs, vmem, stride, *mem, 1);

	rect->left = 0;
	rect->right = 560;
	rect->top = scanline & ~7;
	rect->bottom = rect->top + 8;

	return rect->bottom;
}

static int apple_t40_render_line_tv(struct VIDEO_STATE*vs, int page, int scanline, struct RENDER_SURFACE*target, RECT*rect)
{
	return apple_tv_filter(apple_t40_render_line_bitstream, vs, page, scanline, target, rect);
}

static int apple_t40_render_line_bw(struct VIDEO_STATE*vs, int page, int scanline, struct RENDER_SURFACE*target, RECT*rect)
{
	byte*base_mem = ramptr(vs->sr) + (page + 1) * 0x400;
	int block = (scanline & 0x38) >> 3;
	int sub_block = (scanline & 0xC0) >> 6;
	byte*mem = base_mem + block * 0x80 + sub_block * 40;
	int stride = target->stride;

	byte*vmem = target->pixels + (scanline & ~7) * stride;
	int x;

	for (x = 0; x < 40; x++, mem++) vmem = apple_draw_char_bw(vs, vmem, stride, *mem, 1);

	rect->left = 0;
	rect->right = 560;
	rect->top = scanline & ~7;
	rect->bottom = rect->top + 8;

	return rect->bottom;
}

static int apple_t40_map_address(int page, int addr, int*first_scanline, int*last_scanline)
{
	int block, sub_block;
	if (addr < (page + 1 ) * 0x400 || addr >= (page + 2) * 0x400) return 0;
	sub_block = (addr & 0x7F) / 40;
	if (sub_block == 3) return 0; /* ignore unused bytes */
	block = (addr & 0x380) >> 7;
	*first_scanline = (sub_block * 8 + block) * 8;
	*last_scanline = *first_scanline + 8;
	return 1;
}

static int apple_t40_round_scanline(int scanline)
{
	return (scanline + 7) & ~7;
}

static int apple_t80_render_line(struct VIDEO_STATE*vs, int page, int scanline, struct RENDER_SURFACE*target, RECT*rect)
{
	byte*base_mem = ramptr(vs->sr) + (page + 1) * 0x400;
	int block = (scanline & 0x38) >> 3;
	int sub_block = (scanline & 0xC0) >> 6;
	byte*mem1 = base_mem + block * 0x80 + sub_block * 40;
	byte*mem2 = mem1 + 0x10000;
	int stride = target->stride;

	byte*vmem = target->pixels - 1;
	int x;

	for (x = 0; x < 40; x++, mem1++, mem2++) {
		vmem = apple_draw_char_bitstream(vs, vmem, stride, *mem2, 0);
		vmem = apple_draw_char_bitstream(vs, vmem, stride, *mem1, 0);
	}

	rect->left = 0;
	rect->right = 560;
	rect->top = scanline & ~7;
	rect->bottom = rect->top + 8;

	return rect->bottom;
}

static int apple_t80_map_address(int page, int addr, int*first_scanline, int*last_scanline)
{
	int block, sub_block;
	int page_start = (page + 1 ) * 0x400;
	int page_end = page_start + 0x400;

	if (!((addr >= page_start && addr < page_end) || (addr >= page_start + 0x10000 && addr < page_end + 0x10000))) return 0;

	sub_block = (addr & 0x7F) / 40;
	if (sub_block == 3) return 0; /* ignore unused bytes */
	block = (addr & 0x380) >> 7;

	*first_scanline = (sub_block * 8 + block) * 8;
	*last_scanline = *first_scanline + 8;
	return 1;
}

struct VIDEO_RENDERER vmode_agat_t32 = {
	SURFACE_RGBA_512_256,
	1,
	t32_render_line,
	t32_map_address,
	t32_round_scanline,
};

struct VIDEO_RENDERER vmode_agat_t64 = {
	SURFACE_RGBA_512_256,
	0,
	t64_render_line,
	t64_map_address,
	t64_round_scanline,
};

struct VIDEO_RENDERER vmode_agat_t64_inv = {
	SURFACE_RGBA_512_256,
	0,
	t64_render_line_inv,
	t64_map_address,
	t64_round_scanline,
};

struct VIDEO_RENDERER vmode_apple_t40_tv = {
	SURFACE_RGBA_560_192,
	1,
	apple_t40_render_line_tv,
	apple_t40_map_address,
	apple_t40_round_scanline,
};

struct VIDEO_RENDERER vmode_apple_t40_bw = {
	SURFACE_RGBA_560_192,
	1,
	apple_t40_render_line_bw,
	apple_t40_map_address,
	apple_t40_round_scanline,
};

struct VIDEO_RENDERER vmode_apple_t80 = {
	SURFACE_BOOL_560_8,
	1,
	apple_t80_render_line,
	apple_t80_map_address,
	apple_t40_round_scanline,
};
