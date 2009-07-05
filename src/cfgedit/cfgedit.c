#include <stdio.h>
#include <stdlib.h>
#include "streams.h"
#include "params.h"
#include "sysconf.h"

struct SYSCONFIG sysconf;
int need_save;

struct MAIN_CONFIG mc;

extern BOOL CreateBMPFile(LPCTSTR pszFile, HDC dc, HBITMAP hBMP);


int load_config_file(struct SYSCONFIG*c, const char*fname)
{
	int r;
	ISTREAM *in;
	in = isfopen(fname);
	if (!in) return -1;
	r = load_config(c, in);
	isclose(in);
	return r;
};


int save_config_file(struct SYSCONFIG*c, const char*fname)
{
	int r;
	OSTREAM *out;
	out = osfopen(fname);
	if (!out) return -1;
	r = save_config(c, out);
	osclose(out);
	return r;
};

int list_cmd_slot(struct SYSCONFIG*sc, struct SLOTCONFIG *s)
{
	int i;
	printf(" slot #%i\n", s->slot_no);
	printf(" dev_type: %i\n", s->dev_type);
	printf(" integer params: {");
	for (i = 0; i < CFGNINT; ++i) {
		printf("%i", s->cfgint[i]);
		if (i == CFGNINT - 1) printf("}\n");
		else printf(", ");
	}
	printf(" non-empty string params:\n");
	for (i = 0; i < CFGNSTRINGS; ++i) {
		if (s->cfgstr[i][0]) {
			printf("  [%i]: '%s'\n", i, s->cfgstr[i]);
		}
	}
	printf("\n");
	return 0;
}

int list_cmd(struct SYSCONFIG*sc)
{
	int i;
	printf("systype: %i\n", sc->systype);
	printf("slots:\n");
	for (i = 0; i < NCONFTYPES; ++i) {
		list_cmd_slot(sc, sc->slots + i);
	}
	printf("sysicon: %ix%i (length %i bytes)\n", sc->icon.w, sc->icon.h, sc->icon.imglen);
	return 0;
}

int set_int_cmd(struct SYSCONFIG*sc, int slotno, int indno, int intval)
{
	if (slotno < 0 || slotno >= NCONFTYPES) {
		fprintf(stderr, "invalid slot number %i\n", slotno);
		return 3;
	}
	if (indno < 0 || indno >= CFGNINT) {
		fprintf(stderr, "invalid integer index %i\n", indno);
		return 3;
	}
	if (sc->slots[slotno].cfgint[indno] != intval) need_save = 1;
	sc->slots[slotno].cfgint[indno] = intval;
	return 0;
}

int set_str_cmd(struct SYSCONFIG*sc, int slotno, int indno, const char*strval)
{
	if (slotno < 0 || slotno >= NCONFTYPES) {
		fprintf(stderr, "invalid slot number %i\n", slotno);
		return 3;
	}
	if (indno < 0 || indno >= CFGNINT) {
		fprintf(stderr, "invalid string index %i\n", indno);
		return 3;
	}
	if (strncmp(sc->slots[slotno].cfgstr[indno], strval, sizeof(sc->slots[slotno].cfgstr[indno]))) need_save = 1;
	strncpy(sc->slots[slotno].cfgstr[indno], strval, sizeof(sc->slots[slotno].cfgstr[indno]));
	return 0;
}

int get_icon_cmd(struct SYSCONFIG*sc, const char*iconname)
{
	HBITMAP bmp;
	HDC dc;
	dc = GetDC(GetDesktopWindow());
	bmp = sysicon_to_bitmap(dc, &sc->icon);
	if (!bmp) {
		fprintf(stderr,"can't retrieve icon information from configuration\n");
		return 3;
	}
	if (!CreateBMPFile(iconname, dc, bmp)) {
		fprintf(stderr,"can't write icon into %s\n", iconname);
		return 3;
	}
	return 0;
}

int set_icon_cmd(struct SYSCONFIG*sc, const char*iconname)
{
	HBITMAP bmp;
	HDC dc;
	dc = GetDC(GetDesktopWindow());
	bmp = LoadImage(NULL, iconname, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
	if (!bmp) {
		fprintf(stderr,"can't load icon bitmap from %s\n", iconname);
		return 3;
	}
	if (bitmap_to_sysicon(dc, bmp, &sc->icon)) {
		fprintf(stderr,"can't set configuration bitmap\n");
		return 3;
	}
	need_save = 1;
	return 0;
}

int main(int argc, const char*argv[])
{
	int res;
	res = parse_cmdline(&mc, argc, argv);
	if (res) return res;
	if (!mc.confname[0]) {
		fprintf(stderr, "%s: no configuration file specified in command line\n", argv[0]);
		return 1;
	}
	res = load_config_file(&sysconf, mc.confname);
	if (res) {
		fprintf(stderr, "%s: error loading configuration %s: %i\n", argv[0], mc.confname, res);
		return 2;
	}
	switch (mc.cmd) {
	case CMD_NONE:
		fprintf(stderr, "%s: no operation parameter specified in command line\n", argv[0]);
		return 1;
	case CMD_LIST:
		res = list_cmd(&sysconf);
		break;
	case CMD_SETINT:
		res = set_int_cmd(&sysconf, mc.slotno, mc.indno, mc.intval);
		break;
	case CMD_SETSTR:
		res = set_str_cmd(&sysconf, mc.slotno, mc.indno, mc.strval);
		break;
	case CMD_GETICON:
		res = get_icon_cmd(&sysconf, mc.iconname);
		break;
	case CMD_SETICON:
		res = set_icon_cmd(&sysconf, mc.iconname);
		break;
	}
	if (!res && need_save) {
		printf("writing configuration %s...\n", mc.confname);
		res = save_config_file(&sysconf, mc.confname);
		if (res) {
			fprintf(stderr, "%s: error writing configuration %s: %i\n", argv[0], mc.confname, res);
			return 3;
		}
		puts("done");
	}
	return res;
}
