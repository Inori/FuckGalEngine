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
#include "am_decode.h"

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information MultiObjects_ScriptEngine_cui_information = {
	_T("Leaf"),						/* copyright */
	_T("MultiObjects-ScriptEngine"),/* system */
	_T(".a .am"),					/* package */
	_T("0.2.0"),					/* revision */
	_T("痴漢公賊"),					/* author */
	_T("2008-3-22 2:46"),			/* date */
	NULL,							/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	u16 magic;		// \x1e\xaf
	u16	index_entries;	
} AHEAD;

typedef struct {
	s8 name[23];		/* ascii string terminated with '\0' */
	u8 is_compressed;	/* 0 - don't decode; 1 - need decode; etc - invalid */
	u32 length;			/* byte-length */
	u32 offset;			/* offset from the end of index segment */
} A_FILE_INFO;

typedef struct {
	s8 magic[4];	// "am00"
	u32	index_length;	
	u8 xor;
} AMHEAD;

typedef struct {
	s8 *name;
	u32	offset;	
	u32	length;	
} AMINFO;

typedef struct {
	u32 width;
	u32 height;	
	u32 reserved1;	/* 浮点数 */
	u32 reserved2;	/* 浮点数 */
	u16 type;		/* 类型(gt) */
	u16 bitcount;	/* 色深 */	
} px_header_t; 

typedef struct {
	u32 item_count;			/* 每项36字节，集中在文件结尾 */
	u32 pictures;	
	u32 reserved0;
	u32 reserved1;
	u16 type;				/* 0x0090 */
	u16 reserved2;
	s8 magic[12];			/* "LeafAquaPlus" */
} px0090_header_t;

typedef struct {
	u32 pictures;
	u32 reserved0;
	u32 reserved1;
	u32 reserved2;
	u16 type;				/* 0x0080 */
	u16 reserved3;
	s8 magic[12];			/* "LeafAquaPlus" */
} px0080_header_t;

typedef struct {
	u32 width;
	u32 height;	
	u32 reserved0;
	u32 reserved1;
	u16 type;				/* 0x000a */
	u16 bitcount;
	u32 reserved_width;
	u32 reserved_height;
	u32 reserved2;
} px000a_header_t;

typedef struct {
	u32 picture_id;			/* 所属的picture的编号（从0开始） */
	u32 data[8];
} item_info_t;

typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 length;
} my_am_entry_t;

typedef struct {
	WCHAR name[MAX_PATH];
	u32 offset;
	u32 length;
	u32 width;
	u32 height;
	u32 bitcount;
} my_px_entry_t;

typedef struct {
	u8 Channels;
	u8 BlockAlign;
	u16 SamplesPerSec;
	u16 BitsPerSample;
	u32 AvgBytesPerSec;	
	u32 length;	
	u32 zero;
} w_header_t;

typedef struct {
	u32 unknown0;			// @00, 0x0002ab82
	u8 unknown1;			// @04
	u8 unknown2;			// @05, only bit0-2 is valid
	u64 unknown3;			// @06
	u32 unknown4;							// @0e
	u32 unknown5;							// @12
	u32	crc;						// @16
	u8 para_length_bytes;			// @1a, 该长度的数据从0开始做累加
} g_header_t;

#pragma pack ()

static DWORD crc_table[256] = {
		0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 
		0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005, 
		0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61, 
		0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 
		0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9, 
		0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75, 
		0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 
		0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd, 
		0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039, 
		0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 
		0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81, 
		0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d, 
		0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 
		0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95, 
		0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1, 
		0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 
		0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae, 
		0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072, 
		0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 
		0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca, 
		0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde, 
		0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 
		0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066, 
		0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba, 
		0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 
		0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692, 
		0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6, 
		0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 
		0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e, 
		0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2, 
		0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 
		0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a, 
		0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637, 
		0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 
		0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f, 
		0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53, 
		0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 
		0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b, 
		0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff, 
		0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 
		0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7, 
		0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b, 
		0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 
		0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3, 
		0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7, 
		0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 
		0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f, 
		0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3, 
		0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 
		0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c, 
		0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8, 
		0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 
		0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30, 
		0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec, 
		0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 
		0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654, 
		0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0, 
		0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 
		0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18, 
		0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4, 
		0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 
		0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c, 
		0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668, 
		0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4 
};

