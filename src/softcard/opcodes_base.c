/* opcodes_base.c: unshifted Z80 opcodes
   Copyright (c) 1999-2003 Philip Kendall

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/
/* Some commonly used instructions */
#define AND(value)\
{\
  A &= (value);\
  F = FLAG_H | sz53p_table[A];\
}

#define ADC(value)\
{\
  libspectrum_word adctemp = A + (value) + ( F & FLAG_C ); \
  libspectrum_byte lookup = ( (       A & 0x88 ) >> 3 ) | \
			    ( ( (value) & 0x88 ) >> 2 ) | \
			    ( ( adctemp & 0x88 ) >> 1 );  \
  A=adctemp;\
  F = ( adctemp & 0x100 ? FLAG_C : 0 ) |\
    halfcarry_add_table[lookup & 0x07] | overflow_add_table[lookup >> 4] |\
    sz53_table[A];\
}

#define ADC16(value)\
{\
  libspectrum_dword add16temp= HL + (value) + ( F & FLAG_C ); \
  libspectrum_byte lookup = ( (        HL & 0x8800 ) >> 11 ) | \
			    ( (   (value) & 0x8800 ) >> 10 ) | \
			    ( ( add16temp & 0x8800 ) >>  9 );  \
  HL = add16temp;\
  F = ( add16temp & 0x10000 ? FLAG_C : 0 )|\
    overflow_add_table[lookup >> 4] |\
    ( H & ( FLAG_3 | FLAG_5 | FLAG_S ) ) |\
    halfcarry_add_table[lookup&0x07]|\
    ( HL ? 0 : FLAG_Z );\
}

#define ADD(value)\
{\
  libspectrum_word addtemp = A + (value); \
  libspectrum_byte lookup = ( (       A & 0x88 ) >> 3 ) | \
			    ( ( (value) & 0x88 ) >> 2 ) | \
			    ( ( addtemp & 0x88 ) >> 1 );  \
  A=addtemp;\
  F = ( addtemp & 0x100 ? FLAG_C : 0 ) |\
    halfcarry_add_table[lookup & 0x07] | overflow_add_table[lookup >> 4] |\
    sz53_table[A];\
}

#define ADD16(value1,value2)\
{\
  libspectrum_dword add16temp = (value1) + (value2); \
  libspectrum_byte lookup = ( (  (value1) & 0x0800 ) >> 11 ) | \
			    ( (  (value2) & 0x0800 ) >> 10 ) | \
			    ( ( add16temp & 0x0800 ) >>  9 );  \
  (value1) = add16temp;\
  F = ( F & ( FLAG_V | FLAG_Z | FLAG_S ) ) |\
    ( add16temp & 0x10000 ? FLAG_C : 0 )|\
    ( ( add16temp >> 8 ) & ( FLAG_3 | FLAG_5 ) ) |\
    halfcarry_add_table[lookup];\
}

/* This may look fairly inefficient, but the (gcc) optimiser does the
   right thing assuming it's given a constant for 'bit' */
#define BIT( bit, value ) \
{ \
  F = ( F & FLAG_C ) | FLAG_H | ( value & ( FLAG_3 | FLAG_5 ) ); \
  if( ! ( (value) & ( 0x01 << (bit) ) ) ) F |= FLAG_P | FLAG_Z; \
  if( (bit) == 7 && (value) & 0x80 ) F |= FLAG_S; \
}

#define BIT_I( bit, value, address ) \
{ \
  F = ( F & FLAG_C ) | FLAG_H | ( ( address >> 8 ) & ( FLAG_3 | FLAG_5 ) ); \
  if( ! ( (value) & ( 0x01 << (bit) ) ) ) F |= FLAG_P | FLAG_Z; \
  if( (bit) == 7 && (value) & 0x80 ) F |= FLAG_S; \
}  

#define CALL()\
{\
  libspectrum_byte calltempl, calltemph; \
  calltempl=readbyte(PC++);\
  calltemph=readbyte( PC ); \
  contend_read_no_mreq( PC, 1 ); PC++;\
  PUSH16(PCL,PCH);\
  PCL=calltempl; PCH=calltemph;\
}

#define CP(value)\
{\
  libspectrum_word cptemp = A - value; \
  libspectrum_byte lookup = ( (       A & 0x88 ) >> 3 ) | \
			    ( ( (value) & 0x88 ) >> 2 ) | \
			    ( (  cptemp & 0x88 ) >> 1 );  \
  F = ( cptemp & 0x100 ? FLAG_C : ( cptemp ? 0 : FLAG_Z ) ) | FLAG_N |\
    halfcarry_sub_table[lookup & 0x07] |\
    overflow_sub_table[lookup >> 4] |\
    ( value & ( FLAG_3 | FLAG_5 ) ) |\
    ( cptemp & FLAG_S );\
}

/* Macro for the {DD,FD} CB dd xx rotate/shift instructions */
#define DDFDCB_ROTATESHIFT(time, target, instruction)\
tstates+=(time);\
{\
  (target) = readbyte( tempaddr );\
  instruction( (target) );\
  writebyte( tempaddr, (target) );\
}\
break

#define DEC(value)\
{\
  F = ( F & FLAG_C ) | ( (value)&0x0f ? 0 : FLAG_H ) | FLAG_N;\
  (value)--;\
  F |= ( (value)==0x7f ? FLAG_V : 0 ) | sz53_table[value];\
}

