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
struct acui_information losterika_cui_information = {
	_T("constructor"),		/* copyright */
	_T(""),					/* system */
	_T(".dat .pak"),		/* package */
	_T("1.0.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-9-2 21:21"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	u32 index_entries;
	u32 reserved[3];	// 0
} dat_header_t;

typedef struct {
	u32 next_offset;
	s8 name[28];
} dat_entry_t;
#pragma pack ()

typedef struct {
	char name[MAX_PATH];
	DWORD length;
	DWORD offset;
} my_dat_entry_t;

/********************* dat *********************/

/* 封包匹配回调函数 */
static int losterika_dat_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包索引目录提取函数 */
static int losterika_dat_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{
	dat_header_t dat_header;
	if (pkg->pio->read(pkg, &dat_header, sizeof(dat_header)))
		return -CUI_EREAD;

	DWORD index_buffer_length = dat_header.index_entries * sizeof(dat_entry_t);
	dat_entry_t *index_buffer = new dat_entry_t[dat_header.index_entries];
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		delete [] index_buffer;
		return -CUI_EREAD;
	}

	DWORD offset = sizeof(dat_header) + index_buffer_length;
	for (DWORD i = 0; i < dat_header.index_entries; ++i)
		index_buffer[i].next_offset += offset;		

	pkg_dir->index_entries = dat_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(dat_entry_t);
	//pkg_dir->flags = PKG_DIR_FLAG_SKIP0;
	package_set_private(pkg, offset);

	return 0;
}

/* 封包索引项解析函数 */
static int losterika_dat_parse_resource_info(struct package *pkg,
											 struct package_resource *pkg_res)
{
	dat_entry_t *dat_entry;

	DWORD offset = package_get_private(pkg);
	dat_entry = (dat_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, dat_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = offset;
	pkg_res->raw_data_length = dat_entry->next_offset - offset;
	package_set_private(pkg, dat_entry->next_offset);

	return 0;
}

/* 封包资源提取函数 */
static int losterika_dat_extract_resource(struct package *pkg,
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
static int losterika_dat_save_resource(struct resource *res, 
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
static void losterika_dat_release_resource(struct package *pkg, 
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

/* 封包卸载函数 */
static void losterika_dat_release(struct package *pkg, 
								  struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation losterika_dat_operation = {
	losterika_dat_match,				/* match */
	losterika_dat_extract_directory,	/* extract_directory */
	losterika_dat_parse_resource_info,	/* parse_resource_info */
	losterika_dat_extract_resource,		/* extract_resource */
	losterika_dat_save_resource,		/* save_resource */
	losterika_dat_release_resource,		/* release_resource */
	losterika_dat_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK losterika_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &losterika_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR
		| CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".pak"), NULL, 
		NULL, &losterika_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR
		| CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	return 0;
}
