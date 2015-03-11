/*
	Epson data transfer protocol implementation.
	Author: Sergey "SnakE" Gromov, snake.scaly@gmail.com
*/

#include "printer_cable.h"
#include "printer_emu.h"
#include <errno.h>
#include <stdlib.h>

#define RESET_BIT 0x40
#define DATA_BIT  0x80
#define BUSY_BIT  0x80
#define BYTE_MASK 0xFF

struct PRINTER_EMU
{
	int data;
	int control;
	int state;

	void*param;
	printer_cable_write_t print_byte;
	printer_cable_read_t is_printing;
	printer_cable_action_t reset;
	printer_cable_action_t free_h;
};

static void reset_state(struct PRINTER_EMU*emu)
{
	emu->control = 0xFF;
	emu->state = 0x04;
}

static int cable_reset(struct PRINTER_EMU*emu)
{
	if (!emu) return EINVAL;
	reset_state(emu);
	return emu->reset(emu->param);
}

static int cable_write_data(struct PRINTER_EMU*emu, int data)
{
	if (!emu) return EINVAL;
	emu->data = data & BYTE_MASK;
	return 0;
}

static int cable_write_control(struct PRINTER_EMU*emu, int data)
{
	int err = 0;

	if (!emu) return EINVAL;

	if (!(emu->control & RESET_BIT) && (data & RESET_BIT)) {
		err = cable_reset(emu);
	}
	else if ((emu->control ^ data) & DATA_BIT) {
		if (data & DATA_BIT) {
			err = emu->print_byte(emu->param, emu->data);
			emu->state &= ~BUSY_BIT;
		} else {
			emu->state |= BUSY_BIT;
		}
	}

	emu->control = data & BYTE_MASK;

	return err;
}

static int cable_read_state(struct PRINTER_EMU*emu)
{
	if (!emu) { errno = EINVAL; return -1; }
	return emu->state;
}

static int cable_is_printing(struct PRINTER_EMU*emu)
{
	if (!emu) { errno = EINVAL; return -1; }
	return emu->is_printing(emu->param);
}

static int cable_free(struct PRINTER_EMU*emu)
{
	int err;
	if (!emu) return EINVAL;
	err = emu->free_h(emu->param);
	free(emu);
	return err;
}

PPRINTER_CABLE printer_emu_create(
	void*param,
	printer_cable_write_t print_byte,
	printer_cable_read_t is_printing,
	printer_cable_action_t reset,
	printer_cable_action_t free_h)
{
	struct PRINTER_EMU*emu;
	PPRINTER_CABLE result;

	if (print_byte == NULL || is_printing == NULL || reset == NULL ||
		free_h == NULL)
	{
		return NULL;
	}

	emu = calloc(1, sizeof *emu);
	if (!emu) return NULL;

	emu->param = param;
	emu->print_byte = print_byte;
	emu->is_printing = is_printing;
	emu->reset = reset;
	emu->free_h = free_h;
	reset_state(emu);

	result = printer_cable_create(emu, cable_write_data,
		cable_write_control, cable_read_state, cable_is_printing,
		cable_reset, cable_free);
	if (!result) free(emu);

	return result;
}
