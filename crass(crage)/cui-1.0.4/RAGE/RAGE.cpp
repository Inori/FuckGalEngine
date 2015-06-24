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
#include <utility.h>

struct acui_information RAGE_cui_information = {
	_T("Cool"),				/* copyright */
	_T("RAGE's Adventure Game Engine"),		/* system */
	_T(".Pac"),				/* package */
	_T("1.0.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-6-19 17:56"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};	

#pragma pack (1)
typedef struct {
	s8 magic[4];		// "PAC1"
	u32 index_entries;	// 每项32字节
} Pac_header_t;

typedef struct {
	s8 name[16];
	u32 length;
	u8 data_header[8];	// 资源的头部数据
	u32 offset;			// 原本是和第2个资源头数据一致；所以改用为offset
} Pac_entry_t;

typedef struct {
	s8 magic[4];		// "CMP1"
	u32 uncomprlen;
	u32 comprlen;
} CMP_header_t;
#pragma pack ()

static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static void lzss_decompress(unsigned char *uncompr, DWORD uncomprlen, 
							unsigned char *compr, DWORD comprlen)
{
	unsigned int act_uncomprlen = 0;
	/* compr中的当前字节中的下一个扫描位的位置 */
	unsigned int curbit = 0;
	/* compr中的当前扫描字节 */
	unsigned int curbyte = 0;
	unsigned int nCurWindowByte = 0x7ef;
	BYTE win[2048];
	struct bits bits;
	
	memset(win, 0x20, nCurWindowByte);
	bits_init(&bits, compr, comprlen);
	while (1) {
		/* 如果为0, 表示接下来的1个字节原样输出 */
		unsigned int flag;

		if (bits_get_high(&bits, 1, &flag))
			break;
		if (flag) {
			unsigned int data;

			if (bits_get_high(&bits, 8, &data))
				break;
			uncompr[act_uncomprlen++] = data;
			/* 输出的1字节放入滑动窗口 */
			win[nCurWindowByte++] = data;
			nCurWindowByte &= sizeof(win) - 1;
		} else {
			unsigned int copy_bytes, win_offset;
			unsigned int i;

			if (bits_get_high(&bits, 11, &win_offset))
				break;
			if (bits_get_high(&bits, 4, &copy_bytes))
				break;
			copy_bytes += 2;

			for (i = 0; i < copy_bytes; i++) {	
				unsigned char data;

				data = win[(win_offset + i) & (sizeof(win) - 1)];
				uncompr[act_uncomprlen++] = data;		
				/* 输出的1字节放入滑动窗口 */
				win[nCurWindowByte++] = data;
				nCurWindowByte &= sizeof(win) - 1;	
			}
		}
	}
}

/********************* Pac *********************/

static int RAGE_Pac_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}
	
	if (strncmp(magic, "PAC1", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int RAGE_Pac_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	u32 index_entries;
	Pac_entry_t *index_buffer;
	DWORD index_length;
	
	if (pkg->pio->read(pkg, &index_entries, 4))
		return -CUI_EREAD;

	index_length = index_entries * sizeof(Pac_entry_t);
	index_buffer = (Pac_entry_t *)malloc(index_length);
	if (!index_buffer)
		return -CUI_EMEM;
	
	if (pkg->pio->read(pkg, index_buffer, index_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}
	
	u32 offset = 0;
	for (DWORD i = 0; i < index_entries; i++) {
		Pac_entry_t *entry = &index_buffer[i];
		
		if (*(u32 *)&entry->data_header[4] != entry->offset)
			break;
		
		entry->offset = offset + sizeof(Pac_header_t) + index_length;
		offset += entry->length;
	}
	if (i != index_entries) {
		free(index_buffer);
		return -CUI_EMATCH;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_length;
	pkg_dir->index_entry_length = sizeof(Pac_entry_t);

	return 0;
}

static int RAGE_Pac_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	Pac_entry_t *Pac_entry;

	Pac_entry = (Pac_entry_t *)pkg_res->actual_index_entry;
	strncpy(pkg_res->name, Pac_entry->name, 16);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = Pac_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = Pac_entry->offset;

	return 0;
}

static int RAGE_Pac_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen,	pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}
	
	if (!strncmp((char *)compr, "CMP1", 4)) {
		CMP_header_t *cmp_header = (CMP_header_t *)compr;
		uncomprlen = cmp_header->uncomprlen;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}
		lzss_decompress(uncompr, uncomprlen, (BYTE *)(cmp_header + 1), cmp_header->comprlen);
		if (!strncmp((char *)uncompr, "SCR1", 4)) {
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
			pkg_res->replace_extension = _T(".scr");
		} else if (!strncmp((char *)uncompr, "BM", 2)) {
			BITMAPFILEHEADER *bmfh;
			BITMAPINFOHEADER *bmiHeader;
			DWORD line_len, act_line_len;
			/* TODO line len align */
			
			bmfh = (BITMAPFILEHEADER *)uncompr;
			bmiHeader = (BITMAPINFOHEADER *)(bmfh + 1);
			line_len = bmiHeader->biWidth * bmiHeader->biBitCount / 8;
			act_line_len = (act_line_len + 3) & ~3;
			if (line_len != act_line_len) {
				BYTE *rgb = (BYTE *)(bmiHeader + 1);
				BYTE *act_uncompr, *palette;
				DWORD rgb_len, act_uncomprlen, palette_length;
				
				if (bmiHeader->biBitCount <= 8) {
					palette = rgb;
					palette_length = 0x400;
					rgb += palette_length;
				} else {
					palette = NULL;
					palette_length = 0;
				}
				rgb_len = line_len * bmiHeader->biHeight;
				
				if (MyBuildBMPFile(rgb, rgb_len, palette, palette_length, bmiHeader->biWidth, 
						bmiHeader->biHeight, bmiHeader->biBitCount, &act_uncompr, &act_uncomprlen, my_malloc)) {
					free(uncompr);
					free(compr);
					return -CUI_EMEM;
				}
				free(uncompr);
				uncompr = act_uncompr;
				uncomprlen = act_uncomprlen;
			}
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
			pkg_res->replace_extension = _T(".bmp");
		}
	} else {
		uncompr = NULL;
		uncomprlen = 0;
	}

	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

static int RAGE_Pac_save_resource(struct resource *res, 
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

static void RAGE_Pac_release_resource(struct package *pkg, 
									  struct package_resource *pkg_res)
{
	if (pkg_res && pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res && pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void RAGE_Pac_release(struct package *pkg, 
							 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {		
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg);
}

static cui_ext_operation RAGE_Pac_operation = {
	RAGE_Pac_match,					/* match */
	RAGE_Pac_extract_directory,		/* extract_directory */
	RAGE_Pac_parse_resource_info,	/* parse_resource_info */
	RAGE_Pac_extract_resource,		/* extract_resource */
	RAGE_Pac_save_resource,			/* save_resource */
	RAGE_Pac_release_resource,		/* release_resource */
	RAGE_Pac_release				/* release */
};

int CALLBACK RAGE_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".Pac"), NULL, 
		NULL, &RAGE_Pac_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
