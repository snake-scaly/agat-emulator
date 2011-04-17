/*
	Agat Emulator version 1.19
	Copyright (c) NOP, nnop@newmail.ru
*/

#include "debugint.h"
#include "resource.h"
#include "common.h"
#include <stdlib.h>

#define CMD_ILL		1
#define CMD_NEW		2

#define CMD_LEN__ACC	0
#define CMD_LEN_IMPL	0
#define CMD_LEN_REL	1
#define CMD_LEN_ABS	2
#define CMD_LEN_ZP	1
#define CMD_LEN_IMM	1
#define CMD_LEN_ZPX	1
#define CMD_LEN_ZPY	1
#define CMD_LEN_ABSX	2
#define CMD_LEN_ABSY	2
#define CMD_LEN_IND_ZP	1
#define CMD_LEN_INDX	1
#define CMD_LEN_INDY	1
#define CMD_LEN_IND16X	2
#define CMD_LEN_IND	2

#define MAKE_COMMAND(cmd,addr,ticks) { #cmd, ea_show_##addr, CMD_LEN_##addr, 0}
#define MAKE_COMMAND_ILL(cmd,addr,ticks) { #cmd, ea_show_##addr, CMD_LEN_##addr, CMD_ILL}
#define MAKE_COMMAND_ILLX() { "???", ea_noshow, 0, CMD_ILL}
#define MAKE_COMMAND_NEW(cmd,addr,ticks) { #cmd, ea_show_##addr, CMD_LEN_##addr, CMD_NEW}

struct CMD_6502
{
	const char*cmd_name;
	void(*printadr)(struct DEBUG_INFO*inf, word addr);
	int len;
	unsigned flags;
};


static word mem_read_word(struct DEBUG_INFO*inf, word addr)
{
	return (word)mem_read(addr,inf->sr)|(((word)mem_read((word)(addr+1),inf->sr))<<8);
}


static void ea_noshow(struct DEBUG_INFO*inf, word adr)
{
}


static void ea_show__ACC(struct DEBUG_INFO*inf, word adr)
{
}

static void ea_show_IMPL(struct DEBUG_INFO*inf, word adr)
{
}


static void ea_show_REL(struct DEBUG_INFO*inf, word adr)
{
	word a=(signed char)mem_read(adr, inf->sr)+(adr+1);
	console_printf(inf->con,"$%04X",a);
}

static void ea_show_ABS(struct DEBUG_INFO*inf, word adr)
{
	word a=mem_read_word(inf, adr);
	console_printf(inf->con,"$%04X",a);
}

static void ea_show_ZP(struct DEBUG_INFO*inf, word adr)
{
	byte a=mem_read(adr, inf->sr);
	console_printf(inf->con,"$%02X",a);
}

static void ea_show_IMM(struct DEBUG_INFO*inf, word adr)
{
	byte a=mem_read(adr, inf->sr);
	console_printf(inf->con,"#$%02X",a);
}

static void ea_show_ZPX(struct DEBUG_INFO*inf, word adr)
{
	byte a=mem_read(adr, inf->sr);
	console_printf(inf->con,"$%02X,X",a);
}

static void ea_show_ZPY(struct DEBUG_INFO*inf, word adr)
{
	byte a=mem_read(adr, inf->sr);
	console_printf(inf->con,"$%02X,Y",a);
}

static void ea_show_ABSX(struct DEBUG_INFO*inf, word adr)
{
	word a=mem_read_word(inf, adr);
	console_printf(inf->con,"$%04X,X",a);
}

static void ea_show_ABSY(struct DEBUG_INFO*inf, word adr)
{
	word a=mem_read_word(inf, adr);
	console_printf(inf->con,"$%04X,Y",a);
}

static void ea_show_IND_ZP(struct DEBUG_INFO*inf, word adr)
{
	byte a=mem_read(adr, inf->sr);
	console_printf(inf->con,"($%02X)",a);
}

static void ea_show_INDX(struct DEBUG_INFO*inf, word adr)
{
	byte a=mem_read(adr, inf->sr);
	console_printf(inf->con,"($%02X,X)",a);
}

static void ea_show_IND16X(struct DEBUG_INFO*inf, word adr)
{
	word a=mem_read_word(inf, adr);
	console_printf(inf->con,"($%04X,X)",a);
}

static void ea_show_INDY(struct DEBUG_INFO*inf, word adr)
{
	byte a=mem_read(adr, inf->sr);
	console_printf(inf->con,"($%02X),Y",a);
}

static void ea_show_IND(struct DEBUG_INFO*inf, word adr)
{
	word a=mem_read_word(inf, adr);
	console_printf(inf->con,"($%04X)",a);
}

