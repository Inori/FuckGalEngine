#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <resource.h>
#include <package.h>
#include <cui_error.h>
#include <zlib.h>
#include <utility.h>
#include <crass/locale.h>
#include <stdio.h>
#include "xp3filter_decode.h"
#include <vector>

using namespace std;
using std::vector;

/*
tcwfcomp音频压缩
root/kirikiri2/branches/2.30stable/kirikiri2/src/tools/generic/tcwfcomp/src/comp/Main.cpp
*/

// 以下.xp3和tlg的大部分解析和解码代码参考了kirikiri2-2_27-dev.20060527源码

/*
cxdec解密参数搜索法：
到kernel32下CreateFileW断点，直到访问第一个.xp3文件为止。
（这样做的原因是等tpm初始化代码将解密函数解密出来）
然后到*_cxdec模块的地址空间搜索常量0x18, 应该能找到类似这样的语句：

	CMP     DWORD PTR DS:[ESI], 18
	JE      SHORT cxdec.1E003086

跟随这个JE条件，被调用的函数里面就有2个参数。然后跟随进去，分析和
找到first_stage、stage0/1的函数以及内部switch的具体顺序。
*/

struct acui_information kirikiri2_cui_information = {
	_T("W.Dee"),			/* copyright */
	_T("TVP(KIRIKIRI) 2 core / Scripting Platform for Win32"),	/* system */
	_T(".xp3 .exe .tlg .tlg5 .tlg6 .ksd .kdt .arc"),	/* package */
	_T("1.5.5"),			/* revison */
	_T("痴漢公賊"),			/* author */
	_T("2009-7-31 23:05"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};	

#define TVP_XP3_INDEX_ENCODE_METHOD_MASK	0x07
#define TVP_XP3_INDEX_ENCODE_RAW			0
#define TVP_XP3_INDEX_ENCODE_ZLIB			1

#define TVP_XP3_INDEX_CONTINUE				0x80

#define TVP_XP3_FILE_PROTECTED				(1<<31)

#define TVP_XP3_SEGM_ENCODE_METHOD_MASK		0x07
#define TVP_XP3_SEGM_ENCODE_RAW				0
#define TVP_XP3_SEGM_ENCODE_ZLIB			1

extern unsigned int CheckSum(BYTE *buf, unsigned int len);

// 文件格式定义	
#pragma pack (1)
typedef struct {
	s8 magic[11];		// "XP3\r\n \n\x1a\x8b\x67\x01"
	u32 index_offset_lo;
	u32 index_offset_hi;	
} xp3_header_t;

typedef struct {		// 2.29-dev.20070612 and later
	s8 magic[11];		// "XP3\r\n \n\x1a\x8b\x67\x01"
	u32 cushion_header_offset_lo;	// 11+8+4
	u32 cushion_header_offset_hi;
	u32 minor_version;	// 1
	// cushion header
	u8 flag;			// 0x80, TVP_XP3_INDEX_CONTINUE
	u32 index_size_lo;	
	u32 index_size_hi;	
	u32 index_offset_lo;
	u32 index_offset_hi;
} xp3_header2_t;

typedef struct {
	s8 flag;
	u32 comprlen_lo;
	u32 comprlen_hi;
	u32 uncomprlen_lo;
	u32 uncomprlen_hi;
} xp3_dir_t;

typedef struct {
	s8 magic[4];		
	u32 length_lo;
	u32 length_hi;
} xp3_chunk_t;

typedef struct {
	s8 magic[4];		// "File"
	u32 length_lo;
	u32 length_hi;
} xp3_file_chunk_t;

typedef struct {
	s8 magic[4];		// "info"
	u32 length_lo;
	u32 length_hi;
	u32 flags;
	u32 orig_length_lo;
	u32 orig_length_hi;
	u32 pkg_length_lo;
	u32 pkg_length_hi;
	u16 name_length;	// in WCHAR
	//WCHAR *name;
} xp3_info_chunk_t;

typedef struct {
	s8 magic[4];		// "segm"
	u32 length_lo;
	u32 length_hi;
} xp3_segm_chunk_t;

typedef struct {
	u32 flags;
	u32 offset_lo;
	u32 offset_hi;
	u32 orig_length_lo;
	u32 orig_length_hi;
	u32 pkg_length_lo;
	u32 pkg_length_hi;
} xp3_segm_entry_t;

typedef struct {
	s8 magic[4];		// "adlr"
	u32 length_lo;		// 4
	u32 length_hi;	
	u32 hash;
} xp3_adlr_chunk_t;

typedef struct {
	s8 mark[11];		// "TLG0.0\x0sds\x1a\x0"
	u32 size;	
} tlg_sds_header_t;

/* 
 * tlg5采用差值+lzss的方式对位图进行压缩。
 * 1. 计算同列上下2个相邻象素分量的差值cl。
 * 2. 将差值cl与上一次同列上下2个相邻象素分量的
 * 差值prevcl[c]再次求差, 结果保存在val[c]中，同时
 * prevcl[c]更新为cl。
 * 3. 对同一象素内的相邻vcl[c]求差，结果保存在cmpinbuf[c][inp]中，
 * 直到inp == width。至此colors个cmpinbuf[width]组成一个block。
 * 4. 对block进行lzss(变体)压缩，将压缩后的长度写入block size section。
 */
typedef struct {
	s8 mark[11];	// "TLG5.0\x0raw\x1a"
	u8 colors;		// 3 or 4
	u32 width;
	u32 height;
	u32 blockheight;// 4
} tlg5_header_t;

typedef struct {
	s8 mark[11];		// "TLG6.0\x0raw\x1a"
	u8 colors;
	u8 flag;
	u8 type;
	u8 golomb_bit_length;
	u32 width;
	u32 height;
	u32 max_bit_length;
	u32 filter_length;
	//u8 *filter;
} tlg6_header_t;

// フルハウスキス２～恋愛迷宮～的tpk格式
typedef struct {
	s8 maigc[4];		// "EL"
	s8 version[4];		// "0101"
	u32 size;			// 文件大小，应该等于接下来3个字段之和
	u32 header_size;	// 包含palette
	u32 data_size;		
	u32 block_size;		// 没有则为0
	u32 data_offset;
	u32 block_offset;
	u32 width;
	u32 height;
	u32 unknown[7];		// 0, 0, 0, 0, 3, 3, 0 or 0, 0, 1, 1, 3, 3, 0 or 0, 0, 2, 2, 3, 3, 0
	s8 desc[12];		// "png"
} tpk_header_t;

typedef struct {
	u32 orig_x;
	u32 orig_y;
	u32 width;
	u32 height;
	u32 offset;			// 0开始
	u32 length;
	u32 reserved[2];	// 0
} tpk_entry_t;
#pragma pack ()	

typedef struct {
	WCHAR *name;
	DWORD name_length;
	DWORD comprlen;
	DWORD uncomprlen;
	DWORD flags;
	DWORD hash;
	DWORD segm_number;
	xp3_segm_entry_t *segm;
} my_xp3_entry_t;

static const TCHAR *simplified_chinese_strings[] = {
	_T("%s: crc校验失败(0x%08x), 应该是0x%08x.\n"),
};

static const TCHAR *traditional_chinese_strings[] = {
	_T("%s: crc校驗失敗(0x%08x), 應該是0x%08x.\n"),
};

static const TCHAR *japanese_strings[] = {
	_T("%s: crcチェック失敗(0x%08x)、もとい0x%08x。\n"),
};

static const TCHAR *default_strings[] = {
	_T("%s: crc error(0x%08x), shoule be 0x%08x.\n"),
};

static struct locale_configuration kirikiri2_locale_configurations[4] = {
	{
		936, simplified_chinese_strings, 1
	},
	{
		950, traditional_chinese_strings, 1
	},
	{
		932, japanese_strings, 1
	},
	{
		0, default_strings, 1
	},
};

static int kirikiri2_locale_id;

static const char *xp3dec_string;
static u64 xp3_start_offset = 0;
static void kirikiri2_tlg_release(struct package *pkg, 
								  struct package_directory *pkg_dir);

static BYTE dummy_png[] =
		"\x89\x50\x4e\x47\x0a\x1a\x0a\x00\x00\x00\x0d\x49\x48\x44\x52\x00"
		"\x00\x00\x01\x00\x00\x00\x01\x08\x02\x00\x00\x00\x90\x77\x53\xde"
		"\x00\x00\x00\xa5\x74\x45\x58\x74\x57\x61\x72\x6e\x69\x6e\x67\x00"
		"\x57\x61\x72\x6e\x69\x6e\x67\x3a\x20\x45\x78\x74\x72\x61\x63\x74"
		"\x69\x6e\x67\x20\x74\x68\x69\x73\x20\x61\x72\x63\x68\x69\x76\x65"
		"\x20\x6d\x61\x79\x20\x69\x6e\x66\x72\x69\x6e\x67\x65\x20\x6f\x6e"
		"\x20\x61\x75\x74\x68\x6f\x72\x27\x73\x20\x72\x69\x67\x68\x74\x73"
		"\x2e\x20\x8c\x78\x8d\x90\x20\x3a\x20\x82\xb1\x82\xcc\x83\x41\x81"
		"\x5b\x83\x4a\x83\x43\x83\x75\x82\xf0\x93\x57\x8a\x4a\x82\xb7\x82"
		"\xe9\x82\xb1\x82\xc6\x82\xc9\x82\xe6\x82\xe8\x81\x41\x82\xa0\x82"
		"\xc8\x82\xbd\x82\xcd\x92\x98\x8d\xec\x8e\xd2\x82\xcc\x8c\xa0\x97"
		"\x98\x82\xf0\x90\x4e\x8a\x51\x82\xb7\x82\xe9\x82\xa8\x82\xbb\x82"
		"\xea\x82\xaa\x82\xa0\x82\xe8\x82\xdc\x82\xb7\x81\x42\x4b\x49\x44"
		"\x27\x00\x00\x00\x0c\x49\x44\x41\x54\x78\x9c\x63\xf8\xff\xff\x3f"
		"\x00\x05\xfe\x02\xfe\x0d\xef\x46\xb8\x00\x00\x00\x00\x49\x45\x4e"
		"\x44\xae\x42\x60\x82\x89\x50\x4e\x47\x0a\x1a\x0a\x00\x00\x00\x0d"
		"\x49\x48\x44\x52\x00\x00\x01\xef\x00\x00\x00\x13\x01\x03\x00\x00"
		"\x00\x83\x60\x17\x58\x00\x00\x00\x06\x50\x4c\x54\x45\x00\x00\x00"
		"\xff\xff\xff\xa5\xd9\x9f\xdd\x00\x00\x02\x4f\x49\x44\x41\x54\x78"
		"\xda\xd5\xd3\x31\x6b\xdb\x40\x14\x07\x70\x1d\x0a\x55\xb3\x44\x6d"
		"\xb2\xc4\x20\xac\x14\x9b\x78\x15\xf1\x12\x83\xf1\x0d\x1d\x4a\x21"
		"\x44\x1f\xa2\xa1\x5a\xd3\x78\x49\xc0\x44\x86\x14\xb2\x04\xec\xc4"
		"\x53\x40\xf8\xbe\x8a\x4c\x42\x6a\x83\xf0\x7d\x05\xb9\x15\xba\x55"
		"\xe8\x2d\x3e\x10\x7a\x3d\x25\xf9\x06\x19\x4a\x6f\x38\x74\x9c\x7e"
		"\xf7\x7f\x8f\xe3\x34\xc4\x37\x8c\x52\x7b\x8b\xfe\xe7\x3c\xe3\x8b"
		"\xd7\xef\x02\x45\x06\x99\xae\x99\x02\x11\x10\x39\xa2\x2c\x7d\x2e"
		"\x68\x3b\xf7\x53\x1f\x27\x65\x17\xd6\xba\x44\x51\xed\x31\x79\xcd"
		"\xd4\xff\xbc\xd4\x62\xa2\x78\x3c\xb0\x48\xb5\xcc\x00\xc0\xe1\x82"
		"\xc4\x7d\x89\xbc\xf1\xc2\x0f\xb5\x33\x3f\xbd\x34\xc7\x4e\x02\x12"
		"\xd4\xd9\x04\x0a\xe3\x56\xf1\xdb\x67\x9e\x6d\x0e\x6d\xc4\xb5\x2b"
		"\x15\x5f\x92\xe1\xa9\xae\xbd\x27\x80\x00\x06\xdf\xc4\x70\xd7\x20"
		"\x73\xb3\x9d\xea\x5a\xb1\xbf\x51\x24\xc3\x33\xbd\x33\x27\x10\xd2"
		"\xe5\xc6\xfa\x88\xfc\x0c\x1d\x5d\xf1\x46\x47\x15\x9e\x7b\xf7\x18"
		"\x9b\x4c\x8a\xdc\x55\xe9\xaa\xc0\x1d\x9c\xd5\x54\x7a\x9f\x73\x1a"
		"\x25\x2c\x91\xed\xe1\x87\xa6\x00\x45\x85\x00\xc6\x6e\x56\x26\xbb"
		"\x79\x4e\x2f\xbf\xaa\x3a\x15\x9f\xb0\x82\xdb\x72\x55\xf5\x6e\xcc"
		"\x70\xdd\x47\xde\xc1\xac\x77\x11\xba\x5d\x32\x9b\x6a\xb5\xf6\x66"
		"\x37\x59\xc5\x44\xa2\x31\x03\x4e\x03\xd9\xd2\x83\xb0\x6e\x56\xbd"
		"\x6b\x26\x62\xea\x4d\xc2\x6e\x64\xcb\xc7\x03\x97\x2e\xbd\x00\x25"
		"\x54\x3c\xb6\x04\xe3\xf4\x41\x5c\x09\x68\x6f\x9f\x9f\x3c\x3a\x52"
		"\xdb\xf2\x82\xec\x50\xb7\xe4\x3e\x09\x8a\x82\xbf\x5e\x9c\x48\xbd"
		"\x6b\x2c\x16\x0c\xe6\x19\xd9\x3b\xf6\x7a\x1e\xbc\xf0\x23\xc5\x0f"
		"\x35\x8f\x0d\xfb\x07\xda\x6e\x73\x9e\xeb\x58\x7a\xbd\x6f\x8c\x5a"
		"\xb0\xbf\xf1\xc2\xd7\x46\x48\x91\xa7\x1e\x43\xc9\x19\x64\xf9\x8e"
		"\xc3\x8d\x27\x57\x40\xcf\xec\xe0\x3c\xba\x9e\x44\xbd\x8e\x98\x6e"
		"\xf5\x0f\xb6\x4f\x93\x0c\x5a\x21\x35\x9e\x3e\x4f\x2f\x2d\x68\xde"
		"\x04\x71\x69\xd6\x55\x3a\xa7\x9c\x27\x82\x56\x5c\x24\xf9\x97\x3d"
		"\x57\xdc\xb9\x22\x1f\x3c\x48\x16\x2d\x1a\xad\xc5\x2e\x11\x57\xe1"
		"\x59\x7f\x6c\x15\x09\x8c\x38\x15\x77\x15\x6f\x77\xa3\x22\xa2\xcb"
		"\x63\x95\xce\x55\xba\xa3\x26\xe0\x8c\xab\x2e\x1c\xce\xc7\xbc\x95"
		"\x0f\x16\xc0\x17\xf7\x9f\x5a\x2b\xb3\x23\xd8\xf2\xdc\xbb\x1b\x14"
		"\x02\x5a\x2a\x6a\xfc\x30\xf5\x83\x66\xf7\x5e\x46\x6c\x7a\xac\x49"
		"\xa1\x8a\x8f\x2f\xea\x3e\xa6\x36\xb3\xb3\xe6\xc7\xe6\xc8\x9e\xc5"
		"\xa7\xb5\x77\xf6\x2f\xf1\x9b\x8e\xb2\x13\x9f\x08\x16\x0e\x46\x63"
		"\x6b\x9d\x39\x3f\x42\x6a\xcf\x12\x6a\x4c\xbf\x5f\x36\xfe\xac\x4a"
		"\x5a\x57\xe9\xff\xf1\x8b\x7b\x1b\xff\x0b\x28\x8d\x8d\xf8\xb3\xe9"
		"\xa1\xdf\x00\x00\x00\x00\x49\x45\x4e\x44\xae\x42\x60\x82";

static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static int TVPTLG5DecompressSlide(BYTE *out, const BYTE *in, 
								  int insize, BYTE *text, int initialr)
{
	int r = initialr;
	unsigned int flags = 0;
	const BYTE *inlim = in + insize;

	while (in < inlim) {
		if (!((flags >>= 1) & 0x100))
			flags = *in++ | 0xff00;

		if (flags & 1) {
			int mpos = in[0] | ((in[1] & 0xf) << 8);
			int mlen = (in[1] & 0xf0) >> 4;
			in += 2;
			mlen += 3;
			if (mlen == 18)
				mlen += *in++;

			while (mlen--) {
				*out++ = text[r++] = text[mpos++];
				mpos &= (4096 - 1);
				r &= (4096 - 1);
			}
		} else {
			unsigned char c = *in++;
			*out++ = c;
			text[r++] = c;
/*			0[out++] = text[r++] = 0[in++];*/
			r &= (4096 - 1);
		}
	}

	return r;
}

static void TVPTLG5ComposeColors3(BYTE *outp, const BYTE *upper, 
								  BYTE * const *buf, int width)
{
	BYTE pc[3];
	BYTE c[3];

	pc[0] = pc[1] = pc[2] = 0;
	for (int x = 0; x < width; x++) {
		c[0] = buf[0][x];
		c[1] = buf[1][x];
		c[2] = buf[2][x];
		c[0] += c[1]; c[2] += c[1];
		outp[0] = ((pc[0] += c[0]) + upper[0]) & 0xff;
		outp[1] = ((pc[1] += c[1]) + upper[1]) & 0xff;
		outp[2] = ((pc[2] += c[2]) + upper[2]) & 0xff;
		outp += 3;
		upper += 3;
	}
}

static void TVPTLG5ComposeColors4(BYTE *outp, const BYTE *upper, 
								  BYTE * const *buf, int width)
{
	BYTE pc[4];
	BYTE c[4];

	pc[0] = pc[1] = pc[2] = pc[3] = 0;
	for (int x = 0; x < width; x++) {
		c[0] = buf[0][x];
		c[1] = buf[1][x];
		c[2] = buf[2][x];
		c[3] = buf[3][x];
		c[0] += c[1]; c[2] += c[1];
		*(u32 *)outp = ((((pc[0] += c[0]) + upper[0]) & 0xff)      ) +
						((((pc[1] += c[1]) + upper[1]) & 0xff) <<  8) +
						((((pc[2] += c[2]) + upper[2]) & 0xff) << 16) +
						((((pc[3] += c[3]) + upper[3]) & 0xff) << 24);
		outp += 4;
		upper += 4;
	}
}

static int tlg5_process(BYTE *tlg_raw_data, DWORD tlg_size,
						BYTE **ret_actual_data, DWORD *ret_actual_data_length)
{
	tlg5_header_t *tlg5_header = (tlg5_header_t *)tlg_raw_data;
	int blockcount;
	int insize, outsize;
	BYTE *in, *out;
	BYTE text[4096];

	//outsize = ((tlg5_header->width * tlg5_header->colors + 3) & ~3) * tlg5_header->height;
	outsize = tlg5_header->width * tlg5_header->colors * tlg5_header->height;
	out = (BYTE *)malloc(outsize);
	if (!out)
		return -CUI_EMEM;

	blockcount = (int)((tlg5_header->height - 1) / tlg5_header->blockheight) + 1;
	// skip block size section
	in = (BYTE *)(tlg5_header + 1) + blockcount * 4;
	insize = tlg_size - sizeof(tlg5_header_t) - blockcount * 4;

	BYTE *outbuf[4];
	int r = 0;

	memset(text, 0, 4096);

	outbuf[0] = (BYTE *)malloc(((tlg5_header->blockheight 
			* tlg5_header->width + 10 + 3) & ~3) * tlg5_header->colors);
	if (!outbuf[0]) {
		free(out);
		return -CUI_EMEM;
	}

	for (unsigned int i = 1; i < tlg5_header->colors; i++)
		outbuf[i] = outbuf[i - 1] + ((tlg5_header->blockheight 
			* tlg5_header->width + 10 + 3) & ~3);

	BYTE *prevline = NULL;
	for (unsigned int y_blk = 0; y_blk < tlg5_header->height; y_blk += tlg5_header->blockheight) {
		for (unsigned int c = 0; c < tlg5_header->colors; c++) {
			u8 mark;
			u32 size;

			mark = *in++;
			size = *(u32 *)in;
			in += 4;

			if (!mark)
				// modified LZSS compressed data
				r = TVPTLG5DecompressSlide(outbuf[c], in, size, text, r);
			else
				// raw data
				memcpy(outbuf[c], in, size);

			in += size;
		}

		// compose colors and store
		unsigned int y_lim = y_blk + tlg5_header->blockheight;
		if (y_lim > tlg5_header->height)
			y_lim = tlg5_header->height;

		BYTE *outbufp[4];
		for (c = 0; c < tlg5_header->colors; c++)
			outbufp[c] = outbuf[c];

		for (unsigned int y = y_blk; y < y_lim; y++) {
			//BYTE *current = &out[y * ((tlg5_header->width * tlg5_header->colors + 3) & ~3)];
			BYTE *current = &out[y * tlg5_header->width * tlg5_header->colors];
			BYTE *current_org = current;

			if (prevline) {
				// not first line
				switch (tlg5_header->colors) {
				case 3:
					TVPTLG5ComposeColors3(current, prevline, outbufp, tlg5_header->width);
					outbufp[0] += tlg5_header->width; outbufp[1] += tlg5_header->width;
					outbufp[2] += tlg5_header->width;
					break;
				case 4:
					TVPTLG5ComposeColors4(current, prevline, outbufp, tlg5_header->width);
					outbufp[0] += tlg5_header->width; outbufp[1] += tlg5_header->width;
					outbufp[2] += tlg5_header->width; outbufp[3] += tlg5_header->width;
					break;
				}
			} else {
				unsigned int pr, pg, pb, pa, x;

				// first line
				switch(tlg5_header->colors) {
				case 3:
					for (pr = 0, pg = 0, pb = 0, x = 0; x < tlg5_header->width; x++) {
						int b = outbufp[0][x];
						int g = outbufp[1][x];
						int r = outbufp[2][x];

						b += g; r += g;
						*current++ = pb += b;
						*current++ = pg += g;
						*current++ = pr += r;
					}
					outbufp[0] += tlg5_header->width;
					outbufp[1] += tlg5_header->width;
					outbufp[2] += tlg5_header->width;
					break;
				case 4:
					for(pr = 0, pg = 0, pb = 0, pa = 0, x = 0; x < tlg5_header->width; x++) {
						int b = outbufp[0][x];
						int g = outbufp[1][x];
						int r = outbufp[2][x];
						int a = outbufp[3][x];

						b += g; r += g;
						*current++ = pb += b;
						*current++ = pg += g;
						*current++ = pr += r;
						*current++ = pa += a;
					}
					outbufp[0] += tlg5_header->width;
					outbufp[1] += tlg5_header->width;
					outbufp[2] += tlg5_header->width;
					outbufp[3] += tlg5_header->width;
					break;
				}
			}
			prevline = current_org;
		}
	}
	free(outbuf[0]);

	if (tlg5_header->colors == 4) {
		BYTE *b = out;
		DWORD pixels = tlg5_header->width * tlg5_header->height;
		for (i = 0; i < pixels; ++i) {
			b[0] = b[0] * b[3] / 255 + (255 - b[3]);
			b[1] = b[1] * b[3] / 255 + (255 - b[3]);
			b[2] = b[2] * b[3] / 255 + (255 - b[3]);
			b += 4;
		}
	}
	if (MyBuildBMPFile(out, outsize, NULL, 0, tlg5_header->width,
			0 - tlg5_header->height, tlg5_header->colors * 8,
			ret_actual_data, ret_actual_data_length, my_malloc)) {
		free(out);
		return -CUI_EMEM;
	}
	free(out);

	return 0;
}

static void TVPFillARGB(u32 *dest, int len, u32 value)
{
	int ___index = 0;

	len -= (8-1);

	while (___index < len) {
		dest[(___index+0)] = value;
		dest[(___index+1)] = value;
		dest[(___index+2)] = value;
		dest[(___index+3)] = value;
		dest[(___index+4)] = value;
		dest[(___index+5)] = value;
		dest[(___index+6)] = value;
		dest[(___index+7)] = value;
		___index += 8;
	}

	len += (8-1);

	while (___index < len)
		dest[___index++] = value;
}

#define TVP_TLG6_H_BLOCK_SIZE 8
#define TVP_TLG6_W_BLOCK_SIZE 8

#define TVP_TLG6_GOLOMB_N_COUNT  4
#define TVP_TLG6_LeadingZeroTable_BITS 12
#define TVP_TLG6_LeadingZeroTable_SIZE  (1 << TVP_TLG6_LeadingZeroTable_BITS)
static BYTE TVPTLG6LeadingZeroTable[TVP_TLG6_LeadingZeroTable_SIZE];
static short int TVPTLG6GolombCompressed[TVP_TLG6_GOLOMB_N_COUNT][9] = {
		{3,7,15,27,63,108,223,448,130,},
		{3,5,13,24,51,95,192,384,257,},
		{2,5,12,21,39,86,155,320,384,},
		{2,3,9,18,33,61,129,258,511,},
	/* Tuned by W.Dee, 2004/03/25 */
};

static char TVPTLG6GolombBitLengthTable
	[TVP_TLG6_GOLOMB_N_COUNT*2*128][TVP_TLG6_GOLOMB_N_COUNT] =
	{ { 0 } };

static void TVPTLG6InitGolombTable(void)
{
	int n, i, j;
	for (n = 0; n < TVP_TLG6_GOLOMB_N_COUNT; n++) {
		int a = 0;
		for (i = 0; i < 9; i++) {
			for (j = 0; j < TVPTLG6GolombCompressed[n][i]; j++)
				TVPTLG6GolombBitLengthTable[a++][n] = (char)i;
		}
		if (a != TVP_TLG6_GOLOMB_N_COUNT * 2 * 128)
			*(char *)0 = 0;   /* THIS MUST NOT BE EXECUETED! */
				/* (this is for compressed table data check) */
	}
}

static void TVPTLG6InitLeadingZeroTable(void)
{
	/* table which indicates first set bit position + 1. */
	/* this may be replaced by BSF (IA32 instrcution). */

	int i;
	for (i = 0; i < TVP_TLG6_LeadingZeroTable_SIZE; i++) {
		int cnt = 0;
		int j;
		for (j = 1; j != TVP_TLG6_LeadingZeroTable_SIZE && !(i & j);
			j <<= 1, cnt++);
		cnt++;
		if (j == TVP_TLG6_LeadingZeroTable_SIZE)
			cnt = 0;
		TVPTLG6LeadingZeroTable[i] = cnt;
	}
}

#define TVP_TLG6_FETCH_32BITS(addr) (*(u32 *)addr)

static void TVPTLG6DecodeGolombValuesForFirst(char *pixelbuf, 
											  int pixel_count, 
											  BYTE *bit_pool)
{
	/*
		decode values packed in "bit_pool".
		values are coded using golomb code.

		"ForFirst" function do dword access to pixelbuf,
		clearing with zero except for blue (least siginificant byte).
	*/

	int n = TVP_TLG6_GOLOMB_N_COUNT - 1; /* output counter */
	int a = 0; /* summary of absolute values of errors */

	int bit_pos = 1;
	BYTE zero = (*bit_pool & 1) ? 0 : 1;

	char *limit = pixelbuf + pixel_count * 4;
	while (pixelbuf < limit) {
		/* get running count */
		int count;

		{
			u32 t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
			int b = TVPTLG6LeadingZeroTable[t & (TVP_TLG6_LeadingZeroTable_SIZE - 1)];
			int bit_count = b;

			while (!b) {
				bit_count += TVP_TLG6_LeadingZeroTable_BITS;
				bit_pos += TVP_TLG6_LeadingZeroTable_BITS;
				bit_pool += bit_pos >> 3;
				bit_pos &= 7;
				t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
				b = TVPTLG6LeadingZeroTable[t & (TVP_TLG6_LeadingZeroTable_SIZE - 1)];
				bit_count += b;
			}

			bit_pos += b;
			bit_pool += bit_pos >> 3;
			bit_pos &= 7;

			bit_count--;
			count = 1 << bit_count;
			count += ((TVP_TLG6_FETCH_32BITS(bit_pool) >> (bit_pos)) & (count - 1));

			bit_pos += bit_count;
			bit_pool += bit_pos >> 3;
			bit_pos &= 7;
		}

		if (zero) {
			/* zero values */

			/* fill distination with zero */
			do { *(u32 *)pixelbuf = 0; pixelbuf += 4; } while (--count);

			zero ^= 1;
		} else {
			/* non-zero values */

			/* fill distination with glomb code */

			do {
				int k = TVPTLG6GolombBitLengthTable[a][n], v, sign;

				u32 t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
				int bit_count;
				int b;
				if (t) {
					b = TVPTLG6LeadingZeroTable[t & (TVP_TLG6_LeadingZeroTable_SIZE-1)];
					bit_count = b;
					while (!b) {
						bit_count += TVP_TLG6_LeadingZeroTable_BITS;
						bit_pos += TVP_TLG6_LeadingZeroTable_BITS;
						bit_pool += bit_pos >> 3;
						bit_pos &= 7;
						t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
						b = TVPTLG6LeadingZeroTable[t & (TVP_TLG6_LeadingZeroTable_SIZE - 1)];
						bit_count += b;
					}
					bit_count--;
				} else {
					bit_pool += 5;
					bit_count = bit_pool[-1];
					bit_pos = 0;
					t = TVP_TLG6_FETCH_32BITS(bit_pool);
					b = 0;
				}

				v = (bit_count << k) + ((t >> b) & ((1 << k) - 1));
				sign = (v & 1) - 1;
				v >>= 1;
				a += v;
				*(u32 *)pixelbuf = (unsigned char)((v ^ sign) + sign + 1);
				pixelbuf += 4;

				bit_pos += b;
				bit_pos += k;
				bit_pool += bit_pos >> 3;
				bit_pos &= 7;

				if (--n < 0) {
					a >>= 1;  
					n = TVP_TLG6_GOLOMB_N_COUNT - 1;
				}
			} while (--count);
			zero ^= 1;
		}
	}
}

static void TVPTLG6DecodeGolombValues(char *pixelbuf, 
									  int pixel_count, 
									  BYTE *bit_pool)
{
	/*
		decode values packed in "bit_pool".
		values are coded using golomb code.
	*/

	int n = TVP_TLG6_GOLOMB_N_COUNT - 1; /* output counter */
	int a = 0; /* summary of absolute values of errors */

	int bit_pos = 1;
	BYTE zero = (*bit_pool & 1) ? 0 : 1;

	char *limit = pixelbuf + pixel_count * 4;

	while (pixelbuf < limit) {
		/* get running count */
		int count;

		{
			u32 t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
			int b = TVPTLG6LeadingZeroTable[t & (TVP_TLG6_LeadingZeroTable_SIZE-1)];
			int bit_count = b;
			while (!b) {
				bit_count += TVP_TLG6_LeadingZeroTable_BITS;
				bit_pos += TVP_TLG6_LeadingZeroTable_BITS;
				bit_pool += bit_pos >> 3;
				bit_pos &= 7;
				t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
				b = TVPTLG6LeadingZeroTable[t & (TVP_TLG6_LeadingZeroTable_SIZE - 1)];
				bit_count += b;
			}

			bit_pos += b;
			bit_pool += bit_pos >> 3;
			bit_pos &= 7;

			bit_count--;
			count = 1 << bit_count;
			count += ((TVP_TLG6_FETCH_32BITS(bit_pool) >> (bit_pos)) & (count - 1));

			bit_pos += bit_count;
			bit_pool += bit_pos >> 3;
			bit_pos &= 7;
		}

		if (zero) {
			/* zero values */

			/* fill distination with zero */
			do { *pixelbuf = 0; pixelbuf+=4; } while(--count);

			zero ^= 1;
		} else {
			/* non-zero values */

			/* fill distination with glomb code */

			do {
				int k = TVPTLG6GolombBitLengthTable[a][n], v, sign;

				u32 t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
				int bit_count;
				int b;
				if (t) {
					b = TVPTLG6LeadingZeroTable[t & (TVP_TLG6_LeadingZeroTable_SIZE - 1)];
					bit_count = b;
					while (!b) {
						bit_count += TVP_TLG6_LeadingZeroTable_BITS;
						bit_pos += TVP_TLG6_LeadingZeroTable_BITS;
						bit_pool += bit_pos >> 3;
						bit_pos &= 7;
						t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
						b = TVPTLG6LeadingZeroTable[t & (TVP_TLG6_LeadingZeroTable_SIZE - 1)];
						bit_count += b;
					}
					bit_count--;
				} else {
					bit_pool += 5;
					bit_count = bit_pool[-1];
					bit_pos = 0;
					t = TVP_TLG6_FETCH_32BITS(bit_pool);
					b = 0;
				}

				v = (bit_count << k) + ((t >> b) & ((1 <<k ) - 1));
				sign = (v & 1) - 1;
				v >>= 1;
				a += v;
				*pixelbuf = (char)((v ^ sign) + sign + 1);
				pixelbuf += 4;

				bit_pos += b;
				bit_pos += k;
				bit_pool += bit_pos >> 3;
				bit_pos &= 7;

				if (--n < 0) {
					a >>= 1;
					n = TVP_TLG6_GOLOMB_N_COUNT - 1;
				}
			} while (--count);
			zero ^= 1;
		}
	}
}

static u32 make_gt_mask(u32 a, u32 b)
{
	u32 tmp2 = ~b;
	u32 tmp = ((a & tmp2) + (((a ^ tmp2) >> 1) & 0x7f7f7f7f)) & 0x80808080;
	tmp = ((tmp >> 7) + 0x7f7f7f7f) ^ 0x7f7f7f7f;
	return tmp;
}

static u32 packed_bytes_add(u32 a, u32 b)
{
	u32 tmp = (((a & b) << 1) + ((a ^ b) & 0xfefefefe) ) & 0x01010100;
	return a + b - tmp;
}

static u32 med2(u32 a, u32 b, u32 c)
{
	/* do Median Edge Detector   thx, Mr. sugi  at    kirikiri.info */
	u32 aa_gt_bb = make_gt_mask(a, b);
	u32 a_xor_b_and_aa_gt_bb = ((a ^ b) & aa_gt_bb);
	u32 aa = a_xor_b_and_aa_gt_bb ^ a;
	u32 bb = a_xor_b_and_aa_gt_bb ^ b;
	u32 n = make_gt_mask(c, bb);
	u32 nn = make_gt_mask(aa, c);
	u32 m = ~(n | nn);
	return (n & aa) | (nn & bb) | ((bb & m) - (c & m) + (aa & m));
}

static u32 med(u32 a, u32 b, u32 c, u32 v)
{
	return packed_bytes_add(med2(a, b, c), v);
}

#define TLG6_AVG_PACKED(x, y) ((((x) & (y)) + ((((x) ^ (y)) & 0xfefefefe) >> 1)) +\
			(((x)^(y))&0x01010101))