#define Z80_IN( reg, port )\
{\
  (reg)=readport((port));\
  F = ( F & FLAG_C) | sz53p_table[(reg)];\
}

#define INC(value)\
{\
  (value)++;\
  F = ( F & FLAG_C ) | ( (value)==0x80 ? FLAG_V : 0 ) |\
  ( (value)&0x0f ? 0 : FLAG_H ) | sz53_table[(value)];\
}

#define LD16_NNRR(regl,regh)\
{\
  libspectrum_word ldtemp; \
  ldtemp=readbyte(PC++);\
  ldtemp|=readbyte(PC++) << 8;\
  writebyte(ldtemp++,(regl));\
  writebyte(ldtemp,(regh));\
  break;\
}

#define LD16_RRNN(regl,regh)\
{\
  libspectrum_word ldtemp; \
  ldtemp=readbyte(PC++);\
  ldtemp|=readbyte(PC++) << 8;\
  (regl)=readbyte(ldtemp++);\
  (regh)=readbyte(ldtemp);\
  break;\
}

#define JP()\
{\
  libspectrum_word jptemp=PC; \
  PCL=readbyte(jptemp++);\
  PCH=readbyte(jptemp);\
}

#define JR()\
{\
  libspectrum_signed_byte jrtemp = readbyte( PC ); \
  contend_read_no_mreq( PC, 1 ); contend_read_no_mreq( PC, 1 ); \
  contend_read_no_mreq( PC, 1 ); contend_read_no_mreq( PC, 1 ); \
  contend_read_no_mreq( PC, 1 ); \
  PC += jrtemp; \
}

#define OR(value)\
{\
  A |= (value);\
  F = sz53p_table[A];\
}

#define POP16(regl,regh)\
{\
  (regl)=readbyte(SP++);\
  (regh)=readbyte(SP++);\
}

#define PUSH16(regl,regh)\
{\
  writebyte( --SP, (regh) );\
  writebyte( --SP, (regl) );\
}

#define RET()\
{\
  POP16(PCL,PCH);\
}

#define RL(value)\
{\
  libspectrum_byte rltemp = (value); \
  (value) = ( (value)<<1 ) | ( F & FLAG_C );\
  F = ( rltemp >> 7 ) | sz53p_table[(value)];\
}

#define RLC(value)\
{\
  (value) = ( (value)<<1 ) | ( (value)>>7 );\
  F = ( (value) & FLAG_C ) | sz53p_table[(value)];\
}

#define RR(value)\
{\
  libspectrum_byte rrtemp = (value); \
  (value) = ( (value)>>1 ) | ( F << 7 );\
  F = ( rrtemp & FLAG_C ) | sz53p_table[(value)];\
}

#define RRC(value)\
{\
  F = (value) & FLAG_C;\
  (value) = ( (value)>>1 ) | ( (value)<<7 );\
  F |= sz53p_table[(value)];\
}

#define RST(value)\
{\
  PUSH16(PCL,PCH);\
  PC=(value);\
}

#define SBC(value)\
{\
  libspectrum_word sbctemp = A - (value) - ( F & FLAG_C ); \
  libspectrum_byte lookup = ( (       A & 0x88 ) >> 3 ) | \
			    ( ( (value) & 0x88 ) >> 2 ) | \
			    ( ( sbctemp & 0x88 ) >> 1 );  \
  A=sbctemp;\
  F = ( sbctemp & 0x100 ? FLAG_C : 0 ) | FLAG_N |\
    halfcarry_sub_table[lookup & 0x07] | overflow_sub_table[lookup >> 4] |\
    sz53_table[A];\
}

#define SBC16(value)\
{\
  libspectrum_dword sub16temp = HL - (value) - (F & FLAG_C); \
  libspectrum_byte lookup = ( (        HL & 0x8800 ) >> 11 ) | \
			    ( (   (value) & 0x8800 ) >> 10 ) | \
			    ( ( sub16temp & 0x8800 ) >>  9 );  \
  HL = sub16temp;\
  F = ( sub16temp & 0x10000 ? FLAG_C : 0 ) |\
    FLAG_N | overflow_sub_table[lookup >> 4] |\
    ( H & ( FLAG_3 | FLAG_5 | FLAG_S ) ) |\
    halfcarry_sub_table[lookup&0x07] |\
    ( HL ? 0 : FLAG_Z) ;\
}

#define SLA(value)\
{\
  F = (value) >> 7;\
  (value) <<= 1;\
  F |= sz53p_table[(value)];\
}

#define SLL(value)\
{\
  F = (value) >> 7;\
  (value) = ( (value) << 1 ) | 0x01;\
  F |= sz53p_table[(value)];\
}

#define SRA(value)\
{\
  F = (value) & FLAG_C;\
  (value) = ( (value) & 0x80 ) | ( (value) >> 1 );\
  F |= sz53p_table[(value)];\
}

#define SRL(value)\
{\
  F = (value) & FLAG_C;\
  (value) >>= 1;\
  F |= sz53p_table[(value)];\
}