static const struct CMD_6502 cmds6502[256]=
{
  MAKE_COMMAND(BRK,IMPL,8),
  MAKE_COMMAND(ORA,INDX,6),       //01
  MAKE_COMMAND_ILL(HLT,IMPL,2),       //02 KIL
  MAKE_COMMAND_ILL(SLO,INDX,8),       //03
  MAKE_COMMAND_ILL(NOP,ZP,3),         //04
  MAKE_COMMAND(ORA,ZP,3),         //05
  MAKE_COMMAND(ASL,ZP,5),     //06
  MAKE_COMMAND_ILL(SLO,ZP,5),         //07
  MAKE_COMMAND(PHP,IMPL,3),       //08
  MAKE_COMMAND(ORA,IMM,2),        //09
  MAKE_COMMAND(ASL,_ACC,2),   //0A
  MAKE_COMMAND_ILL(ANC,IMM,2),        //0B
  MAKE_COMMAND_ILL(NOP,ABS,4),        //0C
  MAKE_COMMAND(ORA,ABS,4),        //0D
  MAKE_COMMAND(ASL,ABS,6),    //0E
  MAKE_COMMAND_ILL(SLO,ABS,6),        //0F

  MAKE_COMMAND(BPL,REL,2),        //10
  MAKE_COMMAND(ORA,INDY,5),       //11
  MAKE_COMMAND_ILL(HLT,IMPL,2),       //12 KIL
  MAKE_COMMAND_ILL(SLO,INDY,8),       //13
  MAKE_COMMAND_ILL(NOP,ZPX,4),        //14
  MAKE_COMMAND(ORA,ZPX,4),        //15
  MAKE_COMMAND(ASL,ZPX,6),    //16
  MAKE_COMMAND_ILL(SLO,ZPX,6),        //17
  MAKE_COMMAND(CLC,IMPL,2),       //18
  MAKE_COMMAND(ORA,ABSY,4),       //19
  MAKE_COMMAND_ILL(NOP,IMPL,2),       //1A
  MAKE_COMMAND_ILL(SLO,ABSY,7),       //1B
  MAKE_COMMAND_ILL(NOP,ABSX,4),       //1C
  MAKE_COMMAND(ORA,ABSX,4),       //1D
  MAKE_COMMAND(ASL,ABSX,7),   //1E
  MAKE_COMMAND_ILL(SLO,ABSX,7),       //1F

  MAKE_COMMAND(JSR,ABS,6),        //20
  MAKE_COMMAND(AND,INDX,6),       //21
  MAKE_COMMAND_ILL(HLT,IMPL,2),       //22 KIL
  MAKE_COMMAND_ILL(RLA,INDX,8),       //23
  MAKE_COMMAND(BIT,ZP,3),         //24
  MAKE_COMMAND(AND,ZP,3),         //25
  MAKE_COMMAND(ROL,ZP,5),     //26
  MAKE_COMMAND_ILL(RLA,ZP,5),         //27
  MAKE_COMMAND(PLP,IMPL,4),       //28
  MAKE_COMMAND(AND,IMM,2),        //29
  MAKE_COMMAND(ROL,_ACC,2),   //2A
  MAKE_COMMAND_ILL(ANC,IMM,2),        //2B
  MAKE_COMMAND(BIT,ABS,4),        //2C
  MAKE_COMMAND(AND,ABS,4),        //2D
  MAKE_COMMAND(ROL,ABS,6),    //2E
  MAKE_COMMAND_ILL(RLA,ABS,6),        //2F

  MAKE_COMMAND(BMI,REL,2),        //30
  MAKE_COMMAND(AND,INDY,5),       //31
  MAKE_COMMAND_ILL(HLT,IMPL,2),       //32 KIL
  MAKE_COMMAND_ILL(RLA,INDY,8),             //33
  MAKE_COMMAND_ILL(NOP,ZPX,4),              //34
  MAKE_COMMAND(AND,ZPX,4),        //35
  MAKE_COMMAND(ROL,ZPX,6),        //36
  MAKE_COMMAND_ILL(RLA,ZPX,6),              //37
  MAKE_COMMAND(SEC,IMPL,2),       //38
  MAKE_COMMAND(AND,ABSY,4),       //39
  MAKE_COMMAND_ILL(NOP,IMPL,2),              //3A
  MAKE_COMMAND_ILL(RLA,ABSY,7),              //3B
  MAKE_COMMAND_ILL(NOP,ABSX,4),              //3C
  MAKE_COMMAND(AND,ABSX,3),       //3D
  MAKE_COMMAND(ROL,ABSX,7),       //3E
  MAKE_COMMAND_ILL(RLA,ABSX,7),              //3F

  MAKE_COMMAND(RTI,IMPL,6),       //40
  MAKE_COMMAND(EOR,INDX,6),       //41
  MAKE_COMMAND_ILL(HLT,IMPL,2),       //42 KIL
  MAKE_COMMAND_ILL(SRE,INDX,8),              //43
  MAKE_COMMAND_ILL(NOP,ZP,3),              //44
  MAKE_COMMAND(EOR,ZP,3),         //45
  MAKE_COMMAND(LSR,ZP,5),         //46
  MAKE_COMMAND_ILL(SRE,ZP,5),              //47
  MAKE_COMMAND(PHA,IMPL,3),       //48
  MAKE_COMMAND(EOR,IMM,2),        //49
  MAKE_COMMAND(LSR,_ACC,2),        //4A
  MAKE_COMMAND_ILL(ALR,IMM,2),              //4B
  MAKE_COMMAND(JMP,ABS,3),        //4C
  MAKE_COMMAND(EOR,ABS,4),        //4D
  MAKE_COMMAND(LSR,ABS,6),        //4E
  MAKE_COMMAND_ILL(SRE,ABS,6),              //4F

  MAKE_COMMAND(BVC,REL,2),        //50
  MAKE_COMMAND(EOR,INDY,5),       //51
  MAKE_COMMAND_ILL(HLT,IMPL,2),       //52 KIL
  MAKE_COMMAND_ILL(SRE,INDY,8),              //53
  MAKE_COMMAND_ILL(NOP,ZPX,4),              //54
  MAKE_COMMAND(EOR,ZPX,4),        //55
  MAKE_COMMAND(LSR,ZPX,6),        //56
  MAKE_COMMAND_ILL(SRE,ZPX,6),              //57
  MAKE_COMMAND(CLI,IMPL,2),       //58
  MAKE_COMMAND(EOR,ABSY,4),       //59
  MAKE_COMMAND_ILL(NOP,IMPL,2),              //5A
  MAKE_COMMAND_ILL(SRE,ABSY,7),              //5B
  MAKE_COMMAND_ILL(NOP,ABSX,4),              //5C
  MAKE_COMMAND(EOR,ABSX,4),       //5D
  MAKE_COMMAND(LSR,ABSX,7),       //5E
  MAKE_COMMAND_ILL(SRE,ABSX,7),              //5F

  MAKE_COMMAND(RTS,IMPL,6),       //60
  MAKE_COMMAND(ADC,INDX,6),       //61
  MAKE_COMMAND_ILL(HLT,IMPL,2),       //62 KIL
  MAKE_COMMAND_ILL(RRA,INDX,8),              //63
  MAKE_COMMAND_ILL(NOP,ZP,3),              //64
  MAKE_COMMAND(ADC,ZP,3),         //65
  MAKE_COMMAND(ROR,ZP,5),         //66
  MAKE_COMMAND_ILL(RRA,ZP,5),              //67
  MAKE_COMMAND(PLA,IMPL,4),       //68
  MAKE_COMMAND(ADC,IMM,2),        //69
  MAKE_COMMAND(ROR,_ACC,2),        //6A
  MAKE_COMMAND_ILL(ARR,IMM,2),              //6B
  MAKE_COMMAND(JMP,IND,5),        //6C
  MAKE_COMMAND(ADC,ABS,4),        //6D
  MAKE_COMMAND(ROR,ABS,6),        //6E
  MAKE_COMMAND_ILL(RRA,ABS,6),              //6F

  MAKE_COMMAND(BVS,REL,2),        //70
  MAKE_COMMAND(ADC,INDY,5),       //71
  MAKE_COMMAND_ILL(HLT,IMPL,2),       //72 KIL
  MAKE_COMMAND_ILL(RRA,INDY,8),              //73
  MAKE_COMMAND_ILL(NOP,ZPX,4),              //74
  MAKE_COMMAND(ADC,ZPX,4),        //75
  MAKE_COMMAND(ROR,ZPX,6),        //76
  MAKE_COMMAND_ILL(RRA,ZPX,6),              //77
  MAKE_COMMAND(SEI,IMPL,2),       //78
  MAKE_COMMAND(ADC,ABSY,4),       //79
  MAKE_COMMAND_ILL(NOP,IMPL,2),              //7A
  MAKE_COMMAND_ILL(RRA,ABSY,7),              //7B
  MAKE_COMMAND_ILL(NOP,ABSX,4),              //7C
  MAKE_COMMAND(ADC,ABSX,4),       //7D
  MAKE_COMMAND(ROR,ABSX,7),       //7E
  MAKE_COMMAND_ILL(RRA,ABSX,7),              //7F

  MAKE_COMMAND_ILL(NOP,IMM,2),              //80
  MAKE_COMMAND(STA,INDX,6),       //81
  MAKE_COMMAND_ILL(NOP,IMM,2),       //82
  MAKE_COMMAND_ILL(SAX,INDX,6),              //83
  MAKE_COMMAND(STY,ZP,3),         //84
  MAKE_COMMAND(STA,ZP,3),         //85
  MAKE_COMMAND(STX,ZP,3),         //86
  MAKE_COMMAND_ILL(SAX,ZP,3),              //87
  MAKE_COMMAND(DEY,IMPL,2),       //88
  MAKE_COMMAND_ILL(NOP,IMM,2),              //89
  MAKE_COMMAND(TXA,IMPL,2),       //8A
  MAKE_COMMAND_ILL(XAA,IMM,2),              //8B
  MAKE_COMMAND(STY,ABS,4),        //8C
  MAKE_COMMAND(STA,ABS,4),        //8D
  MAKE_COMMAND(STX,ABS,4),        //8E
  MAKE_COMMAND_ILL(SAX,ABS,4),              //8F

  MAKE_COMMAND(BCC,REL,2),        //90
  MAKE_COMMAND(STA,INDY,6),       //91
  MAKE_COMMAND_ILL(HLT,IMPL,2),       //92 KIL
  MAKE_COMMAND_ILL(AHX,INDY,6),              //93
  MAKE_COMMAND(STY,ZPX,4),        //94
  MAKE_COMMAND(STA,ZPX,4),        //95
  MAKE_COMMAND(STX,ZPY,4),        //96
  MAKE_COMMAND_ILL(SAX,ZPY,4),              //97
  MAKE_COMMAND(TYA,IMPL,2),       //98
  MAKE_COMMAND(STA,ABSY,5),       //99
  MAKE_COMMAND(TXS,IMPL,2),       //9A
  MAKE_COMMAND_ILL(TAS,ABSY,5),              //9B
  MAKE_COMMAND_ILL(SHY,ABSX,5),              //9C
  MAKE_COMMAND(STA,ABSX,5),       //9D
  MAKE_COMMAND_ILL(SHX,ABSY,5), 		//9E
  MAKE_COMMAND_ILL(AHX,ABSY,5),              //9F

  MAKE_COMMAND(LDY,IMM,2),        //A0
  MAKE_COMMAND(LDA,INDX,6),       //A1
  MAKE_COMMAND(LDX,IMM,2),        //A2
  MAKE_COMMAND_ILL(LAX,INDX,6),              //A3
  MAKE_COMMAND(LDY,ZP,3),         //A4
  MAKE_COMMAND(LDA,ZP,3),         //A5
  MAKE_COMMAND(LDX,ZP,3),         //A6
  MAKE_COMMAND_ILL(LAX,ZP,3),              //A7
  MAKE_COMMAND(TAY,IMPL,2),       //A8
  MAKE_COMMAND(LDA,IMM,2),        //A9
  MAKE_COMMAND(TAX,IMPL,2),       //AA
  MAKE_COMMAND_ILL(LAX,IMM,2),              //AB
  MAKE_COMMAND(LDY,ABS,4),        //AC
  MAKE_COMMAND(LDA,ABS,4),        //AD
  MAKE_COMMAND(LDX,ABS,4),        //AE
  MAKE_COMMAND_ILL(LAX,ABS,4),              //AF

  MAKE_COMMAND(BCS,REL,2),        //B0
  MAKE_COMMAND(LDA,INDY,5),       //B1
  MAKE_COMMAND_ILL(HLT,IMPL,2),       //B2 KIL
  MAKE_COMMAND_ILL(LAX,INDY,5),              //B3
  MAKE_COMMAND(LDY,ZPX,4),        //B4
  MAKE_COMMAND(LDA,ZPX,4),        //B5
  MAKE_COMMAND(LDX,ZPY,4),        //B6
  MAKE_COMMAND_ILL(LAX,ZPY,4),              //B7
  MAKE_COMMAND(CLV,IMPL,2),       //B8
  MAKE_COMMAND(LDA,ABSY,4),       //B9
  MAKE_COMMAND(TSX,IMPL,2),       //BA
  MAKE_COMMAND_ILL(LAS,ABSY,4),              //BB
  MAKE_COMMAND(LDY,ABSX,4),       //BC
  MAKE_COMMAND(LDA,ABSX,4),       //BD
  MAKE_COMMAND(LDX,ABSY,4),       //BE
  MAKE_COMMAND_ILL(LAX,ABSY,4),              //BF

  MAKE_COMMAND(CPY,IMM,2),        //C0
  MAKE_COMMAND(CMP,INDX,6),       //C1
  MAKE_COMMAND_ILL(NOP,IMM,2),       //C2
  MAKE_COMMAND_ILL(DCP,INDX,8),              //C3
  MAKE_COMMAND(CPY,ZP,3),         //C4
  MAKE_COMMAND(CMP,ZP,3),         //C5
  MAKE_COMMAND(DEC,ZP,3),         //C6
  MAKE_COMMAND_ILL(DCP,ZP,5),              //C7
  MAKE_COMMAND(INY,IMPL,2),       //C8
  MAKE_COMMAND(CMP,IMM,2),        //C9
  MAKE_COMMAND(DEX,IMPL,2),       //CA
  MAKE_COMMAND_ILL(AXS,IMM,2),              //CB
  MAKE_COMMAND(CPY,ABS,4),        //CC
  MAKE_COMMAND(CMP,ABS,4),        //CD
  MAKE_COMMAND(DEC,ABS,4),        //CE
  MAKE_COMMAND_ILL(DCP,ABS,6),              //CF

  MAKE_COMMAND(BNE,REL,2),        //D0
  MAKE_COMMAND(CMP,INDY,5),       //D1
  MAKE_COMMAND_ILL(HLT,IMPL,2),       //D2 KIL
  MAKE_COMMAND_ILL(DCP,INDY,8),              //D3
  MAKE_COMMAND_ILL(NOP,ZPX,4),              //D4
  MAKE_COMMAND(CMP,ZPX,4),        //D5
  MAKE_COMMAND(DEC,ZPX,6),        //D6
  MAKE_COMMAND_ILL(DCP,ZPX,6),              //D7
  MAKE_COMMAND(CLD,IMPL,2),       //D8
  MAKE_COMMAND(CMP,ABSY,4),       //D9
  MAKE_COMMAND_ILL(NOP,IMPL,2),              //DA
  MAKE_COMMAND_ILL(DCP,ABSY,7),              //DB
  MAKE_COMMAND_ILL(NOP,ABSX,4),              //DC
  MAKE_COMMAND(CMP,ABSX,4),       //DD
  MAKE_COMMAND(DEC,ABSX,7),       //DE
  MAKE_COMMAND_ILL(DCP,ABSX,7),              //DF

  MAKE_COMMAND(CPX,IMM,2),        //E0
  MAKE_COMMAND(SBC,INDX,4),       //E1
  MAKE_COMMAND_ILL(NOP,IMM,2),       //E2
  MAKE_COMMAND_ILL(ISC,INDX,8),              //E3
  MAKE_COMMAND(CPX,ZP,3),         //E4
  MAKE_COMMAND(SBC,ZP,3),         //E5
  MAKE_COMMAND(INC,ZP,5),         //E6
  MAKE_COMMAND_ILL(ISC,ZP,5),              //E7
  MAKE_COMMAND(INX,IMPL,2),       //E8
  MAKE_COMMAND(SBC,IMM,2),        //E9
  MAKE_COMMAND(NOP,IMPL,2),       //EA
  MAKE_COMMAND_ILL(SBC,IMM,2),              //EB
  MAKE_COMMAND(CPX,ABS,4),        //EC
  MAKE_COMMAND(SBC,ABS,4),        //ED
  MAKE_COMMAND(INC,ABS,6),        //EE
  MAKE_COMMAND_ILL(ISC,ABS,6),              //EF

  MAKE_COMMAND(BEQ,REL,2),        //F0
  MAKE_COMMAND(SBC,INDY,5),       //F1
  MAKE_COMMAND_ILL(HLT,IMPL,2),       //F2 KIL
  MAKE_COMMAND_ILL(ISC,INDY,8),              //F3
  MAKE_COMMAND_ILL(NOP,ZPX,4),              //F4
  MAKE_COMMAND(SBC,ZPX,4),        //F5
  MAKE_COMMAND(INC,ZPX,6),        //F6
  MAKE_COMMAND_ILL(ISC,ZPX,6),              //F7
  MAKE_COMMAND(SED,IMPL,2),       //F8
  MAKE_COMMAND(SBC,ABSY,4),       //F9
  MAKE_COMMAND_ILL(NOP,IMPL,2),              //FA
  MAKE_COMMAND_ILL(ISC,ABSY,7),              //FB
  MAKE_COMMAND_ILL(NOP,ABSX,4),              //FC
  MAKE_COMMAND(SBC,ABSX,4),       //FD
  MAKE_COMMAND(INC,ABSX,7),       //FE
  MAKE_COMMAND_ILL(ISC,ABSX,7)              //FF
};

