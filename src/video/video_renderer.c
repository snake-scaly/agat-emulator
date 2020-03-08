#include "video_renderer.h"
#include "video_apple_tv.h"

extern struct VIDEO_RENDERER vmode_agat_t32;
extern struct VIDEO_RENDERER vmode_agat_t64;
extern struct VIDEO_RENDERER vmode_agat_t64_inv;
extern struct VIDEO_RENDERER vmode_agat_lgr;
extern struct VIDEO_RENDERER vmode_agat_mgr;
extern struct VIDEO_RENDERER vmode_agat_hgr;
extern struct VIDEO_RENDERER vmode_agat_mcgr;
extern struct VIDEO_RENDERER vmode_agat_dgr;

extern struct VIDEO_RENDERER vmode_apple_t40_tv;
extern struct VIDEO_RENDERER vmode_apple_t40_bw;
extern struct VIDEO_RENDERER vmode_apple_t80;


static const struct VIDEO_RENDERER*agat_get_renderer_page(struct VIDEO_STATE*vs, int mode, int*page)
{
	switch (mode & 3) {
	case 0:
		if (vs->sr->cursystype == SYSTEM_9) {
			/* MCGR */
			*page = ((mode & 0x60) >> 5) | ((mode & 8) >> 1);
			return &vmode_agat_mcgr;
		}
		else {
			/* LGR */
			*page = ((mode & 0x7C) >> 2);
			return &vmode_agat_lgr;
		}
	case 1:
		/* MGR */
		*page = ((mode & 0x70) >> 4) | (mode & 8);
		return &vmode_agat_mgr;
	case 2:
		/* text */
		*page = ((mode & 0x7C) >> 2);
		if (vs->sr->cursystype == SYSTEM_7 && (mode & 0x84) == 0x80) return &vmode_agat_t64_inv;
		else if (mode & 0x80) return &vmode_agat_t64;
		else return &vmode_agat_t32;
	case 3:
		if (mode & 0x80) {
			/* DGR */
			*page = ((mode & 0x60) >> 5) | ((mode & 8) >> 1);
			return &vmode_agat_dgr;
		} else {
			/* HGR */
			*page = ((mode & 0x70) >> 4) | (mode & 8);
			return &vmode_agat_hgr;
		}
	}
	return NULL; /* never happens; needed to silence a compiler warning */
}

const struct VIDEO_RENDERER*ng_apple_get_current_text_renderer(struct VIDEO_STATE*vs, int*page)
{
	*page = vs->ainf.page;
	if (vs->ainf.text80) return &vmode_apple_t80;
	if (vs->sr->cursystype == SYSTEM_9) return &vmode_apple_t40_bw;
	return &vmode_apple_t40_tv;
}

static const struct VIDEO_RENDERER*apple_get_renderer(struct VIDEO_STATE*vs, int*page)
{
	if (vs->ainf.text_mode) return ng_apple_get_current_text_renderer(vs, page);

	// 80-column mode (AKA double resolution) inhibitis page switching
	if (baseram_read_ext_state(0xC018, vs->sr)) *page = 0;
	else *page = vs->ainf.page;

	if (vs->ainf.dhgr) return &vmode_apple_dhgr;
	if (vs->ainf.hgr) {
		if (vs->sr->cursystype == SYSTEM_9) return &vmode_apple_hgr_nofill;
		return &vmode_apple_hgr_tv;
	}
	return &vmode_apple_lgr;
}

const struct VIDEO_RENDERER*ng_get_renderer(struct VIDEO_STATE*vs, int mode, int*page)
{
	if (vs->video_mode == VIDEO_MODE_AGAT) {
		return agat_get_renderer_page(vs, mode, page);
	}
	return apple_get_renderer(vs, page);
}
