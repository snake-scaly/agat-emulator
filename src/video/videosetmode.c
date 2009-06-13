#include "videoint.h"


void video_update_mode(struct VIDEO_STATE*vs)
{
	switch (vs->video_mode) {
	case VIDEO_MODE_AGAT:
		set_video_size(vs->sr, PIX_W*256, PIX_H*256);
		break;
	case VIDEO_MODE_APPLE:
		if (vs->ainf.videoterm && vs->ainf.text_mode) {
			set_video_size(vs->sr, 
				vs->vinf.scr_size[0]*vs->vinf.char_size[0]*vs->vinf.char_scl[0], 
				vs->vinf.scr_size[1]*vs->vinf.char_size[1]*vs->vinf.char_scl[1]);
		} else {
			set_video_size(vs->sr, PIX_W*280, PIX_H*192);
		}
		break;
	}
}


void video_set_mode(struct VIDEO_STATE*vs, int md)
{
	if (vs->video_mode == md) return;
	vs->video_mode = md;
	video_update_mode(vs);
}

