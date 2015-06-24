#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <resource.h>
#include <package.h>
#include <cui_error.h>
#include <utility.h>
#include <stdio.h>
#include <cui_common.h>

struct acui_information GsWin_cui_information = {
	_T("株式会社ちぇりーそふと"),	/* copyright */
	_T("GsWin3.0.0.0 Project11"),	/* system */
	_T(".pak"),						/* package */
	_T(""),							/* revision */
	_T("痴漢公賊"),					/* author */
	_T(""),							/* date */
	NULL,							/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};	

#pragma pack (1)
typedef struct {
	s8 magic[20];		/* "CHERRY PACK 3.0" */
	u32 index_entries;
	u32 daata_offset;
} pak_header_t;

typedef struct {
	s8 name[16];
	u32 offset;
	u32 length;
} pak_entry_t;
#pragma pack ()

/********************* pak *********************/

static int GsWin_pak_match(struct package *pkg)
{
	s8 magic[20];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (strncmp(magic, "CHERRY PACK 3.0", sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int GsWin_pak_extract_directory(struct package *pkg,
									   struct package_directory *pkg_dir)
{
	u32 index_entries;
	if (pkg->pio->read(pkg, &index_entries, sizeof(index_entries)))
		return -CUI_EREAD;

	u32 data_offset;
	if (pkg->pio->read(pkg, &data_offset, sizeof(data_offset)))
		return -CUI_EREAD;

	DWORD index_buffer_length = index_entries * sizeof(pak_entry_t);
	pak_entry_t *index_buffer = new pak_entry_t[index_entries];
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		delete [] index_buffer;
		return -CUI_EREAD;
	}

	for (DWORD i = 0; i < index_entries; ++i)
		index_buffer[i].offset += data_offset;

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(pak_entry_t);

	return 0;
}

static int GsWin_pak_parse_resource_info(struct package *pkg,
										 struct package_resource *pkg_res)
{
	pak_entry_t *pak_entry;	

	pak_entry = (pak_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, pak_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = pak_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = pak_entry->offset;

	return 0;
}

static int GsWin_pak_extract_resource(struct package *pkg,
									  struct package_resource *pkg_res)
{
	DWORD raw_len = pkg_res->raw_data_length;
	BYTE *raw = new BYTE[raw_len];
	if (!raw)
		return -CUI_EMEM;
		
	if (pkg->pio->readvec(pkg, raw, raw_len, pkg_res->offset, IO_SEEK_SET)) {
		delete [] raw;
		return -CUI_EREADVEC;
	}

	pkg_res->raw_data = raw;

	return 0;
}

static void GsWin_release_resource(struct package *pkg,
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

static cui_ext_operation GsWin_pak_operation = {
	GsWin_pak_match,				/* match */
	GsWin_pak_extract_directory,	/* extract_directory */
	GsWin_pak_parse_resource_info,	/* parse_resource_info */
	GsWin_pak_extract_resource,		/* extract_resource */
	cui_common_save_resource,		/* save_resource */
	GsWin_release_resource,			/* release_resource */
	cui_common_release				/* release */
};

int CALLBACK GsWin_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".pak"), NULL, 
		NULL, &GsWin_pak_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
