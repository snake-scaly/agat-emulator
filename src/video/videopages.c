#include "videoint.h"

void video_switch_block_pages(struct VIDEO_STATE*vs, int rbi)
{
	RECT inv={0,0,0,0};
	dword sa, da, n;
	byte*mem = ramptr(vs->sr);
	int upd=0;
//	printf("video_switch_screen_pages: type = %i, base = %x, prev_base = %x, size = %x\n", vs->vid_type, vs->video_base_addr, vs->prev_base, vs->video_mem_size);
	switch (vs->rb[rbi].el_size) {
	case 1:
		for (sa=vs->rb[rbi].prev_base,da=vs->rb[rbi].base_addr,n=vs->rb[rbi].mem_size;n;n--,sa++,da++) {
			if (mem[sa]!=mem[da]) {
				RECT r;
				paint_addr[vs->rb[rbi].vtype](vs,da,&r);
				upd=1;
				UnionRect(&inv,&inv,&r);
			}
		}
		break;
	case 2:
		for (sa=vs->rb[rbi].prev_base,da=vs->rb[rbi].base_addr,n=vs->rb[rbi].mem_size;n;n-=2,sa+=2,da+=2) {
			if (mem[sa]!=mem[da]||mem[sa+1]!=mem[da+1]) {
				RECT r;
				paint_addr[vs->rb[rbi].vtype](vs,da,&r);
				upd=1;
				UnionRect(&inv,&inv,&r);
			}
		}
		break;
	default:;
		abort();
	}
	if (upd) invalidate_video_window(vs->sr, &inv);
}
