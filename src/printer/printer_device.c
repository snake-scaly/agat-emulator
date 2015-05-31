/*
	Create emulated devices to attach to the parallel port.
	Author: Sergey "SnakE" Gromov
*/

#include "printer_cable.h"
#include "printer_device.h"
#include "epson_emu.h"
#include "export.h"
#include "raw_file_printer.h"
#include "lpt_printer.h"
#include "runmgr.h"
#include "runmgrint.h"
#include "sysconf.h"

static struct PRINTER_CABLE* create_epson_in_mode(struct SYS_RUN_STATE*sr, int mode)
{
	struct EPSON_EXPORT exp = {0};
	int i = -1;
	unsigned fl = EPSON_TEXT_NO_RECODE;
	struct PRINTER_CABLE*cable = NULL;

	switch (mode) {
	case PRINT_TEXT:
		i = export_text_init(&exp, 0, sr->video_w);
		fl = EPSON_TEXT_RECODE_FX;
		break;
	case PRINT_TIFF:
		i = export_tiff_init(&exp, EXPORT_TIFF_COMPRESS_RLE, sr->video_w);
		fl = EPSON_TEXT_RECODE_FX;
		break;
	case PRINT_PRINT:
		i = export_print_init(&exp, 0, sr->video_w);
		fl = EPSON_TEXT_RECODE_FX;
		break;
	}

	if (i >= 0)  {
		cable = epson_create(fl, &exp);
	}

	if (!cable && exp.free_data) {
		exp.free_data(exp.param);
	}

	return cable;
}

struct PRINTER_CABLE* printer_device_for_mode(
	struct SYS_RUN_STATE*sr,
	struct SLOT_RUN_STATE*st,
	int mode)
{
	if (mode == PRINT_RAW) {
		return raw_file_printer_create(sr->video_w);
	}
	if (mode == PRINT_LPT) {
		return lpt_printer_create(st);
	}
	return create_epson_in_mode(sr, mode);
}
