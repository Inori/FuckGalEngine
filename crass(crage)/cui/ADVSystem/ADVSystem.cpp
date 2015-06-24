#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <utility.h>
#include <stdio.h>

struct acui_information ADVSystem_cui_information = {
	_T("U-Me SOFT / Blue"),			/* copyright */
	_T("U-Me SOFT ADV System"),		/* system */
	_T(".PK .TPK .GPK .WPK .MPK .PK0 .PKA .PKB .PKC .PKD .PKE .PKF .DAT"),	/* package */
	_T("1.0.1"),					/* revision */
	_T("痴漢公賊"),					/* author */
	_T("2009-6-15 20:43"),			/* date */
	NULL,							/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};	

#pragma pack (1)
typedef struct {
	s8 magic[4];		// "MGX\x1a"
	u32 entries;
	u32 *offset;
} mgx_header_t;

typedef struct {
	s8 magic[4];		// "SGX\x1a"
	u32 offset;
	u32 entries;
} sgx_header_t;

typedef struct {
	u16 orig_x;		// 左上角坐标
	u16 orig_y;
	u16 width;
	u16 height;
	u32 reserved;	// always -1
} sgx_entry_t;

typedef struct {
	s8 magic[4];		// "GRX\x1a"
	u8 is_compressed;
	u8 have_alpha;
	u16 color_depth;
	u16 width;
	u16 height;
	u32 alpha_offset;	// 0xffffffff
} grx_header_t;
#pragma pack ()

typedef struct {
	char name[MAX_PATH];
	DWORD offset;
	DWORD length;
} my_PK_entry_t;


static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static void rle_decompress(BYTE *uncompr, DWORD uncomprlen, BYTE *compr, DWORD comprlen)
{
	BYTE mask = 0;

	while (1) {
		BYTE flag;
		
		if (!mask) {
			flag = *compr++;
			mask = 0x80;
		}

		if (flag & mask) {
			DWORD offset, count;
			
			offset = *(u16 *)compr;
			count = (offset & 0xf) + 3;
			offset >>= 4;
			if (!offset)
				break;
			compr += 2;
			for (DWORD i = 0; i < count; i++) {
				*uncompr = *(uncompr - offset);
				uncompr++;
			}
		} else
			*uncompr++ = *compr++;
		mask >>= 1;
	}
}

static void xor_decode(BYTE *dat, DWORD dat_len)
{
	for (DWORD i = 0; i < dat_len; i++)
		dat[i] ^= 0x42;
}

static void grx_decompressA(BYTE *uncompr, BYTE *compr, DWORD width, DWORD height)
{
	u32 offset_step[2][16] = {
		{ 0, -1, -1, -1, 0, -2, -2, -2, 0, -4, -4, -4, -2, -2, -4, -4 },
		{ 0, 0, -1, 1, -2, 0, -2, 2, -4, 0, -4, 4, -4, 4, -2, 2 },
	};
	u32 s_offset_step[16];
	DWORD pitch, delta;
	int x_count, y_count;

	pitch = (width + 3) & ~3;
	delta = pitch - width;
	for (DWORD i = 0; i < 16; i++)
		s_offset_step[i] = offset_step[0][i] * pitch + offset_step[1][i];

	x_count = width;
	y_count = height;
	while (1) {
		DWORD copy_pixel_count;
		BYTE flag;

		flag = *compr++;
		copy_pixel_count = flag & 3;
		if (flag & 4)
			copy_pixel_count |= *compr++ << 2;
		copy_pixel_count++;
		x_count -= copy_pixel_count;
		if (!(flag & 0xF0)) {	// 高4位是offset（索引重复行的索引）
			if (flag & 8) {	// 如果设置，表示非重复RGB数据;反之表示是重复的RGB数据
				for (i = 0; i < copy_pixel_count; i++)
					*uncompr++ = *compr++ >> 3;
			} else {
				for (i = 0; i < copy_pixel_count; i++)
					*uncompr++ = *compr >> 3;
				compr++;
			}
		} else {	// 如果高4位offset存在
			BYTE *src = (BYTE *)(uncompr + s_offset_step[flag >> 4]);
			if (!(flag & 8)) {	// 如果设置，表示非重复RGB数据
				for (i = 0; i < copy_pixel_count; i++)
					*uncompr++ = *src;
			} else {	// *如果没设置，表示是重复的RGB数据
				for (i = 0; i < copy_pixel_count; i++)
					*uncompr++ = *src++;
			}
		}

		if (x_count <= 0) {
			if (!--y_count)
				break;
			x_count = width;
			uncompr += delta;
		}
	}
}