static const struct CMD_6502 cmds65c02[256]=
{
  MAKE_COMMAND(BRK,IMPL,7),       //00
  MAKE_COMMAND(ORA,INDX,6),       //01
  MAKE_COMMAND_ILLX(),             //02
  MAKE_COMMAND_ILLX(),             //03
  MAKE_COMMAND_NEW(TSB,ZP,5),     //04
  MAKE_COMMAND(ORA,ZP,3),         //05
  MAKE_COMMAND(ASL,ZP,5),     //06
  MAKE_COMMAND_NEW(RMB0,ZP,2),    //07
  MAKE_COMMAND(PHP,IMPL,3),       //08
  MAKE_COMMAND(ORA,IMM,2),        //09
  MAKE_COMMAND(ASL,_ACC,2),   //0A
  MAKE_COMMAND_ILLX(),             //0B
  MAKE_COMMAND_NEW(TSB,ABS,6),    //0C
  MAKE_COMMAND(ORA,ABS,4),        //0D
  MAKE_COMMAND(ASL,ABS,6),    //0E
  MAKE_COMMAND_NEW(BBR0,REL,2),   //0F

  MAKE_COMMAND(BPL,REL,2),        //10
  MAKE_COMMAND(ORA,INDY,5),       //11
  MAKE_COMMAND_NEW(ORA,IND_ZP,5), //12
  MAKE_COMMAND_ILLX(),             //13
  MAKE_COMMAND_NEW(TRB,ZP,5),     //14
  MAKE_COMMAND(ORA,ZPX,4),        //15
  MAKE_COMMAND(ASL,ZPX,6),    //16
  MAKE_COMMAND_NEW(RMB1,ZP,2),    //17
  MAKE_COMMAND(CLC,IMPL,2),       //18
  MAKE_COMMAND(ORA,ABSY,4),       //19
  MAKE_COMMAND_NEW(INC_ACC,_ACC,2),//1A
  MAKE_COMMAND_ILLX(),             //1B
  MAKE_COMMAND_NEW(TRB,ABS,6),    //1C
  MAKE_COMMAND(ORA,ABSX,4),       //1D
  MAKE_COMMAND(ASL,ABSX,7),   //1E
  MAKE_COMMAND_NEW(BBR1,REL,2),   //1F

  MAKE_COMMAND(JSR,ABS,6),        //20
  MAKE_COMMAND(AND,INDX,6),       //21
  MAKE_COMMAND_ILLX(),             //22
  MAKE_COMMAND_ILLX(),             //23
  MAKE_COMMAND(BIT,ZP,3),         //24
  MAKE_COMMAND(AND,ZP,3),         //25
  MAKE_COMMAND(ROL,ZP,5),     //26
  MAKE_COMMAND_NEW(RMB2,ZP,2),    //27
  MAKE_COMMAND(PLP,IMPL,4),       //28
  MAKE_COMMAND(AND,IMM,2),        //29
  MAKE_COMMAND(ROL,_ACC,2),   //2A
  MAKE_COMMAND_ILLX(),             //2B
  MAKE_COMMAND(BIT,ABS,4),        //2C
  MAKE_COMMAND(AND,ABS,4),        //2D
  MAKE_COMMAND(ROL,ABS,6),    //2E
  MAKE_COMMAND_NEW(BBR2,REL,2),   //2F

  MAKE_COMMAND(BMI,REL,2),        //30
  MAKE_COMMAND(AND,INDY,5),       //31
  MAKE_COMMAND_NEW(AND,IND_ZP,5), //32
  MAKE_COMMAND_ILLX(),             //33
  MAKE_COMMAND_NEW(BIT,ZPX,4),    //34
  MAKE_COMMAND(AND,ZPX,4),        //35
  MAKE_COMMAND(ROL,ZPX,6),    //36
  MAKE_COMMAND_NEW(RMB3,ZP,2),    //37
  MAKE_COMMAND(SEC,IMPL,2),       //38
  MAKE_COMMAND(AND,ABSY,4),       //39
  MAKE_COMMAND_NEW(DEC_ACC,_ACC,2),//3A
  MAKE_COMMAND_ILLX(),             //3B
  MAKE_COMMAND_NEW(BIT,ABSX,4),   //3C
  MAKE_COMMAND(AND,ABSX,3),       //3D
  MAKE_COMMAND(ROL,ABSX,7),   //3E
  MAKE_COMMAND_NEW(BBR3,REL,2),   //3F

  MAKE_COMMAND(RTI,IMPL,6),       //40
  MAKE_COMMAND(EOR,INDX,6),       //41
  MAKE_COMMAND_ILLX(),       	  //42
  MAKE_COMMAND_ILLX(),             //43
  MAKE_COMMAND_ILLX(),             //44
  MAKE_COMMAND(EOR,ZP,3),         //45
  MAKE_COMMAND(LSR,ZP,5),     //46
  MAKE_COMMAND_NEW(RMB4,ZP,2),    //47
  MAKE_COMMAND(PHA,IMPL,3),       //48
  MAKE_COMMAND(EOR,IMM,2),        //49
  MAKE_COMMAND(LSR,_ACC,2),   //4A
  MAKE_COMMAND_ILLX(),             //4B
  MAKE_COMMAND(JMP,ABS,3),        //4C
  MAKE_COMMAND(EOR,ABS,4),        //4D
  MAKE_COMMAND(LSR,ABS,6),    //4E
  MAKE_COMMAND_NEW(BBR4,REL,2),   //4F

  MAKE_COMMAND(BVC,REL,2),        //50
  MAKE_COMMAND(EOR,INDY,5),       //51
  MAKE_COMMAND_NEW(EOR,IND_ZP,5), //52
  MAKE_COMMAND_ILLX(),             //53
  MAKE_COMMAND_ILLX(),             //54
  MAKE_COMMAND(EOR,ZPX,4),        //55
  MAKE_COMMAND(LSR,ZPX,6),    //56
  MAKE_COMMAND_NEW(RMB5,ZP,2),    //57
  MAKE_COMMAND(CLI,IMPL,2),       //58
  MAKE_COMMAND(EOR,ABSY,4),       //59
  MAKE_COMMAND_NEW(PHY,IMPL,3),   //5A
  MAKE_COMMAND_ILLX(),             //5B
  MAKE_COMMAND_ILLX(),             //5C
  MAKE_COMMAND(EOR,ABSX,4),       //5D
  MAKE_COMMAND(LSR,ABSX,7),   //5E
  MAKE_COMMAND_NEW(BBR5,REL,2),   //5F

  MAKE_COMMAND(RTS,IMPL,6),       //60
  MAKE_COMMAND(ADC,INDX,6),       //61
  MAKE_COMMAND_ILLX(),             //62
  MAKE_COMMAND_ILLX(),             //63
  MAKE_COMMAND_NEW(STZ,ZP,3),     //64
  MAKE_COMMAND(ADC,ZP,3),         //65
  MAKE_COMMAND(ROR,ZP,5),     //66
  MAKE_COMMAND_NEW(RMB6,ZP,2),    //67
  MAKE_COMMAND(PLA,IMPL,4),       //68
  MAKE_COMMAND(ADC,IMM,2),        //69
  MAKE_COMMAND(ROR,_ACC,2),   //6A
  MAKE_COMMAND_ILLX(),             //6B
  MAKE_COMMAND(JMP,IND,5),        //6C
  MAKE_COMMAND(ADC,ABS,4),        //6D
  MAKE_COMMAND(ROR,ABS,6),    //6E
  MAKE_COMMAND_NEW(BBR6,REL,2),   //6F

  MAKE_COMMAND(BVS,REL,2),        //70
  MAKE_COMMAND(ADC,INDY,5),       //71
  MAKE_COMMAND_NEW(ADC,IND_ZP,5), //72
  MAKE_COMMAND_ILLX(),             //73
  MAKE_COMMAND_NEW(STZ,ZPX,4),    //74
  MAKE_COMMAND(ADC,ZPX,4),        //75
  MAKE_COMMAND(ROR,ZPX,6),    //76
  MAKE_COMMAND_NEW(RMB7,ZP,2),    //77
  MAKE_COMMAND(SEI,IMPL,2),       //78
  MAKE_COMMAND(ADC,ABSY,4),       //79
  MAKE_COMMAND_NEW(PLY,IMPL,4),   //7A
  MAKE_COMMAND_ILLX(),             //7B
  MAKE_COMMAND_NEW(JMP,IND16X,4), //7C
  MAKE_COMMAND(ADC,ABSX,4),       //7D
  MAKE_COMMAND(ROR,ABSX,7),   //7E
  MAKE_COMMAND_NEW(BBR7,REL,2),   //7F

  MAKE_COMMAND_NEW(BRA,REL,2),    //80
  MAKE_COMMAND(STA,INDX,6),       //81
  MAKE_COMMAND_ILLX(),             //82
  MAKE_COMMAND_ILLX(),             //83
  MAKE_COMMAND(STY,ZP,3),         //84
  MAKE_COMMAND(STA,ZP,3),         //85
  MAKE_COMMAND(STX,ZP,3),         //86
  MAKE_COMMAND_NEW(SMB0,ZP,2),    //87
  MAKE_COMMAND(DEY,IMPL,2),       //88
  MAKE_COMMAND_NEW(BIT,IMM,2),    //89
  MAKE_COMMAND(TXA,IMPL,2),       //8A
  MAKE_COMMAND_ILLX(),             //8B
  MAKE_COMMAND(STY,ABS,4),        //8C
  MAKE_COMMAND(STA,ABS,4),        //8D
  MAKE_COMMAND(STX,ABS,4),        //8E
  MAKE_COMMAND_NEW(BBS0,REL,5),   //8F

  MAKE_COMMAND(BCC,REL,2),        //90
  MAKE_COMMAND(STA,INDY,6),       //91
  MAKE_COMMAND_NEW(STA,IND_ZP,5), //92
  MAKE_COMMAND_ILLX(),             //93
  MAKE_COMMAND(STY,ZPX,4),        //94
  MAKE_COMMAND(STA,ZPX,4),        //95
  MAKE_COMMAND(STX,ZPY,4),        //96
  MAKE_COMMAND_NEW(SMB1,ZP,2),    //97
  MAKE_COMMAND(TYA,IMPL,2),       //98
  MAKE_COMMAND(STA,ABSY,5),       //99
  MAKE_COMMAND(TXS,IMPL,2),       //9A
  MAKE_COMMAND_ILLX(),             //9B
  MAKE_COMMAND_NEW(STZ,ABS,4),    //9C
  MAKE_COMMAND(STA,ABSX,5),       //9D
  MAKE_COMMAND_NEW(STZ,ABSX,5),   //9E
  MAKE_COMMAND_NEW(BBS1,REL,2),   //9F

  MAKE_COMMAND(LDY,IMM,2),        //A0
  MAKE_COMMAND(LDA,INDX,6),       //A1
  MAKE_COMMAND(LDX,IMM,2),        //A2
  MAKE_COMMAND_ILLX(),             //A3
  MAKE_COMMAND(LDY,ZP,3),         //A4
  MAKE_COMMAND(LDA,ZP,3),         //A5
  MAKE_COMMAND(LDX,ZP,3),         //A6
  MAKE_COMMAND_NEW(SMB2,ZP,2),    //A7
  MAKE_COMMAND(TAY,IMPL,2),       //A8
  MAKE_COMMAND(LDA,IMM,2),        //A9
  MAKE_COMMAND(TAX,IMPL,2),       //AA
  MAKE_COMMAND_ILLX(),             //AB
  MAKE_COMMAND(LDY,ABS,4),        //AC
  MAKE_COMMAND(LDA,ABS,4),        //AD
  MAKE_COMMAND(LDX,ABS,4),        //AE
  MAKE_COMMAND_NEW(BBS2,REL,2),   //AF

  MAKE_COMMAND(BCS,REL,2),        //B0
  MAKE_COMMAND(LDA,INDY,5),       //B1
  MAKE_COMMAND_NEW(LDA,IND_ZP,5), //B2
  MAKE_COMMAND_ILLX(),             //B3
  MAKE_COMMAND(LDY,ZPX,4),        //B4
  MAKE_COMMAND(LDA,ZPX,4),        //B5
  MAKE_COMMAND(LDX,ZPY,4),        //B6
  MAKE_COMMAND_NEW(SMB3,ZP,2),    //B7
  MAKE_COMMAND(CLV,IMPL,2),       //B8
  MAKE_COMMAND(LDA,ABSY,4),       //B9
  MAKE_COMMAND(TSX,IMPL,2),       //BA
  MAKE_COMMAND_ILLX(),             //BB
  MAKE_COMMAND(LDY,ABSX,4),       //BC
  MAKE_COMMAND(LDA,ABSX,4),       //BD
  MAKE_COMMAND(LDX,ABSY,4),       //BE
  MAKE_COMMAND_NEW(BBS3,REL,2),   //BF

  MAKE_COMMAND(CPY,IMM,2),        //C0
  MAKE_COMMAND(CMP,INDX,6),       //C1
  MAKE_COMMAND_ILLX(),             //C2
  MAKE_COMMAND_ILLX(),             //C3
  MAKE_COMMAND(CPY,ZP,3),         //C4
  MAKE_COMMAND(CMP,ZP,3),         //C5
  MAKE_COMMAND(DEC,ZP,3),         //C6
  MAKE_COMMAND_NEW(SMB4,ZP,2),    //C7
  MAKE_COMMAND(INY,IMPL,2),       //C8
  MAKE_COMMAND(CMP,IMM,2),        //C9
  MAKE_COMMAND(DEX,IMPL,2),       //CA
  MAKE_COMMAND_NEW(WAI,IMPL,2),   //CB
  MAKE_COMMAND(CPY,ABS,4),        //CC
  MAKE_COMMAND(CMP,ABS,4),        //CD
  MAKE_COMMAND(DEC,ABS,4),        //CE
  MAKE_COMMAND_NEW(BBS4,REL,2),   //CF

  MAKE_COMMAND(BNE,REL,2),        //D0
  MAKE_COMMAND(CMP,INDY,5),       //D1
  MAKE_COMMAND_NEW(CMP,IND_ZP,5), //D2
  MAKE_COMMAND_ILLX(),             //D3
  MAKE_COMMAND_ILLX(),             //D4
  MAKE_COMMAND(CMP,ZPX,4),        //D5
  MAKE_COMMAND(DEC,ZPX,6),        //D6
  MAKE_COMMAND_NEW(SMB5,ZP,2),    //D7
  MAKE_COMMAND(CLD,IMPL,2),       //D8
  MAKE_COMMAND(CMP,ABSY,4),       //D9
  MAKE_COMMAND_NEW(PHX,IMPL,3),   //DA
  MAKE_COMMAND_NEW(STP,IMPL,7),   //DB
  MAKE_COMMAND_ILLX(),             //DC
  MAKE_COMMAND(CMP,ABSX,4),       //DD
  MAKE_COMMAND(DEC,ABSX,7),       //DE
  MAKE_COMMAND_NEW(BBS5,REL,2),   //DF

  MAKE_COMMAND(CPX,IMM,2),        //E0
  MAKE_COMMAND(SBC,INDX,6),       //E1
  MAKE_COMMAND_ILLX(),             //E2
  MAKE_COMMAND_ILLX(),             //E3
  MAKE_COMMAND(CPX,ZP,3),         //E4
  MAKE_COMMAND(SBC,ZP,3),         //E5
  MAKE_COMMAND(INC,ZP,5),         //E6
  MAKE_COMMAND_NEW(SMB6,ZP,2),    //E7
  MAKE_COMMAND(INX,IMPL,2),       //E8
  MAKE_COMMAND(SBC,IMM,2),        //E9
  MAKE_COMMAND(NOP,IMPL,2),       //EA
  MAKE_COMMAND_ILLX(),             //EB
  MAKE_COMMAND(CPX,ABS,4),        //EC
  MAKE_COMMAND(SBC,ABS,4),        //ED
  MAKE_COMMAND(INC,ABS,6),        //EE
  MAKE_COMMAND_NEW(BBS6,REL,2),   //EF

  MAKE_COMMAND(BEQ,REL,2),        //F0
  MAKE_COMMAND(SBC,INDY,5),       //F1
  MAKE_COMMAND_NEW(SBC,IND_ZP,5), //F2
  MAKE_COMMAND_ILLX(),             //F3
  MAKE_COMMAND_ILLX(),             //F4
  MAKE_COMMAND(SBC,ZPX,4),        //F5
  MAKE_COMMAND(INC,ZPX,6),        //F6
  MAKE_COMMAND_NEW(SMB7,ZP,2),    //F7
  MAKE_COMMAND(SED,IMPL,2),       //F8
  MAKE_COMMAND(SBC,ABSY,4),       //F9
  MAKE_COMMAND_NEW(PLX,IMPL,4),   //FA
  MAKE_COMMAND_ILLX(),             //FB
  MAKE_COMMAND_ILLX(),             //FC
  MAKE_COMMAND(SBC,ABSX,4),       //FD
  MAKE_COMMAND(INC,ABSX,7),       //FE
  MAKE_COMMAND_NEW(BBS7,REL,2)    //FF
};

