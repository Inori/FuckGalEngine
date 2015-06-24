#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <resource.h>
#include <package.h>
#include <cui_error.h>
#include <utility.h>
#include <stdio.h>

struct acui_information Stuff_cui_information = {
	_T("©Z¥¦¥£¥ë"),			/* copyright */
	_T("Stuff ¥¹¥¯¥ê¥×¥È¥¨¥ó¥¸¥ó"),					/* system */
	_T(".mpk .msc"),		/* package */
	_T("1.0.2"),			/* revision */
	_T("³Õºº¹«Ôô"),			/* author */
	_T("2007-11-6 20:26"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

#pragma pack (1)
typedef struct {
	u32 index_offset;
	u32 index_entries;
} mpk_header_t;

typedef struct {
	s8 name[32];
	u32 offset;
	u32 length;
} mpk_entry_t;

typedef struct {
	u16 index_entries;	// if 1, alone package
} mgr_header_t;

typedef struct {
	u32 offset;
} mgr_entry_t;

typedef struct {
	u32 uncomprlen;
	u32 comprlen;
} mgr_info_t;
#pragma pack ()	

typedef struct {
	s8 name[32];
	u32 offset;
	u32 comprlen;
	u32 uncomprlen;
} my_mgr_entry_t;	

static unsigned int mgr_rle_decompress(BYTE *uncompr, DWORD uncomprlen, BYTE *compr, DWORD comprlen)
{
	DWORD curbyte = 0;
	DWORD act_uncomprlen = 0;

	while (1) {
		u8 code;
		unsigned int copy_bytes;
		BYTE *src, *dst;

		if (curbyte >= comprlen || act_uncomprlen >= uncomprlen)
			break;

		code = compr[curbyte++];
		if (code < 32) {
			copy_bytes = code + 1;
			if (act_uncomprlen + copy_bytes > uncomprlen)
				goto out;

			src = &compr[curbyte];
			dst = &uncompr[act_uncomprlen];
			curbyte += copy_bytes;
			act_uncomprlen += copy_bytes;
		} else {
			DWORD offset = (code & 0x1f) << 8;

			src = &uncompr[act_uncomprlen - (offset + 1)];
			copy_bytes = code >> 5;

			if (copy_bytes == 7)
				copy_bytes = compr[curbyte++] + 7;
			src -= compr[curbyte++];
			dst = &uncompr[act_uncomprlen];
			copy_bytes += 2;

			if (act_uncomprlen + copy_bytes > uncomprlen)
				goto out;

			if (src < uncompr)
				goto out;
			act_uncomprlen += copy_bytes;
		}

		for (unsigned int i = 0; i < copy_bytes; i++)
			*dst++ = *src++;
	}
out:
	return act_uncomprlen;
}

/********************* mpk *********************/

static int Stuff_mpk_match(struct package *pkg)
{	
	u32 magic;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (magic == 0xba010000) {	// MPEG-1 PS
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->pio->seek(pkg, 0, SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_ESEEK;
	}

	return 0;	
}

static int Stuff_mpk_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{		
	mpk_header_t mpk;
	unsigned int index_buffer_len;
	BYTE *index_buffer;

	if (!pkg || !pkg_dir)
		return -CUI_EPARA;
	
	if (pkg->pio->read(pkg, &mpk, sizeof(mpk)))
		return -CUI_EREAD;

	index_buffer_len = mpk.index_entries * sizeof(mpk_entry_t);
	index_buffer = (BYTE *)malloc(index_buffer_len);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, index_buffer, index_buffer_len, mpk.index_offset, IO_SEEK_SET)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	for (unsigned int i = 0; i < index_buffer_len; i++)
		index_buffer[i] ^= 0x58;

	pkg_dir->index_entries = mpk.index_entries;
	pkg_dir->index_entry_length = sizeof(mpk_entry_t);
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_len;

	return 0;
}

static int Stuff_mpk_parse_resource_info(struct package *pkg,
												 struct package_resource *pkg_res)
{
	mpk_entry_t *mpk_entry;
	
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	mpk_entry = (mpk_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, mpk_entry->name);
	pkg_res->name_length = 32;
	pkg_res->raw_data_length = mpk_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = mpk_entry->offset;

	return 0;
}

static int Stuff_mpk_extract_resource(struct package *pkg,
											  struct package_resource *pkg_res)
{	
	BYTE *data, *act_data = NULL;
	unsigned int act_data_len = 0;
	
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	data = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, data, pkg_res->raw_data_length,	pkg_res->offset, IO_SEEK_SET)) {
		free(data);
		return -CUI_EREADVEC;
	}

