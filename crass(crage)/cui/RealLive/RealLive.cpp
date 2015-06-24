#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>
#include <utility.h>

//Q:\[trial]\【RealLive】

/*
VisualArt's. Version 1.0 was called AVG2000
the name was changed to RealLive around version 1.0.0.8.
the Kinetic interpreter introduced at version 1.2.6.4

The PDT format is a holdover from the older AVG32 system, 
and is the standard bitmap format in AVG2000: 
it stores 8-bit paletted or 24-bit RGB bitmaps, 
together with an optional 8-bit alpha mask. 
In RealLive proper it is normally used only for cursor images.

G00 is the true native format, 
used for most graphics in RealLive games. 
There are three G00 formats, 
each (naturally) with their own advantages and disadvantages:
0	24-bit RGB, no mask
1	8-bit paletted; palette is ARGB
2	32-bit ARGB, plus extra features (see below)
Format 2 G00 bitmaps can contain multiple sub-bitmaps, termed “regions”.

A RealLive game is made up of four main parts: 
the interpreter (avg2000.exe, reallive.exe, or kinetic.exe), 
the configuration file (gameexe.ini), 
the scenario file (typically seen.txt1), 
and accompanying data files, which vary in type and number, 
but typically include graphics (in the g00 and pdt formats), 
animations (in the gan and anm formats), and music (in the nwa format).

Note that in the case of DRM-protected games, 
the configuration and data files are further compacted into 
a single file (typically kineticdata.pak), which is encrypted 
using a rather stronger method that I have not yet discovered 
how to unlock. It is not possible to access the files this 
contains directly with RLdev, although it is simple enough to 
extract the contents manually from a memory dump of a running game.

AVG2000, RealLive, and Kinetic. While the API and basic bytecode 
format used by all three is essentially identical, they are not 
compatible: AVG2000 (the original successor to AVG32, later renamed 
to RealLive) uses a different scenario header and encoding scheme, 
and Kinetic (a special DRM-enabled interpreter used only for the 
Kinetic Novel series) reassigns a large number of fundamental 
operations in what appears to be a futile attempt to make 
reverse-engineering harder.
*/

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information RealLive_cui_information = {
	_T("VisualArt"),		/* copyright */
	_T("VisualArt's ScriptEngine RealLive"),	/* system */
	_T(".g00 .nwk .txt .ovk .cgm .dat .nwk .dbs"),	/* package */
	_T("0.7.7"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-7-21 15:24"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[16];		// "CGTABLE"
	u32 index_entries;	// 每项36字节
	u32 unknown[3];
} cgm_header_t;

typedef struct {
	s8 name[32];
	u32 id;				// 从0开始
} cgm_entry_t;

typedef struct {
	s8 magic[16];		// "FILESET"
	u32 unknown0;		// 1
	u32 unknown[4];		// 0
} dat_header_t;

/* seen子文件编号：
比如AIR 编号是有规律的 游戏的日期对应SEEN的ID
然后每一个角色的分剧情单独放在一套三位数的SEEN里面
一般来说，高号数的的SEEN是用来放辅助脚本的，比如Load/Save CG/音乐页等

老游戏这个地方都是10002
但是最近新的游戏，这个地方标记为110002
这些SEEN采用了更多一层的加密。
到现在为止只有我说的两个数出现过，而且多1000000的那个就是加密的
*/
typedef struct {
	u32 offset;
	u32 length;
} seen_entry_t;

/*
子SEEN分两段储存
头部不加密不压缩，脚本指令部分压缩并加密
 */
typedef struct {
	u32 header_size;			// [no used]0x1d0
	u32 compile_version;		// 10002
	// AVG_READ_FLAG:是有关游戏文本索引的
	// 第一个总是1000000,接下来的就是文本对应的脚本行号,一句文本一个int
	// 分别是索引的Offset,数量和长度
	// Reallive的指令流很奇怪，里面居然夹杂了行号，和内部使用的编程脚本对应 
	// 而定位已读文本使用的也是这个行号
	// 可能是debug用，也可能是方便读档存档用
	u32 AVG_READ_FLAG_offset;
	u32 AVG_READ_FLAG_count;	// 每项4字节
	u32 AVG_READ_FLAG_size;
	//第6,7,8个是有关脚本中出场人物名称索引的
	//分别是索引的Offset,数量和长度
	u32 character_name_offset;
	u32 character_name_count;
	u32 character_name_size;
	// 关于脚本指令段的
	u32 instruction_offset;
	u32 instruction_ORGINAL_size;// [no used]解压缩后的长度
	u32 instruction_size;		// 压缩的数据长度
	//两个int作用不明，事实上我觉得根本就没有使用，只是填了点数进去而已
	//或者跟下面的索引有关
	u32 unknown[2];	// 0和3
	/*
	接下来100个int是脚本指令段分节的索引
	一个脚本可以被分为多个节，在脚本指令中会出现(跳到SEENxxxx的第n节)这样的指令
	这里的索引就是用来定位节的，内容为Offset，以脚本指令段的开始作为0
	貌似原始制作程序一开始把所有的节都初始化到第一节的Offset，因此没有使用的节和第一节Offset是一样的
	然后接下来三个int貌似没有使用，始终是0
	*/
	u32 segment_offset[100];
	u32 reserved[3];
} seen_data_header_t;

typedef struct {
	u8 type;			// 0 - 24 bit, 1 - 8 bit
	u16 width;
	u16 height;
} g00_header_t;

typedef struct {
	u32 orig_x;			// 原点在屏幕中的位置
	u32 orig_y;
	u32 end_x;			// 终点在屏幕中的位置
	u32 end_y;
	u32 unknown[2];		// 0
} g02_info_t;

typedef struct {
	u32 entries;
} g02_header_t;

typedef struct {
	u32 offset;
	u32 length;	
} g02_entry_t;

typedef struct {			// 0x74
	u16 unknown0;			// 1
	u16 blocks;				// 描述热点区域的块数
	u32 hs_orig_x;			// 该part的热点区域(热点区域由N个block描述)的在该part中的位置
	u32 hs_orig_y;			
	u32 hs_width;			// 该part的热点区域的宽度
	u32 hs_height;			// 该part的热点区域的高度
	u32 screen_orig_x;		// 显示时,屏幕的原点在该part中的位置
	u32	screen_orig_y;
	u32 width;				// part图的宽度
	u32 height;				// part图的高度
	u8 unknown1[80];
} g02p_header_t;

typedef struct {		// 0x5c
	u16 start_x;		// 该block在part图中的位置
	u16 start_y;
	u16 flag;			// 0 - 中心部分 非0 - 边缘部分
	u16 width;			// 该block图中的宽度
	u16 height;			// 该block图中的高度
	u8 reserved[82];
} g02b_header_t;

// 如果是raw PCM格式,则紧接头部的就是PCM数据
typedef struct {			// PCMstruct
	u16 channels;			// @00
	u16 bps;				// @02 BitsPerSample
	u32 freq;				// @04
	s32 compr_level;		// @08 -1(raw PCM)， 2
	u32 UseRunLength;		// @0c 0
	u32 NWAtable_len;		// @10 block总数。记录偏移的表，每项4字节,(raw PCM: 0)
	u32 data_size;			// @14 PCM数据长度
	u32 compr_data_size;	// @18 NWA文件的总长度，如果compleve==-1，则压缩长度为0
	u32 sample_count;		// @1c 总采样数
	u32 block_size;			// @20 每块包含的采样数(raw PCM: 0)
	u32 rest_size;			// @24 不足一块的采样数(raw PCM: 0)
	u32 dummy2;				// @28 rest_compr_size?(raw PCM: 0)
} nwa_header_t;

typedef struct {
	u32 entries;
} nwk_header_t;

typedef struct {
	u32 length;
	u32 offset;
	u32 id;
} nwk_entry_t;

typedef struct {
	u32 entries;
} ovk_header_t;

typedef struct {
	u32 length;
	u32 offset;
	u32 id;
	u32 samples;
} ovk_entry_t;
#pragma pack ()

typedef struct {
	char name[MAX_PATH];
	unsigned long offset;
	unsigned long  length;	
} RealLive_generic_entry_t;

typedef struct {
	u8 unknown[6];	// [0-5] 0
	u8 unknown1[10];
	u32 unknown2;		// -1
	u32 unknown3;		// 0
	u32 unknown4;		// 0
	u32 unknown5;		// 80
	u32 unknown6;		// -1
	u32 unknown7;		// 0
} gan_pat_t;


#if 0
if (nwa_header->[c] == -1) {

} else {

}
#endif

static int debug;

static DWORD CLR_TABLE[256][511];
static DWORD *CLR_TABLE_ADR[256];

static void *my_malloc(DWORD len)
{
	return new BYTE[len];
}

static inline DWORD getbits(BYTE *&data, DWORD &shift, DWORD bits)
{
	if (shift >= 8) {
		++data; 
		shift &= 7;
	}
	DWORD ret = *(WORD *)data >> shift;
	shift += bits;
	return ret & ((1 << bits) - 1);
}

static void nwa_expand_block(BYTE *data, BYTE *&out_data, 
							 WORD channels, WORD bps, 
							 int compr_level, DWORD out_data_size,
							 int UseRunLength)
{
	DWORD d[2];
	DWORD shift = 0;

	if (bps == 8)
		d[0] = *data++;
	else {
		d[0] = *(WORD *)data;
		data += 2;
	}
	
	if (channels == 2) {
		if (bps == 8)
			d[1] = *data++;
		else {
			d[1] = *(WORD *)data;
			data += 2;
		}
	}

	int flip_flag = 0;
	int runlength = 0;
	for (DWORD i = 0; i < out_data_size; ++i) {
		if (!runlength) {			
			DWORD type = getbits(data, shift, 3);
			if (type == 7) {	// ??
#if 1
				getbits(data, shift, 1);
				DWORD BITS, SHIFT;
				if (compr_level >= 3) {	// ???
					BITS = 8;
					SHIFT = 9;
				} else {
					BITS = 8 - compr_level;
					SHIFT = 2 + 7 + compr_level;
				}
				DWORD MASK1 = (1 << (BITS - 1));
				DWORD MASK2 = (1 << (BITS - 1)) - 1;
				DWORD b = getbits(data, shift, BITS);
				if (b & MASK1)
					d[flip_flag] -= (b & MASK2) << SHIFT;
				else
					d[flip_flag] += (b & MASK2) << SHIFT;
#else
				/* 7 : 络きな汗尸 */
				/* RunLength() 铜跟箕∈compr_level==5, 不兰ファイル) では痰跟 */
				if (getbits(data, shift, 1) == 1) {
					d[flip_flag] = 0; /* 踏蝗脱 */
				} else {
					int BITS, SHIFT;
					if (compr_level >= 3) {
						BITS = 8;
						SHIFT = 9;
					} else {
						BITS = 8-compr_level;
						SHIFT = 2+7+compr_level;
					}
					const int MASK1 = (1<<(BITS-1));
					const int MASK2 = (1<<(BITS-1))-1;
					int b = getbits(data, shift, BITS);
					if (b&MASK1)
						d[flip_flag] -= (b&MASK2)<<SHIFT;
					else
						d[flip_flag] += (b&MASK2)<<SHIFT;
				}
#endif
			} else if (type != 0) {	// 4, 5, 6 OK
				DWORD BITS, SHIFT;
				if (compr_level >= 3) {	// ??
					BITS = compr_level + 3;
					SHIFT = 1+type;
				} else {
					BITS = 5 - compr_level;
					SHIFT = 2 + type + compr_level;
				}
				DWORD MASK1 = (1 << (BITS - 1));
				DWORD MASK2 = (1 << (BITS - 1)) - 1;
				DWORD b = getbits(data, shift, BITS);
				if (b & MASK1)
					d[flip_flag] -= (b & MASK2) << SHIFT;
				else
					d[flip_flag] += (b & MASK2) << SHIFT;
			} else {	// type == 0, ????
#if 1
				if (UseRunLength) {
					getbits(data, shift, 1);
					runlength = 0;
				}
#else
				if (UseRunLength) {
					runlength = getbits(data, shift, 1);
					if (runlength==1) {
						runlength = getbits(data,shift,2);
						if (runlength == 3) {
							runlength = getbits(data, shift, 8);
						}
					}
				}
#endif
			}
		} else
			--runlength;

		if (bps == 8)
			*out_data++ = (BYTE)d[flip_flag];
		else {
			*(WORD *)out_data = (WORD)d[flip_flag];
			out_data += 2;
		}
		if (channels == 2)
			flip_flag ^= 1;
	}
	return;
}

static BYTE RealLive_decode_table[256] = {
		0x8b, 0xe5, 0x5d, 0xc3, 0xa1, 0xe0, 0x30, 0x44, 0x00, 0x85, 0xc0, 0x74, 0x09, 0x5f, 0x5e, 0x33, 
		0xc0, 0x5b, 0x8b, 0xe5, 0x5d, 0xc3, 0x8b, 0x45, 0x0c, 0x85, 0xc0, 0x75, 0x14, 0x8b, 0x55, 0xec, 
		0x83, 0xc2, 0x20, 0x52, 0x6a, 0x00, 0xe8, 0xf5, 0x28, 0x01, 0x00, 0x83, 0xc4, 0x08, 0x89, 0x45, 
		0x0c, 0x8b, 0x45, 0xe4, 0x6a, 0x00, 0x6a, 0x00, 0x50, 0x53, 0xff, 0x15, 0x34, 0xb1, 0x43, 0x00, 
		0x8b, 0x45, 0x10, 0x85, 0xc0, 0x74, 0x05, 0x8b, 0x4d, 0xec, 0x89, 0x08, 0x8a, 0x45, 0xf0, 0x84, 
		0xc0, 0x75, 0x78, 0xa1, 0xe0, 0x30, 0x44, 0x00, 0x8b, 0x7d, 0xe8, 0x8b, 0x75, 0x0c, 0x85, 0xc0, 
		0x75, 0x44, 0x8b, 0x1d, 0xd0, 0xb0, 0x43, 0x00, 0x85, 0xff, 0x76, 0x37, 0x81, 0xff, 0x00, 0x00, 
		0x04, 0x00, 0x6a, 0x00, 0x76, 0x43, 0x8b, 0x45, 0xf8, 0x8d, 0x55, 0xfc, 0x52, 0x68, 0x00, 0x00, 
		0x04, 0x00, 0x56, 0x50, 0xff, 0x15, 0x2c, 0xb1, 0x43, 0x00, 0x6a, 0x05, 0xff, 0xd3, 0xa1, 0xe0, 
		0x30, 0x44, 0x00, 0x81, 0xef, 0x00, 0x00, 0x04, 0x00, 0x81, 0xc6, 0x00, 0x00, 0x04, 0x00, 0x85, 
		0xc0, 0x74, 0xc5, 0x8b, 0x5d, 0xf8, 0x53, 0xe8, 0xf4, 0xfb, 0xff, 0xff, 0x8b, 0x45, 0x0c, 0x83, 
		0xc4, 0x04, 0x5f, 0x5e, 0x5b, 0x8b, 0xe5, 0x5d, 0xc3, 0x8b, 0x55, 0xf8, 0x8d, 0x4d, 0xfc, 0x51, 
		0x57, 0x56, 0x52, 0xff, 0x15, 0x2c, 0xb1, 0x43, 0x00, 0xeb, 0xd8, 0x8b, 0x45, 0xe8, 0x83, 0xc0, 
		0x20, 0x50, 0x6a, 0x00, 0xe8, 0x47, 0x28, 0x01, 0x00, 0x8b, 0x7d, 0xe8, 0x89, 0x45, 0xf4, 0x8b, 
		0xf0, 0xa1, 0xe0, 0x30, 0x44, 0x00, 0x83, 0xc4, 0x08, 0x85, 0xc0, 0x75, 0x56, 0x8b, 0x1d, 0xd0, 
		0xb0, 0x43, 0x00, 0x85, 0xff, 0x76, 0x49, 0x81, 0xff, 0x00, 0x00, 0x04, 0x00, 0x6a, 0x00, 0x76, 
};

static void RealLive_decode(BYTE *data, DWORD len)
{
	for (DWORD i = 0; i < len; ++i)
		data[i] ^= RealLive_decode_table[i % sizeof(RealLive_decode_table)];
}

static int RealLive_uncompress(BYTE *compr, BYTE **ret_uncompr, 
							   DWORD *ret_uncomprlen)
{
	DWORD comprlen = *(u32 *)compr - 8;
	DWORD uncomprlen = *(u32 *)&compr[4];
	if (uncomprlen) {
		BYTE *uncompr = new BYTE[uncomprlen + 64];
		if (!uncompr)
			return -CUI_EMEM;
		memset(uncompr, 0, uncomprlen + 64);

		comprlen += 8;
		DWORD curbyte = 8;
		DWORD act_uncomprlen = 0;
		DWORD bit_count = 0;
		DWORD flag;
		while (act_uncomprlen != uncomprlen) {
			if (!bit_count) {
				flag = compr[curbyte++];
				bit_count = 8;
			}
			if (flag & 1)
				uncompr[act_uncomprlen++] = compr[curbyte++];
			else {
				DWORD count = compr[curbyte++];
				DWORD offset = (compr[curbyte++] << 4) | (count >> 4);
				count = (count & 0xf) + 2;
				for (DWORD i = 0; i < count; ++i) {
					uncompr[act_uncomprlen] = uncompr[act_uncomprlen - offset];
					++act_uncomprlen;
				}
			}
			flag >>= 1;
			--bit_count;
		}
		*ret_uncompr = uncompr;
		*ret_uncomprlen = uncomprlen;
	} else {
		BYTE *actual_data = new BYTE[comprlen];
		if (!actual_data)
			return -CUI_EMEM;
		memcpy(actual_data, compr + 8, comprlen);
		*ret_uncompr = actual_data;
		*ret_uncomprlen = comprlen;
	}

	return 0;
}

static int RealLive_g00_type0_uncompress(BYTE *compr, BYTE **ret_uncompr, 
										 DWORD *ret_uncomprlen)
{
	DWORD comprlen = *(u32 *)compr - 8;
	DWORD uncomprlen = *(u32 *)&compr[4];
	if (uncomprlen) {
		BYTE *uncompr = new BYTE[uncomprlen + 64];
		if (!uncompr)
			return -CUI_EMEM;
		memset(uncompr, 0, uncomprlen + 64);

		comprlen += 8;
		DWORD curbyte = 8;
		DWORD act_uncomprlen = 0;
		DWORD bit_count = 0;
		DWORD flag;
		while (act_uncomprlen < uncomprlen && curbyte < comprlen) {
			if (!bit_count) {
				flag = compr[curbyte++];
				bit_count = 8;
			}
			if (flag & 1) {
				uncompr[act_uncomprlen++] = compr[curbyte++];
				uncompr[act_uncomprlen++] = compr[curbyte++];
				uncompr[act_uncomprlen++] = compr[curbyte++];
				uncompr[act_uncomprlen++] = 0xff;
			} else {
				DWORD count = compr[curbyte++];
				DWORD offset = (compr[curbyte++] << 4) | (count >> 4);
				count = (count & 0xf) + 1;
				offset *= 4;
				for (DWORD i = 0; i < count; ++i) {
					*(u32 *)&uncompr[act_uncomprlen] = *(u32 *)&uncompr[act_uncomprlen - offset];
					act_uncomprlen += 4;
				}
			}
			flag >>= 1;
			--bit_count;
		}
		*ret_uncompr = uncompr;
		*ret_uncomprlen = uncomprlen;
	} else {
		BYTE *actual_data = new BYTE[comprlen];
		if (!actual_data)
			return -CUI_EMEM;
		memcpy(actual_data, compr + 8, comprlen);
		*ret_uncompr = actual_data;
		*ret_uncomprlen = comprlen;
	}

	return 0;
}

static int RealLive_g00_type1_uncompress(BYTE *compr, BYTE **ret_uncompr, 
										 DWORD *ret_uncomprlen)
{
	DWORD comprlen = *(u32 *)compr - 8;
	DWORD uncomprlen = *(u32 *)&compr[4];
	if (uncomprlen) {
		BYTE *uncompr = new BYTE[uncomprlen + 64];
		if (!uncompr)
			return -CUI_EMEM;
		memset(uncompr, 0, uncomprlen + 64);

		comprlen += 8;
		DWORD curbyte = 8;
		DWORD act_uncomprlen = 0;
		DWORD bit_count = 0;
		DWORD flag;
		while (act_uncomprlen < uncomprlen && curbyte < comprlen) {
			if (!bit_count) {
				flag = compr[curbyte++];
				bit_count = 8;
			}
			if (flag & 1)
				uncompr[act_uncomprlen++] = compr[curbyte++];
			else {
				DWORD count = compr[curbyte++];
				DWORD offset = (compr[curbyte++] << 4) | (count >> 4);
				count = (count & 0xf) + 2;
				for (DWORD i = 0; i < count; ++i) {
					uncompr[act_uncomprlen] = uncompr[act_uncomprlen - offset];
					++act_uncomprlen;
				}
			}
			flag >>= 1;
			--bit_count;
		}
		*ret_uncompr = uncompr;
		*ret_uncomprlen = uncomprlen;
	} else {
		BYTE *actual_data = new BYTE[comprlen];
		if (!actual_data)
			return -CUI_EMEM;
		memcpy(actual_data, compr + 8, comprlen);
		*ret_uncompr = actual_data;
		*ret_uncomprlen = comprlen;
	}

	return 0;
}

/********************* cgm *********************/

static int RealLive_cgm_match(struct package *pkg)
{
	s8 magic[16];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "CGTABLE", 16)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int RealLive_cgm_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	u32 cgm_size;
	if (pkg->pio->length_of(pkg, &cgm_size))
		return -CUI_ELEN;

	BYTE *cgm = new BYTE[cgm_size];
	if (!cgm)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, cgm, cgm_size, 0, IO_SEEK_SET)) {
		delete [] cgm;
		return -CUI_EREADVEC;
	}

	DWORD comprlen = cgm_size - sizeof(cgm_header_t);
	BYTE *compr = cgm + sizeof(cgm_header_t);
	RealLive_decode(compr, comprlen);
	if (RealLive_uncompress(compr, (BYTE **)&pkg_res->actual_data,
			&pkg_res->actual_data_length)) {
		delete [] cgm;
		return -CUI_EMEM;
	}
	delete [] cgm;

	return 0;
}

