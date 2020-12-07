#ifndef RUNMGR_H
#define RUNMGR_H

#include "sysconf.h"
#include "streams.h"
#include "memory.h"

enum {
	SYS_COMMAND_NOP,
	SYS_COMMAND_START,
	SYS_COMMAND_STOP,
	SYS_COMMAND_RESET,
	SYS_COMMAND_HRESET,
	SYS_COMMAND_IRQ,
	SYS_COMMAND_NMI,
	SYS_COMMAND_FLASH, // flash blinking parts of screen and so on
	SYS_COMMAND_REPAINT, // repaint screen
	SYS_COMMAND_TOGGLE_MONO, // toggle mono mode

	SYS_COMMAND_FAST, // set/reset fast mode, data = 1/0
	SYS_COMMAND_XRAM_RELEASE, // restore XRAM module
	SYS_COMMAND_PSROM_RELEASE, // restore PSROM module
	SYS_COMMAND_APPLEMODE, // switch to apple mode
	SYS_COMMAND_BASEMEM9_RESTORE, // restore mapping
	SYS_COMMAND_APPLE9_RESTORE, // restore mapping in apple mode; data=slot, param: int* 1-restore read, 2-restore write.
	SYS_COMMAND_ACTIVATE, // activate window
	SYS_COMMAND_WINCMD, // WM_COMMAND event: data = command, param = hwnd
	SYS_COMMAND_INITMENU, // used to init menus, param = menu handle
	SYS_COMMAND_UPDMENU, // used to update menus, param = menu handle
	SYS_COMMAND_FREEMENU, // used to free menus, param = menu handle

	SYS_COMMAND_DUMPCPUREGS, // dump cpu registers on standard log
	SYS_COMMAND_SET_CPU_HOOK, // set main cpu hook: data = proc addr, param = proc param
	SYS_COMMAND_INIT_DONE, // do some work after all modules are initialized
	SYS_COMMAND_NOIRQ,
	SYS_COMMAND_NONMI,
	SYS_COMMAND_SET_CPUTIMER, // data = delay in cpu ticks, param = timer id
	SYS_COMMAND_CPUTIMER,     // param = timer id
	SYS_COMMAND_LOAD_DONE,     // do some work after all modules are loaded

	SYS_COMMAND_SET_PARENT,     // set parent window (param=hwnd or NULL if detach)
	SYS_COMMAND_MOUSE_EVENT,    // mouse notification (data&1 - left button, 2 - right button, 0x80 - move)
	SYS_COMMAND_BOOST,	    // temporary cpu performance boost, data = cpu ticks period
	SYS_COMMAND_SET_STATUS_TEXT,// set window status text, param = str
	SYS_COMMAND_EXEC,           // set PC=param
	SYS_COMMAND_GETREGS6502,    // param = ptr to REGS_6502, data=0
	SYS_COMMAND_SETREGS6502,    // param = ptr to REGS_6502 data=mask
	SYS_COMMAND_GETREG,    	    // data = reg index, param = pointer to value
	SYS_COMMAND_SETREG,    	    // data = reg index, param = value

	SYS_COMMAND_WAKEUP,    	    // wakeup CPU module, data, param = 0

	SYS_N_COMMANDS
};


struct REGS_6502 // for GETREGS6502/SETREGS6502
{
	byte A, X, Y, S, F;
	word PC;
	dword TSC;
};

enum { // for GETREG/SETREG
	REG6502_A,	// byte
	REG6502_X,	// byte
	REG6502_Y,	// byte
	REG6502_S,	// byte
	REG6502_F,	// byte
	REG6502_PC,	// word
	REG6502_TSC,	// dword
};

struct SYS_RUN_STATE;

#define N_SERVICE_PROCS 4

struct SLOT_RUN_STATE
{
	void *data;
	struct SLOTCONFIG*sc;
	struct SYS_RUN_STATE*sr;
	int (*command)(struct SLOT_RUN_STATE*st, int id, int data, long param);
	int (*save) (struct SLOT_RUN_STATE*st, OSTREAM*out);
	int (*load) (struct SLOT_RUN_STATE*st, ISTREAM*in);
	int (*free) (struct SLOT_RUN_STATE*st);
	struct MEM_PROC *baseio_sel; //c0y0
	struct MEM_PROC *io_sel;  //cx00
	struct MEM_PROC xio_sel; //c800
	struct MEM_PROC service_procs[N_SERVICE_PROCS];
	int	xio_en;
};

#define CPU_TIMER_DEV_BASE 1024
#define DEF_CPU_TIMER_ID(st) ((long)(CPU_TIMER_DEV_BASE + (((st)->sc->slot_no<<8) | (st->sc->dev_type<<2))))


struct SYS_RUN_STATE *init_system_state(struct SYSCONFIG*c, HWND hmain, LPCTSTR name);
int free_system_state(struct SYS_RUN_STATE*sr);
int system_command(struct SYS_RUN_STATE*sr, int id, int data, long param);
int save_system_state(struct SYS_RUN_STATE*sr, OSTREAM*out);
int load_system_state(struct SYS_RUN_STATE*sr, ISTREAM*in);

int update_xio_status(struct SYS_RUN_STATE*sr);
int enable_slot_xio(struct SLOT_RUN_STATE*ss, int en);

byte system_read_rom(word adr,struct SYS_RUN_STATE*sr);

int cpu_get_tsc(struct SYS_RUN_STATE*sr);
int cpu_get_freq(struct SYS_RUN_STATE*sr);
int cpu_get_fast(struct SYS_RUN_STATE*sr);

void disable_ints(struct SYS_RUN_STATE*sr);
void enable_ints(struct SYS_RUN_STATE*sr);
byte baseram_read_ext_state(word adr, struct SYS_RUN_STATE*sr);


#endif //RUNMGR_H
