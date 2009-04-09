/*
	Agat Emulator version 1.0
	Copyright (c) NOP, nnop@newmail.ru
	keyb - keyboard emulation
*/

#include <windows.h>

#ifdef KEY_SCANCODES
unsigned char keycodes_normal[]={
	0x00,0x9B,'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=' ,0x88,0x89,
	'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', 0x8D,0x00,'A', 'S',
	'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'',0xC0,0x00,'\\','Z', 'X', 'C', 'V', 
	'B', 'N', 'M', ',', '.', '/', 0x00,0x00,0x00,' ' ,0x00,0x84,0x85,0x86,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x90,0x91,0x92,'-', 0x93,0x94,0x9C,'+', 0x9D,
	0x9E,0x9F,0x81,0x82,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

unsigned char keycodes_shift[]={
	0x00,0x9B,'!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0x88,0x89,
	'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '{', '}', 0x8D,0x00,'a', 's', 
	'd', 'f', 'g', 'h', 'j', 'k', 'l', ':', '"', 0xDE,0x00,'|', 'z', 'x', 'c', 'v', 
	'b', 'n', 'm', '<', '>', '?', 0x00,0x00,0x00,' ', 0x00,0x84,0x85,0x86,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x90,0x91,0x92,'-', 0x93,0x94,0x9C,'+', 0x9D,
	0x9E,0x9F,0x81,0x82,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

unsigned char keycodes_ctrl[]={
	0x00,0x9B,'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0x82,0x89,
	0x91,0x97,0x85,0x92,0x94,0x99,0x95,0x89,0x8F,0x90,0x9B,0x9D,0x8A,0x00,0x81,0x93,
	0x84,0x86,0x87,0x88,0x8A,0x8B,0x8C,';', '\'', '"',0x00,0x9C,0x9A,0x98,0x83,0x96,
	0x82,0x8E,0x8D,',', '.', '/', 0x00,0x00,0x00,' ', 0x00,0x84,0x85,0x86,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x90,0x91,0x92,'-', 0x93,0x94,0x9C,'+', 0x9D,
	0x9E,0x9F,0x81,0x82,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

unsigned char keycodes_ext[]={
	0x00,0x9B,'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0x82,0x89,
	0x91,0x97,0x85,0x92,0x94,0x99,0x95,0x89,0x8F,0x90,0x9B,0x9D,0x83,0x00,0x81,0x93,
	0x84,0x86,0x87,0x88,0x8A,0x8B,0x8C,';', '\'','"', 0x00,0x9C,0x9A,0x98,0x83,0x96,
	0x82,0x8E,0x8D,',', '.', '/', 0x00,0x00,0x00,' ', 0x00,0x84,0x85,0x86,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x8C,0x99,0x99,'-', 0x88,0x94,0x95,'+' ,0x9D,
	0x9A,0x9A,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

int decode_key(unsigned long keydata)
{
	int scan = (keydata>>16)&0xFF;
	int ext = keydata&(1L<<24);
//	printf("data %x; scan=%x ext=%x\n",keydata,scan,ext);
	if (scan==0x46 && ext) return -1;
	if (ext) {
		return keycodes_ext[scan];
	}
	if (GetKeyState(VK_CONTROL)&0x8000) {
		return keycodes_ctrl[scan];
	} else if (GetKeyState(VK_SHIFT)&0x8000) {
		return keycodes_shift[scan];
	} else {
		return keycodes_normal[scan];
	}
}


#else //KEY_SCANCODES

int decode_key(unsigned long keydata)
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
