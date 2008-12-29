/*
	AVIFILE Library Copyright (c) NOP
*/

#include "avifile.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>

typedef int (*list_handler_t)(FILE*in,int ind,struct S_CHUNK_HEADER*hdr,void*f);

int avifile_align_read_chunks=1;

struct S_LIST_ENTRY_HANDLER
{
	fourcc_t type;
	list_handler_t handler;
};

struct S_LIST_HANDLERS
{
	fourcc_t type;
	int n_handlers;
	struct S_LIST_ENTRY_HANDLER*handlers;
	int n_sublists;
	struct S_LIST_HANDLERS*sublists;
	list_handler_t list_handler;
};


static void print_header(struct S_CHUNK_HEADER*hdr)
{
	fprintf(stderr,"%c%c%c%c; %x; %c%c%c%c\n",hdr->id[0],hdr->id[1],hdr->id[2],hdr->id[3],
		hdr->size,hdr->type[0],hdr->type[1],hdr->type[2],hdr->type[3]);
}


static void print_item(struct S_CHUNK_HEADER*hdr)
{
	fprintf(stderr,"%c%c%c%c; %x\n",hdr->id[0],hdr->id[1],hdr->id[2],hdr->id[3],
		hdr->size);
}


int avih_handler(FILE*in,int ind,struct S_CHUNK_HEADER*hdr,struct S_AVI_FILE*f);
int strh_handler(FILE*in,int ind,struct S_CHUNK_HEADER*hdr,struct S_AVI_FILE*f);
int strf_handler(FILE*in,int ind,struct S_CHUNK_HEADER*hdr,struct S_AVI_FILE*f);
int strn_handler(FILE*in,int ind,struct S_CHUNK_HEADER*hdr,struct S_AVI_FILE*f);
int idx1_handler(FILE*in,int ind,struct S_CHUNK_HEADER*hdr,struct S_AVI_FILE*f);
int movi_handler(FILE*in,int ind,struct S_CHUNK_HEADER*hdr,struct S_AVI_FILE*f);
int stream_junk_handler(FILE*in,int ind,struct S_CHUNK_HEADER*hdr,struct S_AVI_FILE*f);

int fmt_handler(FILE*in,int ind,struct S_CHUNK_HEADER*hdr,struct S_WAVE_FILE*f);
int fact_handler(FILE*in,int ind,struct S_CHUNK_HEADER*hdr,struct S_WAVE_FILE*f);
int data_handler(FILE*in,int ind,struct S_CHUNK_HEADER*hdr,struct S_WAVE_FILE*f);
int vdat_handler(FILE*in,int ind,struct S_CHUNK_HEADER*hdr,struct S_WAVE_FILE*f);

struct S_LIST_ENTRY_HANDLER hdrl_handlers[]={"avih",(list_handler_t)avih_handler};
struct S_LIST_ENTRY_HANDLER strl_handlers[]=
{
	{"strh",(list_handler_t)strh_handler},
	{"strf",(list_handler_t)strf_handler},
	{"strn",(list_handler_t)strn_handler},
	{"JUNK",(list_handler_t)stream_junk_handler}
};

struct S_LIST_ENTRY_HANDLER avi_handlers[]={"idx1",(list_handler_t)idx1_handler};

struct S_LIST_HANDLERS strl_sublist[]={"strl",4,strl_handlers,0,NULL,NULL};
struct S_LIST_HANDLERS avi_sublist[]={
	{"hdrl",1,hdrl_handlers,1,strl_sublist,NULL},
	{"movi",0,NULL,0,NULL,(list_handler_t)movi_handler}
};

struct S_LIST_HANDLERS avi_list[]={"AVI ",1,avi_handlers,2,avi_sublist,NULL};

struct S_LIST_ENTRY_HANDLER wave_handlers[]=
{
	{"fmt ",(list_handler_t)fmt_handler},
	{"fact",(list_handler_t)fact_handler},
	{"data",(list_handler_t)data_handler},
	{"VDAT",(list_handler_t)vdat_handler}
};
struct S_LIST_HANDLERS wave_list[]={"WAVE",4,wave_handlers,0,NULL,NULL};

int avih_handler(FILE*in,int ind,struct S_CHUNK_HEADER*hdr,struct S_AVI_FILE*f)
{
	puts("loading stream info");
	f->info_size=hdr->size;
	fread(&f->info,1,sizeof(f->info),in);
	f->streams=calloc(sizeof(f->streams[0]),f->info.n_streams);
	assert(f->streams);
	return 0;
}

