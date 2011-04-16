/*
	Agat Emulator version 1.19
	Copyright (c) NOP, nnop@newmail.ru
*/

#include "console.h"
#include "debug.h"
#include "streams.h"
#include "sysconf.h"
#include "runmgrint.h"

enum {
	CONTEXT_SYSMON,

	N_CONTEXTS
};


struct SYSMON_STATE
{
	word addr[3], raddr, waddr;
};

struct DEBUG_INFO
{
	struct SYS_RUN_STATE *sr;
	CONSOLE*con;
	HANDLE dbg_thread;
	int cur_context;
	BOOL stop;
	struct SYSMON_STATE sst;
};
