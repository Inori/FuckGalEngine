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

struct acui_information SystemNNN_cui_information = {
	_T("Naynit Kcalbwen Studio"),		/* copyright */
	_T("System-NNN (Project NYANLIB)"),			/* system */
	_T(".spt .fxf .vaw .wgq .dwq .gpk .gtb .vpk .vtb"),		/* package */
	_T("1.2.1"),				/* revision */
	_T("痴漢公賊"),				/* author */
	_T("2009-6-19 19:08"),		/* date */
	NULL,						/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};	

#pragma pack (1)
typedef struct {
	s8 name[8];
	u32 offset;
} vtb_entry_t;
#pragma pack ()
	
typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 length;
} my_gpk_entry_t;

typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 length;
} my_vtb_entry_t;


static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static void LoadPackedDWQData(BYTE *uncompr, BYTE *compr, DWORD width, DWORD height, DWORD pitch, int dy)
{
	if (dy > 0) {
		for (DWORD y = 0; y < height; y++) {
			BYTE *dst = &uncompr[y * pitch];
			BYTE *prev = dst - pitch;
				
			for (DWORD x = 0; x < width; ) {
				BYTE flag = *compr++;
				if (flag)
					dst[x++] = flag;
				else {
					BYTE count = *compr++;
					memset(&dst[x], 0, count);
					x += count;
				}
			}
			
			if (y) {
				for (DWORD i = 0; i < width; i++)
					dst[i] ^= prev[i];
			}
		}
	} else {
		for (DWORD y = height - 1; (int)y >= 0; y--) {
			BYTE *dst = &uncompr[y * pitch];
			BYTE *prev = dst + pitch;
				
			for (DWORD x = 0; x < width; ) {
				BYTE flag = *compr++;
				if (flag)
					dst[x++] = flag;
				else {
					BYTE count = *compr++;
					memset(&dst[x], 0, count);
					x += count;
				}
			}
			
			if (y != height - 1) {
				for (DWORD i = 0; i < width; i++)
					dst[i] ^= prev[i];
			}
		}
	}
}

/********************* gpk *********************/

