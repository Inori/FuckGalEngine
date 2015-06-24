/* Circus cui 0.10
 * 痴汉公贼 (C) 2007.02.17
 *  
 * ChangeLog:
 * 2007.02.17 ver 0.10	支持pck导出
 * 2007.02.13 ver 0.01	最初版本 
 */

#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <resource.h>
#include <package.h>
#include <cui_error.h>
#include <utility.h>
#include <zlib.h>
#include <stdio.h>
	
struct acui_information CIRCUS_cui_information = {
	_T("CIRCUS"),				/* copyright */
	NULL,						/* system */
	_T(".CRX .PCK .pcm"),		/* package */
	_T("0.2.2"),				/* revison */
	_T("痴汉公贼"),				/* author */
	_T("2007-5-2 11:51"),		/* date */
	NULL,						/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};	

static const char *pcm_hack_info = "Extracted By 痴汉公贼";

#pragma pack (1)
typedef struct {
	u32 entries;
} dat_header_t;

typedef struct {
	u8 name[60];
	u32 offset;	
} dat_entry_t;

typedef struct {
	s8 magic[4];	/* CRXG */
	u32 zero;
	u16 width;
	u16 height;
	u16 mode;
	u16 unknown1;	// must be <= 1
	u32 color_type;
} crx_header_t;

typedef struct {
	u32 entries;
/* 接下来是length和offset快速索引（每项8字节） */
/*
	typedef struct {
		u32 offset;
		u32 length;
	} entry_t;
*/
} pck_header_t;

typedef struct {
	u32 offset;
	u32 length;
} __pck_entry_t;

typedef struct {
	s8 name[56];
	u32 offset;
	u32 length;
} pck_entry_t;

typedef struct {
	s8 magic[4];		/* XPCM */
	u32 length;			/* data length */
	u8 mode;			/* 0 - raw wave */ 
	u8 reserved[3];
	u16 wFormatTag;
	u16 nChannels;
	u32 nSamplesPerSec;
	u32 nAvgBytesPerSec;
	u16 nBlockAlign;
	u16 wBitsPerSample;
} pcm_header_t;

typedef struct {
	s8 magic[4];		/* XPCM */
	u32 length;			/* data length */
	u8 mode;			/* 1 - ?? */ 
	u8 reserved[3];
} pcm1_header_t;

typedef struct {
	s8 magic[4];		/* XPCM */
	u32 length;			/* data length */
	u8 mode;			/* 2 - ?? */ 
	u8 reserved[3];
} pcm2_header_t;

typedef struct {
	s8 magic[4];		/* XPCM */
	u32 length;			/* length(需要重新计算) */
	u8 mode;			/* 3 - zlib */ 
	u8 reserved[3];
	u16 wFormatTag;
	u16 nChannels;
	u32 nSamplesPerSec;
	u32 nAvgBytesPerSec;
	u16 nBlockAlign;
	u16 wBitsPerSample;
	u32 compr_length;
	/* BYTE *compr_data */
} pcm3_header_t;

typedef struct {
	s8 ChunkID[4];
	u32 ChunkSize;
	s8 Format[4];
	s8 Subchunk1ID[4];
	u32 Subchunk1Size;
	u16 wFormatTag;
	u16 nChannels;
	u32 nSamplesPerSec;
	u32 nAvgBytesPerSec;
	u16 nBlockAlign;
	u16 wBitsPerSample;
	//WAVEFORMATEX wave;
	s8 Subchunk2ID[4];
	u32 Subchunk2Size;
} wav_header_t;
#pragma pack ()	


