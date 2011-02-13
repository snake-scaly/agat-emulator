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

TCHAR lang[256];

static int g_argc;
static const char**g_argv;

const char*sys_get_parameter(const char*name)
{
	int i;
	const char**p;
	for (i = g_argc, p = g_argv; i > 1; --i, ++p) {
		if (p[0][0] == '-' && !_stricmp(name, p[0] + 1)) {
			return p[1];
		}
	}
	return NULL;
}

int main(int argc, const char* argv[])
{
	const char*cfgname = NULL;
	int r;
	if (argc > 1) {
		g_argc = argc - 2;
		g_argv = argv + 2;
		cfgname = argv[1];
	} else {
		g_argc = argc - 1;
		g_argv = argv + 1;
	}
	{
		const char*dir;
		dir = sys_get_parameter("cd");
		if (dir) SetCurrentDirectory(dir);
	}
	resize_set_cfgname(TEXT(INI_NAME));
	localize_init();
	{
		GetPrivateProfileString(TEXT("Environment"), TEXT("Lang"), TEXT("russian"), lang, 256, TEXT(INI_NAME));
		localize_set_lang(lang);
	}
	debug_init();
	InitCommonControls();
	if (cfgname) {
		r = maindlg_run_config(NULL, cfgname);
		if (r) {
			MessageBox(NULL, TEXT("Error loading configuration"), cfgname, 0);
		}
	} else {
		r = maindlg_run(NULL);
	}
	debug_term();
	localize_term();
	return r;
}
