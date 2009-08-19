/*
	Agat Emulator version 1.0
	Copyright (c) NOP, nnop@newmail.ru
	keyb - keyboard emulation
*/

enum {
	KEYMAP_NORMAL,
	KEYMAP_SHIFT,
	KEYMAP_CTRL,
	KEYMAP_EXT,

	N_KEYMAPS
};

struct KEYMAP
{
	unsigned char data[N_KEYMAPS][0x80];
};

extern struct KEYMAP keymap_default;

int decode_key(unsigned long keydata, struct KEYMAP*map);

int keymap_load(const char*fname, struct KEYMAP*map);
