#include "videoint.h"

void video_flash_text(struct VIDEO_STATE*vs)
{
	int x, y;
#ifdef SYNC_SCREEN_UPDATE
	if (vs->vid_type!=2&&vs->video_mode!=VIDEO_MODE_APPLE) return;
#else
	if (vs->vid_type!=2&&vs->video_mode!=VIDEO_MODE_APPLE) {
		invalidate_video_window(vs->sr, &vs->inv_area);
		ZeroMemory(&vs->inv_area, sizeof(vs->inv_area));
	}
#endif
	vs->flash_mode=!vs->flash_mode;
	video_repaint_screen(vs);
}



