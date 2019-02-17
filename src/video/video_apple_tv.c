#include "video_apple_tv.h"
#include "videoint.h"
#include "video_renderer.h"
#include <assert.h>
#include <math.h>

/* Gaussian distribution with 0.99 area between -1 and 1. */
static double gauss(double x)
{
	const double c = 0.4;
	const double denom = -2 * c * c;
	return exp(x * x / denom);
}

/*
Generate N coefficients with Gaussian distribution with maximum at N/2 and fading
to zero at coeff[0] and coeff[N-1], that is, the user code can assume that
coeff[-1], coeff[-2] etc. are almost zero, as well as coeff[N], coeff[N+1] etc.

Coefficients are normalized, that is, their sum is equal to one.
*/
static void gauss_coef(double*coeff, int n)
{
	double step = 2.0 / (n + 1);
	double p;
	double sum = 0;
	int i;

	/*
	Coefficients are generated as gauss(p) where p is uniformly distributed
	between -1 and 1. However gauss(-1) is almost zero which is a promise for
	coeff[-1] and coeff[N]. Therefore generate N+2 points on the interval -1..1
	and only use the middle ones for the coefficients.
	*/
	for (i = 0, p = -1 + step; i < n; i++, p += step) {
		coeff[i] = gauss(p);
		sum += coeff[i];
	}

	/* Normalize. */
	for (i = 0; i < n; i++) coeff[i] /= sum;
}

static byte double_color_to_byte(double c)
{
	if (c < 0) c = 0;
	if (c > 1) c = 1;
	return (byte)(c * 255 + 0.5);
}

static void yiq_to_rgbquad(double y, double i, double q, RGBQUAD*c)
{
	memset(c, 0, sizeof(*c));
	c->rgbRed = double_color_to_byte(y + 0.9563 * i + 0.6210 * q);
	c->rgbGreen = double_color_to_byte(y - 0.2721 * i - 0.6474 * q);
	c->rgbBlue = double_color_to_byte(y - 1.1070 * i + 1.7046 * q);
}

static void shift_phase_coef(double*coef)
{
	double c = coef[0];
	memmove(coef, coef + 1, sizeof(double) * 3);
	coef[3] = c;
}

/* Amplitudes are taken from https://en.wikipedia.org/wiki/YIQ */
#define I_AMPLITUDE 0.5957
#define Q_AMPLITUDE 0.5226

#define Y_GAMMA_CORRECTION 1  /* gamma correction for a gamma 2.2 display */
#define IQ_ATTENUATION 0.55       /* empiric number to avoid over-saturation of colors */

#define I_FACTOR (I_AMPLITUDE * IQ_ATTENUATION)
#define Q_FACTOR (Q_AMPLITUDE * IQ_ATTENUATION)

int apple_tv_init(struct APPLE_TV*atv)
{
	double y_coef[YIQ_BITS];
	int phase, bits;
	double i_coef[] = {I_FACTOR / 2, -I_FACTOR / 2, -I_FACTOR / 2, I_FACTOR / 2};
	double q_coef[] = {Q_FACTOR / 2, Q_FACTOR / 2, -Q_FACTOR / 2, -Q_FACTOR / 2};

	gauss_coef(y_coef, YIQ_BITS);

	for (phase = 0; phase < 4; phase++) {
		for (bits = 0; bits < (1 << YIQ_BITS); bits++) {
			double y = 0;
			double i, q;
			int k, color_off, color_bits;

			/* Coefficients are symmetric, doesn't matter which direction to go over bits. */
			for (k = 0; k < YIQ_BITS; k++) y += ((bits >> k) & 1) * y_coef[k];
			y = pow(y, Y_GAMMA_CORRECTION); /* luma */

			/* Use 4 middle bits for quadrature modulation. */
			color_off = (YIQ_BITS - 4) / 2;
			color_bits = (bits >> color_off) & 0xF;

			i = ((bits >> 3) & 1) * i_coef[0] +
				((bits >> 2) & 1) * i_coef[1] +
				((bits >> 1) & 1) * i_coef[2] +
				(bits & 1) * i_coef[3];
			q = ((bits >> 3) & 1) * q_coef[0] +
				((bits >> 2) & 1) * q_coef[1] +
				((bits >> 1) & 1) * q_coef[2] +
				(bits & 1) * q_coef[3];

			yiq_to_rgbquad(y, i, q, &atv->colors[phase][bits]);
		}

		/* Prepare the next phase by shifting coefficients left. */
		shift_phase_coef(i_coef);
		shift_phase_coef(q_coef);
	}

	return 0;
}

