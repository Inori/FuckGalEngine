#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <zlib.h>
#include <stdio.h>

struct acui_information origin_cui_information = {
	_T("Studio e.go!"),		/* copyright */
	NULL,					/* system */
	_T(".DAT"),				/* package */
	_T("1.0.0"),			/* revision */
	_T("痴汉公贼"),			/* author */
	_T("2007-10-22 18:07"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};	

#pragma pack (1)
typedef struct {
	u8 zeroed;
	u32 samples;	// sample * 4
	u8 channels;
	u8 unknown;	// 2
	u16 sampling_rate;
	u32 length;
} ogg_header_t;
#pragma pack ()

typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 length;
	u32 flags;	
} my_DAT_entry_t;


/********************* DAT *********************/

static int origin_DAT_match(struct package *pkg)
{
	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->open(pkg->lst, IO_READONLY)) {
		pkg->pio->close(pkg);
		return -CUI_EOPEN;
	}

	return 0;	
}

static int origin_DAT_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	my_DAT_entry_t *index_buffer;
	DWORD index_entries, index_length;
	SIZE_T hed_size, DAT_size;
	BYTE *hed_buffer, *hed_p;

	if (!pkg || !pkg_dir || !pkg->lst)
		return -CUI_EPARA;
	
	if (pkg->pio->length_of(pkg->lst, &hed_size)) 
		return -CUI_ELEN;

	hed_buffer = (BYTE *)pkg->pio->read_only(pkg->lst, hed_size);
	if (!hed_buffer)
		return -CUI_EREADONLY;
	
	hed_p = hed_buffer;
	index_entries = 0;
	while (hed_p != hed_buffer + hed_size) {
		BYTE name_len = *hed_p++;
		hed_p += name_len + 4;
		index_entries++;
	}

	index_length = index_entries * sizeof(my_DAT_entry_t);
	index_buffer = (my_DAT_entry_t *)malloc(index_length);
	if (!index_buffer)
		return -CUI_EMEM;
	
	hed_p = hed_buffer;
	for (DWORD i = 0; i < index_entries; i++) {		
		BYTE name_len = *hed_p++;		
		strncpy(index_buffer[i].name, (char *)hed_p, name_len);
		for (DWORD k = 0; k < name_len; k++)
			index_buffer[i].name[k] = ~index_buffer[i].name[k];
		index_buffer[i].name[name_len] = 0;
		hed_p += name_len;
		index_buffer[i].offset = *(u32 *)hed_p;
		hed_p += 4;
	}
	
	if (pkg->pio->length_of(pkg, &DAT_size))
		return -CUI_ELEN;
	
	for (i = 0; i < index_entries - 1; i++)		
		index_buffer[i].length = index_buffer[i + 1].offset - index_buffer[i].offset;
	index_buffer[i].length = DAT_size - index_buffer[i].offset;

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_length;
	pkg_dir->index_entry_length = sizeof(my_DAT_entry_t);

	return 0;
}

static int origin_DAT_parse_resource_info(struct package *pkg,
										  struct package_resource *pkg_res)
{
	my_DAT_entry_t *my_DAT_entry;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	my_DAT_entry = (my_DAT_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_DAT_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_DAT_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_DAT_entry->offset;

	return 0;
}

static int origin_DAT_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;
	
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)pkg->pio->readvec_only(pkg, comprlen, pkg_res->offset, IO_SEEK_SET);
	if (!compr)
		return -CUI_EREADVECONLY;

	if (!strncmp((char *)compr + 13, "OggS", 4)) {
		ogg_header_t *ogg_header = (ogg_header_t *)compr;
		uncompr = (BYTE *)malloc(ogg_header->length);
		if (!uncompr)
			return -CUI_EMEM;
		
		memcpy(uncompr, ogg_header + 1, ogg_header->length);		
		uncomprlen = ogg_header->length;
		pkg_res->replace_extension = _T(".ogg");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
	} else {
		uncompr = NULL;
		uncomprlen = 0;
	}

	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}
static int origin_DAT_save_resource(struct resource *res, 
									struct package_resource *pkg_res)
{
	if (!res || !pkg_res)
		return -CUI_EPARA;
	
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

static void origin_DAT_release_resource(struct package *pkg, 
										struct package_resource *pkg_res)
{
	if (!pkg)
		return;

	if (pkg_res && pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res && pkg_res->raw_data)
		pkg_res->raw_data = NULL;
}

static void origin_DAT_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (!pkg || !pkg_dir)
		return;

	if (pkg_dir->directory && pkg_dir->directory_length) {		
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg->lst);
	pkg->pio->close(pkg);
}

static cui_ext_operation origin_DAT_operation = {
	origin_DAT_match,					/* match */
	origin_DAT_extract_directory,		/* extract_directory */
	origin_DAT_parse_resource_info,		/* parse_resource_info */
	origin_DAT_extract_resource,		/* extract_resource */
	origin_DAT_save_resource,			/* save_resource */
	origin_DAT_release_resource,		/* release_resource */
	origin_DAT_release					/* release */
};

int CALLBACK origin_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".DAT"), NULL, 
		NULL, &origin_DAT_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NO_MAGIC | CUI_EXT_FLAG_LST))
			return -1;

	return 0;
}