int strh_handler(FILE*in,int ind,struct S_CHUNK_HEADER*hdr,struct S_AVI_FILE*f)
{
	puts("loading stream header");
	f->streams[ind].info_size=hdr->size;
	fread(&f->streams[ind].info,1,sizeof(f->streams[ind].info),in);
	return 0;
}

int stream_junk_handler(FILE*in,int ind,struct S_CHUNK_HEADER*hdr,struct S_AVI_FILE*f)
{
	f->streams[ind].junk_size=hdr->size;
	if (f->streams[ind].junk_data) free(f->streams[ind].junk_data);
	f->streams[ind].junk_data=malloc(hdr->size);
	assert(f->streams[ind].junk_data);
	fread(f->streams[ind].junk_data,hdr->size,1,in);
	return 0;
}

int strf_handler(FILE*in,int ind,struct S_CHUNK_HEADER*hdr,struct S_AVI_FILE*f)
{
	f->streams[ind].fmt_size=hdr->size;
	if (fcc_eq(f->streams[ind].info.type,"vids")) {
		f->video_ind=ind;
		puts("loading video stream format");
                f->streams[ind].video_format=malloc(hdr->size);
                assert(f->streams[ind].video_format);
		fread(f->streams[ind].video_format,1,hdr->size,in);
	} else if (fcc_eq(f->streams[ind].info.type,"auds")) {
		f->audio_ind=ind;
		puts("loading audio stream format");
                f->streams[ind].audio_format=malloc(hdr->size);
                assert(f->streams[ind].audio_format);
		fread(f->streams[ind].audio_format,1,hdr->size,in);
	} else puts("unknown type of stream");
	return 0;
}

int strn_handler(FILE*in,int ind,struct S_CHUNK_HEADER*hdr,struct S_AVI_FILE*f)
{
	if (f->streams[ind].name) free(f->streams[ind].name);
	f->streams[ind].name=malloc(hdr->size);
	assert(f->streams[ind].name);
	fread(f->streams[ind].name,1,hdr->size,in);
	printf("stream #%i has name \"%s\"\n",ind,f->streams[ind].name);
	return 0;
}


int idx1_handler(FILE*in,int ind,struct S_CHUNK_HEADER*hdr,struct S_AVI_FILE*f)
{
	puts("loading index");
	f->n_index=hdr->size/sizeof(f->index[0]);
	f->index=malloc(hdr->size);
	fread(f->index,1,hdr->size,in);
	return 0;
}

int movi_handler(FILE*in,int ind,struct S_CHUNK_HEADER*hdr,struct S_AVI_FILE*f)
{
	f->data_start=ftell(in);
	f->data_size=hdr->size-4;
//	printf("data_start=%x, data_size=%x\n",f->data_start,f->data_size);
	return 0;
}


int show_items(FILE*in,int size,void*f,struct S_LIST_HANDLERS*hnd,int ind,struct S_CHUNK_HEADER*prev_hdr)
{
	int nlists=0;
	if (hnd->list_handler) {
		int r;
		r=hnd->list_handler(in,ind,prev_hdr,f);
		if (r<0) return r;
	}
	if (!hnd->n_sublists&&!hnd->n_handlers) return 0;
//	printf("list: size=%i\n",size);
	while (!feof(in)&&size>0) {
		struct	S_CHUNK_HEADER item_head;
		long	pos;
//		printf("offset=%x\n",ftell(in));
		if (fread(&item_head,4,2,in)!=2) break;
		pos=ftell(in);
		size-=item_head.size+12;
		if (fcc_eq(item_head.id,"LIST")) {
			int i;
			fread(&item_head.type,4,1,in);
//			print_header(&item_head);
			for (i=0;i<hnd->n_sublists;i++) {
				if (fcc_eq(hnd->sublists[i].type,item_head.type)) {
					show_items(in,item_head.size,f,hnd->sublists+i,nlists,&item_head);
					nlists++;
					break;
				}
			}
		} else {
			int i;
//			print_item(&item_head);
			for (i=0;i<hnd->n_handlers;i++) {
				if (fcc_eq(hnd->handlers[i].type,item_head.id)) {
					if (hnd->handlers[i].handler(in,ind,&item_head,f)<0) return -1;
					break;
				}
			}
		}
		pos+=item_head.size;
		if (avifile_align_read_chunks) pos=(pos+1)&~1;
               	fseek(in,pos,SEEK_SET);
	}
	return 0;
}

