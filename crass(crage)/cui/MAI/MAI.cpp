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
#include <cui_common.h>

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information MAI_cui_information = {
	_T(""),					/* copyright */
	_T(""),					/* system */
	_T(".arc"),				/* package */
	_T("0.9.9"),			/* revision */
	_T("痴汉公贼"),			/* author */
	_T("2008-5-16 21:05"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];			// "MAI\xa"
	u32 arc_size;
	u32 index_entries;		
	u8 reserved;			// 0
	u8 dir_level;			// 1表示只有1级根目录；2表示有子目录
	u16 dir_entries;		// 目录项个数
} arc_header_t;

typedef struct {
	s8 name[16];
	u32 offset;
	u32 length;
} arc_entry_t;

typedef struct {
	s8 name[4];
	u32 start_index;		// 属于该目录的资源起始编号
} arc_dir_t;

typedef struct {
	s8 magic[2];			// "CM"
	u32 cm_size;
	u16 width;
	u16 height;
	u16 colors;
	u8 bpp;
	u8 is_compressed;
	u8 type;				// 1
	u8 pad;
	u32 data_offset;
	u32 data_length;
	u32 reserved1[2];
} cm_header_t;

typedef struct {
	s8 magic[2];		// "AM"
	u32 am_size;
	u16 width;
	u16 height;
	u16 mask_width;
	u16 mask_height;
	u16 unknown1;		// [e]
	u16 unknown2;		// [10]
	u16 colors;
	u8 bpp;
	u8 is_compressed;
	u8 am_type;			// 2
	u8 am_pad;
	u8 type;
	u8 pad;
	u32 data_offset;
	u32 data_length;
	u32 mask_data_offset;
	u32 mask_data_length;	
	u8 mask_is_compressed;
	u8 pad1;
	u32 reserved;
} am_header_t;

typedef struct {
	u32 file_size;
	u32 width;
	u32 height;
	u32 reserved;
} mask_header_t;
#pragma pack ()

typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 length;
} my_arc_entry_t;

typedef struct {
	TCHAR name[MAX_PATH];
	u32 offset;
	u32 length;
	u32 width;
	u32 height;
	u32 colors;
	u32 bpp;
	u32 is_compressed;
} am_entry_t;

static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static DWORD rle_uncompress(BYTE *uncompr, DWORD uncomprLen,
							BYTE *compr, DWORD comprLen,
							DWORD cdepth)
{
	DWORD act_uncomprLen = 0;
	DWORD curByte = 0;

	while (curByte < comprLen) {
		BYTE flag;
		DWORD copy_pixel;

		flag = compr[curByte++];
		if (flag < 0x80) {
			copy_pixel = flag;

			for (DWORD k = 0; k < copy_pixel; k++) {
				for (DWORD j = 0; j < cdepth; j++) {
					if (curByte >= comprLen)
						goto out;
					uncompr[act_uncomprLen++] = compr[curByte++];
				}
			}
		} else {
			copy_pixel = flag & 0x7f;
			BYTE pixel[4];

			for (DWORD j = 0; j < cdepth; j++) {
				if (curByte >= comprLen)
					goto out;
				pixel[j] = compr[curByte++];
			}

			for (DWORD k = 0; k < copy_pixel; k++) {
				for (DWORD j = 0; j < cdepth; j++)
					uncompr[act_uncomprLen++] = pixel[j];
			}
		}
	}
out:
	return act_uncomprLen;
}

/********************* arc *********************/

