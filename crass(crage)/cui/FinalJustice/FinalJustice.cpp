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
struct acui_information FinalJustice_cui_information = {
	_T("Kyoya Yuro"),		/* copyright */
	_T("ラムダ"),			/* system */
	_T(".dat .dmf"),		/* package */
	_T("1.0.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-3-24 8:52"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[8];	// "LAG"
} lag_header_t;

typedef struct {
	s8 name[16];
	u32 width;
	u32 height;
	u32 type;		// 1,5 - 4字节; 0,4 - 2字节; 2,6 - 8字节, 3,7 - 16字节
	u32 length;
} lag_entry_t;
#pragma pack ()


static void *my_malloc(DWORD len)
{
	return new BYTE[len];
}

/********************* lag *********************/

/* 封包匹配回调函数 */
static int FinalJustice_lag_match(struct package *pkg)
{
	s8 magic[8];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "LAG", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int FinalJustice_lag_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	u32 lag_size;
	pkg->pio->length_of(pkg, &lag_size);
	DWORD offset = sizeof(lag_header_t);
	for (DWORD i = 0; offset < lag_size; ++i) {
		lag_entry_t entry;

		if (pkg->pio->read(pkg, &entry, sizeof(entry)))
			return -CUI_EREAD;

		if (pkg->pio->seek(pkg, entry.length, IO_SEEK_CUR))
			return -CUI_ESEEK;

		offset += sizeof(entry) + entry.length;
	}
	pkg_dir->index_entries = i;

	if (pkg->pio->seek(pkg, sizeof(lag_header_t), IO_SEEK_SET))
		return -CUI_ESEEK;

	lag_entry_t *index = new lag_entry_t[i];
	if (!index)
		return -CUI_EMEM;

	offset = sizeof(lag_header_t);
	for (i = 0; i < pkg_dir->index_entries; ++i) {
		lag_entry_t *entry = index + i;

		pkg->pio->read(pkg, entry, sizeof(lag_entry_t));
		pkg->pio->seek(pkg, entry->length, IO_SEEK_CUR);
		entry->type = offset;
		offset += sizeof(lag_entry_t) + entry->length;
	}

	pkg_dir->directory = index;
	pkg_dir->directory_length = sizeof(lag_entry_t) * pkg_dir->index_entries;
	pkg_dir->index_entry_length = sizeof(lag_entry_t);
	//pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int FinalJustice_lag_parse_resource_info(struct package *pkg,
									struct package_resource *pkg_res)
{
	lag_entry_t *lag_entry;

	lag_entry = (lag_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, lag_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = lag_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = lag_entry->type;

	return 0;
}

/* 封包资源提取函数 */
static int FinalJustice_lag_extract_resource(struct package *pkg,
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

	lag_entry_t *lag = (lag_entry_t *)raw;
	BYTE *dib = raw + sizeof(lag_entry_t);
	if (MyBuildBMPFile(dib, lag->length, NULL, 0,
			lag->width, 0 - lag->height, 32, (BYTE **)&pkg_res->actual_data,
			&pkg_res->actual_data_length, my_malloc)) {
		delete [] raw;
		return -CUI_EMEM;
	}
	delete [] raw;

	return 0;
}

/* 资源保存函数 */
static int FinalJustice_lag_save_resource(struct resource *res, 
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
static void FinalJustice_lag_release_resource(struct package *pkg, 
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
static void FinalJustice_lag_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation FinalJustice_lag_operation = {
	FinalJustice_lag_match,					/* match */
	FinalJustice_lag_extract_directory,		/* extract_directory */
	FinalJustice_lag_parse_resource_info,		/* parse_resource_info */
	FinalJustice_lag_extract_resource,		/* extract_resource */
	FinalJustice_lag_save_resource,			/* save_resource */
	FinalJustice_lag_release_resource,		/* release_resource */
	FinalJustice_lag_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK FinalJustice_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".lag"), _T(".bmp"), 
		NULL, &FinalJustice_lag_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
