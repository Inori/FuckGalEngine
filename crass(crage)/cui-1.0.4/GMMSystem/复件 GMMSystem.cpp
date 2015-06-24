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
struct acui_information GMMSystem_cui_information = {
	_T(""),		/* copyright */
	_T(""),			/* system */
	_T(".dat"),				/* package */
	_T("1.0.0"),			/* revision */
	_T("痴汉公贼"),			/* author */
	_T("2007-12-28 0:35"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];
	u32 index_entries;
} dat_header_t;

typedef struct {
	s8 name[64];
	u32 offset_lo;
	u32 offset_hi;
	u32 length_lo;
	u32 length_hi;
} dat_entry_t;
#pragma pack ()


static void decode(void *data, DWORD data_length)
{
	BYTE *dat = (BYTE *)data;
	BYTE dec_tbl[10] = { 0xff, 0xff, 0xff, 0x73, 0x55, 0xa5,
		0xaa, 0x55, 0xaa, 0xaa };

	if (*dat & 0x80) {
		DWORD i;

		for (i = 0; i < data_length; i++)
			dat[i] ^= dec_tbl[((i / 5) % 5) + (i % 6)];

		for (i = 3; i < data_length; i += 3)
			dat[i] = -dat[i];
	}
}

/********************* dat *********************/

/* 封包匹配回调函数 */
static int GMMSystem_dat_match(struct package *pkg)
{
	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包索引目录提取函数 */
static int GMMSystem_dat_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	dat_entry_t *index_buffer;
	u32 index_entries;
	DWORD index_buffer_length;	

	if (!pkg || !pkg_dir)
		return -CUI_EPARA;

	if (pkg->pio->readvec(pkg, &index_entries, 4, -4, IO_SEEK_END))
		return -CUI_EREADVEC;

	index_buffer_length = index_entries * sizeof(dat_entry_t);
	index_buffer = (dat_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, index_buffer, index_buffer_length, 
			0 - (index_buffer_length + 4), IO_SEEK_END)) {
		free(index_buffer);
		return -CUI_EREADVEC;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(dat_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int GMMSystem_dat_parse_resource_info(struct package *pkg,
											 struct package_resource *pkg_res)
{
	dat_entry_t *dat_entry;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	dat_entry = (dat_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, dat_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = dat_entry->length_lo;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = dat_entry->offset_lo;

	return 0;
}

/* 封包资源提取函数 */
static int GMMSystem_dat_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	pkg_res->raw_data = malloc(pkg_res->raw_data_length);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EREADVEC;
	}

	decode(pkg_res->raw_data, pkg_res->raw_data_length);

	return 0;
}

/* 资源保存函数 */
static int GMMSystem_dat_save_resource(struct resource *res, 
									   struct package_resource *pkg_res)
{
	if (!res || !pkg_res)
		return -CUI_EPARA;
	
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
static void GMMSystem_dat_release_resource(struct package *pkg, 
										   struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

/* 封包卸载函数 */
static void GMMSystem_dat_release(struct package *pkg, 
								  struct package_directory *pkg_dir)
{
	if (!pkg || !pkg_dir)
		return;

	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation GMMSystem_dat_operation = {
	GMMSystem_dat_match,				/* match */
	GMMSystem_dat_extract_directory,	/* extract_directory */
	GMMSystem_dat_parse_resource_info,	/* parse_resource_info */
	GMMSystem_dat_extract_resource,		/* extract_resource */
	GMMSystem_dat_save_resource,		/* save_resource */
	GMMSystem_dat_release_resource,		/* release_resource */
	GMMSystem_dat_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK GMMSystem_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &GMMSystem_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR| CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	return 0;
}
