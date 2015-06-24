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

struct acui_information AGS32i_cui_information = {
	NULL,					/* copyright */
	_T("AGS32i"),			/* system */
	_T(".GSS .GSL"),		/* package */
	_T("1.0.0"),			/* revision */
	_T("≥’ùhπ´Ÿ\"),			/* author */
	_T("2009-2-14 15:54"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

#pragma pack (1)
typedef struct {
	s8 magic[4];	// "GSS"
	u32 uncomprlen;
} GSS_header_t;
#pragma pack ()

static void decode(BYTE *dat, int len, DWORD seed=0x20050101)
{
	for (int i = 0; i < len; ++i) {
		int v = i / 4;
		DWORD tmp = (DWORD)((v + ((s64)(0xffffffff84210843 * v) >> 32)) >> 4);
		DWORD key = seed + tmp + (tmp >> 31);
		key = key << (v % 31) | key >> (32 - v % 31);
		dat[i] ^= ((BYTE *)&key)[i & 3];
	}
}

/********************* GSL *********************/

static int AGS32i_GSL_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

static int AGS32i_GSL_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	BYTE *raw = new BYTE[pkg_res->raw_data_length];
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length, 0, IO_SEEK_SET)) {
		delete [] raw;
		return -CUI_EREADVEC;
	}

	decode(raw, pkg_res->raw_data_length);

	pkg_res->raw_data = raw;

	return 0;
}

static int AGS32i_GSL_save_resource(struct resource *res, 
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

static void AGS32i_GSL_release_resource(struct package *pkg, 
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

static void AGS32i_GSL_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

static cui_ext_operation AGS32i_GSL_operation = {
	AGS32i_GSL_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	AGS32i_GSL_extract_resource,/* extract_resource */
	AGS32i_GSL_save_resource,	/* save_resource */
	AGS32i_GSL_release_resource,/* release_resource */
	AGS32i_GSL_release			/* release */
};

/********************* wav *********************/

static int AGS32i_wav_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	decode((BYTE *)magic, sizeof(magic));

	if (strncmp(magic, "RIFF", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static cui_ext_operation AGS32i_wav_operation = {
	AGS32i_wav_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	AGS32i_GSL_extract_resource,/* extract_resource */
	AGS32i_GSL_save_resource,	/* save_resource */
	AGS32i_GSL_release_resource,/* release_resource */
	AGS32i_GSL_release			/* release */
};

/********************* GSS *********************/

static int AGS32i_GSS_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	decode((BYTE *)magic, sizeof(magic));

	if (strncmp(magic, "GSS", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int AGS32i_GSS_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	BYTE *raw = new BYTE[pkg_res->raw_data_length];
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length, 0, IO_SEEK_SET)) {
		delete [] raw;
		return -CUI_EREADVEC;
	}

	decode(raw, pkg_res->raw_data_length);

	GSS_header_t *GSS = (GSS_header_t *)raw;
	DWORD uncomprlen = GSS->uncomprlen;
	BYTE *uncompr = new BYTE[uncomprlen + sizeof(BITMAPFILEHEADER)];
	if (!uncompr) {
		delete [] raw;
		return -CUI_EMEM;
	}

	DWORD act_uncomprlen = uncomprlen;
	if (uncompress(uncompr + sizeof(BITMAPFILEHEADER), &act_uncomprlen, raw + sizeof(GSS_header_t),
			pkg_res->raw_data_length - sizeof(GSS_header_t)) != Z_OK) {
		delete [] uncompr;
		delete [] raw;
		return -CUI_EUNCOMPR;
	}
	delete [] raw;

	if (act_uncomprlen != uncomprlen) {
		delete [] uncompr;
		return -CUI_EUNCOMPR;
	}

	BITMAPFILEHEADER *bmfh = (BITMAPFILEHEADER *)uncompr;
	bmfh->bfType = 'MB';
	bmfh->bfSize = uncomprlen + sizeof(BITMAPFILEHEADER);
	bmfh->bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	bmfh->bfReserved1 = 0;
	bmfh->bfReserved2 = 0;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen + sizeof(BITMAPFILEHEADER);

	return 0;
}

static cui_ext_operation AGS32i_GSS_operation = {
	AGS32i_GSS_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	AGS32i_GSS_extract_resource,/* extract_resource */
	AGS32i_GSL_save_resource,	/* save_resource */
	AGS32i_GSL_release_resource,/* release_resource */
	AGS32i_GSL_release			/* release */
};

int CALLBACK AGS32i_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".GSS"), _T(".bmp"), 
		NULL, &AGS32i_GSS_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, NULL, _T(".wav"), 
		NULL, &AGS32i_wav_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_NOEXT))
			return -1;

	if (callback->add_extension(callback->cui, _T(".GSL"), _T("._GSL"), 
		NULL, &AGS32i_GSL_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	return 0;
}
