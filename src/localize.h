int	localize_init();
int	localize_set_lang(LPCTSTR lang);
int	localize_set_lib(HMODULE lib);
void	localize_term();

LPCTSTR localize_str(int modid, int strid, LPTSTR buf, int bufsize);

HMODULE localize_get_lib();
HMODULE localize_get_def_lib();

enum {
	LOC_GENERIC,
	LOC_CPU,
	LOC_FDD,
	LOC_JOYSTICK,
	LOC_SOUND,
	LOC_TAPE,
	LOC_VIDEO,
	LOC_CFG,
	LOC_MAIN,
	LOC_SYSCONF,
	LOC_PRINTER,
	LOC_MOUSE,
	LOC_SCSI,

	LOC_NMODULES
};

#define BASE_LANG_STR_ID	3000
#define N_MODULE_LANG_STR_ID	1000
