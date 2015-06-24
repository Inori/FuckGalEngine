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

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information BSS_cui_information = {
	_T("BISHOP"),			/* copyright */
	_T(""),					/* system */
	_T(".bsa"),				/* package */
	_T("0.5.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-7-18 23:45"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[8];		/* "BSArc" */
	u16 version;		/* 1 or 2 */
	u16 index_entries;
	u32 index_offset; 		
} bsa_header_t;

typedef struct {
	s8 name[32];
	u32 offset;
	u32 length;
} bsa_entry_t;

typedef struct {
	s8 magic[16];			// [0x00]"BSS-Graphics"
	u8 unknown;				// [0x10]0
	u8 number;				// [0x11]图片个数(1)
	u32 uncomprlen;			// [0x12]解压缩数据长度
	u16 width;				// [0x16]
	u16 height;				// [0x16]
	u16 unknown0;			// [0x1a]no use
	u32 unknown1;			// [0x1c]no use	
	u16 unknown2;			// [0x20]
	u16 unknown3;			// [0x22]	
	u16 unknown4;			// [0x24]
	u16 unknown5;			// [0x26]
	u16 width0;				// [0x28]
	u16 height0;			// [0x2a]	
	u8 unknown6;			// [0x2c]
	u8 unknown7;			// [0x2d]
	u16 unknown8;			// [0x2e]no use	
	u8 color_type;			// [0x30]0: 32 bits; 1: 24 bits+mask; 2: 8 bits
	u8 mode;				// [0x31]0 or 1
	u32 compr_data_offset;	// [0x32]压缩数据的起始位置（第一个4字节是本段的压缩长度）
	u32 compr_data_length;	// [0x36]mode==0: 实际数据长度（没有长度字段，也没有压缩）; mode==1: 压缩数据的长度（不包含各段的压缩长度字段）
	u32 palette_offset;		// [0x3a]
	u8 reserved[2];
} bsg_header_t;

typedef struct {
	s8 magic[16];			// [0x00]"BSS-Composition"
	u8 unknown;				// [0x10]
	u8 number;				// [0x11]图片个数
	u32 uncomprlen;			// [0x12]解压缩数据长度
	u16 width;				// [0x16]
	u16 height;				// [0x16]
	u16 unknown0;			// [0x1a]no use
	u32 unknown1;			// [0x1c]no use	
} bsc_header_t;
#pragma pack ()

typedef struct {
	s8 name[32];
	u32 offset;
	u32 length;
} bsc_entry_t;

static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static void bsg_uncompress(BYTE *uncompr, BYTE *compr, DWORD comprlen, int step)
{
	DWORD curbyte = 0;

	while (curbyte < comprlen) {
		BYTE flag;
		int copy_bytes, i;

		flag = compr[curbyte++];
		if ((char)flag >= 0) {	/* 拷贝非重复的数据 */
			copy_bytes = flag + 1;
			for (i = 0; i < copy_bytes; i++) {
				*uncompr = compr[curbyte++];
				uncompr += step;
			}
		} else {
			if (257 - flag > 0) {	/* 拷贝重复的数据 */
				copy_bytes = 257 - flag;
				for (i = 0; i < copy_bytes; i++) {
					*uncompr = compr[curbyte];
					uncompr += step;
				}
			}
			curbyte++;
		}
	}
}

/********************* bsa *********************/

/* 封包匹配回调函数 */
static int BSS_bsa_match(struct package *pkg)
{
	s8 magic[8];
	u16 version;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "BSArc", sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (pkg->pio->read(pkg, &version, sizeof(version))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (version != 1 && version != 2) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int BSS_bsa_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{
	bsa_entry_t *index_buffer;
	u16 index_entries;
	u32 index_offset;
	DWORD index_buffer_length;	

	if (pkg->pio->read(pkg, &index_entries, sizeof(index_entries)))
		return -CUI_EREAD;

	if (pkg->pio->read(pkg, &index_offset, sizeof(index_offset)))
		return -CUI_EREAD;

	index_buffer_length = index_entries * sizeof(bsa_entry_t);
	index_buffer = (bsa_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, index_buffer, index_buffer_length,
			index_offset, IO_SEEK_SET)) {
		free(index_buffer);
		return -CUI_EREADVEC;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(bsa_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;	// 针对ver2中出现的无效文件

	return 0;
}

/* 封包索引项解析函数 */
static int BSS_bsa_parse_resource_info(struct package *pkg,
									   struct package_resource *pkg_res)
{
	bsa_entry_t *bsa_entry;

	bsa_entry = (bsa_entry_t *)pkg_res->actual_index_entry;
	if (bsa_entry->name[0] == '>')	// 表示无效的文件
		bsa_entry->name[0] = '-';
	strncpy(pkg_res->name, bsa_entry->name, sizeof(bsa_entry->name));
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = bsa_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = bsa_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int BSS_bsa_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD uncomprlen, comprlen;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)pkg->pio->readvec_only(pkg, comprlen, pkg_res->offset, IO_SEEK_SET);
	if (!compr)
		return -CUI_EREADVECONLY;

	if (!strncmp((char *)compr, "BSS-Graphics", 13)) {
		bsg_header_t *bsg_header = (bsg_header_t *)compr;
		int bpp;
		BYTE *compr_data;

		if (bsg_header->mode != 0 && bsg_header->mode != 1)
			return -CUI_EMATCH;

		if (bsg_header->color_type != 0 && bsg_header->color_type != 1
				&& bsg_header->color_type != 2)
			return -CUI_EMATCH;

		uncompr = (BYTE *)malloc(bsg_header->uncomprlen);
		if (!uncompr)
			return -CUI_EMEM;

		compr_data = compr + bsg_header->compr_data_offset;
		BYTE *uncompr_data = uncompr;

		if (bsg_header->mode == 1) {
			if (bsg_header->color_type == 2)
				bsg_uncompress(uncompr_data, compr_data + 4, *(u32 *)compr_data, 1);
			else if (bsg_header->color_type == 1) {
				for (DWORD p = 0; p < 3; p++) {
					bsg_uncompress(uncompr_data + p, compr_data + 4, *(u32 *)compr_data, 4);
					compr_data += 4 + *(u32 *)compr_data;
				}
				/* 游戏中根据uncomprlen长度将alpha都设为0xff */
				for (p = 0; p < bsg_header->uncomprlen; p += 4) {
					uncompr_data[3] = 0xff;
					uncompr_data += 4;
				}				
			} else if (bsg_header->color_type == 0) {
				for (int p = 0; p < 4; p++) {
					bsg_uncompress(uncompr_data + p, compr_data + 4, *(u32 *)compr_data, 4);
					compr_data += 4 + *(u32 *)compr_data;
				}
			}
		} else if (bsg_header->mode == 0) {
			if (bsg_header->color_type == 0)
				memcpy(uncompr_data, compr_data, bsg_header->compr_data_length);
			else if (bsg_header->color_type == 1) {
				int count = (bsg_header->compr_data_length - 1) / 3 + 1;

				for (int p = 0; p < count; p++) {
					*uncompr_data++ = *compr_data++;
					*uncompr_data++ = *compr_data++;
					*uncompr_data++ = *compr_data++;
				}
			} else if (bsg_header->color_type == 2)
				memcpy(uncompr_data, compr_data, bsg_header->compr_data_length);
		}

		BYTE *palette;
		DWORD palette_size;
		if (bsg_header->color_type == 0) {
			bpp = 32;
			palette = NULL;
			palette_size = 0;
		} else if (bsg_header->color_type == 1) {
			bpp = 32;	/* 24 Bits + 不透名alpha */
			palette = NULL;
			palette_size = 0;
		} else if (bsg_header->color_type == 2) {
			bpp = 8;
			if (bsg_header->palette_offset) {
				palette = compr + bsg_header->palette_offset;
				palette_size = 1024;
			} else {
				palette = NULL;
				palette_size = 0;
			}
		}

		uncomprlen = bsg_header->uncomprlen;
		if (MyBuildBMPFile(uncompr, uncomprlen, palette, palette_size, bsg_header->width,
				bsg_header->height, bpp, (BYTE **)&pkg_res->actual_data, 
				&pkg_res->actual_data_length, my_malloc)) {
			free(uncompr);
			free(compr);
			return -CUI_EMEM;
		}
		free(uncompr);
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp");
	} else if (!strncmp((char *)compr, "BSS-Composition", 16)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bsc");
	}

	pkg_res->raw_data = compr;

	return 0;
}

/* 资源保存函数 */
static int BSS_bsa_save_resource(struct resource *res, 
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
static void BSS_bsa_release_resource(struct package *pkg, 
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
static void BSS_bsa_release(struct package *pkg, 
							struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation BSS_bsa_operation = {
	BSS_bsa_match,					/* match */
	BSS_bsa_extract_directory,		/* extract_directory */
	BSS_bsa_parse_resource_info,	/* parse_resource_info */
	BSS_bsa_extract_resource,		/* extract_resource */
	BSS_bsa_save_resource,			/* save_resource */
	BSS_bsa_release_resource,		/* release_resource */
	BSS_bsa_release					/* release */
};

/********************* bsc *********************/

/* 封包匹配回调函数 */
static int BSS_bsc_match(struct package *pkg)
{
	s8 magic[16];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "BSS-Composition", sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int BSS_bsc_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{
	bsc_header_t bsc_header;
	bsc_entry_t *index_buffer;
	DWORD index_buffer_length;	

	if (pkg->pio->readvec(pkg, &bsc_header, sizeof(bsc_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	index_buffer_length = bsc_header.number * sizeof(bsc_entry_t);
	index_buffer = (bsc_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	DWORD offset = sizeof(bsc_header_t);
	for (DWORD i = 0; i < bsc_header.number; i++) {
		bsg_header_t bsg_header;

		if (pkg->pio->readvec(pkg, &bsg_header, sizeof(bsg_header), offset, IO_SEEK_SET))
			break;
		sprintf(index_buffer[i].name, "%08d", i);
		index_buffer[i].offset = offset;
		index_buffer[i].length = sizeof(bsg_header) + bsg_header.compr_data_length;
		offset += index_buffer[i].length;
	}
	if (i != bsc_header.number) {
		free(index_buffer);
		return -CUI_EREADVEC;
	}

	pkg_dir->index_entries = bsc_header.number;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(bsc_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int BSS_bsc_parse_resource_info(struct package *pkg,
									   struct package_resource *pkg_res)
{
	bsc_entry_t *bsc_entry;

	bsc_entry = (bsc_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, bsc_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = bsc_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = bsc_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int BSS_bsc_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD uncomprlen, comprlen;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	if (!strncmp((char *)compr, "BSS-Graphics", 13)) {
		bsg_header_t *bsg_header = (bsg_header_t *)compr;
		int bpp;
		BYTE *compr_data;

		if (bsg_header->mode != 0 && bsg_header->mode != 1) {
			free(compr);
			return -CUI_EMATCH;
		}

		if (bsg_header->color_type != 0 && bsg_header->color_type != 1
				&& bsg_header->color_type != 2) {
			free(compr);
			return -CUI_EMATCH;
		}

		uncompr = (BYTE *)malloc(bsg_header->uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}

		compr_data = compr + bsg_header->compr_data_offset;
		BYTE *uncompr_data = uncompr;

		if (bsg_header->mode == 1) {
			if (bsg_header->color_type == 2)
				bsg_uncompress(uncompr_data, compr_data + 4, *(u32 *)compr_data, 1);
			else if (bsg_header->color_type == 1) {
				for (DWORD p = 0; p < 3; p++) {
					bsg_uncompress(uncompr_data + p, compr_data + 4, *(u32 *)compr_data, 4);
					compr_data += 4 + *(u32 *)compr_data;
				}
				/* 游戏中根据uncomprlen长度将alpha都设为0xff */
				for (p = 0; p < bsg_header->uncomprlen; p += 4) {
					uncompr_data[3] = 0xff;
					uncompr_data += 4;
				}				
			} else if (bsg_header->color_type == 0) {
				for (int p = 0; p < 4; p++) {
					bsg_uncompress(uncompr_data + p, compr_data + 4, *(u32 *)compr_data, 4);
					compr_data += 4 + *(u32 *)compr_data;
				}
			}
		} else if (bsg_header->mode == 0) {
			if (bsg_header->color_type == 0)
				memcpy(uncompr_data, compr_data, bsg_header->compr_data_length);
			else if (bsg_header->color_type == 1) {
				int count = (bsg_header->compr_data_length - 1) / 3 + 1;

				for (int p = 0; p < count; p++) {
					*uncompr_data++ = *compr_data++;
					*uncompr_data++ = *compr_data++;
					*uncompr_data++ = *compr_data++;
				}
			} else if (bsg_header->color_type == 2)
				memcpy(uncompr_data, compr_data, bsg_header->compr_data_length);
		}

		BYTE *palette;
		DWORD palette_size;
		if (bsg_header->color_type == 0) {
			bpp = 32;
			palette = NULL;
			palette_size = 0;
		} else if (bsg_header->color_type == 1) {
			bpp = 32;	/* 24 Bits + 不透名alpha */
			palette = NULL;
			palette_size = 0;
		} else if (bsg_header->color_type == 2) {
			bpp = 8;
			if (bsg_header->palette_offset) {
				palette = compr + bsg_header->palette_offset;
				palette_size = 1024;
			} else {
				palette = NULL;
				palette_size = 0;
			}
		}

		uncomprlen = bsg_header->uncomprlen;
		if (MyBuildBMPFile(uncompr, uncomprlen, palette, palette_size, bsg_header->width,
				bsg_header->height, bpp, (BYTE **)&pkg_res->actual_data, 
				&pkg_res->actual_data_length, my_malloc)) {
			free(uncompr);
			free(compr);
			return -CUI_EMEM;
		}
		free(uncompr);
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp");
	}

	pkg_res->raw_data = compr;

	return 0;
}

static void BSS_bsc_release_resource(struct package *pkg, 
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
static cui_ext_operation BSS_bsc_operation = {
	BSS_bsc_match,					/* match */
	BSS_bsc_extract_directory,		/* extract_directory */
	BSS_bsc_parse_resource_info,	/* parse_resource_info */
	BSS_bsc_extract_resource,		/* extract_resource */
	BSS_bsa_save_resource,			/* save_resource */
	BSS_bsc_release_resource,		/* release_resource */
	BSS_bsa_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK BSS_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".bsa"), NULL, 
		NULL, &BSS_bsa_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".bsc"), NULL, 
		NULL, &BSS_bsc_operation, CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
