#include "console.h"
#include <windows.h>
#ifndef UNDER_CE
#include <stdio.h>
#endif //UNDER_CE
#include <tchar.h>
#include <stdlib.h>
#include <stdarg.h>

#define CONSOLE_DEF_FONT TEXT("")
#define CONSOLE_QUEUE_LENGTH 16

#define CONSOLE_DEFAULT_FLAGS (CONFL_CRLF|CONFL_ECHO|CONFL_KILL|CONFL_CONFIRM)

#define CONSOLE_WINDOW_CLASS TEXT("Console")

#define CDATA_WRITE 0
#define CWM_CLEAR (WM_USER)
#define CWM_TERM  (WM_USER+1)

#define CON_WAIT_TIMEOUT 1000


struct S_CONSOLE
{
	HWND hcon;
	HFONT font;
	con_char_t title[MAX_PATH];
	HANDLE thread;
	HANDLE access_sem;
	HANDLE read_sem;
	HBRUSH fill_brush;
	con_char_t queue[CONSOLE_QUEUE_LENGTH];
	int read_pos, write_pos;
	COLORREF colors[2];
	con_char_t*chars;
	int*lens;
	POINT cur_pos;
	SIZE  con_size;
	SIZE  char_size;
	SIZE  cur_size;
	unsigned flags;
	int active;
	int tab_size;
	int err;
};


#define con_alloc(sz) ((sz)?malloc(sz):NULL)
#define con_free(p) if (p) free(p)

static DWORD WINAPI thread_proc(CONSOLE*con);
static LRESULT CALLBACK wnd_proc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp);

static int init_console_params(CONSOLE*con)
{
	HDC dc;
	int wc, hc;
	dc = GetDC(con->hcon);
	wc = con->char_size.cx;
	hc = con->char_size.cy;
	{
		LOGFONT f={hc,wc,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,
			OUT_RASTER_PRECIS,CLIP_DEFAULT_PRECIS,DRAFT_QUALITY,
			FIXED_PITCH,CONSOLE_DEF_FONT};
		SIZE cs;
		con->font=CreateFontIndirect(&f);
		if (!con->font) {
			ReleaseDC(con->hcon, dc);
			return -1;
		}
		SelectObject(dc, con->font);
		GetTextExtentPoint32(dc, TEXT("W"), 1, &cs);
		wc = cs.cx;
		hc = cs.cy;

	}
	ReleaseDC(con->hcon, dc);
	con->char_size.cx = wc;
	con->char_size.cy = hc;
//	printf("char size  %ix%i\n", con->char_size.cx, con->char_size.cy);
	{ // adjust window size
		RECT r1 = {0, 0, wc * con->con_size.cx, hc * con->con_size.cy}, r2;
		AdjustWindowRectEx(&r1, GetWindowLong(con->hcon, GWL_STYLE),
			FALSE, GetWindowLong(con->hcon, GWL_EXSTYLE));
		GetWindowRect(con->hcon, &r2);
		MoveWindow(con->hcon, r2.left, r2.top, r1.right-r1.left, r1.bottom-r1.top, TRUE);
	}
	
	con->cur_size.cx=con->char_size.cx;
	con->cur_size.cy=2;
	con->colors[0]=RGB(0,0,0);
	con->colors[1]=RGB(128,128,128);
	con->tab_size=8;
	con->chars=con_alloc(con->con_size.cx*con->con_size.cy*sizeof(con->chars[0]));
	if (!con->chars) {
		return -1;
	}
	con->read_pos=con->write_pos=0;
	con->access_sem=CreateMutex(NULL,FALSE,NULL);
	con->lens=con_alloc(con->con_size.cy*sizeof(con->lens[0]));
	if (!con->lens) {
		con_free(con->chars);
		con->chars = NULL;
		return -1;
	}
	memset(con->lens,0,con->con_size.cy*sizeof(con->lens[0]));
	con->fill_brush=CreateSolidBrush(con->colors[0]);
	return 0;
}