#define SUB(value)\
{\
  libspectrum_word subtemp = A - (value); \
  libspectrum_byte lookup = ( (       A & 0x88 ) >> 3 ) | \
			    ( ( (value) & 0x88 ) >> 2 ) | \
			    (  (subtemp & 0x88 ) >> 1 );  \
  A=subtemp;\
  F = ( subtemp & 0x100 ? FLAG_C : 0 ) | FLAG_N |\
    halfcarry_sub_table[lookup & 0x07] | overflow_sub_table[lookup >> 4] |\
    sz53_table[A];\
}

#define XOR(value)\
{\
  A ^= (value);\
  F = sz53p_table[A];\
}


    case 0x00:		/* NOP */
      break;
    case 0x01:		/* LD BC,nnnn */
      C=readbyte(PC++);
      B=readbyte(PC++);
      break;
    case 0x02:		/* LD (BC),A */
      writebyte(BC,A);
      break;
    case 0x03:		/* INC BC */
	contend_read_no_mreq( IR, 1 );
	contend_read_no_mreq( IR, 1 );
	BC++;
      break;
    case 0x04:		/* INC B */
      INC(B);
      break;
    case 0x05:		/* DEC B */
      DEC(B);
      break;
    case 0x06:		/* LD B,nn */
      B = readbyte( PC++ );
      break;
    case 0x07:		/* RLCA */
      A = ( A << 1 ) | ( A >> 7 );
      F = ( F & ( FLAG_P | FLAG_Z | FLAG_S ) ) |
	( A & ( FLAG_C | FLAG_3 | FLAG_5 ) );
      break;
    case 0x08:		/* EX AF,AF' */
      /* Tape saving trap: note this traps the EX AF,AF' at #04d0, not
	 #04d1 as PC has already been incremented */
      /* 0x76 - Timex 2068 save routine in EXROM */
