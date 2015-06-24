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

struct acui_information NitroPlus_cui_information = {
	_T("株式会社ニトロプラス"),	/* copyright */
	_T("Nirtroplus system"),	/* system */
	_T(".pak"),					/* package */
	_T("1.0.3"),				/* revision */
	_T("痴漢公賊"),				/* author */
	_T("2009-6-21 12:51"),		/* date */
	NULL,						/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

#pragma pack (1)
typedef struct {
	u32 version;	/* 1 */
	u32 index_entries;
	u32 uncomprlen;
	u32 comprlen;
} pak1_header_t;

typedef struct {
	u32 version;	/* 2 */
	u32 index_entries;
	u32 uncomprlen;
	u32 comprlen;
	u32 unknown1;	/* 0 or 0x100(和is_compr有关？) */	
	s8 magic[256];
} pak2_header_t;

typedef struct {
	u32 version;	/* 3 */
	u32 index_entries;
	u32 uncomprlen;
	u32 comprlen;
	u32 unknown[2];
	s8 magic[256];
} pak3_header_t;

typedef struct {
	u32 version;	/* 4 */
	s8 magic[256];
	u32 xor_factor;
	u32 uncomprlen;
	u32 index_entries;
	u32 comprlen;
} pak4_header_t;
#pragma pack ()


static int make_xor_code(s8 *magic)
{
	int code = 0;
	int chr;

	while ((chr = *magic)) {
		code = code * 0x89 + chr;
		magic++;
	}

	return code;
}

static void xor_decode(int xor_code, BYTE *dec, BYTE *enc, unsigned int len)
{
	for (unsigned int i = 0; i < len; i++)
		dec[i] = enc[i] ^ ((xor_code >> ((i & 3) * 8)) & 0xff);
}

static int pak1_decompress_index(struct package *pkg, struct package_directory *pkg_dir)
{
	pak1_header_t *pak_header;
	DWORD act_uncomprlen;
	BYTE *uncompr, *compr;

	pak_header = (pak1_header_t *)malloc(sizeof(*pak_header));
	if (!pak_header)
		return -CUI_EMEM;
	
	if (pkg->pio->read(pkg, pak_header, sizeof(*pak_header))) {
		free(pak_header);
		return -CUI_EREAD;
	}

	compr = (BYTE *)malloc(pak_header->comprlen);
	if (!compr) {
		free(pak_header);
		return -CUI_EMEM;
	}

	uncompr = (BYTE *)malloc(pak_header->uncomprlen);
	if (!uncompr) {
		free(uncompr);
		free(pak_header);
		return -CUI_EMEM;
	}

	if (pkg->pio->read(pkg, compr, pak_header->comprlen)) {
		free(uncompr);
		free(compr);
		free(pak_header);
		return -CUI_EREAD;
	}

	act_uncomprlen = pak_header->uncomprlen;
	if (uncompress(uncompr, &act_uncomprlen, compr, pak_header->comprlen) != Z_OK) {
		free(uncompr);
		free(compr);
		free(pak_header);
		return -CUI_EUNCOMPR;
	}
	free(compr);
	if (act_uncomprlen != pak_header->uncomprlen) {
		free(uncompr);
		free(pak_header);
		return -CUI_EUNCOMPR;
	}
	pkg_dir->index_entries = pak_header->index_entries;
	pkg_dir->directory = uncompr;
	pkg_dir->directory_length = act_uncomprlen;
	pkg_dir->flags = PKG_DIR_FLAG_VARLEN | PKG_DIR_FLAG_SKIP0;

	package_set_private(pkg, pak_header);

	return 0;
}

static int pak2_decompress_index(struct package *pkg, struct package_directory *pkg_dir)
{
	pak2_header_t *pak_header;
	DWORD act_uncomprlen;
	BYTE *uncompr, *compr;

	pak_header = (pak2_header_t *)malloc(sizeof(*pak_header));
	if (!pak_header)
		return -CUI_EMEM;
	
	if (pkg->pio->read(pkg, pak_header, sizeof(*pak_header))) {
		free(pak_header);
		return -CUI_EREAD;
	}
//if (pak_header->unknown1)
//	printf("%d\n", pak_header->unknown1);
	compr = (BYTE *)malloc(pak_header->comprlen);
	if (!compr) {
		free(pak_header);
		return -CUI_EMEM;
	}

	uncompr = (BYTE *)malloc(pak_header->uncomprlen);
	if (!uncompr) {
		free(uncompr);
		free(pak_header);
		return -CUI_EMEM;
	}

	if (pkg->pio->read(pkg, compr, pak_header->comprlen)) {
		free(uncompr);
		free(compr);
		free(pak_header);
		return -CUI_EREAD;
	}

	act_uncomprlen = pak_header->uncomprlen;
	if (uncompress(uncompr, &act_uncomprlen, compr, pak_header->comprlen) != Z_OK) {
		free(uncompr);
		free(compr);
		free(pak_header);
		return -CUI_EUNCOMPR;
	}
	free(compr);
	if (act_uncomprlen != pak_header->uncomprlen) {
		free(uncompr);
		free(pak_header);
		return -CUI_EUNCOMPR;
	}
	pkg_dir->index_entries = pak_header->index_entries;
	pkg_dir->directory = uncompr;
	pkg_dir->directory_length = act_uncomprlen;
	pkg_dir->flags = PKG_DIR_FLAG_VARLEN | PKG_DIR_FLAG_SKIP0;

	package_set_private(pkg, pak_header);

	return 0;
}

static int pak3_decompress_index(struct package *pkg, struct package_directory *pkg_dir)
{
	pak3_header_t *pak_header;
	DWORD act_uncomprlen;
	BYTE *uncompr, *compr;

	pak_header = (pak3_header_t *)malloc(sizeof(*pak_header));
	if (!pak_header)
		return -CUI_EMEM;
	
	if (pkg->pio->read(pkg, pak_header, sizeof(*pak_header))) {
		free(pak_header);
		return -CUI_EREAD;
	}

	compr = (BYTE *)malloc(pak_header->comprlen);
	if (!compr) {
		free(pak_header);
		return -CUI_EMEM;
	}

	uncompr = (BYTE *)malloc(pak_header->uncomprlen);
	if (!uncompr) {
		free(uncompr);
		free(pak_header);
		return -CUI_EMEM;
	}

	if (pkg->pio->read(pkg, compr, pak_header->comprlen)) {
		free(uncompr);
		free(compr);
		free(pak_header);
		return -CUI_EREAD;
	}

	act_uncomprlen = pak_header->uncomprlen;
	if (uncompress(uncompr, &act_uncomprlen, compr, pak_header->comprlen) != Z_OK) {
		free(uncompr);
		free(compr);
		free(pak_header);
		return -CUI_EUNCOMPR;
	}
	free(compr);
	if (act_uncomprlen != pak_header->uncomprlen) {
		free(uncompr);
		free(pak_header);
		return -CUI_EUNCOMPR;
	}
	pkg_dir->index_entries = pak_header->index_entries;
	pkg_dir->directory = uncompr;
	pkg_dir->directory_length = act_uncomprlen;
	pkg_dir->flags = PKG_DIR_FLAG_VARLEN | PKG_DIR_FLAG_SKIP0;

	package_set_private(pkg, pak_header);

	return 0;
}

static int pak4_decompress_index(struct package *pkg, struct package_directory *pkg_dir)
{
	pak4_header_t *pak_header;
	u32 uncomprlen, index_entries;
	DWORD comprlen, act_uncomprlen;
	BYTE *uncompr, *compr;
	int xor_code;

	pak_header = (pak4_header_t *)malloc(sizeof(*pak_header));
	if (!pak_header)
		return -CUI_EMEM;
	
	if (pkg->pio->read(pkg, pak_header, sizeof(*pak_header))) {
		free(pak_header);
		return -CUI_EREAD;
	}

	xor_code = make_xor_code(pak_header->magic);
	xor_decode(xor_code, (BYTE *)&uncomprlen, (BYTE *)&pak_header->uncomprlen, 4);
	xor_decode(xor_code, (BYTE *)&index_entries, (BYTE *)&pak_header->index_entries, 4);	
	xor_decode(pak_header->xor_factor, (BYTE *)&comprlen, (BYTE *)&pak_header->comprlen, 4);
	pak_header->comprlen = comprlen;	/* 为了后面计算资源文件偏移 */
	
	compr = (BYTE *)malloc(comprlen);
	if (!compr) {
		free(pak_header);
		return -CUI_EMEM;
	}

	uncompr = (BYTE *)malloc(uncomprlen);
	if (!uncompr) {
		free(uncompr);
		free(pak_header);
		return -CUI_EMEM;
	}

	if (pkg->pio->read(pkg, compr, comprlen)) {
		free(uncompr);
		free(compr);
		free(pak_header);
		return -CUI_EREAD;
	}

	act_uncomprlen = uncomprlen;
	if (uncompress(uncompr, &act_uncomprlen, compr, comprlen) != Z_OK) {
		free(uncompr);
		free(compr);
		free(pak_header);
		return -CUI_EUNCOMPR;
	}
	free(compr);
	if (act_uncomprlen != uncomprlen) {
		free(uncompr);
		free(pak_header);
		return -CUI_EUNCOMPR;
	}
	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = uncompr;
	pkg_dir->directory_length = act_uncomprlen;
	pkg_dir->flags = PKG_DIR_FLAG_VARLEN | PKG_DIR_FLAG_SKIP0;

	package_set_private(pkg, pak_header);

	return 0;
}

/********************* pak *********************/

static int NitroPlus_pak_match(struct package *pkg)
{
	u32 version;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &version, 4)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (version != 1 && version != 2 && version != 3 && version != 4) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int NitroPlus_pak_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{
	u32 version;
	int ret = -1;

	if (pkg->pio->readvec(pkg, &version, 4, 0, IO_SEEK_SET)) 
		return -CUI_EREADVEC;

	if (pkg->pio->seek(pkg, 0, IO_SEEK_SET)) 
		return -CUI_ESEEK;

	if (version == 1)
		ret = pak1_decompress_index(pkg, pkg_dir);
	else if (version == 2)
		ret = pak2_decompress_index(pkg, pkg_dir);
	else if (version == 3) {
		ret = pak3_decompress_index(pkg, pkg_dir);
		if (ret) {	// 刃鸣散
			version = 4;
			if (pkg->pio->seek(pkg, 0, IO_SEEK_SET)) 
				return -CUI_ESEEK;
		}
	}
	if (version == 4) {
		ret = pak4_decompress_index(pkg, pkg_dir);
		if (!ret) {	// 刃鸣散
			pak4_header_t *pak_header = (pak4_header_t *)package_get_private(pkg);
			pak_header->version = 4;
		}
	}

	return ret;
}

static int NitroPlus_pak_parse_resource_info(struct package *pkg,
											 struct package_resource *pkg_res)
{
	BYTE *pak_entry_p;
	u32 version, name_length, offset, uncomprlen, comprlen, is_compr, act_index_len;

