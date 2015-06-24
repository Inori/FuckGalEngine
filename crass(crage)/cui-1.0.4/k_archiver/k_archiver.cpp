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

struct acui_information k_archiver_cui_information = {
	NULL,					/* copyright */
	NULL,					/* system */
	_T(".arc"),				/* package */
	_T("0.0.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-12-13 14:26"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};


static const TCHAR *simplified_chinese_strings[] = {
	_T("你应该使用unarc来提取<%s>，")
			_T("你可以在tools\\unarc找到它。\n")
};

static const TCHAR *traditional_chinese_strings[] = {
	_T("你應該使用unarc來提取<%s>，")
			_T("你可以在tools\\unarc找到它。\n")
};

static const TCHAR *japanese_strings[] = {
	_T("<%s>を抽出するにはunarcを使うべきです、")
			_T("tools\\unarcに見つかられます。\n")
};

static const TCHAR *default_strings[] = {
	_T("You should extract <%s> with unarc which can be found ")
			_T("in the tools\\unarc.\n")
};

static struct locale_configuration k_archiver_locale_configurations[4] = {
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

static int k_archiver_locale_id;

/********************* arc *********************/

static int k_archiver_arc_match(struct package *pkg)
{
	s8 magic[3];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (!memcmp(magic, "AR", 2) && memcmp(magic, "ARC", 3))
		locale_app_printf(k_archiver_locale_id, 0, pkg->name);

	return -CUI_EMATCH;	
}

static int k_archiver_arc_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	return 0;
}

static int k_archiver_arc_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	return 0;
}

static int k_archiver_arc_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	return 0;
}

static int k_archiver_arc_save_resource(struct resource *res, 
								  struct package_resource *pkg_res)
{
	return 0;
}

static void k_archiver_arc_release_resource(struct package *pkg, 
									  struct package_resource *pkg_res)
{
}

static void k_archiver_arc_release(struct package *pkg, 
							 struct package_directory *pkg_dir)
{
}

static cui_ext_operation k_archiver_arc_operation = {
	k_archiver_arc_match,				/* match */
	k_archiver_arc_extract_directory,	/* extract_directory */
	k_archiver_arc_parse_resource_info,	/* parse_resource_info */
	k_archiver_arc_extract_resource,	/* extract_resource */
	k_archiver_arc_save_resource,		/* save_resource */
	k_archiver_arc_release_resource,	/* release_resource */
	k_archiver_arc_release				/* release */
};

int CALLBACK k_archiver_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".arc"), NULL, 
		NULL, &k_archiver_arc_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	k_archiver_locale_id = locale_app_register(k_archiver_locale_configurations, 3);

	return 0;
}
