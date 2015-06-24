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
struct acui_information dibsec32_cui_information = {
	_T("minori"),		/* copyright */
	_T("dibsec32"),			/* system */
	_T(".dat"),		/* package */
	_T("1.0.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T(""),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	u32 index_length;
	u32 index_entries;
} dat_header_t;
#pragma pack ()

typedef struct {
	s8 name[256];
	u32 offset;
	u32 length;
} dat_entry_t;

/********************* dat *********************/

/* 封包匹配回调函数 */
static int dibsec32_dat_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包索引目录提取函数 */
static int dibsec32_dat_extract_directory(struct package *pkg,
										  struct package_directory *pkg_dir)
{
	dat_header_t dat_header;
	if (pkg->pio->read(pkg, &dat_header, sizeof(dat_header)))
		return -CUI_EREAD;

	BYTE *orig_index = new BYTE[dat_header.index_length];
	if (!orig_index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, orig_index, dat_header.index_length)) {
		delete [] orig_index;
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = dat_header.index_entries;
	pkg_dir->directory = orig_index;
	pkg_dir->directory_length = dat_header.index_length;
	pkg_dir->flags = PKG_DIR_FLAG_VARLEN;

	return 0;
}

/* 封包索引项解析函数 */
static int dibsec32_dat_parse_resource_info(struct package *pkg,
											struct package_resource *pkg_res)
{
	BYTE *p = (BYTE *)pkg_res->actual_index_entry;
	int nlen = strlen((char *)p);
	strcpy(pkg_res->name, (char *)p);
	p += nlen + 1;
	pkg_res->offset = *(u32 *)p;
	p += 8;	// skip offset_hi
	pkg_res->raw_data_length = *(u32 *)p;
	p += 16;	// skip ??
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->actual_index_entry_length = nlen + 1 + 0x18;

	return 0;
}

/* 封包资源提取函数 */
static int dibsec32_dat_extract_resource(struct package *pkg,
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

	for (DWORD i = 0; i < 256; ++i)
		dec[i] = i;

	BYTE key = 0;
	for (i = 0; i < 256; ++i) {
		BYTE key = key + dec[i] + *((BYTE *)v12 + v12_i++);
		BYTE tmp = dec[key];
		dec[key] = dec[i];  
		dec[i] = tmp;
		if (v12_i >= max_v12_len)
			v12_i = 0;
	}
 


	pkg_res->raw_data = raw;

	return 0;
}

/* 资源保存函数 */
static int dibsec32_dat_save_resource(struct resource *res, 
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
static void dibsec32_dat_release_resource(struct package *pkg, 
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
static void dibsec32_dat_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation dibsec32_dat_operation = {
	dibsec32_dat_match,					/* match */
	dibsec32_dat_extract_directory,		/* extract_directory */
	dibsec32_dat_parse_resource_info,	/* parse_resource_info */
	dibsec32_dat_extract_resource,		/* extract_resource */
	dibsec32_dat_save_resource,			/* save_resource */
	dibsec32_dat_release_resource,		/* release_resource */
	dibsec32_dat_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK dibsec32_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &dibsec32_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR
		| CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	return 0;
}
