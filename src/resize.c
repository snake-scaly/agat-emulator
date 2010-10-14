#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <tchar.h>
#include "resize.h"

int resize_init(struct RESIZE_DIALOG*r)
{
	r -> hsizebox = NULL;
	return 0;
}

int resize_attach(struct RESIZE_DIALOG*r,HWND wnd)
{
	int i;
	struct RESIZE_CONTROL*rc;
	RECT wr;
	GetClientRect(wnd, &wr);
	r->_size[0]=wr.right;
	r->_size[1]=wr.bottom;
	if (GetWindowLong(wnd, GWL_STYLE) & WS_THICKFRAME && !(r->flags&RESIZE_NO_SIZEBOX)) {
		RECT r1 = wr;
		r->hsizebox = CreateWindow(TEXT("ScrollBar"), NULL, WS_CHILD | SBS_SIZEGRIP | SBS_SIZEBOXBOTTOMRIGHTALIGN,
//			0, 0, 100, 100,
			r1.left, r1.top, r1.right - r1.left, r1.bottom - r1.top,
			wnd, (HMENU)(-1), (HINSTANCE)GetWindowLong(wnd, GWL_HINSTANCE), NULL);
		if (r->hsizebox) ShowWindow(r->hsizebox, SW_SHOWNA);
	}
	for (i=0,rc=r->controls;i<r->n_controls;i++,rc++) {
		HWND ctrl;
		RECT rr;
		ctrl=GetDlgItem(wnd,rc->id);
		if (!ctrl) continue;
		GetWindowRect(ctrl,&rr);
		ScreenToClient(wnd,(LPPOINT)&rr);
		ScreenToClient(wnd,((LPPOINT)&rr)+1);
//		printf("control %i: %i,%i->%i,%i\n",rc->id,rr.left,rr.top,rr.right,rr.bottom);
		switch (rc->align[0]) {
		case RESIZE_ALIGN_LEFT: rc->params[0]=wr.right-rr.left; break;
		case RESIZE_ALIGN_RIGHT: rc->params[0]=wr.right-rr.right; break;
		case RESIZE_ALIGN_CENTER: rc->params[0]=wr.right-(rr.right+rr.left)/2; break;
		}
		switch (rc->align[1]) {
		case RESIZE_ALIGN_TOP: rc->params[1]=wr.bottom-rr.top; break;
		case RESIZE_ALIGN_BOTTOM: rc->params[1]=wr.bottom-rr.bottom; break;
		case RESIZE_ALIGN_CENTER: rc->params[1]=wr.bottom-(rr.bottom+rr.top)/2; break;
		}
	}
	return 0;
}

int resize_detach(struct RESIZE_DIALOG*r,HWND wnd)
{
	if (r->hsizebox) {
		DestroyWindow(r->hsizebox);
		r->hsizebox = NULL;
	}
	return 0;
}

int resize_free(struct RESIZE_DIALOG*r)
{
	return 0;
}

int resize_sizing(struct RESIZE_DIALOG*r,HWND wnd,int side,RECT*rw)
{
	int sz[2]={rw->right-rw->left,rw->bottom-rw->top};
	RECT rr[2]={{0,0,r->limits[0][0],r->limits[0][1]},{0,0,r->limits[1][0],r->limits[1][1]}};

	MapDialogRect(wnd,rr+0);
	MapDialogRect(wnd,rr+1);

	if (r->flags&RESIZE_LIMIT_MIN&&sz[0]<rr[0].right) sz[0]=rr[0].right;
	if (r->flags&RESIZE_LIMIT_MIN&&sz[1]<rr[0].bottom) sz[1]=rr[0].bottom;
	if (r->flags&RESIZE_LIMIT_MAX&&sz[0]>rr[1].right) sz[0]=rr[1].right;
	if (r->flags&RESIZE_LIMIT_MAX&&sz[1]>rr[1].bottom) sz[1]=rr[1].bottom;
	switch (side) {
	case WMSZ_LEFT: case WMSZ_BOTTOMLEFT: case WMSZ_TOPLEFT:
		rw->left=rw->right-sz[0];
		break;
	case WMSZ_RIGHT: case WMSZ_TOPRIGHT: case WMSZ_BOTTOMRIGHT:
		rw->right=rw->left+sz[0];
		break;
	}
	switch (side) {
	case WMSZ_BOTTOM: case WMSZ_BOTTOMLEFT: case WMSZ_BOTTOMRIGHT:
		rw->bottom=rw->top+sz[1];
		break;
	case WMSZ_TOP: case WMSZ_TOPLEFT: case WMSZ_TOPRIGHT:
		rw->top=rw->bottom-sz[1];
		break;
	}
	return 0;
}