static int RealLive_cgm_save_resource(struct resource *res, 
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

static void RealLive_cgm_release_resource(struct package *pkg, 
										  struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		delete [] pkg_res->actual_data;
		pkg_res->actual_data = NULL;
	}
	if (pkg_res->raw_data) {
		delete [] pkg_res->raw_data;
		pkg_res->raw_data = NULL;
	}
}

static void RealLive_cgm_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg);
}

static cui_ext_operation RealLive_cgm_operation = {
	RealLive_cgm_match,					/* match */
	NULL,								/* extract_directory */
	NULL,								/* parse_resource_info */
	RealLive_cgm_extract_resource,		/* extract_resource */
	RealLive_cgm_save_resource,			/* save_resource */
	RealLive_cgm_release_resource,		/* release_resource */
	RealLive_cgm_release				/* release */
};

/********************* dbs *********************/

static int RealLive_dbs_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

static int RealLive_dbs_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	u32 dbs_size;
	if (pkg->pio->length_of(pkg, &dbs_size))
		return -CUI_ELEN;

	BYTE *dbs = new BYTE[dbs_size];
	if (!dbs)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, dbs, dbs_size, 0, IO_SEEK_SET)) {
		delete [] dbs;
		return -CUI_EREADVEC;
	}

	DWORD cnt = (dbs_size - 4) / 4;
	for (DWORD i = 0; i < cnt; ++i)
		((u32 *)dbs)[i + 1] ^= 0x89f4622d;

	DWORD uncomprlen;
	BYTE *uncompr;
	if (RealLive_uncompress(dbs + 4, &uncompr,
			&uncomprlen)) {
		delete [] dbs;
		return -CUI_EMEM;
	}

	cnt = uncomprlen / 4;
	for (i = 0; i < cnt; ++i)
		((u32 *)uncompr)[i] ^= 0x7190c70e;

	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;	
	pkg_res->raw_data = dbs;
	pkg_res->raw_data_length = dbs_size;

	return 0;
}

