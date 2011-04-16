/*
	Agat Emulator version 1.19
	Copyright (c) NOP, nnop@newmail.ru
*/



typedef struct S_CONSOLE CONSOLE;

#ifdef UNICODE
typedef short con_char_t;
#else
typedef char con_char_t;
#endif

CONSOLE*console_create(int w,int h,con_char_t*title);
void console_free(CONSOLE*con);
int console_write(CONSOLE*con,const con_char_t*data,int len);
int console_read(CONSOLE*con,con_char_t*data,int len);

int console_gets(CONSOLE*con,con_char_t*buf,int maxlen);
int console_puts(CONSOLE*con,con_char_t*buf);
int console_puts_len(CONSOLE*con,con_char_t*buf,int len);
int console_eof(CONSOLE*con);

int console_printf(CONSOLE*con,con_char_t*fmt,...);

int console_setflags(CONSOLE*con,unsigned flags);
int console_getflags(CONSOLE*con,unsigned*flags);

int console_clear_output(CONSOLE*con);
int console_clear_input(CONSOLE*con);

int console_get_title(CONSOLE*con,con_char_t*buf,int nchars);
int console_set_title(CONSOLE*con,con_char_t*buf);

int console_hide(CONSOLE*con,int hid);


#define CONFL_CRLF 1
#define CONFL_ECHO 2
#define CONFL_KILL 4
#define CONFL_CONFIRM 8
#define CONFL_WAITCLOSE 0x10
