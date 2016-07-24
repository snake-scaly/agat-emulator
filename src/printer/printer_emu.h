/*
	Epson data transfer protocol implementation.
	Author: Sergey "SnakE" Gromov, snake.scaly@gmail.com
*/

/*
	A PPRINTER_CABLE that takes care of the data, control, and
	state registers and produces a ready-to-use byte stream.

	This implementation calls ops->reset() when the printer reset signal
	is received via the protocol and when SYS_COMMAND_HRESET is received
	via the ops->slot_command() in addition to the direct calls.

	The user MUST call printer_emu_init() to initialize the structure. The
	user SHOULD consider all the structure fields as private.

	This implementation does not free the container structure. This task is
	delegated to the ops->free() function.
*/
struct PRINTER_EMU
{
	struct PRINTER_CABLE cab;
	int data;
	int control;
	int state;
	const struct PRINTER_EMU_OPERATIONS*ops;
};

/*
	Operations specific to the PRINTER_EMU.

	print_byte
		Process a single byte received through the hardware protocol.

	Other functions are identical to those in PRINTER_CABLE_OPERATIONS.
	They are called through by this implementation. However see PRINTER_EMU
	docs to understand implementation requirements.
*/
struct PRINTER_EMU_OPERATIONS
{
	int (*print_byte)(struct PRINTER_EMU*emu, int data);
	int (*is_printing)(struct PRINTER_EMU*emu);
	int (*slot_command)(struct PRINTER_EMU*emu, int id, long param);
	int (*reset)(struct PRINTER_EMU*emu);
	int (*free)(struct PRINTER_EMU*emu);
};

/* Initialize a PRINTER_EMU instance. */
int printer_emu_init(
	struct PRINTER_EMU*emu,
	const struct PRINTER_EMU_OPERATIONS*ops);
