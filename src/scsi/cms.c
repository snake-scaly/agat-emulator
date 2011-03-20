/*
	Agat Emulator version 1.18
	Copyright (c) NOP, nnop@newmail.ru
	SCSI CMS Card emulation module
*/

#include "types.h"
#include "memory.h"
#include "streams.h"
#include "debug.h"
#include "sysconf.h"
#include "runmgr.h"
#include "runmgrint.h"

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "syslib.h"

#include "localize.h"

#define MAX_DEVICES	7
#define MAX_CMD_LEN	32
#define MAX_RES_LEN	1024

#define CLEAR_BIT(d,b) ((d)&=~(b))
#define SET_BIT(d,b) ((d)|=(b))
#define IS_SET(d,b) ((d)&(b))


struct CMS_STATE
{
	struct SLOT_RUN_STATE*st;
	struct SYS_RUN_STATE*sr;
	byte	ram[128];
	byte	rom[4][2048];
	int	rom_page;
	int	rom_enabled;
	byte	regs[8];
	byte	reg_odr;

	byte	devices; // bit mask
	int	dev_selected; // selected device number or (-1)

	int	cmd_index, cmd_len;
	byte	cmd_data[MAX_CMD_LEN];
	int	res_index, res_len;
	byte	res_data[MAX_RES_LEN];
	int	recv_index, recv_len;
	byte	res_cmd, res_msg;
	int	cmd_finished;
	int	dma_index, dma_len;
};

enum {
	REG_ODR,
	REG_CSD = REG_ODR,
	REG_ICR,
	REG_MR2,
	REG_TCR,
	REG_SER,
	REG_CSB = REG_SER,
	REG_SDS,
	REG_BSR = REG_SDS,
	REG_SDT,
	REG_IDR = REG_SDT,
	REG_SDI,
	REG_RPI = REG_SDI
};

#define BIT_ICR_RST	0x80
#define BIT_ICR_TEST	0x40
#define BIT_ICR_AIP	0x40
#define BIT_ICR_LADIFF	0x20
#define BIT_ICR_ACK	0x10
#define BIT_ICR_BSY	0x08
#define BIT_ICR_SEL	0x04
#define BIT_ICR_ATN	0x02
#define BIT_ICR_DBUS	0x01

#define BIT_MR2_BLK	0x80
#define BIT_MR2_TARG	0x40
#define BIT_MR2_PCHK	0x20
#define BIT_MR2_PINT	0x10
#define BIT_MR2_EOP	0x08
#define BIT_MR2_BSY	0x04
#define BIT_MR2_DMA	0x02
#define BIT_MR2_ARB	0x01


#define BIT_TCR_REQ	0x08
#define BIT_TCR_MSG	0x04
#define BIT_TCR_CD	0x02
#define BIT_TCR_IO	0x01

#define BIT_CSB_RST	0x80
#define BIT_CSB_BSY	0x40
#define BIT_CSB_REQ	0x20
#define BIT_CSB_MSG	0x10
#define BIT_CSB_CD	0x08
#define BIT_CSB_IO	0x04
#define BIT_CSB_SEL	0x02
#define BIT_CSB_DBP	0x01

#define BIT_BSR_EDMA	0x80
#define BIT_BSR_DRQ	0x40
#define BIT_BSR_SPER	0x20
#define BIT_BSR_INT	0x10
#define BIT_BSR_PHSM	0x08
#define BIT_BSR_BSY	0x04
#define BIT_BSR_ATN	0x02
#define BIT_BSR_ACK	0x01





static int cms_term(struct SLOT_RUN_STATE*st)
{
	free(st->data);
	return 0;
}

static int cms_save(struct SLOT_RUN_STATE*st, OSTREAM*out)
{
	struct CMS_STATE*cms = st->data;
	return 0;
}

static int cms_load(struct SLOT_RUN_STATE*st, ISTREAM*in)
{
	struct CMS_STATE*cms = st->data;

	return 0;
}

static int cms_command(struct SLOT_RUN_STATE*st, int cmd, int data, long param)
{
	struct CMS_STATE*mcs = st->data;
	switch (cmd) {
	case SYS_COMMAND_RESET:
		return 0;
	case SYS_COMMAND_HRESET:
		return 0;
	}
	return 0;
}