static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static void __CIRCUS_crx_defilting(BYTE *dst_base, BYTE *src, int width, 
								   int height, int bpp)
{
	int p, x, y, w;
	int raw_line_len = width * bpp;
	int line_len = (raw_line_len + 3) & ~3;

	for (y = height - 1; y >= 0; y--) {
		BYTE type = *src++;
		BYTE *dst = dst_base + y * line_len;
		BYTE *pre_dst = dst + line_len;

		switch (type) {
		case 0:	// sub
			for (p = 0; p < bpp; p++)
				dst[p] = src[p];
			for (x = bpp; x < raw_line_len; x++)
				dst[x] = src[x] + dst[x - bpp];
			src += raw_line_len;
			break;
		case 1:	// up
			for (x = 0; x < raw_line_len; x++)
				dst[x] = src[x] + pre_dst[x];	
			src += raw_line_len;
			break;
		case 2:
			for (p = 0; p < bpp; p++)
				dst[p] = src[p];
			for (x = bpp; x < raw_line_len; x++)
				dst[x] = src[x] + pre_dst[x - bpp];
			src += raw_line_len;
			break;
		case 3:
			for (x = 0; x < (width - 1) * bpp; x++)
				dst[x] = src[x] + pre_dst[x + bpp];
			for (p = 0; p < bpp; p++)
				dst[x + p] = src[x + p];
			src += raw_line_len;
			break;
		case 4:	// rle
			for (p = 0; p < bpp; p++) {
				w = width;
				while (w > 0) {
					BYTE rep;
					
					rep = *src++;
					*dst = rep;
					dst += bpp;
					w--;
					if (!w)
						break;
					if (rep == *src) {
						BYTE cp;

						src++;
						cp = *src++;
						for (BYTE k = 0; k < cp; k++) {
							*dst = rep;
							dst += bpp;
						}
						w -= cp;
					}
				}
				dst -= raw_line_len;
				dst++;	/* 移动到下一个分量 */
			}
			break;
		default:
			break;
		}
	}

	if (bpp == 4) {
		for (int i = 0; i < width * height; i++) {
			BYTE *pixel = (BYTE *)&((u32 *)dst_base)[i];
			BYTE alpha = pixel[0];
			pixel[0] = pixel[1];
			pixel[1] = pixel[2];
			pixel[2] = pixel[3];
			pixel[3] = alpha;
		}
	}
	// 后处理
	if (bpp == 4) {
		for (int h = 0; h < height; h++) {
			int esi = (h & 1) * 3;

			for (int w = 0;	w < width; w++) {
				BYTE *pixel;
				BYTE alpha;
				int b, g, r;
					
				pixel = (BYTE *)&((u32 *)dst_base)[w];
				alpha =	pixel[3];
				if (alpha != 0xff) {
					b = pixel[0];
					g = pixel[1];
					r = pixel[2];

					b += (w & 1) + esi;
					if (b < 0)
						b = 0;
					else if (b > 0xff)
						b = 0xff;

					g += (w & 1) + esi;
					if (g < 0)
						g = 0;
					else if (g > 0xff)
						g = 0xff;

					r += (w & 1) + esi;
					if (r < 0)
						r = 0;
					else if (r > 0xff)
						r = 0xff;

					pixel[0] = b;
					pixel[1] = g;
					pixel[2] = r;
				}
				esi = 0 - esi;
			}			
			dst_base += line_len;
		}
	} else if (bpp == 3) {
		BYTE *pixel = dst_base;

		for (int h = 0; h < height; h++) {
			int esi = (h & 1) * 3;

			for (int w = 0;	w < width; w++) {
				int b, g, r;

				b = pixel[0];
				g = pixel[1];
				r = pixel[2];
				if (b != 0xff || g || r != b) {
					b += (w & 1) + esi;
					if (b < 0)
						b = 0;
					else if (b > 0xff)
						b = 0xff;

					g += (w & 1) + esi;
					if (g < 0)
						g = 0;
					else if (g > 0xff)
						g = 0xff;

					r += (w & 1) + esi;
					if (r < 0)
						r = 0;
					else if (r > 0xff)
						r = 0xff;

					pixel[0] = b;
					pixel[1] = g;
					pixel[2] = r;
				}
				esi = 0 - esi;
				pixel += 3;
			}			
		}
	}
}

