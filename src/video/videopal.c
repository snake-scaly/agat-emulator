#include "videoint.h"


int video_set_pal(struct VIDEO_STATE*vs, int mode)
{
	if (mode == vs->prev_pal) return 0;
	switch (mode) {
	case 0: // 8/a
		vs->c2_palette[0]=0; vs->c2_palette[1]=15;
		vs->c4_palette[0]=0; vs->c4_palette[1]=1;
		vs->c4_palette[2]=2; vs->c4_palette[3]=4;
		vs->c1_palette[0]=0;
		break;
	case 1: // 9/a
		vs->c2_palette[0]=15; vs->c2_palette[1]=0;
		vs->c4_palette[0]=15; vs->c4_palette[1]=1;
		vs->c4_palette[2]=2; vs->c4_palette[3]=4;
		vs->c1_palette[0]=4;
		break;
	case 2: // 8/b
		vs->c2_palette[0]=0; vs->c2_palette[1]=2;
		vs->c4_palette[0]=0; vs->c4_palette[1]=0;
		vs->c4_palette[2]=2; vs->c4_palette[3]=4;
		vs->c1_palette[0]=0;
		break;
	case 3: // 9/b
		vs->c2_palette[0]=2; vs->c2_palette[1]=0;
		vs->c4_palette[0]=0; vs->c4_palette[1]=1;
		vs->c4_palette[2]=0; vs->c4_palette[3]=4;
		vs->c1_palette[0]=5;
		break;
	default:
		return 0;
	}
	vs->prev_pal = mode;
	return 1;
}



void video_set_palette(struct VIDEO_STATE*vs, int mode)
{
	switch (mode) {
	case 0:
		vs->pal_regs[0] = 0;
		break;
	case 1:
		vs->pal_regs[0] = 1;
		break;
	case 2:
		vs->pal_regs[1] = 0;
		break;
	case 3:
		vs->pal_regs[1] = 1;
		break;
	default:
		return;
	}
	if (video_set_pal(vs, vs->pal_regs[0] | (vs->pal_regs[1]<<1))) video_repaint_screen(vs);
}
