#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <resource.h>
#include <package.h>
#include <cui_error.h>
#include <utility.h>
#include "mt19937int.h"
#include <stdio.h>

/*
H:\tools\sketchFE1.31.19\sketch
J:\eve_x_taiken
H:\Crass\test_files\ExHIBIT\clear_crystal_cm\Clear僋儕僗僞儖僗僩乕儕乕僘撪梕徯夘懱尡斉
N:\MOONSTONE\magiski
E:\Primary_trial\Primary_trial
*/

struct acui_information ExHIBIT_cui_information = {
	_T("RETOUCH"),									/* copyright */
	_T("ADVGオーサリングシステム sketch / ExHIBIT"),/* system */
	_T(".gyu .grp"),								/* package */
	_T("0.3.1"),									/* revison */
	_T("痴漢公賊"),									/* author */
	_T("2009-6-21 21:21"),							/* date */
	NULL,											/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};	

#pragma pack (1)
typedef struct {
	s8 magic[4];	/* "GYU\x1a" */
	u32 mode;		/* 0x02000000 - cmode: SLD; 0x02000000 - SLD2(在SLD的基础上再次加密) */
	u32 key;		/* 官方的工具中，有个gyu程序，执行gyu 9 xxx.gyu可以擦除这个key为0，这样解密这张图的key就没有了 */
	u32 bits_count;
	u32 width;
	u32 height;
	u32 length;
	u32 alpha_length;
	u32 palette_colors;
} gyu_header_t;

typedef struct {
	u32 version;
	u32 width;
	u32 height;
	u32 pages;
	u32 flags;			// bit0 - loop(0:off; 1:on); bit8-15 - colors(0x800:256 colors; other:full colors)
	u32 wait;
	u32 reserved[2];	// 0
} dlt_header_t;

typedef struct {
/*
format = flags & 0xf00;
if (format == 0xf00)
	スナップデータ
else if (format == 0xe00)
	palette data
else if (format == 0xe00)
	０畳み込み
else if (format == 0x200)
	LZ
else
	ベタ
	
if (flags & 4) 更新領域無し else if (flags & 2) 全域 else see dlt_page_pos_t
*/
	u32 flags;
	u32 size;
} dlt_page_t;

typedef struct {
	u32 x_orig;
	u32 y_orig;
	u32 width;
	u32 height;
} dlt_page_pos_t;

typedef struct {
	u32 mask_data_length;
} dlt_page_mask_t;

typedef struct {
	s8 zero;		// 0
	u16 block_size;
	u16 unknown;
	u32 bits_count;
	u32 mode;		// 1 - ADPCM; 2 - PCM; 3 - PCM; 4 - PCM22M; 6 - PCM44M; 7 - ADPCM44
} v_header_t;

typedef struct {
	s8 magic[4];	// "AiFS"
	u16 unknown0;	// 0
	u16 unknown1;	// 5
	u32 unknown;	// 0x20
	u32 res_files;
} grp_header_t;

typedef struct {
	u32 id;				// 从1开始
	u32 start_res_id;
	u32 end_res_id;
	u32 res_entries;
} grp_entry_t;

typedef struct {
	u32 offset;
	u32 length;
} grp_info_entry_t;

typedef struct {
	s8 magic[4];	// "\x00DLR"
	u32 mode;
	u32 unknown;	// 0x110
	u32 unknown1;	// 0x20
} rld_header_t;
#pragma pack ()	

typedef struct {
	char name[16];
	DWORD offset;
	DWORD length;
} my_dlt_entry_t;

static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static inline unsigned char getbit_le(unsigned char byte, unsigned int pos)
{
	return !!(byte & (1 << pos));
}

