#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>

// N:\comic\AlterEgoÌåòY°æ090505

struct acui_information MNP_cui_information = {
	_T(""),/* copyright */
	_T(""),	/* system */
	_T(".mma"),		/* package */
	_T(""),			/* revision */
	_T("³Õh¹«Ù\"),			/* author */
	_T(""),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

#pragma pack (1)
typedef struct {
	s8 magic[4];	// "ARC!"
	u32 unknown0;	// 0x24
	u32 unknown1;	// 0x14
	u32 version;	// 1
	u32 index_entries;
	u32 unknown2;	// 0
	u32 unknown3;	// 0x190
	u32 unknown4;	// 1
	u32 mma_size;
} mma_header_t;

typedef struct {
	u32 offset;
	u32 uncomprlen;
	u32 comprlen;
	u32 unknown0;
	u32 unknown1;
} mma_entry_t;
#pragma pack ()

typedef struct {
	char name[MAX_PATH];
	DWORD uncomprlen;
	DWORD comprlen;
	DWORD offset;
} my_mma_entry_t;

/********************* mma *********************/

static int MNP_mma_match(struct package *pkg)
{
	mma_header_t header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &header, sizeof(header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(header.magic, "ARC!", 4) || header.version != 1) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	return 0;	
}

static int MNP_mma_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{
	mma_header_t header;

	if (pkg->pio->readvec(pkg, &header, sizeof(header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD index_len = header.index_entries * sizeof(mma_entry_t);
	mma_entry_t *index = new mma_entry_t[header.index_entries];
	if (!index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index, index_len)) {
		delete [] index;
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = header.index_entries;
	pkg_dir->directory = index;
	pkg_dir->directory_length = index_len;
	pkg_dir->index_entry_length = sizeof(mma_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

static int MNP_mma_parse_resource_info(struct package *pkg,
									   struct package_resource *pkg_res)
{
	mma_entry_t *entry;

	entry = (mma_entry_t *)pkg_res->actual_index_entry;
	sprintf(pkg_res->name, "%04d", pkg_res->index_number);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = entry->comprlen;
	pkg_res->actual_data_length = entry->uncomprlen;
	pkg_res->offset = entry->offset;

	return 0;
}

static int MNP_mma_extract_resource(struct package *pkg,
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

static int MNP_mma_save_resource(struct resource *res, 
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

static void MNP_mma_release_resource(struct package *pkg, 
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

static void MNP_mma_release(struct package *pkg, 
							struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete []  pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

static cui_ext_operation MNP_mma_operation = {
	MNP_mma_match,				/* match */
	MNP_mma_extract_directory,	/* extract_directory */
	MNP_mma_parse_resource_info,/* parse_resource_info */
	MNP_mma_extract_resource,	/* extract_resource */
	MNP_mma_save_resource,		/* save_resource */
	MNP_mma_release_resource,	/* release_resource */
	MNP_mma_release				/* release */
};

int CALLBACK MNP_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".mma"), NULL, 
		NULL, &MNP_mma_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
