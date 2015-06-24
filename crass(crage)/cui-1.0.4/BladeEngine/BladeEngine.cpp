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
struct acui_information BladeEngine_cui_information = {
	_T("武礼堂"),			/* copyright */
	_T("BLADE VISUAL NOVEL ENGINE"),	/* system */
	_T(".pkd"),				/* package */
	_T("1.0.0"),			/* revision */
	_T("痴汉公贼"),			/* author */
	_T("2007-12-8 11:04"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];			/* "PACK" */
	u32 index_entries;
} pkd_header_t;

typedef struct {
	s8 name[128];
	u32 length;
	u32 offset;				/* from file beginning */
} pkd_entry_t;
#pragma pack ()

/********************* pkd *********************/

/* 封包匹配回调函数 */
static int BladeEngine_pkd_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "PACK", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int BladeEngine_pkd_extract_directory(struct package *pkg,
											 struct package_directory *pkg_dir)
{
	BYTE *index_buffer;
	DWORD index_buffer_length;	

	if (pkg->pio->read(pkg, &pkg_dir->index_entries, 4))
		return -CUI_EREAD;

	index_buffer_length = pkg_dir->index_entries * sizeof(pkd_entry_t);
	index_buffer = (BYTE *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	for (DWORD i = 0; i < index_buffer_length; i++)
		index_buffer[i] ^= 0xc5;

	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(pkd_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int BladeEngine_pkd_parse_resource_info(struct package *pkg,
											   struct package_resource *pkg_res)
{
	pkd_entry_t *pkd_entry;

	pkd_entry = (pkd_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, pkd_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = pkd_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = pkd_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int BladeEngine_pkd_extract_resource(struct package *pkg,
											struct package_resource *pkg_res)
{
	BYTE *data;

	data = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, data, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET)) {
		free(data);
		return -CUI_EREADVEC;
	}

	for (DWORD i = 0; i < pkg_res->raw_data_length; i++)
		data[i] ^= 0xc5;

	pkg_res->raw_data = data;

	return 0;
}

/* 资源保存函数 */
static int BladeEngine_pkd_save_resource(struct resource *res, 
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
static void BladeEngine_pkd_release_resource(struct package *pkg, 
											 struct package_resource *pkg_res)
{
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

/* 封包卸载函数 */
static void BladeEngine_pkd_release(struct package *pkg, 
									struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation BladeEngine_pkd_operation = {
	BladeEngine_pkd_match,					/* match */
	BladeEngine_pkd_extract_directory,		/* extract_directory */
	BladeEngine_pkd_parse_resource_info,	/* parse_resource_info */
	BladeEngine_pkd_extract_resource,		/* extract_resource */
	BladeEngine_pkd_save_resource,			/* save_resource */
	BladeEngine_pkd_release_resource,		/* release_resource */
	BladeEngine_pkd_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK BladeEngine_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".pkd"), NULL, 
		NULL, &BladeEngine_pkd_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
