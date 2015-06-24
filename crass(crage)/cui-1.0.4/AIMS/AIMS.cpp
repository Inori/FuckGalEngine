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

/* TODO: .FMF解析 */

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information AIMS_cui_information = {
	_T("D.N.A. Softwares"),	/* copyright */
	_T("Actor-based Integrated Multimedia Script system"),			/* system */
	_T(".cfg"),				/* package */
	_T("1.0.0"),			/* revision */
	_T("绫波微步"),			/* author */
	_T("2009-1-26 13:05"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 maigc[4];
	u32 uncomprlen;
} cfg_header_t;
#pragma pack ()

/********************* cfg *********************/

/* 封包匹配回调函数 */
static int AIMS_cfg_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "DS@\x1", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int AIMS_cfg_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD uncomprlen, act_uncomprlen;
	u32 comprlen;

	if (pkg->pio->length_of(pkg, &comprlen))
		return -CUI_ELEN;

	if (pkg->pio->readvec(pkg, &uncomprlen, sizeof(uncomprlen), 4, IO_SEEK_SET))
		return -CUI_EREAD;

	comprlen -= sizeof(cfg_header_t);

	compr = (BYTE *)pkg->pio->readvec_only(pkg, comprlen, 8, IO_SEEK_SET);
	if (!compr)
		return -CUI_EREADVECONLY;

	uncompr = (BYTE *)malloc(uncomprlen);
	if (!uncompr)
		return -CUI_EMEM;

	act_uncomprlen = uncomprlen;
	if (uncompress(uncompr, &act_uncomprlen, compr, comprlen) != Z_OK) {
		free(uncompr);
		return -CUI_EUNCOMPR;
	}
	if (act_uncomprlen != uncomprlen) {
		free(uncompr);
		return -CUI_EUNCOMPR;
	}

	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

/* 资源保存函数 */
static int AIMS_cfg_save_resource(struct resource *res, 
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
static void AIMS_cfg_release_resource(struct package *pkg, 
									  struct package_resource *pkg_res)
{
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
static void AIMS_cfg_release(struct package *pkg, 
							 struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation AIMS_cfg_operation = {
	AIMS_cfg_match,				/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	AIMS_cfg_extract_resource,	/* extract_resource */
	AIMS_cfg_save_resource,		/* save_resource */
	AIMS_cfg_release_resource,	/* release_resource */
	AIMS_cfg_release			/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK AIMS_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".cfg"), NULL, 
		NULL, &AIMS_cfg_operation, CUI_EXT_FLAG_PKG))
			return -1;

	return 0;
}
