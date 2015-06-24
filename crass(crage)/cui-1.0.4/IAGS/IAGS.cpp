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

struct acui_information IAGS_cui_information = {
	_T("WINTERS"),				/* copyright */
	_T("IAGS"),					/* system */
	_T(".IFP"),					/* package */
	_T("1.0.1"),				/* revision */
	_T("痴漢公賊"),				/* author */
	_T("2008-2-26 20:04"),		/* date */
	NULL,						/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

#pragma pack (1)
typedef struct {
	s8 magic[16];	/* "IAGS_IFP_01     " */
} IFP_header_t;

typedef struct {
	u32 type;		/* 0 - NULL; 1 - first; c - normal entry */
	u32 offset;
	u32 length;
	u32 reserved;
} IFP_entry_t;
#pragma pack ()

/********************* IFP *********************/

static int IAGS_IFP_match(struct package *pkg)
{
	IFP_header_t IFP_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &IFP_header, sizeof(IFP_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(IFP_header.magic, "IAGS_IFP_01     ", 16)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int IAGS_IFP_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	IFP_entry_t IFP_entry;
	IFP_entry_t *index_buffer;
	unsigned int index_buffer_length;	

	if (pkg->pio->read(pkg, &IFP_entry, sizeof(IFP_entry)))
		return -CUI_EREAD;

	if (IFP_entry.type != 1)
		return -CUI_EMATCH;

	pkg_dir->index_entries = IFP_entry.length / sizeof(IFP_entry) - 1;
	index_buffer_length = IFP_entry.length - sizeof(IFP_entry);
	index_buffer = (IFP_entry_t *)new BYTE[index_buffer_length];
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		delete [] index_buffer;
		return -CUI_EREAD;
	}
	pkg_dir->index_entry_length = sizeof(IFP_entry);
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->flags |= PKG_DIR_FLAG_SKIP0;

	return 0;
}

static int IAGS_IFP_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	IFP_entry_t *IFP_entry;

	IFP_entry = (IFP_entry_t *)pkg_res->actual_index_entry;
	sprintf(pkg_res->name, "%08d", pkg_res->index_number);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = IFP_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = IFP_entry->offset;

	return 0;
}

static int IAGS_IFP_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	pkg_res->raw_data = new BYTE[pkg_res->raw_data_length];
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			delete [] pkg_res->raw_data;
			pkg_res->raw_data = NULL;
			return -CUI_EREADVEC;
	}

	if (!memcmp(pkg_res->raw_data, "\x89PNG", 4)) {
		pkg_res->replace_extension = _T(".png");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
	} else if (!memcmp(pkg_res->raw_data, "BM", 2)) {
		pkg_res->replace_extension = _T(".bmp");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
	} else if (!memcmp(pkg_res->raw_data, "\xff\xd8\xff\xe0\x00\x10JFIF", 10)) {
		pkg_res->replace_extension = _T(".jpg");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
	}

	return 0;
}

static int IAGS_IFP_save_resource(struct resource *res, 
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

static void IAGS_IFP_release_resource(struct package *pkg, 
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

static void IAGS_IFP_release(struct package *pkg, 
							 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

static cui_ext_operation IAGS_IFP_operation = {
	IAGS_IFP_match,					/* match */
	IAGS_IFP_extract_directory,		/* extract_directory */
	IAGS_IFP_parse_resource_info,	/* parse_resource_info */
	IAGS_IFP_extract_resource,		/* extract_resource */
	IAGS_IFP_save_resource,			/* save_resource */
	IAGS_IFP_release_resource,		/* release_resource */
	IAGS_IFP_release				/* release */
};

int CALLBACK IAGS_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".IFP"), NULL, 
		NULL, &IAGS_IFP_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
