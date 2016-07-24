/*
	Epson data transfer protocol implementation.
	Author: Sergey "SnakE" Gromov, snake.scaly@gmail.com
*/

#include "printer_cable.h"
#include "printer_emu.h"
#include "runmgr.h"
#include <errno.h>
#include <stdlib.h>

#define RESET_BIT 0x40
#define DATA_BIT  0x80
#define BUSY_BIT  0x80
#define BYTE_MASK 0xFF

static void reset_state(struct PRINTER_EMU*emu)
{
	emu->control = 0xFF;
	emu->state = 0x04;
}

static int cable_reset(struct PRINTER_CABLE*cab)
{
	struct PRINTER_EMU*emu = (struct PRINTER_EMU*)cab;
	reset_state(emu);
	return emu->ops->reset(emu);
}

static int cable_write_data(struct PRINTER_CABLE*cab, int data)
{
	struct PRINTER_EMU*emu = (struct PRINTER_EMU*)cab;
	emu->data = data & BYTE_MASK;
	return 0;
}

static int cable_write_control(struct PRINTER_CABLE*cab, int data)
{
	struct PRINTER_EMU*emu = (struct PRINTER_EMU*)cab;
	int err = 0;

	if (!(emu->control & RESET_BIT) && (data & RESET_BIT)) {
		err = cable_reset(cab);
	}
	else if ((emu->control ^ data) & DATA_BIT) {
		if (data & DATA_BIT) {
			err = emu->ops->print_byte(emu, emu->data);
			emu->state &= ~BUSY_BIT;
		} else {
			emu->state |= BUSY_BIT;
		}
	}

	emu->control = data & BYTE_MASK;

	return err;
}

static int cable_read_state(struct PRINTER_CABLE*cab)
{
	struct PRINTER_EMU*emu = (struct PRINTER_EMU*)cab;
	return emu->state;
}

static int cable_is_printing(struct PRINTER_CABLE*cab)
{
	struct PRINTER_EMU*emu = (struct PRINTER_EMU*)cab;
	return emu->ops->is_printing(emu);
}

static int cable_slot_command(struct PRINTER_CABLE*cab, int id, long param)
{
	struct PRINTER_EMU*emu = (struct PRINTER_EMU*)cab;
	int err = 0;

	if (id == SYS_COMMAND_HRESET) err = cable_reset(cab);
	if (err == 0) err = emu->ops->slot_command(emu, id, param);

	return err;
}

static int cable_free(struct PRINTER_CABLE*cab)
{
	struct PRINTER_EMU*emu = (struct PRINTER_EMU*)cab;
	return emu->ops->free(emu);
}

static const struct PRINTER_CABLE_OPERATIONS printer_emu_ops =
{
	cable_write_data,
	cable_write_control,
	cable_read_state,
	cable_is_printing,
	cable_slot_command,
	cable_reset,
	cable_free,
};

int printer_emu_init(
	struct PRINTER_EMU*emu,
	const struct PRINTER_EMU_OPERATIONS*ops)
{
	if (!emu || !ops) return EINVAL;

	emu->cab.ops = &printer_emu_ops;
	emu->ops = ops;
	reset_state(emu);

	return 0;
}
