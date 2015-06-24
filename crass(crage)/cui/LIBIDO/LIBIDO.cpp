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
struct acui_information LIBIDO_cui_information = {
	_T(""),		/* copyright */
	_T(""),			/* system */
	_T(".arc"),				/* package */
	_T("1.0.0"),			/* revision */
	_T("痴汉公贼"),			/* author */
	_T(""),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	u32 index_entries;
} arc_header_t;

typedef struct {
	s8 name[20];
	u32 uncomprlen;
	u32 comprlen;
	u32 offset;
} arc_entry_t;
#pragma pack ()

/********************* arc *********************/

/* 封包匹配回调函数 */
static int LIBIDO_arc_match(struct package *pkg)
{
	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包索引目录提取函数 */
static int LIBIDO_arc_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	arc_entry_t *index_buffer;
	unsigned int index_buffer_length;	
	unsigned int i;

	if (!pkg || !pkg_dir)
		return -CUI_EPARA;

	if (pkg->pio->read(pkg, &pkg_dir->index_entries, 4))
		return -CUI_EREAD;

	index_buffer_length = pkg_dir->index_entries * sizeof(arc_entry_t);
	index_buffer = (arc_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	arc_entry_t *index = index_buffer;
	for (i = 0; i < pkg_dir->index_entries; i++) {
		for (unsigned int k = 0; k < 20; k++)
			index->name[k] = ~index->name[k];
		index++;
	}

	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(arc_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int LIBIDO_arc_parse_resource_info(struct package *pkg,
										  struct package_resource *pkg_res)
{
	arc_entry_t *arc_entry;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	arc_entry = (arc_entry_t *)pkg_res->actual_index_entry;
	strncpy(pkg_res->name, arc_entry->name, sizeof(arc_entry->name));
	pkg_res->name_length = sizeof(arc_entry->name);
	pkg_res->raw_data_length = arc_entry->comprlen;
	pkg_res->actual_data_length = arc_entry->uncomprlen;
	pkg_res->offset = arc_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int LIBIDO_arc_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr = NULL;
	DWORD comprlen, uncomprlen = 0;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen,	pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	pkg_res->raw_data = compr;
	pkg_res->raw_data_length = comprlen;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

/* 资源保存函数 */
static int LIBIDO_arc_save_resource(struct resource *res, 
									struct package_resource *pkg_res)
{
	if (!res || !pkg_res)
		return -CUI_EPARA;
	
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
static void LIBIDO_arc_release_resource(struct package *pkg, 
										struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

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
static void LIBIDO_arc_release(struct package *pkg, 
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
static cui_ext_operation LIBIDO_arc_operation = {
	LIBIDO_arc_match,					/* match */
	LIBIDO_arc_extract_directory,		/* extract_directory */
	LIBIDO_arc_parse_resource_info,		/* parse_resource_info */
	LIBIDO_arc_extract_resource,		/* extract_resource */
	LIBIDO_arc_save_resource,			/* save_resource */
	LIBIDO_arc_release_resource,		/* release_resource */
	LIBIDO_arc_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK LIBIDO_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".arc"), NULL, 
		NULL, &LIBIDO_arc_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	return 0;
}