static u32 avg(u32 a, u32 b, u32 c, u32 v)
{
	return packed_bytes_add(TLG6_AVG_PACKED(a, b), v);
}

#define TVP_TLG6_DO_CHROMA_DECODE_PROTO(B, G, R, A, POST_INCREMENT) do \
			{ \
				u32 u = *prevline; \
				p = med(p, u, up, \
					(0xff0000 & ((R) << 16)) + (0xff00 & ((G) << 8)) + (0xff & (B)) + ((A) << 24) ); \
				up = u; \
				*curline = p; \
				curline++; \
				prevline++; \
				POST_INCREMENT \
			} while (--w);
#define TVP_TLG6_DO_CHROMA_DECODE_PROTO2(B, G, R, A, POST_INCREMENT) do \
			{ \
				u32 u = *prevline; \
				p = avg(p, u, up, \
					(0xff0000 & ((R) << 16)) + (0xff00 & ((G) << 8)) + (0xff & (B)) + ((A) << 24) ); \
				up = u; \
				*curline = p; \
				curline ++; \
				prevline ++; \
				POST_INCREMENT \
			} while (--w);

#define TVP_TLG6_DO_CHROMA_DECODE(N, R, G, B) case (N << 1): \
	TVP_TLG6_DO_CHROMA_DECODE_PROTO(R, G, B, IA, {in += step;}) break; \
	case (N << 1) + 1: \
	TVP_TLG6_DO_CHROMA_DECODE_PROTO2(R, G, B, IA, {in += step;}) break;