#if 0
static void __CIRCUS_crx_defilting(u8 *dst, u8 *src, unsigned int width, 
								   unsigned int height, unsigned int bits_count)
{
	u8 *dst_cur_line;
	u8 *dst_prev_line;
	u8 *pprev, *pdst;
	unsigned int line_len;	
	unsigned int bytes_per_pixel;
	unsigned int x, y, p;
	
	bytes_per_pixel = bits_count / 8;
	line_len = width * bytes_per_pixel;
		
	//if (((width * bytes_per_pixel + 3) & ~3) != line_len) {
	//	printf("line len miss-match\n");
	//	exit(0);
	//} 

	for (y = 0; y < height; y++) {
		u8 flag = *src++;

		dst_cur_line = &dst[(height - 1 - y) * line_len];
		dst_prev_line = &dst[(height - y) * line_len];		
		
		if (flag == 0) {
			if (bytes_per_pixel == 4) {
				dst_cur_line[3] = ~*src++;
				for (p = 0; p < 3; p++)
					dst_cur_line[p] = *src++;	
			} else {
				for (p = 0; p < bytes_per_pixel; p++)
					dst_cur_line[p] = *src++;				
			}
				
			for (x = 1; x < width; x++) {
				pprev = &dst_cur_line[(x - 1) * bytes_per_pixel];
				pdst = &dst_cur_line[x * bytes_per_pixel];
					
				if (bytes_per_pixel == 4) {
					pdst[3] = ~(~pprev[3] + *src++);
					for (p = 0; p < 3; p++)
						pdst[p] = pprev[p] + *src++;
				} else {
					for (p = 0; p < bytes_per_pixel; p++)
						pdst[p] = pprev[p] + *src++;				
				}
			}
		} else if (flag	== 1) {
			for (x = 0; x < width; x++) {
				pprev = &dst_prev_line[x * bytes_per_pixel];
				pdst = &dst_cur_line[x * bytes_per_pixel];
					
				if (bytes_per_pixel == 4) {
					pdst[3] = ~(~pprev[3] + *src++);
					for (p = 0; p < 3; p++)
						pdst[p] = pprev[p] + *src++;
				} else {
					for (p = 0; p < bytes_per_pixel; p++)
						pdst[p] = pprev[p] + *src++;				
				}
			}
		} else if (flag == 2) {
			if (bytes_per_pixel == 4) {
				dst_cur_line[3] = ~*src++;
				for (p = 0; p < 3; p++)
					dst_cur_line[p] = *src++;	
			} else {
				for (p = 0; p < bytes_per_pixel; p++)
					dst_cur_line[p] = *src++;				
			}
						
			for (x = 1; x < width; x++) {
				pprev = &dst_prev_line[(x - 1) * bytes_per_pixel];
				pdst = &dst_cur_line[x * bytes_per_pixel];
					
				if (bytes_per_pixel == 4) {
					pdst[3] = ~(~pprev[3] + *src++);
					for (p = 0; p < 3; p++)
						pdst[p] = pprev[p] + *src++;
				} else {
					for (p = 0; p < bytes_per_pixel; p++)
						pdst[p] = pprev[p] + *src++;				
				}			
			}
		} else if (flag == 3) {
			for (x = 0; x < width - 1; x++) {
				pprev = &dst_prev_line[(x + 1) * bytes_per_pixel];
				pdst = &dst_cur_line[x * bytes_per_pixel];
					
				if (bytes_per_pixel == 4) {
					pdst[3] = ~(~pprev[3] + *src++);
					for (p = 0; p < 3; p++)
						pdst[p] = pprev[p] + *src++;
				} else {
					for (p = 0; p < bytes_per_pixel; p++)
						pdst[p] = pprev[p] + *src++;				
				}
			}
			
			pdst = &dst_cur_line[x * bytes_per_pixel];
			
			if (bytes_per_pixel == 4) {
				pdst[3] = ~*src++;
				for (p = 0; p < 3; p++)
					pdst[p] = *src++;	
			} else {
				for (p = 0; p < bytes_per_pixel; p++)
					pdst[p] = *src++;				
			}
		} else if (flag == 4) {
#if 0
			for (p = 0; p < bytes_per_pixel; p++) {
				u8 *tmp_dst_p = dst_cur_line;
				x = 0;
			
				if ((bytes_per_pixel == 4) && !p)
					tmp_dst_p += 3;
				else if (bytes_per_pixel == 4)
					tmp_dst_p += p - 1;
				else
					tmp_dst_p += p;
					
				while (1) {
					u8 rep_byte;
					
					if (x >= width)
						break;
					rep_byte = *src++;	
					tmp_dst_p[x++ * bytes_per_pixel] = ((bytes_per_pixel == 4) && !p) ? ~rep_byte : rep_byte;
					if (rep_byte == *src) {
						u8 copy_bytes;
									
						copy_bytes = src[1];
						if (x + copy_bytes > width)
							break;

						src += 2;
						
						if ((bytes_per_pixel == 4) && !p)
							rep_byte = ~rep_byte;	
					
						for (unsigned int c = 0; c < copy_bytes; c++)
							tmp_dst_p[x++ * bytes_per_pixel] = rep_byte;
					}
				}
			}	
#else			
			u8 *tmp_dst_p = dst_cur_line;
		
			for (p = 0; p < bytes_per_pixel; p++) {
				if (bytes_per_pixel == 4 && p == 0)
					tmp_dst_p += 3;
				x = width;
				while (x > 0) {
					u8 rep_byte;

					rep_byte = *src++;
					*tmp_dst_p = (bytes_per_pixel == 4 && p == 0) ? ~rep_byte : rep_byte;
					tmp_dst_p += bytes_per_pixel;

					if (!--x) {
						//printf("sdf\n");
						break;
					}

					if (rep_byte == *src) {
						u8 copy_bytes;
						
						copy_bytes = *++src;
						x -= copy_bytes;

						if (x < 0) {
							printf("x %x copy_bytes %x \n", x, copy_bytes);
							break;
						}

						if (bytes_per_pixel == 4 && p == 0)
							rep_byte = ~rep_byte;						
						while (copy_bytes--) {
							*tmp_dst_p = rep_byte;
							tmp_dst_p += bytes_per_pixel;
						}
						src++;
					}
				}
				if (bytes_per_pixel == 4 && p == 0)
					tmp_dst_p -= 4;	
				tmp_dst_p -= line_len - 1;
			}
#endif			
		}
	}		

	/* Alpha-Blending */
	if (bytes_per_pixel == 4) {
		for (y = 0; y < height; y++) {
			for (x = 0; x < width; x++) {
				u8 alpha = dst[3];
				
				/* 人物自身的alpha是0xff(不透明）; 背景部分的alpha是0x00；人物边缘是0x00~0xff之间 */
				if (!alpha) {	
					//((255 - buf32[3]) * pBgRGB[i] + buf32[3] * buf32[i]) / 255;
					for (p = 0; p < 3; p++)					
						/* 假设背景全黑(0x00) */
						dst[p] = (0xff * 0x00) / 255;
				}
				dst += 4;
			}
		}
	}
}
#endif
/********************* crx *********************/

