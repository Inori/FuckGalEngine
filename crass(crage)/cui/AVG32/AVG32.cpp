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

// for test: E:\adieu

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information AVG32_cui_information = {
	_T("Visual Art's"),				/* copyright */
	_T("AVG32/16M/TYPE_D"),			/* (1.0.3.5)system */
	_T(".PDT .CGM SEEN.TXT .KOE"),	/* package */
	_T("2008-7-24 23:45"),			/* revision */
	_T("痴漢公賊"),					/* author */
	_T("0.8.2"),					/* date */
	NULL,							/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[8];		// "ADPACK32"
	u16 minor_version;	// 0
	u16 major_version;	// 1
	u32 index_entries;	// 最后一项是空项
} PAK_header_t;

// 最后一项空项的名字字段全0，crc字段无效，offset等于PAK文件总长度
typedef struct {
	s8 name[26];		// NULL terminated
	u16 name_crc;		// 这个crc是封包时针对63字节的名字而计算出来的
	u32 offset;
} PAK_entry_t;

typedef struct {
	s8 magic[8];		// "PDT10" or "PDT11"
	u32 PDT_size;
	u32 width;
	u32 height;
	u32 alpha_orig_x;
	u32 alpha_orig_y;
	u32 alpha_offset;
} PDT_header_t;

typedef struct {
	s8 magic[8];		// "PACL"
	u32 reserved0[2];	// 0
	u32 index_entries;
	u32 reserved1[3];	// 0
} SEEN_header_t;

typedef struct {
	s8 name[16];
	u32 offset;
	u32 comprlen;
	u32 uncomprlen;
	u32 is_compressed;
} SEEN_entry_t;

typedef struct {
	s8 magic[8];		// "PACK"
	u32 uncomprlen;
	u32 comprlen;
} PACK_header_t;

typedef struct {
	s8 magic[8];		// "CGMDT"
	u32 reserved0[2];	// 0
	u32 cg_counts;
	u32 reserved1[3];	// 0
} CGM_header_t;

typedef struct {
	s8 name[32];
	u32 number;
} CGM_info_t;

typedef struct {
	s8 magic[8];		// "KOEPAC"
	u32 reserved0[2];	// 0
	u32 table_len;
	u32 data_offset;
	u32 rate;
	u32 unknown2;		// 0
} KOE_header_t;

typedef struct {
	u16 koe_num;
	u16 length;
	u32 offset;
} KOE_table_t;
#pragma pack ()

typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 length;
} KOE_info_t;

static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static DWORD PDT10_uncompressA(BYTE *uncompr, DWORD uncomprlen,
							   BYTE *compr, DWORD comprlen)
{
	DWORD curbyte = 0;
	DWORD act_uncomprlen = 0;
	DWORD curbits = 0;
	DWORD bits;

	while (curbyte < comprlen) {
		if (!curbits) {
			bits = compr[curbyte++];
			curbits = 8;
			continue;
		}

		if (bits & 0x80)
			uncompr[act_uncomprlen++] = compr[curbyte++];
		else {
			DWORD copy_pixels, offset;

			copy_pixels = compr[curbyte++] + 2;
			offset = compr[curbyte++] + 1;
		
			for (DWORD i = 0; i < copy_pixels; i++) {
				uncompr[act_uncomprlen] = uncompr[act_uncomprlen - offset];
				act_uncomprlen++;
			}
		}
		bits <<= 1;
		curbits--;
	}

	return act_uncomprlen;
}

static DWORD PDT10_uncompress24(BYTE *uncompr, DWORD uncomprlen,
								BYTE *compr, DWORD comprlen)
{
	DWORD curbyte = 0;
	DWORD act_uncomprlen = 0;
	DWORD curbits = 0;
	DWORD bits;

	while (curbyte < comprlen) {
		if (!curbits) {
			bits = compr[curbyte++];
			curbits = 8;
			continue;
		}

		if (bits & 0x80) {
			uncompr[act_uncomprlen++] = compr[curbyte++];
			uncompr[act_uncomprlen++] = compr[curbyte++];
			uncompr[act_uncomprlen++] = compr[curbyte++];
		} else {
			DWORD copy_pixels, offset;

			copy_pixels = compr[curbyte++];
			offset = compr[curbyte++];
			offset = (offset << 4) | (copy_pixels >> 4);
			copy_pixels &= 0xf;
			offset++;
			copy_pixels++;
			offset *= 3;		
			for (DWORD i = 0; i < copy_pixels; i++) {
				uncompr[act_uncomprlen] = uncompr[act_uncomprlen - offset];
				act_uncomprlen++;
				uncompr[act_uncomprlen] = uncompr[act_uncomprlen - offset];
				act_uncomprlen++;
				uncompr[act_uncomprlen] = uncompr[act_uncomprlen - offset];
				act_uncomprlen++;
			}
		}
		bits <<= 1;
		curbits--;
	}

	return act_uncomprlen;
}