static void TVPTLG6DecodeLineGeneric(u32 *prevline, u32 *curline, 
									 int width, int start_block, 
									 int block_limit, BYTE *filtertypes, 
									 int skipblockbytes, u32 *in, 
									 u32 initialp, int oddskip, int dir)
{
	/*
		chroma/luminosity decoding
		(this does reordering, color correlation filter, MED/AVG  at a time)
	*/
	u32 p, up;
	int step, i;

	if (start_block) {
		prevline += start_block * TVP_TLG6_W_BLOCK_SIZE;
		curline  += start_block * TVP_TLG6_W_BLOCK_SIZE;
		p  = curline[-1];
		up = prevline[-1];
	} else {
		p = up = initialp;
	}

	in += skipblockbytes * start_block;
	step = (dir & 1) ? 1 : -1;

	for (i = start_block; i < block_limit; i++) {
		int w = width - i * TVP_TLG6_W_BLOCK_SIZE, ww;
		if (w > TVP_TLG6_W_BLOCK_SIZE) w = TVP_TLG6_W_BLOCK_SIZE;
		ww = w;
		if (step == -1) in += ww-1;
		if (i & 1) in += oddskip * ww;
		switch (filtertypes[i])	{
#define IA	(char)((*in >> 24) & 0xff)
#define IR	(char)((*in >> 16) & 0xff)
#define IG  (char)((*in >> 8 ) & 0xff)
#define IB  (char)((*in      ) & 0xff)
			TVP_TLG6_DO_CHROMA_DECODE( 0, IB, IG, IR); 
			TVP_TLG6_DO_CHROMA_DECODE( 1, IB+IG, IG, IR+IG); 
			TVP_TLG6_DO_CHROMA_DECODE( 2, IB, IG+IB, IR+IB+IG); 
			TVP_TLG6_DO_CHROMA_DECODE( 3, IB+IR+IG, IG+IR, IR); 
			TVP_TLG6_DO_CHROMA_DECODE( 4, IB+IR, IG+IB+IR, IR+IB+IR+IG); 
			TVP_TLG6_DO_CHROMA_DECODE( 5, IB+IR, IG+IB+IR, IR); 
			TVP_TLG6_DO_CHROMA_DECODE( 6, IB+IG, IG, IR); 
			TVP_TLG6_DO_CHROMA_DECODE( 7, IB, IG+IB, IR); 
			TVP_TLG6_DO_CHROMA_DECODE( 8, IB, IG, IR+IG); 
			TVP_TLG6_DO_CHROMA_DECODE( 9, IB+IG+IR+IB, IG+IR+IB, IR+IB); 
			TVP_TLG6_DO_CHROMA_DECODE(10, IB+IR, IG+IR, IR); 
			TVP_TLG6_DO_CHROMA_DECODE(11, IB, IG+IB, IR+IB); 
			TVP_TLG6_DO_CHROMA_DECODE(12, IB, IG+IR+IB, IR+IB); 
			TVP_TLG6_DO_CHROMA_DECODE(13, IB+IG, IG+IR+IB+IG, IR+IB+IG); 
			TVP_TLG6_DO_CHROMA_DECODE(14, IB+IG+IR, IG+IR, IR+IB+IG+IR); 
			TVP_TLG6_DO_CHROMA_DECODE(15, IB, IG+(IB<<1), IR+(IB<<1));
		default: 
			return;
		}
		if (step == 1)
			in += skipblockbytes - ww;
		else
			in += skipblockbytes + 1;
		if (i & 1) in -= oddskip * ww;
#undef IR
#undef IG
#undef IB
	}
}

