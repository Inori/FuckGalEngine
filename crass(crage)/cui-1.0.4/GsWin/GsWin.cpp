#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <resource.h>
#include <package.h>
#include <cui_error.h>
#include <utility.h>
#include <stdio.h>

struct acui_information GsWin_cui_information = {
	_T("株式会社ちぇりーそふと"),	/* copyright */
	_T("GsWin"),					/* system */
	_T(".pak .pa_ .dat"),			/* package */
	_T("1.1.0"),					/* revision */
	_T("痴漢公賊"),					/* author */
	_T("2009-2-14 14:05"),			/* date */
	NULL,							/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

#pragma pack (1)
typedef struct {
	s8 magic[16];		/* "DataPack5" or "GsPack4 abc" */
	s8 description[32];	/* "GsPackFile5" or "GsPackFile4" */
	u16 minor_version;
	u16 major_version;
	u32 index_length;	/* 压缩的索引段长度 */
	u32 decode_key;	
	u32 index_entries;		
	u32 data_offset;
	u32 index_offset;	/* 压缩的索引段长度 */
} pak_header_t;

typedef struct {
	s8 name[64];
	u32 offset;		/* 距离数据段的偏移 */
	u32 length;		/* 4字节对齐 */
	u32 unknown0;	// 非v5没有这些字段
	u32 unknown1;
	BYTE pad[24];
} pak_entry_t;

typedef struct {		// 0x74 bytes
	u32 unknown;
	u32 comprlen;
	u32 uncomprlen;
	u32 data_offset;			
	u32 unknown1;
	u32 width;
	u32 height;
	u32 color_depth;
	u8 reserved[84];
} grp_header_t;

typedef struct {	// 0x1c8
	s8 magic[16];					// "Scw5.x"
	u16 minor_version;
	u16 major_version;
	u32 is_compr;
	u32 uncomprlen;
	u32 comprlen;	
	u32 always_1;					// 1
	u32 instruction_table_entries;	// 脚本指令的个数
	u32 string_table_entries;		// 字符串的个数
	u32 unknown_table_entries;		// ？？的个数
	u32 instruction_data_length;	// 脚本数据总长度
	u32 string_data_length;			// 字符串数据总长度
	u32 unknown_data_length;		// ？？数据总长度
	u8 pad[0x18c];
} scw5_header_t;

typedef struct {	// 0x1c4
	s8 magic[16];					// "Scw4.x"
	u16 minor_version;
	u16 major_version;
	u32 is_compr;
	u32 uncomprlen;
	u32 comprlen;	
	u32 always_1;					// 1
	u32 instruction_table_entries;	// 脚本指令的个数
	u32 string_table_entries;		// 字符串的个数
	u32 unknown_table_entries;		// ？？的个数
	u32 instruction_data_length;	// 脚本数据总长度
	u32 string_data_length;			// 字符串数据总长度
	u32 unknown_data_length;		// ？？数据总长度
	u8 pad[0x188];
} scw4_header_t;

typedef struct {	// 0x100
	s8 magic[16];					// "SCW for GsWin3"
	u16 minor_version;
	u16 major_version;
	u32 is_compr;
	u32 uncomprlen;
	u32 always_1;					// 1
	u32 instruction_table_entries;	// 脚本指令的个数(GsWin3: x16)
	u32 string_table_entries;		// 字符串的个数(GsWin3: x16)
	u32 unknown_table_entries;		// ？？的个数(GsWin3: x16)
	u32 instruction_data_length;	// 脚本数据总长度
	u32 string_data_length;			// 字符串数据总长度
	u32 unknown_data_length;		// ？？数据总长度
	u32 unknown_data1_entries;		// ？？(GsWin3: x8)
	u32 unknown_data2_entries;		// ？？(GsWin3: x8)
	u8 pad[0xc0];
} scw3_header_t;

typedef struct {	// 脚本指令表
	u32 offset;		// 脚本指令的偏移（ 相对于脚本指令表起始）
	u32 length;		// 脚本指令的长度
} instruction_table_entry_t;

typedef struct {	// 字符串表
	u32 offset;		// 字符串的偏移（相对于脚本指令表起始）
	u32 length;		// 字符串的长度（包含NULL）
} string_table_entry_t;

typedef struct {	// ？？表
	u32 offset;		// ？？的偏移（相对于？？表起始）
	u32 length;		// ？？的长度
} unknown_table_entry_t;

typedef struct {			// 0x118
	s8 magic[17];			// "GsSYMBOL5BINDATA"
	u8 pad0[0x93];
	u32 header_size;		// [a4]
	u32 index_entries;		// [a8]
//	u32 ???					// [ac]

	u8 pad1[12];	
	
	u32 index_offset;		// [b8]
	u32 index_length;		// [bc]
	u32 decode_key;			// [c0]
	u32 actual_index_length;// [c4]
	u32 data_offset;		// [c8]
	u32 data_length;		// [cc]
	
	u8 pad2[0x48];
} dat_header_t;

typedef struct {	// 0x18
	u32 offset;	
	u32 comprlen;
	u32 uncomprlen;
	u32 unknown[3];
} dat_entry_t;

typedef struct {
	s8 magic[20];		/* "CHERRY PACK 3.0" */
	u32 index_entries;
	u32 daata_offset;
} pak3_header_t;

typedef struct {
	s8 name[16];
	u32 offset;
	u32 length;
} pak3_entry_t;

typedef struct {
	u32 comprlen;
	u32 uncomprlen;
	u32 unknown[2];	// 0xffff0000, 0
	u32 width;
	u32 height;
	u32 color_depth;
	u16 orig_x;		// 合成图(比如眼神图)在基础图中的位置
	u16 orig_y;
	u16 x;			// (眼神图的)尺寸
	u16 y;
	u32 pics;		// 可选的表情数(总数-1，因为第一个表情总是和基础图一致)
} grp3_header_t;
#pragma pack ()		
	
	
static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static inline unsigned char getbit_le(unsigned char byte, unsigned int pos)
{
	return !!(byte & (1 << pos));
}

static inline unsigned char getbyte_le(unsigned char byte)
{
	return byte;
}

static int lzss_decompress(unsigned char *uncompr, unsigned int uncomprlen,
				unsigned char *compr, unsigned int comprlen)
{
	unsigned int act_uncomprlen = 0;
	unsigned int curbyte = 0;		/* compr中的当前扫描字节 */
	unsigned int nCurWindowByte = 0x0fee;
	unsigned char win[4096];
	unsigned int win_size = 4096;
	
	memset(win, 0, win_size);
	/* 因为flag的实际位数不明，因此由flag引起的compr下溢的问题都不算错误 */
	while (1) {
		unsigned char bitmap;
		
		if (curbyte >= comprlen)
			break;

		bitmap = getbyte_le(compr[curbyte++]);
		for (unsigned int curbit_bitmap = 0; curbit_bitmap < 8; curbit_bitmap++) {
			/* 如果为1, 表示接下来的1个字节原样输出 */
			if (getbit_le(bitmap, curbit_bitmap)) {
				unsigned char data;

				if (curbyte >= comprlen)
					goto out;

				data = getbyte_le(compr[curbyte++]);					
				
				if (act_uncomprlen >= uncomprlen)
					goto out;

				/* 输出1字节非压缩数据 */
				uncompr[act_uncomprlen++] = data;		
				/* 输出的1字节放入滑动窗口 */
				win[nCurWindowByte++] = data;
				nCurWindowByte &= win_size - 1;
			} else {
				unsigned int copy_bytes, win_offset;
				
				if (curbyte >= comprlen)
					goto out;

				win_offset = getbyte_le(compr[curbyte++]);

				if (curbyte >= comprlen)
					goto out;

				copy_bytes = getbyte_le(compr[curbyte++]);
				win_offset |= (copy_bytes >> 4) << 8;
				copy_bytes &= 0x0f;
				copy_bytes += 3;	

				for (unsigned int i = 0; i < copy_bytes; i++) {
					unsigned char data;
					
					data = win[(win_offset + i) & (win_size - 1)];
					if (act_uncomprlen >= uncomprlen)
						goto out;
	
					/* 输出1字节非压缩数据 */
					uncompr[act_uncomprlen++] = data;		
					/* 输出的1字节放入滑动窗口 */
					win[nCurWindowByte++] = data;
					nCurWindowByte &= win_size - 1;		
				}
			}
		}
	}
out:	
	return act_uncomprlen;	
}

/********************* pak/pa_ *********************/

static int GsWin_pak_match(struct package *pkg)
{	
	s8 magic[16];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strcmp(magic, "DataPack5") && strcmp(magic, "GsPack4 abc")) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int GsWin_pak_extract_directory(struct package *pkg,
											  struct package_directory *pkg_dir)
{		
	unsigned int index_entry_size, index_buffer_len, act_uncomprlen;
	u16 maj_ver, min_ver;
	u32 compr_index_buffer_len, decode_key, index_entries, data_offset, index_offset;
	void *index_buffer;

