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
#define MAX_RES_LEN	8192

#define CLEAR_BIT(d,b) ((d)&=~(b))
#define SET_BIT(d,b) ((d)|=(b))
#define IS_SET(d,b) ((d)&(b))


enum {
	PHASE_IDLE,
	PHASE_RESET,
	PHASE_ARBITER,
	PHASE_SELECT,
	PHASE_WAIT, // device is waiting for command
	PHASE_COMMAND, // device is receiving command
	PHASE_DATA_SEND, // device is waiting for data
	PHASE_DATA_RECV, // device is ready to transfer data
	PHASE_STATUS, // device is ready to transfer status
	PHASE_MESSAGE, // device is ready to transfer message
	PHASE_DMA_SEND, // dma transfer from host to device in progress
	PHASE_DMA_RECV, // dma transfer from device to host in progress

	N_PHASES
};

static const char*phnames[N_PHASES] = {
	"IDLE", "RESET", "ARBITER", "SELECT", "WAIT", "COMMAND",
	"DATA_SEND", "DATA_RECV", "STATUS", "MESSAGE", "DMA_SEND",
	"DMA_RECV"
};

struct DSK_INFO
{
	int	no;
	byte	mask;
	unsigned nblk;
	char	name[256];
};

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
	struct DSK_INFO disks[MAX_DEVICES];
	FILE	*img;

	int	dev_selected; // selected device number or (-1)


	int	phase;

	int	cmd_index, cmd_len;
	byte	cmd_data[MAX_CMD_LEN];
	int	res_index, res_len;
	byte	res_data[MAX_RES_LEN];
	int	recv_index, recv_len;
	byte	res_cmd, res_msg;
	int	dma_index, dma_len;

};

