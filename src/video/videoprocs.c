#include "videoint.h"

void paint_lgr_addr(struct VIDEO_STATE*vs, dword addr, RECT*r)
{
	int bmp_pitch = vs->sr->bmp_pitch;
	byte*bmp_bits = vs->sr->bmp_bits;

	int x=(addr<<1)&63;
	int y=(addr>>5)&63;
	int sx=8, sy=4;
	byte*ptr, c1, c2, d;
	const byte*mem = ramptr(vs->sr);
	x<<=2;
	y<<=2;
#ifdef DOUBLE_X
	x<<=1;
	sx<<=1;
#endif
#ifdef DOUBLE_Y
	y<<=1;
	sy<<=1;
#endif
	r->left=x;
	r->top=y;
	r->right=r->left+sx;
	r->bottom=r->top+sy;

	ptr=bmp_bits+(x>>1)+(y*bmp_pitch);
	d=mem[addr];
	c1=d>>4;
	c2=d&0x0F;
	c1=c1|(c1<<4);
	c2=c2|(c2<<4);
#ifdef DOUBLE_X
	ptr[0]=c1;
	ptr[1]=c1;
	ptr[2]=c1;
	ptr[3]=c1;
	ptr[4]=c2;
	ptr[5]=c2;
	ptr[6]=c2;
	ptr[7]=c2;
	ptr+=bmp_pitch;
	ptr[0]=c1;
	ptr[1]=c1;
	ptr[2]=c1;
	ptr[3]=c1;
	ptr[4]=c2;
	ptr[5]=c2;
	ptr[6]=c2;
	ptr[7]=c2;
	ptr+=bmp_pitch;
	ptr[0]=c1;
	ptr[1]=c1;
	ptr[2]=c1;
	ptr[3]=c1;
	ptr[4]=c2;
	ptr[5]=c2;
	ptr[6]=c2;
	ptr[7]=c2;
	ptr+=bmp_pitch;
	ptr[0]=c1;
	ptr[1]=c1;
	ptr[2]=c1;
	ptr[3]=c1;
	ptr[4]=c2;
	ptr[5]=c2;
	ptr[6]=c2;
	ptr[7]=c2;
#ifdef DOUBLE_Y
	ptr+=bmp_pitch;
	ptr[0]=c1;
	ptr[1]=c1;
	ptr[2]=c1;
	ptr[3]=c1;
	ptr[4]=c2;
	ptr[5]=c2;
	ptr[6]=c2;
	ptr[7]=c2;
	ptr+=bmp_pitch;
	ptr[0]=c1;
	ptr[1]=c1;
	ptr[2]=c1;
	ptr[3]=c1;
	ptr[4]=c2;
	ptr[5]=c2;
	ptr[6]=c2;
	ptr[7]=c2;
	ptr+=bmp_pitch;
	ptr[0]=c1;
	ptr[1]=c1;
	ptr[2]=c1;
	ptr[3]=c1;
	ptr[4]=c2;
	ptr[5]=c2;
	ptr[6]=c2;
	ptr[7]=c2;
	ptr+=bmp_pitch;
	ptr[0]=c1;
	ptr[1]=c1;
	ptr[2]=c1;
	ptr[3]=c1;
	ptr[4]=c2;
	ptr[5]=c2;
	ptr[6]=c2;
	ptr[7]=c2;
#endif //DOUBLE_Y
#else // DOUBLE_X
	ptr[0]=c1;
	ptr[1]=c1;
	ptr[2]=c2;
	ptr[3]=c2;
	ptr+=bmp_pitch;
	ptr[0]=c1;
	ptr[1]=c1;
	ptr[2]=c2;
	ptr[3]=c2;
	ptr+=bmp_pitch;
	ptr[0]=c1;
	ptr[1]=c1;
	ptr[2]=c2;
	ptr[3]=c2;
	ptr+=bmp_pitch;
	ptr[0]=c1;
	ptr[1]=c1;
	ptr[2]=c2;
	ptr[3]=c2;
#ifdef DOUBLE_Y
	ptr+=bmp_pitch;
	ptr[0]=c1;
	ptr[1]=c1;
	ptr[2]=c2;
	ptr[3]=c2;
	ptr+=bmp_pitch;
	ptr[0]=c1;
	ptr[1]=c1;
	ptr[2]=c2;
	ptr[3]=c2;
	ptr+=bmp_pitch;
	ptr[0]=c1;
	ptr[1]=c1;
	ptr[2]=c2;
	ptr[3]=c2;
	ptr+=bmp_pitch;
	ptr[0]=c1;
	ptr[1]=c1;
	ptr[2]=c2;
	ptr[3]=c2;
#endif //DOUBLE_Y
#endif //DOUBLE_X
}

