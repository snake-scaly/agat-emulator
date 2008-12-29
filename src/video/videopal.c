#include "videoint.h"


int video_set_pal(struct VIDEO_STATE*vs, int d1,int d2)
{
	int set=0;
	switch (d2) {
	case 2:
		switch (d1) {
		case 0: // 8/a
			vs->c2_palette[0]=0; vs->c2_palette[1]=15;
			vs->c4_palette[0]=0; vs->c4_palette[1]=1;
			vs->c4_palette[2]=2; vs->c4_palette[3]=4;
			vs->c1_palette[0]=0;
			set=1;
			break;
		case 1: // 9/a
			vs->c2_palette[0]=15; vs->c2_palette[1]=0;
			vs->c4_palette[0]=15; vs->c4_palette[1]=1;
			vs->c4_palette[2]=2; vs->c4_palette[3]=4;
			vs->c1_palette[0]=4;
			set=1;
			break;
		}
		break;
	case 3:
		switch (d1) {
		case 0: // 8/b
			vs->c2_palette[0]=0; vs->c2_palette[1]=2;
			vs->c4_palette[0]=0; vs->c4_palette[1]=0;
			vs->c4_palette[2]=2; vs->c4_palette[3]=4;
			vs->c1_palette[0]=0;
			set=1;
			break;
		case 1: // 9/b
			vs->c2_palette[0]=2; vs->c2_palette[1]=0;
			vs->c4_palette[0]=0; vs->c4_palette[1]=1;
			vs->c4_palette[2]=0; vs->c4_palette[3]=4;
			vs->c1_palette[0]=5;
			set=1;
			break;
		}
		break;
	}
	return set;
}



void video_set_palette(struct VIDEO_STATE*vs, int mode)
{
	int set=0;
	printf("palette: mode=%i, prev_pal=%i\n",mode, vs->prev_pal);
	switch (mode) {
	case 0:
		set=video_set_pal(vs, mode,vs->prev_pal);
		break;
	case 1:
		set=video_set_pal(vs, mode,vs->prev_pal);
		break;
	case 2:
		set=video_set_pal(vs, vs->prev_pal,mode);
		break;
	case 3:
		set=video_set_pal(vs, vs->prev_pal,mode);
		break;
	}
	vs->prev_pal=mode;
	if (set) video_repaint_screen(vs);
}