enum {
	_REG_ODR,
	REG_CSD = _REG_ODR,
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


static void output_byte(struct CMS_STATE*cms, byte data);
static byte input_response_byte(struct CMS_STATE*cms);
static void cms_phase(struct CMS_STATE*cms, int phase);


static int open_device(struct CMS_STATE*cms, int dev);
static void close_device(struct CMS_STATE*cms);

static int cms_term(struct SLOT_RUN_STATE*st)
{
	struct CMS_STATE*cms = st->data;
	if (cms->img) fclose(cms->img);
	free(st->data);
	return 0;
}

static int cms_save(struct SLOT_RUN_STATE*st, OSTREAM*out)
{
	struct CMS_STATE*cms = st->data;
	WRITE_ARRAY(out, cms->ram);
	WRITE_ARRAY(out, cms->regs);
	WRITE_FIELD(out, cms->reg_odr);

	WRITE_FIELD(out, cms->rom_page);
	WRITE_FIELD(out, cms->rom_enabled);
	WRITE_FIELD(out, cms->dev_selected);
	WRITE_FIELD(out, cms->phase);
	return 0;
}

static int cms_load(struct SLOT_RUN_STATE*st, ISTREAM*in)
{
	struct CMS_STATE*cms = st->data;
	int phase, dev;
	READ_ARRAY(in, cms->ram);
	READ_ARRAY(in, cms->regs);
	READ_FIELD(in, cms->reg_odr);

	READ_FIELD(in, cms->rom_page);
	READ_FIELD(in, cms->rom_enabled);
	READ_FIELD(in, dev);
	READ_FIELD(in, phase);
	cms->phase = -1;
	cms_phase(cms, phase);
	if (cms->rom_enabled) enable_slot_xio(cms->st, 1);
	open_device(cms, dev);
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
	cms->rom_page = no;
}


static void cms_phase(struct CMS_STATE*cms, int phase)
{
	if (cms->phase == phase) return;
	cms->phase = phase;
	printf("PHASE: %s\n", phnames[phase]);
	switch (phase) {
	case PHASE_RESET:
		cms->regs[REG_CSB] = BIT_CSB_RST;
		cms->regs[REG_BSR] = 0;
		cms->regs[REG_ICR] = IS_SET(cms->regs[REG_ICR], BIT_ICR_RST);
		cms->regs[REG_MR2] = IS_SET(cms->regs[REG_MR2], BIT_MR2_TARG);
		break;
	case PHASE_IDLE:
		cms->regs[REG_CSB] = 0;
		cms->regs[REG_BSR] = BIT_BSR_PHSM;
		break;
	case PHASE_ARBITER:
		cms->regs[REG_ICR] = IS_SET(cms->regs[REG_ICR], BIT_ICR_RST);
		cms->regs[REG_MR2] = IS_SET(cms->regs[REG_MR2], BIT_MR2_TARG);
		cms->regs[REG_CSB] = BIT_CSB_BSY;
		cms->dev_selected = -1;
		break;
	case PHASE_SELECT:
		cms->regs[REG_CSB] = BIT_CSB_SEL | BIT_CSB_BSY;
		SET_BIT(cms->regs[REG_BSR], BIT_BSR_PHSM);
		break;
	case PHASE_WAIT:
		cms->regs[REG_CSB] = 0;
		SET_BIT(cms->regs[REG_BSR], BIT_BSR_PHSM);
		cms->cmd_index = cms->recv_index = cms->res_index = cms->dma_index = -1;
		break;
	case PHASE_COMMAND:
		cms->regs[REG_CSB] = BIT_CSB_BSY | BIT_CSB_REQ | BIT_CSB_CD;
		SET_BIT(cms->regs[REG_BSR], BIT_BSR_PHSM);
		cms->cmd_index = 0;
		cms->cmd_len = 0;
		cms->recv_index = cms->res_index = cms->dma_index = -1;
		break;
	case PHASE_DATA_SEND:
		cms->regs[REG_CSB] = BIT_CSB_BSY | BIT_CSB_REQ;
		SET_BIT(cms->regs[REG_BSR], BIT_BSR_PHSM);
		cms->recv_index = 0;
		cms->cmd_index = cms->res_index = cms->dma_index = -1;
		break;
	case PHASE_DATA_RECV:
		cms->regs[REG_CSB] = BIT_CSB_BSY | BIT_CSB_IO | BIT_CSB_REQ;
		SET_BIT(cms->regs[REG_BSR], BIT_BSR_PHSM);
		cms->res_index = 0;
		cms->cmd_index = cms->recv_index = cms->dma_index = -1;
		cms->regs[REG_CSD] = input_response_byte(cms);
		break;
	case PHASE_STATUS:
		cms->regs[REG_CSB] = BIT_CSB_BSY | BIT_CSB_IO | BIT_CSB_CD | BIT_CSB_REQ;
		SET_BIT(cms->regs[REG_BSR], BIT_BSR_PHSM);
		cms->res_index = cms->cmd_index = cms->recv_index = cms->dma_index = -1;
		cms->regs[REG_CSD] = input_response_byte(cms);
		break;
	case PHASE_MESSAGE:
		cms->regs[REG_CSB] = BIT_CSB_BSY | BIT_CSB_IO | BIT_CSB_MSG | BIT_CSB_CD | BIT_CSB_REQ;
		SET_BIT(cms->regs[REG_BSR], BIT_BSR_PHSM);
		cms->res_index = cms->cmd_index = cms->recv_index = cms->dma_index = -1;
		cms->regs[REG_CSD] = input_response_byte(cms);
		break;
	case PHASE_DMA_SEND:
		cms->regs[REG_CSB] = BIT_CSB_BSY | BIT_CSB_REQ;
		SET_BIT(cms->regs[REG_BSR], BIT_BSR_PHSM | BIT_BSR_DRQ | BIT_BSR_EDMA);
		cms->dma_index = cms->recv_index;
		cms->dma_len = cms->recv_len;
		cms->res_index = cms->cmd_index = cms->recv_index = -1;
		break;
	case PHASE_DMA_RECV:
		cms->regs[REG_CSB] = BIT_CSB_BSY | BIT_CSB_IO;
		SET_BIT(cms->regs[REG_BSR], BIT_BSR_PHSM | BIT_BSR_DRQ | BIT_BSR_EDMA);
		cms->dma_index = cms->res_index;
		cms->dma_len = cms->res_len;
		cms->res_index = cms->cmd_index = cms->recv_index = -1;
		break;
	}
}

static void cms_rst(struct CMS_STATE*cms, int reset) // RST signal
{
	printf("cms: set RST = %i\n", reset?1:0);
	if (reset) {
		extern int cpu_debug;
//		cpu_debug = 1;
		puts("cms: reset registers");
		cms->dev_selected = -1;
		cms->cmd_index = -1;
		cms->cmd_len = 0;
		cms->res_index = -1;
		cms->recv_index = -1;
		cms->dma_index = -1;
		cms_phase(cms, PHASE_RESET);
	} else {
		cms_phase(cms, PHASE_IDLE);
	}
}

static void cms_arb(struct CMS_STATE*cms, int arb, byte mask) // ARB signal
{
	printf("cms: set ARB = %i\n", arb?1:0);
	if (arb) {
		extern int cpu_debug;
//		cpu_debug = 1;
		printf("cms: arbiter %X\n", mask);
		cms->regs[REG_CSD] = mask;
		cms_phase(cms, PHASE_ARBITER);
	} else {
	}
}


static int get_cmd_len(struct CMS_STATE*cms, byte cmd)
{
	int cllens[4] = { 6, 10, 8, 6 };
	return cllens[(cmd>>5)&3];
}

static int open_device(struct CMS_STATE*cms, int dev)
{
	if (dev == cms->dev_selected) return 1;
	if (dev == -1) { close_device(cms); return 2; }
	if (cms->img) fclose(cms->img);
	cms->img = fopen(cms->disks[dev].name, "r+b");
	if (!cms->img) cms->img = fopen(cms->disks[dev].name, "w+b");
	if (!cms->img) {
		MessageBeep(MB_ICONEXCLAMATION);
		cms->dev_selected = -1;
		return -1;
	}
	cms->dev_selected = dev;
	return 0;
}

static void close_device(struct CMS_STATE*cms)
{
	cms->dev_selected = -1;
	if (cms->img) fclose(cms->img);
	cms->img = NULL;
}


static int scsi_read_block(struct CMS_STATE*cms, unsigned long lba, byte*data, int len)
{
	if (!cms->img) return 3;
	printf("scsi: read block with LBA=%i, size=%i\n", lba, len);
	fseek(cms->img, lba * 512, SEEK_SET);
	fread(data, 1, len, cms->img);
	return 0;
}


static int scsi_write_block(struct CMS_STATE*cms, unsigned long lba, const byte*data, int len)
{
	printf("scsi: write block with LBA=%i, size=%i\n", lba, len);
	if (!cms->img) return 3;
	fseek(cms->img, lba * 512, SEEK_SET);
	fwrite(data, 1, len, cms->img);
	return 0;
}

static int scsi_format(struct CMS_STATE*cms, unsigned nb)
{
	char buf[512];
	printf("scsi: formatting device\n");
	if (!cms->img) return 3;
	memset(buf, 0, sizeof(buf));
	for (; nb; --nb) {
		fwrite(buf, 1, sizeof(buf), cms->img);
	}
	return 0;
}


static int finish_command(struct CMS_STATE*cms, byte*packet, int len, byte*data, int dlen)
{
	int res = 0;
	extern int cpu_debug;
/*	int i;
	printf("SCSI DATA: {");
	for (i = 0; i < dlen; ++i) {
		printf("%02X ", data[i]);
	}
	printf("}\n");*/
	switch (packet[0]) {
	case 0x0A: // write
		res = scsi_write_block(cms, packet[3] | (packet[2]<<8) | ((packet[1]&0x1F) << 16), data, dlen);
		break;
	case 0x15: // mode select
//		cpu_debug = 1;
		break;
	}
	return res;
}

static void process_command(struct CMS_STATE*cms, byte*packet, int len)
{
	int res = 0;
	unsigned nb;
	int phase = PHASE_STATUS;
	if (cms->dev_selected == -1) {
		cms_phase(cms, PHASE_IDLE);
		return;
	}
/*	int i;
	printf("SCSI COMMAND: {");
	for (i = 0; i < len; ++i) {
		printf("%02X ", packet[i]);
	}
	printf("}\n");*/
	cms->res_len = 0;
	cms->recv_len = 0;
	switch (packet[0]) {
	case 0x00: // test unit ready
		break;
	case 0x03: // request sense
		cms->res_len = packet[4];
		memset(cms->res_data, 0, cms->res_len);
		cms->res_data[2] = 1; // or 6
		break;
	case 0x04: // format drive
		res = scsi_format(cms, cms->disks[cms->dev_selected].nblk);
		break;
	case 0x08: // read
		cms->res_len = packet[4] * 0x200;
		if (cms->res_len > MAX_RES_LEN) res = 0x22;
		else res = scsi_read_block(cms, packet[3] | (packet[2]<<8) | ((packet[1]&0x1F) << 16), cms->res_data, cms->res_len);
		break;
	case 0x0A: // write
		cms->recv_len = packet[4] * 0x200;
		if (cms->recv_len > MAX_RES_LEN) {
			res = 0x22;
			cms->recv_len = 0;
		}
		break;
	case 0x12: // inqury
		cms->res_len = packet[4];
		memset(cms->res_data, 0, cms->res_len);
		break;
	case 0x15: // mode select
		cms->recv_len = packet[4];
		break;
	case 0x1A: // mode sense
		cms->res_len = packet[4];
		memset(cms->res_data, 0, cms->res_len);
		break;
	case 0x25: // capacity
		cms->res_len = 8;
		memset(cms->res_data, 0, cms->res_len);
		nb = cms->disks[cms->dev_selected].nblk;
		cms->res_data[0] = (nb>>24)&0xFF;
		cms->res_data[1] = (nb>>16)&0xFF;
		cms->res_data[2] = (nb>>8)&0xFF;
		cms->res_data[3] = nb&0xFF;

		cms->res_data[4] = 0x00;
		cms->res_data[5] = 0x00;
		cms->res_data[6] = 0x02;
		cms->res_data[7] = 0x00;
		break;
	default:
		cms->res_len = 0;
		cms->recv_len = 0;
		res = 0x40; // task aborted
	}
	cms->res_cmd = res;
	if (cms->recv_len)
		phase = PHASE_DATA_SEND;
	else if (cms->res_len)
		phase = PHASE_DATA_RECV;
	cms_phase(cms, phase);

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
		process_command(cms, cms->cmd_data, cms->cmd_len);
	}
}