static int SystemNNN_gpk_match(struct package *pkg)
{
	u32 index_entries;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->open(pkg->lst, IO_READONLY)) {
		pkg->pio->close(pkg);
		return -CUI_EOPEN;
	}

	if (pkg->pio->read(pkg->lst, &index_entries, 4)) {
		pkg->pio->close(pkg->lst);
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (!index_entries) {
		pkg->pio->close(pkg->lst);
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int SystemNNN_gpk_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{
	my_gpk_entry_t *index_buffer;
	BYTE *lst_buffer, *name_buffer;
	DWORD *name_offset_buffer, *offset_buffer;
	DWORD index_entries, index_length;
	u32 lst_size, pk_size;

	if (pkg->pio->length_of(pkg->lst, &lst_size))
		return -CUI_ELEN;

	lst_buffer = (BYTE *)malloc(lst_size);
	if (!lst_buffer)
		return -CUI_EMEM;
	
	if (pkg->pio->readvec(pkg->lst, lst_buffer, lst_size, 0, IO_SEEK_SET)) {
		free(lst_buffer);
		return -CUI_EREAD;
	}

	index_entries = *(u32 *)lst_buffer;
	name_offset_buffer = (DWORD *)(lst_buffer + 4);
	offset_buffer = name_offset_buffer + index_entries;
	name_buffer = (BYTE *)(offset_buffer + index_entries);

	index_length = index_entries * sizeof(my_gpk_entry_t);
	index_buffer = (my_gpk_entry_t *)malloc(index_length);
	if (!index_buffer) {
		free(lst_buffer);
		return -CUI_EMEM;
	}
	
	for (DWORD i = 0; i < index_entries; i++) {
		index_buffer[i].offset = offset_buffer[i];
		strcpy(index_buffer[i].name, (char *)(name_buffer + name_offset_buffer[i]));
	}
	free(lst_buffer);

	if (pkg->pio->length_of(pkg, &pk_size))
		return -CUI_ELEN;
	
	for (i = 0; i < index_entries - 1; i++)
		index_buffer[i].length = index_buffer[i + 1].offset - index_buffer[i].offset;
	index_buffer[i].length = pk_size - index_buffer[i].offset;

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_length;
	pkg_dir->index_entry_length = sizeof(my_gpk_entry_t);
	return 0;
}

static int SystemNNN_gpk_parse_resource_info(struct package *pkg,
											 struct package_resource *pkg_res)
{
	my_gpk_entry_t *my_gpk_entry;

	my_gpk_entry = (my_gpk_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_gpk_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_gpk_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_gpk_entry->offset;

	return 0;
}

static int SystemNNN_gpk_extract_resource(struct package *pkg,
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

	uncompr = NULL;
	uncomprlen = 0;
	if (!strncmp((char *)compr, "JPEG            ", 16) || !strncmp((char *)compr, "BMP             ", 16) 
			|| !strncmp((char *)compr, "PNG             ", 16)
			|| !strncmp((char *)compr, "PACKBMP+MASK    ", 16) || !strncmp((char *)compr, "JPEG+MASK       ", 16)
			|| !strncmp((char *)compr, "BMP+MASK        ", 16)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".dwq"); 
	} else if (!strncmp((char *)compr, "IF PACKTYPE==0  ", 16) && (compr[0x39] == '0')) {
		/* TODO: 翻正 */
		uncomprlen = comprlen - 64;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}
		memcpy(uncompr, compr + 64, uncomprlen);
		if (!strncmp((char *)uncompr, "BM", 2)) {
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
			pkg_res->replace_extension = _T(".bmp"); 
		}
	}

	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

static int SystemNNN_gpk_save_resource(struct resource *res, 
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

static void SystemNNN_gpk_release_resource(struct package *pkg, 
										   struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void SystemNNN_gpk_release(struct package *pkg, 
								  struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {		
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg->lst);
	pkg->pio->close(pkg);
}

static cui_ext_operation SystemNNN_gpk_operation = {
	SystemNNN_gpk_match,				/* match */
	SystemNNN_gpk_extract_directory,	/* extract_directory */
	SystemNNN_gpk_parse_resource_info,	/* parse_resource_info */
	SystemNNN_gpk_extract_resource,		/* extract_resource */
	SystemNNN_gpk_save_resource,		/* save_resource */
	SystemNNN_gpk_release_resource,		/* release_resource */
	SystemNNN_gpk_release				/* release */
};

/********************* dwq *********************/

static int SystemNNN_dwq_match(struct package *pkg)
{
	char m_dwq[64];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, m_dwq, sizeof(m_dwq))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(m_dwq, "PACKBMP+MASK    ", 16) && strncmp(m_dwq, "BMP             ", 16) 
			&& strncmp(m_dwq, "JPEG            ", 16) && strncmp(m_dwq, "PNG             ", 16)
			&& strncmp(m_dwq, "JPEG+MASK       ", 16) && strncmp(m_dwq, "BMP+MASK        ", 16)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int SystemNNN_dwq_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	BYTE *m_pic, *m_dwqBuffer;
	char m_dwq[64];
	BOOL m_pngFlag = 0;					// @?? packtype==8
	BOOL m_jpegFlag = 0;				// @58 packtype==5(jpeg) or 7(?) 不是jpeg图就是bmp图
	BOOL m_maskFlag = 0;				// @5c packtype==3(?) or packtype==2(?) or 7(?)
	BOOL m_packFlag = 0;				// @60 packtype==3(?)
	BOOL m_alreadyCutFlag = 0;			// @64 packtype==3A(packbmp+mask) or 2A(bmp+mask) or 7A(jpeg+mask) 指的可能是mask图前面已经没有头部了
	BOOL m_packFileFlag = 0;			// @68
	DWORD m_dwqSize, m_picBufferSize;
	int m_pictureSizeX, m_pictureSizeY;
	int m_dy;							// @48 步长（height > 0: -1 else 1)
	u32 fsize;
	
	pkg->pio->length_of(pkg, &fsize);

	if (pkg->pio->readvec(pkg, m_dwq, sizeof(m_dwq), 0, IO_SEEK_SET)) 
		return -CUI_EREADVEC;

	if (m_dwq[0x39] == '2') {	// BMP+MASK
		m_maskFlag = 1;
		if (m_dwq[0x3a] == 'A')
			m_alreadyCutFlag = 1;
	} else if (m_dwq[0x39] == '3') {	
		m_maskFlag = 1;
		m_packFlag = 1;
		if (m_dwq[0x3a] == 'A')	// PACKBMP+MASK
			m_alreadyCutFlag = 1;
	} else if (m_dwq[0x39] == '5') {
		m_jpegFlag = 1;	// JPEG
	} else if (m_dwq[0x39] == '8') {
		m_pngFlag = 1;	// PNG
	} else if (m_dwq[0x39] == '7') {
		m_jpegFlag = 1;
		m_maskFlag = 1;
		if (m_dwq[0x3a] == 'A')	// JPEG+MASK
			m_alreadyCutFlag = 1;
	} else // BMP packtype=0
		;

	m_dwqSize = *(u32 *)&m_dwq[0x20];
	m_pictureSizeX = *(u32 *)&m_dwq[0x24];
	m_pictureSizeY = *(u32 *)&m_dwq[0x28];
	if (m_pictureSizeY < 0) {
		m_pictureSizeY = -m_pictureSizeY;
		m_dy = 1;
	} else
		m_dy = -1;

	m_pic = NULL;
	m_picBufferSize = 0;
	if (!m_jpegFlag && !m_pngFlag) {
		BITMAPFILEHEADER bh;
		BITMAPINFOHEADER bi;
		DWORD pitch;
		DWORD palette_size;
		
		if (pkg->pio->read(pkg, &bh, sizeof(bh)))
			return -CUI_EREAD;
		
		if (pkg->pio->read(pkg, &bi, sizeof(bi)))
			return -CUI_EREAD;

		pitch = ((m_pictureSizeX * bi.biBitCount / 8) + 3) & ~3;
		if (bi.biBitCount <= 8) {
			if (!bi.biClrUsed)
				palette_size = 1024;
			else
				palette_size = bi.biClrUsed * 4;
		} else
			palette_size = 0;
	
		if (m_maskFlag && (fsize == (sizeof(m_dwq) + *(u32 *)&m_dwq[0x20])))
			m_maskFlag = 0;

		if (m_packFlag) {
			m_dwqSize -= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + palette_size;
			m_dwqBuffer = (BYTE *)malloc(m_dwqSize);
			if (!m_dwqBuffer)
				return -CUI_EMEM;
		
			m_picBufferSize = pitch * m_pictureSizeY + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + palette_size;
			m_pic = (BYTE *)malloc(m_picBufferSize);
			if (!m_pic) {
				free(m_dwqBuffer);
				return -CUI_EMEM;
			}
			
			if (pkg->pio->read(pkg, m_pic + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER), palette_size)) {
				free(m_pic);
				free(m_dwqBuffer);
				return -CUI_EREAD;
			}
			
			if (pkg->pio->read(pkg, m_dwqBuffer, m_dwqSize)) {
				free(m_pic);
				free(m_dwqBuffer);
				return -CUI_EREAD;
			}

			LoadPackedDWQData(m_pic + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + palette_size, 
				m_dwqBuffer, m_pictureSizeX, m_pictureSizeY, pitch, m_dy);
			memcpy(m_pic, &bh, sizeof(BITMAPFILEHEADER));
			memcpy(m_pic + sizeof(BITMAPFILEHEADER), &bi, sizeof(BITMAPINFOHEADER));
		} else {
			m_dwqSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + palette_size + pitch * m_pictureSizeY;
			m_dwqBuffer = (BYTE *)malloc(m_dwqSize);
			if (!m_dwqBuffer)
			return -CUI_EMEM;
			
			if (pkg->pio->readvec(pkg, m_dwqBuffer, m_dwqSize, sizeof(m_dwq), IO_SEEK_SET)) {
				free(m_dwqBuffer);
				return -CUI_EREADVEC;
			}

			if (m_dy < 0) {
				BYTE *dst;
				BYTE *src;
				int sum;

				m_picBufferSize = m_dwqSize;
				m_pic = (BYTE *)malloc(m_picBufferSize);
				if (!m_pic) {
					free(m_dwqBuffer);
					return -CUI_EMEM;
				}

				dst = m_pic + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + palette_size;
				src = &m_dwqBuffer[(m_pictureSizeY - 1) * pitch] + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + palette_size;
				sum = pitch - m_pictureSizeX * bi.biBitCount / 8;
				for (int y = 0; y < m_pictureSizeY; y++) {		
					for (int x = 0; x < m_pictureSizeX; x++) {
						if (bi.biBitCount == 24) {
							BYTE b, g, r;

							r = *src++;
							g = *src++;
							b = *src++;
							*dst++ = b;
							*dst++ = g;
							*dst++ = r;
						} else {
							for (int p = 0; p < bi.biBitCount / 8; p++)
								*dst++ = *src++;
						}
					}
					dst += sum;
					src += sum;
					src -= pitch * 2;
				}
				memcpy(m_pic, m_dwqBuffer, sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + palette_size);
			}
		}

		if (m_maskFlag) {
			DWORD mask_size = fsize - (sizeof(m_dwq) + *(u32 *)&m_dwq[0x20]);
			BYTE *mask = (BYTE *)malloc(mask_size);
			if (!mask) {
				free(m_pic);
				free(m_dwqBuffer);
				return -CUI_EMEM;
			}

			if (pkg->pio->read(pkg, mask, mask_size)) {
				free(mask);
				free(m_pic);
				free(m_dwqBuffer);
				return -CUI_EREAD;
			}

			BITMAPFILEHEADER *m_bh = (BITMAPFILEHEADER *)mask;
			BITMAPINFOHEADER *m_bi = (BITMAPINFOHEADER *)(mask + sizeof(*m_bh));		
			DWORD m_pitch = ((m_bi->biWidth * m_bi->biBitCount / 8) + 3) & ~3;
			DWORD m_palette_size;

			if (m_bi->biBitCount <= 8) {
				if (!m_bi->biClrUsed)
					m_palette_size = 1024;
				else
					m_palette_size = m_bi->biClrUsed * 4;
			} else
				m_palette_size = 0;
				
			mask_size -= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + m_palette_size;
			DWORD *m_palette = (DWORD *)(m_bi + 1);
			BYTE *m_Buffer = (BYTE *)m_palette + m_palette_size;
			DWORD m_BufferSize = m_pitch * m_pictureSizeY;
			BYTE *m_MaskBuffer = (BYTE *)malloc(m_BufferSize);
			if (!m_MaskBuffer) {
				free(mask);
				free(m_pic);
				free(m_dwqBuffer);
				return -CUI_EMEM;
			}

			LoadPackedDWQData(m_MaskBuffer, m_Buffer, m_bi->biWidth, m_bi->biHeight, m_pitch, -1);
		
			DWORD rgba_len = m_pictureSizeX * m_pictureSizeY * 4;
			BYTE *rgba = (BYTE *)malloc(rgba_len);
			if (!rgba) {
				free(m_MaskBuffer);
				free(mask);
				free(m_pic);
				free(m_dwqBuffer);
				return -CUI_EMEM;
			}

			DWORD m_align = ((m_bi->biWidth + 3) & ~3) - m_bi->biWidth;
			BYTE *p_mask = m_MaskBuffer;
			BYTE *dst = rgba;
			if (bi.biBitCount > 8) {
				BYTE *src = m_pic + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
				DWORD src_align = ((m_pictureSizeX * bi.biBitCount / 8 + 3) & ~3) 
					- m_pictureSizeX * bi.biBitCount / 8;
				for (int y = 0; y < m_pictureSizeY; ++y) {
					for (int x = 0; x < m_pictureSizeX; ++x) {
						DWORD pal = m_palette[*p_mask++];
						BYTE m_b = (BYTE)pal;
						BYTE m_g = (BYTE)(pal >> 8);
						BYTE m_r = (BYTE)(pal >> 16);
						BYTE &a = m_b;

						*dst++ = *src++;
						*dst++ = *src++;
						*dst++ = *src++;
						*dst++ = a;
					}
					p_mask += m_align;
					src += src_align;
				}
			} else {
				DWORD *src_pal = (DWORD *)(m_pic + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
				BYTE *src = (BYTE *)src_pal + palette_size;
				for (int y = 0; y < m_pictureSizeY; ++y) {
					for (int x = 0; x < m_pictureSizeX; ++x) {
						DWORD pal = m_palette[*p_mask++];
						BYTE m_b = (BYTE)pal;
						BYTE m_g = (BYTE)(pal >> 8);
						BYTE m_r = (BYTE)(pal >> 16);
						DWORD src_p = src_pal[*src++];
						BYTE p_b = (BYTE)src_p;
						BYTE p_g = (BYTE)(src_p >> 8);
						BYTE p_r = (BYTE)(src_p >> 16);

						*dst++ = p_b;
						*dst++ = p_g;
						*dst++ = p_r;
						*dst++ = m_b;	
					}
					p_mask += m_align;
					src += m_align;
				}
			}

			if (MyBuildBMPFile(rgba, rgba_len, NULL, 0, m_pictureSizeX,
					m_pictureSizeY, 32, (BYTE **)&pkg_res->actual_data,
					&pkg_res->actual_data_length, my_malloc)) {
				free(rgba);
				free(m_MaskBuffer);
				free(mask);
				free(m_pic);
				free(m_dwqBuffer);
				return -CUI_EMEM;
			}
			free(rgba);
			free(m_MaskBuffer);
			free(mask);
			free(m_pic);
			free(m_dwqBuffer);

			pkg_res->flags |= PKG_RES_FLAG_REEXT;
			pkg_res->replace_extension = _T(".bmp"); 

			return 0;
		}
	} else {
		if (m_maskFlag) {
			m_dwqBuffer = (BYTE *)malloc(fsize);
			if (!m_dwqBuffer)
				return -CUI_EMEM;

			if (pkg->pio->readvec(pkg, m_dwqBuffer, fsize, 0, IO_SEEK_SET)) {
				free(m_dwqBuffer);
				return -CUI_EREADVEC;
			}
			m_dwqSize = fsize;
		} else {
			m_dwqBuffer = (BYTE *)malloc(m_dwqSize);
			if (!m_dwqBuffer)
				return -CUI_EMEM;

			if (pkg->pio->readvec(pkg, m_dwqBuffer, m_dwqSize, sizeof(m_dwq), IO_SEEK_SET)) {
				free(m_dwqBuffer);
				return -CUI_EREADVEC;
			}
		}
	}

	if (!strncmp(m_dwq, "JPEG            ", 16)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".jpg"); 
	} else if (!strncmp(m_dwq, "PNG             ", 16)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".png"); 
	} else if (!strncmp(m_dwq, "BMP             ", 16)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp"); 
	} else if (!strncmp(m_dwq, "PACKBMP+MASK    ", 16)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp"); 
	} else if (!strncmp(m_dwq, "JPEG+MASK       ", 16)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".jpgmask");
	} else if (!strncmp(m_dwq, "BMP+MASK        ", 16)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp"); 
	}
	
	pkg_res->raw_data = m_dwqBuffer;
	pkg_res->raw_data_length = m_dwqSize;
	pkg_res->actual_data = m_pic;
	pkg_res->actual_data_length = m_picBufferSize;

	return 0;
}