	/* 跨过描述字段 */
	if (pkg->pio->seek(pkg, 48, IO_SEEK_SET))
		return -CUI_EREAD;
	
	if (pkg->pio->read(pkg, &min_ver, 2))
		return -CUI_EREAD;
	
	if (pkg->pio->read(pkg, &maj_ver, 2))
		return -CUI_EREAD;

	if (maj_ver == 5)
		index_entry_size = sizeof(pak_entry_t);	// 0x68	
	else
		index_entry_size = 0x48;
	
	if (pkg->pio->read(pkg, &compr_index_buffer_len, 4))
		return -CUI_EREAD;
	
	if (pkg->pio->read(pkg, &decode_key, 4))
		return -CUI_EREAD;
	
	if (pkg->pio->read(pkg, &index_entries, 4))
		return -CUI_EREAD;
	
	if (pkg->pio->read(pkg, &data_offset, 4))
		return -CUI_EREAD;
	
	if (pkg->pio->read(pkg, &index_offset, 4))
		return -CUI_EREAD;

	BYTE *compr = (BYTE *)malloc(compr_index_buffer_len);
	if (!compr)
		return -CUI_EMEM;	
	
	if (pkg->pio->readvec(pkg, compr, compr_index_buffer_len, index_offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREAD;
	}
	
	if (decode_key) {
		decode_key &= 0xff;
		for (unsigned int i = 0; i < compr_index_buffer_len; i++)
			compr[i] ^= (i & decode_key);
	}
	
	index_buffer_len = index_entries * index_entry_size;		
	index_buffer = malloc(index_buffer_len);
	if (!index_buffer) {
		free(compr);
		return -CUI_EMEM;
	}
	memset(index_buffer, 0, index_buffer_len);

	act_uncomprlen = lzss_decompress((BYTE *)index_buffer, index_buffer_len, compr, compr_index_buffer_len);
	free(compr);
	if (act_uncomprlen != index_buffer_len) {
		free(index_buffer);
		return -CUI_EUNCOMPR;
	}

	for (unsigned int i = 0; i < index_entries; i++)
		((pak_entry_t *)((BYTE *)index_buffer + i * index_entry_size))->offset += data_offset;
		
	pkg_dir->index_entries = index_entries;
	pkg_dir->index_entry_length = index_entry_size;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_len;

	return 0;
}

static int GsWin_pak_parse_resource_info(struct package *pkg,
												struct package_resource *pkg_res)
{
	pak_entry_t *pak_entry;