static void TVPTLG6DecodeLine(u32 *prevline, u32 *curline, 
							  int width, int block_count, 
							  BYTE *filtertypes, int skipblockbytes, 
							  u32 *in, u32 initialp, int oddskip, 
							  int dir)
{
	TVPTLG6DecodeLineGeneric(prevline, curline, width, 0, block_count,
		filtertypes, skipblockbytes, in, initialp, oddskip, dir);
}

static int tlg6_process(BYTE *tlg_raw_data, DWORD tlg_size,
						BYTE **ret_actual_data, DWORD *ret_actual_data_length)
{
	tlg6_header_t *tlg6_header = (tlg6_header_t *)tlg_raw_data;
	int outsize;
	BYTE *out;

	outsize = tlg6_header->width * 4 * tlg6_header->height;
	out = (BYTE *)malloc(outsize);
	if (!out)
		return -CUI_EMEM;

	int x_block_count = (int)((tlg6_header->width - 1) / TVP_TLG6_W_BLOCK_SIZE) + 1;
	int y_block_count = (int)((tlg6_header->height - 1) / TVP_TLG6_H_BLOCK_SIZE) + 1;
	int main_count = tlg6_header->width / TVP_TLG6_W_BLOCK_SIZE;
	int fraction = tlg6_header->width -  main_count * TVP_TLG6_W_BLOCK_SIZE;

	BYTE *bit_pool = (BYTE *)malloc((tlg6_header->max_bit_length / 8 + 5 + 3) & ~3);
	u32 *pixelbuf = (u32 *)malloc((4 * tlg6_header->width * TVP_TLG6_H_BLOCK_SIZE + 1 + 3) & ~3);
	BYTE *filter_types = (BYTE *)malloc((x_block_count * y_block_count + 3) & ~3);
	u32 *zeroline = (u32 *)malloc(tlg6_header->width * 4);
	if (!bit_pool || !pixelbuf || !filter_types || !zeroline) {
		free(out);
		return -CUI_EMEM;
	}

	BYTE LZSS_text[4096];

	// initialize zero line (virtual y=-1 line)
	TVPFillARGB(zeroline, tlg6_header->width, 
		tlg6_header->colors == 3 ? 0xff000000 : 0x00000000);
	// 0xff000000 for colors=3 makes alpha value opaque

	// initialize LZSS text (used by chroma filter type codes)
	u32 *p = (u32 *)LZSS_text;
	for (u32 i = 0; i < 32 * 0x01010101; i += 0x01010101) {
		for (u32 j = 0; j < 16 * 0x01010101; j += 0x01010101) {
			p[0] = i, p[1] = j, p += 2;
		}
	}

	// read chroma filter types.
	// chroma filter types are compressed via LZSS as used by TLG5.
	int inbuf_size = tlg6_header->filter_length;
	BYTE *inbuf = (BYTE *)(tlg6_header + 1);
	TVPTLG5DecompressSlide(filter_types, inbuf, inbuf_size,	LZSS_text, 0);

	BYTE *in_p = inbuf + inbuf_size;
	// for each horizontal block group ...
	u32 *prevline = zeroline;
	for (int y = 0; y < (int)tlg6_header->height; y += TVP_TLG6_H_BLOCK_SIZE) {
		int ylim = y + TVP_TLG6_H_BLOCK_SIZE;
		if (ylim >= (int)tlg6_header->height)
			ylim = tlg6_header->height;

		int pixel_count = (ylim - y) * tlg6_header->width;

		// decode values
		for (int c = 0; c < tlg6_header->colors; c++) {
			// read bit length
			int bit_length = *(u32 *)in_p;
			in_p += 4;

			// get compress method
			int method = (bit_length >> 30) & 3;
			bit_length &= 0x3fffffff;

			// compute byte length
			int byte_length = bit_length / 8;
			if (bit_length % 8)
				byte_length++;

			// read source from input
			memcpy(bit_pool, in_p, byte_length);
			in_p += byte_length;

			// decode values
			// two most significant bits of bitlength are
			// entropy coding method;
			// 00 means Golomb method,
			// 01 means Gamma method (not yet suppoted),
			// 10 means modified LZSS method (not yet supported),
			// 11 means raw (uncompressed) data (not yet supported).

			switch (method) {
			case 0:
				if (c == 0 && tlg6_header->colors != 1)
					TVPTLG6DecodeGolombValuesForFirst((char *)pixelbuf,
						pixel_count, bit_pool);
				else
					TVPTLG6DecodeGolombValues((char *)pixelbuf + c,
						pixel_count, bit_pool);
				break;
			default:
				if (byte_length & 3)
					free(bit_pool);
				free(zeroline);
				free(filter_types);
				free(pixelbuf);
				free(bit_pool);
				free(out);
				return -CUI_EMATCH;
			}
		}

		// for each line
		unsigned char *ft =
			filter_types + (y / TVP_TLG6_H_BLOCK_SIZE) * x_block_count;
		int skipbytes = (ylim - y) * TVP_TLG6_W_BLOCK_SIZE;

		for (int yy = y; yy < ylim; yy++) {
			u32 *curline = (u32 *)&out[yy * tlg6_header->width * 4];

			int dir = (yy & 1) ^ 1;
			int oddskip = ((ylim - yy - 1) - (yy - y));
			if (main_count) {
				int start =
					((tlg6_header->width < TVP_TLG6_W_BLOCK_SIZE) ? tlg6_header->width : 
						TVP_TLG6_W_BLOCK_SIZE) * (yy - y);
				TVPTLG6DecodeLine(
					prevline,
					curline,
					tlg6_header->width,
					main_count,
					ft,
					skipbytes,
					pixelbuf + start, tlg6_header->colors == 3 ? 0xff000000 : 0, 
					oddskip, dir);
			}

			if (main_count != x_block_count) {
				int ww = fraction;
				if (ww > TVP_TLG6_W_BLOCK_SIZE)
					ww = TVP_TLG6_W_BLOCK_SIZE;
				int start = ww * (yy - y);
				TVPTLG6DecodeLineGeneric(
					prevline,
					curline,
					tlg6_header->width,
					main_count,
					x_block_count,
					ft,
					skipbytes,
					pixelbuf + start, 
					tlg6_header->colors == 3 ? 0xff000000 : 0, 						
					oddskip, dir);
			}
			prevline = curline;
		}
	}
	free(zeroline);
	free(filter_types);
	free(pixelbuf);
	free(bit_pool);

	BYTE *b = out;
	DWORD pixels = tlg6_header->width * tlg6_header->height;
	for (i = 0; i < pixels; ++i) {
		b[0] = b[0] * b[3] / 255 + (255 - b[3]);
		b[1] = b[1] * b[3] / 255 + (255 - b[3]);
		b[2] = b[2] * b[3] / 255 + (255 - b[3]);
		b += 4;
	}

	if (MyBuildBMPFile(out, outsize, NULL, 0, tlg6_header->width,
			0 - tlg6_header->height, 32, ret_actual_data, 
				ret_actual_data_length, my_malloc)) {
		free(out);
		return -CUI_EMEM;
	}
	free(out);

	return 0;
}

