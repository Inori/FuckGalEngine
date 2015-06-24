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
struct acui_information DMC4_cui_information = {
	_T(""),		/* copyright */
	_T(""),			/* system */
	_T(".arc"),				/* package */
	_T(""),			/* revision */
	_T(""),			/* author */
	_T(""),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];	// "ARC" or "ARCS"
	u16 version;	// 0x11
	u16 index_entries;
} arc_header_t;

typedef struct {
	s8 name[64];
	u32 hash;
	u32 comprlen;
	u32 uncomprlen;
	u32 offset;
} arc_entry_t;
#pragma pack ()


/********************* arc *********************/

/* 封包匹配回调函数 */
static int DMC4_arc_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	arc_header_t arc_header;
	if (pkg->pio->read(pkg, &arc_header, sizeof(arc_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(arc_header.magic, "ARC", 4) && strncmp(arc_header.magic, "ARCS", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (arc_header.version != 0x11) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int DMC4_arc_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	arc_header_t arc_header;
	if (pkg->pio->readvec(pkg, &arc_header, sizeof(arc_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD index_buffer_length = arc_header.index_entries * sizeof(arc_entry_t);
	arc_entry_t *index_buffer = new arc_entry_t[arc_header.index_entries];
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		delete [] index_buffer;
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = arc_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(arc_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int DMC4_arc_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	arc_entry_t *arc_entry;

	arc_entry = (arc_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, arc_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = arc_entry->comprlen;
	pkg_res->actual_data_length = arc_entry->uncomprlen;
	pkg_res->offset = arc_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int DMC4_arc_extract_resource(struct package *pkg,
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

	DWORD flag = pkg_res->actual_data_length >> 29;
	if (1) {
		pkg_res->raw_data = raw;
	}

	return 0;
}

/* 资源保存函数 */
static int DMC4_arc_save_resource(struct resource *res, 
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
static void DMC4_arc_release_resource(struct package *pkg, 
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
static void DMC4_arc_release(struct package *pkg, 
							 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation DMC4_arc_operation = {
	DMC4_arc_match,					/* match */
	DMC4_arc_extract_directory,		/* extract_directory */
	DMC4_arc_parse_resource_info,	/* parse_resource_info */
	DMC4_arc_extract_resource,		/* extract_resource */
	DMC4_arc_save_resource,			/* save_resource */
	DMC4_arc_release_resource,		/* release_resource */
	DMC4_arc_release				/* release */
};

/********************* sngw *********************/

/* 封包匹配回调函数 */
static int DMC4_sngw_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	s8 magic[4];
	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "OggS", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int DMC4_sngw_extract_resource(struct package *pkg,
									  struct package_resource *pkg_res)
{
	u32 sngw_size;
	if (pkg->pio->length_of(pkg, &sngw_size))
		return -CUI_ELEN;

	BYTE *sngw = new BYTE[sngw_size];
	if (!sngw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, sngw, sngw_size, 0, IO_SEEK_SET)) {
		delete [] sngw;
		return -CUI_EREADVEC;
	}
	pkg_res->raw_data = sngw;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation DMC4_sngw_operation = {
	DMC4_sngw_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	DMC4_sngw_extract_resource,	/* extract_resource */
	DMC4_arc_save_resource,		/* save_resource */
	DMC4_arc_release_resource,	/* release_resource */
	DMC4_arc_release			/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK DMC4_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".arc"), NULL, 
		NULL, &DMC4_arc_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".sngw"), _T(".ogg"), 
		NULL, &DMC4_sngw_operation, CUI_EXT_FLAG_PKG))
			return -1;

	return 0;
}
