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
	WRITE_FIELD(out, vs->video_base_addr);
	WRITE_FIELD(out, vs->prev_base);
	WRITE_FIELD(out, vs->video_mem_size);
	WRITE_FIELD(out, vs->video_el_size);

	WRITE_FIELD(out, vs->vid_mode);
	WRITE_FIELD(out, vs->vid_type);
	WRITE_FIELD(out, vs->flash_mode);
	WRITE_FIELD(out, vs->cur_mono);
	WRITE_ARRAY(out, vs->c1_palette);
	WRITE_ARRAY(out, vs->c2_palette);
	WRITE_ARRAY(out, vs->c4_palette);

	WRITE_FIELD(out, vs->text_mode);
	WRITE_FIELD(out, vs->combined);
	WRITE_FIELD(out, vs->page);
	WRITE_FIELD(out, vs->hgr);

	WRITE_FIELD(out, vs->prev_pal);


	return 0;
}

static int video_load(struct SLOT_RUN_STATE*st, ISTREAM*in)
{
	struct VIDEO_STATE*vs = st->data;

	READ_FIELD(in, vs->video_mode);
	READ_FIELD(in, vs->video_base_addr);
	READ_FIELD(in, vs->prev_base);
	READ_FIELD(in, vs->video_mem_size);
	READ_FIELD(in, vs->video_el_size);

	READ_FIELD(in, vs->vid_mode);
	READ_FIELD(in, vs->vid_type);
	READ_FIELD(in, vs->flash_mode);
	READ_FIELD(in, vs->cur_mono);
	READ_ARRAY(in, vs->c1_palette);
	READ_ARRAY(in, vs->c2_palette);
	READ_ARRAY(in, vs->c4_palette);

	READ_FIELD(in, vs->text_mode);
	READ_FIELD(in, vs->combined);
	READ_FIELD(in, vs->page);
	READ_FIELD(in, vs->hgr);

	READ_FIELD(in, vs->prev_pal);

	video_update_mode(vs);
	video_repaint_screen(vs);
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
		video_set_pal(vs, 0);
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
	return empty_read(adr, vs);
}



void disable_ints_w(word adr,byte data, struct VIDEO_STATE*vs) // C050-C05F
{
	printf("disable ints_w: %x\n",adr);
	disable_ints(vs->sr);
}

byte disable_ints_r(word adr, struct VIDEO_STATE*vs) // C050-C05F
{
	printf("disable ints_r: %x\n",adr);
	disable_ints(vs->sr);
	return empty_read(adr, vs);
}


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


static void set_palette_w(word adr,byte data, struct VIDEO_STATE*vs) // C050-C05F, agat 9
{
//	printf("set palette: %x",adr);
	if (adr&8) video_set_palette(vs, adr&7);
	else vsel_ap_w(adr, data, vs);
	disable_ints_w(adr, data, vs);
}


static byte set_palette_r(word adr, struct VIDEO_STATE*vs) // C050-C05F, agat 9
{
//	printf("get palette: %x",adr);
	if (adr&8) video_set_palette(vs, adr&7);
	else vsel_ap_r(adr, vs);
	disable_ints_r(adr, vs);
	return (adr&8&&!(adr&4))?0:0xFF;//empty_read(adr,d);
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
	vs->prev_base = -1;
	vs->vid_mode = -1;
	vs->vid_type = -1;
	vs->hgr = 1;
	vs->prev_pal = -1;

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

	video_set_pal(vs, 0);
	switch (sr->cursystype) {
	case SYSTEM_7:
		video_set_mode(vs, VIDEO_MODE_AGAT);
		videosel(vs, 0);
		fill_read_proc(sr->io_sel + 7, 1, videosel_r, vs);
		fill_write_proc(sr->io_sel + 7, 1, videosel_w, vs);
		fill_rw_proc(sr->baseio_sel + 4, 1, enable_ints_r, enable_ints_w, vs);
		fill_rw_proc(sr->baseio_sel + 5, 1, disable_ints_r, disable_ints_w, vs);
		break;
	case SYSTEM_9:
		video_set_mode(vs, VIDEO_MODE_AGAT);
		videosel(vs, 0);
		fill_read_proc(sr->io_sel + 7, 1, videosel_r, vs);
		fill_write_proc(sr->io_sel + 7, 1, videosel_w, vs);
		fill_rw_proc(sr->baseio_sel + 4, 1, toggle_ints_r, toggle_ints_w, vs);
		fill_rw_proc(sr->baseio_sel + 5, 1, set_palette_r, set_palette_w, vs);
		goto l1;
		break;
	case SYSTEM_A:
		video_set_mode(vs, VIDEO_MODE_APPLE);
		update_video_ap(vs);
		fill_read_proc(sr->baseio_sel + 5, 1, vsel_ap_r, vs);
		fill_write_proc(sr->baseio_sel + 5, 1, vsel_ap_w, vs);
	l1:
		vs->page = 0;
		video_set_mono(vs, 0, sr->config->slots[CONF_MONITOR].dev_type == DEV_MONO);
		break;
	}
	return 0;
}
