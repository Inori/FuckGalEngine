#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <utility.h>
#include <stdio.h>

struct acui_information DaiSystem_cui_information = {
	_T("DAI 氏"),				/* copyright */
	_T("DaiSystem"),			/* system */
	_T(".pac"),					/* package */
	_T("0.9.0"),				/* revision */
	_T("痴漢公賊"),				/* author */
	_T("2008-4-21 15:15"),		/* date */
	NULL,						/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

#pragma pack (1)
typedef struct {
	s8 magic[16];	/* "DAI_SYSTEM_01000" */
	u16 index_entries;
	u32 index_length;
} pac_header_t;

typedef struct {
	s8 magic[3];			/* "HA0" */
	u8 parameter_length;
	u32 actual_length;
	u8 mode[4];
	u32 reserved;
} ha0_header_t;

typedef struct {
	u16 width;
	u16 height;
	u8 have_alpha;		/* 0 - 无; 1 - 有 */
} cg_parameter_t;
#pragma pack ()

typedef struct {
	s8 name[32];
	unsigned int name_length;
	unsigned long offset;
	unsigned long length;
} pac_entry_t;

#define SWAP16(v)	((((v) & 0xff) << 8) | (((v) >> 8) & 0xff))
#define SWAP32(v)	((((v) & 0xff) << 24) | (((v) & 0xff00) << 8) | (((v) & 0xff0000) >> 8) | (((v) & 0xff000000) >> 24))

static void *my_malloc(DWORD len)
{
	return new BYTE[len];
}

static int ha0_decode2(BYTE **p_enc, DWORD enc_len)
{
	BYTE *enc = *p_enc;
	DWORD &dec_len = enc_len;
	BYTE *dec = new BYTE[dec_len];
	if (!dec)
		return -CUI_EMEM;

	for (DWORD i = 0; i < 3; i++) {
		BYTE *p = dec + i;
		while (p < dec + dec_len) {
			*p = *enc++;
			p += 3;
		}
	}
	delete [] enc;
	*p_enc = dec;
	return 0;
}

static void ha0_decode3(BYTE *buf, DWORD buf_len)
{
	BYTE *src, *dst;
	
	dst = src = buf + 1;
	for (DWORD i = 1; i < buf_len; i++)
		*dst++ = dst[-1] + *src++;
}

static inline BYTE getbit_le(BYTE byte, DWORD pos)
{
	return !!(byte & (1 << pos));
}

static int ha0_decode4(BYTE **p_compr, DWORD *ret_uncomprlen)
{
	u32 uncomprlen;
	u32 flag_bitmap_bit_length;
	u32 flag_bitmap_byte_length;

	BYTE *compr = *p_compr;
	uncomprlen = SWAP32(*(u32 *)compr);
	BYTE *uncompr = new BYTE[uncomprlen];
	if (!uncompr)
		return -CUI_EMEM;
	compr += 4;
	flag_bitmap_bit_length = SWAP32(*(u32 *)compr);
	compr += 4;
	flag_bitmap_byte_length = SWAP32(*(u32 *)compr);
	compr += 4;
	BYTE *flag_bitmap = compr;
	compr = flag_bitmap + flag_bitmap_byte_length;

	DWORD act_uncomprlen = 0;
	DWORD curbyte = 0;
	for (DWORD b = 0; b < flag_bitmap_bit_length; b++) {
		BYTE flag = flag_bitmap[b >> 3];

		if (!getbit_le(flag, b & 7))
			uncompr[act_uncomprlen++] = compr[curbyte++];
		else {
			DWORD copy_bytes, win_offset;

			win_offset = compr[curbyte++];
			copy_bytes = compr[curbyte++];

			for (DWORD k = 0; k < copy_bytes; k++)
				uncompr[act_uncomprlen + k] = uncompr[act_uncomprlen - win_offset + k];
			act_uncomprlen += copy_bytes;
		}
	}
	delete [] *p_compr;
	*p_compr = uncompr;
	*ret_uncomprlen = act_uncomprlen;
	return 0;
}

static int ha0_decode(BYTE *mode, BYTE **compr, DWORD *uncomprlen)
{
	DWORD act_uncomprlen = *uncomprlen;
	int ret = 0;

	for (int i = 3; i >= 0; i--) {
		switch (mode[i]) {
		case 1:
			printf("unsupport 1\n");
			exit(0);
		case 2:
			ret = ha0_decode2(compr, act_uncomprlen);
			break;
		case 3:
			ha0_decode3(*compr, act_uncomprlen);
			ret = 0;
			break;
		case 4:
			ret = ha0_decode4(compr, &act_uncomprlen);
			break;
		}
		if (ret)
			break;
	}
	*uncomprlen = act_uncomprlen;
	return ret;
}

static int BuildBMPFile(BYTE *buf, DWORD buf_length, 
			DWORD width, DWORD height, DWORD bpp,
			BYTE **ret, DWORD *ret_length)
{
	BITMAPFILEHEADER *bmfh;
	BITMAPINFOHEADER *bmiHeader;
	DWORD aligned_line_length, raw_line_length;
	DWORD act_dib_length, output_length;
	BYTE *pdib, *output;
	
	raw_line_length = (width * 3 + 3) & ~3;
	aligned_line_length = (width * bpp / 8 + 3) & ~3;	
	act_dib_length = aligned_line_length * height;

	output_length = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + act_dib_length;
	output = (BYTE *)malloc(output_length);
	if (!output)
		return -1;
	
	bmfh = (BITMAPFILEHEADER *)output;	
	bmfh->bfType = 0x4D42;
	bmfh->bfSize = output_length;
	bmfh->bfReserved1 = 0;
	bmfh->bfReserved2 = 0;
	bmfh->bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	bmiHeader = (BITMAPINFOHEADER *)(bmfh + 1);	
	bmiHeader->biSize = sizeof(BITMAPINFOHEADER);
	bmiHeader->biWidth = width;		
	bmiHeader->biHeight = height;
	bmiHeader->biPlanes = 1;
	bmiHeader->biBitCount = (WORD)bpp;
	bmiHeader->biCompression = BI_RGB;		
	bmiHeader->biSizeImage = act_dib_length;
	bmiHeader->biXPelsPerMeter = 0;
	bmiHeader->biYPelsPerMeter = 0;
	bmiHeader->biClrUsed = 0;		
	bmiHeader->biClrImportant = 0;
	pdib = (BYTE *)(bmiHeader + 1);
	
	if (bpp == 32) {
		BYTE *alpha = buf + buf_length - width * height;
		for (unsigned int y = 0; y < height; y++) {
			for (unsigned int x = 0; x < width; x++) {
				for (unsigned int p = 0; p < 3; p++)
					pdib[y * aligned_line_length + x * 4 + p] = buf[y * raw_line_length + x * 3 + p];
				pdib[y * aligned_line_length + x * 4 + 3] = *alpha++;
			}
		}
	} else
		memcpy(pdib, buf, buf_length);
	
	*ret = output;
	*ret_length = output_length;	
	return 0;
}

/********************* pac *********************/

static int DaiSystem_pac_match(struct package *pkg)
{
	s8 magic[16];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "DAI_SYSTEM_01000", 16)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int DaiSystem_pac_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{
	unsigned int index_entries;
	BYTE *index_buffer;
	unsigned int index_buffer_length;
	pac_entry_t *pac_index_buffer;
	u32 pac_size;

	if (pkg->pio->read(pkg, &index_entries, 2))
		return -CUI_EREAD;
	index_entries = SWAP16(index_entries);

	if (pkg->pio->read(pkg, &index_buffer_length, 4))
		return -CUI_EREAD;
	index_buffer_length = SWAP32(index_buffer_length);

	index_buffer = new BYTE[index_buffer_length];
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		delete [] index_buffer;
		return -CUI_EREAD;
	}

