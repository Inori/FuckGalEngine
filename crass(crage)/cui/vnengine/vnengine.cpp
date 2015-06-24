#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>

struct acui_information vnengine_cui_information = {
	_T(""),			/* copyright */
	_T("vnengine"),			/* system */
	_T(".axr"),				/* package */
	_T(""),			/* revision */
	_T("³Õh¹«Ù\"),			/* author */
	_T(""),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

#pragma pack (1)
typedef struct {
	s8 magic[4];	// "AXRe"
	u8 key[8];
	u32 unknown;
} axr_header_t;

#pragma pack ()


/********************* axr *********************/

static int vnengine_axr_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "AXRe", sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int vnengine_axr_extract_directory(struct package *pkg,
										  struct package_directory *pkg_dir)
{
	BYTE enc[16];
	if (pkg->pio->readvec(pkg, enc, sizeof(enc), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD key = enc[4];
	for (DWORD i = 1; i < 8; ++i)
		key ^= ((enc[i+4] >> i) | (enc[i+4] << (8-i)));	// 0x4198

	u32 EAX = *(u32 *)enc ^ *(u32 *)&enc[8];
	EAX ^= (EAX & 0xfff) << 17;
	EAX = ~((EAX >> 15 | EAX << 18) ^ EAX);
	EAX ^= (EAX & 0xfff) << 17;
	EAX = ~((EAX >> 15 | EAX << 18) ^ EAX);

	u32 version = *(u32 *)&enc[4] ^ EAX;
	EAX ^= ((EAX & 0xfff) << 17);
	u32 ECX = (EAX >> 15 | EAX << 18) ^ EAX;
	ECX = ~ECX ^ *(u32 *)&enc[12];	// 0x98

	if (version < 8)	// 0x150
		return -CUI_EMATCH;

	if ((u8)(ECX ^ key))
		return -CUI_EMATCH;

	if (ECX & 0xffffff00)
		return -CUI_EMATCH;

	printf("%x\n", ECX & 0xffffff00);exit(0);

	// Q:\ŒŽƒmŒõ‘¾—zƒm‰e‘ÌŒ±”Å
#if 0
	(ESI + 4) & ~3



	if (pkg->pio->seek(pkg, header.index_offset, IO_SEEK_SET))
		return -CUI_ESEEK;

	u32 fsize;
	pkg->pio->length_of(pkg, &fsize);

	DWORD index_len = fsize - header.index_offset;
	DAT_entry_t *index = new DAT_entry_t[header.index_entries];
	if (!index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index, index_len)) {
		delete [] index;
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = header.index_entries;
	pkg_dir->directory = index;
	pkg_dir->directory_length = index_len;
	pkg_dir->index_entry_length = sizeof(DAT_entry_t);
	package_set_private(pkg, sizeof(header));
#endif
	return 0;
}

static int vnengine_axr_parse_resource_info(struct package *pkg,
											struct package_resource *pkg_res)
{
#if 0
	DAT_entry_t *DAT_entry;

	DAT_entry = (DAT_entry_t *)pkg_res->actual_index_entry;
	strncpy(pkg_res->name, DAT_entry->name, 12);
	pkg_res->name_length = 12;
	pkg_res->raw_data_length = DAT_entry->length;
	pkg_res->actual_data_length = 0;
	DWORD offset = (DWORD)package_get_private(pkg);
	pkg_res->offset = offset;
	package_set_private(pkg, offset + DAT_entry->length);
#endif
	return 0;
}

static int vnengine_axr_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
#if 0
	BYTE *raw = new BYTE[pkg_res->raw_data_length];
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length,
			pkg_res->offset, IO_SEEK_SET)) {
		delete [] raw;
		return -CUI_EREADVEC;
	}

	if (!strncmp((char *)raw, "PW10", 4)) {
		memcpy(raw, "RIFF", 4);
		memcpy(raw + 8, "WAVE", 4);
		BYTE *data = raw + 0x2a;
		u32 data_len = *(u32 *)data;
		BYTE *dec = new BYTE[data_len * 2 + 0x2e];
		if (!dec) {
			delete [] raw;
			return -CUI_EMEM;		
		}

		u16 *p = (u16 *)(dec + 0x2e);
		for (DWORD i = 0; i < data_len; ++i)
			*p++ = wav_conv_table[*data++];
		memcpy(dec, raw, 0x2a);
		*(u32 *)(dec + 0x1c) = *(u32 *)(dec + 0x18) * 2;
		*(u32 *)(dec + 0x2a) = data_len * 2;
		*(u16 *)(dec + 0x20) = 2;	// 16 bits
		*(u16 *)(dec + 0x22) = 16;
		*(u32 *)(dec + 4) = data_len * 2 + 0x2e - 8;
		pkg_res->actual_data = dec;
		pkg_res->actual_data_length = data_len * 2 + 0x2e;
	} else if (!strncmp((char *)raw, "D1", 2) || !strncmp((char *)raw, "E1", 2)) {
		bmp_header_t *header = (bmp_header_t *)raw;
		BYTE *compr = (BYTE *)(header + 1);
		BYTE *uncompr = new BYTE[header->uncomprlen + 4096];
		if (!uncompr) {
			delete [] raw;
			return -CUI_EMEM;
		}

		bmp_decompress(uncompr, header->uncomprlen, compr);

		pkg_res->actual_data = uncompr;
		pkg_res->actual_data_length = header->uncomprlen;
	}

	pkg_res->raw_data = raw;
#endif
	return 0;
}

static int vnengine_axr_save_resource(struct resource *res, 
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

static void vnengine_axr_release_resource(struct package *pkg, 
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

static void vnengine_axr_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete []  pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

static cui_ext_operation vnengine_axr_operation = {
	vnengine_axr_match,					/* match */
	vnengine_axr_extract_directory,		/* extract_directory */
	vnengine_axr_parse_resource_info,	/* parse_resource_info */
	vnengine_axr_extract_resource,		/* extract_resource */
	vnengine_axr_save_resource,			/* save_resource */
	vnengine_axr_release_resource,		/* release_resource */
	vnengine_axr_release				/* release */
};

int CALLBACK vnengine_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".axr"), NULL, 
		NULL, &vnengine_axr_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