	version = *(u32 *)package_get_private(pkg);
	pak_entry_p = (BYTE *)pkg_res->actual_index_entry;
	if (version == 4) {
		pak4_header_t *pak_header;
		int xor_code;
		u32 next_offset;	/* 下一个索引项在dir中的偏移 */

		name_length = *(u32 *)pak_entry_p;
		pak_entry_p += 4;
		strncpy(pkg_res->name, (char *)pak_entry_p, name_length);
		pkg_res->name[name_length] = 0;
		pak_entry_p += name_length;
		xor_code = make_xor_code(pkg_res->name);
		xor_decode(xor_code, (BYTE *)&offset, (BYTE *)pak_entry_p, 4);	// OK
		pak_entry_p += 4;
		xor_decode(xor_code, (BYTE *)&uncomprlen, (BYTE *)pak_entry_p, 4);
		pak_entry_p += 4;
		xor_decode(xor_code, (BYTE *)&next_offset, (BYTE *)pak_entry_p, 4);
		pak_entry_p += 4;
		xor_decode(xor_code, (BYTE *)&is_compr, (BYTE *)pak_entry_p, 4);
		pak_entry_p += 4;
		xor_decode(xor_code, (BYTE *)&comprlen, (BYTE *)pak_entry_p, 4);

		pak_header = (pak4_header_t *)package_get_private(pkg);
		offset += pak_header->comprlen + sizeof(pak4_header_t);
		act_index_len = 4 + name_length + 4 * 5;
		comprlen = is_compr ? comprlen : uncomprlen;
		uncomprlen = !is_compr ? 0 : uncomprlen;
	} else if (version == 2 || version == 1) {
		u32 act_length;

		name_length = *(u32 *)pak_entry_p;
		pak_entry_p += 4;
		strncpy(pkg_res->name, (char *)pak_entry_p, name_length);
		pkg_res->name[name_length] = 0;
		pak_entry_p += name_length;
		offset = *(u32 *)pak_entry_p;
		pak_entry_p += 4;
		uncomprlen = *(u32 *)pak_entry_p;
		pak_entry_p += 4;
		act_length = *(u32 *)pak_entry_p;
		pak_entry_p += 4;		
		is_compr = *(u32 *)pak_entry_p;
		pak_entry_p += 4;
		comprlen = *(u32 *)pak_entry_p;

		/* 对于is_compr为0的情况，comprlen和uncomprlen都为0 */
		if (version == 2) {
			pak2_header_t *pak_header = (pak2_header_t *)package_get_private(pkg);
			offset += pak_header->comprlen + sizeof(pak2_header_t);
		} else {
			pak1_header_t *pak_header = (pak1_header_t *)package_get_private(pkg);
			offset += pak_header->comprlen + sizeof(pak1_header_t);
		}

		act_index_len = 4 + name_length + 4 * 5;
		comprlen = is_compr ? comprlen : act_length;
		uncomprlen = is_compr ? uncomprlen : 0;
	} else if (version == 3) {
		pak3_header_t *pak_header;
		u32 act_length;

		name_length = *(u32 *)pak_entry_p;
		pak_entry_p += 4;
		strncpy(pkg_res->name, (char *)pak_entry_p, name_length);
		pkg_res->name[name_length] = 0;
		pak_entry_p += name_length;
		offset = *(u32 *)pak_entry_p;
		pak_entry_p += 4;
		uncomprlen = *(u32 *)pak_entry_p;
		pak_entry_p += 4;
		act_length = *(u32 *)pak_entry_p;
		pak_entry_p += 4;		
		is_compr = *(u32 *)pak_entry_p;
		pak_entry_p += 4;
		comprlen = *(u32 *)pak_entry_p;
	
		/* 对于is_compr为0的情况，comprlen和uncomprlen都为0 */
		pak_header = (pak3_header_t *)package_get_private(pkg);
		offset += pak_header->comprlen + sizeof(pak3_header_t);
		act_index_len = 4 + name_length + 4 * 5;
		comprlen = is_compr ? comprlen : act_length;
		uncomprlen = is_compr ? uncomprlen : 0;
	}