static int CIRCUS_crx_match(struct package *pkg)
{
	crx_header_t crx_header;

	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &crx_header, sizeof(crx_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(crx_header.magic, "CRXG", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (crx_header.mode != 1 && crx_header.mode != 2) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}
	
	if (crx_header.unknown1 > 1) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;
}

static void CIRCUS_crx_decompress1(BYTE *uncompr, DWORD uncomprlen, BYTE *compr, DWORD comprlen)
{
#if 0
	WORD flag = 0;
	DWORD act_uncomprlen = 0;
	int line_len0 = ;
	int line_len1 = ;

	while (1) {
		flag >>= 1;
		if (!(flag & 1))
			flag = *compr++ | 0xff00;

		if (flag & 1) {
			uncompr[act_uncomprlen++] = *compr++;
			act_uncomprlen &= 0xffff;
			line_len--;
			if (!line_len)
				continue;

			if (act_uncomprlen > raw_line_len1) {

			} else {

			}

		} else {
			BYTE sub_flag = *compr++;

			if (sub_flag >= 0xc0) {
				offset = ((sub_flag & 3) << 8) | *compr++;
				count = (sub_flag >> 2) & 0x0f;
				count += 4;
			} else if (sub_flag & 0x80) {
				offset = sub_flag & 0x1f;
				count = (sub_flag >> 5) & 3;
				if (!offset)
					offset = *compr++;
				count += 2;
			}

			i = act_uncomprlen - offset;
			for (int k = 0; k < count; k++) {
				uncompr[act_uncomprlen++] = uncompr[i & 0xffff];
				line_len--;
				i++;
				if (!line_len) {

				}
			}
		}
	}
#endif
}

static int CIRCUS_crx_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	crx_header_t crx_header;
	unsigned int bits_count;
	BYTE *uncompr, *compr, *palette;
	DWORD comprlen, uncomprlen, palette_size;
	unsigned long crx_len;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	if (pkg->pio->length_of(pkg, &crx_len))
		return -CUI_ELEN;

	if (pkg->pio->readvec(pkg, &crx_header, sizeof(crx_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	comprlen = crx_len - sizeof(crx_header);
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, sizeof(crx_header), IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	bits_count = !crx_header.color_type ? 24 : (crx_header.color_type == 1 ? 32 : 8);
	uncomprlen = ((crx_header.width * bits_count / 8 + 3) & ~3) * crx_header.height;
	uncompr = (BYTE *)malloc(uncomprlen);
	if (!uncompr) {
		free(compr);
		return -CUI_EMEM;
	}

	palette = NULL;
	palette_size = 0;
	if (crx_header.mode == 1) {
		CIRCUS_crx_decompress1(uncompr, uncomprlen, compr, comprlen);
	} else if (crx_header.mode == 2) {
		DWORD act_len;
		BYTE *act_data;

		if (crx_header.color_type > 1) {
			palette_size = crx_header.color_type * 3;
			palette = (BYTE *)malloc(palette_size);
			if (!palette) {
				free(uncompr);
				free(compr);
				return -CUI_EMEM;
			}
			memcpy(palette, compr, palette_size);
		}

		/* 因为每行开头的第一个字节表示这行的数据应该如何处理, 因此
		 * 实际的解压长度比原始图像数据长度多了一列.
		 */
		act_len = ((crx_header.width * bits_count / 8) + 1) 
			* crx_header.height;	// 程序中还要多加100字节
		act_data = (u8 *)malloc(act_len);
		if (!uncompr) {
			free(uncompr);
			free(compr);
			return -CUI_EMEM;
		}
//		memset(act_data, 0, act_len);
printf("%x %x\n",act_len,crx_header.color_type);
int ret=uncompress(act_data, &act_len, compr + palette_size, comprlen - palette_size);
		if (ret != Z_OK) {
			free(act_data);
			free(uncompr);
			free(compr);
printf("sdf %x \n",ret);
			return -CUI_EUNCOMPR;
		}

		if (crx_header.color_type <= 1) {
			__CIRCUS_crx_defilting(uncompr, act_data, crx_header.width, 
				crx_header.height, bits_count / 8);
		} else {	/* 实际情况，8位色图像通过调色版转为24位色，然后经过__CIRCUS_crx_defilting处理 */
			BYTE *dst, *src;
			int line_len, aligned_line_len;

			line_len = crx_header.width;
			aligned_line_len = (line_len + 3) & ~3;
			dst = uncompr + aligned_line_len * (crx_header.height - 1);
			src = act_data;
			for (int h = 0; h < crx_header.height; h++) {
				memcpy(dst, src, line_len);
				dst -= aligned_line_len;
				src += line_len;
			}
		}
		free(act_data);
	}

	if (MyBuildBMPFile(uncompr, uncomprlen, palette, palette_size, 
			crx_header.width, crx_header.height, bits_count, 
				(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, my_malloc)) {
		free(uncompr);
		free(compr);
		return -CUI_EMEM;
	}
	free(uncompr);
	pkg_res->raw_data = compr;
	pkg_res->raw_data_length = comprlen;

	return 0;
}

static int CIRCUS_crx_save_resource(struct resource *res, 
									struct package_resource *pkg_res)
{
	if (!res || !pkg_res)
		return -CUI_EPARA;

	if (res->rio->create(res))
		return -CUI_ECREATE;
	if (res->rio->write(res, pkg_res->actual_data, pkg_res->actual_data_length)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}
	res->rio->close(res);
	
	return 0;
}

static void CIRCUS_crx_release_resource(struct package *pkg, 
										struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void CIRCUS_crx_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (!pkg)
		return;

	pkg->pio->close(pkg);	
}

static cui_ext_operation CIRCUS_crx_operation = {
	CIRCUS_crx_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	CIRCUS_crx_extract_resource,	/* extract_resource */
	CIRCUS_crx_save_resource,		/* save_resource */
	CIRCUS_crx_release_resource,	/* release_resource */
	CIRCUS_crx_release				/* release */
};

/********************* pck *********************/

static int CIRCUS_pck_match(struct package *pkg)
{
	pck_header_t pck_header;
	
	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &pck_header, sizeof(pck_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if ((LONG)pck_header.entries <= 0) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->pio->seek(pkg, 0, IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_ESEEK;
	}

	return 0;
}

static int CIRCUS_pck_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	if (!pkg || !pkg_dir)
		return -CUI_EPARA;

	if (pkg->pio->read(pkg, &pkg_dir->index_entries, 4))
		return -CUI_EREAD;

	/* 跨过快速索引段 */
	if (pkg->pio->seek(pkg, pkg_dir->index_entries * sizeof(__pck_entry_t), IO_SEEK_CUR))
		return -CUI_ESEEK;

	pkg_dir->directory_length = pkg_dir->index_entries * sizeof(pck_entry_t);
	pkg_dir->directory = malloc(pkg_dir->directory_length);
	if (!pkg_dir->directory)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, pkg_dir->directory, pkg_dir->directory_length)) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
		return -CUI_EREAD;
	}

	pkg_dir->index_entry_length = sizeof(pck_entry_t);
	pkg_dir->flags |= PKG_DIR_FLAG_SKIP0;
		
	return 0;
}

static int CIRCUS_pck_parse_resource_info(struct package *pkg,
										  struct package_resource *pkg_res)
{
	pck_entry_t *pck_entry;

	if (!pkg || !pkg_res)
		return -1;

	pck_entry = (pck_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, pck_entry->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = pck_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = pck_entry->offset;

	return 0;
}

static int CIRCUS_pck_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	BYTE *dat;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	dat = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!dat)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, dat, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET)) {
		free(dat);
		return -CUI_EREADVEC;
	}

	pkg_res->raw_data = dat;

	return 0;
}