static void select_device(struct CMS_STATE*cms, byte mask)
{
	printf("cms: selecting device %02X\n", mask);
	if (mask & cms->devices & 0x7F) {
		extern int cpu_debug;
		int i, dev = -1;
//		cpu_debug = 1;
		for (i = 0; i < MAX_DEVICES; ++i) {
			if (cms->disks[i].mask & mask) {
				dev = i;
			}
		}
		printf("cms: selected device %i\n", dev);
		open_device(cms, dev);
	}	
}

static void cms_sel(struct CMS_STATE*cms, int sel) // SEL signal
{
	printf("cms: set SEL = %i\n", sel?1:0);
	if (sel) {
		cms_phase(cms, PHASE_SELECT);
		select_device(cms, cms->reg_odr);
		if (IS_SET(cms->regs[REG_ICR], BIT_ICR_BSY))
			SET_BIT(cms->regs[REG_CSB], BIT_CSB_BSY);
		else {
			if (cms->dev_selected != -1) { // acknowledge selection
				SET_BIT(cms->regs[REG_CSB], BIT_CSB_BSY);
			} else {
				CLEAR_BIT(cms->regs[REG_CSB], BIT_CSB_BSY);
			}
		};
	} else {
		cms_phase(cms, (cms->dev_selected==-1)?PHASE_IDLE:PHASE_COMMAND);
	}
}