static cui_ext_operation RealLive_dbs_operation = {
	RealLive_dbs_match,					/* match */
	NULL,								/* extract_directory */
	NULL,								/* parse_resource_info */
	RealLive_dbs_extract_resource,		/* extract_resource */
	RealLive_cgm_save_resource,			/* save_resource */
	RealLive_cgm_release_resource,		/* release_resource */
	RealLive_cgm_release				/* release */
};


/********************* dat *********************/

static int RealLive_dat_match(struct package *pkg)
{
	s8 magic[16];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "FILESET", 16)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int RealLive_dat_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	u32 dat_size;
	if (pkg->pio->length_of(pkg, &dat_size))
		return -CUI_ELEN;

	BYTE *dat = new BYTE[dat_size];
	if (!dat)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, dat, dat_size, 0, IO_SEEK_SET)) {
		delete [] dat;
		return -CUI_EREADVEC;
	}

	DWORD comprlen = dat_size - sizeof(dat_header_t);
	BYTE *compr = dat + sizeof(dat_header_t);
	RealLive_decode(compr, comprlen);
	if (RealLive_uncompress(compr, (BYTE **)&pkg_res->actual_data,
			&pkg_res->actual_data_length)) {
		delete [] dat;
		return -CUI_EMEM;
	}
	delete [] dat;

	return 0;
}