static void grx_decompress8(BYTE *uncompr, BYTE *compr, DWORD width, DWORD height)
{
	u32 offset_step[2][16] = {
		{ 0, -1, -1, -1, 0, -2, -2, -2, 0, -4, -4, -4, -2, -2, -4, -4 },
		{ 0, 0, -1, 1, -2, 0, -2, 2, -4, 0, -4, 4, -4, 4, -2, 2 },
	};
	u32 s_offset_step[16];
	DWORD pitch, delta;
	int x_count, y_count;

	pitch = (width + 3) & ~3;
	delta = pitch - width;
	for (DWORD i = 0; i < 16; i++)
		s_offset_step[i] = offset_step[0][i] * pitch + offset_step[1][i];

	x_count = width;
	y_count = height;
	while (1) {
		DWORD copy_pixel_count = 0;
		BYTE flag;
	
		flag = *compr++;
		copy_pixel_count = flag & 3;
		if (flag & 4)
			copy_pixel_count |= *compr++ << 2;
		copy_pixel_count++;
		x_count -= copy_pixel_count;
		if (!(flag & 0xF0)) {	// 高4位是offset（索引重复行的索引）
			if (flag & 8) {	// 如果设置，表示非重复RGB数据;反之表示是重复的RGB数据
				for (i = 0; i < copy_pixel_count; i++)
					*uncompr++ = *compr++;
			} else {
				for (i = 0; i < copy_pixel_count; i++)
					*uncompr++ = *compr;
				compr++;
			}
		} else {	// 如果高4位offset存在
			BYTE *src = (BYTE *)(uncompr + s_offset_step[flag >> 4]);
			if (!(flag & 8)) {	// 如果设置，表示非重复RGB数据
				for (i = 0; i < copy_pixel_count; i++)
					*uncompr++ = *src;
			} else {	// *如果没设置，表示是重复的RGB数据
				for (i = 0; i < copy_pixel_count; i++)
					*uncompr++ = *src++;
			}
		}

		if (x_count <= 0) {
			if (!--y_count)
				break;
			x_count = width;
			uncompr += delta;
		}
	}
}

static void grx_decompress16(BYTE *uncompr, BYTE *compr, DWORD width, DWORD height)
{
	u32 offset_step[2][16] = {
		{ 0, -1, -1, -1, 0, -2, -2, -2, 0, -4, -4, -4, -2, -2, -4, -4 },
		{ 0, 0, -1, 1, -2, 0, -2, 2, -4, 0, -4, 4, -4, 4, -2, 2 },
	};
	u32 s_offset_step[16];
	DWORD pitch, delta;
	int x_count, y_count;

	pitch = (width * 2 + 3) & ~3;
	delta = pitch - width * 2;
	for (DWORD i = 0; i < 16; i++)
		s_offset_step[i] = offset_step[0][i] * pitch + offset_step[1][i] * 2;

	x_count = width;
	y_count = height;
	while (1) {
		DWORD copy_pixel_count;
		BYTE flag;

		flag = *compr++;
		copy_pixel_count = flag & 3;
		if (flag & 4)
			copy_pixel_count |= *compr++ << 2;
		copy_pixel_count++;
		x_count -= copy_pixel_count;
		if (!(flag & 0xF0)) {	// 高4位是offset（索引重复行的索引）
			if (flag & 8) {//	// 如果设置，表示非重复RGB数据;反之表示是重复的RGB数据
				for (i = 0; i < copy_pixel_count; i++) {
					*(u16 *)uncompr = *(u16 *)compr;
					uncompr += 2;
					compr += 2;
				}
			} else {//
				for (i = 0; i < copy_pixel_count; i++) {
					*(u16 *)uncompr = *(u16 *)compr;
					uncompr += 2;
				}
				compr += 2;
			}
		} else {	// 如果高4位offset存在
			u16 *src = (u16 *)(uncompr + s_offset_step[flag >> 4]);
			if (!(flag & 8)) {	// 如果设置，表示非重复RGB数据
				for (i = 0; i < copy_pixel_count; i++) {//
					*(u16 *)uncompr = *(u16 *)src;
					uncompr += 2;
				}
			} else {	// *如果没设置，表示是重复的RGB数据
				for (i = 0; i < copy_pixel_count; i++) {
					*(u16 *)uncompr = *src++;
					uncompr += 2;
				}
			}
		}

		if (x_count <= 0) {
			if (!--y_count)
				break;
			x_count = width;
			uncompr += delta;
		}
	}
}