void paint_mgr_addr(struct VIDEO_STATE*vs, dword addr, RECT*r)
{
	int bmp_pitch = vs->sr->bmp_pitch;
	byte*bmp_bits = vs->sr->bmp_bits;
	int x=(addr<<1)&127;
	int y=(addr>>6)&127;
	int sx=4, sy=2;
	byte*ptr, c1, c2, d;
	const byte*mem = ramptr(vs->sr);
	x<<=1;
	y<<=1;
#ifdef DOUBLE_X
	x<<=1;
	sx<<=1;
#endif
#ifdef DOUBLE_Y
	y<<=1;
	sy<<=1;
#endif
	r->left=x;
	r->top=y;
	r->right=r->left+sx;
	r->bottom=r->top+sy;
	ptr=(byte*)bmp_bits+(x>>1)+(y*bmp_pitch);
	d=mem[addr];
	c1=d>>4;
	c2=d&0x0F;
	c1=c1|(c1<<4);
	c2=c2|(c2<<4);
#ifdef DOUBLE_X
	ptr[0]=c1;
	ptr[1]=c1;
	ptr[2]=c2;
	ptr[3]=c2;
	ptr+=bmp_pitch;
	ptr[0]=c1;
	ptr[1]=c1;
	ptr[2]=c2;
	ptr[3]=c2;
#ifdef DOUBLE_Y
	ptr+=bmp_pitch;
	ptr[0]=c1;
	ptr[1]=c1;
	ptr[2]=c2;
	ptr[3]=c2;
	ptr+=bmp_pitch;
	ptr[0]=c1;
	ptr[1]=c1;
	ptr[2]=c2;
	ptr[3]=c2;
#endif //DOUBLE_Y
#else // DOUBLE_X
	ptr[0]=c1;
	ptr[1]=c2;
	ptr+=bmp_pitch;
	ptr[0]=c1;
	ptr[1]=c2;
#ifdef DOUBLE_Y
	ptr+=bmp_pitch;
	ptr[0]=c1;
	ptr[1]=c2;
	ptr+=bmp_pitch;
	ptr[0]=c1;
	ptr[1]=c2;
#endif //DOUBLE_Y
#endif //DOUBLE_X
}

