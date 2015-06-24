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

struct acui_information ggRMV_cui_information = {
	_T(""),			/* copyright */
	_T(""),			/* system */
	_T(".ggm .gdp .rsd"),				/* package */
	_T("1.0.0"),			/* revision */
	_T("³Õh¹«Ù\"),			/* author */
	_T("2009-3-18 20:29"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

#pragma pack (1)
typedef struct {
	s8 magic[8];	// "ggDP100A"
	u32 index_entries;
	u8 reserved[20];
} gdp_header_t;

typedef struct {
	s8 name[16];
	u32 offset;
	u32 length;
	u32 id;
	u32 reserved;
} gdp_entry_t;

typedef struct {
	s8 magic[10];	// "ggMP101J\x1e"
	u16 index_entries;
	u16 width;
	u16 height;
	u8 reserved[16];
} ggm_header_t;

typedef struct {
	u32 offset;
	u32 length;
} ggm_entry_t;
#pragma pack ()


/********************* rsd *********************/

static int ggRMV_rsd_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

static int ggRMV_rsd_extract_resource(struct package *pkg,
									  struct package_resource *pkg_res)
{
	BYTE *compr = new BYTE[pkg_res->raw_data_length];
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, pkg_res->raw_data_length,
			pkg_res->offset, IO_SEEK_SET)) {
		delete [] compr;
		return -CUI_EREADVEC;
	}

	DWORD uncomprlen = pkg_res->raw_data_length << 1;
	while (1) {
		BYTE *uncompr = new BYTE[uncomprlen];
		if (!uncompr) {
			delete [] compr;
			return -CUI_EMEM;
		}

		pkg_res->actual_data_length = uncomprlen;
		if (uncompress(uncompr, &pkg_res->actual_data_length, 
				compr, pkg_res->raw_data_length) == Z_OK) {
			pkg_res->actual_data = uncompr;
			break;
		}
		delete [] uncompr;
		uncomprlen <<= 1;
	}

	pkg_res->raw_data = compr;

	return 0;
}

static int ggRMV_rsd_save_resource(struct resource *res, 
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

static void ggRMV_rsd_release_resource(struct package *pkg, 
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

static void ggRMV_rsd_release(struct package *pkg, 
							  struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete []  pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

static cui_ext_operation ggRMV_rsd_operation = {
	ggRMV_rsd_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	ggRMV_rsd_extract_resource,	/* extract_resource */
	ggRMV_rsd_save_resource,	/* save_resource */
	ggRMV_rsd_release_resource,	/* release_resource */
	ggRMV_rsd_release			/* release */
};

/********************* gdp *********************/

static int ggRMV_gdp_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	gdp_header_t header;

	if (pkg->pio->read(pkg, &header, sizeof(header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(header.magic, "ggDP100A", 8)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int ggRMV_gdp_extract_directory(struct package *pkg,
									   struct package_directory *pkg_dir)
{
	gdp_header_t header;
	if (pkg->pio->readvec(pkg, &header, sizeof(header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	gdp_entry_t *index = new gdp_entry_t[header.index_entries];
	if (!index)
		return -CUI_EMEM;

	DWORD index_len = header.index_entries * sizeof(gdp_entry_t);
	if (pkg->pio->read(pkg, index, index_len)) {
		delete [] index;
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = header.index_entries;
	pkg_dir->directory = index;
	pkg_dir->directory_length = index_len;
	pkg_dir->index_entry_length = sizeof(gdp_entry_t);

	return 0;
}

static int ggRMV_gdp_parse_resource_info(struct package *pkg,
										 struct package_resource *pkg_res)
{
	gdp_entry_t *gdp_entry;

	gdp_entry = (gdp_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, gdp_entry->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = gdp_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = gdp_entry->offset;

	return 0;
}

static int ggRMV_gdp_extract_resource(struct package *pkg,
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

	pkg_res->raw_data = raw;

	return 0;
}

static cui_ext_operation ggRMV_gdp_operation = {
	ggRMV_gdp_match,				/* match */
	ggRMV_gdp_extract_directory,	/* extract_directory */
	ggRMV_gdp_parse_resource_info,	/* parse_resource_info */
	ggRMV_gdp_extract_resource,		/* extract_resource */
	ggRMV_rsd_save_resource,		/* save_resource */
	ggRMV_rsd_release_resource,		/* release_resource */
	ggRMV_rsd_release				/* release */
};

/********************* ggm *********************/

static int ggRMV_ggm_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	ggm_header_t header;

	if (pkg->pio->read(pkg, &header, sizeof(header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(header.magic, "ggMP101J\x1e", 10)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int ggRMV_ggm_extract_directory(struct package *pkg,
									   struct package_directory *pkg_dir)
{
	ggm_header_t header;
	if (pkg->pio->readvec(pkg, &header, sizeof(header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	ggm_entry_t *index = new ggm_entry_t[header.index_entries];
	if (!index)
		return -CUI_EMEM;

	DWORD index_len = header.index_entries * sizeof(ggm_entry_t);
	if (pkg->pio->read(pkg, index, index_len)) {
		delete [] index;
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = header.index_entries;
	pkg_dir->directory = index;
	pkg_dir->directory_length = index_len;
	pkg_dir->index_entry_length = sizeof(ggm_entry_t);

	return 0;
}

static int ggRMV_ggm_parse_resource_info(struct package *pkg,
										 struct package_resource *pkg_res)
{
	ggm_entry_t *ggm_entry;

	ggm_entry = (ggm_entry_t *)pkg_res->actual_index_entry;
	sprintf(pkg_res->name, "%06d", pkg_res->index_number);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = ggm_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = ggm_entry->offset;

	return 0;
}

static cui_ext_operation ggRMV_ggm_operation = {
	ggRMV_ggm_match,				/* match */
	ggRMV_ggm_extract_directory,	/* extract_directory */
	ggRMV_ggm_parse_resource_info,	/* parse_resource_info */
	ggRMV_gdp_extract_resource,		/* extract_resource */
	ggRMV_rsd_save_resource,		/* save_resource */
	ggRMV_rsd_release_resource,		/* release_resource */
	ggRMV_rsd_release				/* release */
};

int CALLBACK ggRMV_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".ggm"), _T(".jpg"), 
		NULL, &ggRMV_ggm_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".gdp"), NULL, 
		NULL, &ggRMV_gdp_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".rsd"), NULL, 
		NULL, &ggRMV_rsd_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	return 0;
}
