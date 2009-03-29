/*
	Epson printer command emulation for Agat emulator
	Copyright (c) NOP, nnop@newmail.ru
*/

#include "epson_emu.h"
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#define EPS_BEL	7
#define EPS_FF	12
#define EPS_SO  14
#define EPS_SI  15
#define EPS_DC2 18
#define EPS_DC4 20
#define EPS_EM	25
#define EPS_ESC 27


struct EPSON_EMU
{
	int opened;
	int esc_mode;
	int esc_cnt;
	int esc_cmd;
	FILE*out;
	int nfile;
};

PEPSON_EMU epson_create(unsigned flags)
{
	PEPSON_EMU emu = calloc(1, sizeof(*emu));
	if (!emu) return NULL;

	return emu;
}

void epson_free(PEPSON_EMU emu)
{
	if (!emu) return;
	if (emu->opened) epson_close(emu);
	free(emu);
}


int epson_open(PEPSON_EMU emu, unsigned flags)
{
	char fname[1024];
	if (!emu) return -1;
	if (emu->opened) return 1;
	++emu->nfile;
	sprintf(fname, "spool%03i.bin", emu->nfile);
	emu->out = fopen(fname, "wb");
	if (!emu->out) return -1;
	emu->opened = 1;

	return 0;
}

int epson_close(PEPSON_EMU emu)
{
	if (!emu) return -1;
	if (!emu->opened) return 1;
	if (emu->out) fclose(emu->out);
	emu->out = NULL;
	emu->opened = 0;
	return 0;
}

int epson_command0b(PEPSON_EMU emu, unsigned char cmd)
{
	switch (cmd) {
	case '0':
		puts("Set line spacing to 1/8 inch");
		break;
	case '1':
		puts("Set line spacing to 7/72 inch");
		break;
	case '2':
		puts("Set line spacing to 1/6 inch");
		break;
	case '4':
		puts("Select italic font");
		break;
	case '5':
		puts("Cancel italic font");
		break;
	case '6':
		puts("Extend charset");
		break;
	case '7':
		puts("Reduce charset");
		break;
	case '8':
		puts("Disable paper-out detector");
		break;
	case '9':
		puts("Enable paper-out detector");
		break;
	case 'E':
		puts("Select bold font");
		break;
	case 'F':
		puts("Cancel bold font");
		break;
	case 'G':
		puts("Select double-strike font");
		break;
	case 'H':
		puts("Cancel double-strike font");
		break;
	case 'M':
		puts("Enable 12-cpi printing");
		break;
	case 'O':
		puts("Cancel top and bottom margin setting");
		break;
	case 'P':
		puts("Enable 10-cpi printing");
		break;
	case 'T':
		puts("Cancel superscript/subscript");
		break;
	case 'g':
		puts("Enable 15-cpi printing");
		break;
	case EPS_SI:
		puts("Select condensed printing");
		break;
	case EPS_SO:
		puts("Select double width printing");
		break;
	case '<':
		puts("Print one line unidirectionally");
		break;
	default:
		return 0;
	}
	emu->esc_mode = 0;
	return 0;
}

int epson_command1b(PEPSON_EMU emu, unsigned char cmd, unsigned char data)
{
	switch (cmd) {
	case ' ':
		printf("Select space between chars to %i\n", data);
		break;
	case '!':
		printf("Master select: %02X\n", data);
		break;
	case '-':
		printf("Select underline mode: %i\n", data);
		break;
	case '+':
		printf("Set line spacing to %i/360 inches\n", data);
		break;
	case '3':
		printf("Set line spacing to %i/216 inches\n", data);
		break;
	case 'A':
		printf("Set line spacing to %i/72 inch\n", data);
		break;
	case 'a':
		printf("Select justification %i\n", data);
		break;
	case 'B':
		printf("Setting vertical tabs...\n");
		if (data) return 0;
		break;
	case 'C':
		if (data) {
			printf("Setting page length to %i lines\n", data);
		} else return 0;
		break;
	case 'D':
		printf("Setting horizontal tabs...\n");
		if (data) return 0;
		break;
	case 'e':
		printf("Setting fixed tab increment...\n");
		return 0;
	case 'I':
		printf("Select charset extension %i\n", data);
		break;
	case 'J':
		printf("Advance vertical position by %i/180 inches\n", data);
		break;
	case 'k':
		printf("Select typeface %i\n", data);
		break;
	case 'l':
		printf("Set left margin %i\n", data);
		break;
	case 'm':
		printf("Select charset extension %i\n", data);
		break;
	case 'N':
		printf("Set bottom margin %i\n", data);
		break;
	case 'p':
		printf("Select proportional mode %i\n", data);
		break;
	case 'Q':
		printf("Set right margin %i\n", data);
		break;
	case 'q':
		printf("Select outline/shadow mode %i\n", data);
		break;
	case 'R':
		printf("Select character set %i\n", data);
		break;
	case 'S':
		printf("Select superscript/subscript: %i\n", data);
		break;
	case 's':
		printf("Select speed mode %i\n", data);
		break;
	case 't':
		printf("Select character table %i\n", data);
		break;
	case 'U':
		printf("Select unidirectional printing %i\n", data);
		break;
	case 'W':
		printf("Select double-width printing %i\n", data);
		break;
	case 'w':
		printf("Select double-height printing %i\n", data);
		break;
	case 'x':
		printf("Select print quality %i\n", data);
		break;
	case '/':
		printf("Select vertical tab channel %i\n", data);
		break;
	case '%':
		printf("Select user defined character set %i\n", data);
		break;
	case '.':
		printf("Entering graphic mode...\n", data);
		break;
	case '*':
		printf("Printing bit image...\n", data);
		break;
	case EPS_EM:
		printf("Select cut-sheet %i\n", data);
		break;
	}
	emu->esc_mode = 0;
	return 0;
}

int epson_commandxb(PEPSON_EMU emu, unsigned char cmd, unsigned char data, int no)
{
	switch (cmd) {
	case 'B':
		if (!data) emu->esc_mode = 0;
		break;
	case 'C':
		printf("Setting page length to %i inches\n", data);
		emu->esc_mode = 0;
		break;
	case 'D':
		if (!data) emu->esc_mode = 0;
		break;
	case 'e':
		if (no == 3) emu->esc_mode = 0;
		break;
	}
	return 0;
}

int epson_write(PEPSON_EMU emu, unsigned char data)
{
	int r;
	if (!emu) return -1;
	if (!emu->opened) {
		r = epson_open(emu, 0);
		if (r) return r;
	}
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
			puts("Select condensed printing");
			break;
		case EPS_DC2:
			puts("Cancel condensed printing");
			break;
		case EPS_SO:
			puts("Select double-width printing");
			break;
		case EPS_DC4:
			puts("Cancel double-width printing");
			break;
		case EPS_BEL:
			puts("Printer beep");
			break;
		case EPS_FF:
			puts("Printer form feed");
			break;
		default:
			fwrite(&data, 1, 1, emu->out);
		}
	}
	return 0;
}

