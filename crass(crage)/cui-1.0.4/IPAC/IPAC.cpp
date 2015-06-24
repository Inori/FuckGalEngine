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

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information IPAC_cui_information = {
	NULL,					/* copyright */
	NULL,					/* system */
	_T(".PAK .WST"),		/* package */
	_T("1.0.3"),			/* revision */
	_T("痴汉公贼"),			/* author */
	_T("2009-3-29 23:09"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */

#pragma pack (1)
typedef struct {
	s8 magic[4];		/* "IPAC" */
	u16 index_entries;
	u16 unknown;
} PAK_header_t;

typedef struct {
	s8 name[32];
	u32 unknown;
	u32 offset;
	u32 length;
} PAK_entry_t;

typedef struct {
	s8 magic[4];		/* "IEL1" */
	u32 length;
} iel1_header_t;

/*
20 bytes header
1024 bytes palette
width * height * 3 dib data
width * height alpha
*/
typedef struct {
	s8 magic[4];		/* "IES2" */
	u32 unknown;		/* "IES2" */
	u32 width;
	u32 height;
	u32 bits_count;
	u32 reserved[3];
} ies2_header_t;

struct key {
	s32 low;
	s32 high;
};

typedef struct {
	s8 magic[4];		/* "WST2" */
	u32 reserved[2];
	u16 adpcm_fmt_parameter[14];
} wst_header_t;
#pragma pack ()

static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static inline unsigned char getbit_le(unsigned char byte, unsigned int pos)
{
	return !!(byte & (1 << pos));
}

static DWORD lzss_decompress(unsigned char *uncompr, DWORD uncomprlen, 
							unsigned char *compr, DWORD comprlen)
{
	unsigned int act_uncomprlen = 0;
	/* compr中的当前字节中的下一个扫描位的位置 */
	unsigned int curbit = 0;
	/* compr中的当前扫描字节 */
	unsigned int curbyte = 0;
	unsigned int nCurWindowByte = 0xfee;
	unsigned int win_size = 4096;
	BYTE win[4096];
	
	memset(win, ' ', nCurWindowByte);
	while (1) {
		/* 如果为0, 表示接下来的1个字节原样输出 */
		BYTE flag;

		if (curbyte >= comprlen)
			break;

		flag = compr[curbyte++];
		for (curbit = 0; curbit < 8; curbit++) {
			if (getbit_le(flag, curbit)) {
				unsigned char data;
	
				if (curbyte >= comprlen)
					goto out;

				if (act_uncomprlen >= uncomprlen)
					goto out;

				data = compr[curbyte++];
				uncompr[act_uncomprlen++] = data;
				/* 输出的1字节放入滑动窗口 */
				win[nCurWindowByte++] = data;
				nCurWindowByte &= win_size - 1;
			} else {
				unsigned int copy_bytes, win_offset;
				unsigned int i;

				if (curbyte >= comprlen)
					goto out;

				win_offset = compr[curbyte++];

				if (curbyte >= comprlen)
					goto out;
				copy_bytes = compr[curbyte++];
				win_offset |= (copy_bytes >> 4) << 8;
				copy_bytes &= 0x0f;
				copy_bytes += 3;

				for (i = 0; i < copy_bytes; i++) {	
					unsigned char data;

					if (act_uncomprlen >= uncomprlen)
						goto out;

					data = win[(win_offset + i) & (win_size - 1)];
					uncompr[act_uncomprlen++] = data;		
					/* 输出的1字节放入滑动窗口 */
					win[nCurWindowByte++] = data;
					nCurWindowByte &= win_size - 1;	
				}
			}
		}
	}
out:
	return act_uncomprlen;
}

static int ipac_decompress_iel1(iel1_header_t *iel1, DWORD iel1_len, 
						BYTE **ret_buf, DWORD *ret_len)
{
	BYTE *uncompr, *compr;
	DWORD uncomprLen, comprLen, actlen;

	uncomprLen = iel1->length;
	uncompr = (BYTE *)malloc(uncomprLen);
	if (!uncompr)
		return -1;

	compr = (BYTE *)(iel1 + 1);
	comprLen = iel1_len - sizeof(*iel1);
	actlen = lzss_decompress(uncompr, uncomprLen, compr, comprLen);
	if (actlen != uncomprLen) {
		free(uncompr);
		return -1;
	}
	*ret_buf = uncompr;
	*ret_len = actlen;

	return 0;
}

/********************* WST *********************/

/* 封包匹配回调函数 */
static int IPAC_WST_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "WST2", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int IPAC_WST_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	BYTE *wst_buf;
	u32 wst_buf_len;

	if (pkg->pio->seek(pkg, 0, IO_SEEK_SET))
		return -CUI_ESEEK;

	if (pkg->pio->length_of(pkg, &wst_buf_len))
		return -CUI_ELEN;

	wst_buf = (BYTE *)malloc(wst_buf_len);
	if (!wst_buf)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, wst_buf, wst_buf_len)) {
		free(wst_buf);
		return -CUI_EREAD;
	}

	pkg_res->raw_data = wst_buf;
	pkg_res->raw_data_length = wst_buf_len;

	return 0;
}

