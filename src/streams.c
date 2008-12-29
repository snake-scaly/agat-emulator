/*
	Agat Emulator version 1.0
	Copyright (c) NOP, nnop@newmail.ru
	streams - file input/output library
*/

#include <windows.h>
#include <string.h>
#include "types.h"
#include "streams.h"

struct _STREAM
{
	int isres;
	HANDLE h;
	HRSRC hr;
	HGLOBAL hg;
	LPVOID lp;
	int ofs, sz;
};

ISTREAM*isfopen(const char_t*name)
{
	ISTREAM*r=malloc(sizeof(*r));
	if (!r) return r;
	r->isres = 0;
	r->h=CreateFile(name,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (r->h==INVALID_HANDLE_VALUE) {
		free(r);
		return 0;
	}
	return r;
}

ISTREAM*isfopen_res(const char_t*rname)
{
	ISTREAM*r=malloc(sizeof(*r));
	if (!r) return r;
	r->isres = 1;
	r->hr = FindResource(GetModuleHandle(NULL), rname, RT_RCDATA);
	if (!r->hr) { free(r); return NULL; }
	r->hg = LoadResource(GetModuleHandle(NULL), r->hr);
	if (!r->hg) { free(r); return NULL; }
	r->lp = LockResource(r->hg);
	if (!r->lp) { free(r); FreeResource(r->hg); return NULL; }
	r->ofs = 0;
	r->sz = SizeofResource(GetModuleHandle(NULL),r->hr);
	return r;
}

ISTREAM*isopenstdin()
{
#ifdef UNDER_CE
	return NULL;
#else
	ISTREAM*r=malloc(sizeof(*r));
	if (!r) return r;
	r->isres = 0;
	r->h=GetStdHandle(STD_INPUT_HANDLE);
	if (r->h==INVALID_HANDLE_VALUE) {
		free(r);
		return 0;
	}
	return r;
#endif
}

void isclose(ISTREAM*f)
{
	if (!f) return;
	if (f->isres) {
		UnlockResource(f->lp);
		FreeResource(f->hg);
	} else {
		CloseHandle(f->h);
	}
	free(f);
}

int  isread(ISTREAM*s,void*p,int nb)
{
	DWORD res;
	if (s->isres) {
		if (nb>s->ofs+s->sz) nb = s->sz-s->ofs;
		if (nb>0) memcpy(p,(char*)s->lp+s->ofs,nb);
		s->ofs+=nb;
		return nb;
	} else {
		if (!ReadFile(s->h,p,nb,&res,NULL)) return -1;
	}	
	return res;
}

static int seek_modes[]={FILE_BEGIN,FILE_CURRENT,FILE_END};

int  isseek(ISTREAM*s,int pos,int where)
{
	if (s->isres) {
		switch (where) {
		case SEEK_SET:
			s->ofs = pos;
			break;
		case SEEK_CUR:
			s->ofs += pos;
			break;
		case SEEK_END:
			s->ofs = s->sz + pos;
			break;
		}
	} else {
		SetFilePointer(s->h,pos,NULL,seek_modes[where]);
	}
	return 0;
}


OSTREAM*osfopen(const char_t*name)
{
	OSTREAM*r=malloc(sizeof(*r));
	if (!r) return r;
	r->isres = 0;
	r->h=CreateFile(name,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (r->h==INVALID_HANDLE_VALUE) {
		free(r);
		return 0;
	}
	return r;
}

OSTREAM*osopenstdout(const char_t*name)
{
#ifdef UNDER_CE
	return NULL;
#else
	OSTREAM*r=malloc(sizeof(*r));
	if (!r) return r;
	r->isres = 0;
	r->h=GetStdHandle(STD_OUTPUT_HANDLE);
	if (r->h==INVALID_HANDLE_VALUE) {
		free(r);
		return 0;
	}
	return r;
#endif
}

int  oswrite(OSTREAM*s,const void*p,int nb)
{
	DWORD res;
	if (!WriteFile(s->h,p,nb,&res,NULL)) return -1;
	return res;
}

void osclose(OSTREAM*f)
{
	if (!f) return;
	CloseHandle(f->h);
	free(f);
}

int  osseek(OSTREAM*s,int pos,int where)
{
	SetFilePointer(s->h,pos,NULL,seek_modes[where]);
	return 0;
}

IOSTREAM*iosfopen(const char_t*name)
{
	IOSTREAM*r=malloc(sizeof(*r));
	if (!r) return r;
	r->isres = 0;
	r->h=CreateFile(name,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (r->h==INVALID_HANDLE_VALUE) {
		free(r);
		return 0;
	}
	return r;
}

void iosclose(IOSTREAM*f)
{
	if (!f) return;
	CloseHandle(f->h);
	free(f);
}

int  iosread(IOSTREAM*s,void*p,int nb)
{
	DWORD res;
	if (!ReadFile(s->h,p,nb,&res,NULL)) return -1;
	return res;
}

int  ioswrite(IOSTREAM*s,const void*p,int nb)
{
	DWORD res;
	if (!WriteFile(s->h,p,nb,&res,NULL)) return -1;
	return res;
}

int  iosseek(IOSTREAM*s,int pos,int where)
{
	SetFilePointer(s->h,pos,NULL,seek_modes[where]);
	return 0;
}
