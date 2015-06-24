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
#include <cui_common.h>

/*
audio: OKI adocm
http://www.google.cn/codesearch/p?hl=zh-CN#1CJyVSVDnBc/scummvm-0.8.0/sound/adpcm.cpp&q=%22oki%20adpcm%22
http://www.google.cn/codesearch/p?hl=zh-CN#RfeYQwakdYw/scummvm-0.7.0/saga/sound.cpp&q=%22oki%20adpcm%22
http://www.google.cn/codesearch/p?hl=zh-CN#P8wqyUK7yIM/snd-9/snd-trans.c&q=%22oki%20adpcm%22
http://www.google.cn/codesearch/p?hl=zh-CN#dqInl-z_jOA/advancemame-0.106.0/src/sound/okim6295.c&q=%22oki%20adpcm%22
http://www.google.cn/codesearch/p?hl=zh-CN#yPGMWgS6ti4/chuck-1.2.0.8/src/util_sndfile.c&q=%22oki%20adpcm%22
http://www.google.cn/codesearch/p?hl=zh-CN#O4DTS4WnyEw/xmovie-1.9.13/libsndfile-1.0.5/src/vox_adpcm.c&q=%22oki%20adpcm%22


*/

struct acui_information CIRCUS_cui_information = {
	_T("CIRCUS"),				/* copyright */
	NULL,						/* system */
	_T(".CRX .PCK .pcm .dat"),	/* package */
	_T("0.3.2"),				/* revison */
	_T("痴漢公賊"),				/* author */
	_T("2008-4-24 12:05"),		/* date */
	NULL,						/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};	

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
	u32 color_type;	// only 16 bit
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

typedef struct {		// 28 bytes
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

typedef struct {
	char name[MAX_PATH];
	unsigned int offset;
	unsigned int length;	
} my_dat_entry_t;


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
}

static void CIRCUS_crx_decompress1(BYTE *uncompr, DWORD uncomprlen, BYTE *compr, DWORD comprlen)
{
	WORD flag;
	BYTE window[64 * 1024];
	DWORD win_pos, act_uncomprlen;

	memset(window, 0, sizeof(window));
	flag = 0;
	win_pos = 0;
	act_uncomprlen = 0;
	while (act_uncomprlen < uncomprlen) {
		flag >>= 1;
		if (!(flag & 0x0100))
			flag = *compr++ | 0xff00;

		if (flag & 1) {
			BYTE dat;

			dat = *compr++;
			window[win_pos++] = dat;
			win_pos &= 0xffff;
			uncompr[act_uncomprlen++] = dat;
		} else {
			BYTE sub_flag = *compr++;
			DWORD count, offset;

			if (sub_flag >= 0xc0) {
				offset = ((sub_flag & 3) << 8) | *compr++;
				count = (sub_flag >> 2) & 0xf;
				count += 4;
			} else if (sub_flag & 0x80) {
				offset = sub_flag & 0x1f;
				count = (sub_flag >> 5) & 3;
				if (!offset)
					offset = *compr++;
				count += 2;
			} else if (sub_flag == 0x7f) {
				count = *compr++;
				count |= *compr++ << 8;
				offset = *compr++;
				offset |= *compr++ << 8;
				count += 2;
			} else {
				offset = *compr++;
				offset |= *compr++ << 8;
				count = sub_flag + 4;
			}

			offset = win_pos - offset;
			for (DWORD k = 0; k < count; k++) {
				BYTE dat;

				offset &= 0xffff;
				dat = window[offset];
				window[win_pos] = dat;
				win_pos++;
				win_pos &= 0xffff;
				offset++;
				uncompr[act_uncomprlen++] = dat;
				if (act_uncomprlen >= uncomprlen)
					break;
			}
		}
	}
}

/********************* crx *********************/