	pak_entry = (pak_entry_t *)pkg_res->actual_index_entry;
	if (!pak_entry->name[0])	// fix for メサイア and ファナティカ scr.pak
		pak_entry->name[0] = 'S';
	strncpy(pkg_res->name, pak_entry->name, 64);
	pkg_res->name[64] = 0;
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = pak_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = pak_entry->offset;

	return 0;
}

static int GsWin_pak_extract_resource(struct package *pkg,
											 struct package_resource *pkg_res)
{	
	BYTE *data, *uncompr = NULL;
	unsigned int uncomprlen = 0;

	data = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, data, pkg_res->raw_data_length,	pkg_res->offset, IO_SEEK_SET)) {
		free(data);
		return -CUI_EREADVEC;
	}

	if (!memcmp(data, "OggS", 4)) {
		pkg_res->replace_extension = _T(".ogg");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
	} else if (*(u32 *)data == 0x00040000) {
		grp_header_t *grp = (grp_header_t *)data;
		BYTE *dib;	
		unsigned int act_uncomprlen;

		if (grp->unknown1)
			printf("unknown1 %x\n", grp->unknown1);
		
		uncomprlen = grp->uncomprlen;
		dib = (BYTE *)malloc(uncomprlen);
		if (!dib) {
			free(data);
			return -CUI_EMEM;
		}
		memset(dib, 0, uncomprlen);
		
		act_uncomprlen = lzss_decompress(dib, uncomprlen, data + grp->data_offset, grp->comprlen);
		if (act_uncomprlen != uncomprlen) {
			free(dib);
			free(data);
			return -CUI_EUNCOMPR;
		}
	
		if (MyBuildBMPFile(dib, uncomprlen, 0, NULL, grp->width, 0 - grp->height, grp->color_depth, &uncompr, (DWORD *)&uncomprlen, my_malloc)) {
			free(dib);
			free(data);
			return -CUI_EMEM;			
		}
		free(dib);
		free(grp);
		data = NULL;
		pkg_res->replace_extension = _T(".bmp");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
	} else if (!strncmp((char *)data, "Scw5.x", 16)) {
		scw5_header_t *scw = (scw5_header_t *)data;
		BYTE *compr;	

		if (scw->is_compr == -1) {
			unsigned int act_uncomprlen;
			
			uncomprlen = scw->uncomprlen;
			uncompr = (BYTE *)malloc(scw->uncomprlen);
			if (!uncompr) {
				free(data);
				return -CUI_EMEM;
			}
			memset(uncompr, 0, scw->uncomprlen);
			
			compr = (BYTE *)(scw + 1);
			for (unsigned int i = 0; i < scw->comprlen; i++)
				compr[i] ^= i & 0xff;
	
			act_uncomprlen = lzss_decompress(uncompr, scw->uncomprlen, compr, scw->comprlen);
			if (act_uncomprlen != scw->uncomprlen) {
				free(uncompr);
				free(data);
				return -CUI_EUNCOMPR;
			}
		} else {
			uncomprlen = scw->uncomprlen;
			uncompr = (BYTE *)malloc(scw->uncomprlen);
			if (!uncompr) {
				free(data);
				return -CUI_EMEM;
			}
			memset(uncompr, 0, scw->uncomprlen);			
			compr = (BYTE *)(scw + 1);
			for (unsigned int i = 0; i < scw->uncomprlen; i++)
				uncompr[i] = compr[i] ^ (i & 0xff);
		}
		free(data);
		data = NULL;
		pkg_res->replace_extension = _T(".scw");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
	} else if (!strncmp((char *)data, "Scw4.x", 16)) {
		scw4_header_t *scw = (scw4_header_t *)data;
		BYTE *compr;	

		if (scw->is_compr == -1) {
			unsigned int act_uncomprlen;
			
			uncomprlen = scw->uncomprlen;
			uncompr = (BYTE *)malloc(scw->uncomprlen);
			if (!uncompr) {
				free(data);
				return -CUI_EMEM;
			}
			memset(uncompr, 0, scw->uncomprlen);
			
			compr = (BYTE *)(scw + 1);
			for (unsigned int i = 0; i < scw->comprlen; i++)
				compr[i] ^= i & 0xff;
	
			act_uncomprlen = lzss_decompress(uncompr, scw->uncomprlen, compr, scw->comprlen);
			if (act_uncomprlen != scw->uncomprlen) {
				free(uncompr);
				free(data);
				return -CUI_EUNCOMPR;
			}
		} else {
			uncomprlen = scw->uncomprlen;
			uncompr = (BYTE *)malloc(scw->uncomprlen);
			if (!uncompr) {
				free(data);
				return -CUI_EMEM;
			}
			memset(uncompr, 0, scw->uncomprlen);			
			compr = (BYTE *)(scw + 1);
			for (unsigned int i = 0; i < scw->uncomprlen; i++)
				uncompr[i] = compr[i] ^ (i & 0xff);
		}
		free(data);
		data = NULL;
		pkg_res->replace_extension = _T(".scw");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
	}
	pkg_res->raw_data = data;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;	