/*      if( PC == 0x04d1 || PC == 0x0077 ) {
	if( tape_save_trap() == 0 ) break;
      }*/

      {
	libspectrum_word wordtemp = AF; AF = AF_; AF_ = wordtemp;
      }
      break;
    case 0x09:		/* ADD HL,BC */
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      ADD16(HL,BC);
      break;
    case 0x0a:		/* LD A,(BC) */
      A=readbyte(BC);
      break;
    case 0x0b:		/* DEC BC */
	contend_read_no_mreq( IR, 1 );
	contend_read_no_mreq( IR, 1 );
	BC--;
      break;
    case 0x0c:		/* INC C */
      INC(C);
      break;
    case 0x0d:		/* DEC C */
      DEC(C);
      break;
    case 0x0e:		/* LD C,nn */
      C = readbyte( PC++ );
      break;
    case 0x0f:		/* RRCA */
      F = ( F & ( FLAG_P | FLAG_Z | FLAG_S ) ) | ( A & FLAG_C );
      A = ( A >> 1) | ( A << 7 );
      F |= ( A & ( FLAG_3 | FLAG_5 ) );
      break;
    case 0x10:		/* DJNZ offset */
      contend_read_no_mreq( IR, 1 );
      B--;
      if(B) {
	JR();
      } else {
	contend_read( PC, 3 );
      }
      PC++;
      break;
    case 0x11:		/* LD DE,nnnn */
      E=readbyte(PC++);
      D=readbyte(PC++);
      break;
    case 0x12:		/* LD (DE),A */
      writebyte(DE,A);
      break;
    case 0x13:		/* INC DE */
	contend_read_no_mreq( IR, 1 );
	contend_read_no_mreq( IR, 1 );
	DE++;
      break;
    case 0x14:		/* INC D */
      INC(D);
      break;
    case 0x15:		/* DEC D */
      DEC(D);
      break;
    case 0x16:		/* LD D,nn */
      D = readbyte( PC++ );
      break;
    case 0x17:		/* RLA */
      {
	libspectrum_byte bytetemp = A;
	A = ( A << 1 ) | ( F & FLAG_C );
	F = ( F & ( FLAG_P | FLAG_Z | FLAG_S ) ) |
	  ( A & ( FLAG_3 | FLAG_5 ) ) | ( bytetemp >> 7 );
      }
      break;
    case 0x18:		/* JR offset */
      JR();
      PC++;
      break;
    case 0x19:		/* ADD HL,DE */
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      ADD16(HL,DE);
      break;
    case 0x1a:		/* LD A,(DE) */
      A=readbyte(DE);
      break;
    case 0x1b:		/* DEC DE */
	contend_read_no_mreq( IR, 1 );
	contend_read_no_mreq( IR, 1 );
	DE--;
      break;
    case 0x1c:		/* INC E */
      INC(E);
      break;
    case 0x1d:		/* DEC E */
      DEC(E);
      break;
    case 0x1e:		/* LD E,nn */
      E = readbyte( PC++ );
      break;
    case 0x1f:		/* RRA */
      {
	libspectrum_byte bytetemp = A;
	A = ( A >> 1 ) | ( F << 7 );
	F = ( F & ( FLAG_P | FLAG_Z | FLAG_S ) ) |
	  ( A & ( FLAG_3 | FLAG_5 ) ) | ( bytetemp & FLAG_C ) ;
      }
      break;
    case 0x20:		/* JR NZ,offset */
      if( ! ( F & FLAG_Z ) ) {
        JR();
      } else {
        contend_read( PC, 3 );
      }
      PC++;
      break;
    case 0x21:		/* LD HL,nnnn */
      L=readbyte(PC++);
      H=readbyte(PC++);
      break;
    case 0x22:		/* LD (nnnn),HL */
      LD16_NNRR(L,H);
      break;
    case 0x23:		/* INC HL */
	contend_read_no_mreq( IR, 1 );
	contend_read_no_mreq( IR, 1 );
	HL++;
      break;
    case 0x24:		/* INC H */
      INC(H);
      break;
    case 0x25:		/* DEC H */
      DEC(H);
      break;
    case 0x26:		/* LD H,nn */
      H = readbyte( PC++ );
      break;
    case 0x27:		/* DAA */
      {
	libspectrum_byte add = 0, carry = ( F & FLAG_C );
	if( ( F & FLAG_H ) || ( ( A & 0x0f ) > 9 ) ) add = 6;
	if( carry || ( A > 0x99 ) ) add |= 0x60;
	if( A > 0x99 ) carry = FLAG_C;
	if( F & FLAG_N ) {
	  SUB(add);
	} else {
	  ADD(add);
	}
	F = ( F & ~( FLAG_C | FLAG_P ) ) | carry | parity_table[A];
      }
      break;
    case 0x28:		/* JR Z,offset */
      if( F & FLAG_Z ) {
        JR();
      } else {
        contend_read( PC, 3 );
      }
      PC++;
      break;
    case 0x29:		/* ADD HL,HL */
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      ADD16(HL,HL);
      break;
    case 0x2a:		/* LD HL,(nnnn) */
      LD16_RRNN(L,H);
      break;
    case 0x2b:		/* DEC HL */
	contend_read_no_mreq( IR, 1 );
	contend_read_no_mreq( IR, 1 );
	HL--;
      break;
    case 0x2c:		/* INC L */
      INC(L);
      break;
    case 0x2d:		/* DEC L */
      DEC(L);
      break;
    case 0x2e:		/* LD L,nn */
      L = readbyte( PC++ );
      break;
    case 0x2f:		/* CPL */
      A ^= 0xff;
      F = ( F & ( FLAG_C | FLAG_P | FLAG_Z | FLAG_S ) ) |
	( A & ( FLAG_3 | FLAG_5 ) ) | ( FLAG_N | FLAG_H );
      break;
    case 0x30:		/* JR NC,offset */
      if( ! ( F & FLAG_C ) ) {
        JR();
      } else {
        contend_read( PC, 3 );
      }
      PC++;
      break;
    case 0x31:		/* LD SP,nnnn */
      SPL=readbyte(PC++);
      SPH=readbyte(PC++);
      break;
    case 0x32:		/* LD (nnnn),A */
      {
	libspectrum_word wordtemp = readbyte( PC++ );
	wordtemp|=readbyte(PC++) << 8;
	writebyte(wordtemp,A);
      }
      break;
    case 0x33:		/* INC SP */
	contend_read_no_mreq( IR, 1 );
	contend_read_no_mreq( IR, 1 );
	SP++;
      break;
    case 0x34:		/* INC (HL) */
      {
	libspectrum_byte bytetemp = readbyte( HL );
	contend_read_no_mreq( HL, 1 );
	INC(bytetemp);
	writebyte(HL,bytetemp);
      }
      break;
    case 0x35:		/* DEC (HL) */
      {
	libspectrum_byte bytetemp = readbyte( HL );
	contend_read_no_mreq( HL, 1 );
	DEC(bytetemp);
	writebyte(HL,bytetemp);
      }
      break;
    case 0x36:		/* LD (HL),nn */
      writebyte(HL,readbyte(PC++));
      break;
    case 0x37:		/* SCF */
      F = ( F & ( FLAG_P | FLAG_Z | FLAG_S ) ) |
	  ( A & ( FLAG_3 | FLAG_5          ) ) |
	  FLAG_C;
      break;
    case 0x38:		/* JR C,offset */
      if( F & FLAG_C ) {
        JR();
      } else {
        contend_read( PC, 3 );
      }
      PC++;
      break;
    case 0x39:		/* ADD HL,SP */
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      ADD16(HL,SP);
      break;
    case 0x3a:		/* LD A,(nnnn) */
      {
	libspectrum_word wordtemp;
	wordtemp = readbyte(PC++);
	wordtemp|= ( readbyte(PC++) << 8 );
	A=readbyte(wordtemp);
      }
      break;
    case 0x3b:		/* DEC SP */
	contend_read_no_mreq( IR, 1 );
	contend_read_no_mreq( IR, 1 );
	SP--;
      break;
    case 0x3c:		/* INC A */
      INC(A);
      break;
    case 0x3d:		/* DEC A */
      DEC(A);
      break;
    case 0x3e:		/* LD A,nn */
      A = readbyte( PC++ );
      break;
    case 0x3f:		/* CCF */
      F = ( F & ( FLAG_P | FLAG_Z | FLAG_S ) ) |
	( ( F & FLAG_C ) ? FLAG_H : FLAG_C ) | ( A & ( FLAG_3 | FLAG_5 ) );
      break;
    case 0x40:		/* LD B,B */
      break;
    case 0x41:		/* LD B,C */
      B=C;
      break;
    case 0x42:		/* LD B,D */
      B=D;
      break;
    case 0x43:		/* LD B,E */
      B=E;
      break;
    case 0x44:		/* LD B,H */
      B=H;
      break;
    case 0x45:		/* LD B,L */
      B=L;
      break;
    case 0x46:		/* LD B,(HL) */
      B=readbyte(HL);
      break;
    case 0x47:		/* LD B,A */
      B=A;
      break;
    case 0x48:		/* LD C,B */
      C=B;
      break;
    case 0x49:		/* LD C,C */
      break;
    case 0x4a:		/* LD C,D */
      C=D;
      break;
    case 0x4b:		/* LD C,E */
      C=E;
      break;
    case 0x4c:		/* LD C,H */
      C=H;
      break;
    case 0x4d:		/* LD C,L */
      C=L;
      break;
    case 0x4e:		/* LD C,(HL) */
      C=readbyte(HL);
      break;
    case 0x4f:		/* LD C,A */
      C=A;
      break;
    case 0x50:		/* LD D,B */
      D=B;
      break;
    case 0x51:		/* LD D,C */
      D=C;
      break;
    case 0x52:		/* LD D,D */
      break;
    case 0x53:		/* LD D,E */
      D=E;
      break;
    case 0x54:		/* LD D,H */
      D=H;
      break;
    case 0x55:		/* LD D,L */
      D=L;
      break;
    case 0x56:		/* LD D,(HL) */
      D=readbyte(HL);
      break;
    case 0x57:		/* LD D,A */
      D=A;
      break;
    case 0x58:		/* LD E,B */
      E=B;
      break;
    case 0x59:		/* LD E,C */
      E=C;
      break;
    case 0x5a:		/* LD E,D */
      E=D;
      break;
    case 0x5b:		/* LD E,E */
      break;
    case 0x5c:		/* LD E,H */
      E=H;
      break;
    case 0x5d:		/* LD E,L */
      E=L;
      break;
    case 0x5e:		/* LD E,(HL) */
      E=readbyte(HL);
      break;
    case 0x5f:		/* LD E,A */
      E=A;
      break;
    case 0x60:		/* LD H,B */
      H=B;
      break;
    case 0x61:		/* LD H,C */
      H=C;
      break;
    case 0x62:		/* LD H,D */
      H=D;
      break;
    case 0x63:		/* LD H,E */
      H=E;
      break;
    case 0x64:		/* LD H,H */
      break;
    case 0x65:		/* LD H,L */
      H=L;
      break;
    case 0x66:		/* LD H,(HL) */
      H=readbyte(HL);
      break;
    case 0x67:		/* LD H,A */
      H=A;
      break;
    case 0x68:		/* LD L,B */
      L=B;
      break;
    case 0x69:		/* LD L,C */
      L=C;
      break;
    case 0x6a:		/* LD L,D */
      L=D;
      break;
    case 0x6b:		/* LD L,E */
      L=E;
      break;
    case 0x6c:		/* LD L,H */
      L=H;
      break;
    case 0x6d:		/* LD L,L */
      break;
    case 0x6e:		/* LD L,(HL) */
      L=readbyte(HL);
      break;
    case 0x6f:		/* LD L,A */
      L=A;
      break;
    case 0x70:		/* LD (HL),B */
      writebyte(HL,B);
      break;
    case 0x71:		/* LD (HL),C */
      writebyte(HL,C);
      break;
    case 0x72:		/* LD (HL),D */
      writebyte(HL,D);
      break;
    case 0x73:		/* LD (HL),E */
      writebyte(HL,E);
      break;
    case 0x74:		/* LD (HL),H */
      writebyte(HL,H);
      break;
    case 0x75:		/* LD (HL),L */
      writebyte(HL,L);
      break;
    case 0x76:		/* HALT */
      st->halted=1;
      PC--;
      break;
    case 0x77:		/* LD (HL),A */
      writebyte(HL,A);
      break;
    case 0x78:		/* LD A,B */
      A=B;
      break;
    case 0x79:		/* LD A,C */
      A=C;
      break;
    case 0x7a:		/* LD A,D */
      A=D;
      break;
    case 0x7b:		/* LD A,E */
      A=E;
      break;
    case 0x7c:		/* LD A,H */
      A=H;
      break;
    case 0x7d:		/* LD A,L */
      A=L;
      break;
    case 0x7e:		/* LD A,(HL) */
      A=readbyte(HL);
      break;
    case 0x7f:		/* LD A,A */
      break;
    case 0x80:		/* ADD A,B */
      ADD(B);
      break;
    case 0x81:		/* ADD A,C */
      ADD(C);
      break;
    case 0x82:		/* ADD A,D */
      ADD(D);
      break;
    case 0x83:		/* ADD A,E */
      ADD(E);
      break;
    case 0x84:		/* ADD A,H */
      ADD(H);
      break;
    case 0x85:		/* ADD A,L */
      ADD(L);
      break;
    case 0x86:		/* ADD A,(HL) */
      {
	libspectrum_byte bytetemp = readbyte( HL );
	ADD(bytetemp);
      }
      break;
    case 0x87:		/* ADD A,A */
      ADD(A);
      break;
    case 0x88:		/* ADC A,B */
      ADC(B);
      break;
    case 0x89:		/* ADC A,C */
      ADC(C);
      break;
    case 0x8a:		/* ADC A,D */
      ADC(D);
      break;
    case 0x8b:		/* ADC A,E */
      ADC(E);
      break;
    case 0x8c:		/* ADC A,H */
      ADC(H);
      break;
    case 0x8d:		/* ADC A,L */
      ADC(L);
      break;
    case 0x8e:		/* ADC A,(HL) */
      {
	libspectrum_byte bytetemp = readbyte( HL );
	ADC(bytetemp);
      }
      break;
    case 0x8f:		/* ADC A,A */
      ADC(A);
      break;
    case 0x90:		/* SUB A,B */
      SUB(B);
      break;
    case 0x91:		/* SUB A,C */
      SUB(C);
      break;
    case 0x92:		/* SUB A,D */
      SUB(D);
      break;
    case 0x93:		/* SUB A,E */
      SUB(E);
      break;
    case 0x94:		/* SUB A,H */
      SUB(H);
      break;
    case 0x95:		/* SUB A,L */
      SUB(L);
      break;
    case 0x96:		/* SUB A,(HL) */
      {
	libspectrum_byte bytetemp = readbyte( HL );
	SUB(bytetemp);
      }
      break;
    case 0x97:		/* SUB A,A */
      SUB(A);
      break;
    case 0x98:		/* SBC A,B */
      SBC(B);
      break;
    case 0x99:		/* SBC A,C */
      SBC(C);
      break;
    case 0x9a:		/* SBC A,D */
      SBC(D);
      break;
    case 0x9b:		/* SBC A,E */
      SBC(E);
      break;
    case 0x9c:		/* SBC A,H */
      SBC(H);
      break;
    case 0x9d:		/* SBC A,L */
      SBC(L);
      break;
    case 0x9e:		/* SBC A,(HL) */
      {
	libspectrum_byte bytetemp = readbyte( HL );
	SBC(bytetemp);
      }
      break;
    case 0x9f:		/* SBC A,A */
      SBC(A);
      break;
    case 0xa0:		/* AND A,B */
      AND(B);
      break;
    case 0xa1:		/* AND A,C */
      AND(C);
      break;
    case 0xa2:		/* AND A,D */
      AND(D);
      break;
    case 0xa3:		/* AND A,E */
      AND(E);
      break;
    case 0xa4:		/* AND A,H */
      AND(H);
      break;
    case 0xa5:		/* AND A,L */
      AND(L);
      break;
    case 0xa6:		/* AND A,(HL) */
      {
	libspectrum_byte bytetemp = readbyte( HL );
	AND(bytetemp);
      }
      break;
    case 0xa7:		/* AND A,A */
      AND(A);
      break;
    case 0xa8:		/* XOR A,B */
      XOR(B);
      break;
    case 0xa9:		/* XOR A,C */
      XOR(C);
      break;
    case 0xaa:		/* XOR A,D */
      XOR(D);
      break;
    case 0xab:		/* XOR A,E */
      XOR(E);
      break;
    case 0xac:		/* XOR A,H */
      XOR(H);
      break;
    case 0xad:		/* XOR A,L */
      XOR(L);
      break;
    case 0xae:		/* XOR A,(HL) */
      {
	libspectrum_byte bytetemp = readbyte( HL );
	XOR(bytetemp);
      }
      break;
    case 0xaf:		/* XOR A,A */
      XOR(A);
      break;
    case 0xb0:		/* OR A,B */
      OR(B);
      break;
    case 0xb1:		/* OR A,C */
      OR(C);
      break;
    case 0xb2:		/* OR A,D */
      OR(D);
      break;
    case 0xb3:		/* OR A,E */
      OR(E);
      break;
    case 0xb4:		/* OR A,H */
      OR(H);
      break;
    case 0xb5:		/* OR A,L */
      OR(L);
      break;
    case 0xb6:		/* OR A,(HL) */
      {
	libspectrum_byte bytetemp = readbyte( HL );
	OR(bytetemp);
      }
      break;
    case 0xb7:		/* OR A,A */
      OR(A);
      break;
    case 0xb8:		/* CP B */
      CP(B);
      break;
    case 0xb9:		/* CP C */
      CP(C);
      break;
    case 0xba:		/* CP D */
      CP(D);
      break;
    case 0xbb:		/* CP E */
      CP(E);
      break;
    case 0xbc:		/* CP H */
      CP(H);
      break;
    case 0xbd:		/* CP L */
      CP(L);
      break;
    case 0xbe:		/* CP (HL) */
      {
	libspectrum_byte bytetemp = readbyte( HL );
	CP(bytetemp);
      }
      break;
    case 0xbf:		/* CP A */
      CP(A);
      break;
    case 0xc0:		/* RET NZ */
      contend_read_no_mreq( IR, 1 );
