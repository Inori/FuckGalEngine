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

struct acui_information TmrHiroADVSystem_cui_information = {
	_T(""),						/* copyright */
	_T("Tmr-Hiro ADV System"),	/* system */
	_T(".pac"),					/* package */
	_T("1.0.1"),				/* revision */
	_T("痴汉公贼"),				/* author */
	_T("2007-12-30 11:58"),		/* date */
	NULL,						/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};	

#pragma pack (1)
typedef struct {
	u16 index_entries;
	u8 name_length;
	u32 data_offset;
} pac_header_t;

typedef struct {
	s8 *name;
	u32 offset;
	u32 length;
} pac_entry_t;

/*                      /-pitch_x-\
 |----------------------|---------|\(orig_x, orig_y)
 |                      |         |pitch_y
 |----------------------|         |/
 |                      |         |
 |                      |         |
 |                      |         |
 |                      |         |
 |                      |         |
 |----------------------|---------|
 (0, 0)
 */
typedef struct {
	u8 format;		// 1 - D3DFMT_X8R8G8B8; 2 - D3DFMT_A8R8G8B8
	u8 is_32bit;
	u16 width;		// 可能是屏幕显示宽度
	u16 height;		// 可能是屏幕显示高度
	u16 bits_count;
	u16 pitch_x;
	u16 orig_x;
	u16 pitch_y;
	u16 orig_y;
	u32 alpha_length;
	u32 blue_length;
	u32 green_length;
	u32 red_length;
} grp_header_t;

typedef struct {
	u32 intrs;
} srp_header_t;

typedef struct {
	u16 length;
	u32 value;
	u8 *parameter;
} srp_entry_t;

typedef struct {
	u8 unknown0;	// 0x44	(BCD 44.1k?)
	u32 unknown1;	// 0
	u32 length;
} mus_header_t;
#pragma pack ()

typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 length;
} my_pac_entry_t;

static int extract_raw;

static void *my_malloc(DWORD len)
{
	return malloc(len);
}

/********************* pac *********************/