static int SystemNNN_dwq_save_resource(struct resource *res, 
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

static void SystemNNN_dwq_release_resource(struct package *pkg, 
										   struct package_resource *pkg_res)
{
	if (pkg_res->actual_data && pkg_res->actual_data_length) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res->raw_data && pkg_res->raw_data_length) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void SystemNNN_dwq_release(struct package *pkg, 
								  struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

static cui_ext_operation SystemNNN_dwq_operation = {
	SystemNNN_dwq_match,			/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	SystemNNN_dwq_extract_resource,	/* extract_resource */
	SystemNNN_dwq_save_resource,	/* save_resource */
	SystemNNN_dwq_release_resource,	/* release_resource */
	SystemNNN_dwq_release			/* release */
};
	
/********************* spt *********************/

static int SystemNNN_spt_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

static int SystemNNN_spt_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	u32 fsize;
	
	if (pkg->pio->length_of(pkg, &fsize)) 
		return -CUI_ELEN;
	
	pkg_res->raw_data = malloc(fsize);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;
	
	if (pkg->pio->readvec(pkg, pkg_res->raw_data, fsize, 0, IO_SEEK_SET)) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
		return -CUI_EREADVEC;
	}
	
	for (DWORD i = 0; i < fsize / 4; i++)
		((DWORD *)pkg_res->raw_data)[i] ^= -1;
	for (DWORD k = 0; k < (fsize & 3); k++)
		((BYTE *)(&((DWORD *)pkg_res->raw_data)[i]))[k] ^= -1;

	pkg_res->raw_data = pkg_res->raw_data;
	pkg_res->raw_data_length = fsize;

	return 0;
}