static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static DWORD g_crc_check(BYTE *buf, DWORD buf_len)
{
	DWORD crc = 0;

	for (DWORD i = 0; i < buf_len; i++)
		crc = crc_table[(crc >> 24) ^ buf[i]] ^ (crc << 8);

	return crc;
}

static DWORD lzss_uncompress2(BYTE *uncompr, DWORD uncomprlen, BYTE *compr, DWORD comprlen)
{
	unsigned int act_uncomprlen = 0;
	/* compr中的当前字节中的下一个扫描位的位置 */
	unsigned int curbit = 0;
	/* compr中的当前扫描字节 */
	unsigned int curbyte = 0;
	unsigned int nCurWindowByte = 0xfee;
	unsigned int win_size = 4096;
	BYTE win[4096];
	WORD flag = 0;

	memset(win, 0, nCurWindowByte);
	while (1) {
		flag >>= 1;
		if (!(flag & 0x0100))
			flag = compr[curbyte++] | 0xff00;

		if (flag & 1) {
			unsigned char data;

			data = compr[curbyte++];
			uncompr[act_uncomprlen++] = data;
			/* 输出的1字节放入滑动窗口 */
			win[nCurWindowByte++] = data;
			nCurWindowByte &= win_size - 1;

			if (act_uncomprlen == uncomprlen)
				break;
		} else {
			unsigned int copy_bytes, win_offset;
			unsigned int i;

			if (curbyte + 1 >= comprlen)
				break;

			win_offset = compr[curbyte++];
			copy_bytes = compr[curbyte++];
			win_offset |= (copy_bytes & 0xf0) << 4;
			copy_bytes &= 0x0f;
			copy_bytes += 3;

			for (i = 0; i < copy_bytes; i++) {	
				unsigned char data;

				data = win[(win_offset + i) & (win_size - 1)];
				uncompr[act_uncomprlen++] = data;
				if (act_uncomprlen == uncomprlen)
					return act_uncomprlen;
				/* 输出的1字节放入滑动窗口 */
				win[nCurWindowByte++] = data;
				nCurWindowByte &= win_size - 1;	
			}
		}
	}

	return act_uncomprlen;
}

/********************* a *********************/

