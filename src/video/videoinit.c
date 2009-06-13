#include "videoint.h"

static int load_font_res(int no, void*buf)
{
	return load_buf_res(no, buf, 256*8);
}

static int video_term(struct SLOT_RUN_STATE*st)
{
	free(st->data);
	return 0;
}

static int video_save(struct SLOT_RUN_STATE*st, OSTREAM*out)
{
	struct VIDEO_STATE*vs = st->data;

	WRITE_FIELD(out, vs->video_mode);

	WRITE_ARRAY(out, vs->rb);
	WRITE_FIELD(out, vs->n_rb);
	WRITE_FIELD(out, vs->rbi);
	WRITE_FIELD(out, vs->rb_enabled);
	WRITE_FIELD(out, vs->rb_cur);

	WRITE_FIELD(out, vs->pal);

	WRITE_FIELD(out, vs->ainf.text_mode);
	WRITE_FIELD(out, vs->ainf.combined);
	WRITE_FIELD(out, vs->ainf.page);
	WRITE_FIELD(out, vs->ainf.hgr);

	return 0;
}

static int video_load(struct SLOT_RUN_STATE*st, ISTREAM*in)
{
	struct VIDEO_STATE*vs = st->data;

	READ_FIELD(in, vs->video_mode);

	READ_ARRAY(in, vs->rb);
	READ_FIELD(in, vs->n_rb);
	READ_FIELD(in, vs->rbi);
	READ_FIELD(in, vs->rb_enabled);
	READ_FIELD(in, vs->rb_cur);

	READ_FIELD(in, vs->pal);

	READ_FIELD(in, vs->ainf.text_mode);
	READ_FIELD(in, vs->ainf.combined);
	READ_FIELD(in, vs->ainf.page);
	READ_FIELD(in, vs->ainf.hgr);

	video_update_mode(vs);
	video_first_rb(vs);
	return 0;
}

static int video_command(struct SLOT_RUN_STATE*st, int cmd, int data, long param)
{
	struct VIDEO_STATE*vs = st->data;
	switch (cmd) {
	case SYS_COMMAND_RESET:
		disable_ints(vs->sr);
		return 0;
	case SYS_COMMAND_HRESET:
		disable_ints(vs->sr);
		video_set_pal(&vs->pal, 0);
//		videosel(0);
		return 0;
	case SYS_COMMAND_FLASH:
		video_flash_text(vs);
		return 0;
	case SYS_COMMAND_REPAINT:
		video_repaint_screen(vs);
		return 0;
	case SYS_COMMAND_TOGGLE_MONO:
		video_set_mono(vs, 1, 1);
		return 0;
	case SYS_COMMAND_CPUTIMER:
		if ((param|1) == (((long)vs)|1)) {
			video_timer(vs, param & 1);
			return 1;
		}
		break;
		
	}
	return 0;
}

static void videosel_w(word adr, byte data, struct VIDEO_STATE*vs) // C700-C7FF
{
	videosel(vs, adr&0xFF);
}

static byte videosel_r(word adr, struct VIDEO_STATE*vs) // C700-C7FF
{
	return videosel(vs, adr&0xFF);
}

static void vsel_ap_w(word adr, byte data, struct VIDEO_STATE*vs) // C050-C05F
{
	vsel_ap(vs, adr);
}

static byte vsel_ap_r(word adr, struct VIDEO_STATE*vs) // C050-C05F
{
	vsel_ap(vs, adr);
	return empty_read(adr, NULL);
}


void enable_ints_w(word adr,byte data, struct VIDEO_STATE*vs) // C040-C04F
{
	printf("enable ints_w: %x\n",adr);
	enable_ints(vs->sr);
}

byte enable_ints_r(word adr, struct VIDEO_STATE*vs) // C040-C04F
{
	printf("enable ints_r: %x\n",adr);
	enable_ints(vs->sr);
	return empty_read_addr(adr, vs);
}



void disable_ints_w(word adr,byte data, struct VIDEO_STATE*vs) // C050-C05F
{
//	printf("disable ints_w: %x\n",adr);
	disable_ints(vs->sr);
}

byte disable_ints_r(word adr, struct VIDEO_STATE*vs) // C050-C05F
{
//	printf("disable ints_r: %x\n",adr);
	disable_ints(vs->sr);
	return empty_read_addr(adr, vs);
}

/*
byte toggle_ints_r(word adr, struct VIDEO_STATE*vs) // C050-C05F
{
	printf("toggle ints_r: %x\n",adr);
	toggle_ints(vs->sr);
	return empty_read(adr, vs);
}

void toggle_ints_w(word adr,byte data, struct VIDEO_STATE*vs) // C050-C05F
{
	printf("toggle ints_w: %x\n",adr);
	toggle_ints(vs->sr);
}
*/