void paint_t32_addr(struct VIDEO_STATE*vs, dword addr, RECT*r)
{
	int bmp_pitch = vs->sr->bmp_pitch;
	byte*bmp_bits = vs->sr->bmp_bits;
	int x=(addr>>1)&31;
	int y=(addr>>6)&31;
	byte*ptr=(byte*)bmp_bits+((y*bmp_pitch*CHAR_H+x*CHAR_W2));
	const byte*mem = ramptr(vs->sr);
	byte ch=mem[addr&~1];
	byte atr=mem[addr|1];
	int  tc, bc=vs->pal.c1_palette[0]; // text and back colors
	byte*fnt=vs->font[ch];
	int mask;
	int xi, yi, xn, yn;
	r->left=x*CHAR_W;
	r->top=y*CHAR_H;
	r->right=r->left+CHAR_W;
	r->bottom=r->top+CHAR_H;
	tc=atr&7;
	if (atr&16) tc|=8;
	switch (atr&(8+32)) {
	case 8: if (!vs->pal.flash_mode) break; //flash
	case 0: bc=tc; tc=vs->pal.c1_palette[0]; break; // inverse
	}
//	printf("%x [%x, %x]: %i,%i\n",addr,video_base_addr,video_base_addr+video_mem_size,x,y);
	for (yn=8;yn;yn--,fnt++) {
		byte*p=ptr;
		for (xn=8,mask=0x80;xn;xn--,mask>>=1,p++) {
			byte cl;
#ifdef DOUBLE_X
			byte c=((*fnt)&mask)?tc:bc;
			cl=c|(c<<4);
#else
			byte c1, c2;
			c1=((*fnt)&mask)?tc:bc;
			mask>>=1;
			xn--;
			c2=((*fnt)&mask)?tc:bc;
			cl=c2|(c1<<4);
#endif
			p[0]=cl;
#ifdef DOUBLE_Y
			p[bmp_pitch]=cl;
#endif
		}
		ptr+=bmp_pitch;
#ifdef DOUBLE_Y
		ptr+=bmp_pitch;
#endif
	}
}

void paint_hgr_addr(struct VIDEO_STATE*vs, dword addr, RECT*r)
{
	int bmp_pitch = vs->sr->bmp_pitch;
	byte*bmp_bits = vs->sr->bmp_bits;
	int x=(addr<<3)&255;
	int y=(addr>>5)&255;
	int sx=8, sy=1;
	int clr[2]={vs->pal.c2_palette[0], vs->pal.c2_palette[1]};
	int i;
	const byte*mem = ramptr(vs->sr);
	byte b=mem[addr];
	byte *ptr;
#ifdef DOUBLE_X
	x<<=1;
	sx<<=1;
#endif
#ifdef DOUBLE_Y
	y<<=1;
	sy<<=1;
#endif
	r->left=x;
	r->top=y;
	r->right=r->left+sx;
	r->bottom=r->top+sy;
//	printf("%x: (%i,%i)\n",addr,x,y);
	ptr=(byte*)bmp_bits+(x>>1)+(y*bmp_pitch);
#ifdef DOUBLE_X
	for (i=8;i;i--,b<<=1,ptr++) {
		byte c;
		c=clr[(b&0x80)?1:0];
		c|=(c<<4);
		ptr[0]=c;
#ifdef DOUBLE_Y
		ptr[bmp_pitch]=c;
#endif //DOUBLE_Y
	}
#else
	for (i=4;i;i--,b<<=1,ptr++) {
		byte c1, c2, c;
		c1=clr[(b&0x80)?1:0];
		b<<=1;
		c2=clr[(b&0x80)?1:0];
		c=c2|(c1<<4);
		ptr[0]=c;
#ifdef DOUBLE_Y
		ptr[bmp_pitch]=c;
#endif //DOUBLE_Y
	}
#endif //DOUBLE_X
}


void paint_t64_addr(struct VIDEO_STATE*vs, dword addr, RECT*r, int tc, int bc)
{
	int bmp_pitch = vs->sr->bmp_pitch;
	byte*bmp_bits = vs->sr->bmp_bits;
	int x=(addr)&63;
	int y=(addr>>6)&31;
	byte*ptr=(byte*)bmp_bits+((y*bmp_pitch*CHAR_H+x*CHAR_W2/2));
	const byte*mem = ramptr(vs->sr);
	byte ch=mem[addr];
	byte*fnt=vs->font[ch];
	int mask;
	int xi, yi, xn, yn;
	r->left=x*CHAR_W2;
	r->top=y*CHAR_H;
	r->right=r->left+CHAR_W2;
	r->bottom=r->top+CHAR_H;
//	printf("%x [%x, %x]: %i,%i\n",addr,video_base_addr,video_base_addr+video_mem_size,x,y);
	for (yn=8;yn;yn--,fnt++) {
		byte*p=ptr;
		for (xn=8,mask=0x80;xn;xn--,mask>>=1,p++) {
			byte c1, c2, cl;
			c1=((*fnt)&mask)?tc:bc;
#ifndef DOUBLE_X
			mask>>=1;
			if ((*fnt)&mask) c1=tc;
			xn--;
#endif //DOUBLE_X
			mask>>=1;
			xn--;
			c2=((*fnt)&mask)?tc:bc;
#ifndef DOUBLE_X
			mask>>=1;
			if ((*fnt)&mask) c2=tc;
			xn--;
#endif //DOUBLE_X
			cl=c2|(c1<<4);
			p[0]=cl;
#ifdef DOUBLE_Y
			p[bmp_pitch]=cl;
#endif
		}
		ptr+=bmp_pitch;
#ifdef DOUBLE_Y
		ptr+=bmp_pitch;
#endif
	}
}


