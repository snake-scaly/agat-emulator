#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "resize.h"
#include "debug.h"
#include "localize.h"

#define INI_NAME ".\\emulator.ini"


void usleep(int microsec)
{
	Sleep(microsec/1000);
}

unsigned get_n_msec()
{
	return GetTickCount();
}

void msleep(int msec)
{
	Sleep(msec);
}

int load_buf_res(int no, void*buf, int len)
{
	HRSRC hr;
	HGLOBAL hg;
	LPVOID lp;
	hr = FindResource(GetModuleHandle(NULL), MAKEINTRESOURCE(no), RT_RCDATA);
	if (!hr) return -1;
	hg = LoadResource(GetModuleHandle(NULL), hr);
	if (!hg) return -1;
	lp = LockResource(hg);
	if (!lp) { FreeResource(hg); return -1; }

	memcpy(buf, lp, len);

	UnlockResource(lp);
	FreeResource(hg);
	return 0;
}

int main(int argc, const char* argv[])
{
	resize_set_cfgname(TEXT(INI_NAME));
	localize_init();
	{
		TCHAR lang[256];
		GetPrivateProfileString(TEXT("Environment"), TEXT("Lang"), TEXT(""), lang, 256, TEXT(INI_NAME));
		localize_set_lang(lang);
	}
	debug_init();
	InitCommonControls();
	(argc>1)?maindlg_run_config(NULL, argv[1]):maindlg_run(NULL);
	debug_term();
	localize_term();
	return 0;
}
