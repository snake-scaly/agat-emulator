/*
	Epson printer command emulation for Agat emulator
	Copyright (c) NOP, nnop@newmail.ru
*/

#include <windows.h>

#define EPS_BEL	7
#define EPS_BS	8
#define EPS_LF	10
#define EPS_FF	12
#define EPS_CR	13
#define EPS_SO  14
#define EPS_SI  15
#define EPS_DC2 18
#define EPS_DC4 20
#define EPS_EM	25
#define EPS_ESC 27


#define EPSON_TEXT_NO_RECODE	0
#define EPSON_TEXT_RECODE_KOI	1
#define EPSON_TEXT_RECODE_FX	2
#define EPSON_TEXT_RECODE_MASK	7



typedef struct EPSON_EMU *PEPSON_EMU;

struct EPSON_EXPORT
{
	void*param;
	void (*write_char)(void*param, int ch);
	void (*write_command)(void*param, int cmd, int nparams, unsigned char*params);
	void (*free_data)(void*param);
};

PEPSON_EMU epson_create(unsigned flags, struct EPSON_EXPORT*exp);
void epson_free(PEPSON_EMU emu);

int epson_write(PEPSON_EMU emu, unsigned char data);