static int TmrHiroADVSystem_pac_match(struct package *pkg)
{
	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

static int TmrHiroADVSystem_pac_extract_directory(struct package *pkg,
												  struct package_directory *pkg_dir)
{
	pac_header_t pac_header;
	my_pac_entry_t *index_buffer;
	DWORD index_length;

	if (pkg->pio->read(pkg, &pac_header, sizeof(pac_header)))
		return -CUI_EREAD;

	index_length = pac_header.index_entries * sizeof(my_pac_entry_t);
	index_buffer = (my_pac_entry_t *)malloc(index_length);
	if (!index_buffer)
		return -CUI_EMEM;

	for (DWORD i = 0; i < pac_header.index_entries; i++) {
		my_pac_entry_t *entry = &index_buffer[i];

		if (pkg->pio->read(pkg, entry->name, pac_header.name_length))
			break;
		entry->name[pac_header.name_length] = 0;

		if (pkg->pio->read(pkg, &entry->offset, 4))
			break;

		if (pkg->pio->read(pkg, &entry->length, 4))
			break;

		entry->offset += pac_header.data_offset;
	}
	if (i != pac_header.index_entries) {
		free(index_buffer);
		return -CUI_EMATCH;
	}

	pkg_dir->index_entries = pac_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_length;
	pkg_dir->index_entry_length = sizeof(my_pac_entry_t);

	return 0;
}

static int TmrHiroADVSystem_pac_parse_resource_info(struct package *pkg,
													struct package_resource *pkg_res)
{
	my_pac_entry_t *my_pac_entry;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	my_pac_entry = (my_pac_entry_t *)pkg_res->actual_index_entry;
	strncpy(pkg_res->name, my_pac_entry->name, 16);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_pac_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_pac_entry->offset;

	return 0;
}

/* uncomprlen是解压的象素数 */
static void __grp_decompress(BYTE *uncompr, DWORD uncomprlen, BYTE *compr, DWORD comprlen, int pitch, DWORD line_length, int pixel_offset)
{
	DWORD act_uncomprlen = 0;
	DWORD pixel = 0;
	DWORD w = 0;	/* 行内象素计数 */
	DWORD curbyte = 0;

	while (1) {
		DWORD count;
		BYTE flag;

		if (act_uncomprlen >= uncomprlen)
			break;
		if (curbyte >= comprlen)
			break;
			
		count = compr[curbyte++];
		flag = (BYTE)(count >> 7);
		count &= 0x7f;
			
		if (flag) {
			BYTE *dst = uncompr + pixel * 4 + pixel_offset;
			act_uncomprlen += count;
				
			for (DWORD i = 0; i < count; i++) {
				w++;
				*dst = compr[curbyte];
				/* 因为实际提取的不是一个完整的图像，而是一幅图像中的一个子区域，
				 * 所以用w计数到子区域的行长时要更新参数。
				 */
				if (w == line_length) {
					dst += pitch * 4;	// 移动到下一行
					w = 0;
					pixel += pitch;
				}
				dst += 4;
				pixel++;
			}
			curbyte++;
		} else {
			BYTE *dst = uncompr + pixel * 4 + pixel_offset;
			act_uncomprlen += count;
				
			for (DWORD i = 0; i < count; i++) {
				w++;
				*dst = compr[curbyte++];
				if (w == line_length) {
					dst += pitch * 4;	// 移动到上一行
					w = 0;
					pixel += pitch;
				}
				dst += 4;
				pixel++;
			}
		}
	}
}

static void grp_decompress(grp_header_t *grp_header, BYTE *uncompr, BYTE *compr)
{
	DWORD uncomprlen;
	BYTE *compr_a, *compr_b, *compr_g, *compr_r;

	compr_a = compr;
	compr_b = compr_a + grp_header->alpha_length;
	compr_g = compr_b + grp_header->blue_length;
	compr_r = compr_g + grp_header->green_length;
	uncomprlen = (grp_header->orig_x - grp_header->pitch_x) * (grp_header->orig_y - grp_header->pitch_y);
	if (extract_raw) {
		if (grp_header->is_32bit)
			__grp_decompress(uncompr, uncomprlen, compr_a, grp_header->alpha_length, grp_header->pitch_x, grp_header->orig_x - grp_header->pitch_x, 3);

		__grp_decompress(uncompr, uncomprlen, compr_b, grp_header->blue_length, grp_header->pitch_x, grp_header->orig_x - grp_header->pitch_x, 2);
		__grp_decompress(uncompr, uncomprlen, compr_g, grp_header->green_length, grp_header->pitch_x, grp_header->orig_x - grp_header->pitch_x, 1);
		__grp_decompress(uncompr, uncomprlen, compr_r, grp_header->red_length, grp_header->pitch_x, grp_header->orig_x - grp_header->pitch_x, 0);
	} else {
		if (grp_header->is_32bit)
			__grp_decompress(uncompr, uncomprlen, compr_a, grp_header->alpha_length, 0, grp_header->orig_x - grp_header->pitch_x, 3);

		__grp_decompress(uncompr, uncomprlen, compr_b, grp_header->blue_length, 0, grp_header->orig_x - grp_header->pitch_x, 2);
		__grp_decompress(uncompr, uncomprlen, compr_g, grp_header->green_length, 0, grp_header->orig_x - grp_header->pitch_x, 1);
		__grp_decompress(uncompr, uncomprlen, compr_r, grp_header->red_length, 0, grp_header->orig_x - grp_header->pitch_x, 0);
	}
}

static int TmrHiroADVSystem_pac_extract_resource(struct package *pkg,
												 struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;
	grp_header_t *grp_header;
	srp_header_t *srp_header;
	mus_header_t *mus_header;
	srp_entry_t *srp_entry;
		
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen,	pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}
	
	uncompr = NULL;
	uncomprlen = 0;
	grp_header = (grp_header_t *)compr;	
	srp_header = (srp_header_t *)compr;	
	srp_entry = (srp_entry_t *)(srp_header + 1);

	mus_header = (mus_header_t *)compr;	
	if (!strncmp((char *)compr, "OggS", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".ogg");
	} else if ((sizeof(grp_header_t) + grp_header->alpha_length + grp_header->blue_length + grp_header->green_length + grp_header->red_length) == comprlen) {
		DWORD rgba_len = grp_header->width * grp_header->height * 4;
		BYTE *rgba = (BYTE *)malloc(rgba_len);
		if (!rgba) {
			free(compr);
			return -CUI_EMEM;
		}
		memset(rgba, 0, rgba_len);
		grp_decompress(grp_header, rgba, (BYTE *)(grp_header + 1));
		if (extract_raw) {
			if (MyBuildBMPFile(rgba, rgba_len, NULL, 0, grp_header->orig_x, grp_header->orig_y, 32, &uncompr, &uncomprlen, my_malloc)) {
				free(rgba);
				free(compr);
				return -CUI_EMEM;
			}
		} else {
			if (MyBuildBMPFile(rgba, rgba_len, NULL, 0, grp_header->orig_x - grp_header->pitch_x, grp_header->orig_y - grp_header->pitch_y, 32, &uncompr, &uncomprlen, my_malloc)) {
				free(rgba);
				free(compr);
				return -CUI_EMEM;
			}
		}
		free(rgba);		
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp");
		//printf("format %x is_32bit %x width %x height %x pitch_x %x orig_x %x pitch_y %x orig_y %x\n", grp_header->format, grp_header->is_32bit, grp_header->width, grp_header->height,
			//grp_header->pitch_x, grp_header->orig_x, grp_header->pitch_y, grp_header->orig_y);
	} else if (srp_entry->length == 6 && srp_entry->value == 0x00140050) {
		BYTE *srp;

		srp = compr + 4;
		for (unsigned int i = 0; i < srp_header->intrs; i++) {
			u16 intr_len;
			
			intr_len = *(u16 *)srp - 4;
			srp += 6;
			for (unsigned int k = 0; k < intr_len; k++)
				srp[k] = ((srp[k] & 0xf) << 4) | (srp[k] >> 4);
			srp += intr_len;
		}
	} else if (mus_header->unknown0 == 0x44 && mus_header->unknown1 == 0) {
		BYTE riff[4] = { 'R', 'I', 'F', 'F' };
		char *id = "WAVEfmt "; 			
		WAVEFORMATEX wav_header;
		DWORD header_size = sizeof(wav_header);
		DWORD uncomprlen = 8 + header_size + 8 + mus_header->length;
		char *data = "data";

		wav_header.wFormatTag = 1;
		wav_header.nChannels = 2;
		wav_header.nSamplesPerSec = 0xac44;	
		wav_header.nAvgBytesPerSec = 0x2B110;
		wav_header.nBlockAlign = 4;
		wav_header.wBitsPerSample = 16;
		wav_header.cbSize = 0;
		
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}
		
		BYTE *p = uncompr;
		memcpy(p, riff, sizeof(riff));		
		p += sizeof(riff);
		memcpy(p, &uncomprlen, 4);		
		p += 4;
		memcpy(p, id, 8);
		p += 8;
		memcpy(p, &header_size, 4);		
		p += 4;
		memcpy(p, &wav_header, header_size);		
		p += header_size;
		memcpy(p, data, 4);		
		p += 4;
		memcpy(p, &mus_header->length, 4);		
		p += 4;
		memcpy(p, mus_header + 1, mus_header->length);		

		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".wav");
	}

	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;
	return 0;
}

