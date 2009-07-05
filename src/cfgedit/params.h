

enum {
	CMD_NONE,
	CMD_LIST,
	CMD_SETINT,
	CMD_SETSTR,
	CMD_SETDEV,
	CMD_GETICON,
	CMD_SETICON,
};

struct MAIN_CONFIG
{
	int	cmd;
	char	confname[1024];
	char	iconname[1024];
	int	slotno, indno, intval;
	char	strval[256];
	int	devtype;
};

int parse_cmdline(struct MAIN_CONFIG*cfg, int argc, const char*argv[]);