static int CIRCUS_pck_save_resource(struct resource *res, 
									struct package_resource *pkg_res)
{
	if (!res || !pkg_res)
		return -CUI_EPARA;
	
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

static void CIRCUS_pck_release_resource(struct package *pkg, 
										struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void CIRCUS_pck_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (!pkg || !pkg_dir)
		return;

	pkg->pio->close(pkg);	
}

static cui_ext_operation CIRCUS_pck_operation = {
	CIRCUS_pck_match,				/* match */
	CIRCUS_pck_extract_directory,	/* extract_directory */
	CIRCUS_pck_parse_resource_info,	/* parse_resource_info */
	CIRCUS_pck_extract_resource,	/* extract_resource */
	CIRCUS_pck_save_resource,		/* save_resource */
	CIRCUS_pck_release_resource,	/* release_resource */
	CIRCUS_pck_release				/* release */
};

/********************* pcm *********************/

static int CIRCUS_pcm_match(struct package *pkg)
{
	pcm_header_t *pcm_header;
	
	if (!pkg)
		return -CUI_EPARA;

	pcm_header = (pcm_header_t *)malloc(sizeof(*pcm_header));
	if (!pcm_header)
		return -CUI_EMEM;

	if (pkg->pio->open(pkg, IO_READONLY)) {
		free(pcm_header);
		return -CUI_EOPEN;
	}

	if (pkg->pio->read(pkg, pcm_header, sizeof(*pcm_header))) {
		pkg->pio->close(pkg);
		free(pcm_header);
		return -CUI_EREAD;
	}

	if (strncmp(pcm_header->magic, "XPCM", 4)) {
		pkg->pio->close(pkg);
		free(pcm_header);
		return -CUI_EMATCH;
	}

	if (pcm_header->mode && (pcm_header->mode != 3)) {
		_stprintf(fmt_buf, _T("%s: unsupported mode %d\n"), pkg->name, pcm_header->mode);
		wcprintf_error(fmt_buf);
		pkg->pio->close(pkg);
		free(pcm_header);
		return -CUI_EPARA;
	}
	package_set_private(pkg, pcm_header);

	return 0;
}

static int CIRCUS_pcm_parse_resource_info(struct package *pkg,
										  struct package_resource *pkg_res)
{
	pcm_header_t *pcm_header;
	DWORD riff_chunk_len, fmt_chunk_len, data_chunk_len;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	pcm_header = (pcm_header_t *)package_get_private(pkg);	
	if (!pcm_header->mode) {
		pkg_res->raw_data_length = pcm_header->length - sizeof(pcm_header_t);
		pkg_res->offset = sizeof(pcm_header_t);
		data_chunk_len = pcm_header->length;
	} else if (pcm_header->mode == 3) {
		u32 comprlen;

		if (pkg->pio->read(pkg, &comprlen, sizeof(comprlen)))
			return -CUI_EREAD;

		pkg_res->offset = sizeof(pcm_header_t) + 4;
		pkg_res->raw_data_length = comprlen;
	//	data_chunk_len = (((pcm_header->length + 0x800 - 1) & ~(0x800 - 1))) * 2 + 0x4002;
		data_chunk_len = (pcm_header->length & ~(0x1000 - 1)) + 0x4002;
	}
	