void paint_t64_addr_n(struct VIDEO_STATE*vs, dword addr, RECT*r)
{
	paint_t64_addr(vs, addr, r, vs->pal.c2_palette[1], vs->pal.c2_palette[0]);
}

void paint_t64_addr_i(struct VIDEO_STATE*vs, dword addr, RECT*r)
{
	paint_t64_addr(vs, addr, r, 0, 15);
}


void paint_mcgr_addr(struct VIDEO_STATE*vs, dword addr, RECT*r)
{
	int bmp_pitch = vs->sr->bmp_pitch;
	byte*bmp_bits = vs->sr->bmp_bits;
	int pixels[4]={vs->pal.c4_palette[0],
		vs->pal.c4_palette[1],
		vs->pal.c4_palette[2],
		vs->pal.c4_palette[3]};
	int x,y;
	int sx=4,sy=1;
	const byte*mem = ramptr(vs->sr);
	byte b = mem[addr], *ptr;
	int n;
	addr&=0x3FFF;
	x=(addr&63)<<2;
	y=(addr>>5)&~1;
	if (y&0x100) y-=0xFF;
#ifdef DOUBLE_X
	x<<=1;
	sx<<=1;
#endif
#ifdef DOUBLE_Y
	y<<=1;
	sy<<=1;
#endif
	r->left=x;
	r->top=y;
	r->right=r->left+sx;
	r->bottom=r->top+sy;
//	printf("%x: (%i,%i): %x\n",addr,x,y, b);
	ptr=(byte*)bmp_bits+(x>>1)+(y*bmp_pitch);
	for (n=4;n;n--,b<<=2,ptr++) {
		byte cl, c1, c2;
#ifdef DOUBLE_X
		cl=pixels[b>>6];
		cl=cl|(cl<<4);
#else
		c1=pixels[b>>6];
		b<<=2;
		n--;
		c2=pixels[b>>6];
		cl=c2|(c1<<4);
#endif // DOUBLE_X
		ptr[0]=cl;
#ifdef DOUBLE_Y
		ptr[bmp_pitch]=cl;
#endif // DOUBLE_Y
	}
}


void paint_dgr_addr(struct VIDEO_STATE*vs, dword addr, RECT*r)
{
	int bmp_pitch = vs->sr->bmp_pitch;
	byte*bmp_bits = vs->sr->bmp_bits;
	int pixels[2]={vs->pal.c2_palette[0],vs->pal.c2_palette[1]};
	int x,y;
	int sx=8, sy=1;
	int clr[2]={0,15};
	const byte*mem = ramptr(vs->sr);
	byte b=mem[addr],*ptr;
	int i;
	addr&=0x3FFF;
	x=(addr&63)<<3;
	y=(addr>>5)&~1;
	if (y&0x100) y-=0xFF;
#ifndef DOUBLE_X
	x>>=1;
	sx>>=1;
#endif //DOUBLE_Y
#ifdef DOUBLE_Y
	y<<=1;
	sy<<=1;
#endif //DOUBLE_Y
	r->left=x;
	r->top=y;
	r->right=r->left+sx;
	r->bottom=r->top+sy;
//	printf("%x: (%i,%i)\n",addr,x,y);
	ptr=(byte*)bmp_bits+(x>>1)+(y*bmp_pitch);
	for (i=8;i;i--,b<<=1,ptr++) {
		byte c1, c2, c;
		c1=clr[(b&0x80)?1:0];
#ifndef DOUBLE_X
		b<<=1;
		if (b&0x80) c1=clr[1];
		i--;
#endif //DOUBLE_X
		b<<=1;
		c2=clr[(b&0x80)?1:0];
		i--;
#ifndef DOUBLE_X
		b<<=1;
		if (b&0x80) c2=clr[1];
		i--;
#endif //DOUBLE_X
		c=c2|(c1<<4);
		ptr[0]=c;
#ifdef DOUBLE_Y
		ptr[bmp_pitch]=c;
#endif //DOUBLE_Y
	}
}