void avi_file_clear(struct S_AVI_FILE*f)
{
	memset(f,0,sizeof(*f));
	f->video_ind=f->audio_ind=-1;
}

void avi_file_free(struct S_AVI_FILE*f)
{
	int i;
	if (f->index) free(f->index);
	for (i=0;i<f->info.n_streams;i++) {
		if (f->streams[i].name) free(f->streams[i].name);
		if (f->streams[i].audio_format) free(f->streams[i].audio_format);
	}
	if (f->streams) free(f->streams);
}



int avi_load(FILE*in,struct S_AVI_FILE*avi)
{
	struct S_CHUNK_HEADER file_head;
	avi_file_clear(avi);
	if (fread(&file_head,4,3,in)!=3) return -1;
	if (!fcc_eq(file_head.id,"RIFF ")||!fcc_eq(file_head.type,"AVI ")) {
		puts("invalid file format");
		return -1;
	}
	if (show_items(in,file_head.size,avi,avi_list,0,&file_head)<0) return -1;
	if (!avi->data_size) {
		puts("this file doesn't contains media data");
		return -1;
	}
	return 0;
}


int fmt_handler(FILE*in,int ind,struct S_CHUNK_HEADER*hdr,struct S_WAVE_FILE*f)
{
//	fprintf(stderr,"format\n");
	f->fmt_size=hdr->size;
	if (f->format) free(f->format);
	f->format=malloc(f->fmt_size);
	assert(f->format);
	fread(f->format,1,f->fmt_size,in);
	return 0;
}

int fact_handler(FILE*in,int ind,struct S_CHUNK_HEADER*hdr,struct S_WAVE_FILE*f)
{
//	fprintf(stderr,"fact\n");
	f->fact_size=hdr->size;
	if (f->fact) free(f->fact);
	f->fact=malloc(f->fact_size);
	assert(f->fact);
	fread(f->fact,1,f->fact_size,in);
	return 0;
}

int data_handler(FILE*in,int ind,struct S_CHUNK_HEADER*hdr,struct S_WAVE_FILE*f)
{
	f->data_start=ftell(in);
	f->data_size=hdr->size;
	return 0;
}

int vdat_handler(FILE*in,int ind,struct S_CHUNK_HEADER*hdr,struct S_WAVE_FILE*f)
{
	f->vdat_start=ftell(in);
	f->vdat_size=hdr->size;
	return 0;
}


void wave_file_clear(struct S_WAVE_FILE*f)
{
	memset(f,0,sizeof(*f));
}

void wave_file_free(struct S_WAVE_FILE*f)
{
	if (f->format) free(f->format);
	if (f->fact) free(f->fact);
}

int wave_load(FILE*in,struct S_WAVE_FILE*f)
{
	struct S_CHUNK_HEADER file_head;
//	wave_file_clear(f);
	if (fread(&file_head,4,3,in)!=3) return -1;
	if (!fcc_eq(file_head.id,"RIFF")||!fcc_eq(file_head.type,"WAVE")) {
		puts("invalid file format");
		return -1;
	}
	if (show_items(in,file_head.size,f,wave_list,0,&file_head)<0) return -1;
	if (!f->data_size) {
		puts("this file doesn't contains sound data");
		return -1;
	}
	return 0;
}





////////////////////// OUTPUT FUNCTIONS //////////////////////////


void write_dword(dword_t d,FILE*out)
{
	fwrite(&d,4,1,out);
}



int begin_wave_output(struct S_WAVE_OUTPUT*w,FILE*out,struct S_AUDIO_FORMAT*out_fmt,int fmt_size)
{
	return begin_wave_output_fact(w,out,out_fmt,fmt_size,NULL,0);
}