static void cms_bsy(struct CMS_STATE*cms, int bsy) // ICR BSY signal
{
	printf("cms: set BSY = %i\n", bsy?1:0);
	if (cms->phase == PHASE_SELECT) {
		if (bsy) {
			select_device(cms, cms->reg_odr);
			SET_BIT(cms->regs[REG_CSB], BIT_CSB_BSY);
		} else {
			if (cms->dev_selected != -1) { // acknowledge selection
				SET_BIT(cms->regs[REG_CSB], BIT_CSB_BSY);
			} else {
				CLEAR_BIT(cms->regs[REG_CSB], BIT_CSB_BSY);
			}
		}
	} else {
	}
}

static byte input_response_byte(struct CMS_STATE*cms)
{
	byte res = cms->regs[REG_CSD];

	if (cms->dev_selected == -1) return res;
	switch (cms->phase) {
	case PHASE_STATUS:
		res = cms->res_cmd;
		break;
	case PHASE_MESSAGE:
		res = cms->res_msg;
		break;
	case PHASE_DATA_RECV:
		if (cms->res_index == -1) return res;
		if (cms->res_index < cms->res_len) {
			res = cms->res_data[cms->res_index];
			printf("cms: DATA[%i/%i] => %02X\n", cms->res_index, cms->res_len, res);
		}
	}
	printf("cms: input_response_byte = %02X\n", res);
	return res;
}


