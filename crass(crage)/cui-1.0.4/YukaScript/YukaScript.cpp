#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information YukaScript_cui_information = {
	_T(""),					/* copyright */
	_T("YukaScript"),		/* system */
	_T(".ykc .dat"),		/* package */
	_T("1.0.2"),			/* revision */
	_T("痴汉公贼"),			/* author */
	_T("2008-10-11 10:21"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[8];			/* "YKC001" */
	u32 header_length;
	u32 unknown;		
	u32 index_offset;
	u32 index_length;	
} ykc_header_t;

typedef struct {
	u32 name_offset;
	u32 name_length;		/* include NULL */
	u32 offset;
	u32 length;
	u32 unknown;			
} ykc_entry_t;

typedef struct {
	s8 magic[8];			/* "YKG000" */
	u32 header_length;
	u32 zeroed[7];
	u32 offset0;
	u32 length0;	
	u32 offset1;
	u32 length1;
	u32 offset2;
	u32 length2;	
} ykg_header_t;

typedef struct {
	s8 magic[6];			/* "YKS001" */
	u16 is_encoded;			/* 1 - encode 0 - no-encode */
	u32 header_length;
	u32 unknown;
	u32 unknown2[4];
	u32 text_offset;
	u32 text_length;
	u32 unknown1;	
	u32 unknown0;
} yks_header_t;
#pragma pack ()

typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 length;	
} my_ykc_entry_t;

typedef struct {
	u32 offset;
	u32 length;	
} ykg_entry_t;

/********************* ykc *********************/