void sysmon_err(struct DEBUG_INFO*inf)
{
	console_printf(inf->con, "ERR");
	MessageBeep(MB_ICONEXCLAMATION);
}

void sysmon_l(struct DEBUG_INFO*inf)
{
	word adr = inf->sst.raddr;
	int n = 20, i;
	const struct CMD_6502*cmds;
	if (inf->sr->config->slots[CONF_CPU].dev_type == DEV_6502) cmds = cmds6502;
	else cmds = cmds65c02;

	for (;n; --n) {
		byte cmd;
		const char*cmdname;
		int cmdlen;
		cmd = mem_read(adr, inf->sr);
		console_printf(inf->con, "\n%04X-   %02X", adr++, cmd);
		if (((cmds[cmd].flags & CMD_NEW) && !(inf->sr->gconfig->flags & EMUL_FLAGS_DEBUG_NEW_CMDS)) ||
			((cmds[cmd].flags & CMD_ILL) && !(inf->sr->gconfig->flags & EMUL_FLAGS_DEBUG_ILLEGAL_CMDS))) {
			cmdlen = 0;
			cmdname = "???";
		} else {
			cmdlen = cmds[cmd].len;
			cmdname = cmds[cmd].cmd_name;
		}
		for (i = 0; i < cmdlen; ++i) {
			console_printf(inf->con, " %02X", mem_read(adr + i, inf->sr));
		}
		for (; i < 4; ++i)  console_printf(inf->con, "   ");
		console_printf(inf->con, " %-6s", cmdname);
		if (cmdlen) cmds[cmd].printadr(inf, adr);
		adr += cmdlen;
	}
	inf->sst.raddr = adr;
}

