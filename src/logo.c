#include "common.h"
#include "logo.h"
#include <stdio.h>

static HBITMAP bmp, lbmp;
static HDC bmpdc;
static DWORD w,h, pitch, sz;
static LPBYTE bmpdata;
static int start_ticks=2, fin_ticks=20, end_ticks=80, delta_tick=30, cur_tick=0, simulate=0;

static BOOL (WINAPI *AlphaBlendPtr)( IN HDC, IN int, IN int, IN int, IN int, IN HDC, IN int, IN int, IN int, IN int, IN BLENDFUNCTION);


static int simulate_aplha(HDC dest)
{
	HBITMAP bmpbuf, lbmpbuf;
	LPBYTE p, p1, tbuf;
	int x, y;
	int res=0;
	HDC bmpbufdc;
	BITMAPINFO bi;
	bmpbufdc=CreateCompatibleDC(dest);
	if (!bmpbufdc) { res=-1; goto l0; }
	bmpbuf=CreateCompatibleBitmap(dest,w,h);
	if (!bmpbuf) { res=-2; goto l1; }
	lbmpbuf=SelectObject(bmpbufdc,bmpbuf);
	BitBlt(bmpbufdc,0,0,w,h,dest,0,0,SRCCOPY);
	tbuf=malloc(w*h*3);
	if (!tbuf) { res=-3; goto l2; }
	ZeroMemory(&bi,sizeof(bi));
	bi.bmiHeader.biSize=sizeof(bi.bmiHeader);
	bi.bmiHeader.biWidth=w;
	bi.bmiHeader.biHeight=-h;
	bi.bmiHeader.biPlanes=1;
	bi.bmiHeader.biBitCount=24;
	if (!GetDIBits(bmpbufdc,bmpbuf,0,h,tbuf,&bi,0)) { res=-4; goto l3; }
	for (p=bmpdata,p1=tbuf,y=0;y<h;y++) {
		for (x=0;x<w;x++,p+=4,p1+=3) {
			double al=p[3]/255.0;
			int r, g, b;
			r=p[0]*al+p1[0]*(1-al);
			g=p[1]*al+p1[1]*(1-al);
			b=p[2]*al+p1[2]*(1-al);
			if (r>255) r=255;
			if (g>255) g=255;
			if (b>255) b=255;
			p1[0]=r;
			p1[1]=g;
			p1[2]=b;
		}
	}
	SetDIBits(bmpbufdc,bmpbuf,0,h,tbuf,&bi,0);
	BitBlt(dest,0,0,w,h,bmpbufdc,0,0,SRCCOPY);
l3:
	free(tbuf);
l2:
	SelectObject(bmpbufdc,lbmpbuf);
	DeleteObject(bmpbuf);
l1:
	DeleteDC(bmpbufdc);
l0:
	return res;
}


static LRESULT CALLBACK LogoProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch (msg) {
	case WM_CREATE:
		SetTimer(wnd,1,delta_tick,NULL);
		cur_tick=0;
		break;
	case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC dc;
			BLENDFUNCTION bf;
			dc=BeginPaint(wnd,&ps);
			bf.BlendOp=AC_SRC_OVER;
			bf.BlendFlags=0;
			bf.SourceConstantAlpha=0x30;
			bf.AlphaFormat=AC_SRC_ALPHA;
			if (!AlphaBlendPtr||!AlphaBlendPtr(dc,0,0,w,h,bmpdc,0,0,w,h,bf)) {
				simulate=1;
				if (simulate_aplha(dc)<0) DestroyWindow(wnd);
			}
			EndPaint(wnd,&ps);
		}
		return 0;
	case WM_TIMER:
		cur_tick++;
		if (cur_tick==start_ticks) ShowWindow(wnd,SW_SHOWNA);
		else if (cur_tick==end_ticks) DestroyWindow(wnd);
		else if (cur_tick>start_ticks&&cur_tick<fin_ticks&&!simulate) InvalidateRect(wnd,NULL,FALSE);
		break;
	case WM_LBUTTONDOWN:
		DestroyWindow(wnd);
		break;
	case WM_DESTROY:
		free(bmpdata);
		SelectObject(bmpdc,lbmp);
		DeleteObject(bmp);
		DeleteDC(bmpdc);
		break;
	}
	return DefWindowProc(wnd,msg,wp,lp);
}

HWND CreateLogoWindow(HWND parent)
{
	HWND wnd;
	WNDCLASS cl;
	ATOM at;
	FILE*in;
	DWORD ofbits;
	int x, y;
	in=fopen("logo.bmp","rb");
	if (!in) return NULL;
	fseek(in,0x0A,SEEK_SET);
	fread(&ofbits,1,4,in);
	fseek(in,0x12,SEEK_SET);
	fread(&w,1,4,in);
	fread(&h,1,4,in);
	pitch=w*4;
	sz=h*pitch;
	bmpdata=malloc(sz);
	if (!bmpdata) return NULL;
	fseek(in,ofbits,SEEK_SET);
	{
		unsigned char*p=(unsigned char*)bmpdata+pitch*(h-1), *p1;
		int i=0, j;
		for (;i<h;i++,p-=pitch) {
			fread(p,1,pitch,in);
//			for (x=0,p1=p;x<w;x++,p1+=4) p1[3]=255-p1[3];
		}
		
	}
	fclose(in);
//	bmp=LoadImage(NULL,TEXT("logo.bmp"),IMAGE_BITMAP,0,0,LR_LOADFROMFILE);
	bmp=CreateBitmap(w,h,1,32,bmpdata);
	if (!bmp) return NULL;
	bmpdc=CreateCompatibleDC(NULL);
	if (!bmpdc) return NULL;
	lbmp=SelectObject(bmpdc,bmp);
	ZeroMemory(&cl,sizeof(cl));
	cl.style=0;
	cl.lpfnWndProc=LogoProc;
	cl.lpszClassName=TEXT("Logo");
	at=RegisterClass(&cl);
	if (!at) return NULL;
	{
		RECT r;
		GetWindowRect(GetDesktopWindow(),&r);
		x=(r.right-w)>>1;
		y=(r.bottom-h)>>1;
	}
	{
		HMODULE lib;
		lib=LoadLibrary(TEXT("MSIMG32.DLL"));
		if (lib) {
			AlphaBlendPtr=(void*)GetProcAddress(lib,"AlphaBlend");
			if (!AlphaBlendPtr) FreeLibrary(lib);
		}
	}
	wnd=CreateWindowEx(WS_EX_TOPMOST,(LPCTSTR)at,NULL,WS_POPUP,x,y,w,h,parent,NULL,NULL,NULL);
	return wnd;
}
