#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>
#include <zlib.h>
#include <utility.h>
#include <cui_common.h>

/*
00441408  |.  81FF 07010000 CMP     EDI, 107                         ;  version
0044140E  |.  74 08         JE      SHORT magustal.00441418
00441410  |.  81FF EA000000 CMP     EDI, 0EA				; 0.234
00441416  |.  7C 65         JL      SHORT magustal.0044147D
00441418  |>  8B15 FC886B00 MOV     EDX, DWORD PTR DS:[6B88FC]
0044141E  |.  03D3          ADD     EDX, EBX
*/

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information YU_RIS_cui_information = {
	_T("Firstia"),				/* copyright */
	_T("YU-RIS Script Engine"),	/* system */
	_T(".ypf .ybn .ymv"),		/* package */
	_T("1.0.0"),				/* revision */
	_T("痴漢公賊"),				/* author */
	_T("2009-8-2 21:27"),		/* date */
	NULL,						/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
// β3：[0.256,0.271]
// β2：[0.203,0.255] 0.233, 0.237, 0.240, 0.247, 0.249(アポクリトス-外典-的实际版本), 0.251 - 0.255, 0.290, 0.300（アポクリトス-外典-用的篡改版本）
typedef struct {
	s8 magic[4];			/* "YPF" */
	u32 version;
	u32 index_entries;		
	u32 index_length;		// 老的版本是0
	u8 reserved[16];
} ypf_header_t;

typedef struct {
	s8 name[264];
	// 没使用.
	// cg: 0 - png,db, 1 - bmp; 
	// audio: 0 - wav, 2 - ogg
	// script: 0 - dat
	u32 type;
	u32 is_compressed;		
	u32 uncomprlen;
	u32 comprlen;
	u32 offset;
	u32 reserved;			// 0
} ypf118_entry_t;

typedef struct {
	s8 name[264];
	u32 is_compressed;		
	u32 uncomprlen;
	u32 comprlen;
	u32 offset;
	u32 reserved;			// 0
} ypf133_entry_t;

typedef struct {
	s8 magic[4];			/* "YSMV" */
	u32 version;			// 1 ? 
	u32 index_entries;		
	u32 index_length;		// 0 ?
} ymv_header_t;

typedef struct {
	s8 magic[4];			/* "YSER" */
	u32 offset;
	u32 unknown;			// 0 or non-0
	u32 reserved;			// 0
} yser_header_t;
#pragma pack ()

/* .ypf封包的索引项结构 */
typedef struct {
	u32 name_crc;
	u8 name_length;
	s8 *name;
	u8 type;			// 0 - ybn; 1 - cg(β2); 2 - png(β3); 7 - ogg
	u8 is_compressed;
	u32 uncomprLen;
	u32 comprLen;
	u32 offset;
	u32 data_crc;
	// 0.220(0.223)后面2个4字节，用来当data crc不匹配时计算新crc用的
} ypf_entry_t;

typedef struct {
	s8 magic[4];			// "YSTB"
	u32 version;			// 0.238, 0.247, 0.249, 0.290
	u32 data1_length;
	u32 data2_length;
	u32 data2_offset;
	u32 reserved[3];
} ystb_header_t;

typedef struct {
	char name[MAX_PATH];
	DWORD offset;
	DWORD length;
} ymv_entry_t;

enum {
	keitai = 0,		// アポクリトス-外典-
	MagusTale,		// MagusTale（マギウステイル）～世界樹と恋する魔法使い～
	itaFD,			// いな☆こい！ ファンディスク
	rinjyoku,		// 凛辱の城 傀儡の王 体験版(0.271)
};

static u32 ypf_version;

static int game;

static BYTE decode_table[256];
static WORD decode_table2[256] = {
		0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf, 
		0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7, 
		0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e, 
		0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876, 
		0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd, 
		0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5, 
		0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c, 
		0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974, 
		0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb, 
		0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3, 
		0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a, 
		0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72, 
		0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9, 
		0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1, 
		0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738, 
		0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70, 
		0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7, 
		0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff, 
		0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036, 
		0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e, 
		0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5, 
		0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd, 
		0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134, 
		0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c, 
		0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3, 
		0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb, 
		0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232, 
		0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a, 
		0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1, 
		0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9, 
		0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330, 
		0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78, 
};

#if 0
static void ypf_decode_table_init(void)
{
	BYTE tmp;

	for (unsigned int i = 0; i < 256; i++)
		decode_table[i] = i;

	tmp = decode_table[9];
	decode_table[9] = decode_table[11];
	decode_table[11] = tmp;

	tmp = decode_table[12];
	decode_table[12] = decode_table[16];
	decode_table[16] = tmp;

	tmp = decode_table[13];
	decode_table[13] = decode_table[19];
	decode_table[19] = tmp;

	tmp = decode_table[17];
	decode_table[17] = decode_table[25];
	decode_table[25] = tmp;

	tmp = decode_table[21];
	decode_table[21] = decode_table[27];
	decode_table[27] = tmp;

	tmp = decode_table[28];
	decode_table[28] = decode_table[30];
	decode_table[30] = tmp;

	tmp = decode_table[32];
	decode_table[32] = decode_table[35];
	decode_table[35] = tmp;

	tmp = decode_table[38];
	decode_table[38] = decode_table[41];
	decode_table[41] = tmp;

	tmp = decode_table[44];
	decode_table[44] = decode_table[47];
	decode_table[47] = tmp;

	tmp = decode_table[46];
	decode_table[46] = decode_table[50];
	decode_table[50] = tmp;
}
#else
static void ypf_decode_table_init(void)
{
	BYTE tmp;

	for (unsigned int i = 0; i < 256; i++)
		decode_table[i] = i;

	tmp = decode_table[3];
	decode_table[3] = decode_table[72];
	decode_table[72] = tmp;

	tmp = decode_table[6];
	decode_table[6] = decode_table[53];
	decode_table[53] = tmp;

	tmp = decode_table[9];
	decode_table[9] = decode_table[11];
	decode_table[11] = tmp;

	tmp = decode_table[12];
	decode_table[12] = decode_table[16];
	decode_table[16] = tmp;

	tmp = decode_table[13];
	decode_table[13] = decode_table[19];
	decode_table[19] = tmp;

	tmp = decode_table[17];
	decode_table[17] = decode_table[25];
	decode_table[25] = tmp;

	tmp = decode_table[21];
	decode_table[21] = decode_table[27];
	decode_table[27] = tmp;

	tmp = decode_table[28];
	decode_table[28] = decode_table[30];
	decode_table[30] = tmp;

	tmp = decode_table[32];
	decode_table[32] = decode_table[35];
	decode_table[35] = tmp;

	tmp = decode_table[38];
	decode_table[38] = decode_table[41];
	decode_table[41] = tmp;

	tmp = decode_table[44];
	decode_table[44] = decode_table[47];
	decode_table[47] = tmp;

	tmp = decode_table[46];
	decode_table[46] = decode_table[50];
	decode_table[50] = tmp;
}
#endif

static DWORD ypf_crc(BYTE *data, unsigned int data_length)
{
	unsigned int seed_lo, seed_hi;
	unsigned int seed = 1;
	unsigned int i;

	seed_lo = seed & 0xffff;
	seed_hi = seed >> 16;

	if (data_length < 16) {
		for (i = 0; i < data_length; i++) {
			seed_lo += data[i];
			seed_hi += seed_lo;
		}

		if (seed_lo >= 0xfff1)
			seed_lo += 0xffff000f;
		seed_hi %= 0xfff1;

		return seed_lo | (seed_hi << 16);
	} else if (data_length >= 5552) {
		for (unsigned int k = 0; k < data_length / 5552; k++) {
			for (i = 0; i < 5552; i++) {
				seed_lo += data[i];
				seed_hi += seed_lo;
			}
			data += 5552;
			seed_lo %= 0xfff1;
			seed_hi %= 0xfff1;
		}
		data_length %= 5552;
	}

	if (data_length) {
		for (i = 0; i < data_length; i++) {
			seed_lo += data[i];
			seed_hi += seed_lo;
		}
		seed_lo %= 0xfff1;
		seed_hi %= 0xfff1;
	}

	return seed_lo | (seed_hi << 16);
}

// 0.220(0.223)数据校验用
static DWORD ypf_data_crc(BYTE *buf, DWORD len)
{
	DWORD crc = 0xffff;

	for (unsigned int i = 0; i < len; ++i) {
		DWORD idx;

		idx = (buf[i] ^ crc) & 0xff;
		crc = (decode_table2[idx] ^ (crc >> 8)) & 0xffff;
	}

	return crc;
}

static void __ystb_decode(ystb_header_t *ystb, DWORD ystb_length, BYTE *dec_tbl)
{
	BYTE *enc;
	unsigned int enc_len, i;

	enc = (BYTE *)(ystb + 1);
	enc_len = ystb->data1_length;
	for (i = 0; i < enc_len; i++)
		enc[i] ^= dec_tbl[i & 3];

	enc = (BYTE *)ystb + ystb->data2_offset;
	enc_len = ystb->data2_length;
	for (i = 0; i < enc_len; i++)
		enc[i] ^= dec_tbl[i & 3];
}

static void ystb_decode(ystb_header_t *ystb, DWORD ystb_length, 
						BYTE **ret_data, DWORD *ret_len)
{
	BYTE dec_tbl1[4] = { 0x07, 0xb4, 0x02, 0x4a };
	BYTE dec_tbl2[4] = { 0xd3, 0x6f, 0xac, 0x96 };
	BYTE dec_tbl0[4] = { 0x80, 0x80, 0x80, 0x80 };

	if (ystb->version > 263)
		__ystb_decode(ystb, ystb_length, dec_tbl2);
	else if (ystb->version > 224)	// 确切的上限边界并不清楚，只不过碰到了这个版本而
		__ystb_decode(ystb, ystb_length, dec_tbl1);	
	else if (ystb->version > 2)
		__ystb_decode(ystb, ystb_length, dec_tbl0);
}

/********************* ypf *********************/

/* 封包匹配回调函数 */
static int YU_RIS_ypf_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "YPF", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (pkg->pio->read(pkg, &ypf_version, sizeof(ypf_version))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	package_set_private(pkg, 0);

	return 0;	
}

/* 封包索引目录提取函数 */
static int YU_RIS_ypf_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	BYTE *index_buffer;

	DWORD base_offset = package_get_private(pkg);

	if (pkg->pio->readvec(pkg, &pkg_dir->index_entries, 4, base_offset + 8, IO_SEEK_SET))
		return -CUI_EREAD;

	if (pkg->pio->read(pkg, &pkg_dir->directory_length, 4))
		return -CUI_EREAD;

	// 确切的上限边界并不清楚，只不过碰到了这个版本而
	if (ypf_version <= 133) {
		if (ypf_version == 133)
			pkg_dir->directory_length = pkg_dir->index_entries * sizeof(ypf133_entry_t);
		else if (ypf_version == 118)
			pkg_dir->directory_length = pkg_dir->index_entries * sizeof(ypf118_entry_t);
	}

	index_buffer = (BYTE *)malloc(pkg_dir->directory_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, index_buffer, pkg_dir->directory_length, base_offset + sizeof(ypf_header_t), IO_SEEK_SET)) {
		free(index_buffer);
		return -CUI_EREADVEC;
	}

	pkg_dir->directory = index_buffer;
	// 确切的上限边界并不清楚，只不过碰到了这个版本而
	if (ypf_version > 133) {
		pkg_dir->flags = PKG_DIR_FLAG_VARLEN;
	} else {
		if (ypf_version == 133)
			pkg_dir->index_entry_length = sizeof(ypf133_entry_t);
		else if (ypf_version == 118)
			pkg_dir->index_entry_length = sizeof(ypf118_entry_t);
	}

	return 0;
}

