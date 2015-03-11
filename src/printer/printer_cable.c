/*
	Interface to an arbitrary device connected to the printer port.
	Author: Sergey "SnakE" Gromov, snake.scaly@gmail.com
*/

#include "printer_cable.h"
#include <errno.h>
#include <stdlib.h>

struct PRINTER_CABLE
{
	void*param;
	printer_cable_write_t write_data;
	printer_cable_write_t write_control;
	printer_cable_read_t read_state;
	printer_cable_read_t is_printing;
	printer_cable_action_t reset;
	printer_cable_action_t free_h;
};

PPRINTER_CABLE printer_cable_create(
	void*param,
	printer_cable_write_t write_data,
	printer_cable_write_t write_control,
	printer_cable_read_t read_state,
	printer_cable_read_t is_printing,
	printer_cable_action_t reset,
	printer_cable_action_t free_h)
{
	PPRINTER_CABLE emu;

	// We don't check param here because null param may be valid.
	if (write_data == NULL || write_control == NULL ||
		read_state == NULL || is_printing == NULL || reset == NULL ||
		free == NULL)
	{
		return NULL;
	}

	emu = calloc(1, sizeof *emu);
	if (!emu) return NULL;

	emu->param = param;
	emu->write_data = write_data;
	emu->write_control = write_control;
	emu->read_state = read_state;
	emu->is_printing = is_printing;
	emu->reset = reset;
	emu->free_h = free_h;

	return emu;
}

int printer_cable_write_data(PPRINTER_CABLE emu, int data)
{
	if (!emu) return EINVAL;
	return emu->write_data(emu->param, data);
}

int printer_cable_write_control(PPRINTER_CABLE emu, int data)
{
	if (!emu) return EINVAL;
	return emu->write_control(emu->param, data);
}

int printer_cable_read_state(PPRINTER_CABLE emu)
{
	if (!emu) { errno = EINVAL; return -1; }
	return emu->read_state(emu->param);
}

int printer_cable_is_printing(PPRINTER_CABLE emu)
{
	if (!emu) { errno = EINVAL; return -1; }
	return emu->is_printing(emu->param);
}

int printer_cable_reset(PPRINTER_CABLE emu)
{
	if (!emu) return EINVAL;
	return emu->reset(emu->param);
}

int printer_cable_free(PPRINTER_CABLE emu)
{
	int err;
	if (!emu) return EINVAL;
	err = emu->free_h(emu->param);
	free(emu);
	return err;
}