static DWORD PDT10_uncompress32(BYTE *uncompr, DWORD uncomprlen,
								BYTE *compr, DWORD comprlen)
{
	DWORD curbyte = 0;
	DWORD act_uncomprlen = 0;
	DWORD curbits = 0;
	DWORD bits;

	while (curbyte < comprlen) {
		if (!curbits) {
			bits = compr[curbyte++];
			curbits = 8;
			continue;
		}

		if (bits & 0x80) {
			uncompr[act_uncomprlen++] = compr[curbyte++];
			uncompr[act_uncomprlen++] = compr[curbyte++];
			uncompr[act_uncomprlen++] = compr[curbyte++];
			act_uncomprlen++;
		} else {
			DWORD copy_pixels, offset;

			copy_pixels = compr[curbyte++];
			offset = compr[curbyte++];
			offset = (offset << 4) | (copy_pixels >> 4);
			copy_pixels &= 0xf;
			offset++;
			copy_pixels++;
			offset *= 4;			
			for (DWORD i = 0; i < copy_pixels; i++) {
				uncompr[act_uncomprlen] = uncompr[act_uncomprlen - offset];
				act_uncomprlen++;
				uncompr[act_uncomprlen] = uncompr[act_uncomprlen - offset];
				act_uncomprlen++;
				uncompr[act_uncomprlen] = uncompr[act_uncomprlen - offset];
				act_uncomprlen++;
				uncompr[act_uncomprlen] = uncompr[act_uncomprlen - offset];
				act_uncomprlen++;
			}
		}
		bits <<= 1;
		curbits--;
	}

	return act_uncomprlen;
}

static DWORD PDT10_uncompress(BYTE *uncompr, DWORD uncomprlen,
							  BYTE *compr, DWORD comprlen, DWORD bpp)
{
	if (bpp == 24)
		return PDT10_uncompress24(uncompr, uncomprlen, compr, comprlen);
	else if (bpp == 32)
		return PDT10_uncompress32(uncompr, uncomprlen, compr, comprlen);
	else
		return PDT10_uncompressA(uncompr, uncomprlen, compr, comprlen);
}

static void PDT11_uncompress(BYTE *uncompr, DWORD uncomprlen,
							 BYTE *compr, DWORD comprlen, DWORD *offset_table)
{
	DWORD curbyte = 0;
	DWORD act_uncomprlen = 0;
	DWORD curbits = 0;
	DWORD bits;

    while (act_uncomprlen < uncomprlen) {
		if (!curbits) {
			bits = compr[curbyte++];
			curbits = 8;
			continue;
		}

		if (bits & 0x80)
			uncompr[act_uncomprlen++] = compr[curbyte++];
		else {
			DWORD offset = offset_table[compr[curbyte] & 0xf];
			DWORD copy_pixels = (compr[curbyte++] >> 4) + 2;
			if (copy_pixels + act_uncomprlen >= uncomprlen) {
				copy_pixels = uncomprlen - act_uncomprlen;
				if (act_uncomprlen == uncomprlen)
					break;
			}

			for (DWORD i = 0; i < copy_pixels; ++i) {
				if (act_uncomprlen < offset)
					uncompr[act_uncomprlen] = 0;
				else			
					uncompr[act_uncomprlen] = uncompr[act_uncomprlen - offset];
				++act_uncomprlen;
			}
		}
		bits <<= 1;
		curbits--;
    }
}

