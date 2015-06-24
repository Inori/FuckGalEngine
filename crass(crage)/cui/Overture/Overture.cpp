#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <io_request.h>
#include <package.h>
#include <cui_error.h>
//#include <stdio.h>

struct acui_information Overture_cui_information = {
	_T("F&C Co.,Ltd"),	/* copyright */
	_T("Advanced System \"Overture\""),					/* system */
	_T(".MGR"),					/* package */
	_T("0.01"),					/* revision */
	_T("痴汉公贼"),				/* author */
	_T(""),		/* date */
	NULL,						/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

#pragma pack (1)
/* 体验版 */
typedef struct {
	s8 magic[12];
	u16 entries;
	u16 mode;
} isa0_header_t;

typedef struct {
	s8 name[16];
	u32 offset;
	u32 length;
	u32 reserved[2];
} isa0_entry_t;

typedef struct {
	s8 magic[12];
	u16 entries;
	u16 mode;		/* bit0 - 是否用二分法定位指定的资源；bit15 - 是否需要对索引段解密 */
} isa_header_t;

typedef struct {
	s8 name[36];
	u32 offset;
	u32 length;
	u32 reserved;
} isa_entry_t;
#pragma pack ()

/********************* isa *********************/

static int Overture_MRG_match(struct package *pkg)
{
	s8 magic[12];

	if (!pkg)
		return -CUI_EPARA;

	if (pkg->ior.io->init(&pkg->ior, 0))
		return -CUI_EINIT;

	if (pkg->ior.io->read(&pkg->ior, magic, sizeof(magic))) {
		pkg->ior.io->finish(&pkg->ior);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "ISM ARCHIVED", 12)) {
		pkg->ior.io->finish(&pkg->ior);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int Overture_MRG_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{
	DWORD *index_buffer;
	unsigned int index_buffer_length;	
	u16 mode;

	if (!pkg || !pkg_dir)
		return -CUI_EPARA;

	if (pkg->ior.io->seek(&pkg->ior, 12, IO_SEEK_SET))
		return -CUI_ESEEK;

	if (pkg->ior.io->read(&pkg->ior, &pkg_dir->index_entries, 2))
		return -CUI_EREAD;

	if (pkg->ior.io->read(&pkg->ior, &mode, 2))
		return -CUI_EREAD;

	index_buffer_length = pkg_dir->index_entries * sizeof(isa_entry_t);
	index_buffer = (DWORD *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->ior.io->read(&pkg->ior, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	if (mode & 0x8000) {
		for (unsigned int i = 0; i < index_buffer_length / 4; i++)
			index_buffer[i] ^= ~(((index_buffer_length / 4) - i) + index_buffer_length);
	}

	pkg_dir->index_entry_length = sizeof(isa_entry_t);
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;

	return 0;
}

static int Overture_MRG_parse_resource_info(struct package *pkg,
									   struct package_resource *pkg_res)
{
	isa_entry_t *isa_entry;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	isa_entry = (isa_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, isa_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length_lo = isa_entry->length;
	pkg_res->actual_data_length_lo = 0;	/* 数据都是明文 */
	pkg_res->offset_lo = isa_entry->offset;

	return 0;
}

static int Overture_MRG_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	pkg_res->raw_data = malloc(pkg_res->raw_data_length_lo);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->ior.io->readvec(&pkg->ior, pkg_res->raw_data, pkg_res->raw_data_length_lo,
		pkg_res->offset_lo, IO_SEEK_SET)) {
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EREADVEC;
	}

#if 0	// 体验版
	char *ext = PathFindExtensionA(pkg_res->name);
	if (!ext || (ext[0] != '.') || (ext[1] == '\x0')) {
		if (!memcmp(pkg_res->raw_data, "OggS", 4)) {
			pkg_res->replace_extension = _T(".ogg");
			pkg_res->flags = PKG_RES_FLAG_REEXT;
		} else if (!memcmp(pkg_res->raw_data, "RIFF", 4)) {
			pkg_res->replace_extension = _T(".wav");
			pkg_res->flags = PKG_RES_FLAG_REEXT;
		}
	}
#endif

	return 0;
}

static int Overture_MRG_save_resource(struct resource *res, 
								 struct package_resource *pkg_res)
{
	if (!res || !pkg_res)
		return -CUI_EPARA;
	
	if (res->ior.io->init(&res->ior, 0))
		return -CUI_EINIT;

	if (res->ior.io->write(&res->ior, pkg_res->raw_data, pkg_res->raw_data_length_lo)) {
		res->ior.io->finish(&res->ior);
		return -CUI_EWRITE;
	}
	res->ior.io->finish(&res->ior);
	
	return 0;
}

static void Overture_MRG_release_resource(struct package *pkg, 
									 struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void Overture_MRG_release(struct package *pkg, 
							struct package_directory *pkg_dir)
{
	if (!pkg || !pkg_dir)
		return;

	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->ior.io->finish(&pkg->ior);
}

static cui_ext_operation Overture_MRG_operation = {
	Overture_MRG_match,					/* match */
	Overture_MRG_extract_directory,		/* extract_directory */
	Overture_MRG_parse_resource_info,	/* parse_resource_info */
	Overture_MRG_extract_resource,		/* extract_resource */
	Overture_MRG_save_resource,			/* save_resource */
	Overture_MRG_release_resource,		/* release_resource */
	Overture_MRG_release					/* release */
};

int CALLBACK Overture_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".MRG"), NULL, 
		NULL, &Overture_MRG_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
