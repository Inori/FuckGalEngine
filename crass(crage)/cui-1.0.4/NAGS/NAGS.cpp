#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <zlib.h>
#include <utility.h>
#include <stdio.h>

struct acui_information NAGS_cui_information = {
	NULL,						/* copyright */
	_T("NAGS"),					/* system */
	_T(".nfs"),					/* package */
	_T("1.0.0"),				/* revision */
	_T("痴漢公賊"),				/* author */
	_T("2008-5-4 13:16"),		/* date */
	NULL,						/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

#pragma pack (1)
typedef struct {
	u32 index_entries;
} nfs_header_t;

typedef struct {
	s8 name[24];
	u32 offset;
	u32 length;
} nfs_entry_t;

typedef struct {
	s8 magic[16];	/* "NGP            " */
	u8 reserved;
	u8 zlib_flag;
	u32 comprlen;
	u32 width;
	u32 height;
	u16 bpp;
	u8 unknown[0xe0];
	u32 uncomprlen;
} ngp_header_t;
#pragma pack ()


static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static int nfs_extract_ngp(ngp_header_t *ngp_header, unsigned int ngp_length,
						   BYTE **ret_uncompr, DWORD *ret_uncomprlen)
{
	BYTE *compr, *uncompr;
	DWORD act_uncomprlen;

	compr = (BYTE *)(ngp_header + 1);

	uncompr = (BYTE *)malloc(ngp_header->uncomprlen);
	if (!uncompr)
		return -CUI_EMEM;

	act_uncomprlen = ngp_header->uncomprlen;
	if (uncompress(uncompr, &act_uncomprlen, compr, ngp_header->comprlen) != Z_OK) {
		free(uncompr);
		return -CUI_EUNCOMPR;
	}
	if (act_uncomprlen != ngp_header->uncomprlen) {
		free(uncompr);
		return -CUI_EUNCOMPR;
	}

	if (MyBuildBMPFile(uncompr, act_uncomprlen, NULL, 0,
		ngp_header->width, 0 - ngp_header->height, ngp_header->bpp * 8,
			ret_uncompr, ret_uncomprlen, my_malloc)) {
				free(uncompr);
				return -CUI_EUNCOMPR;
	}
	free(uncompr);

	return 0;
}

/********************* nfs *********************/

static int NAGS_nfs_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

static int NAGS_nfs_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	BYTE *index_buffer;
	unsigned int index_buffer_length;	
	unsigned int index_entries;

	if (pkg->pio->read(pkg, &index_entries, 4))
		return -CUI_EREAD;

	index_buffer_length = index_entries * sizeof(nfs_entry_t);
	index_buffer = (BYTE *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	for (unsigned int i = 0; i < index_buffer_length; i++)
		index_buffer[i] ^= index_entries & 0xff;

	pkg_dir->index_entries = index_entries;
	pkg_dir->index_entry_length = sizeof(nfs_entry_t);
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;

	package_set_private(pkg, (void *)(index_buffer_length + sizeof(nfs_header_t)));

	return 0;
}

static int NAGS_nfs_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	unsigned int resource_base_offset;
	nfs_entry_t *nfs_entry;

	resource_base_offset = (unsigned int)package_get_private(pkg);

	nfs_entry = (nfs_entry_t *)pkg_res->actual_index_entry;
	strncpy(pkg_res->name, nfs_entry->name, 24);
	pkg_res->name_length = 24;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = nfs_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = nfs_entry->offset + resource_base_offset;

	return 0;
}

static int NAGS_nfs_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	s8 magic[16];
	int ret = 0;

	pkg_res->raw_data = malloc((unsigned int)pkg_res->raw_data_length);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EREADVEC;
	}

	if (!memcmp(((ngp_header_t *)pkg_res->raw_data)->magic, "NGP            ", 16)) {
		ret = nfs_extract_ngp((ngp_header_t *)pkg_res->raw_data, (unsigned int)pkg_res->raw_data_length,
			(BYTE **)&pkg_res->actual_data, (DWORD *)&pkg_res->actual_data_length);
		if (!ret) {
			pkg_res->replace_extension = _T(".bmp");
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
		}
	}

	for (unsigned int i = 0; i < 16; i++)
		magic[i] = ~((BYTE *)pkg_res->raw_data)[i];

	if (!memcmp(magic, "NAGS_SCRIPT_0100", 16)) {
		for (i = 0; i < pkg_res->raw_data_length; i++)
			((BYTE *)pkg_res->raw_data)[i] = ~((BYTE *)pkg_res->raw_data)[i];
	}

	return ret;
}

static int NAGS_nfs_save_resource(struct resource *res, 
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

static void NAGS_nfs_release_resource(struct package *pkg, 
									  struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}

	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void NAGS_nfs_release(struct package *pkg, 
							 struct package_directory *pkg_dir)
{
	if (!pkg || !pkg_dir)
		return;

	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

static cui_ext_operation NAGS_nfs_operation = {
	NAGS_nfs_match,					/* match */
	NAGS_nfs_extract_directory,		/* extract_directory */
	NAGS_nfs_parse_resource_info,	/* parse_resource_info */
	NAGS_nfs_extract_resource,		/* extract_resource */
	NAGS_nfs_save_resource,			/* save_resource */
	NAGS_nfs_release_resource,		/* release_resource */
	NAGS_nfs_release				/* release */
};

int CALLBACK NAGS_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".nfs"), NULL, 
		NULL, &NAGS_nfs_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	return 0;
}
