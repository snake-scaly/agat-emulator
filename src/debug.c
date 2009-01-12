/*
	Agat Emulator version 1.0
	Copyright (c) NOP, nnop@newmail.ru
	debug - output of debug information
*/

#include <stdarg.h>
#include "syslib.h"
#include "types.h"
#include "debug.h"
#include "streams.h"


//#define LOG_NAME TEXT("log.txt")
#define BUF_SIZE 256

OSTREAM*hlog;
int start_ticks;

#ifdef CONSOLE
OSTREAM*hstdout;
#endif //CONSOLE


static const char_t* dd_errmsg(int code);
static const char_t* ds_errmsg(int code);
static const char_t* acm_errmsg(int code);
static const char_t* ic_errmsg(int code);


#include <stdio.h>
void debug_init()
{
	setbuf(stdout, NULL);
#ifdef LOG_NAME
	hlog=osfopen(LOG_NAME);
#endif
#ifdef UNICODE
	if (hlog) {
		char_t b=0xFEFF;
		oswrite(hlog,&b,sizeof(b));
	}
#endif
#ifdef CONSOLE
	hstdout=osopenstdout();
#endif //CONSOLE
	start_ticks=get_n_msec();
}

void debug_term()
{
#ifdef CONSOLE
	osclose(hstdout);
#endif //CONSOLE
	if (hlog) osclose(hlog);
}

void logprint(int lev,const char_t*fmt,...)
{
	char_t buf[BUF_SIZE], buf1[BUF_SIZE];
	va_list l;
	va_start(l,fmt);
	xvsprintf(buf,fmt,l);
	va_end(l);
	xsprintf(buf1,TEXT("log[%8i]: %s\n"),get_n_msec()-start_ticks,buf);
	fprintf(stderr,"log: %s",buf1);
	if (hlog) oswrite(hlog,buf1,s_len(buf1)*sizeof(buf1[0]));
#ifdef CONSOLE
	oswrite(hstdout,buf1,s_len(buf1));
#endif//CONSOLE
}

void errprint(const char_t*fmt,...)
{
	char_t buf[BUF_SIZE], buf1[BUF_SIZE];
	va_list l;
	va_start(l,fmt);
	xvsprintf(buf,fmt,l);
	va_end(l);
	xsprintf(buf1,TEXT("error[%8i]: %s\n"),get_n_msec()-start_ticks,buf);
	fprintf(stderr,"err: %s",buf1);
	if (hlog) oswrite(hlog,buf1,s_len(buf1)*sizeof(buf1[0]));
#ifdef CONSOLE
	oswrite(hstdout,buf1,s_len(buf1));
#endif//CONSOLE
//	MessageBox(base_wnd,buf,"Error",MB_OK|MB_ICONEXCLAMATION);
}

int DD_FAILED(int code,const char_t*fmt,...)
{
	char_t buf[BUF_SIZE];
	va_list l;
	if (!FAILED(code)) return 0;
	va_start(l,fmt);
	xvsprintf(buf,fmt,l);
	errprint(0,TEXT("DirectDraw error while %s: %s (code %i)"),buf,dd_errmsg(code),code);
	return 1;
}


int DS_FAILED(int code,const char_t*fmt,...)
{
	char_t buf[BUF_SIZE];
	va_list l;
	if (!FAILED(code)) return 0;
	va_start(l,fmt);
	xvsprintf(buf,fmt,l);
	errprint(TEXT("DirectSound error while %s: %s (code %i)"),buf,ds_errmsg(code),code);
	return 1;
}

int ACM_FAILED(int code,const char_t*fmt,...)
{
	char_t buf[BUF_SIZE];
	va_list l;
	if (!code) return 0;
	va_start(l,fmt);
	xvsprintf(buf,fmt,l);
	errprint(TEXT("ACM error while %s: %s (code %i)"),buf,acm_errmsg(code),code);
	return 1;
}

int IC_FAILED(int code,const char_t*fmt,...)
{
	char_t buf[BUF_SIZE];
	va_list l;
	if (code==0) return 0;
	va_start(l,fmt);
	xvsprintf(buf,fmt,l);
	errprint(TEXT("IC error while %s: %s (code %i)"),buf,acm_errmsg(code),code);
	return 1;
}

int WIN_ERROR(int err,const char_t*fmt,...)
{
	char_t buf[BUF_SIZE];
	if (!err) return 0;
	getsysmsg(err,buf,sizeof(buf));
	errprint(TEXT("Windows error: %s (code %i)"),buf,err);
	return 1;
}



static const char_t* dd_errmsg(int res)
{
	return TEXT("Unknown error");
}

static const char_t* acm_errmsg(int res)
{
	switch (res) {
	case MMSYSERR_INVALHANDLE: return TEXT("Invalid handle");
	case MMSYSERR_NODRIVER: return TEXT("No driver");
	case MMSYSERR_NOMEM: return TEXT("No memory");
	case WAVERR_UNPREPARED: return TEXT("Unprepared header");
	case WAVERR_STILLPLAYING: return TEXT("Still playing");
	case WAVERR_BADFORMAT: return TEXT("Bad format");
	}
	return TEXT("Unknown error");
}

static const char_t* ic_errmsg(int res)
{
	return TEXT("Unknown error");
}

static const char_t* ds_errmsg(int res)
{
	return TEXT("Unknown error");
}