/* 资源保存函数 */
static int IPAC_WST_save_resource(struct resource *res, 
								  struct package_resource *pkg_res)
{
	WAVEFORMATEX wav_header;
	DWORD riff_chunk_len, fmt_chunk_len, data_chunk_len;
	char *riff = "RIFF";
	char *id = "WAVE"; 
	char *fmt_chunk_id = "fmt ";
	char *data_chunk_id = "data";
	char *hack_info = "Hacked By 痴汉公贼";
	WORD adpcm_para[16];
	BYTE *data_chunk; 
	wst_header_t *wst2;
	BYTE *wst_buf = (BYTE *)pkg_res->raw_data;
	DWORD wst_buf_len = pkg_res->raw_data_length;
	
	if (res->rio->create(res))
		return -CUI_ECREATE;

	wst2 = (wst_header_t *)wst_buf;
	/* 这个应该是还原为ADCM以后的结果 */
	/* 00441E00  01 00 02 00 44 AC 00 00 10 B1 02 00 04 00 10 00  ....D?..?..... */
	wav_header.wFormatTag = 2;	/* ADPCM */
	wav_header.nChannels = 2;
	wav_header.nSamplesPerSec = 0xac44;	
	wav_header.nAvgBytesPerSec = 0xac44;	/* ADPCM是4:1压缩，因此ac44的4倍就是0x2b110了 */
	wav_header.nBlockAlign = 0x800;
	wav_header.wBitsPerSample = 4;
	wav_header.cbSize = 32;

	adpcm_para[0] = 0x07f4;
	adpcm_para[1] = 0x0007;

	memcpy(&adpcm_para[2], wst2->adpcm_fmt_parameter, 
		sizeof(wst2->adpcm_fmt_parameter));
	data_chunk = (BYTE *)(wst2 + 1);

	data_chunk_len = wst_buf_len - sizeof(*wst2);
	data_chunk_len &= ~(wav_header.nBlockAlign - 1);
	fmt_chunk_len = 16 + 2 + wav_header.cbSize;
	riff_chunk_len = 4 + (8 + fmt_chunk_len) + (8 + data_chunk_len);

	if (res->rio->write(res, (char *)riff, 4)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}

	if (res->rio->write(res, &riff_chunk_len, 4)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}
			
	if (res->rio->write(res, id, 4)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}		
			
	if (res->rio->write(res, fmt_chunk_id, 4)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}

	if (res->rio->write(res, &fmt_chunk_len, 4)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}
			
	if (res->rio->write(res, &wav_header, sizeof(wav_header))) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}			

	if (res->rio->write(res, adpcm_para, sizeof(adpcm_para))) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}	

	if (res->rio->write(res, data_chunk_id, 4)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}
				
	if (res->rio->write(res, &data_chunk_len, 4)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}

	if (res->rio->write(res, data_chunk, data_chunk_len)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}
			
	if (res->rio->write(res, hack_info, strlen(hack_info))) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}

	res->rio->close(res);
	
	return 0;
}

