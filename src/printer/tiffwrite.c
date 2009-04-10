#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "tiffwrite.h"


int tiff_create(FILE*out, struct TIFF_WRITER*tiff)
{
	unsigned int signature = 0x2A4949;
	fwrite(&signature, 1, 4, out);
	tiff->last_ifd_cnt_ofs = -1;
	tiff->n_ifd = 0;
	tiff->n_en = 0;
	tiff->n_data = 0;
	tiff->data_size = 0;
	tiff->n_strips = 1;
	tiff->strips_ofs_ofs = -1;
	tiff->strips_sz_ofs = -1;
	tiff->strips_size = 0;
	memset(tiff->strips, 0, sizeof(tiff->strips));
	return 0;
}

static int tiff_flush_data(FILE*out, struct TIFF_WRITER*tiff, int last)
{
	long ofs = ftell(out) + 4;
	int i;

	for (i = 0; i < tiff->n_strips; i++) {
		if (tiff->strips[i].length&1) tiff->strips_size++;
	}

	ofs += tiff->data_size + tiff->strips_size;

	if (last) {
		long z = 0;
		fwrite(&z, 1, 4, out);
	} else {
		fwrite(&ofs, 1, 4, out);
	}
	for (i = 0; i < tiff->n_data; i++) {
		int n;
		tiff->data[i].file_ofs = ftell(out);
		if (tiff->data[i].tag == 270) {
			tiff->_descr_ofs = tiff->data[i].file_ofs;
		}
		n = fwrite(tiff->data[i].data, 1, tiff->data[i].length, out);
		if (n != tiff->data[i].length) { // TODO: show write error message
				abort();
		}
		free(tiff->data[i].data);
	}
	for (i = 0; i < tiff->n_strips; i++) {
		int n;
		tiff->strips[i].file_ofs = ftell(out);
		if (!tiff->strips[i].length) continue;
		n = fwrite(tiff->strips[i].data, 1, tiff->strips[i].length, out);
		if (n != tiff->strips[i].length) { // TODO: show write error message
			abort();
		}
		if (tiff->strips[i].length&1) putc(0, out);
		free(tiff->strips[i].data);
	}
	for (i = 0; i < tiff->n_data; i++) {
		switch (tiff->data[i].tag) {
		case 273: // strip offsets
			tiff->strips_ofs_ofs = tiff->data[i].file_ofs;
			break;
		case 279: // strip sizes
			tiff->strips_sz_ofs = tiff->data[i].file_ofs;
			break;
		}
		fseek(out, tiff->data[i].chunk_ofs, SEEK_SET);
		fwrite(&tiff->data[i].file_ofs, 1, 4, out);
	}
	if (tiff->strips_ofs_ofs!=-1) {
		fseek(out, tiff->strips_ofs_ofs, SEEK_SET);
		for (i = 0; i < tiff->n_strips; i++) {
			fwrite(&tiff->strips[i].file_ofs, 1, 4, out);
		}
	}
	if (tiff->strips_sz_ofs!=-1) {
		fseek(out, tiff->strips_sz_ofs, SEEK_SET);
		for (i = 0; i < tiff->n_strips; i++) {
			fwrite(&tiff->strips[i].length, 1, 4, out);
		}
	}
	if (tiff->last_ifd_cnt_ofs!=-1) {
		fseek(out, tiff->last_ifd_cnt_ofs, SEEK_SET);
		fwrite(&tiff->n_en, 1, 2, out);
	}
	tiff->n_en = 0;
	tiff->n_data = 0;
	tiff->data_size = 0;
	tiff->strips_ofs_ofs = -1;
	tiff->strips_sz_ofs = -1;
	tiff->strips_size = 0;
	memset(tiff->strips, 0, sizeof(tiff->strips));
	fseek(out, ofs, SEEK_SET);
	return ofs;
}

int tiff_new_page(FILE*out, struct TIFF_WRITER*tiff)
{
	long ofs = tiff_flush_data(out, tiff, 0);
	tiff->n_ifd ++;
	tiff->last_ifd_cnt_ofs = ofs;
	{
		short cnt = 0;
		fwrite(&cnt, 1, 2, out);
	}
	return 0;
}

int tiff_new_chunk(FILE*out, struct TIFF_WRITER*tiff, int tag, int type, int count, const void*data)
{
	static const int sizes[]={0, 1, 1, 2, 4, 8, 1, 1, 2, 4, 8, 4, 8};
	int rsz, rsz1;
	tiff->n_en ++;
	fwrite(&tag, 1, 2, out);
	fwrite(&type, 1, 2, out);
	fwrite(&count, 1, 4, out);
	rsz = count * sizes[type];
	rsz1 = (rsz+1)&~1;
	if (rsz > 4) {
		int ofs = 0;
		struct TIFF_DATA_ENTRY*en = tiff->data + tiff->n_data;
		en->tag = tag;
		en->chunk_ofs = ftell(out);
		fwrite(&ofs, 1, 4, out);
		en->length = rsz1;
		en->data = malloc(rsz1);
		assert(en->data);
		memcpy(en->data, data, rsz);
		tiff->n_data ++;
		tiff->data_size += rsz1;
	} else {
		char val[4]={0,0,0,0};
		memcpy(val, data, rsz);
		switch (tag) {
		case 273: // strip offsets
			assert(type == TIFF_LONG);
			tiff->strips_ofs_ofs = ftell(out);
			break;
		case 279: // strip sizes
			assert(type == TIFF_LONG);
			tiff->strips_sz_ofs = ftell(out);
			break;
		}
		fwrite(val, 1, 4, out);
	}
	return 0;
}

int tiff_finish(FILE*out, struct TIFF_WRITER*tiff)
{
	tiff_flush_data(out, tiff, 1);
	return 0;
}

int tiff_new_chunk_short(FILE*out, struct TIFF_WRITER*tiff, int tag, int val)
{
	return tiff_new_chunk(out, tiff, tag, TIFF_SHORT, 1, &val);
}

int tiff_new_chunk_long(FILE*out, struct TIFF_WRITER*tiff, int tag, int val)
{
	return tiff_new_chunk(out, tiff, tag, TIFF_LONG, 1, &val);
}

int tiff_new_chunk_string(FILE*out, struct TIFF_WRITER*tiff, int tag, const char*val)
{
	return tiff_new_chunk(out, tiff, tag, TIFF_ASCII, strlen(val)+1, val);
}


int tiff_new_chunk_rat(FILE*out, struct TIFF_WRITER*tiff, int tag, int val1, int val2)
{
	int v[2]={val1, val2};
	return tiff_new_chunk(out, tiff, tag, TIFF_RATIONAL, 1, v);
}


int tiff_set_strips_count(struct TIFF_WRITER*tiff, int nstrips)
{
	assert(nstrips == 1);
	tiff->n_strips = nstrips;
	return 0;
}

int tiff_add_strip_data(FILE*out, struct TIFF_WRITER*tiff, int nstrip, const void*data, int len)
{
	struct TIFF_STRIP_ENTRY*se;
	int ofs;
	assert(!nstrip);
	se = tiff->strips + nstrip;
	ofs = se->length;
	if (ofs + len > se->mem_length) {
		while (ofs + len > se->mem_length) {
			se->mem_length+=TIFF_STRIP_INC;
		}
		se->data = realloc(se->data, se->mem_length);
	}
	memcpy((char*)se->data + ofs, data, len);
	se->length += len;
	tiff->strips_size += len;
	return 0;
}

