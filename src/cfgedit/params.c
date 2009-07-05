#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "params.h"

#define CMDLINE_INVPARAM (-1)
#define CMDLINE_NOVAL    (-2)
#define CMDLINE_INVVAL   (-3)
#define CMDLINE_INVARG   (-4)
#define CMDLINE_ERROR    (-5)


int show_help(struct MAIN_CONFIG*cfg, const char*prg)
{
	printf("\nUsage: %s cfgname cmd params...\n\n"
		"\t--list | -l\t\t\tList configuration parameters\n"
		"\t--setint slot ind val\t\tSet integer parameter\n"
		"\t--setstr slot ind val\t\tSet string parameter\n"
		"\t--setdev slot dev\t\tSet slot device type\n"
		"\t--geticon bmpname\t\tGet configuration icon (bitmap 64x64)\n"
		"\t--seticon bmpname\t\tSet configuration icon (bitmap 64x64)\n"
		"\t--help | -h\t\t\tShow help screen\n\n"
		"Author: Odintsov O.A., nnop@newmail.ru, 2009\n", 
		prg);
	exit(2);
}



int arg_cmp(const char*arg, const char*str, int arglen)
{
	if (arglen == strlen(str) && !memcmp(arg, str, arglen)) return 0;
	return -1;
}

const char* cmdline_errors[] = {
	NULL,
	"invalid command line parameter",
	"no required command line parameter value",
	"invalid command line parameter value",
	"invalid command line argument",
	"command line error"
};

int parse_longpar(struct MAIN_CONFIG*cfg, const char*arg, int argno, int argc, const char*argv[])
{
	const char*eqp;
	int arglen;
	int r;
	eqp = strchr(arg, '=');
	arglen = eqp?eqp - arg: strlen(arg);
	if (!arg_cmp(arg, "list", arglen)) {
		cfg->cmd = CMD_LIST;
		return 0;
	} else if (!arg_cmp(arg, "setint", arglen)) {
		if (eqp) {
			return CMDLINE_INVPARAM;
		} else {
			if (argc <= argno + 3) return CMDLINE_NOVAL;
			cfg->cmd = CMD_SETINT;
			cfg->slotno = atoi(argv[argno + 1]);
			cfg->indno = atoi(argv[argno + 2]);
			cfg->intval = atoi(argv[argno + 3]);
			return 3;
		}
	} else if (!arg_cmp(arg, "setstr", arglen)) {
		if (eqp) {
			return CMDLINE_INVPARAM;
		} else {
			if (argc <= argno + 3) return CMDLINE_NOVAL;
			cfg->cmd = CMD_SETSTR;
			cfg->slotno = atoi(argv[argno + 1]);
			cfg->indno = atoi(argv[argno + 2]);
			strncpy(cfg->strval, argv[argno + 3], sizeof(cfg->strval) - 1);
			return 3;
		}
	} else if (!arg_cmp(arg, "setdev", arglen)) {
		if (eqp) {
			return CMDLINE_INVPARAM;
		} else {
			if (argc <= argno + 2) return CMDLINE_NOVAL;
			cfg->cmd = CMD_SETDEV;
			cfg->slotno = atoi(argv[argno + 1]);
			cfg->devtype = atoi(argv[argno + 2]);
			return 2;
		}
	} else if (!arg_cmp(arg, "geticon", arglen)) {
		if (eqp) {
			return CMDLINE_INVPARAM;
		} else {
			if (argc <= argno + 1) return CMDLINE_NOVAL;
			cfg->cmd = CMD_GETICON;
			strncpy(cfg->iconname, argv[argno + 1], sizeof(cfg->iconname) - 1);
			return 1;
		}
	} else if (!arg_cmp(arg, "seticon", arglen)) {
		if (eqp) {
			return CMDLINE_INVPARAM;
		} else {
			if (argc <= argno + 1) return CMDLINE_NOVAL;
			cfg->cmd = CMD_SETICON;
			strncpy(cfg->iconname, argv[argno + 1], sizeof(cfg->iconname) - 1);
			return 1;
		}
	} else if (!arg_cmp(arg, "help", arglen)) {
		return show_help(cfg, argv[0]);
	} else return CMDLINE_INVPARAM;
	
//	printf("long parameter: %s\n", arg);
	return 0;
}

int parse_shortpar(struct MAIN_CONFIG*cfg, char arg, int argpos, int argno, int argc, const char*argv[])
{
	switch (arg) {
	case 'l':
		cfg->cmd = CMD_LIST;
		break;
	case 'h':
	case '?':
		return show_help(cfg, argv[0]);
	default:
		return CMDLINE_INVPARAM;
	}
//	printf("short parameter: %c\n", arg);
	return 0;
}

int parse_arg(struct MAIN_CONFIG*cfg, const char*arg, int argno, int argc, const char*argv[])
{
	if (!cfg->confname[0]) {
		strncpy(cfg->confname, arg, sizeof(cfg->confname) - 1);
		return 0;
	}
	return CMDLINE_INVARG;
}

int parse_cmdline(struct MAIN_CONFIG*cfg, int argc, const char*argv[])
{
	int i, r;
	for (i = 1; i < argc; ++i) {
		if (argv[i][0] == '-' || argv[i][0] == '/') {
			if (argv[i][1] == '-' && argv[i][0] == '-') {
				r = parse_longpar(cfg, argv[i] + 2, i, argc, argv);
				if (r < 0) {
					fprintf(stderr, "%s: %s: %s\n", argv[0], cmdline_errors[-r], argv[i]);
					return r;
				}
				i += r;
			} else {
				int j, l = strlen(argv[i]);
				for (j = 1; j < l; ++j) {
					r = parse_shortpar(cfg, argv[i][j], j, i, argc, argv);
					if (r < 0) {
						fprintf(stderr, "%s: %s: %s\n", argv[0], cmdline_errors[-r], argv[i]);
						return r;
					}
					j += r;
				}
			}
		} else {
			r = parse_arg(cfg, argv[i], i, argc, argv);
			if (r < 0) {
				fprintf(stderr, "%s: %s: %s\n", argv[0], cmdline_errors[-r], argv[i]);
				return r;
			}
			i += r;
		}
	}
	return 0;
}