void sysmon_g(struct DEBUG_INFO*inf)
{
	system_command(inf->sr, SYS_COMMAND_EXEC, 0, inf->sst.raddr);
}


void sysmon_dump(struct DEBUG_INFO*inf, int aarg)
{
	word adr = inf->sst.addr[1];
	if ((aarg & 4) || !(adr&7)) console_printf(inf->con, "\n%04X-", adr);
	for(;;) {
		console_printf(inf->con, " %02X", mem_read(adr, inf->sr));
		++ adr;
		if (adr > inf->sst.addr[2]) break;
		if (!(adr&7)) {
			console_printf(inf->con, "\n%04X-", adr);
		}
	}
	inf->sst.addr[1] = adr;
	inf->sst.addr[2] = (inf->sst.addr[1] + 1) | 7;
}

void sysmon_close(struct DEBUG_INFO*inf)
{
	PostMessage(inf->sr->video_w, WM_COMMAND, IDC_DEBUGGER, 0);
}

void sysmon_move(struct DEBUG_INFO*inf)
{
	word saddr = inf->sst.addr[1];
	word daddr = inf->sst.addr[0];
	for (;; ++saddr, ++daddr) {
		mem_write(daddr, mem_read(saddr, inf->sr), inf->sr);
		if (saddr == inf->sst.addr[2]) break;
	}
	inf->sst.addr[1] = saddr;
	inf->sst.addr[2] = (inf->sst.addr[1] + 1) | 7;
}