/* 索引段chunk搜索 */
static xp3_chunk_t *kirikiri2_xp3_find_chunk(BYTE *data, 
											 unsigned int data_len, s8 *magic)
{
	xp3_chunk_t *res = NULL;
	unsigned int pos = 0;
	
	while (pos < data_len) {
		xp3_chunk_t *chunk = (xp3_chunk_t *)(data + pos);
		if (!memcmp(chunk->magic, magic, 4)) {
			res = chunk;
			break;
		}
		pos += sizeof(xp3_chunk_t) + chunk->length_lo;		
	}
	
	return res;
}

/********************* xp3 *********************/

static int kirikiri2_xp3_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir);

static int kirikiri2_xp3_match(struct package *pkg)
{
	s8 magic[11];
	u32 index_offset_lo, index_offset_hi;
	u8 index_flag;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->read(pkg, magic, 11)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;	
	}
	
	if (memcmp(magic, "XP3\r\n \n\x1a\x8b\x67\x01", 11)) {		
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}
	xp3_start_offset = 0;

	if (pkg->pio->read(pkg, &index_offset_lo, 4)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;	
	}
		
	if (pkg->pio->read(pkg, &index_offset_hi, 4)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;	
	}
		
	u64 long_offset = (u64)index_offset_lo | ((u64)index_offset_hi) << 32;
	long_offset += xp3_start_offset;

	if (pkg->pio->seek64(pkg, (u32)long_offset, 
			(u32)(long_offset >> 32), IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_ESEEK;
	}
		
	if (pkg->pio->read(pkg, &index_flag, 1)) {
		pkg->pio->close(pkg);
		return -CUI_ESEEK;	
	}
		
	u32 index_size;					
	if (pkg->pio->read(pkg, &index_size, 4)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;	
	}

	if (index_flag != 0x80 && index_offset_lo != 0x17) {	// is not a cushion header
		if (!index_size) {		
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}
	}

	struct package_directory pkg_dir;
	memset(&pkg_dir, 0, sizeof(pkg_dir));
	if (kirikiri2_xp3_extract_directory(pkg, &pkg_dir)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}
	free(pkg_dir.directory);

	return 0;	
}