	//data_chunk_len &= ~(pcm_header->nBlockAlign - 1);
	fmt_chunk_len = 16;	/* PCM时不包括2字节cbSize */
	riff_chunk_len = 4 + (8 + fmt_chunk_len) + (8 + data_chunk_len);
	pkg_res->actual_data_length = 8 + riff_chunk_len + strlen(pcm_hack_info);

	return 0;
}

static int CIRCUS_pcm_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	pcm_header_t *pcm_header;
	wav_header_t *wav_header;
	DWORD riff_chunk_len, fmt_chunk_len, data_chunk_len, act_uncomprlen;
	DWORD comprlen, uncomprlen;
	BYTE *compr, *uncompr;
	BYTE *p;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	uncomprlen = pkg_res->actual_data_length;
	uncompr = (BYTE *)malloc(uncomprlen);
	if (!uncompr) {
		free(compr);
		return -CUI_EMEM;
	}

	pcm_header = (pcm_header_t *)package_get_private(pkg);
	wav_header = (wav_header_t *)uncompr;
	p = (BYTE *)(wav_header + 1);

	switch (pcm_header->mode) {
	case 0:
		data_chunk_len = comprlen;
		memcpy(p, compr, data_chunk_len);
		free(compr);
		break;
	case 3:
		//data_chunk_len = (((pcm_header->length + 0x800 - 1) & ~(0x800 - 1))) * 2 + 0x4002;
		//data_chunk_len = (pcm_header->length & ~(0x1000 - 1)) + 0x4002;
		data_chunk_len = (pcm_header->length & ~(0x1000 - 1)) + 0x8002;
		act_uncomprlen = data_chunk_len;		
		if (uncompress(p, &act_uncomprlen, compr, comprlen) != Z_OK) {
			_stprintf(fmt_buf, _T("%s: decompress error occured (%d VS %d)\n"), 
				pkg->name, data_chunk_len, act_uncomprlen);
			wcprintf_error(fmt_buf);
			free(uncompr);
			free(compr);
			return -CUI_EUNCOMPR;		
		}
		data_chunk_len = act_uncomprlen;
		free(compr);
		break;
	}