static int CIRCUS_crx_match(struct package *pkg)
{
	crx_header_t crx_header;

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

static int CIRCUS_crx_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	crx_header_t crx_header;
	unsigned int bits_count;
	BYTE *uncompr, *compr, *palette;
	DWORD comprlen, uncomprlen, palette_size, line_len;
	u32 crx_len;

	if (pkg->pio->length_of(pkg, &crx_len))
		return -CUI_ELEN;

	if (pkg->pio->readvec(pkg, &crx_header, sizeof(crx_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	crx_header.color_type &= 0xffff;

	comprlen = crx_len - sizeof(crx_header);
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, sizeof(crx_header), IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	bits_count = !crx_header.color_type ? 24 : (crx_header.color_type == 1 ? 32 : 8);
	line_len = (crx_header.width * bits_count / 8 + 3) & ~3;

	if (crx_header.color_type > 1) {
		palette_size = crx_header.color_type * 3;
		palette = compr;
	} else {
		palette = NULL;
		palette_size = 0;
	}

	if (crx_header.mode == 1) {
		uncomprlen = (crx_header.width * bits_count / 8) * crx_header.height;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}

		CIRCUS_crx_decompress1(uncompr, uncomprlen, compr + palette_size, comprlen - palette_size);

		if (crx_header.color_type > 1) {
			BYTE *pal = palette;

			for (unsigned int k = 0; k < crx_header.color_type; k++) {
				BYTE b, g, r;

				r = pal[0];
				g = pal[1];
				b = pal[2];
				if (b == 0xff && !g && r == 0xff)
					g = 0xff;
				pal[0] = b;
				pal[1] = g;
				pal[2] = r;
				pal += 3;
			}
		}
	} else if (crx_header.mode == 2) {
		DWORD act_len;
		BYTE *act_data;

		uncomprlen = line_len * crx_header.height;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
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

		uncompress(act_data, &act_len, compr + palette_size, comprlen - palette_size);

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

	if (crx_header.color_type == 1) {
		for (int i = 0; i < crx_header.width * crx_header.height; i++) {
			BYTE *pixel = (BYTE *)&((u32 *)uncompr)[i];
			BYTE alpha = pixel[0];
			pixel[0] = pixel[1];
			pixel[1] = pixel[2];
			pixel[2] = pixel[3];
			pixel[3] = alpha;
		}
	}

	// 后处理
	if (crx_header.color_type == 1) {
		BYTE *rgba = uncompr;

		for (int h = 0; h < crx_header.height; h++) {
			int esi = (h & 1) * 3;

			for (int w = 0;	w < crx_header.width; w++) {
				BYTE *pixel;
				BYTE alpha;
				int b, g, r;
					
				pixel = (BYTE *)&((u32 *)rgba)[w];
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
			rgba += line_len;
		}
	} else if (!crx_header.color_type) {
		BYTE *pixel = uncompr;

		for (int h = 0; h < crx_header.height; h++) {
			int esi = (h & 1) * 3;

			for (int w = 0;	w < crx_header.width; w++) {
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

	/* Alpha Blending */
	if (crx_header.color_type == 1) {
		BYTE *rgba = uncompr;
		for (int y = 0; y < crx_header.height; y++) {
			for (int x = 0; x < crx_header.width; x++) {
				BYTE alpha = ~rgba[3];
				
				rgba[0] = (rgba[0] * alpha + 0xff * ~alpha) / 255;
				rgba[1] = (rgba[1] * alpha + 0xff * ~alpha) / 255;
				rgba[2] = (rgba[2] * alpha + 0xff * ~alpha) / 255;	
				rgba[3] = alpha;
				rgba += 4;
			}
		}
	} else if (crx_header.color_type == 0) {
		BYTE *rgba = uncompr;
		if (crx_header.mode == 1)
			line_len = crx_header.width * 3;
		for (int y = 0; y < crx_header.height; y++) {
			BYTE *pixel = rgba;
			for (int x = 0; x < crx_header.width; x++) {
				if (pixel[0] == 0xff && !pixel[1] && pixel[2] == 0xff)
					pixel[1] = 0xff;
				pixel += 3;
			}
			rgba += line_len;
		}
	}

	if (crx_header.mode == 2) {
		if (MyBuildBMPFile(uncompr, uncomprlen, palette, palette_size, 
				crx_header.width, crx_header.height, bits_count, 
					(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, my_malloc)) {
			free(uncompr);
			free(compr);
			return -CUI_EMEM;
		}
	} else {
		if (MyBuildBMPFile(uncompr, uncomprlen, palette, palette_size, 
				crx_header.width, 0 - crx_header.height, bits_count, 
					(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, my_malloc)) {
			free(uncompr);
			free(compr);
			return -CUI_EMEM;
		}
	}
	free(uncompr);
	free(compr);
	pkg_res->raw_data = NULL;
	pkg_res->raw_data_length = crx_len;

	return 0;
}

static int CIRCUS_crx_save_resource(struct resource *res, 
									struct package_resource *pkg_res)
{
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
	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
}

static void CIRCUS_crx_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
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
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void CIRCUS_pck_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
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
	pcm_header_t pcm_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &pcm_header, sizeof(pcm_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(pcm_header.magic, "XPCM", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pcm_header.mode) {
	//	_stprintf(fmt_buf, _T("%s: unsupported pcm mode %d\n"), 
	//		pkg->name, pcm_header.mode);
	//	wcprintf_error(fmt_buf);
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;
}

static int CIRCUS_pcm_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	pcm_header_t pcm_header;
	u32 pcm_size;

	if (pkg->pio->length_of(pkg, &pcm_size))
		return -CUI_ELEN;

	if (pkg->pio->readvec(pkg, &pcm_header, sizeof(pcm_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	void *data_chunk;
	DWORD data_chunk_len;
	if (pcm_header.mode == 0) {
		data_chunk_len = pcm_header.length;
		data_chunk = malloc(data_chunk_len);
		if (!data_chunk)
			return -CUI_EMEM;
		if (pkg->pio->read(pkg, data_chunk, data_chunk_len)) {
			free(data_chunk);
			return -CUI_EREAD;
		}
	} else if (pcm_header.mode == 3) {
		DWORD comprlen;

		if (pkg->pio->read(pkg, &comprlen, 4))
			return -CUI_EREAD;

		void *compr = malloc(comprlen);
		if (!compr)
			return -CUI_EMEM;

		if (pkg->pio->read(pkg, compr, comprlen)) {
			free(compr);
			return -CUI_EREAD;
		}

		data_chunk_len = comprlen * 2;
		while (1) {
			data_chunk = malloc(data_chunk_len);
			if (!data_chunk) {
				free(compr);
				return -CUI_EMEM;
			}

			DWORD act_uncomprlen = data_chunk_len;
			if (uncompress((BYTE *)data_chunk, &act_uncomprlen, (BYTE *)compr, comprlen) == Z_OK) {
				data_chunk_len = act_uncomprlen;
				break;
			}
			free(data_chunk);	
			data_chunk_len <<= 1;
		}
		free(compr);
	}

	if (MySaveAsWAVE(data_chunk, data_chunk_len, pcm_header.wFormatTag,
			pcm_header.nChannels, pcm_header.nSamplesPerSec, 
			pcm_header.wBitsPerSample, NULL, 0, 
			(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length,
			my_malloc)) {
		free(data_chunk);
		return -CUI_EMEM;
	}
	free(data_chunk);
	pkg_res->raw_data = NULL;
	pkg_res->raw_data_length = pcm_size;

	return 0;
}

static int CIRCUS_pcm_save_resource(struct resource *res, 
									struct package_resource *pkg_res)
{
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
	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
}

static void CIRCUS_pcm_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	pcm_header_t *pcm_header;

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
	NULL,							/* parse_resource_info */
	CIRCUS_pcm_extract_resource,	/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* dat *********************/

static int CIRCUS_dat_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

#if 0
	dat_header_t *dat_header;
	u32 dat_size;

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
#endif

	return 0;
}

static int CIRCUS_dat_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	dat_header_t dat_header;
	if (pkg->pio->read(pkg, &dat_header, sizeof(dat_header)))
		return -CUI_EREAD;

	DWORD index_length = dat_header.entries * sizeof(dat_entry_t);
	dat_entry_t *index = (dat_entry_t *)malloc(index_length);
	if (!index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index, index_length)) {
		free(index);
		return -CUI_EREAD;
	}

	u32 dat_size;
	if (pkg->pio->length_of(pkg, &dat_size)) {
		free(index);
		return -CUI_ELEN;
	}
	
	DWORD my_index_length = sizeof(my_dat_entry_t) * dat_header.entries;
	my_dat_entry_t *my_index = (my_dat_entry_t *)malloc(my_index_length);
	if (!my_index) {
		free(index);
		return -CUI_EMEM;
	}

	for (DWORD i = 0; i < dat_header.entries - 1; i++) {
		my_index[i].offset = index[i].offset;
		my_index[i].length = index[i + 1].offset - index[i].offset;
		strcpy(my_index[i].name, (char *)index[i].name);
	}
	my_index[i].offset = index[i].offset; 
	my_index[i].length = dat_size - index[i].offset;
	strcpy(my_index[i].name, (char *)index[i].name);
	free(index);

	pkg_dir->index_entries = dat_header.entries;
	pkg_dir->index_entry_length = sizeof(my_dat_entry_t);
	pkg_dir->directory = my_index;
	pkg_dir->directory_length = my_index_length;
	
	return 0;
}

static int CIRCUS_dat_parse_resource_info(struct package *pkg,
										  struct package_resource *pkg_res)
{
	my_dat_entry_t *my_dat_entry;

	my_dat_entry = (my_dat_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_dat_entry->name);
	pkg_res->name_length = -1;
	pkg_res->offset = my_dat_entry->offset;
	pkg_res->raw_data_length = my_dat_entry->length;

#if 0
	dat_entry_t *dat_entry;
	pcm_header_t pcm_header;
	u32 comprlen;

	dat_entry = (dat_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, (char *)dat_entry->name);
	pkg_res->name_length = -1;
	pkg_res->offset = dat_entry->offset;
	pkg_res->raw_data_length = dat_entry->offset;

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
#endif
	return 0;
}

static int CIRCUS_dat_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	BYTE *raw = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET)) {
		free(raw);
		return -CUI_EREADVEC;
	}

	pkg_res->raw_data = raw;

	return 0;
}

static cui_ext_operation CIRCUS_dat_operation = {
	CIRCUS_dat_match,				/* match */
	CIRCUS_dat_extract_directory,	/* extract_directory */
	CIRCUS_dat_parse_resource_info,	/* parse_resource_info */
	CIRCUS_dat_extract_resource,
	CUI_COMMON_OPERATION
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

	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &CIRCUS_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

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
