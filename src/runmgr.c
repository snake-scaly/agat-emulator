#include "runmgr.h"
#include "runmgrint.h"
#include "runstate.h"

#include "resource.h"


static void io6_write(word adr, byte data, struct MEM_PROC*io6_sel); // c060-c06f
static byte io6_read(word adr, struct MEM_PROC*io6_sel); // c060-c06f
static void io_write(word adr, byte data, struct MEM_PROC*io_sel); // C000-C7FF
static byte io_read(word adr, struct MEM_PROC*io_sel); // C000-C7FF
static void baseio_write(word adr, byte data, struct MEM_PROC*baseio_sel); // C000-C0FF
static byte baseio_read(word adr, struct MEM_PROC*baseio_sel);	// C000-C0FF

static byte keyb_read(word adr, struct SYS_RUN_STATE*sr);	// C000-C00F
byte keyb_reg_read(word adr, struct SYS_RUN_STATE*sr);	// C063
void keyb_clear(struct SYS_RUN_STATE*sr);	// C010-C01F
static byte keyb_clear_r(word adr, struct SYS_RUN_STATE*sr);	// C010-C01F
static void keyb_clear_w(word adr, byte d, struct SYS_RUN_STATE*sr);	// C010-C01F


int init_slot_state(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*st, struct SLOTCONFIG*sc)
{
//	printf("********** slot:dev =  %i:%i\n", st->sc->slot_no, st->sc->dev_type);
	switch (st->sc->slot_no) {
	case CONF_ROM:
		puts("rom_install");
		return rom_install(sr, st, sc);
	case CONF_MEMORY:
		puts("ram_install");
		return ram_install(sr, st, sc);
	case CONF_CPU:
		puts("cpu_init");
		return cpu_init(sr, st, sc);
	case CONF_SOUND:
		puts("sound_init");
		return sound_init(sr, st, sc);
	case CONF_TAPE:
		puts("tape_init");
		return tape_init(sr, st, sc);
	case CONF_JOYSTICK:
		puts("joystick_init");
		return joystick_init(sr, st, sc);
	}
	switch (st->sc->dev_type) {
	case DEV_FDD_SHUGART:
		puts("fdd1_init");
		return fdd1_init(sr, st, sc);
	case DEV_FDD_TEAC:
		puts("fdd_init");
		return fdd_init(sr, st, sc);
	case DEV_MEMORY_XRAM7:
		puts("xram7_init");
		return xram7_init(sr, st, sc);
	case DEV_MEMORY_PSROM7:
		puts("psrom7_init");
		return psrom7_init(sr, st, sc);
	case DEV_MEMORY_XRAM9:
		puts("xram9_init");
		return xram9_init(sr, st, sc);
	}
	return 0;
}

int slot_command(struct SLOT_RUN_STATE*st, int id, int data, long param)
{
	if (st->command) return st->command(st, id, data, param);
	else return 0;
}

int save_slot_state(struct SLOT_RUN_STATE*st, OSTREAM*out)
{
	if (st->save) return st->save(st, out);
	else return 0;
}

int load_slot_state(struct SLOT_RUN_STATE*st, ISTREAM*in)
{
	if (st->load) return st->load(st, in);
	else return 0;
}

int free_slot_state(struct SLOT_RUN_STATE*st)
{
	if (st->free) return st->free(st);
	else return -1;
}



struct SYS_RUN_STATE *init_system_state(struct SYSCONFIG*c, HWND hmain, LPCTSTR name)
{
	struct SYS_RUN_STATE*sr;
	int i, r;

	sr = calloc(1, sizeof(*sr));
	if (!sr) return NULL;
	sr->name = name?_tcsdup(name):NULL;
	sr->config = c;
	sr->cursystype = c->systype;
	sr->base_w = hmain;


	fill_read_proc(sr->base_mem, BASEMEM_NBLOCKS - 6, empty_read, NULL);
	fill_read_proc(sr->base_mem + BASEMEM_NBLOCKS - 6, 6, empty_read_addr, NULL);
	fill_write_proc(sr->base_mem, BASEMEM_NBLOCKS, empty_write, NULL);
	fill_read_proc(sr->baseio_sel, 16, empty_read, NULL);
	fill_write_proc(sr->baseio_sel, 16, empty_write, NULL);
	fill_read_proc(sr->io_sel, 8, empty_read, NULL);
	fill_write_proc(sr->io_sel, 8, empty_write, NULL);
	fill_read_proc(sr->io6_sel, 16, empty_read, NULL);
	fill_write_proc(sr->io6_sel, 16, empty_write, NULL);

