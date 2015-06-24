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

struct acui_information SchoolMate_cui_information = {
	_T("illusion"),					/* copyright */
	NULL,							/* system */
	_T(".pp"),						/* package */
	_T("0.01"),						/* revision */
	_T("痴汉公贼"),					/* author */
	_T("2007-5-30 18:47"),			/* date */
	NULL,							/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

#pragma pack (1)
typedef struct {
	u8 unknown;
	u32 index_entries;
} pp_header_t;

typedef struct {
	s8 name[260];
	u32 length;
	u32 offset;
} pp_entry_t;
#pragma pack ()		
	
static void decode(BYTE *enc_buf, DWORD enc_buf_len)
{
	u8 decode_tbl[2][8] = {
		{ 0xfa, 0x49, 0x7b, 0x1c, 0xf9, 0x4d, 0x83, 0x0a },
		{ 0x3a, 0xe3, 0x87, 0xc2, 0xbd, 0x1e, 0xa6, 0xfe }
	};
	
	for (DWORD i = 0; i < enc_buf_len; i++) {
		unsigned int idx = i & 7;
		decode_tbl[0][idx] = decode_tbl[0][idx] + decode_tbl[1][idx];
		enc_buf[i] ^= decode_tbl[0][idx];
	}
}

static void decode_resource(DWORD *enc_buf, DWORD enc_buf_len)
{
	// 体验版
	//u32 decode_tbl[8] = { 0x7735F27C, 0x6E201854, 0x859E7B9C, 0x4071B51F, 
	//	0x4371AD25, 0x7E202064, 0xC085E3CF, 0x1223DE41 };	// ver2
	u32 decode_tbl[8] = { 0xDD135D1E, 0xA74F4C7D, 0x1429A7DB, 0xBEC0F810, 
		0x63D07F44, 0x9F7C221C, 0xBEF8B9E8, 0xF4EFB358 };	// ver2
	
	unsigned int dec_len = enc_buf_len / 4;

	for (unsigned int i = 0; i < dec_len; i++)
		enc_buf[i] ^= decode_tbl[i & 7];
}

/********************* pp *********************/

static int SchoolMate_pp_match(struct package *pkg)
{	
	u8 unknown;

	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->read(pkg, &unknown, 1)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}
	
	decode(&unknown, 1);

	return 0;	
}

static int SchoolMate_pp_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{		
	unsigned int index_buffer_len;
	pp_entry_t *index_buffer;
	u32 index_entries;

	if (!pkg || !pkg_dir)
		return -CUI_EPARA;
	
	if (pkg->pio->read(pkg, &index_entries, 4))
		return -CUI_EREAD;

	decode((BYTE *)&index_entries, 4);

	index_buffer_len = index_entries * sizeof(pp_entry_t);		
	index_buffer = (pp_entry_t *)malloc(index_buffer_len);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_len)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	decode((BYTE *)index_buffer, index_buffer_len);

	pkg_dir->index_entries = index_entries;
	pkg_dir->index_entry_length = sizeof(pp_entry_t);
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_len;

	return 0;
}

static int SchoolMate_pp_parse_resource_info(struct package *pkg,
											 struct package_resource *pkg_res)
{
	pp_entry_t *pp_entry;
	
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	pp_entry = (pp_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, pp_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = pp_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = pp_entry->offset;

	return 0;
}

static int SchoolMate_pp_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{	
	//u8 decode_tbl2[8] = { 0x6b, 0x42, 0x6c, 0xa2, 0x72, 0x30, 0x7d, 0x22 };
	BYTE *data;
	
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	data = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, data, pkg_res->raw_data_length,	pkg_res->offset, IO_SEEK_SET)) {
		free(data);
		return -CUI_EREADVEC;
	}

	decode_resource((DWORD *)data, pkg_res->raw_data_length);

	pkg_res->raw_data = data;
	pkg_res->actual_data = NULL;
	pkg_res->actual_data_length = pkg_res->raw_data_length;	

	return 0;
}

static int SchoolMate_pp_save_resource(struct resource *res, 
									   struct package_resource *pkg_res)
{
	if (!res || !pkg_res)
		return -CUI_EPARA;
	
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}
	res->rio->close(res);
	
	return 0;
}

static void SchoolMate_pp_release_resource(struct package *pkg, 
										   struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void SchoolMate_pp_release(struct package *pkg, 
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

static cui_ext_operation SchoolMate_pp_operation = {
	SchoolMate_pp_match,				/* match */
	SchoolMate_pp_extract_directory,	/* extract_directory */
	SchoolMate_pp_parse_resource_info,	/* parse_resource_info */
	SchoolMate_pp_extract_resource,		/* extract_resource */
	SchoolMate_pp_save_resource,		/* save_resource */
	SchoolMate_pp_release_resource,		/* release_resource */
	SchoolMate_pp_release				/* release */
};
	
int CALLBACK SchoolMate_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".pp"), NULL, 
		NULL, &SchoolMate_pp_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	return 0;
}
