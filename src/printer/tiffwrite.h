
struct TIFF_DATA_ENTRY
{
	void*data;
	int tag;
	int length;
	long chunk_ofs;
	long file_ofs;
};

#define TIFF_MAX_DATA 100
#define TIFF_MAX_STRIPS 1
#define TIFF_STRIP_INC 1024

struct TIFF_STRIP_ENTRY
{
	void*data;
	int  length;
	int  mem_length;
	long file_ofs;
};

struct TIFF_WRITER
{
	long last_ifd_cnt_ofs;
	int n_ifd;
	int n_en;
	int n_data;
	int n_strips;
	struct TIFF_DATA_ENTRY data[TIFF_MAX_DATA];
	struct TIFF_STRIP_ENTRY strips[TIFF_MAX_STRIPS];
	long strips_ofs_ofs, strips_sz_ofs;
	int data_size, strips_size;

	int _descr_ofs;
};


enum {
	TIFF_ZERO,
	TIFF_BYTE,
	TIFF_ASCII,
	TIFF_SHORT,
	TIFF_LONG,
	TIFF_RATIONAL,
	TIFF_SBYTE,
	TIFF_UNDEFINED,
	TIFF_SSHORT,
	TIFF_SLONG,
	TIFF_SRATIONAL,
	TIFF_FLOAT,
	TIFF_DOUBLE
};

int tiff_create(FILE*out, struct TIFF_WRITER*tiff);
int tiff_new_page(FILE*out, struct TIFF_WRITER*tiff);
int tiff_new_chunk(FILE*out, struct TIFF_WRITER*tiff, int tag, int type, int count, const void*data);
int tiff_new_chunk_short(FILE*out, struct TIFF_WRITER*tiff, int tag, int val);
int tiff_new_chunk_long(FILE*out, struct TIFF_WRITER*tiff, int tag, int val);
int tiff_new_chunk_rat(FILE*out, struct TIFF_WRITER*tiff, int tag, int val1, int val2);
int tiff_new_chunk_string(FILE*out, struct TIFF_WRITER*tiff, int tag, const char*val);
int tiff_set_strips_count(struct TIFF_WRITER*tiff, int nstrips);
int tiff_add_strip_data(FILE*out, struct TIFF_WRITER*tiff, int nstrip, const void*data, int len);
int tiff_finish(FILE*out, struct TIFF_WRITER*tiff);