static void next_response_byte(struct CMS_STATE*cms)
{
	if (cms->dev_selected == -1) return;
	switch (cms->phase) {
	case PHASE_STATUS:
		cms_phase(cms, PHASE_MESSAGE);
		break;
	case PHASE_MESSAGE:
		cms_phase(cms, PHASE_WAIT);
		break;
	case PHASE_DATA_RECV:
		if (cms->res_index == -1) {
			cms_phase(cms, PHASE_IDLE);
			return;
		}
		if (cms->res_index < cms->res_len) {
			++ cms->res_index;
		}
		if (cms->res_index == cms->res_len) cms_phase(cms, PHASE_STATUS);
	}
}


static void cms_ack(struct CMS_STATE*cms, int ack) // ICR ACK signal
{
	printf("cms: set ACK = %i\n", ack?1:0);
	if (cms->dev_selected == -1) return;

	switch (cms->phase) {
	case PHASE_COMMAND:
	case PHASE_DATA_SEND:
	case PHASE_DATA_RECV:
	case PHASE_STATUS:
	case PHASE_MESSAGE:
		puts("cms: data acknowledged");
		if (ack) {
			CLEAR_BIT(cms->regs[REG_CSB], BIT_CSB_REQ);
		} else {
			SET_BIT(cms->regs[REG_CSB], BIT_CSB_REQ);
		}
		break;
	}
	switch (cms->phase) {
	case PHASE_STATUS:
	case PHASE_MESSAGE:
	case PHASE_DATA_RECV:
		if (!ack) {
			next_response_byte(cms);
			cms->regs[REG_CSD] = input_response_byte(cms);
		}
	case PHASE_COMMAND:
	case PHASE_DATA_SEND:
		if (!ack) {
			output_byte(cms, cms->reg_odr);
		}
	}
}

static void cms_dma(struct CMS_STATE*cms, int dma) // DMA signal
{
	printf("cms: set DMA = %i\n", dma?1:0);
	if (cms->dev_selected == -1) return;
	if (dma) {
		puts("cms: set DMA flag");
		SET_BIT(cms->regs[REG_BSR], BIT_BSR_DRQ);
	} else {
		CLEAR_BIT(cms->regs[REG_BSR], BIT_BSR_DRQ | BIT_BSR_EDMA);
	}
}



static void cms_tcr(struct CMS_STATE*cms, byte data, byte xd)
{
	if (cms->dev_selected == -1) return;
	SET_BIT(cms->regs[REG_BSR], BIT_BSR_PHSM);
	if (IS_SET(data, BIT_TCR_CD) && !IS_SET(data, BIT_TCR_IO) && IS_SET(xd, BIT_TCR_CD)) {
		puts("cms: preparing to receive command");
		cms_phase(cms, PHASE_COMMAND);
	} else if (IS_SET(data, BIT_TCR_CD) && IS_SET(data, BIT_TCR_IO) && !IS_SET(data, BIT_TCR_MSG)) {
		puts("cms: preparing to send command response");
	} else if (IS_SET(data, BIT_TCR_CD) && IS_SET(data, BIT_TCR_IO) && IS_SET(data, BIT_TCR_MSG)) {
		puts("cms: preparing to send message response");
	}
}


