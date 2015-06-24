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
struct acui_information GScripter_cui_information = {
	_T("GoChin(ACT-Zero)"),	/* copyright */
	_T("Script Player for Win32"),	/* system */
	_T(""),					/* package */
	_T("1.0.2"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-6-8 10:58"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/********************* *********************/

/* 封包匹配回调函数 */
static int GScripter_match(struct package *pkg)
{
	s8 magic[10];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "\x89PNG", 4) 
			&& memcmp(magic, "\xff\xd8\xff\xe0\x00\x10JFIF", 10)
			&& memcmp(magic, "\x00\x00\x01\xba", 4)
			&& memcmp(magic, "\xff\xfb", 2)
			&& memcmp(magic, "ID3", 3)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int GScripter_extract_resource(struct package *pkg,
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

	if (!memcmp(raw, "\x89PNG", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".png");
	} else if (!memcmp(raw, "\xff\xd8\xff\xe0\x00\x10JFIF", 10)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".jpg");
	} else if (!memcmp(raw, "\x00\x00\x01\xba", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".mpg");
	} else if (!memcmp(raw, "\xff\xfb", 2) || !memcmp(raw, "ID3", 3)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".mp3");
	}
	pkg_res->raw_data = raw;

	return 0;
}

/* 资源保存函数 */
static int GScripter_save_resource(struct resource *res, 
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
static void GScripter_release_resource(struct package *pkg, 
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
static void GScripter_release(struct package *pkg, 
							  struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation GScripter_operation = {
	GScripter_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	GScripter_extract_resource,		/* extract_resource */
	GScripter_save_resource,		/* save_resource */
	GScripter_release_resource,		/* release_resource */
	GScripter_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK GScripter_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, NULL, NULL, 
		NULL, &GScripter_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_NOEXT))
			return -1;

	return 0;
}