static cui_ext_operation RealLive_dat_operation = {
	RealLive_dat_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	RealLive_dat_extract_resource,	/* extract_resource */
	RealLive_cgm_save_resource,		/* save_resource */
	RealLive_cgm_release_resource,	/* release_resource */
	RealLive_cgm_release			/* release */
};

/********************* seen.txt *********************/

static int RealLive_seen_match(struct package *pkg)
{
	if (_tcsicmp(pkg->name, _T("Seen.txt")))
		return -CUI_EMATCH;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包索引目录提取函数 */
static int RealLive_seen_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{
	DWORD seen_index_length = 10000 * sizeof(seen_entry_t);
	seen_entry_t *seen_index = new seen_entry_t[10000];
	if (!seen_index)
		return -CUI_EMEM;

	// read SEEN_PACK_HEADER
	if (pkg->pio->read(pkg, seen_index, seen_index_length)) {
		delete [] seen_index;
		return -CUI_EREAD;
	}

	DWORD index_entries = 0;
	for (DWORD i = 0; i < 10000; ++i) {
		if (seen_index[i].length)
			++index_entries;
	}

	RealLive_generic_entry_t *gen_index = new RealLive_generic_entry_t[index_entries];
	if (!gen_index) {
		delete [] seen_index;
		return -CUI_EMEM;
	}

	DWORD k = 0;
	for (i = 0; i < 10000 && k < index_entries; ++i) {
		if (seen_index[i].length) {
			sprintf(gen_index[k].name, "Seen%04d.txt", i);
			gen_index[k].offset = seen_index[i].offset;
			gen_index[k++].length = seen_index[i].length;
		}
	}
	delete [] seen_index;

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = gen_index;
	pkg_dir->directory_length = sizeof(RealLive_generic_entry_t) * index_entries;
	pkg_dir->index_entry_length = sizeof(RealLive_generic_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int RealLive_seen_parse_resource_info(struct package *pkg,
											 struct package_resource *pkg_res)
{
	RealLive_generic_entry_t *gen_entry;

	gen_entry = (RealLive_generic_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, gen_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = gen_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = gen_entry->offset;

	return 0;
}

static int RealLive_seen_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	// process SEEN_DATA
	pkg_res->raw_data = malloc(pkg_res->raw_data_length);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EREADVEC;
	}

	return 0;
}

static cui_ext_operation RealLive_seen_operation = {
	RealLive_seen_match,				/* match */
	RealLive_seen_extract_directory,	/* extract_directory */
	RealLive_seen_parse_resource_info,	/* parse_resource_info */
	RealLive_seen_extract_resource,		/* extract_resource */
	RealLive_cgm_save_resource,			/* save_resource */
	RealLive_cgm_release_resource,		/* release_resource */
	RealLive_cgm_release				/* release */
};

/********************* Seen%04d.txt *********************/

static int RealLive_txt_match(struct package *pkg)
{
	char pkg_name[MAX_PATH];
	unicode2acp(pkg_name, MAX_PATH, pkg->name, -1);

	unsigned int id;
	if (sscanf(pkg_name, "Seen%04d.txt", &id) != 1)
		return -CUI_EMATCH;

	if (id >= 10000)
		return -CUI_EMATCH;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	u32 header_size;
	if (pkg->pio->read(pkg, &header_size, 4)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (header_size != 0x1d0) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int RealLive_txt_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	u32 txt_size;
	if (pkg->pio->length_of(pkg, &txt_size))
		return -CUI_ELEN;

	BYTE *txt = new BYTE[txt_size];
	if (!txt)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, txt, txt_size, 0, IO_SEEK_SET)) {
		delete [] txt;
		return -CUI_EREADVEC;
	}

	seen_data_header_t *sd_head = (seen_data_header_t *)txt;
	if ((sd_head->compile_version % 100) == 2) {
		BYTE *compr = txt + sd_head->instruction_offset;
		RealLive_decode(compr, sd_head->instruction_size);
		BYTE *SEEN_DATA_ORGINAL;
		DWORD SEEN_DATA_ORGINAL_size;
		if (RealLive_uncompress(compr, &SEEN_DATA_ORGINAL,
				&SEEN_DATA_ORGINAL_size)) {
			delete [] txt;
			return -CUI_EMEM;
		}

		BYTE *SEEN_DATA = new BYTE[sd_head->instruction_offset + SEEN_DATA_ORGINAL_size];
		if (!SEEN_DATA) {
			delete [] SEEN_DATA_ORGINAL;
			delete [] txt;
			return -CUI_EMEM;
		}
		memcpy(SEEN_DATA, txt, sd_head->instruction_offset);
		memcpy(SEEN_DATA + sd_head->instruction_offset, SEEN_DATA_ORGINAL,
			SEEN_DATA_ORGINAL_size);
		pkg_res->actual_data = SEEN_DATA;
		pkg_res->actual_data_length = sd_head->instruction_offset + SEEN_DATA_ORGINAL_size;
		delete [] SEEN_DATA_ORGINAL;
		delete [] txt;
	} else {
		delete [] txt;
		return -CUI_EMATCH;
	}

	return 0;
}

static cui_ext_operation RealLive_txt_operation = {
	RealLive_txt_match,					/* match */
	NULL,								/* extract_directory */
	NULL,								/* parse_resource_info */
	RealLive_txt_extract_resource,		/* extract_resource */
	RealLive_cgm_save_resource,			/* save_resource */
	RealLive_cgm_release_resource,		/* release_resource */
	RealLive_cgm_release				/* release */
};

/********************* g00 *********************/

static int RealLive_g00_match(struct package *pkg)
{
	u8 type;
	u32 length;
	DWORD offset;
	u32 g00_size;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->length_of(pkg, &g00_size)) {
		pkg->pio->close(pkg);
		return -CUI_ELEN;
	}

	if (pkg->pio->read(pkg, &type, 1)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (type > 2) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (type == 2) {
		if (pkg->pio->readvec(pkg, &length, 4, 5, IO_SEEK_SET)) {
			pkg->pio->close(pkg);
			return -CUI_EREAD;
		}

		offset = 9 + length * 24;
		if (pkg->pio->readvec(pkg, &length, 4, offset, IO_SEEK_SET)) {
			pkg->pio->close(pkg);
			return -CUI_EREAD;
		}

		u32 index_entries;
		if (pkg->pio->readvec(pkg, &index_entries, 4, sizeof(g00_header_t), IO_SEEK_SET)) {
			pkg->pio->close(pkg);
			return -CUI_EREADVEC;
		}

		if (index_entries > 1) {
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}
	} else {
		if (pkg->pio->readvec(pkg, &length, 4, 5, IO_SEEK_SET)) {
			pkg->pio->close(pkg);
			return -CUI_EREAD;
		}
		offset = 5;
	}

	if (g00_size - offset != length) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->pio->seek(pkg, offset, IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_ESEEK;
	}

	return 0;	
}

static int RealLive_g00_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	u32 g00_size;
	if (pkg->pio->length_of(pkg, &g00_size))
		return -CUI_ELEN;

	u32 offset;
	if (pkg->pio->locate(pkg, &offset))
		return -CUI_ELOC;

	DWORD comprlen = g00_size - offset;
	BYTE *compr = new BYTE[comprlen];
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, offset, IO_SEEK_SET)) {
		delete [] compr;
		return -CUI_EREADVEC;
	}

	g00_header_t g00_header;
	if (pkg->pio->readvec(pkg, &g00_header, sizeof(g00_header), 0, IO_SEEK_SET)) {
		delete [] compr;
		return -CUI_EREADVEC;
	}

	BYTE *rgba;
	DWORD rgba_len;
	if (g00_header.type == 0) {
		if (RealLive_g00_type0_uncompress(compr, &rgba, &rgba_len)) {
			delete [] compr;
			return -CUI_EMEM;
		}
	} else if ((g00_header.type == 1) || (g00_header.type == 2)) {
		if (RealLive_g00_type1_uncompress(compr, &rgba, &rgba_len)) {
			delete [] compr;
			return -CUI_EMEM;
		}
	}
	delete [] compr;

	if (g00_header.type == 0) {
		if (MyBuildBMPFile(rgba, rgba_len, NULL, 0, 
				g00_header.width, -g00_header.height, 32,
				(BYTE **)&pkg_res->actual_data,	
				&pkg_res->actual_data_length, my_malloc)) {
			delete [] rgba;
			return -CUI_EMEM;		
		}
		delete [] rgba;
	} else if (g00_header.type == 1) {	// to test and get palette
		u16 colors = *(u16 *)rgba;
		u32 *palette = (u32 *)(rgba + 2);
		if (MySaveAsBMP(rgba + 2 + colors * 4, g00_header.width * g00_header.height,
			(BYTE *)palette, colors * 4, g00_header.width, -g00_header.height, 8,
			colors, (BYTE **)&pkg_res->actual_data,	
			&pkg_res->actual_data_length, my_malloc)) {
			delete [] rgba;
			return -CUI_EMEM;		
		}
		delete [] rgba;
	} else if (g00_header.type == 2) {
		u32 index_entries;
		if (pkg->pio->readvec(pkg, &index_entries, 4, sizeof(g00_header), IO_SEEK_SET)) {
			delete [] rgba;
			return -CUI_EREADVEC;
		}

		g02_info_t *info = new g02_info_t[index_entries];
		if (!info) {
			delete [] rgba;
			return -CUI_EMEM;
		}
		if (pkg->pio->read(pkg, info, sizeof(g02_info_t) * index_entries)) {
			delete [] info;			
			delete [] rgba;
			return -CUI_EREADVEC;
		}

		g02_header_t *g02_header = (g02_header_t *)rgba;
		DWORD g02_size = rgba_len;
		DWORD line_len = g00_header.width * 4;
		DWORD dib_len = line_len * g00_header.height;
		BYTE *dib = new BYTE[dib_len];
		if (!dib) {
			delete [] info;
			delete [] rgba;
			return -CUI_EMEM;
		}
		memset(dib, 0, dib_len);
		
		if (debug)
			printf("%s: parts %d, width %d, height %d\n", 
				pkg_res->name, index_entries, g00_header.width,
				g00_header.height);

		BYTE *g02 = rgba;
		g02_entry_t *entry = (g02_entry_t *)(g02 + sizeof(g02_header_t));
		for (DWORD i = 0; i < index_entries; ++i) {
			g02p_header_t *p_header = (g02p_header_t *)(g02 + entry->offset);
			int negative = (int)entry->length < 0;
			if (negative)
				entry->length = 0 - entry->length;

			if (debug)
				printf("  [part %03d] blocks %d, width %d, height %d(%c), pos (%d, %d)-(%d, %d)\n", 
					i, p_header->blocks, p_header->width, p_header->height,
					negative ? 'N' : 'P', info[i].orig_x, info[i].orig_y, 
					info[i].end_x, info[i].end_y);

			if (entry->length) {
				g02b_header_t *b_header = (g02b_header_t *)(p_header + 1);
				BYTE *block_dib = (BYTE *)(b_header + 1);
				BYTE *part = dib + info[i].orig_y * line_len + info[i].orig_x * 4;
				for (DWORD j = 0; j < p_header->blocks; ++j) {
					if (debug)
						printf("    [block %03d] width %d, height %d, flag 0x%x, (%d, %d)\n", 
							j, b_header->width, b_header->height,
							b_header->flag, b_header->start_x, 
							b_header->start_y);	

					DWORD block_line_len = b_header->width * 4;
					DWORD block_dib_len = block_line_len * b_header->height;				
					BYTE *line = part + b_header->start_y * line_len + b_header->start_x * 4;
					if (!b_header->flag) {
						for (DWORD y = 0; y < b_header->height; ++y) {
							for (DWORD x = 0; x < b_header->width; ++x) {
								*line++ = *block_dib++;
								*line++ = *block_dib++;
								*line++ = *block_dib++;
								*line++ = *block_dib++;
							}
							line += line_len - block_line_len;
						}
					} else {
#if 0
						block_dib += block_line_len * b_header->height;
#else
						BYTE *dst = line;
						for (DWORD y = 0; y < b_header->height; ++y) {														
							for (DWORD x = 0; x < b_header->width; ++x) {
								BYTE a = block_dib[3];
								if (a) {
									if (a == 0xff) {
										*dst++ = *block_dib++;
										*dst++ = *block_dib++;
										*dst++ = *block_dib++;
										*dst++ = *block_dib++;
									} else {
										DWORD *adr = CLR_TABLE_ADR[a];
										*dst += (BYTE)adr[*block_dib++ - *dst];
										++dst;
										*dst += (BYTE)adr[*block_dib++ - *dst];
										++dst;
										*dst += (BYTE)adr[*block_dib++ - *dst];
										++dst;										
										*dst++ = *block_dib++;
									}									
								} else {
									block_dib += 4;
									dst += 4;
								}
							}
							dst += line_len - block_line_len;							
						}
#endif
					}
					b_header = (g02b_header_t *)block_dib;
					block_dib = (BYTE *)(b_header + 1);
				}
			}
			++entry;
		}
		delete [] info;
		delete [] rgba;

		if (MyBuildBMPFile(dib, dib_len, NULL, 0, g00_header.width, 
				-g00_header.height, 32, (BYTE **)&pkg_res->actual_data,	
				&pkg_res->actual_data_length, my_malloc)) {
			delete [] dib;
			return -CUI_EMEM;		
		}
		delete [] dib;
	}

	return 0;
}