static void grx_decompress24(BYTE *uncompr, BYTE *compr, DWORD width, DWORD height)
{
	u32 offset_step[2][16] = {
		{ 0, -1, -1, -1, 0, -2, -2, -2, 0, -4, -4, -4, -2, -2, -4, -4 },
		{ 0, 0, -1, 1, -2, 0, -2, 2, -4, 0, -4, 4, -4, 4, -2, 2 },
	};
	u32 s_offset_step[16];
	DWORD pitch;
	int x_count, y_count;

	pitch = width * 4;
	for (DWORD i = 0; i < 16; i++)
		s_offset_step[i] = offset_step[0][i] * pitch + offset_step[1][i] * 4;

	x_count = width;
	y_count = height;
	while (1) {
		DWORD copy_pixel_count;
		BYTE flag;

		flag = *compr++;
		copy_pixel_count = flag & 3;
		if (flag & 4)
			copy_pixel_count |= *compr++ << 2;
		copy_pixel_count++;
		x_count -= copy_pixel_count;
		if (!(flag & 0xF0)) {	// 高4位是offset（索引重复行的索引）
			if (flag & 8) {	// 如果设置，表示非重复RGB数据;反之表示是重复的RGB数据
				for (i = 0; i < copy_pixel_count; i++) {
					*(u32 *)uncompr = *(u32 *)compr & 0xFFFFFF;
					uncompr += 4;
					compr += 3;
				}
			} else {
				for (i = 0; i < copy_pixel_count; i++) {
					*(u32 *)uncompr = *(u32 *)compr & 0xFFFFFF;
					uncompr += 4;
				}
				compr += 3;
			}
		} else {	// 如果高4位offset存在
			DWORD *src = (DWORD *)(uncompr + s_offset_step[flag >> 4]);
			if (!(flag & 8)) {	// 如果设置，表示非重复RGB数据
				for (i = 0; i < copy_pixel_count; i++) {
					*(u32 *)uncompr = *(u32 *)src & 0xFFFFFF;
					uncompr += 4;
				}
			} else {	// *如果没设置，表示是重复的RGB数据
				for (i = 0; i < copy_pixel_count; i++) {
					*(u32 *)uncompr = *(u32 *)src++;
					uncompr += 4;
				}
			}
		}

		if (x_count <= 0) {
			x_count = width;
			if (!--y_count)
				break;
		}
	}
}

