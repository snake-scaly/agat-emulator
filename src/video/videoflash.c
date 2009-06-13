#include "videoint.h"

void video_flash_text(struct VIDEO_STATE*vs)
{
	int x, y;
	int i;
	struct RASTER_BLOCK*rb;
	vs->pal.flash_mode=!vs->pal.flash_mode;
	for (i = vs->n_rb, rb = vs->rb; i; --i, ++rb) {
		if (rb->vtype!=2&&vs->video_mode!=VIDEO_MODE_APPLE) continue;
		rb->dirty = 1;
	}
}