void sysmon_verify(struct DEBUG_INFO*inf)
{
	word saddr = inf->sst.addr[1];
	word daddr = inf->sst.addr[0];
	for (;; ++saddr, ++daddr) {
		byte bs, bd;
		bs = mem_read(saddr, inf->sr);
		bd = mem_read(daddr, inf->sr);
		if (bs != bd) 
			console_printf(inf->con, "\n%04X-%02X (%02X)", daddr, bd, bs);
		if (saddr == inf->sst.addr[2]) break;
	}
	inf->sst.addr[1] = saddr;
	inf->sst.addr[2] = (inf->sst.addr[1] + 1) | 7;
}

static TCHAR dname[MAX_PATH] = "dump.bin";

void sysmon_write(struct DEBUG_INFO*inf, int aarg)
{
	if (aarg!=6 || inf->sst.addr[1] > inf->sst.addr[2]) {
		MessageBeep(0);
		return;
	}
	console_hide(inf->con, 1);
	if (!select_save_dump(inf->sr->video_w, dname)) {
		console_hide(inf->con, 0);
		return;
	}
	console_hide(inf->con, 0);
	if (dump_mem(inf->sr, inf->sst.addr[1], inf->sst.addr[2] - inf->sst.addr[1]+1, dname)) {
		sysmon_err(inf);
	}
}

