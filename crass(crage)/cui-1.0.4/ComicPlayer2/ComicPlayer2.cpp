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

struct acui_information ComicPlayer2_cui_information = {
	_T("Masaya Hikida"),			/* copyright */
	_T("¥³¥ß¥Ã¥¯¥×¥ì©`¥ä©`£²"),			/* system */
	_T(".cpf"),				/* package */
	_T("1.0.0"),			/* revision */
	_T("³Õh¹«Ù\"),			/* author */
	_T("2009-2-20 23:31"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

#pragma pack (1)
typedef struct {
	s8 magic[8];	// "ICM95"
	s8 desc[256];
	u32 index_entries;
} cpf_header_t;

typedef struct {
	s8 name[256];
	u32 flags;
	u32 reserved;
	u32 offset;
	u32 length;
} cpf_entry_t;
#pragma pack ()


/********************* cpf *********************/

static int ComicPlayer2_cpf_match(struct package *pkg)
{
	cpf_header_t cpf_header_t;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &cpf_header_t, sizeof(cpf_header_t))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (strncmp(cpf_header_t.magic, "ICM95", sizeof(cpf_header_t.magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int ComicPlayer2_cpf_extract_directory(struct package *pkg,
											  struct package_directory *pkg_dir)
{
	cpf_header_t header;
	if (pkg->pio->readvec(pkg, &header, sizeof(header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	cpf_entry_t *index = new cpf_entry_t[header.index_entries];
	if (!index)
		return -CUI_EMEM;

	DWORD index_len = header.index_entries * sizeof(cpf_entry_t);
	if (pkg->pio->read(pkg, index, index_len)) {
		delete [] index;
		return -CUI_EREAD;
	}

	for (DWORD i = 0; i < header.index_entries; ++i)
		index[i].offset += sizeof(header) + index_len;

	pkg_dir->index_entries = header.index_entries;
	pkg_dir->directory = index;
	pkg_dir->directory_length = index_len;
	pkg_dir->index_entry_length = sizeof(cpf_entry_t);

	return 0;
}

static int ComicPlayer2_cpf_parse_resource_info(struct package *pkg,
												struct package_resource *pkg_res)
{
	cpf_entry_t *cpf_entry;

	cpf_entry = (cpf_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, cpf_entry->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = cpf_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = cpf_entry->offset;

	return 0;
}

static int ComicPlayer2_cpf_extract_resource(struct package *pkg,
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

	cpf_entry_t *cpf_entry = (cpf_entry_t *)pkg_res->actual_index_entry;
	DWORD uncomprlen = pkg_res->raw_data_length << 1;
	if (cpf_entry->flags & 0x80000000) {
		while (1) {
			BYTE *uncompr = new BYTE[uncomprlen];
			if (!uncompr) {
				delete [] raw;
				return -CUI_EMEM;
			}

			pkg_res->actual_data_length = uncomprlen;
			if (uncompress(uncompr, &pkg_res->actual_data_length, 
					raw, pkg_res->raw_data_length) == Z_OK) {
				pkg_res->actual_data = uncompr;
				break;
			}
			delete [] uncompr;
			uncomprlen <<= 1;
		}
	}

	pkg_res->raw_data = raw;

	return 0;
}

static int ComicPlayer2_cpf_save_resource(struct resource *res, 
										  struct package_resource *pkg_res)
{
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (pkg_res->actual_data, pkg_res->actual_data_length) {
		if (res->rio->write(res, pkg_res->actual_data, pkg_res->actual_data_length)) {
			res->rio->close(res);
			return -CUI_EWRITE;
		}
	} else if (pkg_res->raw_data, pkg_res->raw_data_length) {
		if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
			res->rio->close(res);
			return -CUI_EWRITE;
		}
	}

	res->rio->close(res);
	
	return 0;
}

static void ComicPlayer2_cpf_release_resource(struct package *pkg, 
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

static void ComicPlayer2_cpf_release(struct package *pkg, 
									 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete []  pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

static cui_ext_operation ComicPlayer2_cpf_operation = {
	ComicPlayer2_cpf_match,					/* match */
	ComicPlayer2_cpf_extract_directory,		/* extract_directory */
	ComicPlayer2_cpf_parse_resource_info,	/* parse_resource_info */
	ComicPlayer2_cpf_extract_resource,		/* extract_resource */
	ComicPlayer2_cpf_save_resource,			/* save_resource */
	ComicPlayer2_cpf_release_resource,		/* release_resource */
	ComicPlayer2_cpf_release				/* release */
};

int CALLBACK ComicPlayer2_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".cpf"), NULL, 
		NULL, &ComicPlayer2_cpf_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
