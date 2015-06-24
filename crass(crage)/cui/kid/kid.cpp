#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>
#include <utility.h>
#include <cui_common.h>

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information kid_cui_information = {
	_T(""),		/* copyright */
	_T(""),			/* system */
	_T(".dat"),		/* package */
	_T("1.0.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T(""),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];	// "LNK"
	u32 index_entries;
	u32 reserved[2];
} dat_header_t;

typedef struct {
	u32 offset;
	u32 length;
	s8 name[24];
} dat_entry_t;

typedef struct {
	s8 magic[4];	// "CPS"
	u32 cps_size;
	u16 sync;		// 0x66
	u16 flags;
	u32 uncomprlen;
	s8 suffix[4];	// "bmp"
} cps_header_t;

typedef struct {
	s8 magic[4];		// "PRT"
	u16 sync;			// 0x66
	u16 bpp;
	u16 header_size;
	u16 rgb_offset;
	u16 width;
	u16 height;
	u32 unknown[5];
} prt_header_t;
#pragma pack ()


static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static void decode(BYTE *enc, DWORD enc_len)
{
	u32 seed = *(u32 *)&enc[enc_len - 4] - 0x7534682;
	if (seed) {
		u32 rtseed = 0x3786425 + *(u32 *)&enc[seed] + seed;
		DWORD offset = 16;
		u32 *dec = (u32 *)&enc[16];
		while (offset < enc_len - 4) {
			if (offset != seed)
				*dec -= enc_len + rtseed;
			rtseed  = rtseed * 0x41C64E6D + 0x9B06;
			offset += 4;
			dec++;
		}
		*(u32 *)&enc[enc_len - 4] = 0;
	}
}

static DWORD cps_uncompress(BYTE *uncompr, DWORD uncomprlen, 
							BYTE *compr)
{
	DWORD act_uncomprlen = 0;
	DWORD curbyte = 0;

	while (act_uncomprlen < uncomprlen) {
		BYTE flag = compr[curbyte++];
		DWORD copy_bytes;

		if (flag & 0x80) {
			if (flag & 0x40) {
				copy_bytes = (flag & 0x1f) + 2;
				if (flag & 0x20)
					copy_bytes += compr[curbyte++] << 5;
		
				BYTE dat = compr[curbyte++];
				for (DWORD i = 0; i < copy_bytes; i++) {
					if (act_uncomprlen > uncomprlen)
						break;
					uncompr[act_uncomprlen++] = dat;
				}
			} else {				
				DWORD offset, off;
				
				off = compr[curbyte++];
				offset = (flag & 3) << 8;
				copy_bytes = ((flag >> 2) & 0xf) + 2;
				BYTE *src = uncompr + act_uncomprlen - offset - off - 1;
				for (DWORD i = 0; i < copy_bytes; i++) {
					if (act_uncomprlen > uncomprlen)
						break;
					uncompr[act_uncomprlen++] = *src++;
				}
			}
		} else {
			if (flag & 0x40) {
				DWORD count = compr[curbyte++] + 1;
				copy_bytes = (flag & 0x3f) + 2;

				for (DWORD c = 0; c < count; c++) {
					for (DWORD i = 0; i < copy_bytes; i++) {
						if (act_uncomprlen > uncomprlen)
							break;
						uncompr[act_uncomprlen++] = compr[curbyte + i];
					}					
					if (i != copy_bytes)
						break;
				}
				curbyte += copy_bytes;
			} else {
				copy_bytes = (flag & 0x1f) + 1;
				if (flag & 0x20)
					copy_bytes += compr[curbyte++] << 5;

				for (DWORD i = 0; i < copy_bytes; i++) {
					if (act_uncomprlen > uncomprlen)
						break;
					uncompr[act_uncomprlen++] = compr[curbyte++];
				}
			}			
		}
	}

	return act_uncomprlen;
}

/********************* dat *********************/

/* 封包匹配回调函数 */
static int kid_dat_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "LNK", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int kid_dat_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{
	dat_header_t dat_header;
	dat_entry_t *index_buffer;
	DWORD index_buffer_length;	

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

	for (DWORD i = 0; i < dat_header.index_entries; i++) {
		index_buffer[i].offset += index_buffer_length + sizeof(dat_header_t);
		index_buffer[i].length /= 2; 
	}
	pkg_dir->index_entries = dat_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(dat_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int kid_dat_parse_resource_info(struct package *pkg,
									   struct package_resource *pkg_res)
{
	dat_entry_t *dat_entry;

	dat_entry = (dat_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, dat_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = dat_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = dat_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int kid_dat_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	BYTE *raw = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(raw);
			return -CUI_EREADVEC;
	}

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = raw;
		return 0;
	}

	if (!strncmp((char *)raw, "CPS", 4)) {
		cps_header_t *cps_header = (cps_header_t *)raw;
		s8 suffix[4];

		if (cps_header->sync != 0x66) {
			free(raw);
			return -CUI_EMATCH;
		}
		memcpy(suffix, cps_header->suffix, sizeof(suffix));
		decode(raw, pkg_res->raw_data_length);

		const TCHAR *save_tsuffix_list[] = {
			_T(".bmp"),
			NULL
		};
		const char *save_suffix_list[] = {
			"bmp",
			NULL
		};

		
		for (DWORD s = 0; save_suffix_list[s]; s++) {
			if (!strcmp(save_suffix_list[s], suffix))
				break;
		}
		if (!save_suffix_list[s]) {		
			free(raw);
			return -CUI_EMATCH;		
		}
		const TCHAR *save_suffix = save_tsuffix_list[s];

		if (cps_header->flags & 1) {	// 4118E9
			DWORD uncomprlen = cps_header->uncomprlen;
			BYTE *uncompr = (BYTE *)malloc(uncomprlen);
			BYTE *compr = (BYTE *)(cps_header + 1);
			if (!uncompr) {
				free(raw);
				return -CUI_EMEM;
			}
			cps_uncompress(uncompr, uncomprlen, compr);
#if 0
			prt_header_t *prt_header = (prt_header_t *)uncompr;
			if (strcmp(prt_header->magic, "PRT") || prt_header->sync != 0x66) {
				free(uncompr);
				free(raw);
				return -CUI_EMEM;
			}

			if (prt_header->bpp != 8 && prt_header->bpp != 32) {
				if (MyBuildBMPFile((BYTE *)(prt_header + 1), 
					uncomprlen - sizeof(prt_header_t), NULL, 0,
					prt_header->width, prt_header->height, prt_header->bpp,
					(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, 
					my_malloc)) {
						free(uncompr);
						free(raw);
						return -CUI_EMEM;
				}
				free(uncompr);
			}
#endif
pkg_res->actual_data = uncompr;
pkg_res->actual_data_length = uncomprlen;
			if (save_suffix) {
				pkg_res->flags |= PKG_RES_FLAG_REEXT;
				pkg_res->replace_extension = save_suffix;
			}
		} else if (cps_header->flags & 2) {	// 4118FD

		} else {	// 41190D

		}



	}
	pkg_res->raw_data = raw;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation kid_dat_operation = {
	kid_dat_match,				/* match */
	kid_dat_extract_directory,	/* extract_directory */
	kid_dat_parse_resource_info,/* parse_resource_info */
	kid_dat_extract_resource,	/* extract_resource */
	CUI_COMMON_OPERATION
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK kid_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &kid_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