static int SystemNNN_spt_save_resource(struct resource *res, 
									   struct package_resource *pkg_res)
{
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

static void SystemNNN_spt_release_resource(struct package *pkg, 
										   struct package_resource *pkg_res)
{
	if (pkg_res && pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void SystemNNN_spt_release(struct package *pkg, 
								  struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

static cui_ext_operation SystemNNN_spt_operation = {
	SystemNNN_spt_match,			/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	SystemNNN_spt_extract_resource,	/* extract_resource */
	SystemNNN_spt_save_resource,	/* save_resource */
	SystemNNN_spt_release_resource,	/* release_resource */
	SystemNNN_spt_release			/* release */
};
	
/********************* vaw *********************/

static int SystemNNN_vaw_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

static int SystemNNN_vaw_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	char vawHeader[64];
	BYTE *riffBuffer;
	u32 riffBufferSize;
	
	if (pkg->pio->readvec(pkg, vawHeader, sizeof(vawHeader), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;
	
	if (vawHeader[0x39] == '6' || vawHeader[0x39] == '2') {
		char cut[108 - 64];

		if (pkg->pio->length_of(pkg, &riffBufferSize))
			return -CUI_ELEN;

		if (pkg->pio->readvec(pkg, cut, sizeof(cut), sizeof(vawHeader), IO_SEEK_SET))
			return -CUI_EREADVEC;
		
		riffBufferSize -= 108;
		riffBuffer = (BYTE *)malloc(riffBufferSize);
		if (!riffBuffer)
			return -CUI_EMEM;

		if (pkg->pio->readvec(pkg, riffBuffer, riffBufferSize, 108, IO_SEEK_SET)) {
			free(riffBuffer);
			return -CUI_EREADVEC;
		}
		
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".ogg");
	} else {
		char riffHeader[14];

		if (pkg->pio->readvec(pkg, riffHeader, sizeof(riffHeader), sizeof(vawHeader), IO_SEEK_SET))
			return -CUI_EREADVEC;
		
		riffBufferSize = *(DWORD *)(&riffHeader[4]) + 8;
		riffBuffer = (BYTE *)malloc(riffBufferSize);
		if (!riffBuffer)
			return -CUI_EMEM;

		if (pkg->pio->readvec(pkg, riffBuffer, riffBufferSize, sizeof(vawHeader), IO_SEEK_SET)) {
			free(riffBuffer);
			return -CUI_EREADVEC;
		}
		
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".wav");
	}
	pkg_res->raw_data = riffBuffer;
	pkg_res->raw_data_length = riffBufferSize;

	return 0;
}

static int SystemNNN_vaw_save_resource(struct resource *res, 
									   struct package_resource *pkg_res)
{
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

static void SystemNNN_vaw_release_resource(struct package *pkg, 
										   struct package_resource *pkg_res)
{
	if (pkg_res && pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void SystemNNN_vaw_release(struct package *pkg, 
								  struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

static cui_ext_operation SystemNNN_vaw_operation = {
	SystemNNN_vaw_match,			/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	SystemNNN_vaw_extract_resource,	/* extract_resource */
	SystemNNN_vaw_save_resource,	/* save_resource */
	SystemNNN_vaw_release_resource,	/* release_resource */
	SystemNNN_vaw_release			/* release */
};

/********************* wgq *********************/

static int SystemNNN_wgq_match(struct package *pkg)
{
	char wgqHeader[64];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->readvec(pkg, wgqHeader, sizeof(wgqHeader), 0, IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_EREADVEC;
	}
	
	if (strncmp(&wgqHeader[16], "OGG", 3)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int SystemNNN_wgq_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	u32 fsize;
	BYTE *wgqBuffer;
	DWORD wgqBufferSize;
	
	if (pkg->pio->length_of(pkg, &fsize))
		return -CUI_ELEN;
	
	wgqBufferSize = fsize - 64;
	wgqBuffer = (BYTE *)pkg->pio->readvec_only(pkg, wgqBufferSize, 64, IO_SEEK_SET);
	if (!wgqBuffer)
		return -CUI_EREADVECONLY;

	pkg_res->raw_data = wgqBuffer;
	pkg_res->raw_data_length = wgqBufferSize;

	return 0;
}

static void SystemNNN_wgq_release_resource(struct package *pkg, 
										   struct package_resource *pkg_res)
{
	if (pkg_res && pkg_res->raw_data)
		pkg_res->raw_data = NULL;
}

static cui_ext_operation SystemNNN_wgq_operation = {
	SystemNNN_wgq_match,			/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	SystemNNN_wgq_extract_resource,	/* extract_resource */
	SystemNNN_vaw_save_resource,	/* save_resource */
	SystemNNN_wgq_release_resource,	/* release_resource */
	SystemNNN_vaw_release			/* release */
};


/********************* vpk *********************/

static int SystemNNN_vpk_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->open(pkg->lst, IO_READONLY)) {
		pkg->pio->close(pkg);
		return -CUI_EOPEN;
	}

	return 0;	
}

static int SystemNNN_vpk_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{
	vtb_entry_t *vtb_buffer;
	my_vtb_entry_t *index_buffer;
	DWORD index_entries, index_length;
	u32 vtb_size;

	if (pkg->pio->length_of(pkg->lst, &vtb_size))
		return -CUI_ELEN;

	vtb_buffer = (vtb_entry_t *)malloc(vtb_size);
	if (!vtb_buffer)
		return -CUI_EMEM;
	
	if (pkg->pio->read(pkg->lst, vtb_buffer, vtb_size)) {
		free(vtb_buffer);
		return -CUI_EREAD;
	}

	index_entries = (vtb_size / sizeof(vtb_entry_t)) - 1;
	
	index_length = index_entries * sizeof(my_vtb_entry_t);
	index_buffer = (my_vtb_entry_t *)malloc(index_length);
	if (!index_buffer) {
		free(vtb_buffer);
		return -CUI_EMEM;
	}

	for (DWORD i = 0; i < index_entries; i++) {
		memset(index_buffer[i].name, 0, sizeof(index_buffer[i].name));
		strncpy(index_buffer[i].name, vtb_buffer[i].name, 8);
		index_buffer[i].offset = vtb_buffer[i].offset;
		index_buffer[i].length = vtb_buffer[i+1].offset - vtb_buffer[i].offset;
	}
	free(vtb_buffer);
	
	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_length;
	pkg_dir->index_entry_length = sizeof(my_vtb_entry_t);
	return 0;
}

static int SystemNNN_vpk_parse_resource_info(struct package *pkg,
											 struct package_resource *pkg_res)
{
	my_vtb_entry_t *my_vtb_entry;

	my_vtb_entry = (my_vtb_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_vtb_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_vtb_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_vtb_entry->offset;

	return 0;
}

static int SystemNNN_vpk_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	pkg_res->raw_data = (BYTE *)pkg->pio->readvec_only(pkg, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET);
	if (!pkg_res->raw_data)
		return -CUI_EREADVECONLY;

	pkg_res->flags |= PKG_RES_FLAG_REEXT;
	pkg_res->replace_extension = _T(".vaw");
	return 0;
}

static int SystemNNN_vpk_save_resource(struct resource *res, 
									   struct package_resource *pkg_res)
{
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

static cui_ext_operation SystemNNN_vpk_operation = {
	SystemNNN_vpk_match,				/* match */
	SystemNNN_vpk_extract_directory,	/* extract_directory */
	SystemNNN_vpk_parse_resource_info,	/* parse_resource_info */
	SystemNNN_vpk_extract_resource,		/* extract_resource */
	SystemNNN_vpk_save_resource,		/* save_resource */
	SystemNNN_wgq_release_resource,		/* release_resource */
	SystemNNN_gpk_release				/* release */
};

/********************* jpgmask *********************/

static int SystemNNN_jpgmask_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	char m_dwq[64];

	if (pkg->pio->read(pkg, m_dwq, sizeof(m_dwq))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (m_dwq[0x39] != '7') {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int SystemNNN_jpgmask_extract_directory(struct package *pkg,
											   struct package_directory *pkg_dir)
{
	char m_dwq[64];

	if (pkg->pio->readvec(pkg, m_dwq, sizeof(m_dwq), 0, IO_SEEK_SET)) 
		return -CUI_EREADVEC;

	u32 fsize;
	pkg->pio->length_of(pkg, &fsize);

	my_gpk_entry_t *index = (my_gpk_entry_t *)malloc(2 * sizeof(my_gpk_entry_t));
	if (!index)
		return -CUI_EMEM;

	char name[MAX_PATH];
	unicode2acp(name, MAX_PATH, pkg->name, -1);
	*strstr(name, ".jpgmask") = 0;

	sprintf(index[0].name, "%s.jpg", name);
	index[0].offset = sizeof(m_dwq);
	index[0].length = *(u32 *)&m_dwq[0x20];
	sprintf(index[1].name, "%s.mask.bmp", name);
	index[1].offset = index[0].offset + index[0].length;
	index[1].length = fsize - index[1].offset;

	pkg_dir->index_entries = 2;
	pkg_dir->directory = index;
	pkg_dir->directory_length = 2 * sizeof(my_gpk_entry_t);
	pkg_dir->index_entry_length = sizeof(my_gpk_entry_t);

	return 0;
}

static int SystemNNN_jpgmask_extract_resource(struct package *pkg,
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

	if (pkg_res->index_number == 1) {	// mask
		BITMAPFILEHEADER *bh = (BITMAPFILEHEADER *)raw;
		BITMAPINFOHEADER *bi = (BITMAPINFOHEADER *)(raw + sizeof(*bh));		
		DWORD pitch = ((bi->biWidth * bi->biBitCount / 8) + 3) & ~3;
		DWORD palette_size;
		DWORD m_dwqSize = pkg_res->raw_data_length;

		if (bi->biBitCount <= 8) {
			if (!bi->biClrUsed)
				palette_size = 1024;
			else
				palette_size = bi->biClrUsed * 4;
		} else
			palette_size = 0;
			
		m_dwqSize -= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + palette_size;
		BYTE *m_dwqBuffer = (BYTE *)(bi + 1) + palette_size;
			
		DWORD m_picBufferSize = pitch * bi->biHeight + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + palette_size;
		BYTE *m_pic = (BYTE *)malloc(m_picBufferSize);
		if (!m_pic) {
			free(raw);
			return -CUI_EMEM;
		}

		LoadPackedDWQData(m_pic + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + palette_size, 
			m_dwqBuffer, bi->biWidth, bi->biHeight, pitch, -1);
		memcpy(m_pic, raw, sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + palette_size);
		pkg_res->actual_data = m_pic;
		pkg_res->actual_data_length = m_picBufferSize;
	}

	pkg_res->raw_data = raw;

	return 0;
}

static cui_ext_operation SystemNNN_jpgmask_operation = {
	SystemNNN_jpgmask_match,			/* match */
	SystemNNN_jpgmask_extract_directory,/* extract_directory */
	SystemNNN_gpk_parse_resource_info,	/* parse_resource_info */
	SystemNNN_jpgmask_extract_resource,	/* extract_resource */
	SystemNNN_gpk_save_resource,		/* save_resource */
	SystemNNN_gpk_release_resource,		/* release_resource */
	SystemNNN_gpk_release				/* release */
};

int CALLBACK SystemNNN_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".gpk"), _T(".dwq"), 
		NULL, &SystemNNN_gpk_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_LST | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dwq"), NULL, 
		NULL, &SystemNNN_dwq_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".jpgmask"), NULL, 
		NULL, &SystemNNN_jpgmask_operation, CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_RES))
			return -1;
	
	if (callback->add_extension(callback->cui, _T(".spt"), NULL, 
		NULL, &SystemNNN_spt_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_NO_MAGIC))
			return -1;
	
	if (callback->add_extension(callback->cui, _T(".fxf"), _T(".fxf.txt"), 
		NULL, &SystemNNN_spt_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;
	
	if (callback->add_extension(callback->cui, _T(".vaw"), NULL, 
		NULL, &SystemNNN_vaw_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;
	
	if (callback->add_extension(callback->cui, _T(".wgq"), _T(".ogg"), 
		NULL, &SystemNNN_wgq_operation, CUI_EXT_FLAG_PKG))
			return -1;
	
	if (callback->add_extension(callback->cui, _T(".vpk"), NULL, 
		NULL, &SystemNNN_vpk_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_LST | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;
	
	return 0;
}