	return 0;
}

static int GsWin_save_resource(struct resource *res, 
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

static void GsWin_release_resource(struct package *pkg, 
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

static void GsWin_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg);
}

static cui_ext_operation GsWin_pak_operation = {
	GsWin_pak_match,					/* match */
	GsWin_pak_extract_directory,		/* extract_directory */
	GsWin_pak_parse_resource_info,	/* parse_resource_info */
	GsWin_pak_extract_resource,		/* extract_resource */
	GsWin_save_resource,				/* save_resource */
	GsWin_release_resource,			/* release_resource */
	GsWin_release					/* release */
};
	
/********************* dat *********************/

static int GsWin_dat_match(struct package *pkg)
{	
	dat_header_t *dat_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	dat_header = (dat_header_t *)malloc(sizeof(*dat_header));
	if (!dat_header) {
		pkg->pio->close(pkg);	
		return -CUI_EMEM;
	}
	
	if (pkg->pio->read(pkg, dat_header, sizeof(*dat_header))) {
		free(dat_header);
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(dat_header->magic, "GsSYMBOL5BINDATA", 17)) {
		free(dat_header);
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (dat_header->index_entries * sizeof(dat_entry_t) != dat_header->actual_index_length) {
		free(dat_header);
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}
	package_set_private(pkg, dat_header);

	return 0;	
}

static int GsWin_dat_extract_directory(struct package *pkg,
											  struct package_directory *pkg_dir)
{
	dat_header_t *dat_header;		
	unsigned int act_uncomprlen;
	void *index_buffer;
	
	dat_header = (dat_header_t *)package_get_private(pkg);

	BYTE *compr = (BYTE *)malloc(dat_header->index_length);
	if (!compr)
		return -CUI_EMEM;	
	
	if (pkg->pio->read(pkg, compr, dat_header->index_length)) {
		free(compr);
		return -CUI_EREAD;
	}
	
	dat_header->decode_key &= 0xff;
	for (unsigned int i = 0; i < dat_header->index_length; i++)
		compr[i] ^= (i & dat_header->decode_key);

	index_buffer = malloc(dat_header->actual_index_length);
	if (!index_buffer) {
		free(compr);
		return -CUI_EMEM;
	}
	memset(index_buffer, 0, dat_header->actual_index_length);

	act_uncomprlen = lzss_decompress((BYTE *)index_buffer, dat_header->actual_index_length, compr, dat_header->index_length);
	free(compr);
	if (act_uncomprlen != dat_header->actual_index_length) {
		free(index_buffer);
		return -CUI_EUNCOMPR;
	}	

	for (i = 0; i < dat_header->index_entries; i++)
		((dat_entry_t *)((BYTE *)index_buffer + i * sizeof(dat_entry_t)))->offset += dat_header->data_offset;	

	pkg_dir->index_entries = dat_header->index_entries;
	pkg_dir->index_entry_length = sizeof(dat_entry_t);
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = dat_header->actual_index_length;
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

static int GsWin_dat_parse_resource_info(struct package *pkg,
												struct package_resource *pkg_res)
{
	dat_entry_t *dat_entry;

	dat_entry = (dat_entry_t *)pkg_res->actual_index_entry;
	sprintf(pkg_res->name, "%010d", pkg_res->index_number);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = dat_entry->comprlen;
	pkg_res->actual_data_length = dat_entry->uncomprlen;
	pkg_res->offset = dat_entry->offset;

	return 0;
}

static int GsWin_dat_extract_resource(struct package *pkg,
											 struct package_resource *pkg_res)
{	
	BYTE *data, *uncompr = NULL;
	unsigned int uncomprlen = pkg_res->actual_data_length;

	data = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, data, pkg_res->raw_data_length,	pkg_res->offset, IO_SEEK_SET)) {
		free(data);
		return -CUI_EREADVEC;
	}	
	
	if (uncomprlen) {
		unsigned int act_uncomprlen;
		
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(data);
			return -CUI_EMEM;
		}
		memset(uncompr, 0, uncomprlen);
		
		act_uncomprlen = lzss_decompress(uncompr, uncomprlen, data, pkg_res->raw_data_length);
		free(data);
		if (act_uncomprlen != uncomprlen) {
			free(uncompr);
			return -CUI_EUNCOMPR;
		}
		data = NULL;
	}
	pkg_res->raw_data = data;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;	

	return 0;
}

