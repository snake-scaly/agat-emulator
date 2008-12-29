/*
	AVIFILE Library copyright (c) Nop 2000-2008
*/

#ifndef AVIFILE_H
#define AVIFILE_H

#include <stdio.h>

#ifndef USE_WAVE_INPUT
#define USE_WAVE_INPUT 1
#endif

#ifndef USE_WAVE_OUTPUT
#define USE_WAVE_OUTPUT 1
#endif

#ifndef USE_AVI_INPUT
#define USE_AVI_INPUT 1
#endif

#ifndef USE_AVI_OUTPUT
#define USE_AVI_OUTPUT 1
#endif

typedef unsigned char fourcc_t[4];
typedef unsigned int   dword_t;
typedef unsigned short word_t;

extern int avifile_align_read_chunks;

#pragma pack(push,1)


struct S_CHUNK_HEADER
{
	fourcc_t	id;
	dword_t		size;
	fourcc_t	type;
};

struct S_FILE_INFO
{
	dword_t	micro_sec_per_frame;
	dword_t max_bytes_per_sec;
	dword_t granularity;
	dword_t flags;
	dword_t total_frames;
	dword_t initial_frames;
	dword_t n_streams;
	dword_t suggest_buf_size;
	dword_t width;
	dword_t height;
	dword_t res2[4];
};


struct S_STREAM_INFO
{
	fourcc_t	type, handler;
	dword_t		flags;
	word_t		priority, language;
	dword_t		initial_frames;
	dword_t		scale;
	dword_t		rate;
	dword_t		start;
	dword_t		length;
	dword_t		suggest_buf_size;
	dword_t		quality;
	dword_t		sample_size;
	dword_t		frame_x,frame_y,frame_w,frame_h;
};

struct S_VIDEO_FORMAT
{
	dword_t		struct_size;
	long		width, height;
	word_t		n_planes, bit_count;
	dword_t		compr_type;
	dword_t		img_size;
	long		x_res, y_res;
	dword_t		clr_used,clr_important;
};

struct S_AUDIO_FORMAT
{
	word_t		format_tag;
	word_t		n_channels;
	dword_t		n_samples_per_sec;
	dword_t		n_bytes_per_sec;
	word_t		block_align;
	word_t		bits_per_sample;
	word_t		extra_size;
};

struct S_INDEX_ENTRY
{
	fourcc_t	type;
	dword_t		flags;
	dword_t		offset;
	dword_t		size;
};

#pragma pack(pop)

struct S_AVI_FILE
{
	struct S_FILE_INFO info;
	int info_size;
	struct S_AVI_STREAM {
		struct S_STREAM_INFO info;
		union {
			struct S_VIDEO_FORMAT *video_format;
			struct S_AUDIO_FORMAT *audio_format;
			void *format;
		};
		void*junk_data;
		int junk_size;
		int fmt_size, info_size;
		char*name;
	} *streams;
	int video_ind, audio_ind;
	int n_index;
	struct S_INDEX_ENTRY*index;
	long data_start;
	dword_t data_size;
};

struct S_WAVE_FILE
{
	struct S_AUDIO_FORMAT*format;
	int fmt_size;
	void*fact;
	int fact_size;
	long data_start;
	dword_t data_size;
	long vdat_start;
	dword_t vdat_size;
};


#define fcc_eq(d,p) ((p)[0]==(d)[0]&&(p)[1]==(d)[1]&&(p)[2]==(d)[2]&&(p)[3]==(d)[3])
#define fcc_set(p,d) ((p)[0]=(d)[0],(p)[1]=(d)[1],(p)[2]=(d)[2],(p)[3]=(d)[3])
#define fcc_dword(c) (c[0]|(((unsigned)c[1])<<8)|(((unsigned)c[2])<<16)|(((unsigned)c[3])<<24))
#define dword_fcc(c,d) ((c)[0]=(d)&0xFF,(c)[1]=((d)>>8)&0xFF,(c)[2]=((d)>>16)&0xFF,(c)[3]=((d)>>24)&0xFF,d)

void avi_file_clear(struct S_AVI_FILE*f);
void avi_file_free(struct S_AVI_FILE*f);
int avi_load(FILE*in,struct S_AVI_FILE*avi);

void wave_file_clear(struct S_WAVE_FILE*f);
void wave_file_free(struct S_WAVE_FILE*f);
int wave_load(FILE*in,struct S_WAVE_FILE*f);



struct S_WAVE_OUTPUT
{
	long fsize_offset, data_offset;
};
int begin_wave_output(struct S_WAVE_OUTPUT*w,FILE*out,struct S_AUDIO_FORMAT*out_fmt,int fmt_size);
int begin_wave_output_fact(struct S_WAVE_OUTPUT*w,FILE*out,struct S_AUDIO_FORMAT*out_fmt,int fmt_size,void*fact,int fact_size);
int finish_wave_output(struct S_WAVE_OUTPUT*w,FILE*out);
int finish_wave_output_vdat(struct S_WAVE_OUTPUT*w,FILE*out,void*vdat,dword_t vdat_size);


struct S_AVI_OUTPUT
{
	long info_offset, streams_offset, fsize_offset, movi_offset;
};

int begin_avi_output(struct S_AVI_OUTPUT*w,FILE*out,struct S_AVI_FILE*avi);
int finish_avi_output(struct S_AVI_OUTPUT*w,FILE*out,struct S_AVI_FILE*avi);
int append_avi_frame(struct S_AVI_OUTPUT*w,FILE*out,struct S_AVI_FILE*avi,dword_t type,dword_t flags,void*data,dword_t size,int append_index);

#endif //AVIFILE_H