int apple_tv_filter(render_line_t render_line, struct VIDEO_STATE*vs, int page, int scanline, struct RENDER_SURFACE*target, RECT*rect)
{
	/* Filtering with YIQ_BITS window consumes 560+YIQ_BITS-1 source bits. Assume black fields
	   to the left and right of the picture. */
	const int pre_bits = YIQ_BITS / 2;
	const int post_bits = YIQ_BITS - pre_bits;

	byte tmp_pixels[568 * 8]; /* 568 instead of 560 to simplify TV filtering */
	struct RENDER_SURFACE tmp_surface = {tmp_pixels + 4, 560, 8, 568, SURFACE_BOOL_560_8};

	memset(tmp_pixels, 0, sizeof(tmp_pixels));
	int result = render_line(vs, page, scanline, &tmp_surface, rect);

	int src_stride = tmp_surface.stride;
	byte*src_line = tmp_surface.pixels;
	byte*src_end_line = src_line + src_stride * (rect->bottom - rect->top);
	byte*dst_line = target->pixels + rect->top * target->stride;

	for (; src_line != src_end_line; src_line += src_stride, dst_line += target->stride) {
		byte*src = src_line;
		byte*src_end = src + 560 + post_bits;
		RGBQUAD*dst = (RGBQUAD*)dst_line;
		int bits = 0;
		int phase = 2;
		int i;

		/* Prefill bits with half the YIQ bits to center the filtered image on the screen. */
		for (i = 0; i < pre_bits; i++, src++, phase++) {
			bits = ((bits << 1) | *src) & ((1 << YIQ_BITS) - 1);
		}

		for (; src != src_end; src++, dst++, phase++) {
			bits = ((bits << 1) | *src) & ((1 << YIQ_BITS) - 1);
			*dst = vs->ng_apple_tv.colors[phase & 3][bits];
		}
	}

	return result;
}

static const byte empty_char[8] = {0};

/*
Apple renderers are two stage.

1. Each individual renderer renders into a temporary buffer of 560x8 boolean pixels.
2. The temporary buffer is rendered into the 560x192 true-color target surface
   with the active filter: B/W, Hi-Res striped, Hi-Res filled, or TV.
*/


/*
line - line start in target surface
x - position of the first pixel to fill in line
npixels - number of pixels to fill
color - a 4-bit pattern
*/
static void apple_fill_bits(byte*line, int x, int npixels, int color)
{
	while (npixels--) {
		int mask = 1 << (3 - (x & 3));
		line[x++] = (color & mask) != 0;
	}
}

static int cc(int c)
{
	return ((c & 8) >> 2) | ((c & 2) << 2) | (c & 5);
}

static int apple_lgr_render_line(struct VIDEO_STATE*vs, int page, int scanline, struct RENDER_SURFACE*target, RECT*rect)
{
	byte*base_mem = ramptr(vs->sr) + (page + 1) * 0x400;
	int block = (scanline & 0x38) >> 3;
	int sub_block = (scanline & 0xC0) >> 6;
	byte*mem = base_mem + block * 0x80 + sub_block * 40;
	int stride = target->stride;

	byte*vmem1 = target->pixels;
	byte*vmem2 = vmem1 + stride * 4;
	int x;

	for (x = 0; x < 560; x += 14, mem++) {
		int c1 = cc(*mem);
		int c2 = cc(*mem >> 4);
		apple_fill_bits(vmem1, x, 14, c1);
		apple_fill_bits(vmem2, x, 14, c2);
	}

	for (x = 0; x < 3; x++, vmem1 += stride, vmem2 += stride) {
		memcpy(vmem1 + stride, vmem1, target->width);
		memcpy(vmem2 + stride, vmem2, target->width);
	}

	rect->left = 0;
	rect->right = 560;
	rect->top = scanline & ~7;
	rect->bottom = rect->top + 8;

	return rect->bottom;
}

static int apple_lgr_map_address(int page, int addr, int*first_scanline, int*last_scanline)
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

static int apple_lgr_round_scanline(int scanline)
{
	return (scanline + 7) & ~7;
}

static int apple_hgr_render_bitstream(struct VIDEO_STATE*vs, int page, int scanline, struct RENDER_SURFACE*target, RECT*rect)
{
	byte*base_mem = ramptr(vs->sr) + (page + 1) * 0x2000;
	int superblock = scanline & 7;
	int block = (scanline & 0x38) >> 3;
	int sub_block = (scanline & 0xC0) >> 6;
	byte*mem = base_mem + superblock * 0x400 + block * 0x80 + sub_block * 40;
	int stride = target->stride;

	byte*vmem = target->pixels;
	int i, j;

	for (i = 0; i < 40; i++) {
		int b = *mem++;
		int shift = !(b & 0x80);

		if (shift) vmem--;

		for (j = 0; j < 7; j++, b >>= 1) {
			int c = b & 1;
			*vmem++ = c;
			*vmem++ = c;
		}

		if (shift) *vmem++ = 0;
	}

	rect->left = 0;
	rect->right = 560;
	rect->top = scanline;
	rect->bottom = scanline + 1;

	return rect->bottom;
}

static int apple_hgr_render_tv(struct VIDEO_STATE*vs, int page, int scanline, struct RENDER_SURFACE*target, RECT*rect)
{
	return apple_tv_filter(apple_hgr_render_bitstream, vs, page, scanline, target, rect);
}