static int kirikiri2_xp3_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{
	BYTE *indexdata;
	u32 index_offset_lo, index_offset_hi;
	u64 long_offset;
	u8 index_flag;
	xp3_file_chunk_t *file_chunk;

	long_offset = xp3_start_offset + 11;
	if (pkg->pio->seek64(pkg, (u32)long_offset, 
			(u32)(long_offset >> 32), IO_SEEK_SET))
		return -CUI_ESEEK;

	unsigned int index_entries = 0;
	while (1) {
		if (pkg->pio->read(pkg, &index_offset_lo, 4))
			return -CUI_EREAD;
		
		if (pkg->pio->read(pkg, &index_offset_hi, 4))
			return -CUI_EREAD;

		long_offset = (u64)index_offset_lo | ((u64)index_offset_hi) << 32;
		long_offset += xp3_start_offset;

		if (pkg->pio->seek64(pkg, (u32)long_offset, 
			(u32)(long_offset >> 32), IO_SEEK_SET))
			return -CUI_ESEEK;
	
		if (pkg->pio->read(pkg, &index_flag, 1))
			return -CUI_EREAD;
		
		unsigned int index_size;

		if ((index_flag & TVP_XP3_INDEX_ENCODE_METHOD_MASK) == TVP_XP3_INDEX_ENCODE_ZLIB) {
			// compressed index
			u64 compressed_size;
			u64 r_index_size;
					
			if (pkg->pio->read(pkg, &compressed_size, 8))
				return -CUI_EREAD;

			if (pkg->pio->read(pkg, &r_index_size, 8))
				return -CUI_EREAD;

			// too large to handle, or corrupted
			if ((unsigned int)compressed_size != compressed_size || (unsigned int)r_index_size != r_index_size)
				return -CUI_EMATCH;
					
			index_size = (unsigned int)r_index_size;
			indexdata = (BYTE *)malloc(index_size);
			if(!indexdata)
				return -CUI_EMEM;
					
			BYTE *compressed = (BYTE *)malloc((unsigned int)compressed_size);
			if (!compressed) {
				free(indexdata);
				return -CUI_EMEM;
			}

			if (pkg->pio->read(pkg, compressed, (unsigned int)compressed_size)) {
				free(compressed);
				free(indexdata);
				return -CUI_EREAD;
			}

			unsigned long destlen = (unsigned long)index_size;

			int result = uncompress(  /* uncompress from zlib */
				(unsigned char *)indexdata,
					&destlen, (unsigned char *)compressed,
					(unsigned long)compressed_size);
			free(compressed);
			if (result != Z_OK ||
				destlen != (unsigned long)index_size) {
					free(indexdata);
					return -CUI_EUNCOMPR;
			}
		} else if ((index_flag & TVP_XP3_INDEX_ENCODE_METHOD_MASK) == TVP_XP3_INDEX_ENCODE_RAW) {
			// uncompressed index
			u64 r_index_size;
			
			if (pkg->pio->read(pkg, &r_index_size, 8))
				return -CUI_EREAD;

			// too large to handle or corrupted
			if ((unsigned int)r_index_size != r_index_size)
				return -CUI_EMATCH;
			index_size = (unsigned int)r_index_size;
			indexdata = (BYTE *)malloc(index_size);
			if (!indexdata)
				return -CUI_EMEM;
	
			if (pkg->pio->read(pkg, indexdata, index_size)) {
				free(indexdata);
				return -CUI_EREAD;
			}
		} else		// unknown encode method
			return -CUI_EMATCH;

		unsigned int offset = 0;
		while (1) {
			file_chunk = (xp3_file_chunk_t *)kirikiri2_xp3_find_chunk(indexdata + offset, 
				index_size - offset, "File");
			if (!file_chunk)
				break;
			index_entries++;
			offset += sizeof(xp3_file_chunk_t) + file_chunk->length_lo;
		}
		free(indexdata);

		if (!(index_flag & TVP_XP3_INDEX_CONTINUE))
			break;
	}

	long_offset = (u64)xp3_start_offset + 11;
	if (pkg->pio->seek64(pkg, (u32)long_offset, 
			(u32)(long_offset >> 32), IO_SEEK_SET))
		return -CUI_ESEEK;

	DWORD index_length = index_entries * sizeof(my_xp3_entry_t);
	my_xp3_entry_t *my_xp3_entry_buf = (my_xp3_entry_t *)malloc(index_length);
	if (!my_xp3_entry_buf)
		return -CUI_EMEM;
	memset(my_xp3_entry_buf, 0, index_length);

	int err = 0;
	DWORD i = 0;
	while (1) {
		if (pkg->pio->read(pkg, &index_offset_lo, 4)) {
			err = -CUI_EREAD;
			break;
		}
		
		if (pkg->pio->read(pkg, &index_offset_hi, 4)) {
			err = -CUI_EREAD;
			break;
		}

		long_offset = (u64)index_offset_lo | ((u64)index_offset_hi) << 32;
		long_offset += xp3_start_offset;

		if (pkg->pio->seek64(pkg, (u32)long_offset, 
				(u32)(long_offset >> 32), IO_SEEK_SET)) {
			err = -CUI_ESEEK;
			break;
		}

		if (pkg->pio->read(pkg, &index_flag, 1)) {
			err = -CUI_EREAD;
			break;
		}
		
		unsigned int index_size;
		
		if ((index_flag & TVP_XP3_INDEX_ENCODE_METHOD_MASK) == TVP_XP3_INDEX_ENCODE_ZLIB) {
			// compressed index
			u64 compressed_size ;
			u64 r_index_size;
					
			if (pkg->pio->read(pkg, &compressed_size, 8)) {
				err = -CUI_EREAD;
				break;
			}

			if (pkg->pio->read(pkg, &r_index_size, 8)) {
				err = -CUI_EREAD;
				break;
			}
					
			index_size = (unsigned int)r_index_size;
			indexdata = (BYTE *)malloc(index_size);
			if(!indexdata) {
				err = -CUI_EMEM;
				break;
			}
					
			BYTE *compressed = (BYTE *)malloc((unsigned int)compressed_size);
			if (!compressed) {
				free(indexdata);
				err = -CUI_EMEM;
				break;
			}

			if (pkg->pio->read(pkg, compressed, (unsigned int)compressed_size)) {
				free(compressed);
				free(indexdata);
				err = -CUI_EMEM;
				break;
			}

			unsigned long destlen = (unsigned long)index_size;

			int result = uncompress(  /* uncompress from zlib */
				(unsigned char *)indexdata,
					&destlen, (unsigned char *)compressed,
					(unsigned long)compressed_size);
			free(compressed);
			if (result != Z_OK ||
				destlen != (unsigned long)index_size) {
				free(indexdata);
				err = -CUI_EUNCOMPR;
				break;
			}
		} else if ((index_flag & TVP_XP3_INDEX_ENCODE_METHOD_MASK) == TVP_XP3_INDEX_ENCODE_RAW) {
			// uncompressed index
			u64 r_index_size;
					
			if (pkg->pio->read(pkg, &r_index_size, 8)) {
				err = -CUI_EREAD;
				break;
			}
			// too large to handle or corrupted
			if ((unsigned int)r_index_size != r_index_size) {
				err = -CUI_EMATCH;
				break;
			}
			index_size = (unsigned int)r_index_size;
			indexdata = (BYTE *)malloc(index_size);
			if (!indexdata) {
				err = -CUI_EMEM;
				break;
			}
			
			if (pkg->pio->read(pkg, indexdata, index_size)) {
				free(indexdata);
				err = -CUI_EREAD;
				break;
			}
		}

		unsigned int offset = 0;
		while (1) {
			my_xp3_entry_t *my_xp3_entry = &my_xp3_entry_buf[i];
			xp3_info_chunk_t *info_chunk;
			xp3_segm_chunk_t *segm_chunk;
			xp3_adlr_chunk_t *adlr_chunk;
			
			file_chunk = (xp3_file_chunk_t *)kirikiri2_xp3_find_chunk(indexdata + offset, 
				index_size - offset, "File");
			if (!file_chunk)
				break;
			offset += sizeof(xp3_file_chunk_t);

			info_chunk = (xp3_info_chunk_t *)kirikiri2_xp3_find_chunk(indexdata + offset, 
				index_size - offset, "info");
			if (!info_chunk) {
				err = -CUI_EMATCH;
				break;
			}

			my_xp3_entry->name = (WCHAR *)malloc((info_chunk->name_length + 1) * 2);
			if (!my_xp3_entry->name) {
				err = -CUI_EMEM;
				break;
			}
			wcsncpy(my_xp3_entry->name, (WCHAR *)(info_chunk + 1), info_chunk->name_length);
			my_xp3_entry->name[info_chunk->name_length] = 0;
			my_xp3_entry->name_length = info_chunk->name_length;
			my_xp3_entry->comprlen = info_chunk->pkg_length_lo;
			my_xp3_entry->uncomprlen = info_chunk->orig_length_lo;
			my_xp3_entry->flags = info_chunk->flags;

			segm_chunk = (xp3_segm_chunk_t *)kirikiri2_xp3_find_chunk(indexdata + offset, 
				index_size - offset, "segm");
			if (!segm_chunk) {
				free(my_xp3_entry->name);
				err = -CUI_EMATCH;
				break;
			}

			my_xp3_entry->segm_number = segm_chunk->length_lo / sizeof(xp3_segm_entry_t);
			my_xp3_entry->segm = (xp3_segm_entry_t *)malloc(segm_chunk->length_lo);
			if (!my_xp3_entry->segm) {
				free(my_xp3_entry->name);
				err = -CUI_EMEM;
				break;
			}
			memcpy(my_xp3_entry->segm, segm_chunk + 1, segm_chunk->length_lo);

			adlr_chunk = (xp3_adlr_chunk_t *)kirikiri2_xp3_find_chunk(indexdata + offset, 
				index_size - offset, "adlr");
			if (!adlr_chunk) {
				free(my_xp3_entry->segm);
				free(my_xp3_entry->name);
				err = -CUI_EMATCH;
				break;
			}
			my_xp3_entry->hash = adlr_chunk->hash;
			
			// protection warning
			if (((my_xp3_entry->segm->offset_lo == sizeof(xp3_header_t) + 0x30) || (my_xp3_entry->segm->offset_lo == sizeof(xp3_header2_t) + 0x30)) 
					&& (my_xp3_entry->comprlen == my_xp3_entry->uncomprlen)
					&& (my_xp3_entry->uncomprlen == 157) && !my_xp3_entry->flags) {
				BYTE protection_png[157];

				long_offset = (u64)my_xp3_entry->segm->offset_lo | ((u64)my_xp3_entry->segm->offset_hi) << 32;
				long_offset += xp3_start_offset;
				if (pkg->pio->seek64(pkg, (u32)long_offset, 
						(u32)(long_offset >> 32), IO_SEEK_SET)) {
					free(my_xp3_entry->segm);
					free(my_xp3_entry->name);
					err = -CUI_ESEEK;
					break;
				}

				if (pkg->pio->read(pkg, protection_png, 157)) {
					free(my_xp3_entry->segm);
					free(my_xp3_entry->name);
					err = -CUI_EREAD;
					break;
				}

				if (my_xp3_entry->hash != CheckSum(dummy_png + 0x30, 157)) {
					free(my_xp3_entry->segm);
					free(my_xp3_entry->name);
					err = -CUI_EMATCH;
					break;
				}

				wcscpy(my_xp3_entry->name, L"ProtectionWarning.txt");
				my_xp3_entry->name_length = wcslen(L"ProtectionWarning.txt");					
			} else if ((my_xp3_entry->segm->offset_lo == sizeof(xp3_header_t)) && (my_xp3_entry->comprlen == my_xp3_entry->uncomprlen)
					&& (my_xp3_entry->uncomprlen == 156) && !my_xp3_entry->flags) {	// old version
				BYTE protection_warning[156];

				long_offset = (u64)my_xp3_entry->segm->offset_lo | ((u64)my_xp3_entry->segm->offset_hi) << 32;
				long_offset += xp3_start_offset;

				if (pkg->pio->seek64(pkg, (u32)long_offset, 
						(u32)(long_offset >> 32), IO_SEEK_SET)) {
					free(my_xp3_entry->segm);
					free(my_xp3_entry->name);
					err = -CUI_ESEEK;
					break;
				}

				if (pkg->pio->read(pkg, protection_warning, 156)) {
					free(my_xp3_entry->segm);
					free(my_xp3_entry->name);
					err = -CUI_EREAD;
					break;
				}

				if (my_xp3_entry->hash != CheckSum(protection_warning, 156)) {		
					free(my_xp3_entry->segm);
					free(my_xp3_entry->name);
					err = -CUI_EMATCH;
					break;
				}

				wcscpy(my_xp3_entry->name, L"ProtectionWarning.txt");
				my_xp3_entry->name_length = wcslen(L"ProtectionWarning.txt");					
			}// else if (info_chunk->flags & TVP_XP3_FILE_PROTECTED) {
			//	wcprintf(my_xp3_entry->name);
			//	printf(": Specified storage had been protected!\n");
			//}

			offset += file_chunk->length_lo;
			i++;
		}
		free(indexdata);
		if (err) 
			break;

		if (!(index_flag & TVP_XP3_INDEX_CONTINUE))
			break;
	}
	if (err) {		
		for (int j = i - 1; j >= 0; j--) {
			free(my_xp3_entry_buf[j].segm);
			free(my_xp3_entry_buf[j].name);
		}
		free(my_xp3_entry_buf);
		return err;
	}

	if (!index_entries) {
		free(my_xp3_entry_buf);
		return -CUI_EMATCH;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->index_entry_length = sizeof(my_xp3_entry_t);
	pkg_dir->directory = my_xp3_entry_buf;
	pkg_dir->directory_length = index_length;
	pkg_dir->flags |= PKG_DIR_FLAG_SKIP0;

	return 0;
}

static int kirikiri2_xp3_parse_resource_info(struct package *pkg,
											 struct package_resource *pkg_res)
{
	my_xp3_entry_t *my_xp3_entry;

	my_xp3_entry = (my_xp3_entry_t *)pkg_res->actual_index_entry;	
	wcsncpy((WCHAR *)pkg_res->name, my_xp3_entry->name, my_xp3_entry->name_length);	
	pkg_res->name_length = my_xp3_entry->name_length;
	pkg_res->raw_data_length = my_xp3_entry->comprlen;
	pkg_res->actual_data_length = my_xp3_entry->uncomprlen;
	pkg_res->offset = my_xp3_entry->segm->offset_lo;
	pkg_res->flags |= PKG_RES_FLAG_UNICODE;

	return 0;
}

static int kirikiri2_xp3_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{	
	my_xp3_entry_t *my_xp3_entry;	
	BYTE *compr, *uncompr;

	my_xp3_entry = (my_xp3_entry_t *)pkg_res->actual_index_entry;
	compr = (BYTE *)VirtualAlloc(NULL, (DWORD)my_xp3_entry->comprlen, 
		MEM_COMMIT, PAGE_READWRITE);
	if (!compr)
		return -CUI_EMEM;

	if (my_xp3_entry->uncomprlen) {
		uncompr = (BYTE *)VirtualAlloc(NULL, (DWORD)my_xp3_entry->uncomprlen, 
			MEM_COMMIT, PAGE_READWRITE);
		if (!uncompr) {
			VirtualFree(compr, 0, MEM_RELEASE);
			return -CUI_EMEM;
		}
	} else
		uncompr = NULL;

	DWORD compr_offset = 0;
	DWORD uncompr_offset = 0;
	int err = 0;
	for (unsigned int i = 0; i < my_xp3_entry->segm_number; ++i) {
		xp3_segm_entry_t *segm = &my_xp3_entry->segm[i];

		u64 long_offset = (u64)segm->offset_lo | ((u64)segm->offset_hi) << 32;
		long_offset += xp3_start_offset;

		if (pkg->pio->readvec64(pkg, compr + compr_offset, segm->pkg_length_lo, 0,
				(u32)long_offset, (u32)(long_offset >> 32), 
				IO_SEEK_SET)) {
			err = -CUI_EREADVEC;
			break;
		}

//	printf(": total %x / %x, seg(flag %x)[%d / %d]: %x / %x @ %x\n", 
//		my_xp3_entry->comprlen, my_xp3_entry->uncomprlen,
//	   segm->flags, i, my_xp3_entry->segm_number, segm->pkg_length_lo, segm->orig_length_lo, segm->offset_lo);

		if ((segm->flags & TVP_XP3_SEGM_ENCODE_METHOD_MASK) == TVP_XP3_SEGM_ENCODE_RAW)
			memcpy(uncompr + uncompr_offset, compr + compr_offset, segm->pkg_length_lo);
		else if ((segm->flags & TVP_XP3_SEGM_ENCODE_METHOD_MASK) == TVP_XP3_SEGM_ENCODE_ZLIB) {
			if (uncompr && segm->orig_length_lo) {
				DWORD act_uncomprlen = segm->orig_length_lo;
				if (uncompress(uncompr + uncompr_offset, &act_uncomprlen,
						compr + compr_offset, segm->pkg_length_lo) != Z_OK) {
					err = -CUI_EUNCOMPR;
					break;
				}
				if (act_uncomprlen != segm->orig_length_lo) {
					err = -CUI_EUNCOMPR;
					break;
				}
			}
		} else {
			err = -CUI_EMATCH;
			break;
		}

		xp3filter_decode((WCHAR *)pkg_res->name, uncompr, 
			segm->orig_length_lo, uncompr_offset, 
			my_xp3_entry->uncomprlen, my_xp3_entry->hash);

		compr_offset += segm->pkg_length_lo;
		uncompr_offset += segm->orig_length_lo;		
	}	
	if (i != my_xp3_entry->segm_number) {
		VirtualFree(uncompr, 0, MEM_RELEASE);
		VirtualFree(compr, 0, MEM_RELEASE);
		return err;
	}
	VirtualFree(compr, 0, MEM_RELEASE);

	DWORD act_crc = CheckSum(uncompr, my_xp3_entry->uncomprlen);
	if (act_crc != my_xp3_entry->hash) {
		locale_app_printf(kirikiri2_locale_id, 0,
			(WCHAR *)pkg_res->name, act_crc, my_xp3_entry->hash);
//		TCHAR yes_or_no[3];
//		wscanf(yes_or_no, SOT(yes_or_no));
//		if (yes_or_no[0] != _T('y')) {
//			VirtualFree(uncompr, 0, MEM_RELEASE);
//			return -CUI_EMATCH;	
//		}
	}

	xp3filter_post_decode((WCHAR *)pkg_res->name, uncompr, 
		my_xp3_entry->uncomprlen, my_xp3_entry->hash);

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->actual_data = uncompr;
		pkg_res->actual_data_length = my_xp3_entry->uncomprlen;
		return 0;
	}

	if (!uncompr || !my_xp3_entry->uncomprlen)
		goto skip_dec;

	if (wcsstr((WCHAR *)pkg_res->name, L".txt") || wcsstr((WCHAR *)pkg_res->name, L".ks") 
			|| wcsstr((WCHAR *)pkg_res->name, L".tjs") || wcsstr((WCHAR *)pkg_res->name, L".ma") 
			|| wcsstr((WCHAR *)pkg_res->name, L".asd")) {
		if (uncompr[0] == 0xfe && uncompr[1] == 0xfe) {
			u8 CryptMode = uncompr[2];

			if (CryptMode != 0 && CryptMode != 1 && CryptMode != 2) {
				VirtualFree(uncompr, 0, MEM_RELEASE);
				return -CUI_EMATCH;	
			}

			// unicode texts
			if (uncompr[3] != 0xff || uncompr[4] != 0xfe) {
				VirtualFree(uncompr, 0, MEM_RELEASE);
				return -CUI_EMATCH;	
			}

			if (CryptMode == 2) {	// compressed text stream
				u64 compressed = *(u64 *)&uncompr[5];
				u64 uncompressed = *(u64 *)&uncompr[13];

				if (compressed != (unsigned long)compressed || uncompressed != (unsigned long)uncompressed) {
					VirtualFree(uncompr, 0, MEM_RELEASE);
					return -CUI_EMATCH;	
				}

				BYTE *Buffer = (BYTE *)VirtualAlloc(NULL, (DWORD)uncompressed, 
					MEM_COMMIT, PAGE_READWRITE);
				if (!Buffer) {
					VirtualFree(uncompr, 0, MEM_RELEASE);
					return -CUI_EMEM;	
				}

				unsigned long destlen = (unsigned long)uncompressed;
				int result = uncompress(Buffer, &destlen, uncompr + 21, (unsigned long)compressed);
				VirtualFree(uncompr, 0, MEM_RELEASE);
				if (result != Z_OK || destlen != (unsigned long)uncompressed) {
					VirtualFree(Buffer, 0, MEM_RELEASE);
					return -CUI_EUNCOMPR;	
				}

				uncompr = Buffer;
				pkg_res->actual_data_length = destlen;
			} else if (CryptMode == 1) {
				DWORD act_len = (pkg_res->actual_data_length - 5) + 2;
				WCHAR *act;

				act = (WCHAR *)VirtualAlloc(NULL, (DWORD)act_len, 
					MEM_COMMIT, PAGE_READWRITE);
				if (!act) {
					VirtualFree(uncompr, 0, MEM_RELEASE);
					return -CUI_EMEM;
				}
				act[0] = 0xfeff;

				WCHAR *buf = (WCHAR *)&uncompr[5];
				// simple crypt
				for (DWORD i = 0; i < (pkg_res->actual_data_length - 5) / 2; i++) {
					WCHAR ch = buf[i];
					ch = ((ch & 0xaaaaaaaa) >> 1) | ((ch & 0x55555555) << 1);
					act[i + 1] = ch;
				}
				VirtualFree(uncompr, 0, MEM_RELEASE);
				uncompr = (BYTE *)act;
				pkg_res->actual_data_length = act_len;
			} else if (CryptMode == 0) {
				DWORD act_len = (pkg_res->actual_data_length - 5) + 2;
				WCHAR *act;

				act = (WCHAR *)VirtualAlloc(NULL, (DWORD)act_len, 
					MEM_COMMIT, PAGE_READWRITE);
				if (!act) {
					VirtualFree(uncompr, 0, MEM_RELEASE);
					return -CUI_EMEM;
				}
				act[0] = 0xfeff;

				WCHAR *buf = (WCHAR *)&uncompr[5];
				// simple crypt (buggy version)
				for (DWORD i = 0; i < pkg_res->actual_data_length - 5; i++) {
					WCHAR ch = buf[i];
					if (ch >= 0x20)
						act[i + 1] = ch ^ (((ch & 0xfe) << 8) ^ 1);
				}
				VirtualFree(uncompr, 0, MEM_RELEASE);
				uncompr = (BYTE *)act;
				pkg_res->actual_data_length = act_len;
			}
		}
	}

skip_dec:
	pkg_res->actual_data = uncompr;

	return 0;
}