	pkg_res->actual_index_entry_length = act_index_len;
	pkg_res->name_length = name_length;
	pkg_res->raw_data_length = comprlen;
	pkg_res->actual_data_length = uncomprlen;
	pkg_res->offset = offset;

	return 0;
}

static int NitroPlus_pak_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	u32 version;
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;

	comprlen = (DWORD)pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	uncomprlen = pkg_res->actual_data_length;
	if (uncomprlen) {
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}
	} else
		uncompr = NULL;

	if (pkg->pio->readvec(pkg, compr, comprlen, pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	version = *(u32 *)package_get_private(pkg);
	if (version == 4) {
		BYTE *dec_buf;
		DWORD dec_buf_len;

		if (uncomprlen) {
			DWORD act_uncomprlen = uncomprlen;
			if (uncompress(uncompr, &act_uncomprlen, compr, comprlen) != Z_OK) {
				free(uncompr);
				free(compr);
				return -CUI_EUNCOMPR;
			}
			free(compr);
			compr = NULL;
			if (act_uncomprlen != uncomprlen) {
				free(uncompr);
				return -CUI_EUNCOMPR;
			}
			dec_buf = uncompr;
			dec_buf_len = act_uncomprlen;
		} else {
			dec_buf = compr;
			dec_buf_len = comprlen;
		}

		int xor_code = make_xor_code(pkg_res->name);
		if (uncomprlen)
			xor_decode(xor_code, dec_buf, dec_buf, dec_buf_len);
		else
			xor_decode(xor_code, dec_buf, dec_buf, dec_buf_len < 1024 ? dec_buf_len : 1024);

		pkg_res->raw_data = NULL;
		pkg_res->raw_data_length = comprlen;
		pkg_res->actual_data = dec_buf;
		pkg_res->actual_data_length = dec_buf_len;	
	} else if (version == 1 || version == 2 || version == 3) {
		if (uncomprlen) {
			DWORD act_uncomprlen = uncomprlen;
			if (uncompress(uncompr, &act_uncomprlen, compr, comprlen) != Z_OK) {
				free(uncompr);
				free(compr);
				return -CUI_EUNCOMPR;
			}
			free(compr);
			compr = NULL;
			if (act_uncomprlen != uncomprlen) {
				free(uncompr);
				return -CUI_EUNCOMPR;
			}
			pkg_res->actual_data = uncompr;
			pkg_res->actual_data_length = act_uncomprlen;
		} else {
			pkg_res->actual_data = compr;
			pkg_res->actual_data_length = comprlen;
		}	
		pkg_res->raw_data = NULL;
		pkg_res->raw_data_length = comprlen;
	}

	return 0;
}

static int NitroPlus_pak_save_resource(struct resource *res, 
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

static void NitroPlus_pak_release_resource(struct package *pkg, 
										   struct package_resource *pkg_res)
{
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}

	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
}

static void NitroPlus_pak_release(struct package *pkg, 
								  struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

static cui_ext_operation NitroPlus_pak_operation = {
	NitroPlus_pak_match,				/* match */
	NitroPlus_pak_extract_directory,	/* extract_directory */
	NitroPlus_pak_parse_resource_info,	/* parse_resource_info */
	NitroPlus_pak_extract_resource,		/* extract_resource */
	NitroPlus_pak_save_resource,		/* save_resource */
	NitroPlus_pak_release_resource,		/* release_resource */
	NitroPlus_pak_release				/* release */
};

int CALLBACK NitroPlus_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".pak"), NULL, 
		NULL, &NitroPlus_pak_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR 
		| CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	return 0;
}
