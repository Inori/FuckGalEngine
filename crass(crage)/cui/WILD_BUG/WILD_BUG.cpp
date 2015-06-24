#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>
#include <vector>
#include <string>

using namespace std;
using std::vector;

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information WILD_BUG_cui_information = {
	NULL,					/* copyright */
	NULL,					/* system */
	_T(".WBP"),				/* package */
	_T("0.1.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T(""),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[16];		// "ARCFORM4 WBUG \x1a"
	u32 dir_size;
	u32 index_offset;
	u32 index_length;
	u32 data_offset;
	u32 data_length;
} WBP_header_t;

typedef struct {
	u8 hash;
	u8 name_length;
	u16 dir_id;
} WBP_dir_t;

typedef struct {
	u8 hash;
	u8 name_length;
	u16 dir_id;
	u32 offset;
	u32 length;
	u32 time_stamp[2];
	//FILETIME time_stamp;
} WBP_entry_t;
#pragma pack ()

typedef struct {
	char name[MAX_PATH];
	DWORD id;
	DWORD name_length;
} my_WBP_dir_t;

typedef struct {
	char name[MAX_PATH];
	DWORD offset;
	DWORD length;
} my_WBP_entry_t;

/********************* WBP *********************/

/* 封包匹配回调函数 */
static int WILD_BUG_WBP_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	WBP_header_t header;
	if (pkg->pio->read(pkg, &header, sizeof(header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(header.magic, "ARCFORM4 WBUG \x1a", sizeof(header.magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int WILD_BUG_WBP_extract_directory(struct package *pkg,
										  struct package_directory *pkg_dir)
{
	WBP_header_t header;
	if (pkg->pio->readvec(pkg, &header, sizeof(header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	u32 dir_name_table[256];
	if (pkg->pio->read(pkg, dir_name_table, sizeof(dir_name_table)))
		return -CUI_EREAD;

	u32 res_name_table[256];
	if (pkg->pio->read(pkg, res_name_table, sizeof(res_name_table)))
		return -CUI_EREAD;

	BYTE *index = new BYTE[header.index_length];
	if (!index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index, header.index_length)) {
		delete [] index;
		return -CUI_EREAD;
	}

	vector <my_WBP_dir_t> my_dir_index;
	for (DWORD i = 0; i < 256; ++i) {
		if (!dir_name_table[i])
			continue;

		dir_name_table[i] -= header.index_offset;
		WBP_dir_t *dir = (WBP_dir_t *)&index[dir_name_table[i]];

		while (dir->hash == i) {
			my_WBP_dir_t my_dir;

			BYTE hash = 0;
			for (DWORD n = 0; n < dir->name_length; ++n)
				hash += ((BYTE *)(dir + 1))[n];
			if (hash != dir->hash) {
				delete [] index;
				return -CUI_EMATCH;
			}

			strncpy(my_dir.name, (char *)(dir + 1), dir->name_length);
			my_dir.name[dir->name_length] = 0;
			my_dir.name_length = dir->name_length;
			my_dir.id = dir->dir_id;
			my_dir_index.push_back(my_dir);
			dir = (WBP_dir_t *)((u8 *)dir + sizeof(WBP_dir_t) + dir->name_length + 1);
		}
	}	
	if (i != 256) {
		delete [] index;
		return -CUI_EMEM;
	}

	vector <my_WBP_entry_t> my_entry_index;
	for (i = 0; i < 256; ++i) {
		if (!res_name_table[i])
			continue;

		res_name_table[i] -= header.index_offset;
		WBP_entry_t *entry = (WBP_entry_t *)&index[res_name_table[i]];

		while (entry->hash == i) {
			my_WBP_entry_t my_entry;

			BYTE hash = 0;
			for (DWORD n = 0; n < entry->name_length; ++n)
				hash += ((BYTE *)(entry + 1))[n];
			if (hash != entry->hash) {
				delete [] index;
				return -CUI_EMATCH;
			}

			for (DWORD l = 0; l < my_dir_index.size(); ++l) {
				if (entry->dir_id == my_dir_index[l].id)
					break;
			}
			if (l == my_dir_index.size())
				break;

			strncpy(my_entry.name, my_dir_index[l].name, my_dir_index[l].name_length);
			my_entry.name[my_dir_index[l].name_length] = 0;
			strncat(my_entry.name, (char *)(entry + 1), entry->name_length);
			my_entry.name[my_dir_index[l].name_length + entry->name_length] = 0;
			my_entry.offset = entry->offset;
			my_entry.length = entry->length;
			my_entry_index.push_back(my_entry);
			entry = (WBP_entry_t *)((u8 *)entry + sizeof(WBP_entry_t) + entry->name_length + 1);
		}
	}
	delete [] index;
	if (i != 256)
		return -CUI_EMATCH;

	my_WBP_entry_t *my_index = new my_WBP_entry_t[my_entry_index.size()];
	if (!my_index)
		return -CUI_EMEM;

	for (i = 0; i < my_entry_index.size(); ++i)
		my_index[i] = my_entry_index[i];

	pkg_dir->index_entries = my_entry_index.size();
	pkg_dir->directory = my_index;
	pkg_dir->directory_length = sizeof(my_WBP_entry_t) * my_entry_index.size();
	pkg_dir->index_entry_length = sizeof(my_WBP_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int WILD_BUG_WBP_parse_resource_info(struct package *pkg,
											struct package_resource *pkg_res)
{
	my_WBP_entry_t *my_WBP_entry;

	my_WBP_entry = (my_WBP_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_WBP_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_WBP_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_WBP_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int WILD_BUG_WBP_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	BYTE *raw;
	DWORD raw_len;

	raw_len = pkg_res->raw_data_length;
	raw = (BYTE *)malloc(raw_len);
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, raw_len, pkg_res->offset, IO_SEEK_SET)) {
		free(raw);
		return -CUI_EREADVEC;
	}

	pkg_res->raw_data = raw;

	return 0;
}

/* 资源保存函数 */
static int WILD_BUG_WBP_save_resource(struct resource *res, 
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
static void WILD_BUG_WBP_release_resource(struct package *pkg, 
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
static void WILD_BUG_WBP_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation WILD_BUG_WBP_operation = {
	WILD_BUG_WBP_match,					/* match */
	WILD_BUG_WBP_extract_directory,		/* extract_directory */
	WILD_BUG_WBP_parse_resource_info,	/* parse_resource_info */
	WILD_BUG_WBP_extract_resource,		/* extract_resource */
	WILD_BUG_WBP_save_resource,			/* save_resource */
	WILD_BUG_WBP_release_resource,		/* release_resource */
	WILD_BUG_WBP_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK WILD_BUG_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".WBP"), NULL, 
		NULL, &WILD_BUG_WBP_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