	fill_read_proc(sr->base_mem + 24, 1, io_read, sr->io_sel);
	fill_write_proc(sr->base_mem + 24, 1, io_write, sr->io_sel);
	fill_read_proc(sr->io_sel, 1, baseio_read, sr->baseio_sel);
	fill_write_proc(sr->io_sel, 1, baseio_write, sr->baseio_sel);
	fill_read_proc(sr->baseio_sel + 6, 1, io6_read, sr->io6_sel);
	fill_write_proc(sr->baseio_sel + 6, 1, io6_write, sr->io6_sel);

	fill_read_proc(sr->baseio_sel + 0, 1, keyb_read, sr);
	fill_read_proc(sr->io6_sel + 3, 1, keyb_reg_read, sr);
	fill_read_proc(sr->baseio_sel + 1, 1, keyb_clear_r, sr);
	fill_write_proc(sr->baseio_sel + 1, 1, keyb_clear_w, sr);

	set_run_state_ptr(sr->name, sr);

	sr->keyreg = 0xFF;

	r = init_video_window(sr);
	if (r < 0) goto fail;
	if (sr->name) {
		TCHAR title[1024];
		GetWindowText(sr->video_w, title, 1024);
		lstrcat(title, TEXT(" — "));
		lstrcat(title, sr->name);
		SetWindowText(sr->video_w, title);
	}


	for (i = 0; i < NCONFTYPES; i++ ) {
		sr->slots[i].sc = c->slots + i;
		sr->slots[i].sr = sr;
		if (i <= CONF_SLOT6) {
			sr->slots[i].baseio_sel = sr->baseio_sel + i + 8;
			sr->slots[i].io_sel = sr->io_sel + i;
		}
		r = init_slot_state(sr, sr->slots + i, c->slots + i);
		if (r < 0) goto fail;
	}

	r = video_init(sr);
	if (r < 0) goto fail;

	system_command(sr, SYS_COMMAND_INITMENU, 0, (long)sr->popup_menu);

	PostMessage(sr->base_w, WM_COMMAND, MAKEWPARAM(IDC_UPDATE,0), 0);
	return sr;
fail:
	puts("failure");
	{
		int j;

		if (sr->popup_menu)
			system_command(sr, SYS_COMMAND_FREEMENU, 0, (long)sr->popup_menu);
		set_run_state_ptr(sr->name, NULL);
		PostMessage(sr->base_w, WM_COMMAND, MAKEWPARAM(IDC_UPDATE, 0), 0);

		for (j = 0; j < i ; j ++ ) {
			free_slot_state(sr->slots + j);
		}
		if (sr->name) free((void*)sr->name);
		free(sr);
		return NULL;
	}
}

int free_system_state(struct SYS_RUN_STATE*sr)
{
	int i;

	if (!sr) return -1;

	if (sr->popup_menu)
		system_command(sr, SYS_COMMAND_FREEMENU, 0, (long)sr->popup_menu);

	PostMessage(sr->base_w, WM_COMMAND, MAKEWPARAM(IDC_UPDATE,0), 0);

	and_run_state_flags(sr->name, ~RUNSTATE_RUNNING);
	set_run_state_ptr(sr->name, NULL);

	term_video_window(sr);

	for (i = 0; i < NCONFTYPES; i++) {
		free_slot_state(sr->slots + i);
	}
	if (sr->name) free((void*)sr->name);
	free(sr);
	return 0;
}

int system_command(struct SYS_RUN_STATE*sr, int id, int data, long param)
{
	int i, r = 0;
	if (!sr) return -1;
	for (i = 0; i < NCONFTYPES ; i++) {
		int r0;
		r0 = slot_command(sr->slots + i, id, data, param);
//		printf("slot_command(%i, %i, %i, %i) = %i\n", i, id, data, param, r0);
		if (r0 < 0) r = r0;
		if (r0 > 0) { r = r0; break; }
	}
	switch (id) {
	case SYS_COMMAND_START:
		or_run_state_flags(sr->name, RUNSTATE_RUNNING);
		PostMessage(sr->base_w, WM_COMMAND, MAKEWPARAM(IDC_UPDATE,0), 0);
		break;
	case SYS_COMMAND_STOP:
		and_run_state_flags(sr->name, ~RUNSTATE_RUNNING);
		PostMessage(sr->base_w, WM_COMMAND, MAKEWPARAM(IDC_UPDATE,0), 0);
		break;
	case SYS_COMMAND_ACTIVATE:
		ShowWindow(sr->video_w, SW_SHOWNORMAL);
		SetActiveWindow(sr->video_w);
		break;
	}
	return r;
}