	//data_chunk_len &= ~(pcm_header->nBlockAlign - 1);
	fmt_chunk_len = 16;	/* PCM时不包括2字节cbSize */
	riff_chunk_len = 4 + (8 + fmt_chunk_len) + (8 + data_chunk_len);
	strncpy((char *)wav_header->ChunkID, "RIFF", 4);
	wav_header->ChunkSize = riff_chunk_len;
	strncpy((char *)wav_header->Format, "WAVE", 4);
	strncpy((char *)wav_header->Subchunk1ID, "fmt ", 4);
	wav_header->Subchunk1Size = 16;
	wav_header->wFormatTag = pcm_header->wFormatTag;
	wav_header->nChannels = pcm_header->nChannels;
	wav_header->nSamplesPerSec = pcm_header->nSamplesPerSec;
	wav_header->nAvgBytesPerSec = pcm_header->nAvgBytesPerSec;
	wav_header->nBlockAlign = pcm_header->nBlockAlign;
	wav_header->wBitsPerSample = pcm_header->wBitsPerSample;
	strncpy((char *)wav_header->Subchunk2ID, "data", 4);
	wav_header->Subchunk2Size = data_chunk_len;	
	p += data_chunk_len;
	strncpy((char *)p, pcm_hack_info, strlen(pcm_hack_info));

	pkg_res->raw_data = NULL;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = 8 + riff_chunk_len + strlen(pcm_hack_info);

	return 0;
}

static int CIRCUS_pcm_save_resource(struct resource *res, 
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
	}
	res->rio->close(res);
	
	return 0;
}

static void CIRCUS_pcm_release_resource(struct package *pkg, 
										struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
}

static void CIRCUS_pcm_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	pcm_header_t *pcm_header;
	
	if (!pkg)
		return;

	pcm_header = (pcm_header_t *)package_get_private(pkg);
	if (pcm_header) {
		free(pcm_header);
		package_set_private(pkg, NULL);
	}

	pkg->pio->close(pkg);	
}

static cui_ext_operation CIRCUS_pcm_operation = {
	CIRCUS_pcm_match,				/* match */
	NULL,							/* extract_directory */
	CIRCUS_pcm_parse_resource_info,	/* parse_resource_info */
	CIRCUS_pcm_extract_resource,	/* extract_resource */
	CIRCUS_pcm_save_resource,		/* save_resource */
	CIRCUS_pcm_release_resource,	/* release_resource */
	CIRCUS_pcm_release				/* release */
};

/********************* dat *********************/

static int CIRCUS_dat_match(struct package *pkg)
{
	dat_header_t *dat_header;
	unsigned long dat_size;
	
	if (!pkg)
		return -CUI_EPARA;

	dat_header = (dat_header_t *)malloc(sizeof(*dat_header));
	if (!dat_header)
		return -CUI_EMEM;

	if (pkg->pio->open(pkg, IO_READONLY)) {
		free(dat_header);
		return -CUI_EOPEN;
	}

	if (pkg->pio->length_of(pkg, &dat_size)) {
		pkg->pio->close(pkg);
		free(dat_header);
		return -CUI_ELEN;
	}

	if (pkg->pio->read(pkg, dat_header, sizeof(*dat_header))) {
		pkg->pio->close(pkg);
		free(dat_header);
		return -CUI_EREAD;
	}

	if ((LONG)dat_header->entries < 2) {
		pkg->pio->close(pkg);
		free(dat_header);
		return -CUI_EMATCH;
	}

#if 1
	int err = 0;
	for (unsigned int i = 0; i < dat_header->entries; i++) {
		dat_entry_t dat_entry;

		if (pkg->pio->read(pkg, &dat_entry, sizeof(dat_entry))) {
			err = -CUI_EREAD;
			break;
		}

		if (dat_entry.name[59] || (dat_entry.offset >= dat_size)) {
			err = -CUI_EMATCH;
			break;
		}
	}
	if (i != dat_header->entries) {
		pkg->pio->close(pkg);
		free(dat_header);
		return err;
	}
#else
	if (lstrcmpi(pkg->name, _T("voice.dat"))) {
		pkg->pio->close(pkg);
		free(dat_header);
		return -CUI_EMATCH;
	}
#endif
	package_set_private(pkg, dat_header);

	return 0;
}

static int CIRCUS_dat_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	dat_header_t *dat_header;
	
	if (!pkg || !pkg_dir)
		return -CUI_EPARA;

	dat_header = (dat_header_t *)package_get_private(pkg);	
	pkg_dir->directory_length = dat_header->entries * sizeof(dat_entry_t);
	pkg_dir->directory = malloc(pkg_dir->directory_length);
	if (!pkg_dir->directory)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_dir->directory, pkg_dir->directory_length,
			sizeof(dat_header_t), IO_SEEK_SET)) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
		return -CUI_EREAD;
	}
	pkg_dir->index_entries = dat_header->entries;
	pkg_dir->index_entry_length = sizeof(dat_entry_t);
	
	return 0;
}

