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
struct acui_information MBA_cui_information = {
	_T("飛翔システム"),		/* copyright */
	_T(""),					/* system */
	_T(".gdp"),				/* package */
	_T("1.0.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-3-18 20:35"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	u32 index_entries;
} gdp_header_t;

typedef struct {
	s8 name[260];
	u32 length;
	u32 offset;
} gdp_entry_t;
#pragma pack ()

/********************* gdp *********************/

static int MBA_gdp_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir);

/* 封包匹配回调函数 */
static int MBA_gdp_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	struct package_directory pkg_dir;
	int ret = MBA_gdp_extract_directory(pkg, &pkg_dir);
	if (!ret)
		delete [] pkg_dir.directory;

	return ret;	
}

/* 封包索引目录提取函数 */
static int MBA_gdp_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{
	if (pkg->pio->readvec(pkg, &pkg_dir->index_entries, 4, 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD index_buffer_length = pkg_dir->index_entries * sizeof(gdp_entry_t);
	gdp_entry_t *index_buffer = new gdp_entry_t[pkg_dir->index_entries];
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		delete [] index_buffer;
		return -CUI_EREAD;
	}

	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(gdp_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int MBA_gdp_parse_resource_info(struct package *pkg,
									   struct package_resource *pkg_res)
{
	gdp_entry_t *gdp_entry;

	gdp_entry = (gdp_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, gdp_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = gdp_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = gdp_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int MBA_gdp_extract_resource(struct package *pkg,
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
static int MBA_gdp_save_resource(struct resource *res, 
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
static void MBA_gdp_release_resource(struct package *pkg, 
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
static void MBA_gdp_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation MBA_gdp_operation = {
	MBA_gdp_match,					/* match */
	MBA_gdp_extract_directory,		/* extract_directory */
	MBA_gdp_parse_resource_info,		/* parse_resource_info */
	MBA_gdp_extract_resource,		/* extract_resource */
	MBA_gdp_save_resource,			/* save_resource */
	MBA_gdp_release_resource,		/* release_resource */
	MBA_gdp_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK MBA_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".gdp"), NULL, 
		NULL, &MBA_gdp_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR
		| CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	return 0;
}
