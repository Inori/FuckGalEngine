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

struct acui_information Irrlicht_Engine_cui_information = {
	_T("Nikolaus Gebhardt"),/* copyright */
	_T("Irrlicht Engine"),	/* system */
	_T(".ark .pack"),		/* package */
	_T("1.0.0"),			/* revision */
	_T("³Õh¹«Ù\"),			/* author */
	_T("2009-5-12 18:13"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

#pragma pack (1)
typedef struct {
	s8 name[260];
	u32 offset;
	u32 length;
} ark_entry_t;

typedef struct {
	s8 name[260];
	u8 unknown0;
	u32 length;
	u8 unknown1;
} pack_entry_t;
#pragma pack ()

typedef struct {
	char name[260];
	DWORD length;
	DWORD offset;
} my_pack_entry_t;

/********************* ark *********************/

static int Irrlicht_Engine_ark_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	s32 index_entries;
	if (pkg->pio->read(pkg, &index_entries, sizeof(index_entries))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (index_entries <= 0) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	ark_entry_t entry;
	if (pkg->pio->read(pkg, &entry, sizeof(entry))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (entry.offset != (index_entries * sizeof(entry) + 4)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	return 0;	
}

static int Irrlicht_Engine_ark_extract_directory(struct package *pkg,
												 struct package_directory *pkg_dir)
{
	u32 index_entries;
	if (pkg->pio->readvec(pkg, &index_entries, sizeof(index_entries), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD index_len = index_entries * sizeof(ark_entry_t);
	ark_entry_t *index = new ark_entry_t[index_entries];
	if (!index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index, index_len)) {
		delete [] index;
		return -CUI_EREAD;
	}

	for (DWORD j = 0; j < index_entries; ++j)
		for (DWORD i = 0; i < 260; ++i)
			index[j].name[i] = ~index[j].name[i];

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index;
	pkg_dir->directory_length = index_len;
	pkg_dir->index_entry_length = sizeof(ark_entry_t);

	return 0;
}

static int Irrlicht_Engine_ark_parse_resource_info(struct package *pkg,
												   struct package_resource *pkg_res)
{
	ark_entry_t *entry;

	entry = (ark_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, entry->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = entry->offset;

	return 0;
}

static int Irrlicht_Engine_ark_extract_resource(struct package *pkg,
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

	for (DWORD i = 0; i < pkg_res->raw_data_length; ++i)
		raw[i] = ~raw[i];

	pkg_res->raw_data = raw;

	return 0;
}

static int Irrlicht_Engine_ark_save_resource(struct resource *res, 
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

static void Irrlicht_Engine_ark_release_resource(struct package *pkg, 
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

static void Irrlicht_Engine_ark_release(struct package *pkg, 
										struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete []  pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

static cui_ext_operation Irrlicht_Engine_ark_operation = {
	Irrlicht_Engine_ark_match,				/* match */
	Irrlicht_Engine_ark_extract_directory,	/* extract_directory */
	Irrlicht_Engine_ark_parse_resource_info,/* parse_resource_info */
	Irrlicht_Engine_ark_extract_resource,	/* extract_resource */
	Irrlicht_Engine_ark_save_resource,		/* save_resource */
	Irrlicht_Engine_ark_release_resource,	/* release_resource */
	Irrlicht_Engine_ark_release				/* release */
};

/********************* pack *********************/

static int Irrlicht_Engine_pack_extract_directory(struct package *pkg,
												  struct package_directory *pkg_dir);

static int Irrlicht_Engine_pack_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	struct package_directory pkg_dir;
	int ret = Irrlicht_Engine_pack_extract_directory(pkg, &pkg_dir);
	if (ret) {
		pkg->pio->close(pkg);
		return ret;
	}
	delete [] pkg_dir.directory;

	return ret;	
}

static int Irrlicht_Engine_pack_extract_directory(struct package *pkg,
												  struct package_directory *pkg_dir)
{
	DWORD offset = 0;
	DWORD index_entries = 0;
	u32 fsize;

	pkg->pio->length_of(pkg, &fsize);

	while (offset != fsize) {
		pack_entry_t entry;
		if (pkg->pio->readvec(pkg, &entry, sizeof(entry), offset, IO_SEEK_SET))
			return -CUI_EREADVEC;
		offset += entry.length + sizeof(pack_entry_t);		
		++index_entries;
	}

	DWORD index_len = index_entries * sizeof(my_pack_entry_t);
	my_pack_entry_t *index = new my_pack_entry_t[index_entries];
	if (!index)
		return -CUI_EMEM;

	offset = 0;
	for (DWORD i = 0; i < index_entries; ++i) {
		pack_entry_t entry;
		if (pkg->pio->readvec(pkg, &entry, sizeof(entry), offset, IO_SEEK_SET)) {
			delete [] index;
			return -CUI_EREADVEC;
		}
		strcpy(index[i].name, entry.name);
		index[i].offset = offset + sizeof(pack_entry_t);
		index[i].length = entry.length;
		offset += entry.length + sizeof(pack_entry_t);
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index;
	pkg_dir->directory_length = index_len;
	pkg_dir->index_entry_length = sizeof(my_pack_entry_t);

	return 0;
}

static int Irrlicht_Engine_pack_parse_resource_info(struct package *pkg,
													struct package_resource *pkg_res)
{
	my_pack_entry_t *entry;

	entry = (my_pack_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, entry->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = entry->offset;

	return 0;
}

static int Irrlicht_Engine_pack_extract_resource(struct package *pkg,
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

static cui_ext_operation Irrlicht_Engine_pack_operation = {
	Irrlicht_Engine_pack_match,				/* match */
	Irrlicht_Engine_pack_extract_directory,	/* extract_directory */
	Irrlicht_Engine_pack_parse_resource_info,/* parse_resource_info */
	Irrlicht_Engine_pack_extract_resource,	/* extract_resource */
	Irrlicht_Engine_ark_save_resource,		/* save_resource */
	Irrlicht_Engine_ark_release_resource,	/* release_resource */
	Irrlicht_Engine_ark_release				/* release */
};

int CALLBACK Irrlicht_Engine_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".ark"), NULL, 
		NULL, &Irrlicht_Engine_ark_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".pack"), NULL, 
		NULL, &Irrlicht_Engine_pack_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;


	return 0;
}