static DWORD lzss_decompress(BYTE *uncompr, DWORD uncomprlen, 
							 BYTE *compr, DWORD comprlen)
{
	unsigned int act_uncomprlen = 0;
	/* compr中的当前字节中的下一个扫描位的位置 */
	unsigned int curbit = 0;
	/* compr中的当前扫描字节 */
	unsigned int curbyte = 0;
	unsigned int nCurWindowByte = 0xfee;
	unsigned int win_size = 4096;
	BYTE win[4096];
	WORD flag = 0;

	memset(win, 0, nCurWindowByte);
	while (curbyte < comprlen) {
		flag >>= 1;
		if (!(flag & 0x0100)) {
			flag = compr[curbyte++] | 0xff00;
			if (curbyte >= comprlen)
				break;
		}
		if (flag & 1) {
			unsigned char data;

			if (act_uncomprlen >= uncomprlen)
				break;
			data = compr[curbyte++];
			uncompr[act_uncomprlen++] = data;
			/* 输出的1字节放入滑动窗口 */
			win[nCurWindowByte++] = data;
			nCurWindowByte &= win_size - 1;
		} else {
			unsigned int copy_bytes, win_offset;
			unsigned int i;

			win_offset = compr[curbyte++];
			if (curbyte >= comprlen)
				break;
			copy_bytes = compr[curbyte++];
			win_offset |= (copy_bytes >> 4) << 8;
			copy_bytes = copy_bytes & 0x0f;
			copy_bytes += 3;

			for (i = 0; i < copy_bytes; i++) {	
				unsigned char data;

				if (act_uncomprlen >= uncomprlen)
					break;
				data = win[(win_offset + i) & (win_size - 1)];
				uncompr[act_uncomprlen++] = data;
				/* 输出的1字节放入滑动窗口 */
				win[nCurWindowByte++] = data;
				nCurWindowByte &= win_size - 1;	
			}
		}
	}

	return act_uncomprlen;
}

static DWORD gyu_new_uncompress(BYTE *uncompr, BYTE *compr)
{
	BYTE *org_uncompr = uncompr;
	BYTE flag = 0;
	BYTE flag_bits = 1;

	compr += 4;
	*uncompr++ = *compr++;
	while (1) {
		--flag_bits;
		if (!flag_bits) {
			flag = *compr++;
			flag_bits = 8;
		}

		int b = flag & 0x80;
		flag <<= 1;

		if (b)
			*uncompr++ = *compr++;
		else {
			--flag_bits;
			if (!flag_bits) {
				flag = *compr++;
				flag_bits = 8;
			}

			DWORD count, offset;

			b = flag & 0x80;
			flag <<= 1;
			if (!b) {
				--flag_bits;
				if (!flag_bits) {
					flag = *compr++;
					flag_bits = 8;
				}

				count = (!!(flag & 0x80)) * 2;
				flag <<= 1;
				--flag_bits;
				if (!flag_bits) {
					flag = *compr++;
					flag_bits = 8;
				}
				count += (!!(flag & 0x80)) + 1;
				offset = *compr++ - 256;
				flag <<= 1;
			} else {
				offset = compr[1] | (compr[0] << 8);
				compr += 2;
				count = offset & 7;
				offset = (offset >> 3) - 8192;				
				if (count)
					++count;
				else {
					count = *compr++;
					if (!count)
						break;
				}
			}

			++count;
			BYTE *src = uncompr + offset;
			for (DWORD i = 0; i < count; ++i)
				*uncompr++ = *src++;
		}
	}

	return uncompr - org_uncompr;
}

/********************* gyu *********************/

static int ExHIBIT_gyu_match(struct package *pkg)
{
	gyu_header_t *gyu_header;

	gyu_header = (gyu_header_t *)malloc(sizeof(*gyu_header));
	if (!gyu_header)
		return -CUI_EMEM;

	if (pkg->pio->open(pkg, IO_READONLY)) {
		free(gyu_header);
		return -CUI_EOPEN;
	}

	if (pkg->pio->read(pkg, gyu_header, sizeof(*gyu_header))) {
		pkg->pio->close(pkg);
		free(gyu_header);
		return -CUI_EREAD;
	}

	if (strncmp(gyu_header->magic, "GYU\x1a", 4) || !gyu_header->length) {
		pkg->pio->close(pkg);
		free(gyu_header);
		return -CUI_EMATCH;
	}

	if (!gyu_header->key) {
		const char *key, *dll_path;

		//wcprintf(pkg->name);
		//printf(": key erased ...\n");

		key = get_options("key");
		dll_path = get_options("resident");
		if (key) {
			gyu_header->key = strtoul(key, NULL, 16);
		} else if (dll_path) {
			//44ae590 空色
			HMODULE dll;
			u32 (__stdcall *RetouchSystem_dk)(u32, u32);

			dll = LoadLibraryA(dll_path);
			if (!dll) {
				crass_printf(_T("not found resident.dll\n"));
				pkg->pio->close(pkg);
				free(gyu_header);
				return -CUI_EMATCH;			
			}

			RetouchSystem_dk = (u32 (__stdcall *)(u32, u32))GetProcAddress(dll, "?dk@RetouchSystem@@UAEKHH@Z");
			if (!RetouchSystem_dk) {				
				RetouchSystem_dk = (u32 (__stdcall *)(u32, u32))GetProcAddress(dll, "?dk@RetouchSystem@@MAEKHH@Z");
				if (!RetouchSystem_dk) {
					FreeLibrary(dll);
					pkg->pio->close(pkg);
					free(gyu_header);
					return -CUI_EMATCH;
				}
			}
			gyu_header->key = (*RetouchSystem_dk)(_ttoi(pkg->name), 0);
			FreeLibrary(dll);
		} else {
			crass_printf(_T("[WARNING] %s: the key is erased from gyu by author, so you may use either ")
				_T("key or dll parameter to specify it.\n"), pkg->name);
			return -CUI_EMATCH;
		}		
	} 
	package_set_private(pkg, gyu_header);

	return 0;	
}