static int CIRCUS_dat_parse_resource_info(struct package *pkg,
										  struct package_resource *pkg_res)
{
	dat_entry_t *dat_entry;
	pcm_header_t pcm_header;
	u32 comprlen;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	dat_entry = (dat_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, (char *)dat_entry->name);
	pkg_res->name_length = -1;
	pkg_res->offset = dat_entry->offset;

	if (pkg->pio->readvec(pkg, &pcm_header, sizeof(pcm_header_t),
			dat_entry->offset, IO_SEEK_SET))
		return -CUI_EREADVEC;

	switch (pcm_header.mode) {
	case 0:
		pkg_res->raw_data_length = pcm_header.length + sizeof(pcm_header_t);
		break;
	case 3:
		if (pkg->pio->readvec(pkg, &comprlen, sizeof(comprlen), 
				sizeof(pcm_header_t) + dat_entry->offset, IO_SEEK_SET))
			return -CUI_EREADVEC;
		pkg_res->raw_data_length = sizeof(pcm_header_t) + 4 + comprlen;
		break;
	default:	// TODO
		pkg_res->raw_data_length = pcm_header.length + sizeof(pcm_header_t);
		break;
	}

	return 0;
}

static void CIRCUS_dat_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	dat_header_t *dat_header;
	
	if (!pkg || !pkg_dir)
		return;

	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	dat_header = (dat_header_t *)package_get_private(pkg);
	if (dat_header) {
		free(dat_header);
		package_set_private(pkg, NULL);
	}

	pkg->pio->close(pkg);
}

static cui_ext_operation CIRCUS_dat_operation = {
	CIRCUS_dat_match,				/* match */
	CIRCUS_dat_extract_directory,	/* extract_directory */
	CIRCUS_dat_parse_resource_info,	/* parse_resource_info */
	NULL,							/* extract_resource */
	NULL,							/* save_resource */
	NULL,							/* release_resource */
	CIRCUS_dat_release				/* release */
};

#if 0
/********************* pcm3 *********************/

static inline DWORD pcm3_make_length(DWORD len)
{
/*
00424E3A  |.  B8 09040281   MOV EAX,81020409                         ;  ebp是xpcm.length
00424E3F  |.  F7ED          IMUL EBP
00424E41  |.  03D5          ADD EDX,EBP
00424E43  |.  C1FA 0B       SAR EDX,0B
00424E46  |.  8BC2          MOV EAX,EDX
00424E48  |.  C1E8 1F       SHR EAX,1F
00424E4B  |.  03D0          ADD EDX,EAX
00424E4D  |.  C1E2 0C       SHL EDX,0C
00424E50  |.  8DB2 02400000 LEA ESI,DWORD PTR DS:[EDX+4002]
*/
	//return (len & ~(0x1000 - 1)) + 0x8000;
	//return ((len + 0x1000 - 1) & ~(0x1000 - 1)) + 0x4002;
	return (((len + 0x800 - 1) & ~(0x800 - 1))) * 2 + 0x4002;
}

	memset((BYTE *)pkg_res->uncompr + act_uncomprlen, 0, 
		pkg_res->uncompr_length - act_uncomprlen);
	pkg_res->actual_buffer = pkg_res->uncompr;
	pkg_res->actual_length = act_uncomprlen;	
#endif

int CALLBACK CIRCUS_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".CRX"), _T(".BMP"), 
		NULL, &CIRCUS_crx_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".PCK"), NULL, 
		NULL, &CIRCUS_pck_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".pcm"), _T(".WAV"), 
		NULL, &CIRCUS_pcm_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

//	if (callback->add_extension(callback->cui, _T(".DAT"), NULL, 
//		NULL, &CIRCUS_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NO_MAGIC))
//			return -1;

#if 0
	if (callback->add_extension(callback->handle, _T(".pcm"), _T(".WAV"), 
		_T("XPCM Raw Wav"), &wav_package_header, &CIRCUS_wav_operation, 
			CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
				return -1;

	if (callback->add_extension(callback->handle, _T(".pcm"), _T(".WAV"), 
		_T("XPCM 3"), &pcm3_package_header, &CIRCUS_pcm3_operation, 
			CUI_EXT_FLAG_RES))
				return -1;

	if (callback->add_extension(callback->handle, _T(".pcm"), _T(".WAV"), 
		_T("XPCM 2"), &pcm2_package_header, &CIRCUS_pcm2_operation, 
			CUI_EXT_FLAG_RES))
				return -1;

	if (callback->add_extension(callback->handle, _T(".pcm"), _T(".WAV"), 
		_T("XPCM 1"), &pcm1_package_header, &CIRCUS_pcm1_operation, 
			CUI_EXT_FLAG_RES))
				return -1;
#endif

	return 0;
}
