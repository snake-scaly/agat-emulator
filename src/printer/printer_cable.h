/*
	Interface to an arbitrary device connected to the printer port.
	Author: Sergey "SnakE" Gromov, snake.scaly@gmail.com
*/

/*
	Type: PPRINTER_CABLE
	An opaque printer cable type.
*/
typedef struct PRINTER_CABLE *PPRINTER_CABLE;

/*
	Callback: printer_cable_write_t
	Receives bytes written to the cable. Can be used for data or control.
	Should return 0 on success, or an error code on failure.
*/
typedef int (*printer_cable_write_t)(void *param, int data);

/*
	Callback: printer_cable_read_t
	Handles requests for reading printer state. Should return 0-255
	on success, or -1 and set _errno_ on failure.
*/
typedef int (*printer_cable_read_t)(void *param);

/*
	Callback: printer_cable_action_t
	Performs a certain action on request, e.g. reset. Should return 0
	on success or an error code on failure.
*/
typedef int (*printer_cable_action_t)(void *param);

/*
	Function: printer_cable_create
	Create an emulated printer cable with the given callbacks.

	Parameters:
		param         - context parameter for the callbacks
		write_data    - receives data written to the data register
		write_control - receives data written to the control register
		read_state    - reads data from the state register
		is_printing   - implements <printer_cable_is_printing>
		reset         - implements <printer_cable_reset>
		free_h        - frees any resources associated with the printer
*/
PPRINTER_CABLE printer_cable_create(
	void *param,
	printer_cable_write_t write_data,
	printer_cable_write_t write_control,
	printer_cable_read_t read_state,
	printer_cable_read_t is_printing,
	printer_cable_action_t reset,
	printer_cable_action_t free_h);

/*
	Function: printer_cable_write_data
	Send a data byte to the printer.
*/
int printer_cable_write_data(PPRINTER_CABLE emu, int data);

/*
	Function: printer_cable_write_control
	Send a control byte to the printer.
*/
int printer_cable_write_control(PPRINTER_CABLE emu, int data);

/*
	Function: printer_cable_read_state
	Read printer state byte.
*/
int printer_cable_read_state(PPRINTER_CABLE emu);

/*
	Function: printer_cable_is_printing
	Check if the printer is printing.

	Returns non-zero if some data already went to the media and the
	device is waiting for more. Ultimately returning non-zero enables the
	'Finish printing' item in the emulator window menu.
*/
int printer_cable_is_printing(PPRINTER_CABLE emu);

/*
	Function: printer_cable_reset
	Reset the device.

	The device flushes all data, closes all files, and reverts to the
	initial state after the emulator start. It is called when the emulator
	is hard reset, or when the user selects the 'Finish printing' item in
	the emulator window menu.
*/
int printer_cable_reset(PPRINTER_CABLE emu);

/*
	Function: printer_cable_free
	Free resources associated with the device.
*/
int printer_cable_free(PPRINTER_CABLE emu);