static int ExHIBIT_gyu_extract_resource(struct package *pkg,
										struct package_resource *pkg_res)
{
	gyu_header_t *gyu_header;
	BYTE *palette, *compr, *uncompr, *actbuf, *alpha;
	unsigned int palette_size, line_len;
	DWORD uncomprlen, comprlen, act_uncomprlen, actlen, alpha_length;
	u32 mode;

	gyu_header = (gyu_header_t *)package_get_private(pkg);
#if 0
	printf("width %x, height %x, bpp %d, alpha %x, len %x, mode %x, colors %x\n", 
	   gyu_header->width, gyu_header->height, 
	   gyu_header->bits_count, gyu_header->alpha_length, 
	   gyu_header->length, gyu_header->mode, gyu_header->palette_colors
   );
#endif
	if (gyu_header->palette_colors) {
		palette_size = gyu_header->palette_colors * 4;
		palette = (BYTE *)malloc(palette_size);
		if (!palette)
			return -CUI_EMEM;		

		if (pkg->pio->read(pkg, palette, palette_size)) {
			free(palette);
			return -CUI_EREAD;
		}
	} else {
		palette_size = 0;
		palette = NULL;
	}

	comprlen = gyu_header->length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr) {
		if (palette)	
			free(palette);
		return -CUI_EMEM;					
	}

	if (pkg->pio->read(pkg, compr, comprlen)) {
		free(compr);		
		if (palette)
			free(palette);
		return -CUI_EREAD;
	}

	if (gyu_header->mode & 1) {
		alpha_length = gyu_header->alpha_length;
		alpha = (BYTE *)malloc(alpha_length);
		if (!alpha) {
			free(compr);		
			if (palette)	
				free(palette);
			return -CUI_EMEM;
		}

		if (pkg->pio->read(pkg, alpha, alpha_length)) {
			free(alpha);
			free(compr);		
			if (palette)	
				free(palette);
			return -CUI_EREAD;
		}

		uncomprlen = ((gyu_header->width + 3) & ~3) * gyu_header->height;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(alpha);
			free(compr);		
			if (palette)	
				free(palette);
			return -CUI_EMEM;
		}
		memset(uncompr, 0, uncomprlen);
		act_uncomprlen = lzss_decompress(uncompr, uncomprlen, alpha, alpha_length);
		free(alpha);
		if (act_uncomprlen != uncomprlen) {
			free(uncompr);	
			free(compr);		
			if (palette)	
				free(palette);
			return -CUI_EUNCOMPR;
		}
		alpha = uncompr;
		alpha_length = uncomprlen;
	} else {
		alpha_length = 0;
		alpha = NULL;
	}

	if (gyu_header->mode & 0xc0) {
		printf("unsupported mode %x\n", gyu_header->mode);
		exit(0);
	}

	/*
00422978  |.  8B51 04       MOV EDX,DWORD PTR DS:[ECX+4]             ;  [4]mode
0042297B  |.  81E2 00800000 AND EDX,8000                             ;  [4] & 0x8000
00422981  |.  F7DA          NEG EDX                                  ;  0 - ([4] & 0x8000)
00422983  |.  1BD2          SBB EDX,EDX
00422985  |.  83E2 03       AND EDX,3
00422988  |.  8950 10       MOV DWORD PTR DS:[EAX+10],EDX            ;  biCompression
	*/
	line_len = ((gyu_header->width * gyu_header->bits_count / 8) + 3) & ~3;
	mode = gyu_header->mode & 0xffff0000;
	if (mode == 0x02000000 || mode == 0x04000000 || mode == 0x08000000) {			
		uncomprlen = line_len * gyu_header->height;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			if (alpha)
				free(alpha);
			free(compr);
			if (palette)	
				free(palette);
			return -CUI_EMEM;		
		}
		memset(uncompr, 0, uncomprlen);

		if (gyu_header->key != -1) {
			sgenrand(gyu_header->key);

			for (int i = 0; i < 10; i++) {
				BYTE tmp;
				unsigned long rand0, rand1;

				rand0 = genrand() % comprlen;
				rand1 = genrand() % comprlen;
				tmp = compr[rand1];
				compr[rand1] = compr[rand0];
				compr[rand0] = tmp;
			}
		}

		if (mode == 0x08000000)
			act_uncomprlen = gyu_new_uncompress(uncompr, compr);
		else
			act_uncomprlen = lzss_decompress(uncompr, uncomprlen, compr, comprlen);

		if (act_uncomprlen != uncomprlen) {
			printf("%x %x\n", act_uncomprlen, uncomprlen);
			if (alpha)
				free(alpha);			
			free(uncompr);		
			free(compr);
			if (palette)	
				free(palette);		
			return -CUI_EUNCOMPR;	
		}

		free(compr);
		compr = NULL;
		actbuf = uncompr;
		actlen = uncomprlen;
	} else if (mode == 0x1000000) {
		actbuf = compr;
		actlen = comprlen;		
	}

	if (alpha && alpha_length) {	// alpha合成
		BYTE *a = alpha;

		DWORD dib32_line_len = gyu_header->width * 4;
		DWORD dib32_len = dib32_line_len * gyu_header->height;
		BYTE *dib32 = (BYTE *)malloc(dib32_len);
		if (!dib32) {
			if (alpha)
				free(alpha);			
			free(uncompr);		
			free(compr);
			if (palette)	
				free(palette);	
			return -CUI_EMEM;
		}

		if (gyu_header->palette_colors) {
			BYTE *src_line = actbuf;
			BYTE *dst_line = dib32;
			// alpha的值在[0,16]之间
			for (unsigned int y = 0; y < gyu_header->height; ++y) {
				BYTE *sp = src_line;
				BYTE *dp = dst_line;
				BYTE *ap = a;
				if (!(gyu_header->mode & 2)) {
					for (unsigned int x = 0; x < gyu_header->width; ++x) {
						u8 alp = *ap++;
						for (unsigned int p = 0; p < 3; ++p)
							*dp++ = palette[4 * *sp + p] * alp / 16;
						*dp++ = alp == 16 ? 255 : alp * 16;
						++sp;
					}
				} else {
					for (unsigned int x = 0; x < gyu_header->width; ++x) {
						u8 alp = *ap++;
						for (unsigned int p = 0; p < 3; ++p)
							*dp++ = palette[4 * *sp + p] * alp / 255;
						*dp++ = alp;
						++sp;
					}
				}
				a += line_len;
				src_line += line_len;
				dst_line += dib32_line_len;
			}
			free(actbuf);
			free(palette);
			palette = NULL;
			palette_size = 0;
			gyu_header->bits_count = 32;
			gyu_header->palette_colors = 0;
			actbuf = dib32;
			actlen = dib32_len;
		} else {
#if 1
			BYTE *src_line = actbuf;
			BYTE *dst_line = dib32;
			DWORD bpp = gyu_header->bits_count / 8;
			// alpha的值在[0,16]之间
			for (unsigned int y = 0; y < gyu_header->height; ++y) {
				BYTE *sp = src_line;
				BYTE *dp = dst_line;
				BYTE *ap = a;
				if (!(gyu_header->mode & 2)) {
					for (unsigned int x = 0; x < gyu_header->width; ++x) {
						u8 alp = *ap++;
						for (unsigned int p = 0; p < gyu_header->bits_count / 8; ++p)
							*dp++ = *sp++ * alp / 16;
						*dp++ = alp == 16 ? 255 : alp * 16;
					}
				} else {
					for (unsigned int x = 0; x < gyu_header->width; ++x) {
						u8 alp = *ap++;
						for (unsigned int p = 0; p < gyu_header->bits_count / 8; ++p)
							*dp++ = *sp++ * alp / 255;
						*dp++ = alp;
					}
				}
				a += (gyu_header->width + 3) & ~3;
				src_line += line_len;
				dst_line += dib32_line_len;
			}
			free(actbuf);
			free(palette);
			palette = NULL;
			palette_size = 0;
			gyu_header->bits_count = 32;
			gyu_header->palette_colors = 0;
			actbuf = dib32;
			actlen = dib32_len;
#else
			BYTE *line = actbuf;
			for (unsigned int y = 0; y < gyu_header->height; ++y) {
				BYTE *pp = line;
				BYTE *ap = a;
				for (unsigned int x = 0; x < gyu_header->width; ++x) {
					for (unsigned int p = 0; p < gyu_header->bits_count / 8; ++p) {
						*pp = *pp * *ap / 16;
						++pp;
					}
					++ap;
				}
				a += (gyu_header->width + 3) & ~3;
				line += line_len;
			}
#endif
		}
	}

	alpha_blending(actbuf, gyu_header->width, gyu_header->height, gyu_header->bits_count);

	if (MySaveAsBMP(actbuf, actlen, palette, palette_size,
			gyu_header->width, gyu_header->height, gyu_header->bits_count,
			gyu_header->palette_colors, (BYTE **)&pkg_res->actual_data,	
			&pkg_res->actual_data_length, my_malloc)) {
		if (alpha)
			free(alpha);
		free(actbuf);
		if (palette)
			free(palette);
		return -CUI_EMEM;
	}
	free(actbuf);
	if (palette)
		free(palette);

	if (alpha) {
		//if (MyBuildBMPFile(alpha, alpha_length, NULL, 0, 
		//		gyu_header->width, gyu_header->height, 8, 
		//		(BYTE **)&pkg_res->actual_data,	
		//	&pkg_res->actual_data_length, my_malloc)) {
		//	free(alpha);
		//	return -CUI_EMEM;
		//}
		free(alpha);
		//if (pkg->pio->write(pkg, pkg_res, ".MSK.BMP", actbuf, actlen)) {
		//	GlobalFree(actbuf);
		//	return -1;
		//}
		//free(actbuf);
	}
	
	pkg_res->raw_data = NULL;

	return 0;
}