void apaint_t80_addr(struct VIDEO_STATE*vs, dword addr, RECT*r)
{
	dword saddr = addr;
	dword caddr = vs->vinf.cur_ofs;
	div_t d;
	int x, y;
	int cx = vs->vinf.char_size[0] * vs->vinf.char_scl[0];
	int cy = vs->vinf.char_size[1] * vs->vinf.char_scl[1];
	addr -= vs->vinf.ram_ofs;
	addr &= (vs->vinf.ram_size - 1);
	saddr &= (vs->vinf.ram_size - 1);
	caddr &= (vs->vinf.ram_size - 1);
	d = div(addr, vs->vinf.scr_size[0]);
	x = d.rem;
	y = d.quot;
	if (y >= vs->vinf.scr_size[1]) {
		SetRectEmpty(r);
		return;
	}

	{
	int bmp_pitch = vs->sr->bmp_pitch;
	byte*bmp_bits = vs->sr->bmp_bits;
	byte*ptr=(byte*)bmp_bits+(y*bmp_pitch*cy+x*cx/2);
	byte ch=vs->vinf.ram[saddr];
	int  tc=15, bc=0; // text and back colors
	const byte*fnt = vs->vinf.font + ch * 16;
	int mask;
	int fl0 = 0;
	int xi, yi, xn, yn;
	int ys = vs->vinf.cur_size[0] & 0x0F, ye = vs->vinf.cur_size[1] & 0x0F;
//	printf("addr = %x; x = %i; y = %i\n", addr, x, y);
	r->left=x*cx;
	r->top=y*cy;
	r->right=r->left+cx;
	r->bottom=r->top+cy;
//	printf("addr = %x, cur = %x\n", saddr, vs->vinf.cur_ofs);
	if (saddr == caddr) {
		if (vs->pal.flash_mode || !(vs->vinf.cur_size[0]&0x40))
			fl0 = 1;
	}
	for (yn = 0;yn < vs->vinf.char_size[1]; yn++,fnt++) {
		int fl = 0;
		byte*p=ptr;
		if (yn >= ys && yn <= ye) fl = fl0;
		for (xn=vs->vinf.char_size[0],mask=0x80;xn;xn--,mask>>=1,p++) {
			byte c1, c2, cl;
			c1=((((*fnt)&mask)==0)==fl)?tc:bc;
			mask>>=1;
			xn--;
			c2=((((*fnt)&mask)==0)==fl)?tc:bc;
			cl=c2|(c1<<4);
			p[0]=cl;
			if (vs->vinf.char_scl[1] > 1) {
				p[bmp_pitch]=cl;
			}
		}
		ptr+=bmp_pitch;
		if (vs->vinf.char_scl[1] > 1) {
			ptr+=bmp_pitch;
		}
	}
	}
}


