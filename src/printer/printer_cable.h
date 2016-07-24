/*
	Interface to an arbitrary device connected to the printer port.
	Author: Sergey "SnakE" Gromov, snake.scaly@gmail.com
*/

/*
	Represents a cable connected to a printer card's serial port.
*/
struct PRINTER_CABLE
{
	const struct PRINTER_CABLE_OPERATIONS*ops;
};

/*
	Implementations of individual operations on a device attached to this
	printer cable.

	write_data
		Send a data byte to the printer.

	write_control
		Send a control byte to the printer.

	read_state
		Read printer state byte.

	is_printing
		Check if the printer is printing.

		Returns non-zero if some data already went to the media and the
		device is waiting for more. Ultimately returning non-zero enables
		the 'Finish printing' item in the emulator window menu.

	slot_command
		Handle a command sent to the printer slot.

		cab   - printer cable instance
		id    - one of the SYS_COMMAND_ commands
		param - depends on the command

	flush
		Reset the device.

		The device flushes all data, closes all files, and reverts to
		the initial state after the emulator start. It is called when
		the emulator is hard reset, or when the user selects the 'Finish
		printing' item in the emulator window menu.

	free
		Free resources associated with the device.
*/
struct PRINTER_CABLE_OPERATIONS
{
	int (*write_data)(struct PRINTER_CABLE*cab, int data);
	int (*write_control)(struct PRINTER_CABLE*cab, int data);
	int (*read_state)(struct PRINTER_CABLE*cab);
	int (*is_printing)(struct PRINTER_CABLE*cab);
	int (*slot_command)(struct PRINTER_CABLE*cab, int id, long param);
	int (*flush)(struct PRINTER_CABLE*cab);
	int (*free)(struct PRINTER_CABLE*cab);
};
