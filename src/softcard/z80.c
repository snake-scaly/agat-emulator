#include "types.h"
#include "memory.h"
#include "streams.h"
#include "debug.h"
#include "sysconf.h"
#include "runmgr.h"
#include "runmgrint.h"

#define HAVE_ENOUGH_MEMORY

typedef byte libspectrum_byte;
typedef signed char libspectrum_signed_byte;
typedef word libspectrum_word;
typedef dword libspectrum_dword;

struct STATE_Z80
{
	struct SYS_RUN_STATE*sr;
	union {
		struct { byte f, a, c, b, e, d, l, h, f_, a_, c_, b_, e_, d_, l_, h_, ixl, ixh, iyl, iyh, spl, sph, pcl, pch;};
		struct { word af, bc, de, hl, af_, bc_, de_, hl_, ix, iy, sp, pc; };
	};
	byte i, iff1, iff2, im;
	word r;
	byte r7;
	int	halted;
	int	enabled;
};

static word z80_addr(word a)
{
	static const word tr[16] = {
		0x1000, 0x2000, 0x3000, 0x4000, 0x5000, 0x6000, 0x7000, 0x8000, 
		0x9000, 0xA000, 0xB000, 0xD000, 0xE000, 0xF000, 0xC000, 0x0000
	};

	int sel = a >> 12;
	word r = tr[sel] | (a & 0xFFF);
//	printf("z80 translation: %x->%x\n", a, r);
	return r;
}

static byte z80_mem_read(word a, struct SYS_RUN_STATE*sr)
{
	return mem_read(z80_addr(a), sr);
}

static void z80_mem_write(word a, byte b, struct SYS_RUN_STATE*sr)
{
	mem_write(z80_addr(a), b, sr);
}


static byte fetch_cmd_byte(struct STATE_Z80 *st)
{
	st->r++;
	return z80_mem_read(st->pc++, st->sr);
}

#define readbyte(a) z80_mem_read(a,st->sr)
#define readbyte_internal readbyte
#define writebyte(a,b) z80_mem_write(a,b,st->sr)

#define readport readbyte
#define writeport writebyte

#define contend_read(a,b)
#define contend_read_no_mreq(a,b)
#define contend_write_no_mreq(a,b)

#define A st->a
#define F st->f
#define B st->b
#define C st->c
#define D st->d
#define E st->e
#define H st->h
#define L st->l
#define A_ st->a_
#define F_ st->f_
#define B_ st->b_
#define C_ st->c_
#define D_ st->d_
#define E_ st->e_
#define H_ st->h_
#define L_ st->l_
#define PCH st->pch
#define PCL st->pcl
#define SPH st->sph
#define SPL st->spl
#define R7 st->r7
#define I st->i
#define IM st->im
#define IFF1 st->iff1
#define IFF2 st->iff2
#define IXL st->ixl
#define IXH st->ixh
#define IYL st->iyl
#define IYH st->iyh

#define AF st->af
#define BC st->bc
#define DE st->de
#define HL st->hl
#define AF_ st->af_
#define BC_ st->bc_
#define DE_ st->de_
#define HL_ st->hl_
#define SP st->sp
#define PC st->pc
#define R st->r
#define IX st->ix
#define IY st->iy

#define IR ( ( st->i ) << 8 | ( st->r7 & 0x80 ) | ( st->r & 0x7f ) )


#define FLAG_C	0x01
#define FLAG_N	0x02
#define FLAG_P	0x04
#define FLAG_V	FLAG_P
#define FLAG_3	0x08
#define FLAG_H	0x10
#define FLAG_5	0x20
#define FLAG_Z	0x40
#define FLAG_S	0x80

static int z80_initialized = 0;

libspectrum_byte sz53_table[0x100]; /* The S, Z, 5 and 3 bits of the index */
libspectrum_byte parity_table[0x100]; /* The parity of the lookup value */
libspectrum_byte sz53p_table[0x100]; /* OR the above two tables together */

const libspectrum_byte halfcarry_add_table[] =
  { 0, FLAG_H, FLAG_H, FLAG_H, 0, 0, 0, FLAG_H };
const libspectrum_byte halfcarry_sub_table[] =
  { 0, 0, FLAG_H, 0, FLAG_H, 0, FLAG_H, FLAG_H };
const libspectrum_byte overflow_add_table[] = { 0, 0, 0, FLAG_V, FLAG_V, 0, 0, 0 };
const libspectrum_byte overflow_sub_table[] = { 0, FLAG_V, 0, 0, 0, 0, FLAG_V, 0 };

#ifndef HAVE_ENOUGH_MEMORY
static int z80_cbxx(struct STATE_Z80*st, libspectrum_byte opcode2 )
{
  switch(opcode2) {
#include "z80_cb.c"
  }
  return 0;
}