void apaint_t40_addr(struct VIDEO_STATE*vs, dword addr, RECT*r)
{
	int bmp_pitch = vs->sr->bmp_pitch;
	byte*bmp_bits = vs->sr->bmp_bits;
	int nb = (addr&0x3FF)>>7;
	int bofs = addr&0x7F;
	int bl = bofs / 40;
	int y = nb + bl * 8;
	int x = bofs % 40;
	const byte*mem = ramptr(vs->sr);
	byte*ptr=(byte*)bmp_bits+((y*bmp_pitch*CHAR_H+x*PIX_W*7/2));
	byte ch = mem[addr];
	byte atr = ch>>6;
	int  tc=vs->pal.c2_palette[1], bc=vs->pal.c2_palette[0]; // text and back colors
	const byte*fnt;
	int mask;
	int xi, yi, xn, yn;
	if (vs->sr->cursystype != SYSTEM_A && ch<0xA0) {
		ch+=0x20;
		ch&=0x3F;
		ch+=0xA0;
	}
	fnt = vs->font[ch];
//	printf("%x -> (%i, %i)\n",addr, x, y);
	r->left=x*PIX_W*7;
	r->top=y*CHAR_H;
	r->right=r->left+PIX_W*7;
	r->bottom=r->top+CHAR_H;
	switch(atr) {
	case 1: if (!vs->pal.flash_mode) break; //flash
	case 0: bc=vs->pal.c2_palette[1]; tc=vs->pal.c2_palette[0]; break; // inverse
	}
//	printf("%x [%x, %x]: %i,%i\n",addr,video_base_addr,video_base_addr+video_mem_size,x,y);
	for (yn=8;yn;yn--,fnt++) {
		byte*p=ptr;
		for (xn=7,mask=0x40;xn;xn--,mask>>=1,p++) {
			byte cl;
#ifdef DOUBLE_X
			byte c=((*fnt)&mask)?tc:bc;
			cl=c|(c<<4);
#else
			byte c1, c2;
			c1=((*fnt)&mask)?tc:bc;
			mask>>=1;
			xn--;
			c2=((*fnt)&mask)?tc:bc;
			cl=c2|(c1<<4);
#endif
			p[0]=cl;
#ifdef DOUBLE_Y
			p[bmp_pitch]=cl;
#endif
		}
		ptr+=bmp_pitch;
#ifdef DOUBLE_Y
		ptr+=bmp_pitch;
#endif
	}
}

void apaint_t40_addr_mix(struct VIDEO_STATE*vs, dword addr, RECT*r)
{
	int bmp_pitch = vs->sr->bmp_pitch;
	byte*bmp_bits = vs->sr->bmp_bits;
	int nb = (addr&0x3FF)>>7;
	int bofs = addr&0x7F;
	int bl = bofs / 40;
	int y = nb + bl * 8;
	int x = bofs % 40;
	if (y<20) return;
	apaint_t40_addr(vs, addr, r);
}


void apaint_gr_addr(struct VIDEO_STATE*vs, dword addr, RECT*r)
{
	int bmp_pitch = vs->sr->bmp_pitch;
	byte*bmp_bits = vs->sr->bmp_bits;
	int nb = (addr&0x3FF)>>7;
	int bofs = addr&0x7F;
	int bl = bofs / 40;
	int y = (nb + bl * 8)*2;
	int x = bofs % 40;
	const byte*mem = ramptr(vs->sr);
	byte*ptr=(byte*)bmp_bits+((y*bmp_pitch*CHAR_H/2+x*PIX_W*7/2));
	byte d = mem[addr], c1, c2, c;
	int xn, yn;
	static const byte gr_pal[16] = {0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15};
//	static const byte gr_pal[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
	if (vs->ainf.combined&&y>=40) {
		return;
	}
	c1=gr_pal[d>>4];
	c2=gr_pal[d&0x0F];
	c1=c1|(c1<<4);
	c2=c2|(c2<<4);
	r->left=x*PIX_W*7;
	r->top=y*CHAR_H/2;
	r->right=r->left+CHAR_W;
	r->bottom=r->top+CHAR_H;
	c = c2;
	for (yn=8;yn;yn--) {
		byte*p=ptr;
		if (yn==4) c = c1;
		for (xn=4;xn;xn--,p++) {
#ifdef DOUBLE_X
			if (xn > 1) {
				p[0]=c;
				p++;
			}
#endif
			p[0]=c;
#ifdef DOUBLE_Y
			p[bmp_pitch]=c;
#ifdef DOUBLE_X
			if (xn > 1) {
				p[bmp_pitch-1]=c;
			}
#endif
#endif
		}
		ptr+=bmp_pitch;
#ifdef DOUBLE_Y
		ptr+=bmp_pitch;
#endif
	}
}

