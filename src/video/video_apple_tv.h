#ifndef VIDEO_APPLE_TV_H
#define VIDEO_APPLE_TV_H

#include <Windows.h>

#define YIQ_BITS 6

struct APPLE_TV
{
	RGBQUAD colors[4][1 << YIQ_BITS];
};

typedef int (*render_line_t)(struct VIDEO_STATE*vs, int page, int scanline, struct RENDER_SURFACE*target, RECT*rect);

int apple_tv_init(struct APPLE_TV*atv);
int apple_tv_filter(render_line_t render_line, struct VIDEO_STATE*vs, int page, int scanline, struct RENDER_SURFACE*target, RECT*rect);

extern struct VIDEO_RENDERER vmode_apple_lgr;
extern struct VIDEO_RENDERER vmode_apple_hgr_tv;
extern struct VIDEO_RENDERER vmode_apple_hgr_nofill;
extern struct VIDEO_RENDERER vmode_apple_dhgr;

#endif /* VIDEO_APPLE_TV_H */