static void grx_decompress32(BYTE *uncompr, BYTE *compr, DWORD width, DWORD height)
{
	u32 offset_step[2][16] = {
		{ 0, -1, -1, -1, 0, -2, -2, -2, 0, -4, -4, -4, -2, -2, -4, -4 },
		{ 0, 0, -1, 1, -2, 0, -2, 2, -4, 0, -4, 4, -4, 4, -2, 2 },
	};
	u32 s_offset_step[16];
	DWORD pitch;
	int x_count, y_count;

	pitch = width * 4;
	for (DWORD i = 0; i < 16; i++)
		s_offset_step[i] = offset_step[0][i] * pitch + offset_step[1][i] * 4;

	x_count = width;
	y_count = height;
	while (1) {
		DWORD copy_pixel_count;
		BYTE flag;

		flag = *compr++;
		copy_pixel_count = flag & 3;
		if (flag & 4)
			copy_pixel_count |= *compr++ << 2;
		copy_pixel_count++;
		x_count -= copy_pixel_count;
		if (!(flag & 0xF0)) {	// 高4位是offset（索引重复行的索引）
			if (flag & 8) {	// 如果设置，表示非重复RGB数据;反之表示是重复的RGB数据
				for (i = 0; i < copy_pixel_count; i++) {
					*(u32 *)uncompr = *(u32 *)compr;
					uncompr += 4;
					compr += 4;
				}
			} else {
				for (i = 0; i < copy_pixel_count; i++) {
					*(u32 *)uncompr = *(u32 *)compr;
					uncompr += 4;
				}
				compr += 4;
			}
		} else {	// 如果高4位offset存在
			DWORD *src = (DWORD *)(uncompr + s_offset_step[flag >> 4]);
			if (!(flag & 8)) {	// 如果设置，表示非重复RGB数据
				for (i = 0; i < copy_pixel_count; i++) {
					*(u32 *)uncompr = *src;
					uncompr += 4;
				}
			} else {	// *如果没设置，表示是重复的RGB数据
				for (i = 0; i < copy_pixel_count; i++) {
					*(u32 *)uncompr = *src++;
					uncompr += 4;
				}
			}
		}

		if (x_count <= 0) {
			x_count = width;
			if (!--y_count)
				break;
		}
	}
}

static void grx_decompress(BYTE *uncompr, BYTE *compr, DWORD width, DWORD height, DWORD bpp)
{
	switch (bpp) {
	case 8:
		grx_decompress8(uncompr, compr, width, height);
		break;
	case 16:
		grx_decompress16(uncompr, compr, width, height);
		break;
	case 24:
		grx_decompress24(uncompr, compr, width, height);
		break;
	case 32:
		grx_decompress32(uncompr, compr, width, height);
	}
}

/********************* *PK *********************/

static int ADVSystem_PK_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

static int ADVSystem_PK_extract_directory(struct package *pkg,
										  struct package_directory *pkg_dir)
{
	BYTE *index_buffer;
	my_PK_entry_t *entry_index;
	DWORD index_length, entry_index_length;

	if (pkg->pio->readvec(pkg, &index_length, 4, -4, IO_SEEK_END))
		return -CUI_EREADVEC;

	index_buffer = (BYTE *)malloc(index_length);
	if (!index_buffer)
		return -CUI_EMEM;
	
	if (pkg->pio->readvec(pkg, index_buffer, index_length, -4 - index_length, IO_SEEK_END)) {
		free(index_buffer);
		return -CUI_EREADVEC;
	}
	
	BYTE *p = index_buffer;
	for (DWORD index_entries = 0; ; index_entries++) {
		BYTE name_len = *p++;
		if (!name_len)
			break;
		p += name_len + 1;
		p += 5;	/* unknown field */
		p += 8;
	}

	entry_index_length = index_entries * sizeof(my_PK_entry_t);
	entry_index = (my_PK_entry_t *)malloc(entry_index_length);
	if (!entry_index) {
		free(index_buffer);
		return -CUI_EMEM;
	}

	p = index_buffer;
	for (DWORD i = 0; i < index_entries; i++) {
		BYTE name_len = *p++;
		memcpy(entry_index[i].name, p, name_len);
		entry_index[i].name[name_len] = 0;
		p += name_len + 1;
		p += 5;	/* unknown field */
		entry_index[i].length = *(u32 *)p;
		p += 4;
		entry_index[i].offset = *(u32 *)p;;
		p += 4;
	}
	free(index_buffer);

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = entry_index;
	pkg_dir->directory_length = entry_index_length;
	pkg_dir->index_entry_length = sizeof(my_PK_entry_t);

	return 0;
}

static int ADVSystem_PK_parse_resource_info(struct package *pkg,
											struct package_resource *pkg_res)
{
	my_PK_entry_t *my_PK_entry;