int resize_realign(struct RESIZE_DIALOG*r,HWND wnd,int neww,int newh)
{
	int i;
	struct RESIZE_CONTROL*rc;
	if (r->hsizebox) {
		WINDOWPLACEMENT plc;
		plc.length = sizeof(plc);
		if (GetWindowPlacement(wnd, &plc)) {
			ShowWindow(r->hsizebox, (plc.showCmd != SW_SHOWMAXIMIZED));
		}
	}
	if (r->flags&RESIZE_NO_REALIGN) return 1;
	for (i=0,rc=r->controls;i<r->n_controls;i++,rc++) {
		HWND ctrl;
		RECT rr;
		int d;
		ctrl=GetDlgItem(wnd,rc->id);
		if (!ctrl) continue;
//		if (!IsWindowVisible(ctrl)) continue;
		GetWindowRect(ctrl,&rr);
		ScreenToClient(wnd,(LPPOINT)&rr);
		ScreenToClient(wnd,((LPPOINT)&rr)+1);
		switch (rc->align[0]) {
		case RESIZE_ALIGN_LEFT: 
			d=rr.left-neww+rc->params[0];
			rr.left-=d;
			rr.right-=d;
			break;
		case RESIZE_ALIGN_RIGHT:
			rr.right=neww-rc->params[0];
			break;
		case RESIZE_ALIGN_CENTER:
			d=((rr.left+rr.right)/2-(neww+r->_size[0])/2+rc->params[0]);
			rr.left-=d;
			rr.right-=d;
			break;
		}
		switch (rc->align[1]) {
		case RESIZE_ALIGN_TOP:
			d=rr.top-newh+rc->params[1];
			rr.top-=d;
			rr.bottom-=d;
			break;
		case RESIZE_ALIGN_BOTTOM:
			rr.bottom=newh-rc->params[1];
			break;
		case RESIZE_ALIGN_CENTER:
			d=((rr.top+rr.bottom)/2-(newh+r->_size[1])/2+rc->params[1]);
			rr.top-=d;
			rr.bottom-=d;
			break;
		}
		MoveWindow(ctrl,rr.left,rr.top,rr.right-rr.left,rr.bottom-rr.top,TRUE);
	}
	if (r->hsizebox) {
		int ww = 30, wh = 30;
		MoveWindow(r->hsizebox, neww - ww,  newh - wh, ww, wh, TRUE);
	}
	return 0;
}

int resize_attach_control(struct RESIZE_DIALOG*r,HWND wnd,int id)
{
	int i;
	struct RESIZE_CONTROL*rc;
	RECT wr;
	GetClientRect(wnd,&wr);
	for (i=0,rc=r->controls;i<r->n_controls;i++,rc++) {
		HWND ctrl;
		RECT rr;
		if (rc->id!=id) continue;
		ctrl=GetDlgItem(wnd,rc->id);
		if (!ctrl) continue;
		GetWindowRect(ctrl,&rr);
		ScreenToClient(wnd,(LPPOINT)&rr);
		ScreenToClient(wnd,((LPPOINT)&rr)+1);
//		printf("control %i: %i,%i->%i,%i\n",rc->id,rr.left,rr.top,rr.right,rr.bottom);
		switch (rc->align[0]) {
		case RESIZE_ALIGN_LEFT: rc->params[0]=wr.right-rr.left; break;
		case RESIZE_ALIGN_RIGHT: rc->params[0]=wr.right-rr.right; break;
		}
		switch (rc->align[1]) {
		case RESIZE_ALIGN_TOP: rc->params[1]=wr.bottom-rr.top; break;
		case RESIZE_ALIGN_BOTTOM: rc->params[1]=wr.bottom-rr.bottom; break;
		}
	}
	return 0;
}

static LPTSTR cfg = NULL;

int resize_set_cfgname(LPCTSTR name)
{
	if (cfg) free(cfg);
	cfg = name?_tcsdup(name):NULL;
	return 0;
}

int resize_init_placement(HWND wnd, LPCTSTR id)
{
	WINDOWPLACEMENT place, *pl = &place;
	static TCHAR sid[10];
	if (!id) id = (LPCTSTR)GetWindowLong(wnd, GWL_ID);
	if (IS_INTRESOURCE(id)) {
		wsprintf(sid, TEXT("#%i"), id);
		id = sid;
	}
	if (cfg) {
		if (!GetPrivateProfileStruct(TEXT("placement"), id, pl, sizeof(*pl), cfg)) 
			return -3;
	} else return -1;
	if (pl->length) SetWindowPlacement(wnd, pl);
	else return -2;
	return 0;
}

int resize_save_placement(HWND wnd, LPCTSTR id)
{
	WINDOWPLACEMENT place, *pl = &place;
	static TCHAR sid[10];
	if (!id) id = (LPCTSTR)GetWindowLong(wnd, GWL_ID);
	if (IS_INTRESOURCE(id)) {
		wsprintf(sid, TEXT("#%i"), id);
		id = sid;
	}
	pl->length = sizeof(*pl);
	GetWindowPlacement(wnd, pl);
	if (cfg) {
		if (!WritePrivateProfileStruct(TEXT("placement"), id, pl, sizeof(*pl), cfg)) 
			return -3;
	} else return -1;
	return 0;
}



void resize_savelistviewcolumns(HWND hlist, LPCTSTR param)
{
	int i;
	int ww[256];
	if (!cfg) return;
	for (i=0;i<256;i++) {
		int n;
		n=ListView_GetColumnWidth(hlist,i);
		if (!n) break;
		ww[i]=n;
	}
	WritePrivateProfileStruct(TEXT("columns"),param,ww,i*sizeof(ww[0]),cfg);
}


void resize_loadlistviewcolumns(HWND hlist, LPCTSTR param)
{
	int i,j;
	int ww[256];
	if (!cfg) return;
	ZeroMemory(ww,sizeof(ww));
	for (i=0;i<256;i++) {
		int n;
		n=ListView_GetColumnWidth(hlist,i);
		if (!n) break;
	}
	GetPrivateProfileStruct(TEXT("columns"),param,ww,i*sizeof(ww[0]),cfg);
	for (j=0;j<i;j++) {
		if (!ww[j]) break;
		ListView_SetColumnWidth(hlist,j,ww[j]);
	}
}

