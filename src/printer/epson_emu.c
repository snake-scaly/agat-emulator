/*
	Epson printer command emulation for Agat emulator
	Copyright (c) NOP, nnop@newmail.ru
*/

#include "printer_cable.h"
#include "printer_emu.h"
#include "epson_emu.h"
#include "sysconf.h"
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

typedef struct EPSON_EMU *PEPSON_EMU;

//#define EPSON_DEBUG

#ifdef EPSON_DEBUG
#define Pprintf(s) printf s
#define Pputs(s) puts(s)
#else
#define Pprintf(s)
#define Pputs(s)
#endif

static unsigned char koi2win[128] = {
	128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
	144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 218, 155, 176, 157, 183, 159,
	160, 161, 162, 184, 186, 165, 166, 191, 168, 169, 170, 171, 172, 173, 174, 175,
	156, 177, 178, 168, 170, 181, 182, 175, 184, 185, 186, 187, 188, 189, 190, 185,
	254, 224, 225, 246, 228, 229, 244, 227, 245, 232, 233, 234, 235, 236, 237, 238,
	239, 255, 240, 241, 242, 243, 230, 226, 252, 251, 231, 248, 253, 249, 247, 250,
	222, 192, 193, 214, 196, 197, 212, 195, 213, 200, 201, 202, 203, 204, 205, 206,
	207, 223, 208, 209, 210, 211, 198, 194, 220, 219, 199, 216, 221, 217, 215, 218
};

