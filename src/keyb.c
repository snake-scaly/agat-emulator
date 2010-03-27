/*
	Agat Emulator version 1.0
	Copyright (c) NOP, nnop@newmail.ru
	keyb - keyboard emulation
*/

#include <windows.h>
#include "keyb.h"
#include "streams.h"

#ifdef KEY_SCANCODES

struct KEYMAP keymap_default = {
{{{
	0x00,'`','1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=' ,0x88,0x89,
	'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', 0x8D,0x00,'A', 'S',
	'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'',0xC0,0x00,'\\','Z', 'X', 'C', 'V', 
	'B', 'N', 'M', ',', '.', '/', 0x00,'*', 0x00,' ' ,0x00,0x84,0x85,0x86,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x99,0x91,0x92,'-', 0x93,0x94,0x9C,'+', 0x9D,
	0x9E,0x9F,0x81,0x82,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
},
{
	0x00,'~','!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0x88,0x89,
	'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '{', '}', 0x8D,0x00,'a', 's', 
	'd', 'f', 'g', 'h', 'j', 'k', 'l', ':', '"', 0xDE,0x00,'|', 'z', 'x', 'c', 'v', 
	'b', 'n', 'm', '<', '>', '?', 0x00,'*', 0x00,' ', 0x00,0x84,0x85,0x86,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x90,0x91,0x92,'-', 0x93,0x94,0x9C,'+', 0x9D,
	0x9E,0x9F,0x81,0x82,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
},
{
	0x00,0x9B,'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0x82,0x89,
	0x91,0x97,0x85,0x92,0x94,0x99,0x95,0x89,0x8F,0x90,0x9B,0x9D,0x8A,0x00,0x81,0x93,
	0x84,0x86,0x87,0x88,0x8A,0x8B,0x8C,';', '\'', '"',0x00,0x9C,0x9A,0x98,0x83,0x96,
	0x82,0x8E,0x8D,',', '.', '/', 0x00,'*' ,0x00,' ', 0x00,0x84,0x85,0x86,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x90,0x91,0x92,'-', 0x93,0x94,0x9C,'+', 0x9D,
	0x9E,0x9F,0x81,0x82,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
},
{
	0x00,0x9B,'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0x82,0x89,
	0x00,0x97,0x85,0x92,0x94,0x99,0x95,0x89,0x8F,0x00,0x9B,0x9D,0x83,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x8A,0x8B,0x8C,';', '\'','"', 0x00,0x9C,0x9A,0x00,0x00,0x00,
	0x00,0x00,0x00,',', '.', '/', 0x00,0x00,0x00,' ', 0x00,0x84,0x85,0x86,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x8B,0x99,0x99,'-', 0x88,0x94,0x95,'+' ,0x8A,
	0x9A,0x9A,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
}},
{{
	0x00,0x9B,'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=' ,0x88,0x89,
	'J', 'C', 'U', 'K', 'E', 'N', 'G', '[', ']', 'Z', 'H', '_', 0x8D,0x00,'F', 'Y',
	'W', 'A', 'P', 'R', 'O', 'L', 'D', 'V', '\\',0xC0,0x00,'\\','Q', '^', 'S', 'M', 
	'I', 'T', 'X', 'B', '@', '.', 0x00,'*', 0x00,' ' ,0x00,0x84,0x85,0x86,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x99,0x91,0x92,'-', 0x93,0x94,0x9C,'+', 0x9D,
	0x9E,0x9F,0x81,0x82,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
},
{
	0x00,0x9B,'!', '"', '#', ';', '%', ':', '?', '*', '(', ')', '-', '+', 0x88,0x89,
	'j', 'c', 'u', 'k', 'e', 'n', 'g', '{', '}', 'z', 'h', 0x7F, 0x8D,0x00,'f', 'y', 
	'w', 'a', 'p', 'r', 'o', 'l', 'd', 'v', '|', 0xDE,0x00,'|', 'q', '~', 's', 'm', 
	'i', 't', 'x', 'b', '`', ',', 0x00,'*', 0x00,' ', 0x00,0x84,0x85,0x86,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x90,0x91,0x92,'-', 0x93,0x94,0x9C,'+', 0x9D,
	0x9E,0x9F,0x81,0x82,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
},
{
	0x00,0x9B,'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0x82,0x89,
	0x91,0x97,0x85,0x92,0x94,0x99,0x95,0x89,0x8F,0x90,0x9B,0x9D,0x8A,0x00,0x81,0x93,
	0x84,0x86,0x87,0x88,0x8A,0x8B,0x8C,';', '\'', '"',0x00,0x9C,0x9A,0x98,0x83,0x96,
	0x82,0x8E,0x8D,',', '.', '/', 0x00,'*' ,0x00,' ', 0x00,0x84,0x85,0x86,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x90,0x91,0x92,'-', 0x93,0x94,0x9C,'+', 0x9D,
	0x9E,0x9F,0x81,0x82,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
},
{
	0x00,0x9B,'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0x82,0x89,
	0x00,0x97,0x85,0x92,0x94,0x99,0x95,0x89,0x8F,0x00,0x9B,0x9D,0x83,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x8A,0x8B,0x8C,';', '\'','"', 0x00,0x9C,0x9A,0x00,0x00,0x00,
	0x00,0x00,0x00,',', '.', '/', 0x00,0x00,0x00,' ', 0x00,0x84,0x85,0x86,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x8B,0x99,0x99,'-', 0x88,0x94,0x95,'+' ,0x8A,
	0x9A,0x9A,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
}}}
};