static int apple_hgr_render_nofill(struct VIDEO_STATE*vs, int page, int scanline, struct RENDER_SURFACE*target, RECT*rect)
{
	byte*base_mem = ramptr(vs->sr) + (page + 1) * 0x2000;
	int superblock = scanline & 7;
	int block = (scanline & 0x38) >> 3;
	int sub_block = (scanline & 0xC0) >> 6;
	byte*mem = base_mem + superblock * 0x400 + block * 0x80 + sub_block * 40;
	byte*mem_end = mem + 40;
	int stride = target->stride;

	/* pal[odd_column][shift_bit][color_bit] */
	int pal[2][2][2] = {
		{{0, 5}, {0, 6}},
		{{0, 2}, {0, 1}},
	};

	RGBQUAD*vmem = (RGBQUAD*)(target->pixels + scanline * stride);
	int j, b, curr, odd, shift;

	assert(target->type == SURFACE_RGBA_560_192);

	b = *mem++;
	shift = (b & 0x80) >> 7;
	curr = pal[0][shift][b & 1];
	b >>= 1;
	j = 1;
	odd = 1;

	for (;;) {
		RGBQUAD*c;

		for (; j < 7; j++, b >>= 1, odd = !odd) {
			int next = pal[odd][shift][b & 1];
			if (curr && next) curr = next = 15;
			c = &vs->sr->ng_palette[curr];
			*vmem++ = *c;
			*vmem++ = *c;
			curr = next;
		}

		if (mem == mem_end) {
			c = &vs->sr->ng_palette[curr];
			*vmem++ = *c;
			*vmem++ = *c;
			break;
		}

		b = *mem++;
		shift = (b & 0x80) >> 7;
		j = 0;
	}

	rect->left = 0;
	rect->right = 560;
	rect->top = scanline;
	rect->bottom = scanline + 1;

	return rect->bottom;
}

static int apple_hgr_map_address(int page, int addr, int*first_scanline, int*last_scanline)
{
	int superblock, block, sub_block;
	if (addr < (page + 1 ) * 0x2000 || addr >= (page + 2) * 0x2000) return 0;
	sub_block = (addr & 0x7F) / 40;
	if (sub_block == 3) return 0; /* ignore unused bytes */
	block = (addr & 0x380) >> 7;
	superblock = (addr & 0x1C00) >> 10;
	*first_scanline = sub_block * 64 + block * 8 + superblock;
	*last_scanline = *first_scanline + 1;
	return 1;
}

static int apple_hgr_round_scanline(int scanline)
{
	return scanline;
}

static int apple_dhgr_render_line(struct VIDEO_STATE*vs, int page, int scanline, struct RENDER_SURFACE*target, RECT*rect)
{
	byte*base_mem = ramptr(vs->sr) + (page + 1) * 0x2000;
	int superblock = scanline & 7;
	int block = (scanline & 0x38) >> 3;
	int sub_block = (scanline & 0xC0) >> 6;
	byte*mem1 = base_mem + superblock * 0x400 + block * 0x80 + sub_block * 40;
	byte*mem2 = mem1 + 0x10000;
	int stride = target->stride;

	byte*vmem = target->pixels;
	int i, j;

	for (i = 0; i < 40; i++) {
		int b = *mem2++;
		for (j = 0; j < 7; j++, b >>= 1) *vmem++ = b & 1;
		b = *mem1++;
		for (j = 0; j < 7; j++, b >>= 1) *vmem++ = b & 1;
	}

	rect->left = 0;
	rect->right = 560;
	rect->top = scanline;
	rect->bottom = scanline + 1;

	return rect->bottom;
}

static int apple_dhgr_map_address(int page, int addr, int*first_scanline, int*last_scanline)
{
	int superblock, block, sub_block;
	int page_start = (page + 1 ) * 0x2000;
	int page_end = page_start + 0x2000;

	if (!((addr >= page_start && addr < page_end) || (addr >= page_start + 0x10000 && addr < page_end + 0x10000))) return 0;

	sub_block = (addr & 0x7F) / 40;
	if (sub_block == 3) return 0; /* ignore unused bytes */
	block = (addr & 0x380) >> 7;
	superblock = (addr & 0x1C00) >> 10;

	*first_scanline = sub_block * 64 + block * 8 + superblock;
	*last_scanline = *first_scanline + 1;
	return 1;
}

struct VIDEO_RENDERER vmode_apple_lgr = {
	SURFACE_BOOL_560_8,
	0,
	apple_lgr_render_line,
	apple_lgr_map_address,
	apple_lgr_round_scanline,
};

struct VIDEO_RENDERER vmode_apple_hgr_tv = {
	SURFACE_RGBA_560_192,
	0,
	apple_hgr_render_tv,
	apple_hgr_map_address,
	apple_hgr_round_scanline,
};

struct VIDEO_RENDERER vmode_apple_hgr_nofill = {
	SURFACE_RGBA_560_192,
	0,
	apple_hgr_render_nofill,
	apple_hgr_map_address,
	apple_hgr_round_scanline,
};

struct VIDEO_RENDERER vmode_apple_dhgr = {
	SURFACE_BOOL_560_8,
	0,
	apple_dhgr_render_line,
	apple_dhgr_map_address,
	apple_hgr_round_scanline,
};
