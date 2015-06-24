#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>
#include <utility.h>
#include <crass\locale.h>

struct acui_information pk_cui_information = {
	NULL,					/* copyright */
	NULL,					/* system */
	_T(".pk"),				/* package */
	_T("0.0.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-2-11 22:20"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};


static const TCHAR *simplified_chinese_strings[] = {
	_T("%s可能是一个zip文件，你只要把扩展名改成.zip就能用winrar或winzip提取了。\n")
};

static const TCHAR *traditional_chinese_strings[] = {
	_T("%s可能是一個zip文件，你只要把擴展名改成.zip就能用winrar或winzip提取了。\n")
};

static const TCHAR *japanese_strings[] = {
	_T("%sは単なる一個のzipファイルかもしれない、拡張子を.zipに変更さえすればwinrarまたはwinzipで抽出する事ができるでしょう。\n")
};

static const TCHAR *default_strings[] = {
	_T("%s may be a zip archive, you can simple change the extension to .zip ")
		_T("and use winrar or winzip to extract it.\n")
};

static struct locale_configuration pk_locale_configurations[4] = {
	{
		936, simplified_chinese_strings, 1
	},
	{
		950, traditional_chinese_strings, 1
	},
	{
		932, japanese_strings, 1
	},
	{
		0, default_strings, 1
	},
};

static int pk_locale_id;

/********************* pk *********************/

static int pk_match(struct package *pkg)
{
	s8 magic[3];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (!strncmp(magic, "PK", 2))
		locale_app_printf(pk_locale_id, 0, pkg->name);

	return -CUI_EMATCH;	
}

static int pk_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	return 0;
}

static int pk_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	return 0;
}

static int pk_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	return 0;
}

static int pk_save_resource(struct resource *res, 
								  struct package_resource *pkg_res)
{
	return 0;
}

static void pk_release_resource(struct package *pkg, 
									  struct package_resource *pkg_res)
{
}

static void pk_release(struct package *pkg, 
							 struct package_directory *pkg_dir)
{
}

static cui_ext_operation pk_operation = {
	pk_match,				/* match */
	pk_extract_directory,	/* extract_directory */
	pk_parse_resource_info,	/* parse_resource_info */
	pk_extract_resource,	/* extract_resource */
	pk_save_resource,		/* save_resource */
	pk_release_resource,	/* release_resource */
	pk_release				/* release */
};

int CALLBACK pk_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".pk"), NULL, 
		NULL, &pk_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	pk_locale_id = locale_app_register(pk_locale_configurations, 3);

	return 0;
}