	char *ext = PathFindExtensionA(pkg_res->name);
	if (ext && !strcmp(ext, ".msc")) {
		for (unsigned int i = 0; i < pkg_res->raw_data_length; i++)
			data[i] ^= 0x88;
	} else if (ext && !strcmp(ext, ".mgr")) {
		u16 index_entries = *(u16 *)data;
		if (index_entries == 1) {
			unsigned int act_uncomprlen;

			act_data_len = *(u32 *)(data + 2);
			act_data = (BYTE *)malloc(act_data_len);
			if (!act_data) {
				free(data);
				return -CUI_EMEM;
			}
			memset(act_data, 0, act_data_len);

			act_uncomprlen = mgr_rle_decompress(act_data, act_data_len, data + 10, *(u32 *)(data + 6));
			if (act_uncomprlen != act_data_len) {
				free(act_data);
				free(data);
				return -CUI_EUNCOMPR;
			}
			pkg_res->replace_extension = _T(".bmp");
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
		}
	} else if (!memcmp(data, "OggS", 4)) {
		pkg_res->replace_extension = _T(".ogg");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
	}
	pkg_res->raw_data = data;
	pkg_res->actual_data = act_data;
	pkg_res->actual_data_length = act_data_len;	

	return 0;
}

static int Stuff_mpk_save_resource(struct resource *res, 
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

static void Stuff_mpk_release_resource(struct package *pkg, 
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

static void Stuff_mpk_release(struct package *pkg, 
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

static cui_ext_operation Stuff_mpk_operation = {
	Stuff_mpk_match,				/* match */
	Stuff_mpk_extract_directory,	/* extract_directory */
	Stuff_mpk_parse_resource_info,/* parse_resource_info */
	Stuff_mpk_extract_resource,	/* extract_resource */
	Stuff_mpk_save_resource,		/* save_resource */
	Stuff_mpk_release_resource,	/* release_resource */
	Stuff_mpk_release				/* release */
};

/********************* mgr *********************/

static int Stuff_mgr_match(struct package *pkg)
{	
	mgr_header_t mgr_header;
	
	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &mgr_header, sizeof(mgr_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (mgr_header.index_entries == 1) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->pio->seek(pkg, 0, IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_ESEEK;
	}

	return 0;	
}

static int Stuff_mgr_extract_directory(struct package *pkg,
											   struct package_directory *pkg_dir)
{		
	mgr_header_t mgr;
	unsigned int index_buffer_len;
	my_mgr_entry_t *index_buffer;
	mgr_info_t mgr_info;

	if (!pkg || !pkg_dir)
		return -CUI_EPARA;

	if (pkg->pio->read(pkg, &mgr, sizeof(mgr)))
		return -CUI_EREAD;

	index_buffer_len = mgr.index_entries * sizeof(my_mgr_entry_t);
	index_buffer = (my_mgr_entry_t *)malloc(index_buffer_len);
	if (!index_buffer)
		return -CUI_EMEM;

	if (mgr.index_entries != 1) {
		mgr_entry_t *entry_index = (mgr_entry_t *)malloc(mgr.index_entries * sizeof(mgr_entry_t));
		if (!entry_index) {
			free(index_buffer);
			return -CUI_EMEM;
		}

		if (pkg->pio->read(pkg, entry_index, mgr.index_entries * sizeof(mgr_entry_t))) {
			free(entry_index);
			free(index_buffer);
			return -CUI_EREAD;
		}

		for (int i = 0; i < mgr.index_entries; i++) {
			if (pkg->pio->readvec(pkg, &mgr_info, sizeof(mgr_info), entry_index[i].offset, IO_SEEK_SET))
				break;
			index_buffer[i].offset = entry_index[i].offset + sizeof(mgr_info);
			index_buffer[i].comprlen = mgr_info.comprlen;
			index_buffer[i].uncomprlen = mgr_info.uncomprlen;
			sprintf(index_buffer[i].name, "%06d", i);
		}
		free(entry_index);
		if (i != mgr.index_entries) {
			free(index_buffer);
			return -CUI_EREADVEC;
		}
	} else {
		if (pkg->pio->read(pkg, &mgr_info, sizeof(mgr_info))) {
			free(index_buffer);
			return -CUI_EREAD;
		}
		index_buffer[0].offset = 2 + sizeof(mgr_info);
		index_buffer[0].comprlen = mgr_info.comprlen;
		index_buffer[0].uncomprlen = mgr_info.uncomprlen;
		strcpy(index_buffer[0].name, "000000");
	}
	pkg_dir->index_entries = mgr.index_entries;
	pkg_dir->index_entry_length = sizeof(my_mgr_entry_t);
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_len;

	return 0;
}

static int Stuff_mgr_parse_resource_info(struct package *pkg,
												 struct package_resource *pkg_res)
{
	my_mgr_entry_t *mgr_entry;
	
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	mgr_entry = (my_mgr_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, mgr_entry->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = mgr_entry->comprlen;
	pkg_res->actual_data_length = mgr_entry->uncomprlen;
	pkg_res->offset = mgr_entry->offset;

	return 0;
}

static int Stuff_mgr_extract_resource(struct package *pkg,
											  struct package_resource *pkg_res)
{	
	BYTE *data, *act_data;
	DWORD act_data_len;
	
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	data = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, data, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET)) {
		free(data);
		return -CUI_EREADVEC;
	}

	act_data = (BYTE *)malloc(pkg_res->actual_data_length);
	if (!act_data) {
		free(data);
		return -CUI_EMEM;
	}
	memset(act_data, 0, pkg_res->actual_data_length);

	act_data_len = mgr_rle_decompress(act_data, pkg_res->actual_data_length, data, pkg_res->raw_data_length);
	if (act_data_len != pkg_res->actual_data_length) {
		free(act_data);
		return -CUI_EUNCOMPR;
	}
	pkg_res->raw_data = data;
	pkg_res->actual_data = act_data;

	return 0;
}

static int Stuff_mgr_save_resource(struct resource *res, 
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

static void Stuff_mgr_release_resource(struct package *pkg, 
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

static void Stuff_mgr_release(struct package *pkg, 
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

static cui_ext_operation Stuff_mgr_operation = {
	Stuff_mgr_match,				/* match */
	Stuff_mgr_extract_directory,	/* extract_directory */
	Stuff_mgr_parse_resource_info,	/* parse_resource_info */
	Stuff_mgr_extract_resource,		/* extract_resource */
	Stuff_mgr_save_resource,		/* save_resource */
	Stuff_mgr_release_resource,		/* release_resource */
	Stuff_mgr_release				/* release */
};

/********************* msc *********************/

static int Stuff_msc_match(struct package *pkg)
{	
	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

static int Stuff_msc_extract_resource(struct package *pkg,
											  struct package_resource *pkg_res)
{	
	BYTE *data;
	u32 file_len;

	if (pkg->pio->length_of(pkg, &file_len))
		return -CUI_ELEN;

	data = (BYTE *)malloc(file_len);
	if (!data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, data, file_len, 0, IO_SEEK_SET)) {
		free(data);
		return -CUI_EREADVEC;
	}

	for (DWORD i = 0; i < file_len; i++)
		data[i] ^= 0x88;

	pkg_res->raw_data = data;
	pkg_res->raw_data_length = file_len;	

	return 0;
}

static int Stuff_msc_save_resource(struct resource *res, 
										   struct package_resource *pkg_res)
{
	if (!res || !pkg_res)
		return -CUI_EPARA;
	
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (pkg_res->raw_data && pkg_res->raw_data_length) {
		if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
			res->rio->close(res);
			return -CUI_EWRITE;
		}
	}

	res->rio->close(res);
	
	return 0;
}

static void Stuff_msc_release_resource(struct package *pkg, 
											   struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void Stuff_msc_release(struct package *pkg, 
									  struct package_directory *pkg_dir)
{
	if (!pkg || !pkg_dir)
		return;

	pkg->pio->close(pkg);
}

static cui_ext_operation Stuff_msc_operation = {
	Stuff_msc_match,				/* match */
	NULL,									/* extract_directory */
	NULL,									/* parse_resource_info */
	Stuff_msc_extract_resource,		/* extract_resource */
	Stuff_msc_save_resource,		/* save_resource */
	Stuff_msc_release_resource,		/* release_resource */
	Stuff_msc_release				/* release */
};
	
int CALLBACK Stuff_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".mpk"), NULL, 
		NULL, &Stuff_mpk_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_SUFFIX_SENSE))
			return -1;

	if (callback->add_extension(callback->cui, _T(".mgr"), _T(".bmp"), 
		NULL, &Stuff_mgr_operation, CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".msc"), NULL, 
		NULL, &Stuff_msc_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	return 0;
}