void sysmon_read(struct DEBUG_INFO*inf, int aarg)
{
	int sz = 0;
	int n;
	if (aarg & 4) sz = inf->sst.addr[2] - inf->sst.addr[1] + 1;
	console_hide(inf->con, 1);
	if (!select_open_dump(inf->sr->video_w, dname)) {
		console_hide(inf->con, 0);
		return;
	}
	console_hide(inf->con, 0);
	n = read_dump_mem(inf->sr, inf->sst.addr[1], sz, dname);
	if (n <= 0) {
		sysmon_err(inf);
		return;
	}
	inf->sst.addr[1] += n;
}

void sysmon_show_regs(struct DEBUG_INFO*inf)
{
	struct REGS_6502 regs;
	int r;
	r = system_command(inf->sr, SYS_COMMAND_GETREGS6502, 0, (long)&regs);
	if (r <= 0) {
		sysmon_err(inf);
		return;
	}
	console_printf(inf->con, "\n%04X-    A=%02X X=%02X Y=%02X P=%02X S=%02X",
		regs.PC, regs.A, regs.X, regs.Y, regs.F, regs.S);
	inf->sst.last_regs = 1;
	inf->sst.reg_index = 0;
}

void sysmon_set_reg(struct DEBUG_INFO*inf, byte data)
{
	static const int regitems[] = {
		REG6502_A, REG6502_X, REG6502_Y, REG6502_F, REG6502_S
	};
	int r;
	if (inf->sst.reg_index == sizeof(regitems) / sizeof(regitems[0])) return;
	r = system_command(inf->sr, SYS_COMMAND_SETREG, regitems[inf->sst.reg_index], data);
	if (r <= 0) {
		sysmon_err(inf);
		return;
	}
	++ inf->sst.reg_index;
}


void sysmon_help(struct DEBUG_INFO*inf)
{
	console_printf(inf->con, 
		"[a1][.a2]      Show memory dump a1..a2\n"
		"[a]:d1 [d2]... Enter data bytes\n"
		"[a]L           Disassemble memory\n"
		"a1[.a2]R       Read memory from file\n"
		"a1.a2W         Write memory to file\n"
		"a1<a2.a3M      Move memory\n"
		"a1<a2.a3V      Verify memory\n"
		"[a]G           Change execution flow\n"
		"P              Show/edit registers\n"
		"Q              Quit debugger\n"
		"?              Show this help"
	);
}