//#define DEBUG_KEYS

#ifdef DEBUG_KEYS
int _decode_key(unsigned long keydata, struct KEYMAP*map, int layout);
int decode_key(unsigned long keydata, struct KEYMAP*map, int layout)
{
	int scan = (keydata>>16)&0xFF;
	int ext = keydata&(1L<<24);
	int res;
	res = _decode_key(keydata, map, layout);
	printf("data %x; scan=%x; ext=%x; res = %x\n",keydata,scan,ext,res);
	return res;
}
#else
#define _decode_key decode_key
#endif

int _decode_key(unsigned long keydata, struct KEYMAP*map, int layout)
{
	int scan = (keydata>>16)&0xFF;
	int ext = keydata&(1L<<24);
	if (scan==0x46 && ext) return -1;
	if (ext) {
		return map->data[layout][KEYMAP_EXT][scan];
	}
	if (GetKeyState(VK_CONTROL)&0x8000) {
		return map->data[layout][KEYMAP_CTRL][scan];
	} else if (GetKeyState(VK_SHIFT)&0x8000) {
		return map->data[layout][KEYMAP_SHIFT][scan];
	} else {
		return map->data[layout][KEYMAP_NORMAL][scan];
	}
}

int keymap_load(const char*fname, struct KEYMAP*map)
{
	ISTREAM*in;
	int r;
	in = isfopen(fname);
	if (!in) return -1;
	r = isread(in, map->data, sizeof(map->data));
	if (r == sizeof(map->data)/2) {
		memcpy(map->data[LAYOUT_RUS], map->data[LAYOUT_LAT], sizeof(map->data[LAYOUT_LAT]));
	} else {
		if (r != sizeof(map->data)) return -2;
	}
	isclose(in);
	return 0;
}



#else //KEY_SCANCODES

int decode_key(unsigned long keydata, struct KEYMAP*map)
{
//	printf("data=%x\n",keydata);
	switch (keydata) {
	case VK_CANCEL:
		return -1;
	case VK_LEFT:
		return 0x8;
	case VK_RIGHT:
		return 0x15;
	case VK_UP:
		return 0x19;
	case VK_DOWN:
		return 0x1A;
	case VK_RETURN:
		return 0xD;
	case VK_BACK:
		return 0x8;
	case VK_F1:
		return 4;
	case VK_F2:
		return 5;
	case VK_F3:
		return 6;
	case VK_SHIFT:
	case VK_CONTROL:
	case VK_CAPITAL:
	case VK_NUMLOCK:
	case VK_SCROLL:
		return 0;
	case VK_NUMPAD0:
		return 1;
	case VK_DECIMAL:
		return 2;
	case VK_NUMPAD1:
		return 0x1D;
	case VK_NUMPAD2:
		return 0x1E;
	case VK_NUMPAD3:
		return 0x1F;
	case VK_NUMPAD4:
		return 0x13;
	case VK_NUMPAD5:
		return 0x14;
	case VK_NUMPAD6:
		return 0x1C;
	case VK_NUMPAD7:
		return 0x10;
	case VK_NUMPAD8:
		return 0x11;
	case VK_NUMPAD9:
		return 0x12;
	case VK_MULTIPLY:
		return '*';
	case VK_ADD:
		return '+';
	case VK_SUBTRACT:
		return '-';
	case VK_DIVIDE:
		return '/';
	}
	printf("%x\n", keydata);
	if (GetKeyState(VK_CONTROL)&0x8000) {
		if (keydata>=0x40) return keydata-0x40;
		else return keydata;
	}
	if (GetKeyState(VK_SHIFT)&0x8000) {
		keydata&=0x7F;
		if (keydata>=0x40) return keydata+0x20;
		else return keydata-0x10;
	}
	return keydata;
}

#endif//KEY_SCANCODES