	my_PK_entry = (my_PK_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_PK_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_PK_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_PK_entry->offset;

	return 0;
}

static int ADVSystem_PK_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;
	
	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = compr;
		return 0;
	}

	if (!memcmp(compr, "FANI", 4) || !memcmp(compr, "CANI", 4)) {
		pkg_res->raw_data = compr;
		return 0;
	}

	char *ext = strstr(pkg_res->name, ".");
	if (ext && (!lstrcmpA(ext, ".SCR") || !lstrcmpA(ext, ".TBL"))) {
		uncomprlen = *(u32 *)compr;

		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}
		rle_decompress(uncompr, uncomprlen, compr + 4, comprlen - 4);
		xor_decode(uncompr, uncomprlen);
	} else if (ext && (!lstrcmpA(ext, ".GRX") || !lstrcmpA(ext, ".DAT"))) {
		if (!strncmp((char *)compr, "MGX\x1a", 4)) {
			uncompr = NULL;
			uncomprlen = 0;
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
			pkg_res->replace_extension = _T(".MGX");
		} else {
			grx_header_t *grx_header = (grx_header_t *)compr;
			BYTE *ret_uncompr;
			DWORD ret_uncomprlen;

			if (!strncmp(grx_header->magic, "SGX\x1a", 4))
				grx_header = (grx_header_t *)(compr + *(u32 *)(compr + 4));

			if (strncmp(grx_header->magic, "GRX\x1a", 4)) {
				free(compr);
				return -CUI_EMATCH;
			}

			if (grx_header->color_depth == 24)
				uncomprlen = ((grx_header->width + 3) & ~3) * grx_header->height * 4;
			else
				uncomprlen = ((grx_header->width + 3) & ~3) * grx_header->height * grx_header->color_depth / 8;
			uncompr = (BYTE *)malloc(uncomprlen);
			if (!uncompr) {
				free(compr);
				return -CUI_EMEM;
			}
			memset(uncompr, 0, uncomprlen);

			if (!grx_header->is_compressed)
				memcpy(uncompr, grx_header + 1, uncomprlen);
			else
				grx_decompress(uncompr, (BYTE *)(grx_header + 1), grx_header->width, 
					grx_header->height, grx_header->color_depth);

			if (grx_header->have_alpha == 1 && (grx_header->alpha_offset != -1)) {
				BYTE *alpha;
				DWORD alpha_length;

				alpha_length = ((grx_header->width + 3) & ~3) * grx_header->height;
				alpha = (BYTE *)malloc(alpha_length);
				if (!alpha) {
					free(uncompr);
					free(compr);
					return -CUI_EMEM;
				}
				// TODO: grx_decompressA
				grx_decompress8(alpha, (BYTE *)(grx_header + 1) + grx_header->alpha_offset, grx_header->width,
					grx_header->height);
				/* TODO: other color depth alpha blending */
				if (grx_header->color_depth == 24) {
					BYTE *p_alpha = alpha;
					BYTE *p_rgba = uncompr;

					for (DWORD y = 0; y < grx_header->height; y++) {
						for (DWORD x = 0; x < grx_header->width; x++) {
							p_rgba[3] = *p_alpha++;
							p_rgba += 4;
						}
						p_alpha += ((grx_header->width + 3) & ~3) - grx_header->width;
					}
					free(alpha);
				}
			}

			if (grx_header->color_depth == 16) {
				if (MyBuildBMP16File(uncompr, uncomprlen, grx_header->width, 0 - grx_header->height, &ret_uncompr, 
						&ret_uncomprlen, RGB565, NULL, my_malloc)) {
					free(uncompr);
					free(compr);
					return -CUI_EMEM;
				}
			} else if (grx_header->color_depth == 24) {
				if (MyBuildBMPFile(uncompr, uncomprlen, NULL, 0, grx_header->width, 
						0 - grx_header->height, 32, &ret_uncompr, 
						&ret_uncomprlen, my_malloc)) {
					free(uncompr);
					free(compr);
					return -CUI_EMEM;
				}
			} else {
				if (MyBuildBMPFile(uncompr, uncomprlen, NULL, 0, grx_header->width, 
						0 - grx_header->height, grx_header->color_depth, &ret_uncompr, 
						&ret_uncomprlen, my_malloc)) {
					free(uncompr);
					free(compr);
					return -CUI_EMEM;			
				}
			}
			free(uncompr);
			uncompr = ret_uncompr;
			uncomprlen = ret_uncomprlen;
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

static int ADVSystem_PK_save_resource(struct resource *res, 
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

static void ADVSystem_PK_release_resource(struct package *pkg, 
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

static void ADVSystem_PK_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory && pkg_dir->directory_length) {		
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg);
}

static cui_ext_operation ADVSystem_PK_operation = {
	ADVSystem_PK_match,					/* match */
	ADVSystem_PK_extract_directory,		/* extract_directory */
	ADVSystem_PK_parse_resource_info,	/* parse_resource_info */
	ADVSystem_PK_extract_resource,		/* extract_resource */
	ADVSystem_PK_save_resource,			/* save_resource */
	ADVSystem_PK_release_resource,		/* release_resource */
	ADVSystem_PK_release				/* release */
};

/********************* GPK *********************/

static int ADVSystem_GPK_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, 4)) {
		pkg->pio->close(pkg);
		return -CUI_EREADVEC;
	}

	if (strncmp(magic, "SGX\x1a", 4) && strncmp(magic, "GRX\x1a", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}


static cui_ext_operation ADVSystem_GPK_operation = {
	ADVSystem_GPK_match,				/* match */
	ADVSystem_PK_extract_directory,		/* extract_directory */
	ADVSystem_PK_parse_resource_info,	/* parse_resource_info */
	ADVSystem_PK_extract_resource,		/* extract_resource */
	ADVSystem_PK_save_resource,			/* save_resource */
	ADVSystem_PK_release_resource,		/* release_resource */
	ADVSystem_PK_release				/* release */
};
	
int CALLBACK ADVSystem_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".PK"), NULL, 
		NULL, &ADVSystem_PK_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC | CUI_EXT_FLAG_SUFFIX_SENSE))
			return -1;

	if (callback->add_extension(callback->cui, _T(".GPK"), NULL, 
		NULL, &ADVSystem_GPK_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_SUFFIX_SENSE))
			return -1;
	
	if (callback->add_extension(callback->cui, _T(".WPK"), NULL, 
		NULL, &ADVSystem_PK_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC | CUI_EXT_FLAG_SUFFIX_SENSE))
			return -1;
	
	if (callback->add_extension(callback->cui, _T(".TPK"), NULL, 
		NULL, &ADVSystem_PK_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC | CUI_EXT_FLAG_SUFFIX_SENSE))
			return -1;

	if (callback->add_extension(callback->cui, _T(".MPK"), NULL, 
		NULL, &ADVSystem_PK_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC | CUI_EXT_FLAG_SUFFIX_SENSE))
			return -1;

	if (callback->add_extension(callback->cui, _T(".PKA"), NULL, 
		NULL, &ADVSystem_PK_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC | CUI_EXT_FLAG_SUFFIX_SENSE))
			return -1;

	if (callback->add_extension(callback->cui, _T(".PKB"), NULL, 
		NULL, &ADVSystem_PK_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC | CUI_EXT_FLAG_SUFFIX_SENSE))
			return -1;	

	if (callback->add_extension(callback->cui, _T(".PKC"), NULL, 
		NULL, &ADVSystem_PK_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC | CUI_EXT_FLAG_SUFFIX_SENSE))
			return -1;

	if (callback->add_extension(callback->cui, _T(".PKD"), NULL, 
		NULL, &ADVSystem_PK_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC | CUI_EXT_FLAG_SUFFIX_SENSE))
			return -1;

	if (callback->add_extension(callback->cui, _T(".PKE"), NULL, 
		NULL, &ADVSystem_PK_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC | CUI_EXT_FLAG_SUFFIX_SENSE))
			return -1;

	if (callback->add_extension(callback->cui, _T(".PKF"), NULL, 
		NULL, &ADVSystem_PK_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC | CUI_EXT_FLAG_SUFFIX_SENSE))
			return -1;

	if (callback->add_extension(callback->cui, _T(".PK0"), NULL, 
		NULL, &ADVSystem_PK_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC | CUI_EXT_FLAG_SUFFIX_SENSE))
			return -1;	

	if (callback->add_extension(callback->cui, _T(".DAT"), NULL, 
		NULL, &ADVSystem_PK_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NO_MAGIC | CUI_EXT_FLAG_SUFFIX_SENSE))
			return -1;

	return 0;
}
