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
#include <vector>

using namespace std;
using std::vector;

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information SoW_cui_information = {
	_T(""),		/* copyright */
	_T(""),			/* system */
	_T(".aod"),	/* package */
	_T("0.9.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-7-3 22:05"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 name[40];
	u32 offset;
	u32 size;
	u8 unknown[16];		// 时间戳？
} aod_entry_t;
#pragma pack ()


/********************* aod *********************/

/* 封包匹配回调函数 */
static int SoW_aod_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包索引目录提取函数 */
static int SoW_aod_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{
	u32 fsize;
	pkg->pio->length_of(pkg, &fsize);

	vector<aod_entry_t> index_vec;
	DWORD offset = 0;

	while (offset < fsize) {
		aod_entry_t entry;

		if (pkg->pio->readvec(pkg, &entry, sizeof(entry), offset, IO_SEEK_SET))
			return -CUI_EREADVEC;

		if (!entry.offset)
			break;
				
		offset += sizeof(entry);
		entry.offset += offset;
		if (entry.size)
			index_vec.push_back(entry);
		else
			offset = entry.offset;
	}

	DWORD index_buffer_length = index_vec.size() * sizeof(aod_entry_t);
	aod_entry_t *index_buffer = new aod_entry_t[index_vec.size()];
	if (!index_buffer)
		return -CUI_EMEM;

	for (DWORD i = 0; i < index_vec.size(); ++i)
		index_buffer[i] = index_vec[i];

	pkg_dir->index_entries = index_vec.size();
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(aod_entry_t);
	//pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int SoW_aod_parse_resource_info(struct package *pkg,
									   struct package_resource *pkg_res)
{
	aod_entry_t *aod_entry;

	aod_entry = (aod_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, aod_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = aod_entry->size;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = aod_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int SoW_aod_extract_resource(struct package *pkg,
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
static int SoW_aod_save_resource(struct resource *res, 
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
static void SoW_aod_release_resource(struct package *pkg, 
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
static void SoW_aod_release(struct package *pkg, 
							struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation SoW_aod_operation = {
	SoW_aod_match,					/* match */
	SoW_aod_extract_directory,		/* extract_directory */
	SoW_aod_parse_resource_info,	/* parse_resource_info */
	SoW_aod_extract_resource,		/* extract_resource */
	SoW_aod_save_resource,			/* save_resource */
	SoW_aod_release_resource,		/* release_resource */
	SoW_aod_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK SoW_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".aod"), NULL, 
		NULL, &SoW_aod_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR
		| CUI_EXT_FLAG_WEAK_MAGIC))

	return 0;
}
