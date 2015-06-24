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
struct acui_information ym1n2_cui_information = {
	_T("M no Violet"),	/* copyright */
	_T(""),				/* system */
	_T(".bin .exe"),	/* package */
	_T("1.0.0"),		/* revision */
	_T("痴漢公賊"),		/* author */
	_T("2008-11-6 21:00"),	/* date */
	NULL,				/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {	
	u16 name[256];
	u32 offset;
	u32 length;
} bin_entry_t;
#pragma pack ()

/********************* bin *********************/

/* 封包匹配回调函数 */
static int ym1n2_bin_match(struct package *pkg)
{
	s8 magic[2];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->open(pkg->lst, IO_READONLY)) {
		pkg->pio->close(pkg);
		return -CUI_EOPEN;
	}

	if (pkg->pio->read(pkg->lst, magic, sizeof(magic))) {
		pkg->pio->close(pkg->lst);
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "MZ", 2)) {
		pkg->pio->close(pkg->lst);
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int ym1n2_bin_extract_directory(struct package *pkg,
									   struct package_directory *pkg_dir)
{
	if (pkg->pio->seek(pkg->lst, 0x136998, IO_SEEK_SET))
		return -CUI_ESEEK;

	DWORD index_entries = 0;
	while (1) {
		bin_entry_t entry;
		if (pkg->pio->read(pkg->lst, &entry, sizeof(entry)))
			return -CUI_EREAD;

		if (entry.offset == -1)
			break;

		++index_entries;
	}
	if (!index_entries)
		return -CUI_EMATCH;

	bin_entry_t *index = new bin_entry_t[index_entries];
	if (!index)
		return -CUI_EMEM;

	DWORD index_len = sizeof(bin_entry_t) * index_entries;
	if (pkg->pio->readvec(pkg->lst, index, index_len, 0x136998, IO_SEEK_SET)) {
		delete [] index;
		return -CUI_EREADVEC;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index;
	pkg_dir->directory_length = index_len;
	pkg_dir->index_entry_length = sizeof(bin_entry_t);
	//pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int ym1n2_bin_parse_resource_info(struct package *pkg,
										 struct package_resource *pkg_res)
{
	bin_entry_t *bin_entry;

	bin_entry = (bin_entry_t *)pkg_res->actual_index_entry;
	wcsncpy((WCHAR *)pkg_res->name, bin_entry->name, 256);
	pkg_res->name_length = 256;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = bin_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = bin_entry->offset;
	pkg_res->flags = PKG_RES_FLAG_UNICODE;

	return 0;
}

/* 封包资源提取函数 */
static int ym1n2_bin_extract_resource(struct package *pkg,
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
static int ym1n2_bin_save_resource(struct resource *res, 
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
static void ym1n2_bin_release_resource(struct package *pkg, 
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
static void ym1n2_bin_release(struct package *pkg, 
							  struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg->lst);
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation ym1n2_bin_operation = {
	ym1n2_bin_match,				/* match */
	ym1n2_bin_extract_directory,	/* extract_directory */
	ym1n2_bin_parse_resource_info,	/* parse_resource_info */
	ym1n2_bin_extract_resource,		/* extract_resource */
	ym1n2_bin_save_resource,		/* save_resource */
	ym1n2_bin_release_resource,		/* release_resource */
	ym1n2_bin_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK ym1n2_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".bin"), NULL, 
		NULL, &ym1n2_bin_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_LST))
			return -1;

	return 0;
}
