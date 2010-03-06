/*
	Agat Emulator version 1.0
	Copyright (c) NOP, nnop@newmail.ru
	baseram - emulation of paddles via joystick or mouse
*/

#include <windows.h>
#include <mmsystem.h>
#include "types.h"
#include "memory.h"
#include "streams.h"
#include "debug.h"
#include "sysconf.h"
#include "runmgr.h"
#include "runmgrint.h"

#define MAX_TIME 3600

struct JOYPROC {
	int (*status)(struct JOYDATA*j, int no, unsigned dt);
	int (*button)(struct JOYDATA*j, int no);
	void (*reset)(struct JOYDATA*j);
};

struct JOYDATA
{
	int reset_time;
	int joy_present;
	JOYINFO lastinf;
	struct SYS_RUN_STATE*sr;
	struct JOYPROC procs;
};


static int  joy_status_none(struct JOYDATA*j, int no, unsigned dt)
{
	return 0xFF;
}

static int  joy_button_none(struct JOYDATA*j, int no)
{
	return 0xFF;
}

static void joy_reset_none(struct JOYDATA*j)
{
}

static int  joy_status_joy(struct JOYDATA*j, int no, unsigned dt)
{
	int clk;
	if (!j->joy_present) return 0x7F;
//	printf("dt=%i; X=%i; Y=%i\n",reset_time,lastinf.wXpos,lastinf.wYpos);
	switch (no) {
	case 0:	return (dt<(j->lastinf.wXpos>>8)*MAX_TIME/255)?0xFF:0x7F; break;
	case 1:	return (dt<(j->lastinf.wYpos>>8)*MAX_TIME/255)?0xFF:0x7F; break;
	}
	return 0x7F;
}

static int  joy_button_joy(struct JOYDATA*j, int no)
{
	if (!j->joy_present) return 0x7F;
	if (joyGetPos(JOYSTICKID1,&j->lastinf)==JOYERR_NOERROR) {
		switch (no) {
		case 0:	return (j->lastinf.wButtons&JOY_BUTTON1)?0xFF:0x7F; break;
		case 1:	return (j->lastinf.wButtons&JOY_BUTTON2)?0xFF:0x7F; break;
		}
	}
	return 0x7F;
}

static void joy_reset_joy(struct JOYDATA*j)
{
	if (joyGetPos(JOYSTICKID1,&j->lastinf)==JOYERR_NOERROR) {
		j->joy_present=1;
	} else {
		j->joy_present=0;
		puts("joy_error");
	}
}


static int  joy_status_mouse(struct JOYDATA*j, int no, unsigned dt)
{
	if (!j->joy_present) return 0x7F;
//	printf("dt=%i; X=%i; Y=%i\n",dt,(j->sr->xmousepos>>8)*MAX_TIME,(j->sr->ymousepos>>8)*MAX_TIME);
	switch (no) {
	case 0:	return (dt<(j->sr->xmousepos>>8)*MAX_TIME/255)?0xFF:0x7F; break;
	case 1:	return (dt<(j->sr->ymousepos>>8)*MAX_TIME/255)?0xFF:0x7F; break;
	}
	return 0x7F;
}

static int  joy_button_mouse(struct JOYDATA*j, int no)
{
	if (!j->joy_present) return 0x7F;
	switch (no) {
	case 0:	return (j->sr->mousebtn&1)?0xFF:0x7F; break;
	case 1:	return (j->sr->mousebtn&2)?0xFF:0x7F; break;
	}
	return 0x7F;
}

static void joy_reset_mouse(struct JOYDATA*j)
{
	j->joy_present=1;
}

static struct JOYPROC procs[3]={
	{joy_status_none, joy_button_none, joy_reset_none},
	{joy_status_mouse, joy_button_mouse, joy_reset_mouse},
	{joy_status_joy, joy_button_joy, joy_reset_joy}
};


static int joystick_command(struct SLOT_RUN_STATE*st, int cmd, int data, long param)
{
	return 0;
}


static int joystick_term(struct SLOT_RUN_STATE*st)
{
	return 0;
}

static void joy_w(word adr, byte data, struct JOYDATA*d) // C070-C07F
{
	d->reset_time=cpu_get_tsc(d->sr);
	d->procs.reset(d);
}

static byte joy_r(word adr, struct JOYDATA*d) // C070-C07F
{
	joy_w(adr, 0, d);
	return empty_read(adr, d);
}


static byte joyb_r(word adr, struct JOYDATA*d) // C061-C062
{
	return d->procs.button(d, (adr&1)^1);
}

static byte joyp_r(word adr, struct JOYDATA*d) // C064-C065
{
	unsigned clk;
	clk = cpu_get_tsc(d->sr) - d->reset_time;
	return d->procs.status(d, (adr&1), clk);
}

int  joystick_init(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*st, struct SLOTCONFIG*cf)
{
	struct JOYDATA *data;
	int n = 0;
	data = calloc(1, sizeof(*data));
	if (!data) return -1;
	switch (cf->dev_type) {
	case DEV_NOJOY: break;
	case DEV_MOUSE: n = 1; break;
	case DEV_JOYSTICK: n = 2; break;
	}
	data->procs = procs[n];
	data->sr = sr;

	fill_read_proc(sr->baseio_sel + 7, 1, joy_r, data);
	fill_write_proc(sr->baseio_sel + 7, 1, joy_w, data);

	fill_read_proc(sr->io6_sel + 1, 1, joyb_r, data);
	fill_read_proc(sr->io6_sel + 2, 1, joyb_r, data);

	fill_read_proc(sr->io6_sel + 4, 1, joyp_r, data);
	fill_read_proc(sr->io6_sel + 5, 1, joyp_r, data);
	
	st->data = data;
	st->free = joystick_term;
	st->command = joystick_command;
	return 0;
}