static cui_ext_operation RealLive_g00_operation = {
	RealLive_g00_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	RealLive_g00_extract_resource,	/* extract_resource */
	RealLive_cgm_save_resource,		/* save_resource */
	RealLive_cgm_release_resource,	/* release_resource */
	RealLive_cgm_release			/* release */
};

/********************* g00 *********************/

static int RealLive_g00_2_dir_match(struct package *pkg)
{
	u8 type;
	u32 length;
	DWORD offset;
	u32 g00_size;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->length_of(pkg, &g00_size)) {
		pkg->pio->close(pkg);
		return -CUI_ELEN;
	}

	if (pkg->pio->read(pkg, &type, 1)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (type != 2) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->pio->readvec(pkg, &length, 4, 5, IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	offset = 9 + length * 24;
	if (pkg->pio->readvec(pkg, &length, 4, offset, IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	u32 index_entries;
	if (pkg->pio->readvec(pkg, &index_entries, 4, sizeof(g00_header_t), IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_EREADVEC;
	}

	if (index_entries <= 1) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (g00_size - offset != length) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->pio->seek(pkg, offset, IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_ESEEK;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int RealLive_g00_2_dir_extract_directory(struct package *pkg,
												struct package_directory *pkg_dir)
{
	g00_header_t g00_header;
	if (pkg->pio->readvec(pkg, &g00_header, sizeof(g00_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	u32 index_entries;
	if (pkg->pio->read(pkg, &index_entries, 4))
		return -CUI_EREAD;

	g02_info_t *info = new g02_info_t[index_entries];
	if (!info)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, info, sizeof(g02_info_t) * index_entries)) {
		delete [] info;			
		return -CUI_EREAD;
	}

	u32 offset;
	pkg->pio->locate(pkg, &offset);
	package_set_private(pkg, offset);

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = info;
	pkg_dir->directory_length = sizeof(g02_info_t) * index_entries;
	pkg_dir->index_entry_length = sizeof(g02_info_t);

	return 0;
}

/* 封包索引项解析函数 */
static int RealLive_g00_2_dir_resource_info(struct package *pkg,
											struct package_resource *pkg_res)
{
	g02_info_t *g02_info;

	g02_info = (g02_info_t *)pkg_res->actual_index_entry;
	sprintf(pkg_res->name, "%04d", pkg_res->index_number);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = 1;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = package_get_private(pkg);

	return 0;
}

static int RealLive_g00_2_dir_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	u32 g00_size;
	if (pkg->pio->length_of(pkg, &g00_size))
		return -CUI_ELEN;

	u32 offset = (u32)package_get_private(pkg);

	DWORD comprlen = g00_size - offset;
	pkg_res->raw_data_length = comprlen;
	BYTE *compr = new BYTE[comprlen];
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, offset, IO_SEEK_SET)) {
		delete [] compr;
		return -CUI_EREADVEC;
	}

	g00_header_t g00_header;
	if (pkg->pio->readvec(pkg, &g00_header, sizeof(g00_header), 0, IO_SEEK_SET)) {
		delete [] compr;
		return -CUI_EREADVEC;
	}

	g02_info_t *info = (g02_info_t *)pkg_res->actual_index_entry;

	BYTE *rgba;
	DWORD rgba_len;
	if (RealLive_g00_type1_uncompress(compr, &rgba, &rgba_len)) {
		delete [] compr;
		return -CUI_EMEM;
	}
	delete [] compr;

	DWORD g02_size = rgba_len;
	DWORD line_len = g00_header.width * 4;
	DWORD dib_len = line_len * g00_header.height;
	BYTE *dib = new BYTE[dib_len];
	if (!dib) {
		delete [] rgba;
		return -CUI_EMEM;
	}
	memset(dib, 0, dib_len);

	DWORD i = pkg_res->index_number;
	
	if (debug)
		printf("%s: part %d, width %d, height %d\n", 
			pkg_res->name, i, g00_header.width,
			g00_header.height);

	BYTE *g02 = rgba;
	g02_entry_t *entry = (g02_entry_t *)(g02 + sizeof(g02_header_t)) + i;
	g02p_header_t *p_header = (g02p_header_t *)(g02 + entry->offset);
	int negative = (int)entry->length < 0;
	if (negative)
		entry->length = 0 - entry->length;

	if (debug)
		printf("  [part %03d] blocks %d, width %d, height %d(%c), pos (%d, %d)-(%d, %d)\n", 
			i, p_header->blocks, p_header->width, p_header->height,
			negative ? 'N' : 'P', info->orig_x, info->orig_y, 
			info->end_x, info->end_y);

	if (entry->length) {
		g02b_header_t *b_header = (g02b_header_t *)(p_header + 1);
		BYTE *block_dib = (BYTE *)(b_header + 1);
		BYTE *part = dib + info->orig_y * line_len + info->orig_x * 4;
		for (DWORD j = 0; j < p_header->blocks; ++j) {
			if (debug)
				printf("    [block %03d] width %d, height %d, flag 0x%x, (%d, %d)\n", 
					j, b_header->width, b_header->height,
					b_header->flag, b_header->start_x, 
					b_header->start_y);	

			DWORD block_line_len = b_header->width * 4;
			DWORD block_dib_len = block_line_len * b_header->height;				
			BYTE *line = part + b_header->start_y * line_len + b_header->start_x * 4;
			if (!b_header->flag) {
				for (DWORD y = 0; y < b_header->height; ++y) {
					for (DWORD x = 0; x < b_header->width; ++x) {
						*line++ = *block_dib++;
						*line++ = *block_dib++;
						*line++ = *block_dib++;
						*line++ = *block_dib++;
					}
					line += line_len - block_line_len;
				}
			} else {
#if 0
				block_dib += block_line_len * b_header->height;
#else
				BYTE *dst = line;
				for (DWORD y = 0; y < b_header->height; ++y) {														
					for (DWORD x = 0; x < b_header->width; ++x) {
						BYTE a = block_dib[3];
						if (a) {
							if (a == 0xff) {
								*dst++ = *block_dib++;
								*dst++ = *block_dib++;
								*dst++ = *block_dib++;
								*dst++ = *block_dib++;
							} else {
								DWORD *adr = CLR_TABLE_ADR[a];
								*dst += (BYTE)adr[*block_dib++ - *dst];
								++dst;
								*dst += (BYTE)adr[*block_dib++ - *dst];
								++dst;
								*dst += (BYTE)adr[*block_dib++ - *dst];
								++dst;										
								*dst++ = *block_dib++;
							}									
						} else {
							block_dib += 4;
							dst += 4;
						}
					}
					dst += line_len - block_line_len;							
				}
#endif
			}
			b_header = (g02b_header_t *)block_dib;
			block_dib = (BYTE *)(b_header + 1);
		}
	}
	delete [] rgba;

	if (MyBuildBMPFile(dib, dib_len, NULL, 0, g00_header.width, 
			-g00_header.height, 32, (BYTE **)&pkg_res->actual_data,	
			&pkg_res->actual_data_length, my_malloc)) {
		delete [] dib;
		return -CUI_EMEM;
	}
	delete [] dib;

	return 0;
}

static cui_ext_operation RealLive_g00_2_dir_operation = {
	RealLive_g00_2_dir_match,				/* match */
	RealLive_g00_2_dir_extract_directory,	/* extract_directory */
	RealLive_g00_2_dir_resource_info,		/* parse_resource_info */
	RealLive_g00_2_dir_extract_resource,	/* extract_resource */
	RealLive_cgm_save_resource,				/* save_resource */
	RealLive_cgm_release_resource,			/* release_resource */
	RealLive_cgm_release					/* release */
};

#if 0	// 没有实际数据
/********************* gan *********************/

static int RealLive_gan_match(struct package *pkg)
{
	u32 code[2];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, code, sizeof(code))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (code[0] != 10000 || code[1] != 10000) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;
}

/* 封包索引目录提取函数 */
static int RealLive_gan_extract_directory(struct package *pkg,
										  struct package_directory *pkg_dir)
{
	u32 code;

	while (1) {
		if (pkg->pio->read(pkg, &code, 4))
			return -CUI_EREAD;

		if (code == 20000)
			break;	
		else if (code == 10100) {
			s32 name_len;

			if (pkg->pio->read(pkg, &name_len, 4))
				return -CUI_EREAD;

			if (pkg->pio->seek(pkg, name_len, IO_SEEK_CUR))
				return -CUI_EREAD;
		} else
			return -CUI_EMATCH;
	}

	s32 G00ANIM_SET_count;
	if (pkg->pio->read(pkg, &G00ANIM_SET_count, 4))
		return -CUI_EREAD;

	if (G00ANIM_SET_count <= 0)
		return -CUI_EMATCH;
printf("%x\n",G00ANIM_SET_count);
	while (1) {
		if (pkg->pio->read(pkg, &code, 4))
			return -CUI_EREAD;

		if (code == 30000)
			break;
		else if (code != -99999)
			return -CUI_EMATCH;
	}

	RealLive_generic_entry_t *gen_index = new RealLive_generic_entry_t[G00ANIM_SET_count];
	if (!gen_index)
		return -CUI_EMEM;

	s32 G00ANIME_PAT_count;
	if (pkg->pio->read(pkg, &G00ANIME_PAT_count, 4)) {
		delete [] gen_index;
		return -CUI_EREAD;
	}

	if (G00ANIME_PAT_count <= 0) {
		delete [] gen_index;
		return -CUI_EMATCH;
	}

	gan_pat_t *gan_pat = new gan_pat_t[G00ANIME_PAT_count];
	if (!gan_pat) {
		delete [] gen_index;
		return -CUI_EMEM;
	}

	for (s32 i = 0; i < G00ANIME_PAT_count; ++i) {
		while (1) {
			if (pkg->pio->read(pkg, &code, 4)) {
				delete [] gan_pat;
				delete [] gen_index;
				return -CUI_EREAD;
			}

			if (code == 999999)
				break;

			u32 value;
			if (pkg->pio->read(pkg, &value, 4)) {
				delete [] gan_pat;
				delete [] gen_index;
				return -CUI_EREAD;
			}

			if (code == 30100)
				gan_pat[i].unknown2 = value;
			else if (code == 30101)
				gan_pat[i].unknown3 = value;
			else if (code == 30102)
				gan_pat[i].unknown4 = value;
			else if (code == 30103)
				gan_pat[i].unknown5 = value;
			else if (code == 30104)
				gan_pat[i].unknown6 = value;
			else if (code == 30105)
				gan_pat[i].unknown7 = value;
			else {
				delete [] gan_pat;
				delete [] gen_index;
				return -CUI_EREAD;
			}			
		}
	}

	for (i = 0; i < G00ANIME_PAT_count; ++i)
		printf("%x %x %x %x %x %x\n",
			gan_pat[i].unknown2,gan_pat[i].unknown3,
			gan_pat[i].unknown4,gan_pat[i].unknown5,
			gan_pat[i].unknown6,gan_pat[i].unknown7);

	pkg_dir->index_entries = G00ANIM_SET_count;
	pkg_dir->directory = gen_index;
	pkg_dir->directory_length = sizeof(RealLive_generic_entry_t) * G00ANIM_SET_count;
	pkg_dir->index_entry_length = sizeof(RealLive_generic_entry_t);
//	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

static int RealLive_gan_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	int reverse;
	DWORD g02p_size;
	if ((int)pkg_res->raw_data_length < 0) {
		reverse = 1;
		g02p_size = 0 - pkg_res->raw_data_length;
	} else {
		reverse = 0;
		g02p_size = pkg_res->raw_data_length;
	}

	BYTE *g02p = new BYTE[g02p_size];
	if (!g02p)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, g02p, g02p_size, 
			pkg_res->offset, IO_SEEK_SET)) {
		delete [] g02p;
		return -CUI_EREADVEC;
	}

	pkg_res->raw_data = g02p;
	pkg_res->raw_data_length = g02p_size;

	return 0;
}

static cui_ext_operation RealLive_gan_operation = {
	RealLive_gan_match,					/* match */
	RealLive_gan_extract_directory,		/* extract_directory */
	RealLive_seen_parse_resource_info,	/* parse_resource_info */
	RealLive_gan_extract_resource,		/* extract_resource */
	RealLive_cgm_save_resource,			/* save_resource */
	RealLive_cgm_release_resource,		/* release_resource */
	RealLive_cgm_release				/* release */
};

/********************* g02p *********************/

static int RealLive_g02p_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;
}

