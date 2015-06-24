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
struct acui_information GrowLanser_cui_information = {
	NULL,					/* copyright */
	NULL,					/* system */
	_T(".PDT .SDT .BIN"),		/* package */
	_T("0.0.3"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T(""),					/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	u32 index_entries;
} NEFF_BIN_header_t;

typedef struct {
	u32 offset;
	u32 length;
} NEFF_BIN_entry_t;
#pragma pack ()


/********************* SDT *********************/

/* 封包匹配回调函数 */
static int GrowLanser_SDT_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "RIFF", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int GrowLanser_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	u32 size;
	if (pkg->pio->length_of(pkg, &size))
		return -CUI_ELEN;

	BYTE *data = new BYTE[size];
	if (!data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, data, size, 0, IO_SEEK_SET)) {
		delete [] data;
		return -CUI_EREADVEC;
	}

	pkg_res->raw_data = data;

	return 0;
}

/* 资源保存函数 */
static int GrowLanser_save_resource(struct resource *res, 
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
static void GrowLanser_release_resource(struct package *pkg, 
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
static void GrowLanser_release(struct package *pkg, 
						struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation GrowLanser_SDT_operation = {
	GrowLanser_SDT_match,				/* match */
	NULL,								/* extract_directory */
	NULL,								/* parse_resource_info */
	GrowLanser_extract_resource,		/* extract_resource */
	GrowLanser_save_resource,			/* save_resource */
	GrowLanser_release_resource,		/* release_resource */
	GrowLanser_release					/* release */
};

/********************* PDT *********************/

/* 封包匹配回调函数 */
static int GrowLanser_PDT_match(struct package *pkg)
{
	s8 magic[2];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "BM", 2)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包处理回调函数集合 */
static cui_ext_operation GrowLanser_PDT_operation = {
	GrowLanser_PDT_match,				/* match */
	NULL,								/* extract_directory */
	NULL,								/* parse_resource_info */
	GrowLanser_extract_resource,		/* extract_resource */
	GrowLanser_save_resource,			/* save_resource */
	GrowLanser_release_resource,		/* release_resource */
	GrowLanser_release					/* release */
};

/********************* NEFF.BIN *********************/

/* 封包匹配回调函数 */
static int GrowLanser_NEFF_match(struct package *pkg)
{
	if (lstrcmpi(pkg->name, _T("NEFF.BIN")))
		return -CUI_EMATCH;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包索引目录提取函数 */
static int GrowLanser_NEFF_extract_directory(struct package *pkg,
											 struct package_directory *pkg_dir)
{
	if (pkg->pio->read(pkg, &pkg_dir->index_entries, 4))
		return -CUI_EREAD;

	DWORD index_buffer_length = pkg_dir->index_entries * sizeof(NEFF_BIN_entry_t);
	NEFF_BIN_entry_t *index_buffer = new NEFF_BIN_entry_t[pkg_dir->index_entries];
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		delete [] index_buffer;
		return -CUI_EREAD;
	}

	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(NEFF_BIN_entry_t);
	//pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int GrowLanser_NEFF_parse_resource_info(struct package *pkg,	
											   struct package_resource *pkg_res)
{
	NEFF_BIN_entry_t *NEFF_BIN_entry;

	NEFF_BIN_entry = (NEFF_BIN_entry_t *)pkg_res->actual_index_entry;
	sprintf(pkg_res->name, "%04d", pkg_res->index_number);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = NEFF_BIN_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = NEFF_BIN_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int GrowLanser_NEFF_extract_resource(struct package *pkg,
											struct package_resource *pkg_res)
{
	BYTE *data = new BYTE[pkg_res->raw_data_length];
	if (!data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, data, pkg_res->raw_data_length, 
			pkg_res->offset, IO_SEEK_SET)) {
		delete [] data;
		return -CUI_EREADVEC;
	}

	pkg_res->raw_data = data;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation GrowLanser_NEFF_operation = {
	GrowLanser_NEFF_match,				/* match */
	GrowLanser_NEFF_extract_directory,	/* extract_directory */
	GrowLanser_NEFF_parse_resource_info,/* parse_resource_info */
	GrowLanser_NEFF_extract_resource,	/* extract_resource */
	GrowLanser_save_resource,			/* save_resource */
	GrowLanser_release_resource,		/* release_resource */
	GrowLanser_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK GrowLanser_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".PDT"), _T(".bmp"), 
		 NULL, &GrowLanser_PDT_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".SDT"), _T(".wav"), 
		 NULL, &GrowLanser_SDT_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".BIN"), _T(".tim"), 
		 NULL, &GrowLanser_NEFF_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR 
		 | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	return 0;
}