static BYTE cgm_decode_table[256] = {
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

static DWORD PACK_uncompress(BYTE *uncompr, DWORD uncomprlen,
							 BYTE *compr, DWORD comprlen)
{
	DWORD curbyte = 0;
	DWORD act_uncomprlen = 0;
	DWORD curbits = 0;
	DWORD bits;

	while (curbyte < comprlen) {
		if (!curbits) {
			bits = compr[curbyte++];
			curbits = 8;
			continue;
		}

		if (bits & 0x80)
			uncompr[act_uncomprlen++] = compr[curbyte++];
		else {
			DWORD copy_bytes, offset;

			copy_bytes = compr[curbyte++];
			offset = compr[curbyte++];
			offset = (offset << 4) | (copy_bytes >> 4);
			copy_bytes &= 0xf;
			offset++;
			copy_bytes += 2;		
			for (DWORD i = 0; i < copy_bytes; i++) {
				uncompr[act_uncomprlen] = uncompr[act_uncomprlen - offset];
				act_uncomprlen++;
			}
		}
		bits <<= 1;
		curbits--;
	}

	return act_uncomprlen;
}


/* 8bit -> 16bit への恃垂テ〖ブル。塑丸は signed short だが
** とりあえず unsigned で胺っている
*/

static unsigned short koe_8bit_trans_tbl[256] = {
  0x8000,0x81ff,0x83f9,0x85ef,0x87e1,0x89cf,0x8bb9,0x8d9f,
  0x8f81,0x915f,0x9339,0x950f,0x96e1,0x98af,0x9a79,0x9c3f,
  0x9e01,0x9fbf,0xa179,0xa32f,0xa4e1,0xa68f,0xa839,0xa9df,
  0xab81,0xad1f,0xaeb9,0xb04f,0xb1e1,0xb36f,0xb4f9,0xb67f,
  0xb801,0xb97f,0xbaf9,0xbc6f,0xbde1,0xbf4f,0xc0b9,0xc21f,
  0xc381,0xc4df,0xc639,0xc78f,0xc8e1,0xca2f,0xcb79,0xccbf,
  0xce01,0xcf3f,0xd079,0xd1af,0xd2e1,0xd40f,0xd539,0xd65f,
  0xd781,0xd89f,0xd9b9,0xdacf,0xdbe1,0xdcef,0xddf9,0xdeff,
  0xe001,0xe0ff,0xe1f9,0xe2ef,0xe3e1,0xe4cf,0xe5b9,0xe69f,
  0xe781,0xe85f,0xe939,0xea0f,0xeae1,0xebaf,0xec79,0xed3f,
  0xee01,0xeebf,0xef79,0xf02f,0xf0e1,0xf18f,0xf239,0xf2df,
  0xf381,0xf41f,0xf4b9,0xf54f,0xf5e1,0xf66f,0xf6f9,0xf77f,
  0xf801,0xf87f,0xf8f9,0xf96f,0xf9e1,0xfa4f,0xfab9,0xfb1f,
  0xfb81,0xfbdf,0xfc39,0xfc8f,0xfce1,0xfd2f,0xfd79,0xfdbf,
  0xfe01,0xfe3f,0xfe79,0xfeaf,0xfee1,0xff0f,0xff39,0xff5f,
  0xff81,0xff9f,0xffb9,0xffcf,0xffe1,0xffef,0xfff9,0xffff,
  0x0000,0x0001,0x0007,0x0011,0x001f,0x0031,0x0047,0x0061,
  0x007f,0x00a1,0x00c7,0x00f1,0x011f,0x0151,0x0187,0x01c1,
  0x01ff,0x0241,0x0287,0x02d1,0x031f,0x0371,0x03c7,0x0421,
  0x047f,0x04e1,0x0547,0x05b1,0x061f,0x0691,0x0707,0x0781,
  0x07ff,0x0881,0x0907,0x0991,0x0a1f,0x0ab1,0x0b47,0x0be1,
  0x0c7f,0x0d21,0x0dc7,0x0e71,0x0f1f,0x0fd1,0x1087,0x1141,
  0x11ff,0x12c1,0x1387,0x1451,0x151f,0x15f1,0x16c7,0x17a1,
  0x187f,0x1961,0x1a47,0x1b31,0x1c1f,0x1d11,0x1e07,0x1f01,
  0x1fff,0x2101,0x2207,0x2311,0x241f,0x2531,0x2647,0x2761,
  0x287f,0x29a1,0x2ac7,0x2bf1,0x2d1f,0x2e51,0x2f87,0x30c1,
  0x31ff,0x3341,0x3487,0x35d1,0x371f,0x3871,0x39c7,0x3b21,
  0x3c7f,0x3de1,0x3f47,0x40b1,0x421f,0x4391,0x4507,0x4681,
  0x47ff,0x4981,0x4b07,0x4c91,0x4e1f,0x4fb1,0x5147,0x52e1,
  0x547f,0x5621,0x57c7,0x5971,0x5b1f,0x5cd1,0x5e87,0x6041,
  0x61ff,0x63c1,0x6587,0x6751,0x691f,0x6af1,0x6cc7,0x6ea1,
  0x707f,0x7261,0x7447,0x7631,0x781f,0x7a11,0x7c07,0x7fff
};

/* ADPCMˇˇˇじゃないらしい。ただのDPCMのナめたテ〖ブル。
** 极瓢栏喇すりゃいいんだけど256byteだったら
** テ〖ブルでも啼玛ないでしょ
*/

static BYTE koe_ad_trans_tbl[256] = {
  0x00,0xff,0x01,0xfe,0x02,0xfd,0x03,0xfc,0x04,0xfb,0x05,0xfa,0x06,0xf9,0x07,0xf8,
  0x08,0xf7,0x09,0xf6,0x0a,0xf5,0x0b,0xf4,0x0c,0xf3,0x0d,0xf2,0x0e,0xf1,0x0f,0xf0,
  0x10,0xef,0x11,0xee,0x12,0xed,0x13,0xec,0x14,0xeb,0x15,0xea,0x16,0xe9,0x17,0xe8,
  0x18,0xe7,0x19,0xe6,0x1a,0xe5,0x1b,0xe4,0x1c,0xe3,0x1d,0xe2,0x1e,0xe1,0x1f,0xe0,
  0x20,0xdf,0x21,0xde,0x22,0xdd,0x23,0xdc,0x24,0xdb,0x25,0xda,0x26,0xd9,0x27,0xd8,
  0x28,0xd7,0x29,0xd6,0x2a,0xd5,0x2b,0xd4,0x2c,0xd3,0x2d,0xd2,0x2e,0xd1,0x2f,0xd0,
  0x30,0xcf,0x31,0xce,0x32,0xcd,0x33,0xcc,0x34,0xcb,0x35,0xca,0x36,0xc9,0x37,0xc8,
  0x38,0xc7,0x39,0xc6,0x3a,0xc5,0x3b,0xc4,0x3c,0xc3,0x3d,0xc2,0x3e,0xc1,0x3f,0xc0,
  0x40,0xbf,0x41,0xbe,0x42,0xbd,0x43,0xbc,0x44,0xbb,0x45,0xba,0x46,0xb9,0x47,0xb8,
  0x48,0xb7,0x49,0xb6,0x4a,0xb5,0x4b,0xb4,0x4c,0xb3,0x4d,0xb2,0x4e,0xb1,0x4f,0xb0,
  0x50,0xaf,0x51,0xae,0x52,0xad,0x53,0xac,0x54,0xab,0x55,0xaa,0x56,0xa9,0x57,0xa8,
  0x58,0xa7,0x59,0xa6,0x5a,0xa5,0x5b,0xa4,0x5c,0xa3,0x5d,0xa2,0x5e,0xa1,0x5f,0xa0,
  0x60,0x9f,0x61,0x9e,0x62,0x9d,0x63,0x9c,0x64,0x9b,0x65,0x9a,0x66,0x99,0x67,0x98,
  0x68,0x97,0x69,0x96,0x6a,0x95,0x6b,0x94,0x6c,0x93,0x6d,0x92,0x6e,0x91,0x6f,0x90,
  0x70,0x8f,0x71,0x8e,0x72,0x8d,0x73,0x8c,0x74,0x8b,0x75,0x8a,0x76,0x89,0x77,0x88,
  0x78,0x87,0x79,0x86,0x7a,0x85,0x7b,0x84,0x7c,0x83,0x7d,0x82,0x7e,0x81,0x7f,0x80
};

/********************* SEEN.txt *********************/

/* 封包匹配回调函数 */
static int AVG32_SEEN_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "PACL", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int AVG32_SEEN_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	SEEN_header_t SEEN_header;
	SEEN_entry_t *index_buffer;
	DWORD index_buffer_length;	

	if (pkg->pio->readvec(pkg, &SEEN_header, sizeof(SEEN_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	index_buffer_length = SEEN_header.index_entries * sizeof(SEEN_entry_t);
	index_buffer = (SEEN_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = SEEN_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(SEEN_entry_t);
//	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int AVG32_SEEN_parse_resource_info(struct package *pkg,
										  struct package_resource *pkg_res)
{
	SEEN_entry_t *SEEN_entry;

	SEEN_entry = (SEEN_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, SEEN_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = SEEN_entry->comprlen;
	if (SEEN_entry->is_compressed)
		pkg_res->actual_data_length = SEEN_entry->uncomprlen;	/* 数据都是明文 */
	else
		pkg_res->actual_data_length = 0;
	pkg_res->offset = SEEN_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int AVG32_SEEN_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	BYTE *SEEN = (BYTE *)pkg->pio->readvec_only(pkg,
		pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET);
	if (!SEEN)
		return -CUI_EREADVECONLY;

	if (!strncmp((char *)SEEN, "PACK", 4)) {
		PACK_header_t *PACK_header = (PACK_header_t *)SEEN;
		BYTE *compr = (BYTE *)(PACK_header + 1);
		DWORD uncomprlen = PACK_header->uncomprlen;
		BYTE *uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr)
			return -CUI_EMEM;

		DWORD act_uncomprlen = PACK_uncompress(uncompr, uncomprlen,
			compr, PACK_header->comprlen - sizeof(PACK_header_t));	
		if (act_uncomprlen != uncomprlen) {
			printf("%x %x\n", act_uncomprlen, uncomprlen);
			free(uncompr);
			return -CUI_EUNCOMPR;
		}
		pkg_res->actual_data = uncompr;
		pkg_res->actual_data_length = act_uncomprlen;
	} else {
		pkg_res->actual_data = NULL;
		pkg_res->actual_data_length = 0;		
	}
	pkg_res->raw_data = SEEN;

	return 0;
}

/* 资源保存函数 */
static int AVG32_SEEN_save_resource(struct resource *res, 
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

/* 封包资源释放函数 */
static void AVG32_SEEN_release_resource(struct package *pkg, 
										struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res->raw_data)
		pkg_res->raw_data = NULL;
}

/* 封包卸载函数 */
static void AVG32_SEEN_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation AVG32_SEEN_operation = {
	AVG32_SEEN_match,					/* match */
	AVG32_SEEN_extract_directory,		/* extract_directory */
	AVG32_SEEN_parse_resource_info,		/* parse_resource_info */
	AVG32_SEEN_extract_resource,		/* extract_resource */
	AVG32_SEEN_save_resource,			/* save_resource */
	AVG32_SEEN_release_resource,		/* release_resource */
	AVG32_SEEN_release					/* release */
};

/********************* PDT *********************/

/* 封包匹配回调函数 */
static int AVG32_PDT_match(struct package *pkg)
{
	s8 magic[8];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "PDT10", sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int AVG32_PDT_extract_resource(struct package *pkg,
									  struct package_resource *pkg_res)
{
	PDT_header_t *PDT_header;
	u32 PDT_size;

	if (pkg->pio->length_of(pkg, &PDT_size))
		return -CUI_ELEN;

	BYTE *PDT = (BYTE *)pkg->pio->readvec_only(pkg,
		PDT_size, 0, IO_SEEK_SET);
	if (!PDT)
		return -CUI_EREADVECONLY;

	PDT_header = (PDT_header_t *)PDT;
	BYTE *compr = (BYTE *)(PDT_header + 1);

	DWORD comprlen, bpp, alpha_length;
	BYTE *alpha;
	if (PDT_header->alpha_offset) {
		comprlen = PDT_header->alpha_offset - sizeof(PDT_header_t);
		bpp = 32;

		alpha_length = PDT_header->width * PDT_header->height;
		alpha = (BYTE *)malloc(alpha_length);
		if (!alpha)
			return -CUI_EMEM;
		memset(alpha, 0, alpha_length);

		DWORD act_uncomprlen = PDT10_uncompress(alpha, alpha_length,
			compr + comprlen, PDT_header->PDT_size - PDT_header->alpha_offset, 8);
		if (act_uncomprlen != alpha_length) {
			printf("alpha %x %x\n", act_uncomprlen, alpha_length);
			free(alpha);
			return -CUI_EUNCOMPR;
		}
	} else {
		comprlen = PDT_size - sizeof(PDT_header_t);
		bpp = 24;
		alpha_length = 0;
		alpha = NULL;
	}

	DWORD uncomprlen = PDT_header->width * PDT_header->height * bpp / 8;
	BYTE *uncompr = (BYTE *)malloc(uncomprlen);
	if (!uncompr) {
		free(alpha);
		return -CUI_EMEM;
	}
	memset(uncompr, 0, uncomprlen);

	if (bpp == 32) {
		DWORD i = 0;
		for (DWORD y = 0; y < PDT_header->height; y++) {
			for (DWORD x = 0; x < PDT_header->width; x++)
				uncompr[y * PDT_header->width * 4 + x * 4 + 3] = alpha[i++];
		}
		free(alpha);
	}

	DWORD act_uncomprlen = PDT10_uncompress(uncompr, uncomprlen, 
		compr, comprlen, bpp);
	if (act_uncomprlen != uncomprlen) {
		free(uncompr);
		return -CUI_EUNCOMPR;
	}

	if (MyBuildBMPFile(uncompr, uncomprlen, NULL, 0, PDT_header->width,
			0 - PDT_header->height, bpp, (BYTE **)&pkg_res->actual_data,
			&pkg_res->actual_data_length, my_malloc)) {
		free(uncompr);
		return -CUI_EMEM;
	}
	free(uncompr);

	pkg_res->raw_data = compr;
	pkg_res->raw_data_length = comprlen;

	return 0;
}

static void AVG32_PDT_release(struct package *pkg, 
							  struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation AVG32_PDT_operation = {
	AVG32_PDT_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	AVG32_PDT_extract_resource,	/* extract_resource */
	AVG32_SEEN_save_resource,	/* save_resource */
	AVG32_SEEN_release_resource,/* release_resource */
	AVG32_PDT_release			/* release */
};

/********************* PDT11 *********************/

/* 封包匹配回调函数 */
static int AVG32_PDT11_match(struct package *pkg)
{
	s8 magic[8];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "PDT11", sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int AVG32_PDT11_extract_resource(struct package *pkg,
										struct package_resource *pkg_res)
{
	PDT_header_t *PDT_header;
	u32 PDT_size;

	if (pkg->pio->length_of(pkg, &PDT_size))
		return -CUI_ELEN;

	BYTE *PDT = (BYTE *)pkg->pio->readvec_only(pkg,
		PDT_size, 0, IO_SEEK_SET);
	if (!PDT)
		return -CUI_EREADVECONLY;

	PDT_header = (PDT_header_t *)PDT;
	BYTE *palette = (BYTE *)(PDT_header + 1);
	DWORD *offset_table = (DWORD *)(palette + 1024);
	BYTE *compr = (BYTE *)(offset_table + 16);

	DWORD comprlen, alpha_length;
	BYTE *alpha;
	if (PDT_header->alpha_offset) {
		alpha_length = PDT_header->width * PDT_header->height;
		alpha = (BYTE *)malloc(alpha_length);
		if (!alpha)
			return -CUI_EMEM;
		memset(alpha, 0, alpha_length);

		DWORD act_uncomprlen = PDT10_uncompressA(alpha, alpha_length,
			PDT + PDT_header->alpha_offset, PDT_header->PDT_size - PDT_header->alpha_offset);
		if (act_uncomprlen != alpha_length) {
			printf("11 alpha %x %x\n", act_uncomprlen, alpha_length);
			free(alpha);
			return -CUI_EUNCOMPR;
		}
		comprlen = PDT_header->alpha_offset - sizeof(PDT_header_t) - 64 - 1024;
	} else {
		comprlen = PDT_header->PDT_size - sizeof(PDT_header_t) - 64 - 1024;
		alpha_length = 0;
		alpha = NULL;
	}

	DWORD uncomprlen = PDT_header->width * PDT_header->height;
	BYTE *uncompr = (BYTE *)malloc(uncomprlen);
	if (!uncompr) {
		free(alpha);
		return -CUI_EMEM;
	}
	memset(uncompr, 0, uncomprlen);

	PDT11_uncompress(uncompr, uncomprlen, compr, comprlen, offset_table);

	// TODO: alpha blending

	if (MyBuildBMPFile(uncompr, uncomprlen, palette, 1024, PDT_header->width,
			0 - PDT_header->height, 8, (BYTE **)&pkg_res->actual_data,
			&pkg_res->actual_data_length, my_malloc)) {
		free(uncompr);
		free(alpha);
		return -CUI_EMEM;
	}
	free(uncompr);
	free(alpha);

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation AVG32_PDT11_operation = {
	AVG32_PDT11_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	AVG32_PDT11_extract_resource,	/* extract_resource */
	AVG32_SEEN_save_resource,		/* save_resource */
	AVG32_SEEN_release_resource,	/* release_resource */
	AVG32_PDT_release				/* release */
};

/********************* CGM *********************/

/* 封包匹配回调函数 */
static int AVG32_CGM_match(struct package *pkg)
{
	s8 magic[8];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "CGMDT", sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int AVG32_CGM_extract_resource(struct package *pkg,
									  struct package_resource *pkg_res)
{
	CGM_header_t *CGM_header;
	u32 CGM_size;

	if (pkg->pio->length_of(pkg, &CGM_size))
		return -CUI_ELEN;

	BYTE *CGM = (BYTE *)pkg->pio->readvec_only(pkg,
		CGM_size, 0, IO_SEEK_SET);
	if (!CGM)
		return -CUI_EREADVECONLY;

	CGM_header = (CGM_header_t *)CGM;
	BYTE *enc_CGM = (BYTE *)(CGM_header + 1);
	DWORD enc_CGM_len = CGM_size - sizeof(CGM_header_t);
	DWORD dec_CGM_len = enc_CGM_len;
	BYTE *dec_CGM = (BYTE *)malloc(dec_CGM_len);
	if (!dec_CGM)
		return -CUI_EMEM;

	for (DWORD i = 0; i < dec_CGM_len; i++)
		dec_CGM[i] = enc_CGM[i] ^ cgm_decode_table[i & 0xff];

	if (!strncmp((char *)dec_CGM, "PACK", 4)) {
		PACK_header_t *PACK_header = (PACK_header_t *)dec_CGM;
		BYTE *compr = (BYTE *)(PACK_header + 1);
		DWORD uncomprlen = PACK_header->uncomprlen;
		BYTE *uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(dec_CGM);
			return -CUI_EMEM;
		}

		DWORD act_uncomprlen = PACK_uncompress(uncompr, uncomprlen,
			compr, PACK_header->comprlen - sizeof(PACK_header_t));	
		free(dec_CGM);
		if (act_uncomprlen != uncomprlen) {
			printf("%x %x\n", act_uncomprlen, uncomprlen);
			free(uncompr);
			return -CUI_EUNCOMPR;
		}
		pkg_res->actual_data = uncompr;
		pkg_res->actual_data_length = act_uncomprlen;
	} else {
		pkg_res->actual_data = dec_CGM;
		pkg_res->actual_data_length = dec_CGM_len;		
	}
	pkg_res->raw_data = CGM;
	pkg_res->raw_data_length = CGM_size;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation AVG32_CGM_operation = {
	AVG32_CGM_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	AVG32_CGM_extract_resource,	/* extract_resource */
	AVG32_SEEN_save_resource,	/* save_resource */
	AVG32_SEEN_release_resource,/* release_resource */
	AVG32_PDT_release			/* release */
};

/********************* KOE *********************/

/* 封包匹配回调函数 */
static int AVG32_KOE_match(struct package *pkg)
{
	s8 magic[8];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "KOEPAC", 8)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int AVG32_KOE_extract_directory(struct package *pkg,
									   struct package_directory *pkg_dir)
{
	KOE_header_t KOE_header;

	if (pkg->pio->readvec(pkg, &KOE_header, sizeof(KOE_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	if (!KOE_header.rate)
		KOE_header.rate = 22050;

	DWORD table_size = KOE_header.table_len * sizeof(KOE_table_t);
	KOE_table_t *table = (KOE_table_t *)malloc(table_size);
	if (!table)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, table, table_size)) {
		free(table);
		return -CUI_EREAD;
	}

	DWORD index_buffer_len = KOE_header.table_len * sizeof(KOE_table_t);
	if (!table)
		return -CUI_EMEM;

	DWORD KOE_info_length = KOE_header.table_len * sizeof(KOE_info_t);
	KOE_info_t *KOE_info = (KOE_info_t *)malloc(KOE_info_length);
	if (!KOE_info) {
		free(table);
		return -CUI_EMEM;
	}

	for (DWORD i = 0; i < KOE_header.table_len; i++) {
		sprintf(KOE_info[i].name, "%05d", table[i].koe_num);
		KOE_info[i].length = table[i].length;
		KOE_info[i].offset = table[i].offset;
	}

	pkg_dir->index_entries = KOE_header.table_len;
	pkg_dir->directory = KOE_info;
	pkg_dir->directory_length = KOE_info_length;
	pkg_dir->index_entry_length = sizeof(KOE_info_t);
//	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;
	package_set_private(pkg, KOE_header.rate);

	return 0;
}

/* 封包索引项解析函数 */
static int AVG32_KOE_parse_resource_info(struct package *pkg,
										 struct package_resource *pkg_res)
{
	KOE_info_t *KOE_info;

	KOE_info = (KOE_info_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, KOE_info->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = KOE_info->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = KOE_info->offset;

	return 0;
}

/* 封包资源提取函数 */
static int AVG32_KOE_extract_resource(struct package *pkg,
									  struct package_resource *pkg_res)
{
	KOE_info_t *KOE_info = (KOE_info_t *)pkg_res->actual_index_entry;
	
	WORD *table = (WORD *)malloc(KOE_info->length * 2);
	if (!table)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, table, KOE_info->length * 2, 
			KOE_info->offset, IO_SEEK_SET)) {
		free(table);
		return -CUI_EREADVEC;
	}

	DWORD all_len = 0;
	for (DWORD i = 0; i < KOE_info->length; i++)
		all_len += table[i];

	BYTE *src_orig = (BYTE *)malloc(all_len);
	if (!src_orig) {
		free(table);
		return -CUI_EMEM;
	}

	WORD *dest_orig = (WORD *)malloc(KOE_info->length * 4096);
	if (!dest_orig) {
		free(src_orig);
		free(table);
		return -CUI_EMEM;
	}

	BYTE *src = src_orig;
	WORD *dest = dest_orig;

	if (pkg->pio->read(pkg, src_orig, all_len)) {
		free(dest_orig);
		free(src_orig);
		free(table);
		return -CUI_EREAD;
	}

	for (i = 0; i < KOE_info->length; i++) {
		unsigned int slen = table[i];

		if (!slen) { // do nothing
			memset(dest, 0, 4096);
			dest += 2048;
			src += 0;
		} else if (slen == 1024) { // table ハムエケ
			for (DWORD j = 0; j < 1024; j++) {
				dest[0] = dest[1] = koe_8bit_trans_tbl[*src];
				dest += 2;
				src++;
			}
		} else { // DPCM
			char d = 0;
			short o2;
			unsigned int k, j; 
			
			for (j = 0, k = 0; j < slen && k < 2048; j++) {
				unsigned char s = src[j];

				if ((s + 1) & 0x0f) {
					d -= koe_ad_trans_tbl[s & 0x0f];
				} else {
					unsigned char s2;
					s >>= 4; s &= 0x0f; s2 = s;
					s = src[++j]; s2 |= (s<<4) & 0xf0;
					d -= koe_ad_trans_tbl[s2];
				}
				o2 = koe_8bit_trans_tbl[ (unsigned char)d];
				dest[k] = dest[k+1] = o2; k+=2;
				s >>= 4;
				if ((s+1) & 0x0f) {
					d -= koe_ad_trans_tbl[s & 0x0f];
				} else {
					d -= koe_ad_trans_tbl[ src[++j] ];
				}
				o2 = koe_8bit_trans_tbl[ (unsigned char)d];
				dest[k] = dest[k+1] = o2; k+=2;
			}
			dest += 2048;
			src += slen;
		}
	}
	free(table);

	DWORD rate = package_get_private(pkg);
	if (MySaveAsWAVE(dest_orig, KOE_info->length * 4096, 1,
			2, rate, 16, NULL, 0, (BYTE **)&pkg_res->actual_data, 
			&pkg_res->actual_data_length, my_malloc)) {
		free(dest_orig);
		free(src_orig);
		return -CUI_EMEM;
	}
	free(dest_orig);

	pkg_res->raw_data = src_orig;
	pkg_res->raw_data_length = all_len;

	return 0;
}

static void AVG32_KOE_release_resource(struct package *pkg, 
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

/* 封包处理回调函数集合 */
static cui_ext_operation AVG32_KOE_operation = {
	AVG32_KOE_match,				/* match */
	AVG32_KOE_extract_directory,	/* extract_directory */
	AVG32_KOE_parse_resource_info,	/* parse_resource_info */
	AVG32_KOE_extract_resource,		/* extract_resource */
	AVG32_SEEN_save_resource,		/* save_resource */
	AVG32_KOE_release_resource,		/* release_resource */
	AVG32_SEEN_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK AVG32_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".PDT"), _T(".BMP"), 
		NULL, &AVG32_PDT_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".PDT"), _T(".BMP"), 
		NULL, &AVG32_PDT11_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".TXT"), NULL, 
		NULL, &AVG32_SEEN_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".CGM"), _T(".CGM_"), 
		NULL, &AVG32_CGM_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".KOE"), _T(".WAV"), 
		NULL, &AVG32_KOE_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
