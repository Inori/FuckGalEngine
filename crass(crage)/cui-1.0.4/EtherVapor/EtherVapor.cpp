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
struct acui_information EtherVapor_cui_information = {
	_T("Edelweiss"),		/* copyright */
	_T(""),					/* system */
	_T(".pac"),				/* package */
	_T("1.0.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-8-20 21:29"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];		// "YPAC"
	u32 version;		// 1
	u32 index_entries;
	u32 zero;
} pac_header_t;

typedef struct {
	s8 name[96];
	u32 offset_lo;
	u32 offset_hi;
	u32 length_lo;
	u32 length_hi;
} pac_entry_t;
#pragma pack ()

/********************* pac *********************/

/* 封包匹配回调函数 */
static int EtherVapor_pac_match(struct package *pkg)
{
	pac_header_t pac_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &pac_header, sizeof(pac_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(pac_header.magic, "YPAC", 4) || pac_header.version != 1) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int EtherVapor_pac_extract_directory(struct package *pkg,
											struct package_directory *pkg_dir)
{
	pac_header_t pac_header;

	if (pkg->pio->readvec(pkg, &pac_header, sizeof(pac_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	pac_entry_t *index_buffer = new pac_entry_t[pac_header.index_entries];
	if (!index_buffer)
		return -CUI_EMEM;

	DWORD index_buffer_length = pac_header.index_entries * sizeof(pac_entry_t);
	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		delete [] index_buffer;
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = pac_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(pac_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int EtherVapor_pac_parse_resource_info(struct package *pkg,
									struct package_resource *pkg_res)
{
	pac_entry_t *pac_entry;

	pac_entry = (pac_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, pac_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = pac_entry->length_lo;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = pac_entry->offset_lo;

	return 0;
}

/* 封包资源提取函数 */
static int EtherVapor_pac_extract_resource(struct package *pkg,
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

	if (strstr(pkg_res->name, ".tgs")) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".png");
	}

	pkg_res->raw_data = raw;

	return 0;
}

/* 资源保存函数 */
static int EtherVapor_pac_save_resource(struct resource *res, 
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
static void EtherVapor_pac_release_resource(struct package *pkg, 
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
static void EtherVapor_pac_release(struct package *pkg, 
								   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation EtherVapor_pac_operation = {
	EtherVapor_pac_match,				/* match */
	EtherVapor_pac_extract_directory,	/* extract_directory */
	EtherVapor_pac_parse_resource_info,	/* parse_resource_info */
	EtherVapor_pac_extract_resource,	/* extract_resource */
	EtherVapor_pac_save_resource,		/* save_resource */
	EtherVapor_pac_release_resource,	/* release_resource */
	EtherVapor_pac_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK EtherVapor_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".pac"), NULL, 
		NULL, &EtherVapor_pac_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