static void z80_ddfdcbxx(struct STATE_Z80*st, libspectrum_byte opcode3, libspectrum_word tempaddr )
{
  switch(opcode3) {
#include "z80_ddfdcb.c"
  }
}

static int z80_fdxx(struct STATE_Z80*st, libspectrum_byte opcode2 )
{
  switch(opcode2) {
#define REGISTER  IY
#define REGISTERL IYL
#define REGISTERH IYH
#include "z80_ddfd.c"
#undef REGISTERH
#undef REGISTERL
#undef REGISTER
  }
  return 0;
}


static int z80_ddxx(struct STATE_Z80*st, libspectrum_byte opcode2 )
{
  switch(opcode2) {
#define REGISTER  IX
#define REGISTERL IXL
#define REGISTERH IXH
#include "z80_ddfd.c"
#undef REGISTERH
#undef REGISTERL
#undef REGISTER
  }
  return 0;
}

static int z80_edxx(struct STATE_Z80*st, libspectrum_byte opcode2 )
{
  switch(opcode2) {
#include "z80_ed.c"
  }
  return 0;
}
#endif



int exec_z80(struct STATE_Z80*st)
{
	byte opcode;

	opcode = fetch_cmd_byte(st);
	switch (opcode) {
	#include "opcodes_base.c"
	}	
end_opcode:;
	return 0;
}

int init_z80()
{
	int i,j,k;
	libspectrum_byte parity;

	for(i=0;i<0x100;i++) {
		sz53_table[i]= i & ( FLAG_3 | FLAG_5 | FLAG_S );
		j=i; parity=0;
		for(k=0;k<8;k++) { parity ^= j & 1; j >>=1; }
		parity_table[i]= ( parity ? 0 : FLAG_P );
		sz53p_table[i] = sz53_table[i] | parity_table[i];
	}

	sz53_table[0]  |= FLAG_Z;
	sz53p_table[0] |= FLAG_Z;

	return 0;
}

void reset_z80(struct STATE_Z80*st)
{
	AF =BC =DE =HL =0;
	AF_=BC_=DE_=HL_=0;
	IX=IY=0;
	I=R=R7=0;
	SP=PC=0;
	IFF1=IFF2=IM=0;
	st->halted=0;
}




static int softcard_free(struct SLOT_RUN_STATE*st)
{
	struct STATE_Z80*data = st->data;
	system_command(st->sr, SYS_COMMAND_SET_CPU_HOOK, 0, 0);
	return 0;
}

static int hook_z80(struct STATE_Z80*data)
{
	if (!data->enabled) return 0;
	exec_z80(data);
	return 1;
}

static int softcard_command(struct SLOT_RUN_STATE*st, int cmd, int cdata, long param)
{
	struct STATE_Z80*data = st->data;
	switch (cmd) {
	case SYS_COMMAND_HRESET:
		data->enabled = 0;
	case SYS_COMMAND_RESET:
		reset_z80(data);
		break;
	}
	return 0;
}

static int softcard_save(struct SLOT_RUN_STATE*st, OSTREAM*out)
{
	struct STATE_Z80*data = st->data;

	WRITE_FIELD(out, *data);
	return 0;
}


static int softcard_load(struct SLOT_RUN_STATE*st, ISTREAM*in)
{
	struct STATE_Z80*data = st->data, tmp;

	READ_FIELD(in, tmp);
	tmp.sr = data->sr;
	*data = tmp;
	if (data->enabled) system_command(data->sr, SYS_COMMAND_SET_CPU_HOOK, (int)hook_z80, (long)data);
	else system_command(data->sr, SYS_COMMAND_SET_CPU_HOOK, 0, 0);
	return 0;
}

static void softcard_switch(word a, byte d, struct STATE_Z80*data)
{
	data->enabled = !data->enabled;
	if (data->enabled) {
//		puts("Z80 enabled!");
		system_command(data->sr, SYS_COMMAND_SET_CPU_HOOK, (int)hook_z80, (long)data);
	} else {
//		puts("Z80 disabled!");
	}	
}


static byte softcard_read(word a, struct STATE_Z80*data)
{
	return 0;
}




int  softcard_init(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*st, struct SLOTCONFIG*cf)
{
	struct STATE_Z80*data;

	if (!z80_initialized) {
		init_z80();
		z80_initialized = 1;
	}

	data = calloc(1, sizeof(*data));
	if (!data) return -1;

	data->sr = sr;

	reset_z80(data);

	fill_read_proc(st->io_sel, 1, softcard_read, data);
	fill_write_proc(st->io_sel, 1, softcard_switch, data);

	st->data = data;
	st->free = softcard_free;
	st->command = softcard_command;
	st->load = softcard_load;
	st->save = softcard_save;
	return 0;
}