static void GsWin_dat_release(struct package *pkg, 
									 struct package_directory *pkg_dir)
{
	dat_header_t *dat_header;

	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	dat_header = (dat_header_t *)package_get_private(pkg);
	if (dat_header) {
		free(dat_header);
		package_set_private(pkg, NULL);
	}
	pkg->pio->close(pkg);
}

static cui_ext_operation GsWin_dat_operation = {
	GsWin_dat_match,					/* match */
	GsWin_dat_extract_directory,		/* extract_directory */
	GsWin_dat_parse_resource_info,	/* parse_resource_info */
	GsWin_dat_extract_resource,		/* extract_resource */
	GsWin_save_resource,				/* save_resource */
	GsWin_release_resource,			/* release_resource */
	GsWin_dat_release				/* release */
};
	
/********************* PAK *********************/

static int GsWin3_PAK_match(struct package *pkg)
{
	s8 magic[20];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (strncmp(magic, "CHERRY PACK 3.0", sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int GsWin3_PAK_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	u32 index_entries;
	if (pkg->pio->read(pkg, &index_entries, sizeof(index_entries)))
		return -CUI_EREAD;

	u32 data_offset;
	if (pkg->pio->read(pkg, &data_offset, sizeof(data_offset)))
		return -CUI_EREAD;

	DWORD index_buffer_length = index_entries * sizeof(pak3_entry_t);
	pak3_entry_t *index_buffer = new pak3_entry_t[index_entries];
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		delete [] index_buffer;
		return -CUI_EREAD;
	}

	for (DWORD i = 0; i < index_entries; ++i)
		index_buffer[i].offset += data_offset;

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(pak3_entry_t);

	return 0;
}

static int GsWin3_PAK_parse_resource_info(struct package *pkg,
										  struct package_resource *pkg_res)
{
	pak3_entry_t *pak_entry;	

	pak_entry = (pak3_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, pak_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = pak_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = pak_entry->offset;

	return 0;
}

static int GsWin3_PAK_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	DWORD raw_len = pkg_res->raw_data_length;
	BYTE *raw = new BYTE[raw_len];
	if (!raw)
		return -CUI_EMEM;
		
	if (pkg->pio->readvec(pkg, raw, raw_len, pkg_res->offset, IO_SEEK_SET)) {
		delete [] raw;
		return -CUI_EREADVEC;
	}

	grp3_header_t *grp3_header = (grp3_header_t *)raw;
	scw3_header_t *scw3_header = (scw3_header_t *)raw;
	if (!strncmp((char *)raw, "RIFF", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".wav");
	} else if (grp3_header->comprlen + sizeof(grp3_header_t) == raw_len) {
		BYTE *compr = raw + sizeof(grp3_header_t);
		BYTE *uncompr = new BYTE[grp3_header->uncomprlen];
		if (!uncompr) {
			delete [] raw;
			return -CUI_EMEM;
		}

		DWORD act_uncomprlen = lzss_decompress(uncompr, grp3_header->uncomprlen, 
			compr, grp3_header->comprlen);
		if (act_uncomprlen != grp3_header->uncomprlen) {
			delete [] uncompr;
			delete [] raw;
			return -CUI_EUNCOMPR;
		}

		int ret;
		if (grp3_header->color_depth != 8) {
			ret = MyBuildBMPFile(uncompr, grp3_header->uncomprlen, 0, NULL, 
					grp3_header->width, 0 - grp3_header->height, grp3_header->color_depth, 
					(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, my_malloc);
		} else {
			DWORD pal_len = grp3_header->uncomprlen - grp3_header->width * grp3_header->height;
			ret = MyBuildBMPFile(uncompr + pal_len, grp3_header->width * grp3_header->height, 
				uncompr, pal_len, grp3_header->width, 0 - grp3_header->height, grp3_header->color_depth, 
				(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, my_malloc);
		}
		delete [] uncompr;
		delete [] raw;
		if (ret)
			return -CUI_EMEM;			
		raw = NULL;
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".bmp");
	} else if (!strncmp((char *)raw, "SCW for GsWin3", 16)) {
		DWORD enc_off = raw_len - scw3_header->string_data_length - scw3_header->unknown_data_length;
		BYTE *enc = raw + enc_off;
		for (DWORD i = 0; i < scw3_header->string_data_length; ++i)
			*enc++ ^= i & 0xff;
		for (i = 0; i < scw3_header->unknown_data_length; ++i)
			*enc++ ^= i & 0xff;
	}

	pkg_res->raw_data = raw;

	return 0;
}

static cui_ext_operation GsWin3_PAK_operation = {
	GsWin3_PAK_match,				/* match */
	GsWin3_PAK_extract_directory,	/* extract_directory */
	GsWin3_PAK_parse_resource_info,	/* parse_resource_info */
	GsWin3_PAK_extract_resource,	/* extract_resource */
	GsWin_save_resource,		/* save_resource */
	GsWin_release_resource,	/* release_resource */
	GsWin_release			/* release */
};

int CALLBACK GsWin_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".pak"), NULL, 
		NULL, &GsWin_pak_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &GsWin_pak_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".pa_"), NULL, 
		NULL, &GsWin_pak_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;
	
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &GsWin_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".PAK"), NULL, 
		NULL, &GsWin3_PAK_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