static int kirikiri2_xp3_save_resource(struct resource *res, 
									   struct package_resource *pkg_res)
{
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (pkg_res->actual_data && pkg_res->actual_data_length) {
		if (res->rio->write(res, pkg_res->actual_data, pkg_res->actual_data_length)) {
			res->rio->close(res);
			return -CUI_EWRITE;
		}
	} else if (pkg_res->raw_data && pkg_res->raw_data_length) {
		if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
			res->rio->close(res);
			return -CUI_EWRITE;
		}
	}
	res->rio->close(res);

	return 0;
}

static void kirikiri2_xp3_release_resource(struct package *pkg, 
										   struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		VirtualFree(pkg_res->actual_data, 0, MEM_RELEASE);
		pkg_res->actual_data = NULL;
	}
	
	if (pkg_res->raw_data) {
		VirtualFree(pkg_res->raw_data, 0, MEM_RELEASE);
		pkg_res->raw_data = NULL;
	}
}

static void kirikiri2_xp3_release(struct package *pkg, 
								  struct package_directory *pkg_dir)
{
	my_xp3_entry_t *my_xp3_entry_buf;

	my_xp3_entry_buf = (my_xp3_entry_t *)pkg_dir->directory;
	if (my_xp3_entry_buf) {
		for (unsigned int i = 0; i < pkg_dir->index_entries; i++)
			free(my_xp3_entry_buf[i].segm);
		free(my_xp3_entry_buf);
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg);
}

static cui_ext_operation kirikiri2_xp3_operation = {
	kirikiri2_xp3_match,				/* match */
	kirikiri2_xp3_extract_directory,	/* extract_directory */
	kirikiri2_xp3_parse_resource_info,	/* parse_resource_info */
	kirikiri2_xp3_extract_resource,		/* extract_resource */
	kirikiri2_xp3_save_resource,		/* save_resource */
	kirikiri2_xp3_release_resource,		/* release_resource */
	kirikiri2_xp3_release				/* release */
};

/********************* exe *********************/

static int kirikiri2_exe_match(struct package *pkg)
{
	s8 mz[2];
	u8 buffer[256 * 1024];
	u32 exe_size;
	unsigned int cmp, blocks, remain;
	int found = 0;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->read(pkg, mz, 2)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;	
	}
	
	if (memcmp(mz, "MZ", 2)) {		
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->pio->seek(pkg, 16, IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_ESEEK;	
	}

	if (pkg->pio->length_of(pkg, &exe_size)) {
		pkg->pio->close(pkg);
		return -CUI_ELEN;	
	}

	// XP3 mark must be aligned by a paragraph ( 16 bytes )
	blocks = (exe_size - 16) / sizeof(buffer);
	remain = (exe_size - 16) % sizeof(buffer);
	cmp = sizeof(buffer) / 16;
	int err = 0;
	xp3_header_t *header = NULL;
	for (unsigned int i = 0; i < blocks; i++) {
		if (pkg->pio->read(pkg, buffer, sizeof(buffer))) {
			err = -CUI_EREAD;
			break;
		}
		
		for (unsigned int k = 0; k < cmp; k++) {
			if (!memcmp(&buffer[k * 16], "XP3\r\n \n\x1a\x8b\x67\x01", 11)) {
				xp3_start_offset = i * sizeof(buffer) + k * 16;
				header = (xp3_header_t *)&buffer[k * 16];
				break;
			}
		}
		if (k != cmp) {
			found = 1;
			break;
		}	
	}
	if (!found && remain > 16) {
		remain &= ~(16 - 1);
		if (pkg->pio->read(pkg, buffer, remain))
			return -CUI_EREAD;

		cmp = remain / 16;
		for (unsigned int k = 0; k < cmp; k++) {
			if (!memcmp(&buffer[k * 16], "XP3\r\n \n\x1a\x8b\x67\x01", 11)) {
				xp3_start_offset = blocks * sizeof(buffer) + k * 16;
				header = (xp3_header_t *)&buffer[k * 16];
				break;
			}
		}
		if (k != cmp)
			found = 1;
	}
	xp3_start_offset += 16;

	if (found && header) {
		u64 long_offset = (u64)header->index_offset_lo | ((u64)header->index_offset_hi) << 32;
		long_offset += xp3_start_offset;

		if (pkg->pio->seek64(pkg, (u32)long_offset, 
				(u32)(long_offset >> 32), IO_SEEK_SET)) {
			pkg->pio->close(pkg);
			return -CUI_ESEEK;
		}
		
		u8 index_flag;
		if (pkg->pio->read(pkg, &index_flag, 1)) {
			pkg->pio->close(pkg);
			return -CUI_ESEEK;	
		}
			
		u32 index_size;					
		if (pkg->pio->read(pkg, &index_size, 4)) {
			pkg->pio->close(pkg);
			return -CUI_EREAD;	
		}

		if (index_flag != 0x80 && header->index_offset_lo != 0x17) {	// is not a cushion header
			if (!index_size) {		
				pkg->pio->close(pkg);
				return -CUI_EMATCH;
			}
		}

		struct package_directory pkg_dir;
		memset(&pkg_dir, 0, sizeof(pkg_dir));
		if (kirikiri2_xp3_extract_directory(pkg, &pkg_dir)) {
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}
		free(pkg_dir.directory);
	}

	return found ? 0 : (!err ? -CUI_EMATCH : err);	
}

static cui_ext_operation kirikiri2_exe_operation = {
	kirikiri2_exe_match,				/* match */
	kirikiri2_xp3_extract_directory,	/* extract_directory */
	kirikiri2_xp3_parse_resource_info,	/* parse_resource_info */
	kirikiri2_xp3_extract_resource,		/* extract_resource */
	kirikiri2_xp3_save_resource,		/* save_resource */
	kirikiri2_xp3_release_resource,		/* release_resource */
	kirikiri2_xp3_release				/* release */
};

/********************* tlg *********************/

static int kirikiri2_tlg_match(struct package *pkg)
{
	unsigned char mark[11];
	unsigned long tlg_version;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->read(pkg, mark, sizeof(mark))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;	
	}

	if (!memcmp(mark, "TLG6.0\x0raw\x1a", 11)) {
		tlg6_header_t tlg6_header;

		if (pkg->pio->readvec(pkg, &tlg6_header, 
				sizeof(tlg6_header), 0, IO_SEEK_SET)) {
			pkg->pio->close(pkg);
			return -CUI_EREADVEC;	
		}

		if (tlg6_header.colors != 1 && tlg6_header.colors != 3 
				&& tlg6_header.colors != 4) {
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}

		if (tlg6_header.flag) {
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}

		if (tlg6_header.type) {
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}

		if (tlg6_header.golomb_bit_length) {
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}

		tlg_version = 6;
	} else if (!memcmp(mark, "TLG5.0\x0raw\x1a", 11)) {
		tlg5_header_t tlg5_header;

		if (pkg->pio->readvec(pkg, &tlg5_header, 
				sizeof(tlg5_header), 0, IO_SEEK_SET)) {
			pkg->pio->close(pkg);
			return -CUI_EREADVEC;	
		}

		if (tlg5_header.colors != 3 && tlg5_header.colors != 4) {
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}

		tlg_version = 5;		
	} else {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	package_set_private(pkg, tlg_version);			

	return 0;	
}