static unsigned char fx2win[128] = {
         0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
         0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
         0, 192, 193, 194, 195, 196, 197,   0, 198, 199, 200, 201, 202, 203, 204, 205,
       206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221,
       222, 223,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
         0, 224, 225, 226, 227, 228, 229,   0, 230, 231, 232, 233, 234, 235, 236, 237,
       238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253,
       254, 255,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

static int charkoi2win(unsigned char c)
{
	if (c < 0x80) return c;
	return koi2win[c&0x7F];
}


static int charfx2win(unsigned char c)
{
	if (c < 0x80) return c;
	return fx2win[c&0x7F]?fx2win[c&0x7F]:c;
}

static int no_recode(unsigned char c)
{
	return c;
}



struct EPSON_EMU
{
	unsigned flags;

	int esc_mode;
	int esc_cnt;
	int esc_cmd;

	int gr_mode;
	int bin_cnt;
	unsigned char*data;

	struct EPSON_EXPORT exp;

	int (*recode)(unsigned char c);
};

static int epson_free(PEPSON_EMU emu)
{
	if (!emu) return EINVAL;
	if (emu->data) free(emu->data);
	emu->data = NULL;
	if (emu->exp.free_data) emu->exp.free_data(emu->exp.param);
	free(emu);
	return 0;
}

static int epson_command0b(PEPSON_EMU emu, unsigned char cmd)
{
	switch (cmd) {
	case '`':
		Pputs("Unknown Esc ` command");
		break;
	case '@':
		Pputs("Initialize printer");
		break;
	case '0':
		Pputs("Set line spacing to 1/8 inch");
		break;
	case '1':
		Pputs("Set line spacing to 7/72 inch");
		break;
	case '2':
		Pputs("Set line spacing to 1/6 inch");
		break;
	case '4':
		Pputs("Select italic font");
		break;
	case '5':
		Pputs("Cancel italic font");
		break;
	case '6':
		Pputs("Extend charset");
		break;
	case '7':
		Pputs("Reduce charset");
		break;
	case '8':
		Pputs("Disable paper-out detector");
		break;
	case '9':
		Pputs("Enable paper-out detector");
		break;
	case 'E':
		Pputs("Select bold font");
		break;
	case 'F':
		Pputs("Cancel bold font");
		break;
	case 'G':
		Pputs("Select double-strike font");
		break;
	case 'H':
		Pputs("Cancel double-strike font");
		break;
	case 'M':
		Pputs("Enable 12-cpi printing");
		break;
	case 'O':
		Pputs("Cancel top and bottom margin setting");
		break;
	case 'P':
		Pputs("Enable 10-cpi printing");
		break;
	case 'T':
		Pputs("Cancel superscript/subscript");
		break;
	case 'g':
		Pputs("Enable 15-cpi printing");
		break;
	case EPS_SI:
		Pputs("Select condensed printing");
		break;
	case EPS_SO:
		Pputs("Select double width printing");
		break;
	case '<':
		Pputs("Print one line unidirectionally");
		break;
	default:
		return 0;
	}
	if (emu->exp.write_command) {
		emu->exp.write_command(emu->exp.param, cmd, 0, NULL);
	}
	emu->esc_mode = 0;
	return 0;
}

static int epson_command1b(PEPSON_EMU emu, unsigned char cmd, unsigned char data)
{
	switch (cmd) {
	case ' ':
		Pprintf(("Select space between chars to %i\n", data));
		break;
	case '!':
		Pprintf(("Master select: %02X\n", data));
		break;
	case '-':
		Pprintf(("Select underline mode: %i\n", data));
		break;
	case '+':
		Pprintf(("Set line spacing to %i/360 inches\n", data));
		break;
	case '3':
		Pprintf(("Set line spacing to %i/216 inches\n", data));
		break;
	case 'A':
		Pprintf(("Set line spacing to %i/72 inch\n", data));
		break;
	case 'a':
		Pprintf(("Select justification %i\n", data));
		break;
	case 'B':
		Pprintf(("Setting vertical tabs...\n"));
		if (data) return 0;
		break;
	case 'C':
		if (data) {
			Pprintf(("Setting page length to %i lines\n", data));
		} else return 0;
		break;
	case 'D':
		Pprintf(("Setting horizontal tabs...\n"));
		if (data) return 0;
		break;
	case 'e':
		Pprintf(("Setting fixed tab increment...\n"));
		return 0;
	case 'I':
		Pprintf(("Select charset extension %i\n", data));
		break;
	case 'J':
		Pprintf(("Advance vertical position by %i/180 inches\n", data));
		break;
	case 'k':
		Pprintf(("Select typeface %i\n", data));
		break;
	case 'l':
		Pprintf(("Set left margin %i\n", data));
		break;
	case 'K':
		Pprintf(("Selecting 60-dpi graphics...\n"));
		emu->bin_cnt = data;
		return 0;
	case 'L':
		Pprintf(("Selecting 120-dpi graphics...\n"));
		emu->bin_cnt = data;
		return 0;
	case 'Y':
		Pprintf(("Selecting ds/dd - graphics...\n"));
		emu->bin_cnt = data;
		return 0;
	case 'Z':
		Pprintf(("Selecting qd - graphics...\n"));
		emu->bin_cnt = data;
		return 0;
	case '*':
		Pprintf(("Selecting graphics %i...\n", data));
		emu->gr_mode = data;
		return 0;
	case 'm':
		Pprintf(("Select charset extension %i\n", data));
		break;
	case 'N':
		Pprintf(("Set bottom margin %i\n", data));
		break;
	case 'p':
		Pprintf(("Select proportional mode %i\n", data));
		break;
	case 'Q':
		Pprintf(("Set right margin %i\n", data));
		break;
	case 'q':
		Pprintf(("Select outline/shadow mode %i\n", data));
		break;
	case 'R':
		Pprintf(("Select character set %i\n", data));
		break;
	case 'S':
		Pprintf(("Select superscript/subscript: %i\n", data));
		break;
	case 's':
		Pprintf(("Select speed mode %i\n", data));
		break;
	case 't':
		Pprintf(("Select character table %i\n", data));
		break;
	case 'U':
		Pprintf(("Select unidirectional printing %i\n", data));
		break;
	case 'W':
		Pprintf(("Select double-width printing %i\n", data));
		break;
	case 'w':
		Pprintf(("Select double-height printing %i\n", data));
		break;
	case 'x':
		Pprintf(("Select print quality %i\n", data));
		break;
	case '/':
		Pprintf(("Select vertical tab channel %i\n", data));
		break;
	case '%':
		Pprintf(("Select user defined character set %i\n", data));
		break;
	case '.':
		Pprintf(("Entering graphic mode...\n", data));
		break;
	case EPS_EM:
		Pprintf(("Select cut-sheet %i\n", data));
		break;
	}
	if (emu->exp.write_command) {
		emu->exp.write_command(emu->exp.param, cmd, 1, &data);
	}
	emu->esc_mode = 0;
	return 0;
}

static int epson_commandxb(PEPSON_EMU emu, unsigned char cmd, unsigned char data, int no)
{
	switch (cmd) {
	case 'B':
		if (!data) emu->esc_mode = 0;
		break;
	case 'C':
		if (emu->exp.write_command) {
			unsigned char bb[2] = {0, data};
			emu->exp.write_command(emu->exp.param, cmd, 2, bb);
		}
		Pprintf(("Setting page length to %i inches\n", data));
		emu->esc_mode = 0;
		break;
	case 'D':
		if (!data) emu->esc_mode = 0;
		break;
	case 'e':
		if (no == 3) emu->esc_mode = 0;
		break;
	case 'L': case 'K': case 'Y': case 'Z':
		if (no == 3) {
			emu->bin_cnt |= data<<8;
			emu->data = realloc(emu->data, emu->bin_cnt);
			assert(emu->data);
		}
		if (no > 3) {
			emu->data[no - 4] = data;
		}
		if (no >= emu->bin_cnt + 3) {
			if (emu->exp.write_command) {
				emu->exp.write_command(emu->exp.param, cmd, emu->bin_cnt, emu->data);
			}
			emu->esc_mode = 0;
		}
		break;
	case '*':
		switch (no) {
		case 3: emu->bin_cnt = data; break;
		case 4: emu->bin_cnt |= data<<8;
			emu->data = realloc(emu->data, emu->bin_cnt + 1);
			assert(emu->data);
			emu->data[emu->bin_cnt] = emu->gr_mode;
		default:
			if (no > 4) {
				emu->data[no - 5] = data;
			}
			if (no >= emu->bin_cnt + 4) {
				if (emu->exp.write_command) {
					emu->exp.write_command(emu->exp.param, cmd, emu->bin_cnt, emu->data);
				}
				emu->esc_mode = 0;
			}
		}
		break;
	}
	return 0;
}

static int epson_write(PEPSON_EMU emu, int data)
{
	int r;
	if (!emu) return EINVAL;
	Pprintf(("esc_mode = %i; esc_cnt = %i; char = %c (%i, 0x%02X)\n", emu->esc_mode, emu->esc_cnt, data, data, data));
	if (emu->esc_mode) {
		emu->esc_cnt ++;
		switch (emu->esc_cnt) {
		case 1:
			emu->esc_cmd = data;
			epson_command0b(emu, emu->esc_cmd);
			break;
		case 2:
			epson_command1b(emu, emu->esc_cmd, data);
			break;
		default:
			epson_commandxb(emu, emu->esc_cmd, data, emu->esc_cnt);
			break;
		}
	} else {
		switch (data) {
		case EPS_ESC:
			emu->esc_mode = 1;
			emu->esc_cnt = 0;
			emu->esc_cmd = 0;
			break;
		case EPS_SI:
			Pputs("Select condensed printing");
			if (emu->exp.write_char) {
				emu->exp.write_char(emu->exp.param, data);
			}
			break;
		case EPS_DC2:
			Pputs("Cancel condensed printing");
			if (emu->exp.write_char) {
				emu->exp.write_char(emu->exp.param, data);
			}
			break;
		case EPS_SO:
			Pputs("Select double-width printing");
			if (emu->exp.write_char) {
				emu->exp.write_char(emu->exp.param, data);
			}
			break;
		case EPS_DC4:
			Pputs("Cancel double-width printing");
			if (emu->exp.write_char) {
				emu->exp.write_char(emu->exp.param, data);
			}
			break;
		case EPS_BEL:
			Pputs("Printer beep");
			if (emu->exp.write_char) {
				emu->exp.write_char(emu->exp.param, data);
			}
			break;
		case EPS_FF:
			Pputs("Printer form feed");
			if (emu->exp.write_char) {
				emu->exp.write_char(emu->exp.param, data);
			}
			break;
		case EPS_CR:
			Pputs("Printer carriage return");
			if (emu->exp.write_char) {
				emu->exp.write_char(emu->exp.param, data);
			}
			break;
		case EPS_LF:
			Pputs("Printer line feed");
			if (emu->exp.write_char) {
				emu->exp.write_char(emu->exp.param, data);
			}
			break;
		default:
			if (emu->exp.write_char) {
				emu->exp.write_char(emu->exp.param, emu->recode(data));
			}
		}
	}
	return 0;
}

static int epson_flush(PEPSON_EMU emu)
{
	if (!emu) return EINVAL;
	emu->esc_mode = 0;
	if (!emu->exp.close) return EINVAL;
	emu->exp.close(emu->exp.param);
	return 0;
}

static int epson_hasdata(PEPSON_EMU emu)
{
	if (!emu) { errno = EINVAL; return -1; }
	if (emu->exp.opened) return emu->exp.opened(emu->exp.param);
	return 0;
}

PPRINTER_CABLE epson_create(unsigned flags, struct EPSON_EXPORT*exp)
{
	PEPSON_EMU emu;
	PPRINTER_CABLE result;

	emu = calloc(1, sizeof(*emu));
	if (!emu) return NULL;

	emu->flags = flags;
	emu->exp = *exp;

	switch (emu->flags & EPSON_TEXT_RECODE_MASK) {
	case EPSON_TEXT_NO_RECODE:
		emu->recode = no_recode;
		break;
	case EPSON_TEXT_RECODE_KOI:
		emu->recode = charkoi2win;
		break;
	case EPSON_TEXT_RECODE_FX:
		emu->recode = charfx2win;
		break;
	default:
		free(emu);
		return NULL;
	}

	result = printer_emu_create(emu, epson_write, epson_hasdata, epson_flush, epson_free);
	if (!result) free(emu);

	return result;
}
