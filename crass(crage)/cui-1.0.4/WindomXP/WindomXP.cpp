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
struct acui_information WindomXP_cui_information = {
	_T("Y.Kamada"),	/* copyright */
	_T("Ultimate Knight ウィンダムXP"),	/* system */
	_T(".png .jpg .bmp .x .spt .dds"),				/* package */
	_T("1.0.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-1-15 22:44"),	/* date */
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

static void xor_decode(BYTE *data, DWORD length)
{
	for (DWORD i = 0; i < length / 4; ++i)
		((u32 *)data)[i] ^= 0x0B7E7759;
}

/********************* xor *********************/

/* 封包匹配回调函数 */
static int WindomXP_png_match(struct package *pkg)
{
	u32 magic;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (magic != 0x4c3027d0) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包资源提取函数 */
static int WindomXP_xor_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	u32 fsize;
	pkg->pio->length_of(pkg, &fsize);

	BYTE *data = new BYTE[fsize];
	if (!data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, data, fsize, 0, IO_SEEK_SET)) {
		delete [] data;
		return -CUI_EREADVEC;
	}

	xor_decode(data, fsize);

	pkg_res->raw_data = data;
	pkg_res->raw_data_length = fsize;

	return 0;
}

/* 资源保存函数 */
static int WindomXP_xor_save_resource(struct resource *res, 
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
static void WindomXP_xor_release_resource(struct package *pkg, 
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
static void WindomXP_xor_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation WindomXP_png_operation = {
	WindomXP_png_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	WindomXP_xor_extract_resource,	/* extract_resource */
	WindomXP_xor_save_resource,		/* save_resource */
	WindomXP_xor_release_resource,	/* release_resource */
	WindomXP_xor_release			/* release */
};

/********************* x *********************/

/* 封包匹配回调函数 */
static int WindomXP_x_match(struct package *pkg)
{
	u32 magic;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (magic != 0x2b181821) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包处理回调函数集合 */
static cui_ext_operation WindomXP_x_operation = {
	WindomXP_x_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	WindomXP_xor_extract_resource,	/* extract_resource */
	WindomXP_xor_save_resource,		/* save_resource */
	WindomXP_xor_release_resource,	/* release_resource */
	WindomXP_xor_release			/* release */
};

/********************* jpg *********************/

/* 封包匹配回调函数 */
static int WindomXP_jpg_match(struct package *pkg)
{
	u32 magic;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (magic != 0xeb81afa6) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包处理回调函数集合 */
static cui_ext_operation WindomXP_jpg_operation = {
	WindomXP_jpg_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	WindomXP_xor_extract_resource,	/* extract_resource */
	WindomXP_xor_save_resource,		/* save_resource */
	WindomXP_xor_release_resource,	/* release_resource */
	WindomXP_xor_release			/* release */
};

/********************* bmp *********************/

/* 封包匹配回调函数 */
static int WindomXP_bmp_match(struct package *pkg)
{
	u16 magic;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (magic != 0x3a1b) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包处理回调函数集合 */
static cui_ext_operation WindomXP_bmp_operation = {
	WindomXP_bmp_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	WindomXP_xor_extract_resource,	/* extract_resource */
	WindomXP_xor_save_resource,		/* save_resource */
	WindomXP_xor_release_resource,	/* release_resource */
	WindomXP_xor_release			/* release */
};

/********************* spt *********************/

/* 封包匹配回调函数 */
static int WindomXP_spt_match(struct package *pkg)
{
	u16 magic;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (magic != 0x3a7e && magic != 0x4e19 && magic != 0x1617) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包处理回调函数集合 */
static cui_ext_operation WindomXP_spt_operation = {
	WindomXP_spt_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	WindomXP_xor_extract_resource,	/* extract_resource */
	WindomXP_xor_save_resource,		/* save_resource */
	WindomXP_xor_release_resource,	/* release_resource */
	WindomXP_xor_release			/* release */
};

/********************* dds *********************/

/* 封包匹配回调函数 */
static int WindomXP_dds_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "\x1d\x33\x2d\x2b", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包处理回调函数集合 */
static cui_ext_operation WindomXP_dds_operation = {
	WindomXP_dds_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	WindomXP_xor_extract_resource,	/* extract_resource */
	WindomXP_xor_save_resource,		/* save_resource */
	WindomXP_xor_release_resource,	/* release_resource */
	WindomXP_xor_release			/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK WindomXP_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".png"), _T(".png"), 
		NULL, &WindomXP_png_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".x"), _T(".x"), 
		NULL, &WindomXP_x_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".jpg"), _T(".jpg"), 
		NULL, &WindomXP_jpg_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".bmp"), _T(".bmp"), 
		NULL, &WindomXP_bmp_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".spt"), _T(".spt"), 
		NULL, &WindomXP_spt_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dds"), _T(".dds"), 
		NULL, &WindomXP_dds_operation, CUI_EXT_FLAG_PKG))
			return -1;

	return 0;
}