	for (unsigned int i = 0; i < index_buffer_length; i++)
		index_buffer[i] -= i + 0x28;

	if (pkg->pio->length_of(pkg, &pac_size)) {
		delete [] index_buffer;
		return -CUI_EREAD;
	}

	index_buffer_length = sizeof(pac_entry_t) * index_entries;
	pac_index_buffer = new pac_entry_t[index_buffer_length];
	if (!pac_index_buffer) {
		delete [] index_buffer;
		return -CUI_EREAD;
	}

	/* 解析名称和偏移 */
	BYTE *p = index_buffer;
	for (i = 0; i < index_entries; i++) {
		unsigned int name_len = 0;

		while (1) {
			if (p[name_len] == '\x2c')
				break;
			name_len++;
		}
		memcpy(pac_index_buffer[i].name, p, name_len);
		pac_index_buffer[i].name[name_len] = 0;
		pac_index_buffer[i].name_length = name_len;
		p += name_len + 1;
		pac_index_buffer[i].offset = SWAP32(*(u32 *)p);
		p += 4 + 1;
	}	
	delete [] index_buffer;
	/* 解析长度 */
	for (i = 0; i < index_entries - 1; i++)
		pac_index_buffer[i].length = pac_index_buffer[i + 1].offset
			- pac_index_buffer[i].offset;
	pac_index_buffer[i].length = (unsigned long)pac_size - pac_index_buffer[i].offset;

