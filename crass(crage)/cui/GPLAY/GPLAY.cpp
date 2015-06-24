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
struct acui_information GPLAY_cui_information = {
	NULL,					/* copyright */
	NULL,					/* system */
	_T(".ysk"),				/* package */
	_T(""),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T(""),					/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[12];			// "AA2956255240"
	u32 index_entries;
} ysk_header_t;

typedef struct {
	s8 name[20];
	u32 length;
} ysk_entry_t;
#pragma pack ()

typedef struct {
	s8 name[20];
	u32 length;
	u32 offset;
} my_ysk_entry_t;

/********************* ysk *********************/

/* 封包匹配回调函数 */
static int GPLAY_ysk_match(struct package *pkg)
{
	s8 magic[2];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "AA", 2)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int GPLAY_ysk_extract_directory(struct package *pkg,
									   struct package_directory *pkg_dir)
{
	ysk_header_t ysk_header;

	if (pkg->pio->readvec(pkg, &ysk_header, sizeof(ysk_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD index_length = sizeof(ysk_entry_t) * ysk_header.index_entries;
	ysk_entry_t *index_buffer = new ysk_entry_t[ysk_header.index_entries];
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_length)) {
		delete [] index_buffer;
		return -CUI_EREAD;
	}

	DWORD my_index_length = sizeof(my_ysk_entry_t) * ysk_header.index_entries;
	my_ysk_entry_t *my_index_buffer = new my_ysk_entry_t[ysk_header.index_entries];
	if (!my_index_buffer) {
		delete [] index_buffer;
		return -CUI_EMEM;
	}
	memset(my_index_buffer, 0, my_index_length);

	DWORD offset = sizeof(ysk_header) + index_length;
	for (DWORD k = 0; k < ysk_header.index_entries; k++) {
		strncpy(my_index_buffer[k].name, index_buffer[k].name, 
			sizeof(index_buffer[k].name));
		my_index_buffer[k].offset = offset;
		my_index_buffer[k].length = index_buffer[k].length;
		offset += index_buffer[k].length;
	}
	delete [] index_buffer;

	pkg_dir->index_entries = ysk_header.index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = my_index_length;
	pkg_dir->index_entry_length = sizeof(my_ysk_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int GPLAY_ysk_parse_resource_info(struct package *pkg,
										 struct package_resource *pkg_res)
{
	my_ysk_entry_t *ysk_entry;

	ysk_entry = (my_ysk_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, ysk_entry->name);
	pkg_res->name_length = sizeof(ysk_entry->name);
	pkg_res->raw_data_length = ysk_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = ysk_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int GPLAY_ysk_extract_resource(struct package *pkg,
									  struct package_resource *pkg_res)
{
	BYTE *raw = new BYTE[pkg_res->raw_data_length];
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			delete [] raw;
			return -CUI_EREADVEC;
	}
	pkg_res->raw_data = raw;

	return 0;
}

/* 资源保存函数 */
static int GPLAY_ysk_save_resource(struct resource *res, 
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
static void GPLAY_ysk_release_resource(struct package *pkg, 
									   struct package_resource *pkg_res)
{
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

/* 封包卸载函数 */
static void GPLAY_ysk_release(struct package *pkg, 
							  struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation GPLAY_ysk_operation = {
	GPLAY_ysk_match,				/* match */
	GPLAY_ysk_extract_directory,	/* extract_directory */
	GPLAY_ysk_parse_resource_info,	/* parse_resource_info */
	GPLAY_ysk_extract_resource,		/* extract_resource */
	GPLAY_ysk_save_resource,		/* save_resource */
	GPLAY_ysk_release_resource,		/* release_resource */
	GPLAY_ysk_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK GPLAY_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".ysk"), NULL, 
		NULL, &GPLAY_ysk_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
