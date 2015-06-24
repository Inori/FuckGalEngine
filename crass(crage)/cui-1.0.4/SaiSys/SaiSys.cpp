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
struct acui_information SaiSys_cui_information = {
	_T(""),					/* copyright */
	_T(""),					/* system */
	_T(".ssb"),				/* package */
	_T("1.0.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-12-14 21:21"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];
	u32 index_entries;
} dat_header_t;
#pragma pack ()

/* .dat封包的索引项结构 */
typedef struct {
	s8 name[256];
	u32 name_length;
	u32 offset;
	u32 length;
} dat_entry_t;

/********************* png *********************/

/* 封包匹配回调函数 */
static int SaiSys_png_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "\x89PNG", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int SaiSys_png_extract_resource(struct package *pkg,
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
static int SaiSys_save_resource(struct resource *res, 
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
static void SaiSys_release_resource(struct package *pkg, 
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
static void SaiSys_release(struct package *pkg, 
						   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation SaiSys_png_operation = {
	SaiSys_png_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	SaiSys_png_extract_resource,/* extract_resource */
	SaiSys_save_resource,		/* save_resource */
	SaiSys_release_resource,	/* release_resource */
	SaiSys_release				/* release */
};

/********************* SSB *********************/

/* 封包匹配回调函数 */
static int SaiSys_ssb_match(struct package *pkg)
{
	if (lstrcmpi(pkg->name, _T("Data.ssb")))
		return -CUI_EMATCH;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包资源提取函数 */
static int SaiSys_ssb_extract_resource(struct package *pkg,
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

	for (DWORD i = 0; i < pkg_res->raw_data_length; ++i)
		raw[i] ^= 0xaa;

	pkg_res->raw_data = raw;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation SaiSys_ssb_operation = {
	SaiSys_ssb_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	SaiSys_ssb_extract_resource,/* extract_resource */
	SaiSys_save_resource,		/* save_resource */
	SaiSys_release_resource,	/* release_resource */
	SaiSys_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK SaiSys_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, NULL, _T(".png"), 
		NULL, &SaiSys_png_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_NOEXT))
			return -1;

	if (callback->add_extension(callback->cui, _T(".ssb"), NULL, 
		NULL, &SaiSys_ssb_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	return 0;
}
