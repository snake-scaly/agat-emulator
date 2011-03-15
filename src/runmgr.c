#include "runmgr.h"
#include "runmgrint.h"
#include "runstate.h"

#include "localize.h"

#include "resource.h"

extern int init_system_1(struct SYS_RUN_STATE*sr);
extern int init_system_2e(struct SYS_RUN_STATE*sr);
extern int init_systems(struct SYS_RUN_STATE*sr); // other systems



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
	case DEV_VIDEOTERM:
		puts("videoterm_init");
		return videoterm_init(sr, st, sc);
	case DEV_MEMORY_XRAM7:
		puts("xram7_init");
		return xram7_init(sr, st, sc);
	case DEV_MEMORY_PSROM7:
		puts("psrom7_init");
		return psrom7_init(sr, st, sc);
	case DEV_MEMORY_XRAM9:
		puts("xram9_init");
		return xram9_init(sr, st, sc);
	case DEV_SOFTCARD:
		puts("softcard_init");
		return softcard_init(sr, st, sc);
	case DEV_THUNDERCLOCK:
		puts("thunderclock_init");
		return thunderclock_init(sr, st, sc);
	case DEV_PRINTER9:
		puts("printer9_init");
		return printer9_init(sr, st, sc);
	case DEV_MOCKINGBOARD:
		puts("mockingboard_init");
		return mockingboard_init(sr, st, sc);
	case DEV_NIPPELCLOCK:
		puts("nippelclock_init");
		return nippelclock_init(sr, st, sc);
	case DEV_PRINTERA:
		puts("printera_init");
		return printera_init(sr, st, sc);
	case DEV_A2RAMCARD:
	case DEV_RAMFACTOR:
		puts("a2ramcard_init");
		return a2ramcard_init(sr, st, sc);
	case DEV_MEMORY_SATURN:
		puts("saturnmem_init");
		return saturnmem_init(sr, st, sc);
	case DEV_MOUSE_PAR:
		puts("mouse9_init");
		return mouse9_init(sr, st, sc);
	case DEV_MOUSE_APPLE:
		puts("applemouse_init");
		return applemouse_init(sr, st, sc);
	case DEV_MOUSE_NIPPEL:
		puts("nippelmouse_init");
		return nippelmouse_init(sr, st, sc);
	case DEV_ACI:
		puts("aci_init");
		return aci_init(sr, st, sc);
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
	static const int sys_types[NSYSTYPES] = {
		SYSTEM_7,
		SYSTEM_9,
		SYSTEM_A, // original apple 2
		SYSTEM_A, // apple 2 plus -> apple 2
		SYSTEM_E, // apple 2e
		SYSTEM_1, // apple 1
	};

	sr = calloc(1, sizeof(*sr));
	if (!sr) return NULL;
	sr->name = name?_tcsdup(name):NULL;
	sr->config = c;
	sr->cursystype = sys_types[c->systype]; // translate compatible system type
	sr->base_w = hmain;

	sr->keyreg = (get_keyb_language() == LANG_RUSSIAN)?0x7F:0xFF;

	set_run_state_ptr(sr->name, sr);

	switch (sr->cursystype) {
	case SYSTEM_1:
		r = init_system_1(sr);
		break;
	case SYSTEM_E:
		r = init_system_2e(sr);
		break;
	default:
		r = init_systems(sr);
		break;
	}
	if (r < 0) goto fail;

	r = init_video_window(sr);
	if (r < 0) goto fail;
	if (sr->name) {
		GetWindowText(sr->video_w, sr->title, 1024);
		lstrcat(sr->title, TEXT(" — "));
		lstrcat(sr->title, sr->name);
		SetWindowText(sr->video_w, sr->title);
	}

	for (i = 0; i < NCONFTYPES; i++ ) {
		sr->slots[i].sc = c->slots + i;
		sr->slots[i].sr = sr;
		if (i <= CONF_SLOT7) {
			sr->slots[i].baseio_sel = sr->baseio_sel + i + 8;
			sr->slots[i].io_sel = sr->io_sel + i;
		}
		r = init_slot_state(sr, sr->slots + i, c->slots + i);
		if (r < 0) goto fail;
	}

	r = video_init(sr);
	if (r < 0) goto fail;

	update_xio_status(sr);

	system_command(sr, SYS_COMMAND_INITMENU, 0, (long)sr->popup_menu);
	system_command(sr, SYS_COMMAND_INIT_DONE, 0, 0);

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
		term_video_window(sr);

		if (sr->sys.free_system) sr->sys.free_system(sr);
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

	system_command(sr, SYS_COMMAND_STOP, 0, 0);
	PostMessage(sr->base_w, WM_COMMAND, MAKEWPARAM(IDC_UPDATE,0), 0);

	and_run_state_flags(sr->name, ~RUNSTATE_RUNNING);
	set_run_state_ptr(sr->name, NULL);

	term_video_window(sr);

	for (i = 0; i < NCONFTYPES; i++) {
		free_slot_state(sr->slots + i);
	}
	if (sr->sys.free_system) sr->sys.free_system(sr);
	if (sr->name) free((void*)sr->name);
	free(sr);
	return 0;
}