	pkg_dir->index_entries = index_entries;
	pkg_dir->index_entry_length = sizeof(pac_entry_t);
	pkg_dir->directory = pac_index_buffer;
	pkg_dir->directory_length = index_buffer_length;

	return 0;
}

static int DaiSystem_pac_parse_resource_info(struct package *pkg,
											 struct package_resource *pkg_res)
{
	pac_entry_t *pac_entry;

	pac_entry = (pac_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, pac_entry->name);
	pkg_res->name_length = pac_entry->name_length;
	pkg_res->raw_data_length = pac_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = pac_entry->offset;

	return 0;
}

static int DaiSystem_pac_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	ha0_header_t ha0_header;

	if (pkg->pio->readvec(pkg, &ha0_header, sizeof(ha0_header), pkg_res->offset, IO_SEEK_SET))
		return -CUI_EREADVEC;

	if ((pkg_res->flags & PKG_RES_FLAG_RAW) || strncmp(ha0_header.magic, "HA0", 3)) {
		pkg_res->raw_data = new BYTE[pkg_res->raw_data_length];
		if (!pkg_res->raw_data)
			return -CUI_EMEM;

		if (pkg->pio->readvec(pkg, pkg_res->raw_data, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET)) {
			delete [] pkg_res->raw_data;
			pkg_res->raw_data = NULL;
			return -CUI_EREADVEC;
		}

		return 0;
	}

	BYTE *parameter = new BYTE[ha0_header.parameter_length];
	if (!parameter)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, parameter, ha0_header.parameter_length)) {
		delete [] parameter;
		return -CUI_EREAD;
	}

	DWORD buffer_len = pkg_res->raw_data_length - sizeof(ha0_header_t) - ha0_header.parameter_length;
	BYTE *buffer = new BYTE[buffer_len];
	if (!buffer) {
		delete [] parameter;
		return -CUI_EMEM;
	}

	if (pkg->pio->read(pkg, buffer, buffer_len)) {
		delete [] buffer;
		delete [] parameter;
		return -CUI_EREAD;
	}

	pkg_res->actual_data = buffer;
	pkg_res->actual_data_length = SWAP32(ha0_header.actual_length);
	int ret = ha0_decode(ha0_header.mode, (BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length);
	if (ret) {
		delete [] buffer;
		delete [] parameter;
		return ret;
	}

	buffer = (BYTE *)pkg_res->actual_data;
	char *ext = PathFindExtensionA(pkg_res->name);
	if (!strncmp((char *)buffer, "OggS", 4)) {
		pkg_res->replace_extension = _T(".ogg");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;	
	} else if ((ext && !strcmp(ext, ".txt")) || !lstrcmpi(pkg->name, _T("data_script.pac"))) {
		pkg_res->replace_extension = _T(".txt");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;	
	} else if (!strncmp((char *)buffer + pkg_res->actual_data_length - 18, "TRUEVISION-XFILE.", 18)) {
		pkg_res->replace_extension = _T(".tga");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;	
	} else if (!strncmp((char *)buffer, "BM", 2)) {
		pkg_res->replace_extension = _T(".bmp");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;	
	} else if (ha0_header.parameter_length == 5) {
		cg_parameter_t *cg_para = (cg_parameter_t *)parameter;

		/* uncomprlen ＝ aligned_line_length * height + width * height */
		if (BuildBMPFile(buffer, pkg_res->actual_data_length, SWAP16(cg_para->width), SWAP16(cg_para->height), 
				!cg_para->have_alpha ? 24 : 32, (BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length)) {
			delete [] buffer;
			delete [] parameter;
			return -CUI_EMEM;
		}
		delete [] buffer;
		pkg_res->replace_extension = _T(".bmp");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;	
	}
	delete [] parameter;

	return 0;
}

static int DaiSystem_pac_save_resource(struct resource *res, 
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

static void DaiSystem_pac_release_resource(struct package *pkg, 
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

static void DaiSystem_pac_release(struct package *pkg, 
								  struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

static cui_ext_operation DaiSystem_pac_operation = {
	DaiSystem_pac_match,				/* match */
	DaiSystem_pac_extract_directory,	/* extract_directory */
	DaiSystem_pac_parse_resource_info,	/* parse_resource_info */
	DaiSystem_pac_extract_resource,		/* extract_resource */
	DaiSystem_pac_save_resource,		/* save_resource */
	DaiSystem_pac_release_resource,		/* release_resource */
	DaiSystem_pac_release				/* release */
};

int CALLBACK DaiSystem_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".pac"), NULL, 
		NULL, &DaiSystem_pac_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
