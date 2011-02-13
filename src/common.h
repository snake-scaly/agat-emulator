/*
	Agat Emulator version 1.0
	Copyright (c) NOP, nnop@newmail.ru
	common - common function declarations
*/


void usleep(int microsec);
void msleep(int msec);
unsigned get_n_msec();
const char*sys_get_parameter(const char*name);

int load_buf_res(int no, void*buf, int len);
