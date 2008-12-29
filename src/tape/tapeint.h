#ifndef TAPEINT_H
#define TAPEINT_H

#include "types.h"
#include "memory.h"
#include "streams.h"
#include "debug.h"
#include "sysconf.h"
#include "runmgr.h"
#include "runmgrint.h"
#include "avifile.h"

struct TAPE_STATE
{
	struct S_WAVE_OUTPUT wo;
	struct S_WAVE_FILE wi;
	FILE*f, *in;
	int prevsmp;
	HWND w;
	int freq;
	int val;
	int out_disabled, in_disabled;
	unsigned lastmsec, tr, tw;
	int tm;
	int ntest;
	int insmp, inlen, inpsmp;
	int inprc;
	TCHAR basename[CFGSTRLEN];
	struct SYS_RUN_STATE*sr;
	int use_fast;
};

int  tape_init(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*st, struct SLOTCONFIG*sc);


#endif // TAPEINT_H