static void cms_xrom_enable(struct CMS_STATE*cms, int en)
{
	if (en == cms->rom_enabled) return;
	enable_slot_xio(cms->st, en);
	cms->rom_enabled = en;
	printf("cms: xrom %sabled\n", en?"en":"dis");
}

static byte cms_rom_r(word adr, struct CMS_STATE*cms) // CX00-CXFF
{
//	printf("cms: rom[%04X] = %02X\n", adr, cms->rom[3][adr&0x7FF]);
	cms_xrom_enable(cms, 1);
//	system_command(cms->st->sr, SYS_COMMAND_DUMPCPUREGS, 0, 0);
	return cms->rom[3][adr&0x7FF];
}

static byte cms_xrom_r(word adr, struct CMS_STATE*cms) // C800-CFFF
{
	adr &= 0x7FF;
	if (adr < 0x80) {
//		printf("cms: ram[%04X] -> %02X\n", adr, cms->ram[adr]);
		return cms->ram[adr];
	} else if (adr == 0x7FF || cms->rom_page == -1) {
		cms_xrom_enable(cms, 0);
	}
//	system_command(cms->st->sr, SYS_COMMAND_DUMPCPUREGS, 0, 0);
//	printf("cms: rom%i[%04X] = %02X\n", cms->rom_page, adr|0xC800, cms->rom[cms->rom_page][adr]);
	return cms->rom[cms->rom_page][adr];
}

static void cms_xrom_w(word adr, byte data, struct CMS_STATE*cms) // C800-C87F
{
	adr &= 0x7FF;
	if (adr < 0x80) {
//		printf("cms: ram[%04X] <- %02X\n", adr, data);
		cms->ram[adr] = data;
	}
}

static void select_rom_bank(struct CMS_STATE*cms, int no)
{
	cms_xrom_enable(cms, 1);
	cms->rom_page = no;
}

static void cms_rst(struct CMS_STATE*cms, int reset) // RST signal
{
	if (reset) {
		puts("cms: reset registers");
		cms->regs[REG_ICR] = IS_SET(cms->regs[REG_ICR], BIT_ICR_RST);
		cms->regs[REG_MR2] = IS_SET(cms->regs[REG_MR2], BIT_MR2_TARG);
		cms->regs[REG_CSB] = BIT_CSB_RST;
		cms->dev_selected = -1;
	} else {
		CLEAR_BIT(cms->regs[REG_CSB], BIT_CSB_RST);
	}
}

static void cms_arb(struct CMS_STATE*cms, int arb, byte mask) // ARB signal
{
	if (arb) {
		extern int cpu_debug;
//		cpu_debug = 1;
		printf("cms: arbiter %X\n", mask);
		cms->regs[REG_ICR] = IS_SET(cms->regs[REG_ICR], BIT_ICR_RST);
		cms->regs[REG_MR2] = IS_SET(cms->regs[REG_MR2], BIT_MR2_TARG);
		cms->regs[REG_CSB] = 0;
	}
}


static int get_cmd_len(struct CMS_STATE*cms, byte cmd)
{
	int cllens[4] = { 6, 10, 8, 6 };
	return cllens[(cmd>>5)&3];
}

static int scsi_read_block(struct CMS_STATE*cms, unsigned long lba, byte*data, int len)
{
	FILE*in;
	in = fopen("scsi.dsk", "rb");
	if (!in) return 3;
	printf("scsi: read block with LBA=%i, size=%i\n", lba, len);
	fseek(in, lba * 512, SEEK_SET);
	fread(data, 1, len, in);
	fclose(in);
//	memset(data, 0x00, len);
//	data[0] = 1;
	return 0;
}


static int scsi_write_block(struct CMS_STATE*cms, unsigned long lba, const byte*data, int len)
{
	FILE*in;
	printf("scsi: write block with LBA=%i, size=%i\n", lba, len);
	in = fopen("scsi.dsk", "r+b");
	if (!in) return 3;
	fseek(in, lba * 512, SEEK_SET);
	fwrite(data, 1, len, in);
	fclose(in);
	return 0;
}