void parse_sysmon(struct DEBUG_INFO*inf, const con_char_t*buf)
{
	const con_char_t*p;
	word adr = 0;
	int adig = 0, aarg = 0;
	int lcmd = 0;
	byte arg1 = 0;
	for (p = buf; ; ++p) {
		int c = *p;
		if (c >= 'a' && c <= 'z') c += 'Z' - 'z';
		switch (c) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			adr<<=4;
			adr |= c - '0';
			++ adig;
			break;
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
			adr<<=4;
			adr |= c - 'A' + 10;
			++ adig;
			break;
		case 'L':
			if (adig) inf->sst.raddr = adr;
			adig = 0;
			adr = 0;
			aarg = 0;
			lcmd = c;
			sysmon_l(inf);
			break;
		case 'G':
			if (adig) inf->sst.raddr = adr;
			adig = 0;
			adr = 0;
			aarg = 0;
			lcmd = c;
			sysmon_g(inf);
			break;
		case '.':
			if (adig) inf->sst.addr[1] = adr;
			adig = 0;
			adr = 0;
			aarg |= 2;
			lcmd = c;
			break;
		case '<':
			if (adig) inf->sst.addr[0] = adr;
			adig = 0;
			adr = 0;
			aarg |= 1;
			lcmd = c;
			break;
		case '+':
			arg1 = adr;
			adig = 0;
			adr = 0;
			lcmd = c;
			break;
		case '-':
			arg1 = adr;
			adig = 0;
			adr = 0;
			lcmd = c;
			break;
		case ':':
			if (adig) {
				inf->sst.addr[1] = inf->sst.waddr = adr;
				inf->sst.addr[2] = (inf->sst.addr[1] + 1) | 7;
				inf->sst.last_regs = 0;
			}
			adig = 0;
			adr = 0;
			aarg = 0;
			lcmd = c;
			break;
		case 0:
		case ' ':
			if (p > buf && p[-1] == ' ') break;
			if (lcmd == ':') {
				if (!adig) break;
				if (inf->sst.last_regs)
					sysmon_set_reg(inf, adr);
				else {
					mem_write(inf->sst.waddr, adr, inf->sr);
					++ inf->sst.waddr;
				}
				adig = 0;
				adr = 0;
				aarg = 0;
				break;
			} else if (lcmd == '+') {
				byte sum = arg1 + adr;
				console_printf(inf->con, "=%02X", sum);
				adig = 0;
				adr = 0;
				lcmd = 0;
				aarg = 0;
				break;
			} else if (lcmd == '-') {
				byte sum = arg1 - adr;
				console_printf(inf->con, "=%02X", sum);
				adig = 0;
				adr = 0;
				lcmd = 0;
				aarg = 0;
				break;
			}
			if (adig) {
				if (aarg & 2) inf->sst.addr[2] = adr;
				else inf->sst.addr[1] = inf->sst.addr[2] = adr;
				inf->sst.waddr = inf->sst.addr[1];
				aarg |= 4;
			} else {
				if (aarg & 2 || lcmd) break;
			}
			sysmon_dump(inf, aarg);
			adig = 0;
			adr = 0;
			aarg = 0;
			lcmd = 0;
			break;
		case 'Q':
			sysmon_close(inf);
			return;
		case 'R':
			if (adig) {
				if (aarg & 2) {
					inf->sst.addr[2] = adr;
					aarg |= 4;
				} else {
					inf->sst.addr[1] = adr;
					aarg |= 2;
				}
			}	
			sysmon_read(inf, aarg);
			adig = 0;
			adr = 0;
			aarg = 0;
			lcmd = c;
			break;
		case 'W':
			if (adig) {
				if (aarg & 2) {
					inf->sst.addr[2] = adr;
					aarg |= 4;
				} else {
					inf->sst.addr[1] = adr;
					aarg |= 2;
				}
			}	
			sysmon_write(inf, aarg);
			adig = 0;
			adr = 0;
			aarg = 0;
			lcmd = c;
			break;
		case '?':
			sysmon_help(inf);
			adig = 0;
			adr = 0;
			aarg = 0;
			lcmd = c;
			break;
		case 'M':
			inf->sst.addr[2] = adr;
			sysmon_move(inf);
			adig = 0;
			adr = 0;
			aarg = 0;
			lcmd = c;
			break;
		case 'P':
			sysmon_show_regs(inf);
			adig = 0;
			adr = 0;
			aarg = 0;
			lcmd = c;
			break;
		case 'V':
			inf->sst.addr[2] = adr;
			sysmon_verify(inf);
			adig = 0;
			adr = 0;
			aarg = 0;
			lcmd = c;
			break;
		default:
			MessageBeep(0);
			return;
		}
		if (!c) break;
	}
	console_printf(inf->con, "\n");
}

DWORD CALLBACK dbg_thread(struct DEBUG_INFO*inf)
{
	con_char_t prompt = '*';
	console_printf(inf->con, TEXT("Agat emulator debugger console (c) Nop\n"));
	while (!inf->stop && !console_eof(inf->con)) {
		con_char_t buf[256];
		console_write(inf->con, &prompt, sizeof(prompt));
		console_gets(inf->con, buf, 256);
		if (console_eof(inf->con)) break;
		parse_sysmon(inf, buf);
	}
	return 0;
}


int debug_attach(struct SYS_RUN_STATE*sr)
{
	struct DEBUG_INFO*inf;
	DWORD tid;
	if (sr->debug_ptr) return 1;
	inf = calloc(1, sizeof(*inf));
	if (!inf) return -1;
	inf->sr = sr;
	inf->con = console_create(40,24,sr->title);
	if (!inf->con) {
		free(inf);
		return -2;
	}
	inf->dbg_thread = CreateThread(NULL, 0, dbg_thread, inf, 0, &tid);
	if (inf->dbg_thread == NULL) {
		console_free(inf->con);
		free(inf);
		return -3;
	}
	sr->debug_ptr = inf;
	return 0;
}

int debug_detach(struct SYS_RUN_STATE*sr)
{
	struct DEBUG_INFO*inf;
	if (!sr->debug_ptr) return -1;
	inf = sr->debug_ptr;
	sr->debug_ptr = NULL;
	inf->stop = TRUE;
	if (inf->con) console_free(inf->con);
	WaitForSingleObject(inf->dbg_thread, INFINITE);
	CloseHandle(inf->dbg_thread);
	free(inf);
	return 0;
}

