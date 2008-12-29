

#define MAX_RESIZE_CONTROLS 100

enum {
	RESIZE_ALIGN_NONE,
	RESIZE_ALIGN_LEFT,
	RESIZE_ALIGN_TOP=RESIZE_ALIGN_LEFT,
	RESIZE_ALIGN_RIGHT,
	RESIZE_ALIGN_BOTTOM=RESIZE_ALIGN_RIGHT,
	RESIZE_ALIGN_CENTER
};


struct RESIZE_CONTROL
{
	int id;
	int align[2];
	int params[2];
};

#define RESIZE_LIMIT_MIN	1
#define RESIZE_LIMIT_MAX	2
#define RESIZE_NO_REALIGN	4
#define RESIZE_NO_SIZEBOX	8

struct RESIZE_DIALOG
{
	unsigned flags;
	int limits[2][2]; // [0][] - min size, [1][] - max size
	int n_controls;
	struct RESIZE_CONTROL controls[MAX_RESIZE_CONTROLS];
	int _size[2];
	HWND hsizebox;
};


int resize_init(struct RESIZE_DIALOG*r);
int resize_attach(struct RESIZE_DIALOG*r,HWND wnd);
int resize_detach(struct RESIZE_DIALOG*r,HWND wnd);
int resize_free(struct RESIZE_DIALOG*r);
int resize_sizing(struct RESIZE_DIALOG*r,HWND wnd,int side,RECT*rw);
int resize_realign(struct RESIZE_DIALOG*r,HWND wnd,int neww,int newh);
int resize_attach_control(struct RESIZE_DIALOG*r,HWND wnd,int id);

int resize_set_cfgname(const char*name);
int resize_init_placement(HWND wnd, const char*id);
int resize_save_placement(HWND wnd, const char*id);

void resize_savelistviewcolumns(HWND hlist, LPCTSTR param);
void resize_loadlistviewcolumns(HWND hlist, LPCTSTR param);