/*      if( PC==0x056c || PC == 0x0112 ) {
	if( tape_load_trap() == 0 ) break;
      }*/
      if( ! ( F & FLAG_Z ) ) { RET(); }
      break;
    case 0xc1:		/* POP BC */
      POP16(C,B);
      break;
    case 0xc2:		/* JP NZ,nnnn */
      if( ! ( F & FLAG_Z ) ) {
	JP();
      } else {
	contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
      }
      break;
    case 0xc3:		/* JP nnnn */
      JP();
      break;
    case 0xc4:		/* CALL NZ,nnnn */
      if( ! ( F & FLAG_Z ) ) {
	CALL();
      } else {
	contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
      }
      break;
    case 0xc5:		/* PUSH BC */
      contend_read_no_mreq( IR, 1 );
      PUSH16(C,B);
      break;
    case 0xc6:		/* ADD A,nn */
      {
	libspectrum_byte bytetemp = readbyte( PC++ );
	ADD(bytetemp);
      }
      break;
    case 0xc7:		/* RST 00 */
      contend_read_no_mreq( IR, 1 );
      RST(0x00);
      break;
    case 0xc8:		/* RET Z */
      contend_read_no_mreq( IR, 1 );
      if( F & FLAG_Z ) { RET(); }
      break;
    case 0xc9:		/* RET */
      RET();
      break;
    case 0xca:		/* JP Z,nnnn */
      if( F & FLAG_Z ) {
	JP();
      } else {
	contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
      }
      break;
    case 0xcb:		/* shift CB */
      {
	libspectrum_byte opcode2;
	contend_read( PC, 4 );
	opcode2 = fetch_cmd_byte(st);
#ifdef HAVE_ENOUGH_MEMORY
	switch(opcode2) {
#include "z80_cb.c"
	}
#else			/* #ifdef HAVE_ENOUGH_MEMORY */
	if( z80_cbxx(st, opcode2) ) goto end_opcode;
#endif			/* #ifdef HAVE_ENOUGH_MEMORY */
      }
      break;
    case 0xcc:		/* CALL Z,nnnn */
      if( F & FLAG_Z ) {
	CALL();
      } else {
	contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
      }
      break;
    case 0xcd:		/* CALL nnnn */
      CALL();
      break;
    case 0xce:		/* ADC A,nn */
      {
	libspectrum_byte bytetemp = readbyte( PC++ );
	ADC(bytetemp);
      }
      break;
    case 0xcf:		/* RST 8 */
      contend_read_no_mreq( IR, 1 );
      RST(0x08);
      break;
    case 0xd0:		/* RET NC */
      contend_read_no_mreq( IR, 1 );
      if( ! ( F & FLAG_C ) ) { RET(); }
      break;
    case 0xd1:		/* POP DE */
      POP16(E,D);
      break;
    case 0xd2:		/* JP NC,nnnn */
      if( ! ( F & FLAG_C ) ) {
	JP();
      } else {
	contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
      }
      break;
    case 0xd3:		/* OUT (nn),A */
      { 
	libspectrum_word outtemp;
	outtemp = readbyte( PC++ ) + ( A << 8 );
	writeport( outtemp, A );
      }
      break;
    case 0xd4:		/* CALL NC,nnnn */
      if( ! ( F & FLAG_C ) ) {
	CALL();
      } else {
	contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
      }
      break;
    case 0xd5:		/* PUSH DE */
      contend_read_no_mreq( IR, 1 );
      PUSH16(E,D);
      break;
    case 0xd6:		/* SUB nn */
      {
	libspectrum_byte bytetemp = readbyte( PC++ );
	SUB(bytetemp);
      }
      break;
    case 0xd7:		/* RST 10 */
      contend_read_no_mreq( IR, 1 );
      RST(0x10);
      break;
    case 0xd8:		/* RET C */
      contend_read_no_mreq( IR, 1 );
      if( F & FLAG_C ) { RET(); }
      break;
    case 0xd9:		/* EXX */
      {
	libspectrum_word wordtemp;
	wordtemp = BC; BC = BC_; BC_ = wordtemp;
	wordtemp = DE; DE = DE_; DE_ = wordtemp;
	wordtemp = HL; HL = HL_; HL_ = wordtemp;
      }
      break;
    case 0xda:		/* JP C,nnnn */
      if( F & FLAG_C ) {
	JP();
      } else {
	contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
      }
      break;
    case 0xdb:		/* IN A,(nn) */
      { 
	libspectrum_word intemp;
	intemp = readbyte( PC++ ) + ( A << 8 );
        A=readport( intemp );
      }
      break;
    case 0xdc:		/* CALL C,nnnn */
      if( F & FLAG_C ) {
	CALL();
      } else {
	contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
      }
      break;
    case 0xdd:		/* shift DD */
      {
	libspectrum_byte opcode2;
	contend_read( PC, 4 );
	opcode2 = fetch_cmd_byte(st);
#ifdef HAVE_ENOUGH_MEMORY
	switch(opcode2) {
#define REGISTER  IX
#define REGISTERL IXL
#define REGISTERH IXH
#include "z80_ddfd.c"
#undef REGISTERH
#undef REGISTERL
#undef REGISTER
	}
#else			/* #ifdef HAVE_ENOUGH_MEMORY */
	if( z80_ddxx(st, opcode2) ) goto end_opcode;
#endif			/* #ifdef HAVE_ENOUGH_MEMORY */
      }
      break;
    case 0xde:		/* SBC A,nn */
      {
	libspectrum_byte bytetemp = readbyte( PC++ );
	SBC(bytetemp);
      }
      break;
    case 0xdf:		/* RST 18 */
      contend_read_no_mreq( IR, 1 );
      RST(0x18);
      break;
    case 0xe0:		/* RET PO */
      contend_read_no_mreq( IR, 1 );
      if( ! ( F & FLAG_P ) ) { RET(); }
      break;
    case 0xe1:		/* POP HL */
      POP16(L,H);
      break;
    case 0xe2:		/* JP PO,nnnn */
      if( ! ( F & FLAG_P ) ) {
	JP();
      } else {
	contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
      }
      break;
    case 0xe3:		/* EX (SP),HL */
      {
	libspectrum_byte bytetempl, bytetemph;
	bytetempl = readbyte( SP );
	bytetemph = readbyte( SP + 1 ); contend_read_no_mreq( SP + 1, 1 );
	writebyte( SP + 1, H );
	writebyte( SP,     L  );
	contend_write_no_mreq( SP, 1 ); contend_write_no_mreq( SP, 1 );
	L=bytetempl; H=bytetemph;
      }
      break;
    case 0xe4:		/* CALL PO,nnnn */
      if( ! ( F & FLAG_P ) ) {
	CALL();
      } else {
	contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
      }
      break;
    case 0xe5:		/* PUSH HL */
      contend_read_no_mreq( IR, 1 );
      PUSH16(L,H);
      break;
    case 0xe6:		/* AND nn */
      {
	libspectrum_byte bytetemp = readbyte( PC++ );
	AND(bytetemp);
      }
      break;
    case 0xe7:		/* RST 20 */
      contend_read_no_mreq( IR, 1 );
      RST(0x20);
      break;
    case 0xe8:		/* RET PE */
      contend_read_no_mreq( IR, 1 );
      if( F & FLAG_P ) { RET(); }
      break;
    case 0xe9:		/* JP HL */
      PC=HL;		/* NB: NOT INDIRECT! */
      break;
    case 0xea:		/* JP PE,nnnn */
      if( F & FLAG_P ) {
	JP();
      } else {
	contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
      }
      break;
    case 0xeb:		/* EX DE,HL */
      {
	libspectrum_word wordtemp=DE; DE=HL; HL=wordtemp;
      }
      break;
    case 0xec:		/* CALL PE,nnnn */
      if( F & FLAG_P ) {
	CALL();
      } else {
	contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
      }
      break;
    case 0xed:		/* shift ED */
      {
	libspectrum_byte opcode2;
	contend_read( PC, 4 );
	opcode2 = fetch_cmd_byte(st);
#ifdef HAVE_ENOUGH_MEMORY
	switch(opcode2) {
#include "z80_ed.c"
	}
#else			/* #ifdef HAVE_ENOUGH_MEMORY */
	if( z80_edxx(st, opcode2) ) goto end_opcode;
#endif			/* #ifdef HAVE_ENOUGH_MEMORY */
      }
      break;
    case 0xee:		/* XOR A,nn */
      {
	libspectrum_byte bytetemp = readbyte( PC++ );
	XOR(bytetemp);
      }
      break;
    case 0xef:		/* RST 28 */
      contend_read_no_mreq( IR, 1 );
      RST(0x28);
      break;
    case 0xf0:		/* RET P */
      contend_read_no_mreq( IR, 1 );
      if( ! ( F & FLAG_S ) ) { RET(); }
      break;
    case 0xf1:		/* POP AF */
      POP16(F,A);
      break;
    case 0xf2:		/* JP P,nnnn */
      if( ! ( F & FLAG_S ) ) {
	JP();
      } else {
	contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
      }
      break;
    case 0xf3:		/* DI */
      IFF1=IFF2=0;
      break;
    case 0xf4:		/* CALL P,nnnn */
      if( ! ( F & FLAG_S ) ) {
	CALL();
      } else {
	contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
      }
      break;
    case 0xf5:		/* PUSH AF */
      contend_read_no_mreq( IR, 1 );
      PUSH16(F,A);
      break;
    case 0xf6:		/* OR nn */
      {
	libspectrum_byte bytetemp = readbyte( PC++ );
	OR(bytetemp);
      }
      break;
    case 0xf7:		/* RST 30 */
      contend_read_no_mreq( IR, 1 );
      RST(0x30);
      break;
    case 0xf8:		/* RET M */
      contend_read_no_mreq( IR, 1 );
      if( F & FLAG_S ) { RET(); }
      break;
    case 0xf9:		/* LD SP,HL */
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      SP = HL;
      break;
    case 0xfa:		/* JP M,nnnn */
      if( F & FLAG_S ) {
	JP();
      } else {
	contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
      }
      break;
    case 0xfb:		/* EI */
      /* Interrupts are not accepted immediately after an EI, but are
	 accepted after the next instruction */
      IFF1 = IFF2 = 1;
      break;
    case 0xfc:		/* CALL M,nnnn */
      if( F & FLAG_S ) {
	CALL();
      } else {
	contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
      }
      break;
    case 0xfd:		/* shift FD */
      {
	libspectrum_byte opcode2;
	contend_read( PC, 4 );
	opcode2 = fetch_cmd_byte(st);
#ifdef HAVE_ENOUGH_MEMORY
	switch(opcode2) {
#define REGISTER  IY
#define REGISTERL IYL
#define REGISTERH IYH
#include "z80_ddfd.c"
#undef REGISTERH
#undef REGISTERL
#undef REGISTER
	}
#else			/* #ifdef HAVE_ENOUGH_MEMORY */
	if( z80_fdxx(st, opcode2) ) goto end_opcode;
#endif			/* #ifdef HAVE_ENOUGH_MEMORY */
      }
      break;
    case 0xfe:		/* CP nn */
      {
	libspectrum_byte bytetemp = readbyte( PC++ );
	CP(bytetemp);
      }
      break;
    case 0xff:		/* RST 38 */
      contend_read_no_mreq( IR, 1 );
      RST(0x38);
      break;