static int finish_command(struct CMS_STATE*cms, byte*packet, int len, byte*data, int dlen)
{
	int res = 0;
	int i;
	printf("SCSI DATA: {");
	for (i = 0; i < dlen; ++i) {
		printf("%02X ", data[i]);
	}
	printf("}\n");
	switch (packet[0]) {
	case 0x0A: // write
		res = scsi_write_block(cms, packet[3] | (packet[2]<<8) | ((packet[1]&0x1F) << 16), data, dlen);
		break;
	}
	return res;
}

static int process_command(struct CMS_STATE*cms, byte*packet, int len)
{
	int res = 0;
	int i;
	printf("SCSI COMMAND: {");
	for (i = 0; i < len; ++i) {
		printf("%02X ", packet[i]);
	}
	printf("}\n");
	cms->cmd_index = -1;
	cms->res_index = 0;
	cms->recv_index = -1;
	switch (packet[0]) {
	case 0x00: // test unit ready
		cms->res_len = 0;
		break;
	case 0x03: // request sense
		cms->res_len = packet[4];
		memset(cms->res_data, 0, cms->res_len);
		cms->res_data[2] = 1; // or 6
		break;
	case 0x08: // read
		cms->res_len = packet[4] * 0x200;
		res = scsi_read_block(cms, packet[3] | (packet[2]<<8) | ((packet[1]&0x1F) << 16), cms->res_data, cms->res_len);
		break;
	case 0x0A: // write
		cms->res_len = 0;
		cms->recv_len = packet[4] * 0x200;
		cms->recv_index = 0;
		break;
	case 0x12: // inqury
		cms->res_len = packet[4];
		memset(cms->res_data, 0, cms->res_len);
		break;
	case 0x25: // capacity
		cms->res_len = 8;
		memset(cms->res_data, 0, cms->res_len);
		cms->res_data[0] = 0x12;
		cms->res_data[1] = 0x34;
		cms->res_data[2] = 0x56;
		cms->res_data[3] = 0x78;

		cms->res_data[4] = 0x21;
		cms->res_data[5] = 0x43;
		cms->res_data[6] = 0x65;
		cms->res_data[7] = 0x87;
		break;
	default:
		cms->res_len = 0;
		cms->recv_len = 0;
		return 0x40; // task aborted
	}
	return res;

}

static void output_command_byte(struct CMS_STATE*cms, byte data)
{
	if (cms->dev_selected == -1) return;
	if (cms->cmd_index == -1) return;
	if (!cms->cmd_index && !cms->cmd_len) {
		cms->cmd_len = get_cmd_len(cms, data);
		printf("SCSI[%i] command length(%02X) = %i bytes\n", cms->dev_selected, data, cms->cmd_len);
	}
	if (cms->cmd_index < cms->cmd_len) {
		cms->cmd_data[cms->cmd_index] = data;
		++cms->cmd_index;
	}
	if (cms->cmd_index == cms->cmd_len) {
		cms->res_cmd = process_command(cms, cms->cmd_data, cms->cmd_len);
	}
}


static byte input_response_byte(struct CMS_STATE*cms)
{
	byte res = 0;
	byte tcr = cms->regs[REG_TCR];

	if (cms->dev_selected == -1) return 0xFF;
	if (!IS_SET(tcr, BIT_TCR_IO)) return 0xFF;
	if (IS_SET(tcr, BIT_TCR_CD) && !IS_SET(tcr, BIT_TCR_MSG)) {
		return cms->res_cmd;
	} else if (IS_SET(tcr, BIT_TCR_CD) && IS_SET(tcr, BIT_TCR_MSG)) {
		cms->cmd_finished = 1;
		return cms->res_msg;
	}

	if (cms->res_index == -1) return 0xFF;
	if (cms->res_index < cms->res_len) {
		res = cms->res_data[cms->res_index];
	}
	if (cms->res_index <= cms->res_len) {
		++ cms->res_index;
	} else {
		cms->res_index = -1;
	}
	printf("cms: input_response_byte = %02X\n", res);
	return res;
}

