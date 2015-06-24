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

struct acui_information InnocentGrey_cui_information = {
	_T("Innocent Grey"),	/* copyright */
	NULL,					/* system */
	_T(".dat .s .d"),		/* package */
	_T("0.9.3"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-7-30 10:18"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

#pragma pack (1)
typedef struct {
	s8 magic[8];		/* "PACKDAT." */
	u32 dummy_index_entries;
	u32 index_entries;
} dat_header_t;

typedef struct {
	s8 name[32];
	u32 offset;
	u32 mode;		/* BMP: 0x20000000 */
	u32 uncomprlen;
	u32 comprlen;
} dat_entry_t;
#pragma pack ()

/********************* dat *********************/

static int InnocentGrey_dat_match(struct package *pkg)
{
	s8 magic[8];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (strncmp(magic, "PACKDAT.", 8)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int InnocentGrey_dat_extract_directory(struct package *pkg,
											  struct package_directory *pkg_dir)
{
	dat_header_t dat_header;
	DWORD index_buffer_length;
	dat_entry_t *index_buffer;

	if (pkg->pio->readvec(pkg, &dat_header, sizeof(dat_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	index_buffer_length = dat_header.index_entries * sizeof(dat_entry_t);
	index_buffer = (dat_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = dat_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(dat_entry_t);

	return 0;
}

static int InnocentGrey_dat_parse_resource_info(struct package *pkg,
												struct package_resource *pkg_res)
{
	dat_entry_t *dat_entry;	

	dat_entry = (dat_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, dat_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = dat_entry->comprlen;
	pkg_res->actual_data_length = dat_entry->uncomprlen;	/* 数据都是明文 */
	pkg_res->offset = dat_entry->offset;

	return 0;
}

#if 0
static int InnocentGrey_dat_extract_resource(struct package *pkg,
											 struct package_resource *pkg_res)
{
	dat_entry_t *dat_entry;
	u32 magic;
	u32 *enc_buf;

	pkg_res->raw_data = malloc(pkg_res->raw_data_length);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, 
			pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET)) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
		return -CUI_EREADVEC;
	}

	dat_entry = (dat_entry_t *)pkg_res->actual_index_entry;
	magic = dat_entry->comprlen;
	enc_buf = (u32 *)pkg_res->raw_data;
	/* TODO:
	00404DFE   . /0F84 43040000 JE 和み匣.00405247                          ;  if (!(type & 0xffffff))
	00404E04   . |8B4424 50     MOV EAX,DWORD PTR SS:[ESP+50]            ;  comprlen
	00404E08   . |8B53 04       MOV EDX,DWORD PTR DS:[EBX+4]             ;  compr
	00404E0B   . |55            PUSH EBP
	00404E0C   . |8BE8          MOV EBP,EAX                              ;  comprlen
	*/
#if 0
	if (dat_entry->mode & 0xffffff) {
		if (dat_entry->mode & 0x10000) {
			DWORD xor = dat_entry->raw_data_length;
			xor >>= 2;
			xor = (xor << ((xor & 7) + 8)) ^ xor;

			for (DWORD i = 0; i < pkg_res->raw_data_length / 4; i++) {
				u8 shifts;

				*enc_buf ^= xor;
				shifts = *enc_buf++ % 24;
				xor = (xor << shifts) | (xor >> (32 - shifts));
			}

			if (mode & 1) {

			}

			if (mode & 2) {

			}

			if (mode & 0x200) {

			}

			if (mode & 0x100) {

			}

		}
		_stprintf(fmt_buf, _T("%s: unsupported mode %x\n"), 
			pkg->name, dat_entry->mode);
		wcprintf_error(fmt_buf);
		exit(0);
	}
#endif

//	char *ext = PathFindExtensionA(pkg_res->name);
//	if (!(pkg_res->flags & PKG_RES_FLAG_RAW) && !lstrcmpiA(ext, ".s")) {
//		for (unsigned int i = 0; i < pkg_res->raw_data_length; i++)
//			((u8 *)pkg_res->raw_data)[i] = ~((u8 *)pkg_res->raw_data)[i];
//	}

	return 0;
}
#endif

static int InnocentGrey_dat_extract_resource(struct package *pkg,
											 struct package_resource *pkg_res)
{
	DWORD comprlen = pkg_res->raw_data_length;
	BYTE *compr = (BYTE *)pkg->pio->readvec_only(pkg, comprlen, 
		pkg_res->offset, IO_SEEK_SET);
	if (!compr)
		return -CUI_EREADVECONLY;

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = compr;
		return 0;
	}

	BYTE *act_data;
	DWORD act_data_len;
	DWORD uncomprlen = pkg_res->actual_data_length;
	if (uncomprlen != -1) {
		BYTE *uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr)
			return -CUI_EMEM;

		act_data = uncompr;
		act_data_len = uncomprlen;
		pkg_res->actual_data = uncompr;
		pkg_res->actual_data_length = uncomprlen;

		dat_entry_t *dat_entry = (dat_entry_t *)pkg_res->actual_index_entry;
		if (dat_entry->mode & 0xffffff) {
			if (dat_entry->mode & 0x10000) {
				DWORD xor = comprlen / 4;
				xor = (xor << ((xor & 7) + 8)) ^ xor;
				DWORD *enc_buf = (DWORD *)compr;
				DWORD *dec_buf = (DWORD *)uncompr;
				for (DWORD i = 0; i < comprlen / 4; i++) {
					BYTE shifts;
					dec_buf[i] = enc_buf[i] ^ xor;
					shifts = (BYTE)(dec_buf[i] % 24);
					xor = (xor << shifts) | (xor >> (32 - shifts));
				}

				if (dat_entry->mode & 1) {

				}

				if (dat_entry->mode & 2) {

				}

				if (dat_entry->mode & 0x200) {

				}

				if (dat_entry->mode & 0x100) {

				}
			}
		} else
			memcpy(uncompr, compr, comprlen);
	} else {
		pkg_res->raw_data = compr;
		act_data = compr;
		act_data_len = comprlen;
	}
#if 0
	char *ext = PathFindExtensionA(pkg_res->name);
	if (!(pkg_res->flags & PKG_RES_FLAG_RAW) && !lstrcmpiA(ext, ".s")) {
		for (unsigned int i = 0; i < act_data_len; i++)
			act_data[i] = ~act_data[i];
	}
#endif
	return 0;
}

static void InnocentGrey_dat_release_resource(struct package *pkg,
											  struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res->raw_data)
		pkg_res->raw_data = NULL;
}

static cui_ext_operation InnocentGrey_dat_operation = {
	InnocentGrey_dat_match,					/* match */
	InnocentGrey_dat_extract_directory,		/* extract_directory */
	InnocentGrey_dat_parse_resource_info,	/* parse_resource_info */
	InnocentGrey_dat_extract_resource,		/* extract_resource */
	cui_common_save_resource,				/* save_resource */
	InnocentGrey_dat_release_resource,		/* release_resource */
	cui_common_release						/* release */
};

/********************* s *********************/

static int InnocentGrey_s_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

static int InnocentGrey_s_extract_resource(struct package *pkg,
										   struct package_resource *pkg_res)
{
	u32 s_size;
	if (pkg->pio->length_of(pkg, &s_size))
		return -CUI_ELEN;

	BYTE *dat = (BYTE *)malloc(s_size);
	if (!dat)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, dat, s_size, 0, IO_SEEK_SET)) {
		free(dat);
		return -CUI_EREADVEC;
	}

	for (unsigned int i = 0; i < s_size; i++)
		dat[i] = ~dat[i];

	pkg_res->raw_data = dat;
	pkg_res->raw_data_length = s_size;

	return 0;
}

static void InnocentGrey_s_release_resource(struct package *pkg,
											struct package_resource *pkg_res)
{
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static cui_ext_operation InnocentGrey_s_operation = {
	InnocentGrey_s_match,				/* match */
	NULL,								/* extract_directory */
	NULL,								/* parse_resource_info */
	InnocentGrey_s_extract_resource,	/* extract_resource */
	cui_common_save_resource,			/* save_resource */
	InnocentGrey_s_release_resource,	/* release_resource */
	cui_common_release					/* release */
};

int CALLBACK InnocentGrey_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &InnocentGrey_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".d"), NULL, 
		NULL, &InnocentGrey_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".s"), NULL, 
		NULL, &InnocentGrey_s_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	return 0;
}
