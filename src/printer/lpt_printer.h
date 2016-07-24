/*
	Passthrough printer that forwards data to the PC parallel port
	without modifications.
	Author: Sergey "SnakE" Gromov
*/

struct PRINTER_CABLE* lpt_printer_create(struct SLOT_RUN_STATE*st);