static void cms_sel(struct CMS_STATE*cms, int sel, byte mask) // SEL signal
{
	if (sel) {
		printf("cms: select %X\n", mask);
		if (mask & cms->devices & 0x7F) {
			if (!IS_SET(cms->regs[REG_ICR], BIT_ICR_BSY)) {
				extern int cpu_debug;
				int i;
//				cpu_debug = 1;
				cms->dev_selected = -1;
				for (i = 0; i < MAX_DEVICES; ++i, mask >>= 1) {
					if (mask & 0x01) {
						cms->dev_selected = i;
					}
				}
				if (cms->dev_selected != -1)
					SET_BIT(cms->regs[REG_CSB], BIT_CSB_BSY);
				printf("cms: selected device %X (%i)\n", mask, cms->dev_selected);
			}	
		}	
	}
}


static void cms_ack(struct CMS_STATE*cms, int ack) // ACK signal
{
	if (cms->dev_selected == -1) return;
	if (ack) {
		if (!IS_SET(cms->regs[REG_TCR], BIT_TCR_IO)) {
			printf("cms: acknowledge: %s %s = %02X\n",
				IS_SET(cms->regs[REG_TCR], BIT_TCR_IO)?"read":"write",
				IS_SET(cms->regs[REG_TCR], BIT_TCR_CD)?"command":"data",
				cms->regs[REG_ODR]);
		}
		printf("cms: set request inactive\n");
		CLEAR_BIT(cms->regs[REG_CSB], BIT_CSB_REQ);
		if (cms->cmd_finished) CLEAR_BIT(cms->regs[REG_CSB], BIT_CSB_BSY);
	} else {
		if (!cms->cmd_finished) SET_BIT(cms->regs[REG_CSB], BIT_CSB_REQ);
	}
}

static void cms_dma(struct CMS_STATE*cms, int dma) // DMA signal
{
	if (cms->dev_selected == -1) return;
	if (dma) {
		puts("cms: set DMA flag");
		SET_BIT(cms->regs[REG_BSR], BIT_BSR_DRQ);
	} else {
		CLEAR_BIT(cms->regs[REG_BSR], BIT_BSR_DRQ);
	}
}



static void cms_tcr(struct CMS_STATE*cms, byte data)
{
	if (cms->dev_selected == -1) return;
	SET_BIT(cms->regs[REG_BSR], BIT_BSR_PHSM);
	if (IS_SET(data, BIT_TCR_CD) && !IS_SET(data, BIT_TCR_IO)) {
		puts("cms: preparing to receive command");
		cms->cmd_index = 0;
		cms->cmd_len = 0;
		cms->cmd_finished = 0;
		SET_BIT(cms->regs[REG_CSB], BIT_CSB_REQ);
	} else if (IS_SET(data, BIT_TCR_CD) && IS_SET(data, BIT_TCR_IO) && !IS_SET(data, BIT_TCR_MSG)) {
		puts("cms: preparing to send command response");
		SET_BIT(cms->regs[REG_CSB], BIT_CSB_REQ);
	} else if (IS_SET(data, BIT_TCR_CD) && IS_SET(data, BIT_TCR_IO) && IS_SET(data, BIT_TCR_MSG)) {
		puts("cms: preparing to send message response");
		SET_BIT(cms->regs[REG_CSB], BIT_CSB_REQ);
	}
}


static void dma_receive(struct CMS_STATE*cms)
{
	SET_BIT(cms->regs[REG_BSR], BIT_BSR_EDMA);
	printf("cms: starting DMA receive: length = %i, index = %i\n", cms->res_len, cms->res_index);
	cms->dma_len = cms->res_len;
	cms->dma_index = cms->res_index;
	cms->res_index = - 1;
}

static void dma_send(struct CMS_STATE*cms)
{
	SET_BIT(cms->regs[REG_BSR], BIT_BSR_EDMA);
	printf("cms: starting DMA send: length = %i, index = %i\n", 
		cms->recv_len, cms->recv_index);
	cms->dma_len = cms->recv_len;
	cms->dma_index = cms->recv_index;
	cms->recv_index = - 1;
}



static byte input_dma_byte(struct CMS_STATE*cms)
{
	byte res = 0;
	if (cms->dev_selected == -1) return 0xFF;
	if (cms->dma_index == -1) return 0xFF;
	if (cms->dma_index < cms->dma_len) {
		res = cms->res_data[cms->dma_index];
		++ cms->dma_index;
	} else {
		cms->dma_index = -1;
	}
	return res;
}


