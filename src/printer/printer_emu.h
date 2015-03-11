/*
	Epson data transfer protocol implementation.
	Author: Sergey "SnakE" Gromov, snake.scaly@gmail.com
*/

/*
	Function: printer_emu_create
	Create a <PPRINTER_CABLE> that takes care of the data, control, and
	state registers and produces a ready-to-use byte stream.

	_reset_ is called when <printer_cable_reset> is called or when the
	printer reset signal is received via the protocol.

	Parameters:
		param - context parameter for the callbacks
		print_byte  - receives raw bytes to be printed
		is_printing - see <printer_cable_is_printing>
		reset       - see <printer_cable_reset>
		free_h      - frees all resources associated with the printer
*/
PPRINTER_CABLE printer_emu_create(
	void*param,
	printer_cable_write_t print_byte,
	printer_cable_read_t is_printing,
	printer_cable_action_t reset,
	printer_cable_action_t free_h);