/* 封包匹配回调函数 */
static int YukaScript_ykc_match(struct package *pkg)
{
	s8 magic[8];

	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "YKC001", 8)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int YukaScript_ykc_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	ykc_header_t ykc_header;
	ykc_entry_t *index_buffer;
	my_ykc_entry_t *my_index_buffer;
	DWORD index_entries;
	DWORD my_index_buffer_length;	

	if (!pkg || !pkg_dir)
		return -CUI_EPARA;

	if (pkg->pio->readvec(pkg, &ykc_header, sizeof(ykc_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	index_buffer = (ykc_entry_t *)malloc(ykc_header.index_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, index_buffer, ykc_header.index_length, 
			ykc_header.index_offset, IO_SEEK_SET)) {
		free(index_buffer);
		return -CUI_EREADVEC;	
	}

	index_entries = ykc_header.index_length / sizeof(ykc_entry_t);
	my_index_buffer_length = sizeof(my_ykc_entry_t) * index_entries;
	my_index_buffer = (my_ykc_entry_t *)malloc(my_index_buffer_length);
	if (!my_index_buffer) {
		free(index_buffer);
		return -CUI_EMEM;
	}

	ykc_entry_t *entry = index_buffer;
	for (DWORD i = 0; i < index_entries; i++) {
		ykc_entry_t *entry = &index_buffer[i];
		my_ykc_entry_t *my_entry = &my_index_buffer[i];

		if (pkg->pio->readvec(pkg, my_entry->name, entry->name_length, entry->name_offset, IO_SEEK_SET))
			break;

		my_entry->length = entry->length;
		my_entry->offset = entry->offset;

	}
	free(index_buffer);
	if (i != index_entries) {
		free(index_buffer);
		return -CUI_EREADVEC;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = my_index_buffer_length;
	pkg_dir->index_entry_length = sizeof(my_ykc_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int YukaScript_ykc_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	my_ykc_entry_t *my_ykc_entry;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	my_ykc_entry = (my_ykc_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_ykc_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_ykc_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_ykc_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int YukaScript_ykc_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	BYTE *raw, *actual;
	DWORD raw_length, actual_length;

	raw_length = pkg_res->raw_data_length;
	raw = (BYTE *)malloc(raw_length);
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, raw_length,	pkg_res->offset, IO_SEEK_SET)) {
		free(raw);
		return -CUI_EREADVEC;
	}

	actual = NULL;
	actual_length = 0;

	if (!memcmp((char *)raw, "YKS001", 6)) {
		yks_header_t *yks;
		BYTE *text;	
				
		yks = (yks_header_t *)raw;
		text = raw + yks->text_offset;
		if (yks->is_encoded) {
			for (DWORD e = 0; e < yks->text_length; e++)
				text[e] ^= 0xaa;
		}
	// for FairlyLife WEB体験版01: ll02_mn_ev01_gr_w.ykg
	//} else if (!strncmp((char *)raw, "YKG000", 8)) {
	} else if (!strncmp((char *)raw, "YKG000", 8) && (
		*(u32 *)(&raw[0x2c]) && !*(u32 *)(&raw[0x34]) && !*(u32 *)(&raw[0x3c])
		|| !*(u32 *)(&raw[0x2c]) && *(u32 *)(&raw[0x34]) && !*(u32 *)(&raw[0x3c])
		|| !*(u32 *)(&raw[0x2c]) && !*(u32 *)(&raw[0x34]) && *(u32 *)(&raw[0x3c])
		)) {
		ykg_header_t *ykg = (ykg_header_t *)raw;
		BYTE *ykg_data = raw + ykg->header_length;

		if (!strncmp((char *)ykg_data, "\x89GNP", 4)) {
			strncpy((char *)ykg_data, "\x89PNG", 4);
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
			pkg_res->replace_extension = _T(".png");
		} else if (!strncmp((char *)ykg_data, "\x89PNG", 4)) {
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
			pkg_res->replace_extension = _T(".png");
		} else if (!strncmp((char *)ykg_data, "BM", 2)) {
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
			pkg_res->replace_extension = _T(".bmp");
		}

		actual_length = ykg->length0 | ykg->length1 | ykg->length2;
		actual = (BYTE *)malloc(actual_length);
		if (!actual) {
			free(raw);
			return -CUI_EMEM;
		}
		memcpy(actual, raw + ykg->header_length, actual_length);
	}

	pkg_res->raw_data = raw;
	pkg_res->actual_data = actual;
	pkg_res->actual_data_length = actual_length;

	return 0;
}

/* 资源保存函数 */
static int YukaScript_ykc_save_resource(struct resource *res, 
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
static void YukaScript_ykc_release_resource(struct package *pkg, 
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
static void YukaScript_ykc_release(struct package *pkg, 
							 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation YukaScript_ykc_operation = {
	YukaScript_ykc_match,					/* match */
	YukaScript_ykc_extract_directory,		/* extract_directory */
	YukaScript_ykc_parse_resource_info,	/* parse_resource_info */
	YukaScript_ykc_extract_resource,		/* extract_resource */
	YukaScript_ykc_save_resource,			/* save_resource */
	YukaScript_ykc_release_resource,		/* release_resource */
	YukaScript_ykc_release				/* release */
};

/********************* ykg *********************/

/* 封包匹配回调函数 */
static int YukaScript_ykg_match(struct package *pkg)
{
	s8 magic[8];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "YKG000", 8)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int YukaScript_ykg_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	ykg_header_t ykg_header;
	ykg_entry_t *index_buffer;
	DWORD index_buffer_length;	

	if (pkg->pio->readvec(pkg, &ykg_header, sizeof(ykg_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	index_buffer_length = 3 * sizeof(ykg_entry_t);
	index_buffer = (ykg_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	index_buffer[0].offset = ykg_header.offset0;
	index_buffer[0].length = ykg_header.length0;
	index_buffer[1].offset = ykg_header.offset1;
	index_buffer[1].length = ykg_header.length1;
	/* coordinate数据 */
	index_buffer[2].offset = ykg_header.offset2;
	index_buffer[2].length = ykg_header.length2;

	pkg_dir->index_entries = 3;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(ykg_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int YukaScript_ykg_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	ykg_entry_t *ykg_entry;

	ykg_entry = (ykg_entry_t *)pkg_res->actual_index_entry;
	if (pkg_res->index_number != 2)		
		sprintf(pkg_res->name, "%d", pkg_res->index_number);
	else
		strcpy(pkg_res->name, "coordinate");		
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = ykg_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = ykg_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int YukaScript_ykg_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	BYTE *raw, *actual;
	DWORD raw_length, actual_length;

	raw_length = pkg_res->raw_data_length;
	raw = (BYTE *)malloc(raw_length);
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, raw_length,	pkg_res->offset, IO_SEEK_SET)) {
		free(raw);
		return -CUI_EREADVEC;
	}

	if (!strncmp((char *)raw, "\x89GNP", 4)) {
		strncpy((char *)raw, "\x89PNG", 4);
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".png");
	} else if (!strncmp((char *)raw, "\x89PNG", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".png");
	} else if (!strncmp((char *)raw, "BM", 2)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp");
	}

	pkg_res->raw_data = raw;
	pkg_res->actual_data = actual = NULL;
	pkg_res->actual_data_length = actual_length = 0;

	return 0;
}

/* 资源保存函数 */
static int YukaScript_ykg_save_resource(struct resource *res, 
								  struct package_resource *pkg_res)
{
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (pkg_res->raw_data && pkg_res->raw_data_length) {
		if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
			res->rio->close(res);
			return -CUI_EWRITE;
		}
	}

	res->rio->close(res);
	
	return 0;
}

/* 封包资源释放函数 */
static void YukaScript_ykg_release_resource(struct package *pkg, 
									  struct package_resource *pkg_res)
{
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

/* 封包卸载函数 */
static void YukaScript_ykg_release(struct package *pkg, 
							 struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation YukaScript_ykg_operation = {
	YukaScript_ykg_match,					/* match */
	YukaScript_ykg_extract_directory,		/* extract_directory */
	YukaScript_ykg_parse_resource_info,	/* parse_resource_info */
	YukaScript_ykg_extract_resource,		/* extract_resource */
	YukaScript_ykg_save_resource,			/* save_resource */
	YukaScript_ykg_release_resource,		/* release_resource */
	YukaScript_ykg_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK YukaScript_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".ykc"), NULL, 
		NULL, &YukaScript_ykc_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &YukaScript_ykc_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".ykg"), NULL, 
		NULL, &YukaScript_ykg_operation, CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