CONSOLE*console_create(int w,int h,con_char_t*title)
{
	CONSOLE*con;
	DWORD t;
	con=con_alloc(sizeof(*con));
	if (!con) return NULL;
	ZeroMemory(con, sizeof(*con));
	con->flags=CONSOLE_DEFAULT_FLAGS;
	con->char_size.cx = 10;
	con->char_size.cy = 20;
	if (!w || !h) {
		RECT r;
		GetClientRect(GetDesktopWindow(), &r);
		if (!w) w = r.right / con->char_size.cx - 2;
		if (!h) h = r.bottom / con->char_size.cy - 5;
	}
	con->con_size.cx=w;
	con->con_size.cy=h;
	con->cur_pos.x=0;
	con->cur_pos.y=0;
	if (title) _tcsncpy(con->title,title,sizeof(con->title)/sizeof(con->title[0]));
	else _tcscpy(con->title,TEXT("Untitled Console"));
	con->read_sem=CreateEvent(NULL,FALSE,FALSE,NULL);
	con->hcon=0;
	con->thread=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)thread_proc,con,0,&t);
	if ((WaitForSingleObject(con->read_sem, CON_WAIT_TIMEOUT) == WAIT_TIMEOUT) ||
		con->err) {
		con_free(con);
		return NULL;
	}
	return con;
}

static int console_read_char(CONSOLE*con);

void console_free(CONSOLE*con)
{
	if (!con) return;
	if (con->flags&CONFL_WAITCLOSE) {
		console_puts(con,TEXT("Press any key to exit"));
		console_read_char(con);
	}
	PostMessage(con->hcon,CWM_TERM,0,0);
	if (con->thread) {
		if (WaitForSingleObject(con->thread,CON_WAIT_TIMEOUT) == WAIT_TIMEOUT)
			TerminateThread(con->thread,1);
		CloseHandle(con->thread);
	}
	if (con->access_sem) CloseHandle(con->access_sem);
	if (con->read_sem) CloseHandle(con->read_sem);
	if (con->fill_brush) DeleteObject(con->fill_brush);
	if (con->font) DeleteObject(con->font);
	con_free(con->lens);
	con_free(con->chars);
	con_free(con);
}

int console_write(CONSOLE*con,const con_char_t*data,int len)
{
	COPYDATASTRUCT cds;
	if (!con) return -1;
	cds.dwData=CDATA_WRITE;
	cds.cbData=len;
	cds.lpData=(void*)data;
	return SendMessage(con->hcon,WM_COPYDATA,0,(LPARAM)&cds);
}


static int console_read_char(CONSOLE*con)
{
	int r;
	if (console_eof(con)) return -1;
//	printf("read %i write %i\n",con->read_pos,con->write_pos);
	if (con->read_pos==con->write_pos) {
		WaitForSingleObject(con->read_sem,INFINITE);
		if (console_eof(con)) return -1;
	}
	WaitForSingleObject(con->access_sem,INFINITE);
	r=con->queue[con->read_pos];
	con->read_pos++;
	if (con->read_pos==CONSOLE_QUEUE_LENGTH) con->read_pos=0;
	ReleaseMutex(con->access_sem);
	return r;
}

int console_read(CONSOLE*con,con_char_t*data,int len)
{
	int l=0;
	if (!con) return -1;
	for (;len;len-=sizeof(data[0]),data++,l+=sizeof(data[0])) {
		int r=console_read_char(con);
		if (r<0) break;
		data[0]=r;
	}
	return l;
}

int console_setflags(CONSOLE*con,unsigned flags)
{
	if (console_eof(con)) return -1;
	con->flags=flags;
	return 0;
}

int console_getflags(CONSOLE*con,unsigned*flags)
{
	if (console_eof(con)) return -1;
	*flags=con->flags;
	return 0;
}

int console_clear_output(CONSOLE*con)
{
	if (console_eof(con)) return -1;
	return SendMessage(con->hcon,CWM_CLEAR,0,0);
}

int console_clear_input(CONSOLE*con)
{
	if (console_eof(con)) return -1;
	WaitForSingleObject(con->access_sem,INFINITE);
	con->read_pos=con->write_pos=0;
	ResetEvent(con->read_sem);
	ReleaseMutex(con->access_sem);
	return 0;
}

int console_get_title(CONSOLE*con,con_char_t*buf,int nchars)
{
	if (console_eof(con)) return -1;
	return SendMessage(con->hcon,WM_GETTEXT,nchars,(LPARAM)buf);
}

int console_set_title(CONSOLE*con,con_char_t*buf)
{
	if (!buf||console_eof(con)) return -1;
	_tcsncpy(con->title,buf,sizeof(con->title)/sizeof(con->title[0]));
	return SendMessage(con->hcon,WM_SETTEXT,0,(LPARAM)con->title);
}

