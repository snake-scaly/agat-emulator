/*
	Agat Emulator version 1.21
	Copyright (c) NOP, nnop@newmail.ru
*/

#include "runmgr.h"
#include "runmgrint.h"
#include "runstate.h"
#include "videow.h"

#include "resource.h"

struct AATOM_DATA
{
	int x;
};


static int free_system_aa(struct SYS_RUN_STATE*sr);
static int restart_system_aa(struct SYS_RUN_STATE*sr);
static int save_system_aa(struct SYS_RUN_STATE*sr, OSTREAM*out);
static int load_system_aa(struct SYS_RUN_STATE*sr, ISTREAM*in);
static int xio_control_aa(struct SYS_RUN_STATE*sr, int req);


int init_system_aa(struct SYS_RUN_STATE*sr)
{
	struct AATOM_DATA*p;
	p = calloc(1, sizeof(*p));
	if (!p) return -1;
	sr->sys.ptr = p;
	sr->sys.free_system = free_system_aa;
	sr->sys.restart_system = restart_system_aa;
	sr->sys.save_system = save_system_aa;
	sr->sys.load_system = load_system_aa;
	sr->sys.xio_control = xio_control_aa;

	puts("init_system_aa");

	return 0;
}


int free_system_aa(struct SYS_RUN_STATE*sr)
{
	if (sr->sys.ptr) free(sr->sys.ptr);
	sr->sys.ptr = NULL;
	return 0;
}

int restart_system_aa(struct SYS_RUN_STATE*sr)
{
	struct AATOM_DATA*aa = sr->sys.ptr;
	memset(aa, 0, sizeof(*aa));
	return 0;
}

int save_system_aa(struct SYS_RUN_STATE*sr, OSTREAM*out)
{
	struct AATOM_DATA*aa = sr->sys.ptr;
	puts("save_system_aa");
	oswrite(out, aa, sizeof(*aa));
	return 0;
}

int load_system_aa(struct SYS_RUN_STATE*sr, ISTREAM*in)
{
	struct AATOM_DATA*aa = sr->sys.ptr;
	puts("load_system_aa");
	isread(in, aa, sizeof(*aa));
	return 0;
}

int xio_control_aa(struct SYS_RUN_STATE*sr, int req)
{
	return 0;
}