void apaint_hgr_addr_mono(struct VIDEO_STATE*vs, dword addr, RECT*r)
{
	int bmp_pitch = vs->sr->bmp_pitch;
	byte*bmp_bits = vs->sr->bmp_bits;
	int np = (addr&0x1FFF)>>10;
	int nb = (addr&0x3FF)>>7;
	int bofs = addr&0x7F;
	int bl = bofs / 40;
	int y = (nb + bl * 8) * 8 + np;
	int x = (bofs % 40) * 7;
	int i;
	const byte*mem = ramptr(vs->sr);
	byte*ptr=(byte*)bmp_bits+((y*bmp_pitch*HGR_H+x*HGR_W/2));
	int clr[2]={vs->pal.c2_palette[0],vs->pal.c2_palette[1]};
	byte b=mem[addr];
//	printf("%x -> (%i, %i), np = %i, nb = %i\n",addr, x, y, np, nb);
	if (vs->ainf.combined&&y>=160) {
		return;
	}
	r->left=x*HGR_W;
	r->top=y*HGR_H;
	r->right=r->left+HGR_W*7;
	r->bottom=r->top+HGR_H;
	for (i=7;i;i--,b>>=1,ptr++) {
		byte c;
		c=clr[(b&1)?1:0];
		c|=(c<<4);
		ptr[0]=c;
		ptr[bmp_pitch]=c;
	}
}

