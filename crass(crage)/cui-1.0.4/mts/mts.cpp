#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>
#include <zlib.h>

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information mts_cui_information = {
	_T(""),					/* copyright */
	_T("mts"),				/* system */
	_T(".pak .z"),			/* package */
	_T("1.0.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-12-13 9:10"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[48];		/* "DATA$TOP" */
	u32 reserved[2];
	u32 index_entries;		
	u32 zero;
} pak_header_t;

typedef struct {
	s8 name[48];
	u32 offset0;
	u32 offset1;
	u32 length;
	u32 zero;
} pak_entry_t;
#pragma pack ()

/********************* pak *********************/

/* 封包匹配回调函数 */
static int mts_pak_match(struct package *pkg)
{
	s8 magic[8];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "DATA$TOP", 8)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int mts_pak_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{
	pak_header_t pak_header;

	if (pkg->pio->readvec(pkg, &pak_header, sizeof(pak_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	pkg_dir->index_entries = pak_header.index_entries - 1;

	DWORD index_buffer_length = pkg_dir->index_entries * sizeof(pak_entry_t);
	pak_entry_t *index_buffer = new pak_entry_t[pkg_dir->index_entries];
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		delete [] index_buffer;
		return -CUI_EREAD;
	}

	for (DWORD i = 0; i < pkg_dir->index_entries; ++i)
		index_buffer[i].offset1 += index_buffer_length + sizeof(pak_header_t);

	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(pak_entry_t);
//	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int mts_pak_parse_resource_info(struct package *pkg,
									   struct package_resource *pkg_res)
{
	pak_entry_t *pak_entry;

	pak_entry = (pak_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, pak_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = pak_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = pak_entry->offset1;

	return 0;
}

/* 封包资源提取函数 */
static int mts_pak_extract_resource(struct package *pkg,
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
static int mts_pak_save_resource(struct resource *res, 
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
static void mts_pak_release_resource(struct package *pkg, 
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
static void mts_pak_release(struct package *pkg, 
							struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation mts_pak_operation = {
	mts_pak_match,					/* match */
	mts_pak_extract_directory,		/* extract_directory */
	mts_pak_parse_resource_info,	/* parse_resource_info */
	mts_pak_extract_resource,		/* extract_resource */
	mts_pak_save_resource,			/* save_resource */
	mts_pak_release_resource,		/* release_resource */
	mts_pak_release					/* release */
};

/********************* z *********************/

/* 封包匹配回调函数 */
static int mts_z_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

static int mts_z_extract_resource(struct package *pkg,
								  struct package_resource *pkg_res)
{
	u32 comprlen;
	pkg->pio->length_of(pkg, &comprlen);

	BYTE *compr = new BYTE[comprlen];
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, 0, IO_SEEK_SET)) {
		delete [] compr;
		return -CUI_EREADVEC;
	}

	DWORD uncomprlen = comprlen;
	BYTE *uncompr;
	while (1) {
		uncomprlen *= 2;
		uncompr = new BYTE[uncomprlen];
		if (!uncompr) {
			printf("%x\n", uncomprlen);
			delete [] compr;
			return -CUI_EMEM;
		}

		if (uncompress(uncompr, &uncomprlen, compr, comprlen) == Z_OK) {
			delete [] compr;
			break;
		}
		delete [] uncompr;
	}
	memcpy(uncompr, "DATA$TOP", 8);

	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation mts_z_operation = {
	mts_z_match,				/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	mts_z_extract_resource,		/* extract_resource */
	mts_pak_save_resource,		/* save_resource */
	mts_pak_release_resource,	/* release_resource */
	mts_pak_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK mts_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".z"), _T(".pak"), 
		NULL, &mts_z_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".pak"), NULL, 
		NULL, &mts_pak_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR
		| CUI_EXT_FLAG_RES))
			return -1;

	return 0;
}
