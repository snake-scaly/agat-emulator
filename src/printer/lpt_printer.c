/*
	Passthrough printer that forwards data to the PC parallel port
	without modifications.
	Author: Sergey "SnakE" Gromov
*/

#include "printer_cable.h"
#include "lpt_printer.h"
#include "printer_emu.h"
#include "common.h"
#include "runmgr.h"
#include <errno.h>
#include <memory.h>
#include <fcntl.h>
#include <sys/stat.h>

#define CHECK_ACTIVITY_INTERVAL (1000000 / 5)
#define INACTIVITY_TIMEOUT 1000

struct LPT_PRINTER
{
	struct PRINTER_EMU emu;
	struct SLOT_RUN_STATE*st;
	int out;
	int bad;
	unsigned last_activity;
};

/* Open the parallel port. We use low-level I/O to avoid buffering. */
static int lpt_open(struct LPT_PRINTER *p)
{
	if (p->bad) return ENOENT;

	if (p->out == -1) p->out = open("PRN", O_WRONLY | O_BINARY, S_IREAD | S_IWRITE);

	if (p->out == -1) {
		printf("lpt_printer: Parallel port is not available\n");
		p->bad = 1;
		return ENOENT;
	}

	system_command(p->st->sr, SYS_COMMAND_SET_CPUTIMER, CHECK_ACTIVITY_INTERVAL, DEF_CPU_TIMER_ID(p->st));
	return 0;
}

static void lpt_close(struct LPT_PRINTER *p)
{
	if (p->out != -1) {
		close(p->out);
		p->out = -1;
		system_command(p->st->sr, SYS_COMMAND_SET_CPUTIMER, 0, DEF_CPU_TIMER_ID(p->st));
	}
}

static int lpt_print_byte(struct PRINTER_EMU *emu, int data)
{
	struct LPT_PRINTER *p = (struct LPT_PRINTER*)emu;
	int err;
	char buf[1];

	p->last_activity = get_n_msec();

	err = lpt_open(p);
	if (err) return err;

	buf[0] = data;
	if (write(p->out, buf, 1) == -1) {
		printf("lpt_printer: Error writing to PRN: %s\n", strerror(errno));
		lpt_close(p);
		p->bad = 1;
		return errno;
	}

	return 0;
}

static int lpt_is_printing(struct PRINTER_EMU *emu)
{
	struct LPT_PRINTER *p = (struct LPT_PRINTER*)emu;
	return p->out != -1;
}

static int lpt_reset(struct PRINTER_EMU *emu)
{
	struct LPT_PRINTER *p = (struct LPT_PRINTER*)emu;
	lpt_close(p);
	p->bad = 0;
	return 0;
}

static int lpt_free(struct PRINTER_EMU *emu)
{
	struct LPT_PRINTER *p = (struct LPT_PRINTER*)emu;
	lpt_close(p);
	free(p);
	return 0;
}

static int slot_command(struct PRINTER_EMU *emu, int id, long param)
{
	struct LPT_PRINTER *p = (struct LPT_PRINTER*)emu;

	if (id == SYS_COMMAND_CPUTIMER && param == DEF_CPU_TIMER_ID(p->st) && p->out != -1) {
		unsigned inactive = get_n_msec() - p->last_activity;
		if (inactive >= INACTIVITY_TIMEOUT) {
			/* Closing PRN seems to be the only way to flush all
			   buffers to the printer. And yes there seems to be
			   some buffer even when using low-level I/O. */
			lpt_close(p);
		}
	}

	return 0;
}

static const struct PRINTER_EMU_OPERATIONS ops =
{
	lpt_print_byte,
	lpt_is_printing,
	slot_command,
	lpt_reset,
	lpt_free,
};

struct PRINTER_CABLE* lpt_printer_create(struct SLOT_RUN_STATE*st)
{
	struct LPT_PRINTER *p;
	int err;

	p = calloc(1, sizeof *p);
	if (!p) return NULL;

	err = printer_emu_init(&p->emu, &ops);
	if (err) {
		free(p);
		errno = err;
		return NULL;
	}

	p->out = -1;
	p->st = st;

	return (struct PRINTER_CABLE*)p;
}