/* 封包索引项解析函数 */
static int YU_RIS_ypf_parse_resource_info(struct package *pkg,
										  struct package_resource *pkg_res)
{
	// 确切的上限边界并不清楚，只不过碰到了这个版本而
	if (ypf_version <= 133) {
		u32 is_compressed, uncomprlen, comprlen, offset;
		char *name;

		if (ypf_version == 133) {
			ypf133_entry_t *ypf133_entry = (ypf133_entry_t *)pkg_res->actual_index_entry;
			name = (char *)ypf133_entry->name;
			is_compressed = ypf133_entry->is_compressed;
			uncomprlen = ypf133_entry->uncomprlen;
			comprlen = ypf133_entry->comprlen;
			offset = ypf133_entry->offset;
		} else if (ypf_version == 118) {
			ypf118_entry_t *ypf118_entry = (ypf118_entry_t *)pkg_res->actual_index_entry;
			name = (char *)ypf118_entry->name;
			is_compressed = ypf118_entry->is_compressed;
			uncomprlen = ypf118_entry->uncomprlen;
			comprlen = ypf118_entry->comprlen;
			offset = ypf118_entry->offset;	
		}
		strcpy(pkg_res->name, name);
		pkg_res->name_length = -1;
		pkg_res->actual_data_length = is_compressed ? uncomprlen : 0;
		pkg_res->raw_data_length = comprlen;
		pkg_res->offset = offset;
	} else if (ypf_version > 133) {
		BYTE *ypf_entry_p;
		BYTE name_len;
		u32 name_crc;

		ypf_entry_p = (BYTE *)pkg_res->actual_index_entry;
		name_crc = *(u32 *)ypf_entry_p;
		ypf_entry_p += 4;
		//name_len = decode_table[~*ypf_entry_p++ & 0xff];
		name_len = decode_table[255 - *ypf_entry_p++];

#if 0
		/* 用之前的decode_table生成代码会有这个问题：
		77 ～And, two stars meet again～
		cgsys\option\sound2\btn_クイックセーブの確認_over.png @ cgsys.ypf
		实际得到的name_len是6: cgsys\
		*/
		if (name_len == 6) {
			char tmp_name[6];

			strncpy(tmp_name, (char *)ypf_entry_p, 6);

			for (unsigned int i = 0; i < name_len; ++i)
				tmp_name[i] = ~ypf_entry_p[i] ^ 0x3f;	
			
			if (!strncmp(tmp_name, "cgsys\\", 6))
				name_len = 53;
		}
#endif

		for (unsigned int i = 0; i < name_len; i++)
			pkg_res->name[i] = ~ypf_entry_p[i];

#if 0
		if ((crc32(0, (BYTE *)pkg_res->name, name_len) != name_crc)
				&& (ypf_crc((BYTE *)pkg_res->name, name_len) != name_crc)) {
			char bak_name[MAX_PATH];

			strncpy(bak_name, pkg_res->name, name_len);
			u8 key_list[] = { 0x3f, 0x40, 0x00 };
			for (DWORD j = 0; key_list[j]; ++j) {
				for (unsigned int i = 0; i < name_len; ++i)
					pkg_res->name[i] ^= key_list[j];

				if ((crc32(0, (BYTE *)pkg_res->name, name_len) == name_crc)
						|| (ypf_crc((BYTE *)pkg_res->name, name_len) == name_crc))
					break;
				
				strncpy(pkg_res->name, bak_name, name_len);
			}
			if (!key_list[j])		
				return -CUI_EMATCH;
		}
#else
		if ((crc32(0, (BYTE *)pkg_res->name, name_len) != name_crc)
				&& (ypf_crc((BYTE *)pkg_res->name, name_len) != name_crc)) {
			// 利用后缀"."来猜测key
			u8 key = pkg_res->name[name_len - 4] ^ '.';	
			for (unsigned int i = 0; i < name_len; ++i)
				pkg_res->name[i] ^= key;
			
			if (crc32(0, (BYTE *)pkg_res->name, name_len) != name_crc 
					&& ypf_crc((BYTE *)pkg_res->name, name_len) != name_crc) {
				return -CUI_EMATCH;
			}
		}
#endif

#if 0
	#if 1
		if (game == keitai && ypf_version == 300) {
			if (ypf_crc((BYTE *)pkg_res->name, name_len) != name_crc)
				return -CUI_EMATCH;
			goto skip_crc;
		} else if (game != -1) {
			for (unsigned int i = 0; i < name_len; i++)
				pkg_res->name[i] ^= 0x40;
		}

		if (ypf_version >= 256)	{ // β3
			if (crc32(0, (BYTE *)pkg_res->name, name_len) != name_crc 
					&& ypf_crc((BYTE *)pkg_res->name, name_len) != name_crc)
				return -CUI_EMATCH;
		} else {	// β2
			if (ypf_crc((BYTE *)pkg_res->name, name_len) != name_crc)
				return -CUI_EMATCH;
		}

	skip_crc:
	#else
		if (game == rinjyoku) {
			for (unsigned int i = 0; i < name_len; i++)
				pkg_res->name[i] ^= 0x40;

			if (crc32(0, (BYTE *)pkg_res->name, name_len) != name_crc)
				return -CUI_EMATCH;
		} else if (game == MagusTale || game == itaFD) {
			for (unsigned int i = 0; i < name_len; i++)
				pkg_res->name[i] ^= 0x40;

			if (ypf_crc((BYTE *)pkg_res->name, name_len) != name_crc)
				return -CUI_EMATCH;
		} else if (game == keitai && ypf_version == 300) {
			if (ypf_crc((BYTE *)pkg_res->name, name_len) != name_crc)
				return -CUI_EMATCH;
		} else if (ypf_version >= 256)	{ // β3
			if (crc32(0, (BYTE *)pkg_res->name, name_len) != name_crc 
					&& ypf_crc((BYTE *)pkg_res->name, name_len) != name_crc)
				return -CUI_EMATCH;
		} else {	// β2
			if (ypf_crc((BYTE *)pkg_res->name, name_len) != name_crc)
				return -CUI_EMATCH;
		}
	#endif
#endif

		pkg_res->name_length = name_len;
		ypf_entry_p += name_len + 2;
		pkg_res->actual_data_length = *(u32 *)ypf_entry_p;
		ypf_entry_p += 4;
		pkg_res->raw_data_length = *(u32 *)ypf_entry_p;
		ypf_entry_p += 4;

		DWORD base_offset = package_get_private(pkg);
		pkg_res->offset = *(u32 *)ypf_entry_p + base_offset;
		ypf_entry_p += 4;
		if (ypf_version <= 223)	// 确切的上限边界并不清楚，只不过碰到了这个版本而
			pkg_res->actual_index_entry_length = 31 + name_len;
		else
			pkg_res->actual_index_entry_length = 23 + name_len;
	}