static DWORD WINAPI thread_proc(CONSOLE*con)
{
	WNDCLASS cl;
	DWORD st=WS_BORDER|WS_CAPTION|WS_OVERLAPPED|WS_SYSMENU, 
		st_ex=0;
	RECT r;
	MSG msg;
	memset(&cl,0,sizeof(cl));
	cl.style=CS_HREDRAW|CS_VREDRAW;
	cl.lpfnWndProc=wnd_proc;
	cl.lpszClassName=CONSOLE_WINDOW_CLASS;
	RegisterClass(&cl);
	r.left=0;
	r.top=0;
	r.right=con->con_size.cx*con->char_size.cx;
	r.bottom=con->con_size.cy*con->char_size.cy;
	AdjustWindowRectEx(&r,st,FALSE,st_ex);
	con->active=0;
	con->hcon=CreateWindowEx(st_ex,CONSOLE_WINDOW_CLASS,con->title,st,
		CW_USEDEFAULT,CW_USEDEFAULT,
//		CW_USEDEFAULT,CW_USEDEFAULT,
		r.right-r.left,r.bottom-r.top,
		FALSE,FALSE,NULL,con);
	if (!con->hcon) {
		con->err = 1;
		goto err;
	}
	if (init_console_params(con) < 0) {
		con->err = 2;
		goto err;
	}
	ShowWindow(con->hcon,SW_SHOW);
	SetEvent(con->read_sem);
	while (GetMessage(&msg,NULL,0,0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
err:
	if (con->hcon) DestroyWindow(con->hcon);
	con->hcon=0;
	SetEvent(con->read_sem);
	return 0;
}

static void paint_con(HDC dc,CONSOLE*con,RECT*r)
{
	int line, nlines, pos, nchars;
	int y=r->top, x=r->left;
	con_char_t*pc;
	line=y/con->char_size.cy;
	y=line*con->char_size.cy;
	nlines=(r->bottom+con->char_size.cy-1)/con->char_size.cy-line;
	pos=x/con->char_size.cx;
	x=pos*con->char_size.cx;
	pc=con->chars+line*con->con_size.cx+pos;
	nchars=(r->right+con->char_size.cx-1)/con->char_size.cx-pos;
	SelectObject(dc,con->font);
	SetBkColor(dc,con->colors[0]);
	SetTextColor(dc,con->colors[1]);
	SetBkMode(dc,OPAQUE);
	for (;nlines;nlines--,line++,y+=con->char_size.cy,pc+=con->con_size.cx) {
		int nc=nchars;
		if (pos+nchars>con->lens[line]) {
			SIZE sz;
			RECT t={0,y,r->right,y+con->char_size.cy};
			nc=con->lens[line]-pos;
			GetTextExtentPoint32(dc,pc,nc,&sz);
			t.left = sz.cx + x;
			FillRect(dc,&t,con->fill_brush);
		}
		if (pos>con->lens[line]) continue;
		ExtTextOut(dc,x,y,0,NULL,pc,nc,NULL);
	}
}


static void upd_cursor(CONSOLE*con)
{
	if (con->active) 
	{	
		SetCaretPos(con->cur_pos.x*con->char_size.cx,
			con->cur_pos.y*con->char_size.cy+con->char_size.cy-con->cur_size.cy);
	}
}

static void fill_str(con_char_t*p,con_char_t c,int l)
{
	for (;l;l--,p++) *p=c;
}

static void write_raw_char(CONSOLE*con,con_char_t ch,RECT*r)
{
	RECT r1;
	int ll;
	con_char_t*pc;
	pc=con->chars+con->cur_pos.y*con->con_size.cx;
	ll=con->lens[con->cur_pos.y];
	if (con->cur_pos.x>=ll) {
		if (con->cur_pos.x>ll) {
			fill_str(pc+ll,TEXT(' '),con->cur_pos.x-ll);
		}
		con->lens[con->cur_pos.y]=con->cur_pos.x+1;
	} else if (pc[con->cur_pos.x]==ch) return;
	pc[con->cur_pos.x]=ch;
	r1.left=con->cur_pos.x*con->char_size.cx;
	r1.top=con->cur_pos.y*con->char_size.cy;
	r1.right=r1.left+con->char_size.cx;
	r1.bottom=r1.top+con->char_size.cy;
	UnionRect(r,r,&r1);
}


static int max_width(CONSOLE*con)
{
	int r=0;
	int*pl=con->lens;
	int i;
	for (i=con->con_size.cy;i;i--,pl++) if (r<*pl) r=*pl;
	return r;
}


static void cursor_down(CONSOLE*con,RECT*r)
{
	con->cur_pos.y++;
	if (con->cur_pos.y==con->con_size.cy) {
		int mw=con->con_size.cx;//max_width(con);
		con->cur_pos.y--;
		memmove(con->lens,con->lens+1,(con->con_size.cy-1)*sizeof(con->lens[0]));
		memmove(con->chars,con->chars+con->con_size.cx,(con->con_size.cy-1)*con->con_size.cx*sizeof(con->chars[0]));
		con->lens[con->con_size.cy-1]=0;
/*		r->left=r->top=0;
		r->right=mw*con->char_size.cx;
		r->bottom=con->con_size.cy*con->char_size.cy;*/
		{
			HDC dc;
			dc = GetDC(con->hcon);
			if (!dc) {
				r->left=r->top=0;
				r->right=mw*con->char_size.cx;
				r->bottom=con->con_size.cy*con->char_size.cy;
			} else {
				RECT r1, r2;
				HideCaret(con->hcon);
				ScrollDC(dc, 0, -con->char_size.cy, NULL, NULL, NULL, &r2);
				OffsetRect(r, 0, -con->char_size.cy);
				r1.left = 0;
				r1.right = mw*con->char_size.cx;
				r1.bottom = con->con_size.cy*con->char_size.cy;
				r1.top = r1.bottom - con->char_size.cy;
				FillRect(dc,&r1,con->fill_brush);
				ReleaseDC(con->hcon, dc);
				ShowCaret(con->hcon);
				UnionRect(r,r,&r2);
			}
		}
	}
}

static void cursor_right(CONSOLE*con,RECT*r)
{
	con->cur_pos.x++;
	if (con->cur_pos.x==con->con_size.cx) {
		con->cur_pos.x=0;
		cursor_down(con,r);
	}
}


static void internal_console_clear(CONSOLE*con)
{
	memset(con->lens,0,con->con_size.cy*sizeof(con->lens[0]));
	InvalidateRect(con->hcon,NULL,FALSE);
}


static void cursor_left(CONSOLE*con,RECT*r)
{
	if (con->cur_pos.x) {
		con->cur_pos.x--;
	} else {
		con->cur_pos.x=con->con_size.cx-1;
		if (con->cur_pos.y) con->cur_pos.y--;
	}
}

static void write_tab(CONSOLE*con,RECT*r)
{
	int i;
	int newpos=((con->cur_pos.x+con->tab_size)/con->tab_size)*con->tab_size;
	for (i=con->cur_pos.x;i<newpos;i++) {
		write_raw_char(con,TEXT(' '),r);
		cursor_right(con,r);
	}
}

static void clear_screen(CONSOLE*con,RECT*r)
{
	int i;
	con->cur_pos.x = 0;
	con->cur_pos.y = 0;
	internal_console_clear(con);
}

static void write_char(CONSOLE*con,con_char_t ch,RECT*r)
{
	switch (ch) {
	case TEXT('\n'):
		if (con->flags&CONFL_CRLF) {
			if (con->lens[con->cur_pos.y]>con->cur_pos.x) {
//				con->lens[con->cur_pos.y]=con->cur_pos.x;
			}
			con->cur_pos.x=0;
		}
		cursor_down(con,r);
		break;
	case TEXT('\r'):
		con->cur_pos.x=0;
//		if (con->flags&CONFL_CRLF) cursor_down(con,r);
		break;
	case TEXT('\a'):
		MessageBeep(0);
		break;
	case TEXT('\b'):
		cursor_left(con,r);
		break;
	case TEXT('\t'):
		write_tab(con,r);
		break;
	case TEXT('\f'):	
		clear_screen(con,r);
		break;
	default:
		write_raw_char(con,ch,r);
		cursor_right(con,r);
	}
}

static void on_char(CONSOLE*con,con_char_t c)
{
	int np;
	WaitForSingleObject(con->access_sem,INFINITE);
	np=con->write_pos+1;
	if (np==CONSOLE_QUEUE_LENGTH) np=0;
	if (np==con->read_pos) {
		MessageBeep(0);
		ReleaseMutex(con->access_sem);
		return;
	}
	con->queue[con->write_pos]=c;
	if (con->write_pos==con->read_pos) SetEvent(con->read_sem);
	con->write_pos=np;
	ReleaseMutex(con->access_sem);

	if (con->flags&CONFL_ECHO) {
		RECT r={0,0,0,0};
		write_char(con,c,&r);
		upd_cursor(con);
		InvalidateRect(con->hcon,&r,FALSE);
	}
}


static int write_buf(CONSOLE*con,con_char_t*c,int l)
{
	RECT r={0,0,0,0};
	int res=0;
	for (;l;l-=sizeof(c[0]),c++,res+=sizeof(c[0])) write_char(con,c[0],&r);
	upd_cursor(con);
	InvalidateRect(con->hcon,&r,FALSE);
	UpdateWindow(con->hcon);
//	printf("write_buf: %i, %i",con->cur_pos.x,con->cur_pos.y);
	return res;
}


static void console_close(CONSOLE*con)
{
	if (con->flags&CONFL_KILL) {
		if (con->flags&CONFL_CONFIRM) {
			if (MessageBox(con->hcon,TEXT("Terminate Process?"),TEXT("Console Request"),MB_ICONQUESTION|MB_YESNO)!=IDYES) {
				return;
			}
		}
//		TerminateProcess(GetCurrentProcess(),100);
	}
	PostQuitMessage(0);
}

static LRESULT CALLBACK wnd_proc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	CONSOLE*con=(CONSOLE*)GetWindowLong(wnd,GWL_USERDATA);
	switch (msg) {
	case WM_CREATE:
		{
			CREATESTRUCT*cs=(CREATESTRUCT*)lp;
			con=cs->lpCreateParams;
			SetWindowLong(wnd,GWL_USERDATA,(LONG)con);
		}
		break;
	case WM_CHAR:
		if (wp==TEXT('\r')) wp=TEXT('\n');
		on_char(con,wp);
		break;
	case WM_CLOSE:
		console_close(con);
		return 0;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			RECT r;
			HDC dc;
			GetClientRect(wnd,&r);
			dc=BeginPaint(wnd,&ps);
			paint_con(dc,con,&r);
//			paint_con(dc,con,&ps.rcPaint);
			EndPaint(wnd,&ps);
		}
		return 0;
	case WM_SETFOCUS:
		con->active=1;
		CreateCaret(wnd,NULL,con->cur_size.cx,con->cur_size.cy);
		upd_cursor(con);
		ShowCaret(wnd);
		break;
	case WM_KILLFOCUS:
		con->active=0;
		HideCaret(wnd);
		break;
	case WM_COPYDATA:
		{
			COPYDATASTRUCT*cds=(COPYDATASTRUCT*)lp;
			switch (cds->dwData) {
			case CDATA_WRITE:
				return write_buf(con,cds->lpData,cds->cbData);
			}
			return -1;
		}
	case CWM_CLEAR:
		internal_console_clear(con);
		return 0;
	case CWM_TERM:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(wnd,msg,wp,lp);
}


int console_gets(CONSOLE*con,con_char_t*buf,int maxlen)
{
	con_char_t c;
	unsigned last_flags;
	int r=0;
	console_getflags(con,&last_flags);
	console_setflags(con,last_flags&~CONFL_ECHO); // turn off echo mode
	while (!console_eof(con)) {
		int n=console_read(con,&c,sizeof(c));
		if (n!=sizeof(c)) break;
		if (c==TEXT('\n')) break;
		if (c=='\b') {
			if (r) { 
				buf--;
				maxlen++;
				r--;
				console_puts(con,TEXT("\b \b"));
			}
			else MessageBeep(0);
			continue;
		}
		if (c<TEXT(' ')) continue;
		if (maxlen<=1) {
			MessageBeep(0);
			continue;
		}
		*buf=c;
		buf++;
		r++;
		maxlen--;
		console_puts_len(con,&c,1);
	}
	console_puts(con,TEXT("\r\n"));
	console_setflags(con,last_flags);
	*buf=0;
	return r;
}

int console_puts(CONSOLE*con,con_char_t*buf)
{
	return console_write(con,buf,_tcslen(buf)*sizeof(buf[0]))/sizeof(buf[0]);
}

int console_puts_len(CONSOLE*con,con_char_t*buf,int len)
{
	return console_write(con,buf,len*sizeof(buf[0]))/sizeof(buf[0]);
}

int console_eof(CONSOLE*con)
{
	return !con||!con->hcon||!IsWindow(con->hcon);
}


int console_printf(CONSOLE*con,con_char_t*fmt,...)
{
	con_char_t buf[MAX_PATH];
	int r;
	va_list l;
	va_start(l,fmt);
//	r=_vsntprintf(buf,MAX_PATH,fmt,l);
	r=wvsprintf(buf,fmt,l);
	va_end(l);
	console_puts(con,buf);
	return r;
}