static void dma_receive(struct CMS_STATE*cms)
{
	printf("cms: starting DMA receive: length = %i, index = %i\n", cms->res_len, cms->res_index);
	cms_phase(cms, PHASE_DMA_RECV);
}

static void dma_send(struct CMS_STATE*cms)
{
	printf("cms: starting DMA send: length = %i, index = %i\n", 
		cms->recv_len, cms->recv_index);
	cms_phase(cms, PHASE_DMA_SEND);
}



static byte input_dma_byte(struct CMS_STATE*cms)
{
	byte res = 0;
	if (cms->dev_selected == -1) return 0xFF;
	if (cms->dma_index == -1) return 0xFF;
	if (cms->phase != PHASE_DMA_RECV) return 0xFF;
	if (cms->dma_index < cms->dma_len) {
		res = cms->res_data[cms->dma_index];
		++ cms->dma_index;
	}
	if (cms->dma_index == cms->dma_len) {
		cms_phase(cms, PHASE_STATUS);
	}
	return res;
}


static void output_data_byte(struct CMS_STATE*cms, byte data)
{
	if (cms->dev_selected == -1) return;
	if (cms->recv_index == -1) return;
	if (cms->phase != PHASE_DATA_SEND) return;
	if (cms->recv_index < cms->recv_len) {
		printf("cms: DATA[%i/%i] <= %02X\n", cms->recv_index, cms->recv_len, data);
		cms->res_data[cms->recv_index] = data;
		++cms->recv_index;
	}
	if (cms->recv_index == cms->recv_len) {
		cms->res_cmd = finish_command(cms, cms->cmd_data, cms->cmd_len, cms->res_data, cms->recv_len);
		cms_phase(cms, PHASE_STATUS);
	}
}

