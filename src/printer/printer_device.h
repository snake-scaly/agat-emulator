/*
	Create emulated devices to attach to the parallel port.
	Author: Sergey "SnakE" Gromov
*/

struct PRINTER_CABLE* printer_device_for_mode(
	struct SYS_RUN_STATE*sr,
	struct SLOT_RUN_STATE*st,
	int mode);
