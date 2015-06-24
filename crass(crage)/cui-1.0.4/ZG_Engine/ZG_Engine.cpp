#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <utility.h>
#include <stdio.h>

struct acui_information ZG_Engine_cui_information = {
	_T("mu:die"),				/* copyright */
	_T("ZG-Engine"),			/* system */
	_T(".wav .png .map .bmp"),				/* package */
	_T("1.0.1"),		/* revision */
	_T("³Õh¹«Ù\"),		/* author */
	_T("2009-7-12 11:45"),/* date */
	NULL,				/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

#pragma pack (1)
typedef struct {
	u32 width;
	u32 height;
	u32 unknown[6];
} map_header_t;
#pragma pack ()

static void *my_malloc(DWORD len)
{
	return new BYTE[len];
}

/********************* wav *********************/

static int ZG_Engine_wav_match(struct package *pkg)
{
	s8 magic[16];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "RIFF", 4) || strncmp(magic + 8, "ZAVE", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int ZG_Engine_wav_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	BYTE *wav = new BYTE[pkg_res->raw_data_length];
	if (!wav)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, wav, pkg_res->raw_data_length, 0, IO_SEEK_SET)) {
		delete [] wav;
		return -CUI_EREADVEC;
	}

	wav[8] = 'W';

	pkg_res->raw_data = wav;

	return 0;
}

static int ZG_Engine_save_resource(struct resource *res,

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

static void ZG_Engine_release_resource(struct package *pkg, 
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

static void ZG_Engine_release(struct package *pkg, 
							  struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	package_set_private(pkg, NULL);
	pkg->pio->close(pkg);
}

static cui_ext_operation ZG_Engine_wav_operation = {
	ZG_Engine_wav_match,			/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	ZG_Engine_wav_extract_resource,	/* extract_resource */
	ZG_Engine_save_resource,		/* save_resource */
	ZG_Engine_release_resource,		/* release_resource */
	ZG_Engine_release				/* release */
};

/********************* map *********************/

static int ZG_Engine_map_match(struct package *pkg)
{
	map_header_t map_header;
	
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &map_header, sizeof(map_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	u32 size;
	pkg->pio->length_of(pkg, &size);

	if (size != sizeof(map_header) + map_header.width * map_header.height) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int ZG_Engine_map_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	map_header_t map_header;
	
	if (pkg->pio->readvec(pkg, &map_header, sizeof(map_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD rgb_len = pkg_res->raw_data_length - sizeof(map_header);
	BYTE *rgb = new BYTE[rgb_len];
	if (pkg->pio->readvec(pkg, rgb, rgb_len, sizeof(map_header), IO_SEEK_SET)) {
		delete [] rgb;
		return -CUI_EREADVEC;
	}

	if (MyBuildBMPFile(rgb, rgb_len, NULL, 0, map_header.width, map_header.height,
			8, (BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, my_malloc)) {
		delete [] rgb;
		return -CUI_EREADVEC;		
	}
	delete [] rgb;

	return 0;
}

static cui_ext_operation ZG_Engine_map_operation = {
	ZG_Engine_map_match,			/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	ZG_Engine_map_extract_resource,	/* extract_resource */
	ZG_Engine_save_resource,		/* save_resource */
	ZG_Engine_release_resource,		/* release_resource */
	ZG_Engine_release				/* release */
};

/********************* png *********************/

static int ZG_Engine_png_match(struct package *pkg)
{
	s8 magic[5];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "Z\x89PNG", 5)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int ZG_Engine_png_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	BYTE *png = new BYTE[pkg_res->raw_data_length - 1];
	if (!png)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, png, pkg_res->raw_data_length - 1, 1, IO_SEEK_SET)) {
		delete [] png;
		return -CUI_EREADVEC;
	}

	pkg_res->raw_data = png;
	--pkg_res->raw_data_length;

	return 0;
}

static cui_ext_operation ZG_Engine_png_operation = {
	ZG_Engine_png_match,			/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	ZG_Engine_png_extract_resource,	/* extract_resource */
	ZG_Engine_save_resource,		/* save_resource */
	ZG_Engine_release_resource,		/* release_resource */
	ZG_Engine_release				/* release */
};

/********************* bmp *********************/

static int ZG_Engine_bmp_match(struct package *pkg)
{
	s8 magic[5];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "ZBM", 3)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static cui_ext_operation ZG_Engine_bmp_operation = {
	ZG_Engine_bmp_match,			/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	ZG_Engine_png_extract_resource,	/* extract_resource */
	ZG_Engine_save_resource,		/* save_resource */
	ZG_Engine_release_resource,		/* release_resource */
	ZG_Engine_release				/* release */
};

int CALLBACK ZG_Engine_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".wav"), _T(".wav"), 
		NULL, &ZG_Engine_wav_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".png"), _T(".png"),
		NULL, &ZG_Engine_png_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".bmp"), _T(".png"),
		NULL, &ZG_Engine_bmp_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".map"), _T(".bmp"), 
		NULL, &ZG_Engine_map_operation, CUI_EXT_FLAG_PKG))
			return -1;

	return 0;
}