static void output_data_byte(struct CMS_STATE*cms, byte data)
{
	if (cms->dev_selected == -1) return;
	if (cms->recv_index == -1) return;
	if (cms->recv_index < cms->recv_len) {
		cms->res_data[cms->recv_index] = data;
		++cms->recv_index;
	}
	if (cms->recv_index == cms->recv_len) {
		cms->res_cmd = finish_command(cms, cms->cmd_data, cms->cmd_len, cms->res_data, cms->recv_len);
	}
}

static void output_dma_byte(struct CMS_STATE*cms, byte data)
{
	if (cms->dev_selected == -1) return;
	if (cms->dma_index == -1) return;
	if (cms->dma_index < cms->dma_len) {
		cms->res_data[cms->dma_index] = data;
		++cms->dma_index;
	}
	if (cms->dma_index == cms->dma_len) {
		cms->res_cmd = finish_command(cms, cms->cmd_data, cms->cmd_len, cms->res_data, cms->dma_len);
	}
}



static const char*rrnames[8] = { "CSD", "ICR", "MR2", "TCR", "CSB", "BSR", "IDR", "RPI" };
static const char*rwnames[8] = { "ODR", "ICR", "MR2", "TCR", "SER", "SDS", "SDT", "SDI" };
static const char*bwnames[8][8] = {
	{ "DB7", "DB6", "DB5", "DB4", "DB3", "DB2", "DB1", "DB0" }, // 0, ODR
	{ "RST", "TEST", "DIFF", "ACK", "BSY", "SEL", "ATN", "DBUS" }, // 1, ICR
	{ "BLK", "TARG", "PCHK", "PINT", "EOP", "BSY", "DMA", "ARB" }, // 2, MR2
	{ "x7", "x6", "x5", "x4", "REQ", "MSG", "C/D", "I/O" }, // 3, TCR
	{ "DB7", "DB6", "DB5", "DB4", "DB3", "DB2", "DB1", "DB0" }, // 4, SER
	{ "x7", "x6", "x5", "x4", "x3", "x2", "x1", "x0" }, // 5, SDS
	{ "x7", "x6", "x5", "x4", "x3", "x2", "x1", "x0" }, // 6, SDT
	{ "x7", "x6", "x5", "x4", "x3", "x2", "x1", "x0" }, // 7, SDI
};

static const char*brnames[8][8] = {
	{ "DB7", "DB6", "DB5", "DB4", "DB3", "DB2", "DB1", "DB0" }, // 0, CSD
	{ "RST", "AIP", "LA", "ACK", "BSY", "SEL", "ATN", "DBUS" }, // 1, ICR
	{ "BLK", "TARG", "PCHK", "PINT", "EOP", "BSY", "DMA", "ARB" }, // 2, MR2
	{ "x7", "x6", "x5", "x4", "REQ", "MSG", "C/D", "I/O" }, // 3, TCR
	{ "RST", "BSY", "REQ", "MSG", "C/D", "I/O", "SEL", "DBP" }, // 4, CSB
	{ "EDMA", "DRQ", "SPER", "INT", "PHSM", "BSY", "ATN", "ACK" }, // 5, BSR
	{ "x7", "x6", "x5", "x4", "x3", "x2", "x1", "x0" }, // 6, IDR
	{ "x7", "x6", "x5", "x4", "x3", "x2", "x1", "x0" }, // 7, RPI
};