/* 封包资源释放函数 */
static void IPAC_WST_release_resource(struct package *pkg, 
									  struct package_resource *pkg_res)
{
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

/* 封包卸载函数 */
static void IPAC_WST_release(struct package *pkg, 
							 struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation IPAC_WST_operation = {
	IPAC_WST_match,				/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	IPAC_WST_extract_resource,	/* extract_resource */
	IPAC_WST_save_resource,		/* save_resource */
	IPAC_WST_release_resource,	/* release_resource */
	IPAC_WST_release			/* release */
};

/********************* PAK *********************/

/* 封包匹配回调函数 */
static int IPAC_PAK_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "IPAC", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int IPAC_PAK_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	PAK_header_t PAK_header;
	PAK_entry_t *index_buffer;
	unsigned int index_buffer_length;	

	if (pkg->pio->seek(pkg, 0, IO_SEEK_SET))
		return -CUI_ESEEK;

	if (pkg->pio->read(pkg, &PAK_header, sizeof(PAK_header)))
		return -CUI_EREAD;

	pkg_dir->index_entries = PAK_header.index_entries;
	index_buffer_length = pkg_dir->index_entries * sizeof(PAK_entry_t);
	index_buffer = (PAK_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(PAK_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int IPAC_PAK_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	PAK_entry_t *PAK_entry;

	PAK_entry = (PAK_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, PAK_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = PAK_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = PAK_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int IPAC_PAK_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr, *actbuf;
	DWORD uncomprlen, comprlen, actlen;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen,	pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = compr;
		return 0;
	}

	if (!memcmp(compr, "IEL1", 4)) {
		if (ipac_decompress_iel1((iel1_header_t *)compr, comprlen, &uncompr, &uncomprlen)) {
			free(compr);
			return -CUI_EUNCOMPR;
		}
		free(compr);
		compr = NULL;
		actbuf = uncompr;
		actlen = uncomprlen;
	} else {
		uncompr = NULL;
		uncomprlen = 0;
		actbuf = compr;
		actlen = comprlen;
	}

	if (!memcmp(actbuf, "IES2", 4)) {
		ies2_header_t *ies2 = (ies2_header_t *)actbuf;
		BYTE *save_buf;
		DWORD save_len;

		if (ies2->bits_count == 24) {
			BYTE *rgba;

			actlen = ies2->width * ies2->height * 4;
			rgba = (BYTE *)malloc(actlen);
			if (!rgba) {
				free(actbuf);
				free(compr);
				return -CUI_EMEM;
			}

			BYTE *rgb = actbuf + 0x420;
			BYTE *p_alpha = rgb + ies2->width * ies2->height * 3;
			BYTE *p_rgba = rgba;
			for (unsigned int y = 0; y < ies2->height; y++) {
				for (unsigned int x = 0; x < ies2->width; x++) {
					BYTE alpha = *p_alpha++;					
					p_rgba[0] = (rgb[0] * alpha + 0xff * ~alpha) / 255;
					p_rgba[1] = (rgb[1] * alpha + 0xff * ~alpha) / 255;
					p_rgba[2] = (rgb[2] * alpha + 0xff * ~alpha) / 255;	
					p_rgba[3] = alpha;	
					rgb += 3;
					p_rgba += 4;
				}
			}

			if (MyBuildBMPFile(rgba, actlen, NULL, 0, ies2->width,
					0 - ies2->height, 32, &save_buf, &save_len, my_malloc)) {
				free(rgba);
				free(actbuf);
				free(compr);
				return -CUI_EMEM;				
			}
			free(rgba);
		} else {
			if (MyBuildBMPFile(actbuf + 0x420, actlen - 0x420, (BYTE *)(ies2 + 1), 0x400, ies2->width,
					0 - ies2->height, ies2->bits_count, &save_buf, &save_len, my_malloc)) {
				free(actbuf);
				free(compr);
				return -CUI_EMEM;				
			}
		}
		free(actbuf);
		uncompr = save_buf;
		uncomprlen = save_len;
		pkg_res->replace_extension = _T(".IES2.bmp");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
//	} else if (!lstrcmpi(pkg->extension, _T(".IES"))) {
	} else if (strstr(pkg_res->name, ".IES")) {
		BYTE *save_buf;
		DWORD save_len;

#if 0
//		if (MyBuildBMPFile(actbuf + 0x420, actlen - 0x420, actbuf + 0x20, 0x400, *(DWORD *)actbuf,
		if (MyBuildBMPFile(actbuf + 0x414, actlen - 0x414, NULL, 0, *(DWORD *)actbuf,
				0 - *(DWORD *)(actbuf + 4), *(DWORD *)(actbuf + 8), &save_buf, &save_len, my_malloc)) {
			free(actbuf);
			return -CUI_EMEM;			
		}
#else
		if (!memcmp(actbuf, "BM", 2)) {
			pkg_res->raw_data = compr;
			pkg_res->raw_data_length = comprlen;
			pkg_res->actual_data = actbuf;
			pkg_res->actual_data_length = actlen;
			return 0;
		} else if (*(DWORD *)(actbuf + 8) == 24) {
			DWORD width = *(DWORD *)actbuf;
			DWORD height = *(DWORD *)(actbuf + 4);
			BYTE *rgba;

			actlen = width * height * 4;
			rgba = (BYTE *)malloc(actlen);
			if (!rgba) {
				free(actbuf);
				free(compr);
				return -CUI_EMEM;
			}

			BYTE *rgb = actbuf + 0x414;
			BYTE *p_alpha = rgb + width * height * 3;
			BYTE *p_rgba = rgba;
			for (unsigned int y = 0; y < height; y++) {
				for (unsigned int x = 0; x < width; x++) {
					BYTE alpha = *p_alpha++;					
					p_rgba[0] = (rgb[0] * alpha + 0xff * ~alpha) / 255;
					p_rgba[1] = (rgb[1] * alpha + 0xff * ~alpha) / 255;
					p_rgba[2] = (rgb[2] * alpha + 0xff * ~alpha) / 255;	
					p_rgba[3] = alpha;	
					rgb += 3;
					p_rgba += 4;
				}
			}

			if (MyBuildBMPFile(rgba, actlen, NULL, 0, width,
					0 - height, 32, &save_buf, &save_len, my_malloc)) {
				free(rgba);
				free(actbuf);
				free(compr);
				return -CUI_EMEM;				
			}
			free(rgba);
		} else {
			if (MyBuildBMPFile(actbuf + 0x414, actlen - 0x414, NULL, 0, *(DWORD *)actbuf,
					0 - *(DWORD *)(actbuf + 4), *(DWORD *)(actbuf + 8), &save_buf, &save_len, my_malloc)) {
				free(actbuf);
				free(compr);
				return -CUI_EMEM;			
			}
		}
#endif
		free(actbuf);
		uncompr = save_buf;
		uncomprlen = save_len;
		pkg_res->replace_extension = _T(".IES.bmp");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
	}

	pkg_res->raw_data = compr;
	pkg_res->raw_data_length = comprlen;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

/* 资源保存函数 */
static int IPAC_PAK_save_resource(struct resource *res, 
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

/* 封包资源释放函数 */
static void IPAC_PAK_release_resource(struct package *pkg, 
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

/* 封包卸载函数 */
static void IPAC_PAK_release(struct package *pkg, 
							 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation IPAC_PAK_operation = {
	IPAC_PAK_match,					/* match */
	IPAC_PAK_extract_directory,		/* extract_directory */
	IPAC_PAK_parse_resource_info,	/* parse_resource_info */
	IPAC_PAK_extract_resource,		/* extract_resource */
	IPAC_PAK_save_resource,			/* save_resource */
	IPAC_PAK_release_resource,		/* release_resource */
	IPAC_PAK_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK IPAC_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".PAK"), NULL, 
		NULL, &IPAC_PAK_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".WST"), _T(".wav"), 
		NULL, &IPAC_WST_operation, CUI_EXT_FLAG_PKG))
			return -1;	

	return 0;
}