/* 封包索引目录提取函数 */
static int RealLive_g02p_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{
	g02p_header_t g02p_header;
	if (pkg->pio->read(pkg, &g02p_header, sizeof(g02p_header)))
		return -CUI_EREAD;

	DWORD g02b_index_length = g02p_header.blocks * sizeof(RealLive_generic_entry_t);
	RealLive_generic_entry_t *g02b_index = new RealLive_generic_entry_t[g02p_header.blocks];
	if (!g02b_index)
		return -CUI_EMEM;

	char pkg_name[MAX_PATH];
	unicode2acp(pkg_name, MAX_PATH, pkg->name, -1);
	*strstr(pkg_name, ".g02p") = 0;
	DWORD offset = sizeof(g02p_header_t);
	for (DWORD i = 0; i < g02p_header.blocks; ++i) {
		g02b_header_t g02b;
		if (pkg->pio->readvec(pkg, &g02b, sizeof(g02b), offset, IO_SEEK_SET)) {
			delete [] g02b_index;
			return -CUI_EREADVEC;
		}
		sprintf(g02b_index[i].name, "%s_B%04d", pkg_name, i);
		g02b_index[i].offset = offset;
		g02b_index[i].length = sizeof(g02b_header_t) + 4 * g02b.width * g02b.height;
		offset += g02b_index[i].length;
	}

	pkg_dir->index_entries = g02p_header.blocks;
	pkg_dir->directory = g02b_index;
	pkg_dir->directory_length = sizeof(RealLive_generic_entry_t) * g02p_header.blocks;
	pkg_dir->index_entry_length = sizeof(RealLive_generic_entry_t);
//	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

static int RealLive_g02p_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	g02b_header_t g02b;
	if (pkg->pio->readvec(pkg, &g02b, sizeof(g02b), 
			pkg_res->offset, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD dib_len = g02b.width * g02b.height * 4;
	BYTE *dib = new BYTE[dib_len];
	if (!dib)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, dib, dib_len)) {
		delete [] dib;
		return -CUI_EREAD;
	}

	if (MyBuildBMPFile(dib, dib_len, NULL, 0, g02b.width,
			-g02b.height, 32, (BYTE **)&pkg_res->actual_data, 
			&pkg_res->actual_data_length, my_malloc)) {
		delete [] dib;
		return -CUI_EMEM;
	}
	delete [] dib;

	return 0;
}