static int ExHIBIT_gyu_save_resource(struct resource *res, 
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

static void ExHIBIT_gyu_release_resource(struct package *pkg, 
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

static void ExHIBIT_gyu_release(struct package *pkg, 
								struct package_directory *pkg_dir)
{
	void *priv;
	
	pkg->pio->close(pkg);
	priv = (void *)package_get_private(pkg);
	if (priv) {
		free(priv);
		package_set_private(pkg, NULL);
	}
}

static cui_ext_operation ExHIBIT_gyu_operation = {
	ExHIBIT_gyu_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	ExHIBIT_gyu_extract_resource,	/* extract_resource */
	ExHIBIT_gyu_save_resource,		/* save_resource */
	ExHIBIT_gyu_release_resource,	/* release_resource */
	ExHIBIT_gyu_release				/* release */
};

/********************* dlt *********************/

static int ExHIBIT_dlt_match(struct package *pkg)
{
	dlt_header_t header;
	
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &header, sizeof(header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (header.version != 0x101) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int ExHIBIT_dlt_extract_directory(struct package *pkg,
										  struct package_directory *pkg_dir)
{
	dlt_header_t header;
	if (pkg->pio->readvec(pkg, &header, sizeof(header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD dlt_index_len = sizeof(my_dlt_entry_t) * header.pages;
	my_dlt_entry_t *dlt_index = (my_dlt_entry_t *)malloc(dlt_index_len);
	if (!dlt_index)
		return -CUI_EMEM;

	int ret = 0;
	DWORD offset = sizeof(header);
	for (DWORD i = 0; i < header.pages; ++i) {
		dlt_page_t page;

		sprintf(dlt_index[i].name, "%04d", i);
		dlt_index[i].offset = offset;

		if (pkg->pio->readvec(pkg, &page, sizeof(page), offset, IO_SEEK_SET)) {
			ret = -CUI_EREAD;
			break;
		}
		offset += sizeof(page);

		dlt_page_pos_t pos;
		if (!(page.flags & 6) && !(page.flags & 0x8000)) {
			if (pkg->pio->read(pkg, &pos, sizeof(pos))) {
				ret = -CUI_EREAD;
				break;
			}
			offset += sizeof(pos);
		}

		dlt_page_mask_t mask;
		if (page.flags & 0x4000) {
			if (pkg->pio->read(pkg, &mask, sizeof(mask))) {
				ret = -CUI_EREAD;
				break;
			}
			offset += sizeof(mask);
			offset += mask.mask_data_length;
		}

		offset += page.size;
		dlt_index[i].length = offset - dlt_index[i].offset;
	}
	if (i != header.pages) {
		free(dlt_index);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = header.pages;
	pkg_dir->directory = dlt_index;
	pkg_dir->directory_length = dlt_index_len;
	pkg_dir->index_entry_length = sizeof(my_dlt_entry_t);

	return 0;
}

static int ExHIBIT_dlt_parse_resource_info(struct package *pkg,
										   struct package_resource *pkg_res)
{
	my_dlt_entry_t *my_dlt_entry;

	my_dlt_entry = (my_dlt_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_dlt_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_dlt_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = my_dlt_entry->offset;

	return 0;
}

static int ExHIBIT_dlt_extract_resource(struct package *pkg,
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

	pkg_res->raw_data = raw;

	return 0;
}

static void ExHIBIT_dlt_release(struct package *pkg, 
								struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	
	pkg->pio->close(pkg);
}

static cui_ext_operation ExHIBIT_dlt_operation = {
	ExHIBIT_dlt_match,				/* match */
	ExHIBIT_dlt_extract_directory,	/* extract_directory */
	ExHIBIT_dlt_parse_resource_info,/* parse_resource_info */
	ExHIBIT_dlt_extract_resource,	/* extract_resource */
	ExHIBIT_gyu_save_resource,		/* save_resource */
	ExHIBIT_gyu_release_resource,	/* release_resource */
	ExHIBIT_dlt_release				/* release */
};

#if 0
/********************* v *********************/

static int ExHIBIT_v_match(struct package *pkg)
{
	v_header_t *v_header;

	if (!pkg)
		return -CUI_EPARA;

	v_header = (v_header_t *)malloc(sizeof(*v_header));
	if (!gyu_header)
		return -CUI_EMEM;

	if (pkg->pio->open(pkg, IO_READONLY)) {
		free(v_header);
		return -CUI_EOPEN;
	}

	if (pkg->pio->read(pkg, gyu_header, sizeof(*gyu_header))) {
		pkg->pio->close(pkg);
		free(v_header);
		return -CUI_EREAD;
	}

	if (gyu_header->zero) {
		pkg->pio->close(pkg);
		free(v_header);
		return -CUI_EMATCH;
	}
	
	switch (gyu_header->mode)
	case 1:
	case 2:
	case 3:
	case 4:
	case 6:
	case 7:
		break;
	default:
		pkg->pio->close(pkg);
		free(v_header);
		return -CUI_EMATCH;
	}
	package_set_private(pkg, v_header);

	return 0;	
}

static int build_pcm44_header(BYTE **ret_wwav, DWORD *ret_wav_length)
{
	
	
}

static int ExHIBIT_v_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	v_header_t *v_header;
	BYTE *palette, *compr, *uncompr, *actbuf, *alpha;
	unsigned int palette_size, line_len;
	DWORD uncomprlen, comprlen, act_uncomprlen, actlen, alpha_length;
	u32 mode;
	DWORD v_size;
	
	if (pkg->pio->length_of(pkg, &v_size))
		return -CUI_ELEN;
	
	if (v_size <= sizeof(*v_header))
		return -CUI_EMATCH;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	v_header = (v_header_t *)package_get_private(pkg);
	switch (v_header->mode)
	case 1:	// ADPCM
	case 2:	// PCM??
	case 3:	// PCM??
	case 4:	// PCM22M
		ret = build_pcm22_header();
		break;
	case 6:	// PCM44M
		ret = build_pcm44_header();
		break;
	case 7:	// ADPCM44
		break;
	}

	if (gyu_header->palette_colors) {
		palette_size = gyu_header->palette_colors * 4;
		palette = (BYTE *)malloc(palette_size);
		if (!palette)
			return -CUI_EMEM;		

		if (pkg->pio->read(pkg, palette, palette_size)) {
			free(palette);
			return -CUI_EREAD;
		}
	} else {
		palette_size = 0;
		palette = NULL;
	}

	comprlen = gyu_header->length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr) {
		if (palette)	
			free(palette);
		return -CUI_EMEM;					
	}

	if (pkg->pio->read(pkg, compr, comprlen)) {
		free(compr);		
		if (palette)
			free(palette);
		return -CUI_EREAD;
	}

	if (gyu_header->mode & 1) {
		alpha_length = gyu_header->alpha_length;
		alpha = (BYTE *)malloc(alpha_length);
		if (!alpha) {
			free(compr);		
			if (palette)	
				free(palette);
			return -CUI_EMEM;
		}

		if (pkg->pio->read(pkg, alpha, alpha_length)) {
			free(alpha);
			free(compr);		
			if (palette)	
				free(palette);
			return -CUI_EREAD;
		}

		uncomprlen = ((gyu_header->width + 3) & ~3) * gyu_header->height;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(alpha);
			free(compr);		
			if (palette)	
				free(palette);
			return -CUI_EMEM;
		}
		memset(uncompr, 0, uncomprlen);
		act_uncomprlen = lzss_decompress(uncompr, uncomprlen, alpha, alpha_length);
		free(alpha);	
		if (act_uncomprlen != uncomprlen) {
			free(uncompr);	
			free(compr);		
			if (palette)	
				free(palette);
			return -CUI_EUNCOMPR;
		}
		alpha = uncompr;
		alpha_length = act_uncomprlen;
	} else {
		alpha_length = 0;
		alpha = NULL;
	}

	mode = gyu_header->mode & 0xffff0000;

	}

	return 0;
}

static int ExHIBIT_v_save_resource(struct resource *res, 
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

static void ExHIBIT_v_release_resource(struct package *pkg, 
										struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
}

static void ExHIBIT_v_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (!pkg)
		return;

	pkg->pio->close(pkg);
}

static cui_ext_operation ExHIBIT_v_operation = {
	ExHIBIT_v_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	ExHIBIT_v_extract_resource,	/* extract_resource */
	ExHIBIT_v_save_resource,		/* save_resource */
	ExHIBIT_v_release_resource,	/* release_resource */
	ExHIBIT_v_release				/* release */
};
#endif

/********************* grp *********************/

static int ExHIBIT_grp_match(struct package *pkg)
{
	grp_header_t header;
	
	if (pkg->pio->open(pkg->lst, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg->lst, &header, sizeof(header))) {
		pkg->pio->close(pkg->lst);
		return -CUI_EREAD;
	}

	if (strncmp(header.magic, "AiFS", 4)) {
		pkg->pio->close(pkg->lst);
		return -CUI_EMATCH;
	}

	if (pkg->pio->open(pkg, IO_READONLY)) {
		pkg->pio->close(pkg->lst);	
		return -CUI_EOPEN;
	}

	char res_name[MAX_PATH];
	unicode2acp(res_name, MAX_PATH, pkg->name, -1);

	DWORD res_id;
	if (sscanf(res_name, "res%d.grp", &res_id) != 1) {
		pkg->pio->close(pkg);
		pkg->pio->close(pkg->lst);
		return -CUI_EMATCH;
	}

	DWORD pkg_id;
	unicode2acp(res_name, MAX_PATH, pkg->lst->name, -1);
	if (sscanf(res_name, "res%d.grp", &pkg_id) != 1) {
		pkg->pio->close(pkg);
		pkg->pio->close(pkg->lst);
		return -CUI_EMATCH;
	}

	for (DWORD i = 0; i < header.res_files; ++i) {
		if (pkg_id + 1 + i == res_id)
			break;
	}
	if (i == header.res_files) {
		pkg->pio->close(pkg);
		pkg->pio->close(pkg->lst);
		return -CUI_EMATCH;
	}

	package_set_private(pkg, res_id - pkg_id);

	return 0;	
}

static int ExHIBIT_grp_extract_directory(struct package *pkg,
										  struct package_directory *pkg_dir)
{
	grp_header_t header;
	
	if (pkg->pio->readvec(pkg->lst, &header, sizeof(header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD res_id = package_get_private(pkg);
	DWORD offset = sizeof(grp_header_t);
	grp_entry_t entry;
	for (DWORD i = 0; i < header.res_files; ++i) {
		if (pkg->pio->readvec(pkg->lst, &entry, sizeof(entry), offset, IO_SEEK_SET))
			return -CUI_EREADVEC;

		if (entry.id == res_id)
			break;

		offset += sizeof(grp_entry_t) + entry.res_entries * sizeof(grp_info_entry_t);
	}

	DWORD index_len = entry.res_entries * sizeof(grp_info_entry_t);
	grp_info_entry_t *index = (grp_info_entry_t *)malloc(index_len);
	if (!index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg->lst, index, index_len)) {
		free(index);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = entry.res_entries;
	pkg_dir->directory = index;
	pkg_dir->directory_length = index_len;
	pkg_dir->index_entry_length = sizeof(grp_info_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;
	package_set_private(pkg, entry.start_res_id);

	return 0;
}

static int ExHIBIT_grp_parse_resource_info(struct package *pkg,
										   struct package_resource *pkg_res)
{
	grp_info_entry_t *entry;

	entry = (grp_info_entry_t *)pkg_res->actual_index_entry;
	DWORD start_res_id = package_get_private(pkg);
	sprintf(pkg_res->name, "%06d.ogg", start_res_id + pkg_res->index_number);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = entry->offset;

	return 0;
}

static int ExHIBIT_grp_extract_resource(struct package *pkg,
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

	pkg_res->raw_data = raw;

	return 0;
}

static void ExHIBIT_grp_release(struct package *pkg, 
								struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
	pkg->pio->close(pkg->lst);
}

static cui_ext_operation ExHIBIT_grp_operation = {
	ExHIBIT_grp_match,				/* match */
	ExHIBIT_grp_extract_directory,	/* extract_directory */
	ExHIBIT_grp_parse_resource_info,/* parse_resource_info */
	ExHIBIT_grp_extract_resource,	/* extract_resource */
	ExHIBIT_gyu_save_resource,		/* save_resource */
	ExHIBIT_gyu_release_resource,	/* release_resource */
	ExHIBIT_grp_release				/* release */
};

/********************* rld *********************/

static int ExHIBIT_rld_match(struct package *pkg)
{
	rld_header_t rld_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &rld_header, sizeof(rld_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(rld_header.magic, "\x00\x44LR", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int ExHIBIT_rld_extract_resource(struct package *pkg,
										struct package_resource *pkg_res)
{
	
	BYTE *raw = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length, 0, IO_SEEK_SET)) {
		free(raw);
		return -CUI_EREADVEC;
	}

	char *string = "PrimaryPrimary丂乣Magical仛Trouble仛Scramble乣丂TRIAL80060020482566144100225253380048000400100";
	BYTE code[4] = { 0, 0, 0, 0 };
	BYTE chr;
	while ((chr = *string++)) {
		code[0] += chr;
		chr = *string++;
		if (!chr)
			break;
		code[1] += chr;
		chr = *string++;
		if (!chr)
			break;
		code[2] += chr;
		chr = *string++;
		if (!chr)
			break;
		code[3] += chr;
	}

	DWORD dec_len = (pkg_res->raw_data_length - sizeof(rld_header_t)) / 4;
	if (dec_len > 0x3FF0)
		dec_len = 0x3FF0;

	dec_len -= sizeof(rld_header_t) / 4;
	u8 *tbl = (u8 *)(raw + sizeof(rld_header_t));
	u8 *dec = tbl + 256;
	dec_len -= 256 / 4;
	for (DWORD i = 0; i < dec_len * 4; ++i)
		*dec++ ^= tbl[i & 0xff] ^ 0x6e;

	pkg_res->raw_data = raw;

	return 0;
}

static cui_ext_operation ExHIBIT_rld_operation = {
	ExHIBIT_rld_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	ExHIBIT_rld_extract_resource,	/* extract_resource */
	ExHIBIT_gyu_save_resource,		/* save_resource */
	ExHIBIT_gyu_release_resource,	/* release_resource */
	ExHIBIT_gyu_release				/* release */
};

int CALLBACK ExHIBIT_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".gyu"), _T(".bmp"), 
		NULL, &ExHIBIT_gyu_operation, CUI_EXT_FLAG_PKG))
			return -1;

#if 0
	if (callback->add_extension(callback->cui, _T(".dlt"), _T(".bmp"), 
		NULL, &ExHIBIT_dlt_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;
#endif

	if (callback->add_extension(callback->cui, _T(".grp"), _T(".ogg"), 
		NULL, &ExHIBIT_grp_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".rld"), _T(".txt"), 
		NULL, &ExHIBIT_rld_operation, CUI_EXT_FLAG_PKG))
			return -1;

	return 0;
}