int system_command(struct SYS_RUN_STATE*sr, int id, int data, long param)
{
	int i, r = 0;
	if (!sr) return -1;
//	printf("system_command(%i, %i, %i)\n", id, data, param);
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
	case SYS_COMMAND_HRESET:
		if (sr->sys.restart_system)
			sr->sys.restart_system(sr);
		update_xio_status(sr);
		break;
	case SYS_COMMAND_ACTIVATE:
		ShowWindow(sr->video_w, SW_SHOWNORMAL);
		SetActiveWindow(sr->video_w);
		break;
	case SYS_COMMAND_SET_PARENT:
		if (sr->video_w) SetParent(sr->video_w, (HWND)param);
		else r = -1;
		break;
	case SYS_COMMAND_SET_STATUS_TEXT:
		if (sr->video_w) {
			if (param) {
				TCHAR title[1024];
				lstrcpy(title, sr->title);
				lstrcat(title, TEXT(": "));
				lstrcat(title, (LPCTSTR)param);
				SetWindowText(sr->video_w, title);
			} else {
				SetWindowText(sr->video_w, sr->title);
			}
		}
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
		WRITE_FIELD(out, sr->slots[i].xio_en);
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
//		printf("loading conf %i\n", i);
		r0 = isread(in, &bb1, sizeof(bb1));
		if (r0 != sizeof(bb1)) {
			return -1;
		}	
		if (bb1 != bb) {
			printf("conf %i prefix failed\n", i);
			return -2;
		}
		r0 = load_slot_state(sr->slots + i, in);
		if (r0 < 0) return r;
		READ_FIELD(in, sr->slots[i].xio_en);
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
	update_xio_status(sr);
	system_command(sr, SYS_COMMAND_INITMENU, 0, (long)sr->popup_menu);
	system_command(sr, SYS_COMMAND_LOAD_DONE, 0, 0);
	return r;
}


// ======================================================

void mem_write(word adr, byte data, struct SYS_RUN_STATE*sr)
{
	int ind=adr>>0x0B;
//	if (adr == 0x87) {
//		logprint(0, "mem_write(%04X, %02X)", adr, data);
//		system_command(sr, SYS_COMMAND_DUMPCPUREGS, 0, 0);
//		if (access("memdump1.bin", 0)) dump_mem(sr, 0, 0x10000, "memdump1.bin");
//	}
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

int update_xio_status(struct SYS_RUN_STATE*sr)
{
	int i;
	if (sr->sys.xio_control) {
		if (!sr->sys.xio_control(sr, 1)) {
			sr->sys.xio_control(sr, 0);
			return -1;
		}
	}
	for (i = 0; i < NCONFTYPES; ++i) {
		if (sr->slots[i].xio_en) {
			sr->base_mem[0xC800 >> BASEMEM_BLOCK_SHIFT] = sr->slots[i].xio_sel;
//			printf("selected xrom#%i\n", i);
			return i;
		}
	}
//	printf("disabled all xrom\n");
	if (sr->sys.xio_control) sr->sys.xio_control(sr, 0);
	else {
		sr->base_mem[0xC800 >> BASEMEM_BLOCK_SHIFT].read = empty_read;
		sr->base_mem[0xC800 >> BASEMEM_BLOCK_SHIFT].write = empty_write;
	}	
	return -1;
}

int enable_slot_xio(struct SLOT_RUN_STATE*ss, int en)
{
	if (ss->xio_en == en) return 0;
	ss->xio_en = en;
//	printf("%sabled xrom#%i\n", en?"en":"dis", ss->sc->slot_no);
	return update_xio_status(ss->sr);
}