static int TmrHiroADVSystem_pac_save_resource(struct resource *res, 
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

static void TmrHiroADVSystem_pac_release_resource(struct package *pkg, 
												  struct package_resource *pkg_res)
{
	if (!pkg)
		return;

	if (pkg_res && pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res && pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void TmrHiroADVSystem_pac_release(struct package *pkg, 
										 struct package_directory *pkg_dir)
{
	if (!pkg || !pkg_dir)
		return;

	if (pkg_dir->directory && pkg_dir->directory_length) {		
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg);
}

static cui_ext_operation TmrHiroADVSystem_pac_operation = {
	TmrHiroADVSystem_pac_match,					/* match */
	TmrHiroADVSystem_pac_extract_directory,		/* extract_directory */
	TmrHiroADVSystem_pac_parse_resource_info,	/* parse_resource_info */
	TmrHiroADVSystem_pac_extract_resource,		/* extract_resource */
	TmrHiroADVSystem_pac_save_resource,			/* save_resource */
	TmrHiroADVSystem_pac_release_resource,		/* release_resource */
	TmrHiroADVSystem_pac_release				/* release */
};

int CALLBACK TmrHiroADVSystem_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".pac"), NULL, 
		NULL, &TmrHiroADVSystem_pac_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	if (get_options("raw"))
		extract_raw = 1;
	else
		extract_raw = 0;
	return 0;
}