static cui_ext_operation RealLive_g02p_operation = {
	RealLive_g02p_match,				/* match */
	RealLive_g02p_extract_directory,	/* extract_directory */
	RealLive_seen_parse_resource_info,	/* parse_resource_info */
	RealLive_g02p_extract_resource,		/* extract_resource */
	RealLive_cgm_save_resource,			/* save_resource */
	RealLive_cgm_release_resource,		/* release_resource */
	RealLive_cgm_release				/* release */
};
#endif

/********************* ovk *********************/

static int RealLive_ovk_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

static int RealLive_ovk_extract_directory(struct package *pkg,
										  struct package_directory *pkg_dir)
{
	ovk_header_t ovk_header;
	if (pkg->pio->readvec(pkg, &ovk_header, sizeof(ovk_header), 
			0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	ovk_entry_t *ovk_index = new ovk_entry_t[ovk_header.entries];
	if (!ovk_index)
		return -CUI_EMEM;

	DWORD ovk_index_len = sizeof(ovk_entry_t) * ovk_header.entries;
	if (pkg->pio->read(pkg, ovk_index, ovk_index_len)) {
		delete [] ovk_index;
		return -CUI_EREAD;
	}

	RealLive_generic_entry_t *index = new RealLive_generic_entry_t[ovk_header.entries];
	if (!index) {
		delete [] ovk_index;
		return -CUI_EREAD;		
	}

	for (DWORD i = 0; i < ovk_header.entries; ++i) {
		sprintf(index[i].name, "%04d", ovk_index[i].id);
		index[i].offset = ovk_index[i].offset;
		index[i].length = ovk_index[i].length;
	}
	delete [] ovk_index;

	pkg_dir->index_entries = ovk_header.entries;
	pkg_dir->directory = index;
	pkg_dir->directory_length = sizeof(RealLive_generic_entry_t) * ovk_header.entries;
	pkg_dir->index_entry_length = sizeof(RealLive_generic_entry_t);

	return 0;
}

static int RealLive_ovk_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	BYTE *ovk = new BYTE[pkg_res->raw_data_length];
	if (!ovk)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, ovk, pkg_res->raw_data_length, 
			pkg_res->offset, IO_SEEK_SET)) {
		delete [] ovk;
		return -CUI_EREADVEC;
	}
	pkg_res->raw_data = ovk;

	return 0;
}

static cui_ext_operation RealLive_ovk_operation = {
	RealLive_ovk_match,					/* match */
	RealLive_ovk_extract_directory,		/* extract_directory */
	RealLive_seen_parse_resource_info,	/* parse_resource_info */
	RealLive_ovk_extract_resource,		/* extract_resource */
	RealLive_cgm_save_resource,			/* save_resource */
	RealLive_cgm_release_resource,		/* release_resource */
	RealLive_cgm_release				/* release */
};

/********************* nwk *********************/

static int RealLive_nwk_extract_directory(struct package *pkg,
										  struct package_directory *pkg_dir)
{
	nwk_header_t nwk_header;
	if (pkg->pio->readvec(pkg, &nwk_header, sizeof(nwk_header), 
			0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	nwk_entry_t *nwk_index = new nwk_entry_t[nwk_header.entries];
	if (!nwk_index)
		return -CUI_EMEM;

	DWORD nwk_index_len = sizeof(nwk_entry_t) * nwk_header.entries;
	if (pkg->pio->read(pkg, nwk_index, nwk_index_len)) {
		delete [] nwk_index;
		return -CUI_EREAD;
	}

	RealLive_generic_entry_t *index = new RealLive_generic_entry_t[nwk_header.entries];
	if (!index) {
		delete [] nwk_index;
		return -CUI_EREAD;		
	}

	for (DWORD i = 0; i < nwk_header.entries; ++i) {
		sprintf(index[i].name, "%04d", nwk_index[i].id);
		index[i].offset = nwk_index[i].offset;
		index[i].length = nwk_index[i].length;
	}
	delete [] nwk_index;

	pkg_dir->index_entries = nwk_header.entries;
	pkg_dir->directory = index;
	pkg_dir->directory_length = sizeof(RealLive_generic_entry_t) * nwk_header.entries;
	pkg_dir->index_entry_length = sizeof(RealLive_generic_entry_t);

	return 0;
}

static int RealLive_nwk_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	BYTE *nwk = new BYTE[pkg_res->raw_data_length];
	if (!nwk)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, nwk, pkg_res->raw_data_length, 
			pkg_res->offset, IO_SEEK_SET)) {
		delete [] nwk;
		return -CUI_EREADVEC;
	}
	pkg_res->raw_data = nwk;

	return 0;
}