void apaint_hgr_addr_color(struct VIDEO_STATE*vs, dword addr, RECT*r)
{
	int bmp_pitch = vs->sr->bmp_pitch;
	byte*bmp_bits = vs->sr->bmp_bits;
	int np = (addr&0x1FFF)>>10;
	int nb = (addr&0x3FF)>>7;
	int bofs = addr&0x7F;
	int bl = bofs / 40;
	int y = (nb + bl * 8) * 8 + np;
	int x7 = (bofs % 40);
	int x = x7 * 7;
	int i;
	const byte*mem = ramptr(vs->sr);
	byte*ptr=(byte*)bmp_bits+((y*bmp_pitch*HGR_H+x*HGR_W/2));
	const int clr1[2]={0,15};
//	const int clr2[2][2]={{5,2},{12,1}};
	const int clr2[2][2]={{5,10},{6,9}};
	int b=mem[addr], bp;
	int palbits;
	int i2;
	int wasc = 0;
	int npix = 7;
	int fill = (vs->sr->cursystype == SYSTEM_A);
//	byte fl;

//	printf("%x -> (%i, %i), np = %i, nb = %i\n",addr, x, y, np, nb);
	if (vs->ainf.combined&&y>=160) {
		return;
	}

	if (b & 0x80) palbits = 0x7F; else palbits = 0;
	b &= 0x7F;
	i2 = x & 1;

	if (x > 0) {
		bp = mem[addr - 1];
//		if ((bp & 0x40) != ((bp & 0x20)<<1)) { // check two rightmost bits of the previous byte
			// this byte affects pixels from the previouts byte
			npix +=2;
			x -= 2;
			ptr -= 2;
//			i2 ^= 1;
			b <<= 2;
			palbits <<= 2;
			if (bp & 0x80) palbits |= 3;
			if (bp & 0x40) b |= 2;
			if (bp & 0x20) b |= 1;
			if (bp & 0x10) {
				wasc = ptr[-1]&0x0F;
//				if (wasc != clr1[1]) wasc = clr1[0];
			} else wasc = clr1[0];
//		}
	}

	if (x<273) {
		bp = mem[addr + 1];
		if (bp & 0x80) palbits |= (7<<npix);
		if (bp & 0x01) b |= (1<<npix);
		if (bp & 0x02) b |= (2<<npix);
		if (bp & 0x04) b |= (4<<npix);
		npix += 2;
	}

	r->left=x*HGR_W;
	r->top=y*HGR_H;
	r->right=r->left+HGR_W*npix;
	r->bottom=r->top+HGR_H;


	for (; npix; --npix, b>>=1, palbits>>=1, ptr++, i2^=1) {
		byte c, c0;
		if (b&1) {
			c0 = clr2[palbits&1][i2];
			if (wasc||(b&2)) {
				c = c0 = clr1[1];
			} else c = c0;
		} else { c0 = clr1[0]; c = (fill&&(b&2)&&(wasc!=clr1[1]||!(b&4)))?wasc:c0; }
		wasc = c0;
		c|=(c<<4);
		ptr[0]=c;
		ptr[bmp_pitch]=c;
	}
/*	if (b&0x80) i1 = 1; else i1 = 0;
	b&=0x7F;
	if (x) {
		firstc = vs->ainf.hgr_flags[x7-1][y]&0x0F;
	} else firstc = clr1[0];
	wasc = firstc;
	if (x<273) {
		lastc = vs->ainf.hgr_flags[x7+1][y]>>4;
	} else lastc = clr1[0];
	if (lastc) b|=0x80;
	i2 = x&1;
	if ((b&3)==3) {
		fl = clr1[1]<<4;
	} else {
		fl = ((b&1)?clr2[i1][i2]:clr1[0])<<4; // first color
	}
	if ((b&0x60)==0x60) fl|=clr1[1];

	r->left=(x-1)*HGR_W;
	r->top=y*HGR_H;
	r->right=r->left+HGR_W*9;
	r->bottom=r->top+HGR_H;
	if (x) {
		if (firstc&&(fl>>4)) {
			byte c;
			c = clr1[1];
			c|=(c<<4);
			ptr[-1]=c;
			ptr[bmp_pitch-1]=c;
		} else {
			byte c;
			c = firstc;
			c|=(c<<4);
			ptr[-1]=c;
			ptr[bmp_pitch-1]=c;
		}
	}
	for (i=7;i;i--,b>>=1,ptr++,i2^=1) {
		byte c, c0;
		if (b&1) {
			c0 = clr2[i1][i2];
			if (wasc||b&2) {
				c = clr1[1];
			} else c = c0;
		} else c0 = c = clr1[0];
		wasc = c0;
		c|=(c<<4);
		ptr[0]=c;
		ptr[bmp_pitch]=c;
	}
	fl |= wasc;
	vs->ainf.hgr_flags[x7][y] = fl;
	if (x<273) {
		if (lastc&&wasc) {
			byte c;
			c = clr1[1];
			c|=(c<<4);
			ptr[0]=c;
			ptr[bmp_pitch]=c;
		} else {
			byte c;
			c = lastc;
			c|=(c<<4);
			ptr[0]=c;
			ptr[bmp_pitch]=c;
		}
	}*/
}


void apaint_hgr_addr(struct VIDEO_STATE*vs, dword addr, RECT*r)
{
	if (vs->pal.cur_mono) apaint_hgr_addr_mono(vs, addr, r);
	else apaint_hgr_addr_color(vs, addr, r);
}

void apaint_apple1(struct VIDEO_STATE*vs, dword addr, RECT*r);

void (*paint_addr[])(struct VIDEO_STATE*vs, dword addr, RECT*r) =
{
	paint_lgr_addr, //0
	paint_mgr_addr, //1
	paint_t32_addr, //2
	paint_hgr_addr, //3
	paint_t64_addr_n, //4
	paint_mcgr_addr,//5
	paint_dgr_addr, //6
	apaint_t40_addr, // 7
	apaint_gr_addr, // 8
	apaint_hgr_addr, // 9
	paint_t64_addr_i, //10 inverse
	apaint_t80_addr, //11
	apaint_apple1, //12
};