static void cms_io_w(word adr, byte data, struct CMS_STATE*cms) // C0X0-C0XF
{
	printf("cms: write io[%04X] <= %02X\n", adr, data);
	if (adr&0x08) { // switches
		switch (adr & 0x0F) {
		case 9: select_rom_bank(cms, data & 3); break;
		case 10: output_dma_byte(cms, data); break;
		}
	} else {
		int i, d = data;
//		printf("cms: REGISTER: %s (%i)\n", rnames[adr&7], adr&7);
		printf("cms: %s WRITE FLAGS(%02X): {", rwnames[adr&7], data);
		for (i = 0; i < 8; ++i, d<<=1) {
			printf("%s:%i ", bwnames[adr&7][i], (d&0x80)?1:0);
		}
		printf("}\n");
		switch (adr&7) {
		case REG_ODR:
			cms->reg_odr = data;
			if (cms->cmd_index != -1) output_command_byte(cms, data);
			else if (cms->recv_index != -1) output_data_byte(cms, data);
			break;
		case REG_MR2:
			cms_arb(cms, IS_SET(data, BIT_MR2_ARB), cms->reg_odr);
			cms_dma(cms, IS_SET(data, BIT_MR2_DMA));
		case REG_SER:
			cms->regs[adr&7] = data;
			break;
		case REG_TCR:
			cms->regs[adr&7] = data;
			cms_tcr(cms, data);
			break;
		case REG_ICR:
			cms->regs[adr&7] = data;
			cms_ack(cms, IS_SET(data, BIT_ICR_ACK));
			cms_sel(cms, IS_SET(data, BIT_ICR_SEL), cms->reg_odr);
			cms_rst(cms, IS_SET(data, BIT_ICR_RST));
			break;
		case REG_SDS: // start DMA send
			dma_send(cms);
			break;
		case REG_SDI: // start DMA receive
			dma_receive(cms);
			break;
		}
//		system_command(cms->st->sr, SYS_COMMAND_DUMPCPUREGS, 0, 0);
	}
}

static byte cms_io_r(word adr, struct CMS_STATE*cms) // C0X0-C0XF
{
	byte res = 0;
	if (adr&0x08) { // switches
		switch (adr&0x0F) {
		case 8: res = 1; break; // exec delay
		case 10: res = input_dma_byte(cms); break;
		case 12: res = 1; break; // ready timeout
		case 14: res = 0; break; // delay?
		case 15: res = 0; break;// delay?
		}
	} else { // registers
		int i, d;
		res = cms->regs[adr&7];
		switch (adr&7) {
		case REG_CSD:
			res = input_response_byte(cms);
			break;
		case REG_ICR:
			res &= BIT_ICR_DBUS | BIT_ICR_ATN | BIT_ICR_SEL | BIT_ICR_BSY | BIT_ICR_ACK | BIT_ICR_RST;
			if (IS_SET(cms->regs[REG_MR2], BIT_MR2_ARB)) res |= BIT_ICR_AIP;
			break;
		case REG_CSB:
			if (IS_SET(cms->regs[REG_ICR], BIT_ICR_RST)) SET_BIT(res, BIT_CSB_RST);
			else CLEAR_BIT(res, BIT_CSB_RST);
			break;
		case REG_BSR:
			break;
		}
		printf("cms: %s READ FLAGS(%02X): {", rrnames[adr&7], res);
		for (i = 0, d = res; i < 8; ++i, d<<=1) {
			printf("%s:%i ", brnames[adr&7][i], (d&0x80)?1:0);
		}
		printf("}\n");
		system_command(cms->st->sr, SYS_COMMAND_DUMPCPUREGS, 0, 0);
	}
	printf("cms: read io[%04X] => %02X\n", adr, res);
	return res;
}


int  cms_init(struct SYS_RUN_STATE*sr, struct SLOT_RUN_STATE*st, struct SLOTCONFIG*cf)
{
	ISTREAM*rom;
	struct CMS_STATE*cms;

	puts("in cms_init");

	cms = calloc(1, sizeof(*cms));
	if (!cms) return -1;

	cms->st = st;
	cms->sr = sr;

	cms->devices = 0x40;
	cms->dev_selected = -1;
	cms->cmd_index = -1;
	cms->res_index = -1;
	cms->dma_index = -1;
	cms->recv_index = -1;


	st->data = cms;
	st->free = cms_term;
	st->command = cms_command;
	st->load = cms_load;
	st->save = cms_save;

	rom = isfopen(cf->cfgstr[CFG_STR_ROM]);
	if (!rom) { free(cms); return -1; }
	isread(rom, cms->rom, sizeof(cms->rom));
	isclose(rom);

	fill_rw_proc(st->io_sel, 1, cms_rom_r, empty_write, cms);
	fill_rw_proc(&st->xio_sel, 1, cms_xrom_r, cms_xrom_w, cms);
	fill_rw_proc(st->baseio_sel, 1, cms_io_r, cms_io_w, cms);

	return 0;
}