static void set_palette_w(word adr,byte data, struct VIDEO_STATE*vs) // C050-C05F, agat 9
{
//	printf("set palette: %x",adr);
	if (adr&8) video_set_palette(vs, adr&7);
	else vsel_ap_w(adr, data, vs);
}


static byte set_palette_r(word adr, struct VIDEO_STATE*vs) // C050-C05F, agat 9
{
//	printf("get palette: %x",adr);
	if (adr&8) video_set_palette(vs, adr&7);
	else vsel_ap_r(adr, vs);
	return (adr&8&&!(adr&4))?0:0xFF;//empty_read(adr,d);
}


int set_rb_count(struct VIDEO_STATE*vs, int n)
{
	struct RASTER_BLOCK*rb;
	int i;

	vs->n_rb = n;
	for (i = n, rb = vs->rb; i; --i, ++rb) {
		rb->vmode = -1;
		rb->vtype = -1;
		rb->prev_base = -1;
	}
	if (vs->rb_enabled) {
		system_command(vs->sr, SYS_COMMAND_SET_CPUTIMER, 20000, ((long)vs) & ~1);
		system_command(vs->sr, SYS_COMMAND_SET_CPUTIMER, 20000 / n, ((long)vs) | 1);
	}
	return 0;
}

int  video_init(struct SYS_RUN_STATE*sr)
{
	int i;
	ISTREAM*s;
	struct VIDEO_STATE*vs;
	struct SLOT_RUN_STATE*st = sr->slots + CONF_CHARSET;

	puts("video_init");

	vs = calloc(1, sizeof(*vs));
	if (!vs) return -1;


	vs->sr = sr;
	vs->video_mode = VIDEO_MODE_INVALID;
	vs->ainf.hgr = 1;
	vs->pal.prev_pal = -1;
	vs->rb_enabled = 0;

	puts(sr->config->slots[CONF_CHARSET].cfgstr[CFG_STR_ROM]);
	s=isfopen(sr->config->slots[CONF_CHARSET].cfgstr[CFG_STR_ROM]);
	if (!s) {
		load_font_res(sr->config->slots[CONF_CHARSET].cfgint[CFG_INT_ROM_RES], vs->font[0]);
	} else {
		if (isread(s,vs->font[0],sizeof(vs->font))!=sizeof(vs->font)) {
			WIN_ERROR(GetLastError(),TEXT("can't read font file"));
		}
		isclose(s);
	}

	st->data = vs;
	st->free = video_term;
	st->command = video_command;
	st->load = video_load;
	st->save = video_save;

	video_set_pal(&vs->pal, 0);
	switch (sr->cursystype) {
	case SYSTEM_7:
		set_rb_count(vs, vs->rb_enabled?N_RB_7:1);
		video_set_mode(vs, VIDEO_MODE_AGAT);
		videosel(vs, 0);
		fill_read_proc(sr->io_sel + 7, 1, videosel_r, vs);
		fill_write_proc(sr->io_sel + 7, 1, videosel_w, vs);
		fill_rw_proc(sr->baseio_sel + 4, 1, enable_ints_r, enable_ints_w, vs);
		fill_rw_proc(sr->baseio_sel + 5, 1, disable_ints_r, disable_ints_w, vs);
		break;
	case SYSTEM_9:
		set_rb_count(vs, vs->rb_enabled?N_RB_9:1);
		video_set_mode(vs, VIDEO_MODE_AGAT);
		videosel(vs, 0);
		fill_read_proc(sr->io_sel + 7, 1, videosel_r, vs);
		fill_write_proc(sr->io_sel + 7, 1, videosel_w, vs);
		fill_rw_proc(sr->baseio_sel + 2, 1, disable_ints_r, disable_ints_w, vs);
		fill_rw_proc(sr->baseio_sel + 4, 1, enable_ints_r, enable_ints_w, vs);
		fill_rw_proc(sr->baseio_sel + 5, 1, set_palette_r, set_palette_w, vs);
		goto l1;
		break;
	case SYSTEM_A:
		set_rb_count(vs, 1);
		video_set_mode(vs, VIDEO_MODE_APPLE);
		update_video_ap(vs);
		fill_read_proc(sr->baseio_sel + 5, 1, vsel_ap_r, vs);
		fill_write_proc(sr->baseio_sel + 5, 1, vsel_ap_w, vs);
	l1:
		vs->ainf.page = 0;
		video_set_mono(vs, 0, sr->config->slots[CONF_MONITOR].dev_type == DEV_MONO);
		break;
	}
	return 0;
}