/* 封包匹配回调函数 */
static int MultiObjects_ScriptEngine_a_match(struct package *pkg)
{
	u16 magic;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (magic != 0xaf1e) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int MultiObjects_ScriptEngine_a_extract_directory(struct package *pkg,
														 struct package_directory *pkg_dir)
{
	DWORD index_buffer_length;	
	u16	index_entries;

	if (pkg->pio->read(pkg, &index_entries, 2))
		return -CUI_EREAD;

	index_buffer_length = index_entries * sizeof(A_FILE_INFO);
	A_FILE_INFO *index_buffer = (A_FILE_INFO *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	for (DWORD i = 0; i < index_entries; i++)
		index_buffer[i].offset += sizeof(AHEAD) + index_buffer_length;

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(A_FILE_INFO);
	//pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int MultiObjects_ScriptEngine_a_parse_resource_info(struct package *pkg,
														   struct package_resource *pkg_res)
{
	A_FILE_INFO *a_entry;

	a_entry = (A_FILE_INFO *)pkg_res->actual_index_entry;
	if (a_entry->name[22])
		pkg_res->name_length = 23;
	else
		pkg_res->name_length = strlen(a_entry->name);
	strncpy(pkg_res->name, a_entry->name, pkg_res->name_length);
	pkg_res->raw_data_length = a_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = a_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int MultiObjects_ScriptEngine_a_extract_resource(struct package *pkg,
														struct package_resource *pkg_res)
{
	A_FILE_INFO *a_entry;
	BYTE *compr, *uncompr, *act_data;
	DWORD comprlen, uncomprlen, act_data_length;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)pkg->pio->readvec_only(pkg, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET);
	if (!compr)
		return -CUI_EREADVECONLY;

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = compr;
		return 0;		
	}

	a_entry = (A_FILE_INFO *)pkg_res->actual_index_entry;
	if (!a_entry->is_compressed) {
		uncompr = NULL;
		uncomprlen = 0;
		act_data = compr;
		act_data_length = comprlen;
	} else {
		uncomprlen = *(u32 *)compr;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr)
			return -CUI_EMEM;

		if (lzss_uncompress2(uncompr, uncomprlen, compr + 4, comprlen - 4) != uncomprlen) {
			free(uncompr);
			return -CUI_EUNCOMPR;
		}
		act_data = uncompr;
		act_data_length = uncomprlen;
	}	

	if (strstr(pkg_res->name, ".w")) {
		w_header_t *w_header = (w_header_t *)act_data;

		if (MySaveAsWAVE(w_header + 1, w_header->length, 
						   1, w_header->Channels, 
						   w_header->SamplesPerSec, w_header->BitsPerSample, 
						   NULL, 0, &act_data, &act_data_length, my_malloc)) {
			free(uncompr);
			return -CUI_EMEM;
		}
		free(uncompr);
		uncompr = act_data;
		uncomprlen = act_data_length;
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".wav");
	} else if (strstr(pkg_res->name, ".g")) {
		g_header_t g_header;
		BYTE *crc_data;
		DWORD para_len = 0;
		DWORD crc, crc_data_len;

		memcpy(&g_header, act_data, sizeof(g_header_t));
		for (DWORD i = 0; i < g_header.para_length_bytes; i++)
			para_len += act_data[sizeof(g_header_t) + i];

		crc_data_len = sizeof(g_header_t) + g_header.para_length_bytes + para_len;
		crc_data = (BYTE *)malloc(crc_data_len);
		if (!crc_data) {
			free(uncompr);
			return -CUI_EMEM;
		}
		crc = g_header.crc;
		g_header.crc = 0;
		memcpy(crc_data, &g_header, sizeof(g_header_t));
		memcpy(crc_data + sizeof(g_header_t), &act_data[sizeof(g_header_t)], g_header.para_length_bytes);
		memcpy(crc_data + sizeof(g_header_t) + g_header.para_length_bytes, act_data + sizeof(g_header_t) 
			+ g_header.para_length_bytes, para_len);

		if (crc != g_crc_check(crc_data, crc_data_len)) {
			free(crc_data);
			free(uncompr);
			return -CUI_EMATCH;
		}
		free(crc_data);
	}

	pkg_res->raw_data = compr;
	pkg_res->raw_data_length = comprlen;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