#define MAKEDWORD(a,b) (((a)&0xFFFF) | (((b) & 0xFFFF)<<16))

int save_system_state(struct SYS_RUN_STATE*sr, OSTREAM*out)
{
	int i, r = 0;
	if (!sr) return -1;
	for (i = 0; i < NCONFTYPES; i++) {
		int r0;
		DWORD bb = MAKEDWORD(i, ~i);
		r0 = oswrite(out, &bb, sizeof(bb));
		if (r0 != sizeof(bb)) return -1;
		r0 = save_slot_state(sr->slots + i, out);
		if (r0 < 0) return r;
	}
	WRITE_FIELD(out, sr->ints_enabled);
	return r;
}

int load_system_state(struct SYS_RUN_STATE*sr, ISTREAM*in)
{
	int i, r = 0;
	if (!sr) return -1;
	system_command(sr, SYS_COMMAND_FREEMENU, 0, (long)sr->popup_menu);
	for (i = 0; i < NCONFTYPES; i++) {
		int r0;
		DWORD bb = MAKEDWORD(i, ~i), bb1;
		r0 = isread(in, &bb1, sizeof(bb1));
		if (r0 != sizeof(bb1)) return -1;
		if (bb1 != bb) return -2;
		r0 = load_slot_state(sr->slots + i, in);
		if (r0 < 0) return r;
	}
	READ_FIELD(in, sr->ints_enabled);
	switch (sr->cursystype) {
	case SYSTEM_7:
		system_command(sr, SYS_COMMAND_XRAM_RELEASE, CONF_SLOT6 + 1, 0);
		break;
	case SYSTEM_9:
		for (i = 0; i < 8; ++i)
			system_command(sr, SYS_COMMAND_BASEMEM9_RESTORE, CONF_SLOT6 + 1, i);
		break;
	}
	system_command(sr, SYS_COMMAND_INITMENU, 0, (long)sr->popup_menu);
	return r;
}

// ===================================================================

static void io6_write(word adr, byte data, struct MEM_PROC*io6_sel) // c060-c06f
{
	int ind = adr & 0x0F;
	mem_proc_write(adr, data, io6_sel + ind);
}

static byte io6_read(word adr, struct MEM_PROC*io6_sel) // c060-c06f
{
	int ind = adr & 0x0F;
	return mem_proc_read(adr, io6_sel + ind);
}

static void io_write(word adr, byte data, struct MEM_PROC*io_sel) // C000-C7FF
{
	int ind = (adr>>8) & 7;
	mem_proc_write(adr, data, io_sel + ind);
}

static byte io_read(word adr, struct MEM_PROC*io_sel) // C000-C7FF
{
	int ind = (adr>>8) & 7;
	return mem_proc_read(adr, io_sel + ind);
}

void baseio_write(word adr, byte data, struct MEM_PROC*baseio_sel) // C000-C0FF
{
	int ind = (adr>>4) & 0x0F;
	mem_proc_write(adr, data, baseio_sel + ind);
}

byte baseio_read(word adr, struct MEM_PROC*baseio_sel)	// C000-C0FF
{
	int ind = (adr>>4) & 0x0F;
	return mem_proc_read(adr, baseio_sel + ind);
}

byte keyb_read(word adr, struct SYS_RUN_STATE*sr)	// C000-C00F
{
	return sr->cur_key;
}

byte keyb_reg_read(word adr, struct SYS_RUN_STATE*sr)	// C063
{
	return sr->keyreg;
}

void keyb_clear(struct SYS_RUN_STATE*sr)	// C010-C01F
{
	sr->cur_key = 0;
}

byte keyb_clear_r(word adr, struct SYS_RUN_STATE*sr)	// C010-C01F
{
	keyb_clear(sr);
	return empty_read(adr, NULL);
}

void keyb_clear_w(word adr, byte d, struct SYS_RUN_STATE*sr)	// C010-C01F
{
	keyb_clear(sr);
}


// ======================================================

void mem_write(word adr, byte data, struct SYS_RUN_STATE*sr)
{
	int ind=adr>>0x0B;
//	printf("mem_write(%04X, %02X)\n", adr, data);
	mem_proc_write(adr, data, sr->base_mem + ind);
}

byte mem_read(word adr, struct SYS_RUN_STATE*sr)
{
	int ind=adr>>0x0B;
	byte r;
	r = mem_proc_read(adr, sr->base_mem + ind);
//	printf("mem_read(%04X) = %02X\n", adr, r);
	return r;
}
