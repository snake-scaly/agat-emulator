/*
	Agat Emulator version 1.19
	Copyright (c) NOP, nnop@newmail.ru
*/

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "common.h"
#include "debug.h"
#include "localize.h"
#include "msgloop.h"

#define INI_NAME ".\\emulator.ini"

struct GLOBAL_CONFIG g_config = { EMUL_FLAGS_DEFAULT };


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

BOOL WritePrivateProfileInt(LPCTSTR lpAppName, LPCTSTR lpKeyName, INT iValue, LPCTSTR lpFileName)
{
	TCHAR buf[64];
	wsprintf(buf, TEXT("%i"), iValue);
	return WritePrivateProfileString(lpAppName, lpKeyName, buf, lpFileName);
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
	g_config.flags = GetPrivateProfileInt(TEXT("Environment"), TEXT("Flags"), EMUL_FLAGS_DEFAULT, TEXT(INI_NAME));
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
	msgloop_run();
	WritePrivateProfileInt(TEXT("Environment"), TEXT("Flags"), g_config.flags, TEXT(INI_NAME));
	debug_term();
	localize_term();
	return r;
}