/* 资源保存函数 */
static int MultiObjects_ScriptEngine_a_save_resource(struct resource *res,
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
static void MultiObjects_ScriptEngine_a_release_resource(struct package *pkg, 
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
static void MultiObjects_ScriptEngine_a_release(struct package *pkg, 
												struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}


/* 封包处理回调函数集合 */
static cui_ext_operation MultiObjects_ScriptEngine_a_operation = {
	MultiObjects_ScriptEngine_a_match,					/* match */
	MultiObjects_ScriptEngine_a_extract_directory,		/* extract_directory */
	MultiObjects_ScriptEngine_a_parse_resource_info,	/* parse_resource_info */
	MultiObjects_ScriptEngine_a_extract_resource,		/* extract_resource */
	MultiObjects_ScriptEngine_a_save_resource,			/* save_resource */
	MultiObjects_ScriptEngine_a_release_resource,		/* release_resource */
	MultiObjects_ScriptEngine_a_release					/* release */
};


/********************* am *********************/

/* 封包匹配回调函数 */
static int MultiObjects_ScriptEngine_am_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "am00", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int MultiObjects_ScriptEngine_am_extract_directory(struct package *pkg,
														  struct package_directory *pkg_dir)
{
	u32	index_length;
	u8 xor;

	if (pkg->pio->read(pkg, &index_length, 4))
		return -CUI_EREAD;

	if (pkg->pio->read(pkg, &xor, 1))
		return -CUI_EREAD;

	BYTE *index_buffer = (BYTE *)malloc(index_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	for (DWORD i = 0; i < index_length; i++)
		index_buffer[i] ^= xor;

	DWORD index_entries = 0;
	for (i = 0; i < index_length; ) {
		i += strlen((char *)index_buffer + i) + 1 + 8;		
		index_entries++;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_length;
	pkg_dir->flags = PKG_DIR_FLAG_VARLEN;
	package_set_private(pkg, index_length + sizeof(AMHEAD));

	return 0;
}

/* 封包索引项解析函数 */
static int MultiObjects_ScriptEngine_am_parse_resource_info(struct package *pkg,
														   struct package_resource *pkg_res)
{
	BYTE *am_entry;
	unsigned long base_offset;

	am_entry = (BYTE *)pkg_res->actual_index_entry;
	base_offset = package_get_private(pkg);
	pkg_res->name_length = strlen((char *)am_entry);
	strncpy(pkg_res->name, (char *)am_entry, pkg_res->name_length + 1);
	am_entry += pkg_res->name_length + 1;
	pkg_res->offset = *(u32 *)am_entry + base_offset;
	am_entry += 4;
	pkg_res->raw_data_length = *(u32 *)am_entry;
	pkg_res->actual_data_length = 0;	
	pkg_res->actual_index_entry_length = pkg_res->name_length + 1 + 8;

	return 0;
}

/* 封包资源提取函数 */
static int MultiObjects_ScriptEngine_am_extract_resource(struct package *pkg,
														struct package_resource *pkg_res)
{
	BYTE *raw, *act_data;
	DWORD raw_len, act_data_length;

	raw_len = pkg_res->raw_data_length;
	raw = (BYTE *)pkg->pio->readvec_only(pkg, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET);
	if (!raw)
		return -CUI_EREADVECONLY;

	act_data_length = raw_len;
	act_data = (BYTE *)malloc(act_data_length);
	if (!act_data)
		return -CUI_EMEM;

	for (DWORD i = 0; i < raw_len; i++)
		act_data[i] = raw[i] ^ am_decode_table[i & 0xffff];

	pkg_res->raw_data = raw;
	pkg_res->raw_data_length = raw_len;
	pkg_res->actual_data = act_data;
	pkg_res->actual_data_length = act_data_length;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation MultiObjects_ScriptEngine_am_operation = {
	MultiObjects_ScriptEngine_am_match,					/* match */
	MultiObjects_ScriptEngine_am_extract_directory,		/* extract_directory */
	MultiObjects_ScriptEngine_am_parse_resource_info,	/* parse_resource_info */
	MultiObjects_ScriptEngine_am_extract_resource,		/* extract_resource */
	MultiObjects_ScriptEngine_a_save_resource,			/* save_resource */
	MultiObjects_ScriptEngine_a_release_resource,		/* release_resource */
	MultiObjects_ScriptEngine_a_release					/* release */
};

/********************* px0090 *********************/

/* 封包匹配回调函数 */
static int MultiObjects_ScriptEngine_px0090_match(struct package *pkg)
{
	px0090_header_t px0090_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &px0090_header, sizeof(px0090_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if ((px0090_header.type != 0x0090) || strncmp(px0090_header.magic, "LeafAquaPlus", 12)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (px0090_header.reserved0)
		printf("reserved0: %x\n", px0090_header.reserved0);
	if (px0090_header.reserved1)
		printf("reserved1: %x\n", px0090_header.reserved1);
	if (px0090_header.reserved2)
		printf("reserved2: %x\n", px0090_header.reserved2);

	return 0;	
}

/* 封包索引目录提取函数 */
static int MultiObjects_ScriptEngine_px0090_extract_directory(struct package *pkg,
															  struct package_directory *pkg_dir)
{
	px0090_header_t px0090_header;
	DWORD index_buffer_length;	

	if (pkg->pio->readvec(pkg, &px0090_header, sizeof(px0090_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	index_buffer_length = px0090_header.pictures * sizeof(my_px_entry_t);
	my_px_entry_t *index_buffer = (my_px_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	DWORD offset = sizeof(px0090_header_t);
	for (DWORD i = 0; i < px0090_header.pictures; i++) {
		px000a_header_t px000a_header;

		if (pkg->pio->readvec(pkg, &px000a_header, sizeof(px000a_header), offset, IO_SEEK_SET)) {
			free(index_buffer);
			return -CUI_EREADVEC;		
		}

		swprintf(index_buffer[i].name, L"%s.%04d.bmp", pkg->name, i);
		offset += sizeof(px000a_header);
		index_buffer[i].offset = offset;
		index_buffer[i].length = px000a_header.width * px000a_header.height * px000a_header.bitcount / 8;
		offset += index_buffer[i].length;
		index_buffer[i].width = px000a_header.width;
		index_buffer[i].height = px000a_header.height;
		index_buffer[i].bitcount = px000a_header.bitcount;

		if (px000a_header.type != 0x000a) {
			wcprintf(index_buffer[i].name);
			printf(" not type 0x000a, %x\n", px000a_header.type);
			exit(0);
		}
	}

	pkg_dir->index_entries = px0090_header.pictures;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(my_px_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int MultiObjects_ScriptEngine_px0090_parse_resource_info(struct package *pkg,
																struct package_resource *pkg_res)
{
	my_px_entry_t *my_px_entry;

	my_px_entry = (my_px_entry_t *)pkg_res->actual_index_entry;
	wcscpy((WCHAR *)pkg_res->name, my_px_entry->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = my_px_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = my_px_entry->offset;
	pkg_res->flags |= PKG_RES_FLAG_UNICODE;

	return 0;
}

/* 封包资源提取函数 */
static int MultiObjects_ScriptEngine_px0090_extract_resource(struct package *pkg,
															 struct package_resource *pkg_res)
{
	BYTE *raw;
	my_px_entry_t *my_px_entry;

	raw = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET)) {
		free(raw);
		return -CUI_EREADVEC;
	}

	my_px_entry = (my_px_entry_t *)pkg_res->actual_index_entry;
	if (MyBuildBMPFile(raw, pkg_res->raw_data_length, NULL, 0, my_px_entry->width,
			0 - my_px_entry->height, my_px_entry->bitcount, (BYTE **)&pkg_res->actual_data,
			&pkg_res->actual_data_length, my_malloc)) {
		free(raw);
		return -CUI_EMEM;
	}

	pkg_res->raw_data = raw;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation MultiObjects_ScriptEngine_px0090_operation = {
	MultiObjects_ScriptEngine_px0090_match,					/* match */
	MultiObjects_ScriptEngine_px0090_extract_directory,		/* extract_directory */
	MultiObjects_ScriptEngine_px0090_parse_resource_info,	/* parse_resource_info */
	MultiObjects_ScriptEngine_px0090_extract_resource,		/* extract_resource */
	MultiObjects_ScriptEngine_a_save_resource,			/* save_resource */
	MultiObjects_ScriptEngine_a_release_resource,			/* release_resource */
	MultiObjects_ScriptEngine_a_release					/* release */
};


/********************* px0080 *********************/

static int MultiObjects_ScriptEngine_px0080_match(struct package *pkg)
{
	px0080_header_t px0080_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &px0080_header, sizeof(px0080_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (!(px0080_header.type & 0x0080) || !px0080_header.pictures 
			|| strncmp(px0080_header.magic, "LeafAquaPlus", 12)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (px0080_header.reserved0)
		printf("reserved0: %x\n", px0080_header.reserved0);
	if (px0080_header.reserved1)
		printf("reserved1: %x\n", px0080_header.reserved1);
	if (px0080_header.reserved2)
		printf("reserved2: %x\n", px0080_header.reserved2);
	if (px0080_header.reserved3)
		printf("reserved3: %x\n", px0080_header.reserved3);

	return 0;	
}

/* 封包索引目录提取函数 */
static int MultiObjects_ScriptEngine_px0080_extract_directory(struct package *pkg,
															  struct package_directory *pkg_dir)
{
	px0080_header_t px0080_header;
	DWORD index_buffer_length;	
	u32 *offset_buffer;
	SIZE_T px_size;

	if (pkg->pio->length_of(pkg, &px_size))
		return -CUI_ELEN;

	if (pkg->pio->readvec(pkg, &px0080_header, sizeof(px0080_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	offset_buffer = (u32 *)malloc(px0080_header.pictures * 4);
	if (!offset_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, offset_buffer, px0080_header.pictures * 4)) {
		free(offset_buffer);
		return -CUI_EREAD;
	}

	index_buffer_length = px0080_header.pictures * sizeof(my_px_entry_t);
	my_px_entry_t *index_buffer = (my_px_entry_t *)malloc(index_buffer_length);
	if (!index_buffer) {
		free(offset_buffer);
		return -CUI_EMEM;
	}

	DWORD offset = sizeof(px0080_header_t) + px0080_header.pictures * 4;
	for (DWORD i = 0; i < px0080_header.pictures; i++) {
		px000a_header_t px000a_header;

		if (pkg->pio->readvec(pkg, &px000a_header, sizeof(px000a_header), offset, IO_SEEK_SET)) {
			free(index_buffer);
			return -CUI_EREADVEC;		
		}

		swprintf(index_buffer[i].name, L"%s.%04d.bmp", pkg->name, i);
		index_buffer[i].offset = offset + offset_buffer[i];
#if 0
		index_buffer[i].length = px000a_header.width * px000a_header.height * px000a_header.bitcount / 8;
		index_buffer[i].width = px000a_header.width;
		index_buffer[i].height = px000a_header.height;
		index_buffer[i].bitcount = px000a_header.bitcount;

		if (px000a_header.type != 0x000a) {
			wcprintf(index_buffer[i].name);
			printf(" not type 0x000a, %x\n", px000a_header.type);
			exit(0);
		}
#endif
	}

	for (i = 0; i < px0080_header.pictures - 1; i++)
		index_buffer[i].length = index_buffer[i+1].offset - index_buffer[i].offset;
	index_buffer[i].length = px_size - index_buffer[i].offset;	

	pkg_dir->index_entries = px0080_header.pictures;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(my_px_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int MultiObjects_ScriptEngine_px0080_parse_resource_info(struct package *pkg,
																struct package_resource *pkg_res)
{
	my_px_entry_t *my_px_entry;

	my_px_entry = (my_px_entry_t *)pkg_res->actual_index_entry;
	wcscpy((WCHAR *)pkg_res->name, my_px_entry->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = my_px_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = my_px_entry->offset;
	pkg_res->flags |= PKG_RES_FLAG_UNICODE;

	return 0;
}

/* 封包资源提取函数 */
static int MultiObjects_ScriptEngine_px0080_extract_resource(struct package *pkg,
															 struct package_resource *pkg_res)
{
	BYTE *raw;
//	my_px_entry_t *my_px_entry;

	raw = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET)) {
		free(raw);
		return -CUI_EREADVEC;
	}
#if 0
	my_px_entry = (my_px_entry_t *)pkg_res->actual_index_entry;
	if (MyBuildBMPFile(raw, pkg_res->raw_data_length, NULL, 0, my_px_entry->width,
			0 - my_px_entry->height, my_px_entry->bitcount, (BYTE **)&pkg_res->actual_data,
			&pkg_res->actual_data_length, my_malloc)) {
		free(raw);
		return -CUI_EMEM;
	}
#endif
	pkg_res->raw_data = raw;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation MultiObjects_ScriptEngine_px0080_operation = {
	MultiObjects_ScriptEngine_px0080_match,					/* match */
	MultiObjects_ScriptEngine_px0080_extract_directory,		/* extract_directory */
	MultiObjects_ScriptEngine_px0080_parse_resource_info,	/* parse_resource_info */
	MultiObjects_ScriptEngine_px0080_extract_resource,		/* extract_resource */
	MultiObjects_ScriptEngine_a_save_resource,				/* save_resource */
	MultiObjects_ScriptEngine_a_release_resource,			/* release_resource */
	MultiObjects_ScriptEngine_a_release					/* release */
};

#if 0
 else if (strstr(pkg_res->name, ".px")) {
		px_header_t *px_header = (px_header_t *)act_data;
#if 0
		// sys.a: leaflogo.px
		if (px_header->type != 0x0c {
			free(uncompr);
			return -CUI_EMATCH;
		}
		
		if (px_header->bitCnt == 8) {
			
			
		} else if (px_header->bitCnt != 32) {

		} else {
			free(uncompr);
			return -CUI_EMATCH;
		}
#endif
#endif

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK MultiObjects_ScriptEngine_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".a"), NULL, 
		NULL, &MultiObjects_ScriptEngine_a_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".am"), NULL, 
		NULL, &MultiObjects_ScriptEngine_am_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".px"), NULL, 
		NULL, &MultiObjects_ScriptEngine_px0090_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
			return -1;

//	if (callback->add_extension(callback->cui, _T(".px"), NULL, 
//		NULL, &MultiObjects_ScriptEngine_px0080_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
//			return -1;

	return 0;
}