int begin_wave_output_fact(struct S_WAVE_OUTPUT*w,FILE*out,struct S_AUDIO_FORMAT*out_fmt,int fmt_size,void*fact_data,int fact_size)
{
	dword_t riff=fcc_dword("RIFF"), wave=fcc_dword("WAVE"),
		fmt=fcc_dword("fmt "), data=fcc_dword("data"),
		fact=fcc_dword("fact");
	dword_t nul=0;
	dword_t s1=(fmt_size+1)&~1;
	fwrite(&riff,4,1,out);
	w->fsize_offset=ftell(out);
	fwrite(&nul,4,1,out);
	fwrite(&wave,4,1,out);
	fwrite(&fmt,4,1,out);
	fwrite(&fmt_size,4,1,out);
	fwrite(out_fmt,1,s1,out);
	if (fact_data) {
		fwrite(&fact,4,1,out);
		fwrite(&fact_size,4,1,out);
		fwrite(fact_data,1,fact_size,out);
	}
	fwrite(&data,4,1,out);
	w->data_offset=ftell(out);
	fwrite(&nul,4,1,out);
	return 0;
}

int finish_wave_output(struct S_WAVE_OUTPUT*w,FILE*out)
{
	long pos=ftell(out);
	dword_t d;
	fseek(out,w->data_offset,SEEK_SET);
	d=pos-w->data_offset-4;
	fwrite(&d,4,1,out);
	fseek(out,w->fsize_offset,SEEK_SET);
	d=pos-w->fsize_offset-4;
	fwrite(&d,4,1,out);
	fseek(out,pos,SEEK_SET);
	return 0;
}

int finish_wave_output_vdat(struct S_WAVE_OUTPUT*w,FILE*out,void*vdat,dword_t vdat_size)
{
	const dword_t vdattag=fcc_dword("VDAT");
	long pos, pos1;
	dword_t d;
	if (!vdat_size) return finish_wave_output(w,out);
	pos=ftell(out);
	fwrite(&vdattag,4,1,out);
	fwrite(&vdat_size,4,1,out);
	fwrite(vdat,1,vdat_size,out);
	pos1=ftell(out);

	fseek(out,w->data_offset,SEEK_SET);
	d=pos-w->data_offset-4;
	fwrite(&d,4,1,out);
	fseek(out,w->fsize_offset,SEEK_SET);
	d=pos1-w->fsize_offset-4;
	fwrite(&d,4,1,out);
	fseek(out,pos1,SEEK_SET);
	return 0;
}

static int write_info(struct S_AVI_OUTPUT*w,FILE*out,struct S_AVI_FILE*avi)
{
	return fwrite(&avi->info,avi->info_size,1,out);
}

static int write_streams(struct S_AVI_OUTPUT*w,FILE*out,struct S_AVI_FILE*avi)
{
	int i;
	const dword_t riff=fcc_dword("RIFF"), savi=fcc_dword("AVI "),
		avih=fcc_dword("avih"), list=fcc_dword("LIST"),
		strl=fcc_dword("strl"), strf=fcc_dword("strf"),
		strn=fcc_dword("strn"), movi=fcc_dword("movi"),
		hdrl=fcc_dword("hdrl"), strh=fcc_dword("strh");
	for (i=0;i<avi->info.n_streams;i++) {
		dword_t ss=4;
		write_dword(list,out);
		ss+=8+avi->streams[i].fmt_size;
		ss+=8+avi->streams[i].info_size;
                if (avi->streams[i].junk_size) {
                	ss+=8+avi->streams[i].junk_size;
                }
		write_dword(ss,out);
		write_dword(strl,out);
		write_dword(strh,out);
		write_dword(avi->streams[i].info_size,out);
		fwrite(&avi->streams[i].info,avi->streams[i].info_size,1,out);
		write_dword(strf,out);
		write_dword(avi->streams[i].fmt_size,out);
		fwrite(avi->streams[i].format,avi->streams[i].fmt_size,1,out);
                if (avi->streams[i].junk_size) {
			write_dword(fcc_dword("JUNK"),out);
			write_dword(avi->streams[i].junk_size,out);
			fwrite(avi->streams[i].junk_data,1,avi->streams[i].junk_size,out);
		}
	}
	return 0;
}

