/*
	Agat Emulator version 1.19
	Copyright (c) NOP, nnop@newmail.ru
*/

#include "systemstate.h"
#include "runmgrint.h"

int clear_system_state_file(struct SYS_RUN_STATE*sr)
{
	int r;
	TCHAR pathname[_MAX_PATH];
	wsprintf(pathname, TEXT(SAVES_DIR"\\%s.sav"), sr->name);
	r = DeleteFile(pathname)?0:-1;
	update_save_state(sr->name);
	return r;
}

int load_system_state_file(struct SYS_RUN_STATE*sr)
{
	int r = 0;
	ISTREAM*in;
	TCHAR pathname[_MAX_PATH];

	wsprintf(pathname, TEXT(SAVES_DIR"\\%s.sav"), sr->name);

	in = isfopen(pathname);
	if (!in) {
		return -2;
	}

	system_command(sr, SYS_COMMAND_STOP, 0, 0);
	r = load_system_state(sr, in);
	if (!r) system_command(sr, SYS_COMMAND_START, 0, 0);

	isclose(in);
	update_save_state(sr->name);
	return r;
}

int save_system_state_file(struct SYS_RUN_STATE*sr)
{
	int r = 0;
	OSTREAM*out;
	TCHAR pathname[_MAX_PATH];

	wsprintf(pathname, TEXT(SAVES_DIR"\\%s.sav"), sr->name);

	out = osfopen(pathname);
	if (!out) {
		return -2;
	}

	system_command(sr, SYS_COMMAND_STOP, 0, 0);
	r = save_system_state(sr, out);
	system_command(sr, SYS_COMMAND_START, 0, 0);

	osclose(out);

	update_save_state(sr->name);
	return r;
}