	return 0;
}

/* 封包资源提取函数 */
static int YU_RIS_ypf_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;
	BYTE is_compressed;
	BYTE *ypf_entry_p;
	ystb_header_t *ystb;
	DWORD ystb_length;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	// 确切的上限边界并不清楚，只不过碰到了这个版本而
	if (ypf_version > 133) {
		ypf_entry_p = (BYTE *)pkg_res->actual_index_entry;
		ypf_entry_p += 4 + 1 + pkg_res->name_length + 1;
		is_compressed = *ypf_entry_p++;
		ypf_entry_p += 12;

		if ((ypf_crc(compr, comprlen) != *(u32 *)ypf_entry_p)
				&& (ypf_data_crc(compr, comprlen) != *(u32 *)ypf_entry_p)) {
			free(compr);
			return -CUI_EMATCH;
		}
	} else
		is_compressed = pkg_res->actual_data_length != 0;

	if (is_compressed) {
		DWORD act_uncomprlen;
		int err;

		act_uncomprlen = uncomprlen = pkg_res->actual_data_length;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}

		err = uncompress(uncompr, &act_uncomprlen, compr, comprlen);
		free(compr);
		compr = NULL;
		if (err != Z_OK) {
			free(uncompr);
			return -CUI_EUNCOMPR;
		}
		ystb = (ystb_header_t *)uncompr;
		ystb_length = uncomprlen;
	} else {
		uncomprlen = 0;
		uncompr = NULL;
		ystb = (ystb_header_t *)compr;
		ystb_length = comprlen;
	}

	if (!memcmp(ystb->magic, "YSTB", 4))
		ystb_decode(ystb, ystb_length, (BYTE **)&pkg_res->actual_data, 
			&pkg_res->actual_data_length);
	
	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;

	return 0;
}