int begin_avi_output(struct S_AVI_OUTPUT*w,FILE*out,struct S_AVI_FILE*avi)
{
	const dword_t riff=fcc_dword("RIFF"), savi=fcc_dword("AVI "),
		avih=fcc_dword("avih"), list=fcc_dword("LIST"),
		strl=fcc_dword("strl"), strf=fcc_dword("strf"),
		strn=fcc_dword("strn"), movi=fcc_dword("movi"),
		hdrl=fcc_dword("hdrl"), strh=fcc_dword("strh");
	int i;
	dword_t header_size=12+avi->info_size;
	for (i=0;i<avi->info.n_streams;i++) {
		header_size+=12;
		header_size+=8+avi->streams[i].fmt_size;
		header_size+=8+avi->streams[i].info_size;
                if (avi->streams[i].junk_size) {
                	header_size+=8+avi->streams[i].junk_size;
                }
	}
	write_dword(riff,out);
	w->fsize_offset=ftell(out);
	write_dword(0,out);
	write_dword(savi,out);
	write_dword(list,out);
	write_dword(header_size,out);
	write_dword(hdrl,out);
	write_dword(avih,out);
	write_dword(avi->info_size,out);
	w->info_offset=ftell(out);
	write_info(w,out,avi);
	w->streams_offset=ftell(out);
	write_streams(w,out,avi);

	write_dword(list,out);
	w->movi_offset=ftell(out);
	write_dword(0,out);
	write_dword(movi,out);
	return 0;
}

int finish_avi_output(struct S_AVI_OUTPUT*w,FILE*out,struct S_AVI_FILE*avi)
{
	long pos, m_pos;
	dword_t d;
	m_pos=ftell(out);
	if (avi->n_index) {
		dword_t idx1=fcc_dword("idx1");
		int size=avi->n_index*sizeof(avi->index[0]);
		write_dword(idx1,out);
		write_dword(size,out);
		fwrite(avi->index,1,size,out);
	}
 	pos=ftell(out);
	fseek(out,w->info_offset,SEEK_SET);
	write_info(w,out,avi);
	fseek(out,w->streams_offset,SEEK_SET);
	write_streams(w,out,avi);
	fseek(out,w->movi_offset,SEEK_SET);
	d=m_pos-w->movi_offset-4;
	fwrite(&d,4,1,out);
	fseek(out,w->fsize_offset,SEEK_SET);
	d=pos-w->fsize_offset-4;
	fwrite(&d,4,1,out);
	fseek(out,pos,SEEK_SET);
	return 0;
}

int append_avi_frame(struct S_AVI_OUTPUT*w,FILE*out,struct S_AVI_FILE*avi,dword_t type,dword_t flags,void*data,dword_t size,int append_index)
{
	long ofs=ftell(out)-w->movi_offset-4;
	write_dword(type,out);
	write_dword(size,out);
	fwrite(data,1,size,out);
	if (size&1) fwrite(data,1,1,out);
	if (append_index) {
		struct S_INDEX_ENTRY en;
		dword_fcc(en.type,type);
		en.size=size;
		en.flags=flags;
		en.offset=ofs;
		avi->n_index++;
		avi->index=realloc(avi->index,avi->n_index*sizeof(avi->index[0]));
		assert(avi->index);
		avi->index[avi->n_index-1]=en;
//		printf("type=%x, size=%x, offset=%x, flags=%x\n",type,en.size,en.offset,en.flags);
	}
	return 0;
}

int append_avi_frame_from_file(struct S_AVI_OUTPUT*w,FILE*out,struct S_AVI_FILE*avi,dword_t type,dword_t flags,FILE*in,dword_t size,int append_index)
{
	long ofs=ftell(out)-w->movi_offset-4;
	char buffer[16384];
	int rem=size, r, rr, rw;
	write_dword(type,out);
	write_dword(size,out);
	while (rem) {
		r=rem;
		if (r>sizeof(buffer)) r=sizeof(buffer);
		rr=fread(buffer,1,r,in);
		if (rr!=r) {
			perror("can't read file data");
			return -1;
		}
		rw=fwrite(buffer,1,rr,out);
		if (rw!=rr) {
			perror("can't write file data");
			return -1;
		}
		rem-=r;
	}
	if (size&1) fwrite(buffer,1,1,out);
	if (append_index) {
		struct S_INDEX_ENTRY en;
		dword_fcc(en.type,type);
		en.size=size;
		en.flags=flags;
		en.offset=ofs;
		avi->n_index++;
		avi->index=realloc(avi->index,avi->n_index*sizeof(avi->index[0]));
		assert(avi->index);
		avi->index[avi->n_index-1]=en;
//		printf("type=%x, size=%x, offset=%x, flags=%x\n",type,en.size,en.offset,en.flags);
	}
	return 0;
}