/* 封包匹配回调函数 */
static int MAI_arc_match(struct package *pkg)
{
	arc_header_t arc_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &arc_header, sizeof(arc_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(arc_header.magic, "MAI\xa", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (!arc_header.index_entries) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int MAI_arc_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{
	arc_header_t arc_header;
	arc_entry_t *index_buffer;
	DWORD index_buffer_length;	

	if (pkg->pio->readvec(pkg, &arc_header, sizeof(arc_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	index_buffer_length = arc_header.index_entries * sizeof(arc_entry_t);
	index_buffer = (arc_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	DWORD my_index_buffer_length = sizeof(my_arc_entry_t) * arc_header.index_entries;
	my_arc_entry_t *my_index_buffer = (my_arc_entry_t *)malloc(my_index_buffer_length);
	if (!my_index_buffer) {
		free(index_buffer);
		return -CUI_EMEM;
	}

	if (arc_header.dir_entries && (arc_header.dir_level == 2)) {
		DWORD dir_index_buffer_len = sizeof(arc_dir_t) * arc_header.dir_entries;
		arc_dir_t *dir_index_buffer = (arc_dir_t *)malloc(dir_index_buffer_len);
		if (!dir_index_buffer) {
			free(my_index_buffer);
			free(index_buffer);
			return -CUI_EMEM;			
		}

		if (pkg->pio->read(pkg, dir_index_buffer, dir_index_buffer_len)) {
			free(dir_index_buffer);	
			free(my_index_buffer);
			free(index_buffer);
			return -CUI_EREAD;
		}

		for (DWORD i = 0; i < arc_header.index_entries; i++) {
			for (DWORD d = 0; d < arc_header.dir_entries; d++) {
				if (i < dir_index_buffer[d].start_index)
					break;
			}
			d--;

			strncpy(my_index_buffer[i].name, dir_index_buffer[d].name, 4);
			strcat(my_index_buffer[i].name, "\\");
			if (!index_buffer[i].name[15])
				strcat(my_index_buffer[i].name, index_buffer[i].name);
			else 
				strncat(my_index_buffer[i].name, index_buffer[i].name, 16);
			my_index_buffer[i].offset = index_buffer[i].offset;
			my_index_buffer[i].length = index_buffer[i].length;
		}
		free(dir_index_buffer);
	} else {
		for (DWORD i = 0; i < arc_header.index_entries; i++) {
			if (!index_buffer[i].name[15])
				strcpy(my_index_buffer[i].name, index_buffer[i].name);
			else {
				strncpy(my_index_buffer[i].name, index_buffer[i].name, 16);
				my_index_buffer[i].name[16] = 0;
			}
			my_index_buffer[i].offset = index_buffer[i].offset;
			my_index_buffer[i].length = index_buffer[i].length;
		}
	}
	free(index_buffer);

	pkg_dir->index_entries = arc_header.index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = my_index_buffer_length;
	pkg_dir->index_entry_length = sizeof(my_arc_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int MAI_arc_parse_resource_info(struct package *pkg,
									   struct package_resource *pkg_res)
{
	my_arc_entry_t *my_arc_entry;

	my_arc_entry = (my_arc_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_arc_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_arc_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = my_arc_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int MAI_arc_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{	
	BYTE *arc_data;

	arc_data = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!arc_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, arc_data, pkg_res->raw_data_length,
			pkg_res->offset, IO_SEEK_SET)) {
		free(arc_data);
		return -CUI_EREADVEC;
	}

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = arc_data;
		return 0;
	}

	if (!strncmp((char *)arc_data, "RIFF", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".wav");
	} else if (!strncmp((char *)arc_data, "OggS", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".ogg");
	} else if (!strncmp((char *)arc_data, "BM", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bm");
	} else if (!strncmp((char *)arc_data, "AM", 2)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".am");
	} else if (!strncmp((char *)arc_data, "CM", 2)) {
		cm_header_t *cm_header = (cm_header_t *)arc_data;
		BYTE *compr = (BYTE *)(cm_header + 1);
		DWORD uncomprlen;
		BYTE *uncompr;
		DWORD palette_size;
		BYTE *palette;

		if (cm_header->type != 1) {
			free(arc_data);
			return -CUI_EMATCH;
		}

		if (cm_header->colors) {
			palette_size = cm_header->colors * 4;
			palette = (BYTE *)malloc(palette_size);
			if (!palette) {
				free(arc_data);
				return -CUI_EMEM;
			}

			BYTE *raw_pallette = compr;
			for (DWORD p = 0; p < cm_header->colors; p++) {
				palette[p * 4 + 0] = raw_pallette[p * 3 + 0];
				palette[p * 4 + 1] = raw_pallette[p * 3 + 1];
				palette[p * 4 + 2] = raw_pallette[p * 3 + 2];
				palette[p * 4 + 3] = 0;
			}
			compr += cm_header->colors * 3;
		} else {
			palette_size = 0;
			palette = NULL;
		}

		if (cm_header->is_compressed == 1) {
			uncomprlen = cm_header->width * cm_header->height 
				* cm_header->bpp / 8;
			uncompr = (BYTE *)malloc(uncomprlen);
			if (!uncompr) {
				free(palette);
				free(arc_data);
				return -CUI_EMEM;
			}

			DWORD act_uncomprlen =rle_uncompress(uncompr, uncomprlen, 
				compr, cm_header->data_length, cm_header->bpp / 8);					
			if (act_uncomprlen != uncomprlen) {
				free(uncompr);
				free(palette);
				free(arc_data);
				return -CUI_EUNCOMPR;
			}
		} else {
			uncomprlen = cm_header->data_length;
			uncompr = (BYTE *)malloc(uncomprlen);
			if (!uncompr) {
				free(palette);
				free(arc_data);
				return -CUI_EMEM;
			}
			memcpy(uncompr, compr, uncomprlen);
		}

		if (MyBuildBMPFile(uncompr, uncomprlen, palette, palette_size,
				cm_header->width, cm_header->height, 
				cm_header->bpp, (BYTE **)&pkg_res->actual_data,
				&pkg_res->actual_data_length, my_malloc)) {
			free(uncompr);
			free(palette);
			free(arc_data);
			return -CUI_EMEM;
		}
		free(uncompr);
		free(palette);
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp");
	} else if (!_tcscmp(pkg->name, _T("mask.arc"))) {
		mask_header_t *mask_header = (mask_header_t *)arc_data;
		DWORD dib_size = mask_header->width * mask_header->height;
		DWORD palette_size = mask_header->file_size - dib_size - sizeof(mask_header_t);
		BYTE *palette = (BYTE *)(mask_header + 1);
		BYTE *dib = palette + palette_size;

		if (MyBuildBMPFile(dib, dib_size, palette, palette_size,
				mask_header->width, mask_header->height, 
				8, (BYTE **)&pkg_res->actual_data,
				&pkg_res->actual_data_length, my_malloc)) {
			free(palette);
			free(arc_data);
			return -CUI_EMEM;
		}
		free(palette);		
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp");
	} else if (!memcmp((char *)arc_data, "\xff\xd8\xff\xe0\x00\x10JFIF", 10)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".jpg");
	}

	pkg_res->raw_data = arc_data;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation MAI_arc_operation = {
	MAI_arc_match,				/* match */
	MAI_arc_extract_directory,	/* extract_directory */
	MAI_arc_parse_resource_info,/* parse_resource_info */
	MAI_arc_extract_resource,	/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* am *********************/

/* 封包匹配回调函数 */
static int MAI_am_match(struct package *pkg)
{
	am_header_t am_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &am_header, sizeof(am_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (am_header.unknown1 || am_header.unknown2) {
		wcprintf(pkg->name);
		printf(": unknown1 %x unknown2 %x\n",
			am_header.unknown1, am_header.unknown2);
		exit(0);
	}

	// am type为1的mask图，用默认的调色版，总是全黑
	if (strncmp(am_header.magic, "AM", 2)
			|| (am_header.am_type != 2 && am_header.am_type != 1)
			|| am_header.type != 1) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int MAI_am_extract_directory(struct package *pkg,
									struct package_directory *pkg_dir)
{
	am_header_t am_header;
	am_entry_t *index_buffer;
	DWORD index_buffer_length;	

	if (pkg->pio->readvec(pkg, &am_header, sizeof(am_header), 
			0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	index_buffer_length = 2 * sizeof(am_entry_t);
	index_buffer = (am_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	_stprintf(index_buffer[0].name, _T("%s.bmp"), pkg->name);
	index_buffer[0].length = am_header.data_length;
	index_buffer[0].offset = am_header.data_offset;
	index_buffer[0].width = am_header.width;
	index_buffer[0].height = am_header.height;
	index_buffer[0].colors = am_header.colors;
	index_buffer[0].bpp = am_header.bpp;
	index_buffer[0].is_compressed = am_header.is_compressed;

	_stprintf(index_buffer[1].name, _T("%s.mask.bmp"), pkg->name);
	index_buffer[1].length = am_header.mask_data_length;
	index_buffer[1].offset = am_header.mask_data_offset;
	index_buffer[1].width = am_header.mask_width;
	index_buffer[1].height = am_header.mask_height;
	index_buffer[1].colors = am_header.colors;
	index_buffer[1].bpp = 8;
	index_buffer[1].is_compressed = am_header.mask_is_compressed;

	pkg_dir->index_entries = 2;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(am_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int MAI_am_parse_resource_info(struct package *pkg,
									  struct package_resource *pkg_res)
{
	am_entry_t *am_entry;

	am_entry = (am_entry_t *)pkg_res->actual_index_entry;
	wcscpy((WCHAR *)pkg_res->name, am_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = am_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = am_entry->offset;
	pkg_res->flags = PKG_RES_FLAG_UNICODE;

	return 0;
}

/* 封包资源提取函数 */
static int MAI_am_extract_resource(struct package *pkg,
								   struct package_resource *pkg_res)
{
	am_entry_t *am_entry = (am_entry_t *)pkg_res->actual_index_entry;

	BYTE *compr = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, pkg_res->raw_data_length, 
			pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	DWORD palette_size;
	BYTE *palette;
	if (am_entry->colors) {
		palette_size = am_entry->colors * 4;
		palette = (BYTE *)malloc(palette_size);
		if (!palette) {
			free(compr);
			return -CUI_EMEM;
		}

		DWORD raw_palette_size = am_entry->colors * 3;
		BYTE *raw_palette = (BYTE *)malloc(raw_palette_size);
		if (!raw_palette) {
			free(palette);
			free(compr);
			return -CUI_EMEM;
		}

		if (pkg->pio->readvec(pkg, raw_palette, raw_palette_size, 
				sizeof(am_header_t), IO_SEEK_SET)) {
			free(raw_palette);
			free(palette);
			free(compr);
			return -CUI_EREADVEC;
		}

		for (DWORD p = 0; p < am_entry->colors; p++) {
			palette[p * 4 + 0] = raw_palette[p * 3 + 0];
			palette[p * 4 + 1] = raw_palette[p * 3 + 1];
			palette[p * 4 + 2] = raw_palette[p * 3 + 2];
			palette[p * 4 + 3] = 0;
		}
		free(raw_palette);
	} else {
		palette_size = 0;
		palette = NULL;
	}

	DWORD uncomprlen;
	BYTE *uncompr;	
	if (am_entry->is_compressed == 1) {
		uncomprlen = am_entry->width * am_entry->height 
			* am_entry->bpp / 8;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(palette);
			free(compr);			
			return -CUI_EMEM;
		}

		DWORD act_uncomprlen = rle_uncompress(uncompr, uncomprlen, 
			compr, pkg_res->raw_data_length, 
			am_entry->bpp / 8);					
		if (act_uncomprlen != uncomprlen) {
			free(uncompr);
			free(palette);
			free(compr);
			return -CUI_EUNCOMPR;
		}
	} else {
		uncomprlen = pkg_res->raw_data_length;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(palette);
			free(compr);
			return -CUI_EMEM;
		}
		memcpy(uncompr, compr, uncomprlen);
	}
	free(compr);

	if (pkg_res->index_number == 1)
		am_entry->height = 0 - am_entry->height;
	if (MyBuildBMPFile(uncompr, uncomprlen, palette, palette_size,
			am_entry->width, am_entry->height, 
			am_entry->bpp, (BYTE **)&pkg_res->actual_data,
			&pkg_res->actual_data_length, my_malloc)) {
		free(uncompr);
		free(palette);
		return -CUI_EMEM;
	}
	free(uncompr);
	free(palette);

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation MAI_am_operation = {
	MAI_am_match,				/* match */
	MAI_am_extract_directory,	/* extract_directory */
	MAI_am_parse_resource_info,	/* parse_resource_info */
	MAI_am_extract_resource,	/* extract_resource */
	CUI_COMMON_OPERATION
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK MAI_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".arc"), NULL, 
		NULL, &MAI_arc_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".am"), NULL, 
		NULL, &MAI_am_operation, CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