static cui_ext_operation RealLive_nwk_operation = {
	RealLive_ovk_match,					/* match */
	RealLive_nwk_extract_directory,		/* extract_directory */
	RealLive_seen_parse_resource_info,	/* parse_resource_info */
	RealLive_nwk_extract_resource,		/* extract_resource */
	RealLive_cgm_save_resource,			/* save_resource */
	RealLive_cgm_release_resource,		/* release_resource */
	RealLive_cgm_release				/* release */
};

/********************* nwa *********************/

static int RealLive_nwa_match(struct package *pkg)
{
	nwa_header_t nwa_header;
	u32 nwa_size;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->length_of(pkg, &nwa_size)) {
		pkg->pio->close(pkg);
		return -CUI_ELEN;
	}

	if (pkg->pio->read(pkg, &nwa_header, sizeof(nwa_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (nwa_header.compr_level == -1) {	// ?
		nwa_header.block_size = 65536;
		nwa_header.rest_size = (nwa_header.data_size % (nwa_header.block_size * (nwa_header.bps/8))) / (nwa_header.bps/8);
		nwa_header.NWAtable_len = nwa_header.data_size / (nwa_header.block_size * (nwa_header.bps/8)) + (nwa_header.rest_size > 0 ? 1 : 0);
	}

	if (!nwa_header.NWAtable_len || nwa_header.NWAtable_len > 1000000) {// ??
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (nwa_header.channels != 1 && nwa_header.channels != 2) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (nwa_header.bps != 8 && nwa_header.bps != 16) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if ((nwa_header.channels == 2) && (nwa_header.bps != 16)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (nwa_header.compr_level == -1) {	// ??
		int byps = nwa_header.bps / 8; /* bytes per sample */
		if (nwa_header.data_size != nwa_header.sample_count * byps) {
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}

		if (nwa_header.sample_count != (nwa_header.NWAtable_len - 1) * nwa_header.block_size + nwa_header.rest_size) {
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}
	} else {
		if (nwa_header.compr_level < 0 || nwa_header.compr_level > 5) {// ??
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}

		if (nwa_size != nwa_header.compr_data_size) {
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}

		DWORD *NWATable = (DWORD *)malloc(nwa_header.NWAtable_len * 4);
		if (!NWATable) {
			pkg->pio->close(pkg);
			return -CUI_EMEM;
		}

		if (pkg->pio->read(pkg, NWATable, nwa_header.NWAtable_len * 4)) {
			free(NWATable);
			pkg->pio->close(pkg);
			return -CUI_EREAD;
		}

		if (NWATable[nwa_header.NWAtable_len - 1] >= nwa_header.compr_data_size) {
			free(NWATable);
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}

		int byps = nwa_header.bps / 8; /* bytes per sample */
		if (nwa_header.data_size != nwa_header.sample_count * byps) {
			free(NWATable);
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}

#if 0
		if (nwa_header.sample_count != (nwa_header.NWAtable_len - 1) * nwa_header.NWAtable_len + nwa_header.rest_size) {
			free(NWATable);
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}
#endif
		free(NWATable);
	}

	if ((nwa_header.channels != 2 && nwa_header.channels != 1) 
		|| nwa_header.bps != 16 
		|| (nwa_header.compr_level != -1 && nwa_header.compr_level != 0 && nwa_header.compr_level != 2 && nwa_header.compr_level != 1 && nwa_header.compr_level != 5)
		||  !nwa_header.data_size 
		|| nwa_header.UseRunLength) {
			if (debug)
				printf("channel %x, bps %x, level %x, csize %x, dsize %x rl %x\n", 
					nwa_header.channels,
					nwa_header.bps, 
					nwa_header.compr_level,
					nwa_header.compr_data_size,
					nwa_header.data_size,
					nwa_header.UseRunLength);
	}

	return 0;	
}

static int RealLive_nwa_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	u32 nwa_size;
	if (pkg->pio->length_of(pkg, &nwa_size))
		return -CUI_ELEN;
	
	BYTE *nwa = new BYTE[nwa_size];
	if (!nwa)
		return -CUI_EMEM;
	
	if (pkg->pio->readvec(pkg, nwa, nwa_size, 0, IO_SEEK_SET)) {
		delete [] nwa;
		return -CUI_EREADVEC;
	}

	nwa_header_t *nwa_header = (nwa_header_t *)nwa;
	WORD channels = nwa_header->channels;
	WORD bps = nwa_header->bps;
	DWORD freq = nwa_header->freq;
	int compr_level = nwa_header->compr_level;
	DWORD block_size = nwa_header->block_size;

	BYTE *out_data;
	DWORD out_data_len = nwa_header->data_size;
	if (nwa_header->compr_level != -1) {
		u32 *NWAtable = (u32 *)(nwa_header + 1);
		BYTE *pcm_data = new BYTE[nwa_header->data_size];
		if (!pcm_data) {
			delete [] nwa;
			return -CUI_EMEM;
		}
		out_data = pcm_data;
		for (DWORD i = 0; i < nwa_header->NWAtable_len - 1; ++i){
			nwa_expand_block(nwa + NWAtable[i], out_data, 
				channels, bps, compr_level, nwa_header->block_size,
				nwa_header->UseRunLength);
		}
		
		if (nwa_header->rest_size)
			nwa_expand_block(nwa + NWAtable[i], out_data, 
				channels, bps, compr_level, nwa_header->rest_size,
				nwa_header->UseRunLength);
		else
			nwa_expand_block(nwa + NWAtable[i], out_data, 
				channels, bps, compr_level, nwa_header->block_size,
				nwa_header->UseRunLength);		
		if (MySaveAsWAVE(pcm_data, nwa_header->data_size, 1, channels, 
				freq, bps, NULL, 0, (BYTE **)&pkg_res->actual_data, 
				&pkg_res->actual_data_length, my_malloc)) {
			delete [] pcm_data;
			delete [] nwa;
			return -CUI_EMEM;
		}
		delete [] pcm_data;
	} else {
		if (MySaveAsWAVE(nwa + sizeof(nwa_header_t), nwa_header->data_size, 1, channels, 
				freq, bps, NULL, 0, (BYTE **)&pkg_res->actual_data, 
				&pkg_res->actual_data_length, my_malloc)) {
			delete [] nwa;
			return -CUI_EMEM;
		}
	}
	delete [] nwa;
	
	return 0;
}

static cui_ext_operation RealLive_nwa_operation = {
	RealLive_nwa_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	RealLive_nwa_extract_resource,	/* extract_resource */
	RealLive_cgm_save_resource,		/* save_resource */
	RealLive_cgm_release_resource,	/* release_resource */
	RealLive_cgm_release			/* release */
};

int CALLBACK RealLive_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".g00"), _T(".bmp"), 
		NULL, &RealLive_g00_operation, CUI_EXT_FLAG_PKG 
		| CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".txt"), NULL, 
		NULL, &RealLive_seen_operation, CUI_EXT_FLAG_PKG 
		| CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".txt"), NULL, 
		NULL, &RealLive_txt_operation, CUI_EXT_FLAG_PKG
		| CUI_EXT_FLAG_RES | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".nwa"), _T(".wav"), 
		NULL, &RealLive_nwa_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES 
		| CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".ovk"), _T(".ogg"), 
		NULL, &RealLive_ovk_operation, CUI_EXT_FLAG_PKG 
		| CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".ovk"), _T(".ogg"), 
		NULL, &RealLive_ovk_operation, CUI_EXT_FLAG_PKG 
		| CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".cgm"), _T(".cgm_"), 
		NULL, &RealLive_cgm_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), _T(".dat_"), 
		NULL, &RealLive_dat_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".nwk"), _T(".nwa1"), 
		NULL, &RealLive_nwk_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR 
		| CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".g00"), _T(".bmp"), 
		NULL, &RealLive_g00_2_dir_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR
		| CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dbs"), _T(".dbs_"), 
		NULL, &RealLive_dbs_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

#if 0
	if (callback->add_extension(callback->cui, _T(".gan"), _T("G00 ANIME"), 
		_T(""), &RealLive_gan_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;
#endif

	// init CLR_TABLE and CLR_TABLE_ADR
	int val = 0;
	for (DWORD i = 0; i < 256; ++i) {
		int v = val;
		for (DWORD j = 0; j < 511; ++j) {
			CLR_TABLE[i][j] = v / 255;
			v += i;
		}
		val -= 255;
		CLR_TABLE_ADR[i] = &CLR_TABLE[i][255];
	}

	if (get_options2(_T("debug")))
		debug = 1;
	else
		debug = 0;

	return 0;
}