static void output_byte(struct CMS_STATE*cms, byte data)
{
	printf("OUTPUT[%s]: %02X\n", phnames[cms->phase], data);
	switch (cms->phase) {
	case PHASE_SELECT:
		select_device(cms, data);
		if (IS_SET(cms->regs[REG_ICR], BIT_ICR_BSY))
			SET_BIT(cms->regs[REG_CSB], BIT_CSB_BSY);
		else {
			if (cms->dev_selected != -1) { // acknowledge selection
				SET_BIT(cms->regs[REG_CSB], BIT_CSB_BSY);
			} else {
				CLEAR_BIT(cms->regs[REG_CSB], BIT_CSB_BSY);
			}
		}
		break;
	case PHASE_COMMAND:
		output_command_byte(cms, data);
		break;
	case PHASE_DATA_SEND:
		output_data_byte(cms, data);
		break;
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
		cms_phase(cms, PHASE_STATUS);
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
//	printf("cms: write io[%04X] <= %02X\n", adr, data);
	if (adr&0x08) { // switches
		switch (adr & 0x0F) {
		case 9: select_rom_bank(cms, data & 3); break;
		case 10: output_dma_byte(cms, data); break;
		}
	} else {
		byte xd;
		int i, d = data;
//		printf("cms: REGISTER: %s (%i)\n", rnames[adr&7], adr&7);
/*		printf("cms: %s WRITE FLAGS(%02X): {", rwnames[adr&7], data);
		for (i = 0; i < 8; ++i, d<<=1) {
			printf("%s:%i ", bwnames[adr&7][i], (d&0x80)?1:0);
		}
		printf("}\n");*/
		xd = cms->regs[adr&7] ^ data;
		switch (adr&7) {
		case _REG_ODR:
			cms->reg_odr = data;
			break;
		case REG_MR2:
			if (IS_SET(xd, BIT_MR2_ARB)) cms_arb(cms, IS_SET(data, BIT_MR2_ARB), cms->reg_odr);
			if (IS_SET(xd, BIT_MR2_DMA)) cms_dma(cms, IS_SET(data, BIT_MR2_DMA));
		case REG_SER:
			cms->regs[adr&7] = data;
			break;
		case REG_TCR:
			cms_tcr(cms, data, xd);
			cms->regs[adr&7] = data;
			break;
		case REG_ICR:
			if (IS_SET(xd, BIT_ICR_ACK)) cms_ack(cms, IS_SET(data, BIT_ICR_ACK));
			if (IS_SET(xd, BIT_ICR_SEL)) cms_sel(cms, IS_SET(data, BIT_ICR_SEL));
			if (IS_SET(xd, BIT_ICR_RST)) cms_rst(cms, IS_SET(data, BIT_ICR_RST));
			if (IS_SET(xd, BIT_ICR_BSY)) cms_bsy(cms, IS_SET(data, BIT_ICR_BSY));
			cms->regs[adr&7] = data;
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
		case 9: res = 0; break;  //?
		case 10: res = input_dma_byte(cms); break;
		case 11: res = 0; break; // arbiter delay
		case 12: res = 1; break; // ready timeout
		case 13: res = 0; break; // flags
		case 14: res = 0; break; // delay?
		case 15: res = 0; break;// delay?
		}
//		printf("cms: read io[%04X] => %02X\n", adr, res);
	} else { // registers
		int i, d;
		res = cms->regs[adr&7];
		switch (adr&7) {
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
/*		printf("cms: %s READ FLAGS(%02X): {", rrnames[adr&7], res);
		for (i = 0, d = res; i < 8; ++i, d<<=1) {
			printf("%s:%i ", brnames[adr&7][i], (d&0x80)?1:0);
		}
		printf("}\n");
		system_command(cms->st->sr, SYS_COMMAND_DUMPCPUREGS, 0, 0);*/
	}
//	printf("cms: read io[%04X] => %02X\n", adr, res);
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

	cms->devices = 0;
	if (cf->cfgint[CFG_INT_SCSI_NO1]) cms->devices |= 1<<(cf->cfgint[CFG_INT_SCSI_NO1]-1);
	if (cf->cfgint[CFG_INT_SCSI_NO2]) cms->devices |= 1<<(cf->cfgint[CFG_INT_SCSI_NO2]-1);
	if (cf->cfgint[CFG_INT_SCSI_NO3]) cms->devices |= 1<<(cf->cfgint[CFG_INT_SCSI_NO3]-1);

	if (cf->cfgint[CFG_INT_SCSI_NO1]) {
		cms->disks[0].no = cf->cfgint[CFG_INT_SCSI_NO1];
		cms->disks[0].mask = 1<<(cf->cfgint[CFG_INT_SCSI_NO1]-1);
		cms->disks[0].nblk = cf->cfgint[CFG_INT_SCSI_SZ1] * 2048;
		strcpy(cms->disks[0].name, cf->cfgstr[CFG_STR_SCSI_NAME1]);
	}
	if (cf->cfgint[CFG_INT_SCSI_NO2]) {
		cms->disks[1].no = cf->cfgint[CFG_INT_SCSI_NO2];
		cms->disks[1].mask = 1<<(cf->cfgint[CFG_INT_SCSI_NO2]-1);
		cms->disks[1].nblk = cf->cfgint[CFG_INT_SCSI_SZ2] * 2048;
		strcpy(cms->disks[1].name, cf->cfgstr[CFG_STR_SCSI_NAME2]);
	}
	if (cf->cfgint[CFG_INT_SCSI_NO3]) {
		cms->disks[2].no = cf->cfgint[CFG_INT_SCSI_NO3];
		cms->disks[2].mask = 1<<(cf->cfgint[CFG_INT_SCSI_NO3]-1);
		cms->disks[2].nblk = cf->cfgint[CFG_INT_SCSI_SZ3] * 2048;
		strcpy(cms->disks[2].name, cf->cfgstr[CFG_STR_SCSI_NAME3]);
	}

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