/* 资源保存函数 */
static int YU_RIS_ypf_save_resource(struct resource *res, 
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
static void YU_RIS_ypf_release_resource(struct package *pkg, 
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

/* 封包卸载函数 */
static void YU_RIS_ypf_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation YU_RIS_ypf_operation = {
	YU_RIS_ypf_match,					/* match */
	YU_RIS_ypf_extract_directory,		/* extract_directory */
	YU_RIS_ypf_parse_resource_info,	/* parse_resource_info */
	YU_RIS_ypf_extract_resource,		/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* ymv *********************/

/* 封包匹配回调函数 */
static int YU_RIS_ymv_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "YSMV", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int YU_RIS_ymv_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	ymv_header_t ymv_header;
	if (pkg->pio->readvec(pkg, &ymv_header, sizeof(ymv_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	ymv_entry_t *index = new ymv_entry_t[ymv_header.index_entries];
	if (!index)
		return -CUI_EMEM;

	for (DWORD i = 0; i < ymv_header.index_entries; ++i) {
		if (pkg->pio->read(pkg, &index[i].offset, 4))
			break;
	}
	if (i != ymv_header.index_entries) {
		delete [] index;
		return -CUI_EREAD;
	}

	for (i = 0; i < ymv_header.index_entries; ++i) {
		if (pkg->pio->read(pkg, &index[i].length, 4))
			break;
		sprintf(index[i].name, "%08d", i);
	}
	if (i != ymv_header.index_entries) {
		delete [] index;
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = ymv_header.index_entries;
	pkg_dir->directory = index;
	pkg_dir->directory_length = ymv_header.index_entries * sizeof(ymv_entry_t);
	pkg_dir->index_entry_length = sizeof(ymv_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int YU_RIS_ymv_parse_resource_info(struct package *pkg,
										  struct package_resource *pkg_res)
{
	ymv_entry_t *ymv_entry;

	ymv_entry = (ymv_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, ymv_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = ymv_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = ymv_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int YU_RIS_ymv_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	BYTE *ymv = new BYTE[pkg_res->raw_data_length];
	if (!ymv)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, ymv, pkg_res->raw_data_length, 
			pkg_res->offset, IO_SEEK_SET)) {
		delete [] ymv;
		return -CUI_EREADVEC;
	}

	for (DWORD i = 0; i < pkg_res->raw_data_length; ++i)
		ymv[i] ^= (i & 0x0f) + 0x10;

	pkg_res->raw_data = ymv;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation YU_RIS_ymv_operation = {
	YU_RIS_ymv_match,					/* match */
	YU_RIS_ymv_extract_directory,		/* extract_directory */
	YU_RIS_ymv_parse_resource_info,	/* parse_resource_info */
	YU_RIS_ymv_extract_resource,		/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* ybn *********************/

/* 封包匹配回调函数 */
static int YU_RIS_ybn_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "YSTB", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (pkg->pio->seek(pkg, 0, IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_ESEEK;
	}

	return 0;	
}

/* 封包资源提取函数 */
static int YU_RIS_ybn_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	ystb_header_t *ystb;
	u32 ystb_length;

	if (pkg->pio->length_of(pkg, &ystb_length))
		return -CUI_ELEN;

	ystb = (ystb_header_t *)malloc(ystb_length);
	if (!ystb)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, ystb, ystb_length)) {
		free(ystb);
		return -CUI_EREAD;
	}

	ystb_decode(ystb, ystb_length, (BYTE **)&pkg_res->actual_data, 
			&pkg_res->actual_data_length);
	
	pkg_res->raw_data = ystb;
	pkg_res->raw_data_length = ystb_length;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation YU_RIS_ybn_operation = {
	YU_RIS_ybn_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	YU_RIS_ybn_extract_resource,	/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* exe *********************/

/* 封包匹配回调函数 */
static int YU_RIS_exe_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	u32 fsize;
	pkg->pio->length_of(pkg, &fsize);
	fsize &= ~0xffffff;

	if (!fsize) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	BYTE *tmp = (BYTE *)malloc(0x1000000);
	if (!tmp) {
		pkg->pio->close(pkg);
		return -CUI_EMEM;		
	}

	DWORD off;
	for (DWORD size = 0; size < fsize; size += 0x1000000) {
		pkg->pio->read(pkg, tmp, 0x1000000);
		
		BYTE *p = tmp;
		for (off = 0; off < 0x1000000; off += 16) {
			if (*(u32 *)p == 'RESY')
				break;
			p += 16;
		}
		if (off != 0x1000000)
			break;
	}
	free(tmp);
	if (size >= 0x1000000) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	DWORD base_offset = size + off;

	yser_header_t header;
	if (pkg->pio->readvec(pkg, &header, sizeof(header), base_offset, IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_EREADVEC;
	}

	s8 magic[4];
	if (pkg->pio->readvec(pkg, magic, sizeof(magic), base_offset + header.offset, IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_EREADVEC;
	}

	if (memcmp(magic, "YPF", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (pkg->pio->read(pkg, &ypf_version, 4)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	package_set_private(pkg, base_offset + header.offset);

	return 0;	
}

/* 封包处理回调函数集合 */
static cui_ext_operation YU_RIS_exe_operation = {
	YU_RIS_exe_match,				/* match */
	YU_RIS_ypf_extract_directory,	/* extract_directory */
	YU_RIS_ypf_parse_resource_info,	/* parse_resource_info */
	YU_RIS_ypf_extract_resource,	/* extract_resource */
	CUI_COMMON_OPERATION
};


/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK YU_RIS_register_cui(struct cui_register_callback *callback)
{
	const char *game_name;

	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".ypf"), NULL, 
		NULL, &YU_RIS_ypf_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".ymv"), _T(".jpg"), 
		NULL, &YU_RIS_ymv_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".ybn"), NULL, 
		NULL, &YU_RIS_ybn_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".exe"), NULL, 
		NULL, &YU_RIS_exe_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	ypf_decode_table_init();

	game_name = get_options("gametype");
	if (game_name)
		game = atoi(game_name);
	else
		game = -1;

	return 0;
}