static int kirikiri2_tlg_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{	
	u32 tlg_size;
	unsigned long tlg_version;
	int ret;

	tlg_version = package_get_private(pkg);

	DWORD tlg_offset;
	if (tlg_version == -5 || tlg_version == -6) {
		tlg_sds_header_t tlg_sds_header;

		if (pkg->pio->readvec(pkg, &tlg_sds_header, sizeof(tlg_sds_header), 
				0, IO_SEEK_SET))
			return -CUI_EREADVEC;	

		tlg_size = tlg_sds_header.size;
		tlg_offset = sizeof(tlg_sds_header_t);
		tlg_version = 0 - tlg_version;
	} else {
		if (pkg->pio->length_of(pkg, &tlg_size))
			return -CUI_ELEN;
		tlg_offset = 0;
	}

	BYTE *tlg_raw_data = (BYTE *)malloc(tlg_size);
	if (!tlg_raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, tlg_raw_data, tlg_size, tlg_offset, IO_SEEK_SET)) {
		free(tlg_raw_data);
		return -CUI_EREADVEC;	
	}

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = tlg_raw_data;
		pkg_res->raw_data_length = tlg_size;
		return 0;
	}

	if (tlg_version == 5)
		ret = tlg5_process(tlg_raw_data, tlg_size, 
			(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length);
	else if (tlg_version == 6)
		ret = tlg6_process(tlg_raw_data, tlg_size, 
			(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length);

	if (ret) {
		free(tlg_raw_data);
		return ret;
	}

	pkg_res->raw_data = tlg_raw_data;
	pkg_res->raw_data_length = tlg_size;
	
	return 0;
}

static void kirikiri2_tlg_release_resource(struct package *pkg, 
										   struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void kirikiri2_tlg_release(struct package *pkg, 
								  struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

static cui_ext_operation kirikiri2_tlg_operation = {
	kirikiri2_tlg_match,			/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	kirikiri2_tlg_extract_resource,	/* extract_resource */
	kirikiri2_xp3_save_resource,	/* save_resource */
	kirikiri2_tlg_release_resource,	/* release_resource */
	kirikiri2_tlg_release			/* release */
};

/********************* tlg sds *********************/

static int kirikiri2_tlg_sds_match(struct package *pkg)
{
	tlg_sds_header_t tlg_sds_header;
	unsigned long tlg_version;
	s8 mark[11];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->read(pkg, &tlg_sds_header, sizeof(tlg_sds_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;	
	}

	if (memcmp(tlg_sds_header.mark, "TLG0.0\x0sds\x1a", 11)) {		
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	// fix for maf_1_2_c.tlg @ まままままま
	u32 size;
	pkg->pio->length_of(pkg, &size);
	if (!tlg_sds_header.size || size < tlg_sds_header.size) {		
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->pio->read(pkg, mark, sizeof(mark))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;	
	}

	if (!memcmp(mark, "TLG6.0\x0raw\x1a", 11)) {
		tlg6_header_t tlg6_header;

		if (pkg->pio->readvec(pkg, &tlg6_header, 
				sizeof(tlg6_header), sizeof(tlg_sds_header_t), IO_SEEK_SET)) {
			pkg->pio->close(pkg);
			return -CUI_EREADVEC;	
		}

		if (tlg6_header.colors != 1 && tlg6_header.colors != 3 
				&& tlg6_header.colors != 4) {
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}

		if (tlg6_header.flag) {
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}

		if (tlg6_header.type) {
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}

		if (tlg6_header.golomb_bit_length) {
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}

		tlg_version = -6;
	} else if (!memcmp(mark, "TLG5.0\x0raw\x1a", 11)) {
		tlg5_header_t tlg5_header;

		if (pkg->pio->readvec(pkg, &tlg5_header, 
				sizeof(tlg5_header), sizeof(tlg_sds_header_t), IO_SEEK_SET)) {
			pkg->pio->close(pkg);
			return -CUI_EREADVEC;	
		}

		if (tlg5_header.colors != 3 && tlg5_header.colors != 4) {
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}

		tlg_version = -5;
	}
	package_set_private(pkg, tlg_version);

	return 0;	
}

static cui_ext_operation kirikiri2_tlg_sds_operation = {
	kirikiri2_tlg_sds_match,		/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	kirikiri2_tlg_extract_resource,	/* extract_resource */
	kirikiri2_xp3_save_resource,	/* save_resource */
	kirikiri2_tlg_release_resource,	/* release_resource */
	kirikiri2_tlg_release			/* release */
};

/********************* ksd *********************/

static int kirikiri2_ksd_match(struct package *pkg)
{
	u8 mark[5];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->read(pkg, mark, sizeof(mark))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;	
	}

	if (mark[0] != 0xfe || mark[1] != 0xfe || mark[2] > 2
			|| mark[3] != 0xff || mark[4] != 0xfe) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;
}

static int kirikiri2_ksd_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{	
	u32 ksd_size;

	if (pkg->pio->length_of(pkg, &ksd_size))
		return -CUI_ELEN;

	BYTE *raw = (BYTE *)malloc(ksd_size);
	if (!raw)
		return -CUI_EMEM;
	
	if (pkg->pio->readvec(pkg, raw, ksd_size, 0, IO_SEEK_SET)) {
		free(raw);
		return -CUI_EREADVEC;	
	}

	u8 CryptMode = raw[2];
	if (CryptMode == 2) {	// compressed text stream
		u64 compressed = *(u64 *)&raw[5];
		u64 uncompressed = *(u64 *)&raw[13];

		if (compressed != (unsigned long)compressed || uncompressed != (unsigned long)uncompressed) {
			free(raw);
			return -CUI_EMATCH;	
		}

		BYTE *uncompr = (BYTE *)malloc((unsigned long)uncompressed);
		if (!uncompr) {
			free(raw);
			return -CUI_EMEM;	
		}

		unsigned long destlen = (unsigned long)uncompressed;
		int result = uncompress(uncompr, &destlen, raw + 21, (unsigned long)compressed);
		if (result != Z_OK || destlen != (unsigned long)uncompressed) {
			free(uncompr);
			free(raw);
			return -CUI_EUNCOMPR;	
		}
		pkg_res->actual_data = uncompr;
		pkg_res->actual_data_length = destlen;
	} else if (CryptMode == 1) {
		DWORD act_len = (ksd_size - 5) + 2;
		WCHAR *act;

		act = (WCHAR *)malloc(act_len);
		if (!act) {
			free(raw);
			return -CUI_EMEM;
		}
		act[0] = 0xfeff;

		WCHAR *buf = (WCHAR *)&raw[5];
		// simple crypt
		for (DWORD i = 0; i < (ksd_size - 5) / 2; i++) {
			WCHAR ch = buf[i];
			ch = ((ch & 0xaaaaaaaa) >> 1) | ((ch & 0x55555555) << 1);
			act[i + 1] = ch;
		}
		pkg_res->actual_data = act;
		pkg_res->actual_data_length = act_len;
	} else if (CryptMode == 0) {
		DWORD act_len = (ksd_size - 5) + 2;
		WCHAR *act;

		act = (WCHAR *)malloc(act_len);
		if (!act) {
			free(raw);
			return -CUI_EMEM;
		}
		act[0] = 0xfeff;

		WCHAR *buf = (WCHAR *)&raw[5];
		// simple crypt (buggy version)
		for (DWORD i = 0; i < ksd_size - 5; i++) {
			WCHAR ch = buf[i];
			if (ch >= 0x20)
				act[i + 1] = ch ^ (((ch & 0xfe) << 8) ^ 1);
		}
		pkg_res->actual_data = act;
		pkg_res->actual_data_length = act_len;
	}
	free(raw);

	return 0;
}

static cui_ext_operation kirikiri2_ksd_operation = {
	kirikiri2_ksd_match,			/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	kirikiri2_ksd_extract_resource,	/* extract_resource */
	kirikiri2_xp3_save_resource,	/* save_resource */
	kirikiri2_tlg_release_resource,	/* release_resource */
	kirikiri2_tlg_release			/* release */
};

/********************* tpk *********************/

static int kirikiri2_tpk_match(struct package *pkg)
{
	tpk_header_t tpk_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->read(pkg, &tpk_header, sizeof(tpk_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;	
	}

	if (strncmp(tpk_header.maigc, "EL", 2) ||
			strncmp(tpk_header.version, "0101", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;			
	}		

	return 0;	
}

static int kirikiri2_tpk_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{
	tpk_header_t tpk_header;
	
	if (pkg->pio->readvec(pkg, &tpk_header, sizeof(tpk_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;
	
	vector<tpk_entry_t> tpk_index;
	tpk_entry_t entry;

	entry.offset = tpk_header.data_offset;
	entry.length = tpk_header.data_size;
	entry.width = tpk_header.width;
	entry.height = tpk_header.height;
	tpk_index.push_back(entry);

	while (1) {
		tpk_entry_t entry;

		if (pkg->pio->read(pkg, &entry, sizeof(entry)))
			return -CUI_EREAD;

		if (!entry.length)
			break;

		tpk_index.push_back(entry);
	}

	for (DWORD i = 1; i < tpk_index.size(); ++i)
		tpk_index[i].offset += tpk_header.block_offset;

	tpk_entry_t *index = new tpk_entry_t[tpk_index.size()];
	if (!index)
		return -CUI_EMEM;

	for (i = 0; i < tpk_index.size(); ++i)
		index[i] = tpk_index[i];

	pkg_dir->index_entries = tpk_index.size();
	pkg_dir->index_entry_length = sizeof(tpk_entry_t);
	pkg_dir->directory = index;
	pkg_dir->directory_length = sizeof(tpk_entry_t) * tpk_index.size();
	package_set_private(pkg, index);
	
	return 0;
}

static int kirikiri2_tpk_parse_resource_info(struct package *pkg,
											 struct package_resource *pkg_res)
{
	tpk_entry_t *tpk_entry;

	tpk_entry = (tpk_entry_t *)pkg_res->actual_index_entry;
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = tpk_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = tpk_entry->offset;
#ifdef UNICODE
	pkg_res->flags |= PKG_RES_FLAG_UNICODE;
#endif
	if (pkg_res->index_number) {
		TCHAR tmp[MAX_PATH];
		_tcscpy(tmp, pkg->name);
		*(_tcschr(tmp, _T('.'))) = 0;	
		_stprintf((TCHAR *)pkg_res->name, _T("%s@%03d"), tmp, pkg_res->index_number);
	} else
		_tcscpy((TCHAR *)pkg_res->name, pkg->name);

	return 0;
}

static BYTE *base_tpk;

static int kirikiri2_tpk_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{	
	BYTE *tpk = new BYTE[pkg_res->raw_data_length];
	if (!tpk)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, tpk, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET)) {
		delete [] tpk;
		return -CUI_EREADVEC;	
	}

	tpk_entry_t *tpk_entry = (tpk_entry_t *)pkg_res->actual_index_entry;
	for (DWORD i = 0; i < tpk_entry->width * tpk_entry->height; ++i) {
		BYTE b = tpk[i * 4 + 0];
		BYTE r = tpk[i * 4 + 2];
		tpk[i * 4 + 0] = r;
		tpk[i * 4 + 2] = b;
	}

	if (!pkg_res->index_number) {
		base_tpk = new BYTE[pkg_res->raw_data_length]; 
		if (!base_tpk) {
			delete [] tpk;
			return -CUI_EMEM;
		}
		memcpy(base_tpk, tpk, pkg_res->raw_data_length);

		BYTE *b = tpk;
		DWORD pixels = tpk_entry->width * tpk_entry->height;
		for (i = 0; i < pixels; ++i) {
			b[0] = b[0] * b[3] / 255 + (255 - b[3]);
			b[1] = b[1] * b[3] / 255 + (255 - b[3]);
			b[2] = b[2] * b[3] / 255 + (255 - b[3]);
			b += 4;
		}

		if (MyBuildBMPFile(tpk, pkg_res->raw_data_length, NULL, 0,
				tpk_entry->width, 0-tpk_entry->height, 32, (BYTE **)&pkg_res->actual_data,
				&pkg_res->actual_data_length, my_malloc)) {
			delete [] tpk;
			return -CUI_EMEM;
		}		
	} else {
		tpk_entry_t *first_entry = (tpk_entry_t *)package_get_private(pkg);

		DWORD end_y = tpk_entry->orig_y + tpk_entry->height;
		DWORD end_x = tpk_entry->orig_x + tpk_entry->width;
		u32 *dst = (u32 *)base_tpk + first_entry->width * tpk_entry->orig_y 
			+ tpk_entry->orig_x;
		u32 *src = (u32 *)tpk;
		for (DWORD y = tpk_entry->orig_y; y < end_y; ++y) {
			for (DWORD x = tpk_entry->orig_x; x < end_x; ++x)
				*dst++ = *src++;
			dst += first_entry->width - tpk_entry->width;
		}

		if (MyBuildBMPFile(base_tpk, first_entry->length, NULL, 0,
				first_entry->width, 0-first_entry->height, 32, (BYTE **)&pkg_res->actual_data,
				&pkg_res->actual_data_length, my_malloc)) {
			delete [] tpk;
			return -CUI_EMEM;
		}
	}
	delete [] tpk;

	return 0;
}

static void kirikiri2_tpk_release(struct package *pkg, 
								  struct package_directory *pkg_dir)
{
	if (base_tpk) {
		delete [] base_tpk;
		base_tpk = NULL;
	}

	pkg->pio->close(pkg);
}

static cui_ext_operation kirikiri2_tpk_operation = {
	kirikiri2_tpk_match,				/* match */
	kirikiri2_tpk_extract_directory,	/* extract_directory */
	kirikiri2_tpk_parse_resource_info,	/* parse_resource_info */
	kirikiri2_tpk_extract_resource,		/* extract_resource */
	kirikiri2_xp3_save_resource,		/* save_resource */
	kirikiri2_tlg_release_resource,		/* release_resource */
	kirikiri2_tpk_release				/* release */
};

int CALLBACK kirikiri2_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".xp3"), NULL, 
		NULL, &kirikiri2_xp3_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;
	
	if (callback->add_extension(callback->cui, _T(".exe"), NULL, 
		NULL, &kirikiri2_exe_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".xp3"), NULL, 
		NULL, &kirikiri2_exe_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".tlg"), _T(".bmp"), 
		_T("TLG0.0 Structured Data Stream"), &kirikiri2_tlg_sds_operation, 
				CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".tlg"), _T(".bmp"), 
		_T("TLG raw data"), &kirikiri2_tlg_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;
#if 0
	if (callback->add_extension(callback->cui, _T(".tlg5"), _T(".bmp"), 
		_T("TLG5 raw data"), &kirikiri2_tlg_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".tlg6"), _T(".bmp"), 
		_T("TLG6 raw data"), &kirikiri2_tlg_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;
#endif
	if (callback->add_extension(callback->cui, _T(".ksd"), _T(".ksd.txt"), 
		NULL, &kirikiri2_ksd_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".kdt"), _T(".kdt.txt"), 
		NULL, &kirikiri2_ksd_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".arc"), NULL, 
		NULL, &kirikiri2_xp3_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".tpk"), _T(".tpk.bmp"), 
		NULL, &kirikiri2_tpk_operation, CUI_EXT_FLAG_PKG 
		| CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_RES))
			return -1;

	TVPTLG6InitLeadingZeroTable();
	TVPTLG6InitGolombTable();
	
	xp3filter_decode_init();

	kirikiri2_locale_id = locale_app_register(kirikiri2_locale_configurations, 3);

	return 0;
}
