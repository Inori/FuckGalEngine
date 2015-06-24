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

//I:\ふるふる☆フルムーン\(18禁ゲーム)[080530][リトルプリンセス]ふるふる☆フルムーン+予約特典 (iso+mds+ccd)\game

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information ags_cui_information = {
	_T(""),		/* copyright */
	_T(""),			/* system */
	_T(".pk .px"),			/* package */
	_T("0.9.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-6-15 16:08"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];
	u32 size;
} ags_head_t;

typedef struct {
	s8 magic[4];		// "cLST"
	u32 size;
	u32 head_size;		// 0x18
	u32 flag;			// bit0 is valid
	u32 entries;
	u32 key;
} cLST_head_t;

typedef struct {
	u32 name_offset;
	u32 offset;
	u32 length;
} cLST_entry_t;

typedef struct {
	s8 magic[4];		// "cNAM"
	u32 size;
	u32 head_size;		// 0x14
	u32 unknown0;		// 0
	u32 unknown1;		// 0
} cNAM_head_t;

typedef struct {
	s8 magic[4];		// "cDAT"
	u32 size;
	u32 head_size;		// 0x0c
} cDAT_head_t;

typedef struct {
	s8 magic[4];		// "cTRK"
	u32 size;			// 当前cTRK块的长度
	u32 trk_number;		// cTRK块的编号，从1开始
	u32 head_size;		// 0x24，cTRK头的长度
	u32 Samples;		// 当前cTRK块的采样数
	u32 SamplesPerSec;
	u16 reserved0;		// 0x0000
	u16 Channels;
	u16 BitsPerSample;
	u16 type;			// 0x0000(.wav) / 0x0003(.ogg)
	u16 unknown0;		// 0x0010
	u16 unknown1;		// 0x0001
} cTRK_head_t;

typedef struct {
	s8 magic[4];		// "cJPG"
	u32 size;			// 当前cJPG块的长度
	u32 head_size;		// 0x24, cJPG头的长度
	u32 data_length;	// 当前块的数据长度
	u16 x_limit;		// [0x10] 该图所处的坐标空间的最大宽度
	u16 y_limit;		// [0x12] 该图所处的坐标空间的最大高度
	u16 x;				// [0x14] 该图原点x的位置
	u16 y;				// [0x16] 该图原点y的位置
	u16 width;			// [0x18]
	u16 height;			// [0x1a]
	u16 unknown0;		// [0x1c] 0x48
	u16 unknown1;		// [0x1e] 0x00
	u32 seed;			// [0x20] 解密用
} cJPG_head_t;

typedef struct {
	s8 magic[4];		// "cRGB"
	u32 size;			// 当前cRGB块的长度
	u32 head_size;		// 0x2c，cRGB头的长度
	u32 data_length;	// 当前块的数据长度
	u16 mode;			// [0x10] 0, 1(bg), 2(with palette), 3(立绘), 4, etc
	u16 unknown2;		// [0x12] type2: 0x8808
	u16 x_limit;		// [0x14] 该图所处的坐标空间的最大宽度
	u16 y_limit;		// [0x16] 该图所处的坐标空间的最大高度
	u16 x;				// [0x18] 该图原点x的位置
	u16 y;				// [0x1a] 该图原点y的位置
	u16 width;			// [0x1c]
	u16 height;			// [0x1e]
	u16 unknown4;		// [0x20] 0x48
	u16 bpp;			// [0x22] < 24就是cRGB块 0x18(with palette) or 0x20
	u8 unknown[8];		// [0x24] 0x08 0x18 0x08 0x10 0x08 0x08 0x08 0x00
} cRGB_head_t;
#pragma pack ()

/* .pk封包的索引项结构 */
typedef struct {
	char name[256];
	DWORD name_length;
	DWORD offset;
	DWORD length;
} pk_entry_t;

typedef struct {
	char name[256];
	DWORD offset;
	DWORD length;
} px_entry_t;

static void *my_malloc(DWORD len)
{
	return new BYTE[len];
}

static void cLST_decode(cLST_head_t *cLST, 
						DWORD *cLST_data, 
						DWORD cLST_data_size)
{
	DWORD dec_key[32];
	WORD dec_code[32];

	DWORD key = cLST->key;
	for (DWORD j = 0; j < 32; ++j) {
		DWORD code = 0;
		dec_key[j] = key;
		for (DWORD i = 0; i < 16; ++i) {
			code = ((key ^ (key >> 1)) << 15) | (code >> 1) & 0xffff;
			key >>= 2;
		}		
		key = (key << 1) | (key >> 31);
		dec_code[j] = (WORD)code;
	}

	for (j = 0; j < cLST_data_size / 4; ++j) {
		DWORD EDI = 0, EDX = 1, ECX = 2, EBX = 3;
		DWORD flag = dec_code[j];
		for (DWORD i = 0; i < 16; ++i) {
			DWORD EAX;
			if (flag & 1) {
				EAX = ECX >> 1;
				EAX &= cLST_data[j] >> 1;
				EAX |= (cLST_data[j] & EDX) << 1;
			} else {
				EAX = cLST_data[j] & EBX;
			}
			flag >>= 1;
			EBX <<= 2;
			ECX <<= 2;
			EDX <<= 2;
			EDI |= EAX;
		}
		cLST_data[j] = dec_key[j & 0x1f] ^ EDI;
	}
}


static int cRGB_decode(cRGB_head_t *cRGB, BYTE **ret_uncompr, DWORD *ret_uncomprlen,
					   BYTE **ret_palette, DWORD *ret_palette_len)
{
	BYTE *cRGB_data = (BYTE *)cRGB + cRGB->head_size;
	BYTE *dib_buffer = NULL;
	u32 *line_index;
	DWORD dib_buffer_length = 0;

	switch (cRGB->mode) {
	case 0:
		dib_buffer_length = cRGB->width * cRGB->height * cRGB->bpp / 8;
		dib_buffer = new BYTE[dib_buffer_length];
		if (!dib_buffer)
			return -CUI_EMEM;

		if (cRGB->bpp > 24) {
			BYTE *src = cRGB_data;
			BYTE *RGB = dib_buffer;
			// alpha blending
			for (DWORD y = 0; y < cRGB->height; ++y) {
				BYTE alpha = src[3];
				for (DWORD x = 0; x < cRGB->width; ++x) {
					*RGB++ = *src++ * alpha / 255;
					*RGB++ = *src++ * alpha / 255;
					*RGB++ = *src++ * alpha / 255;
					*RGB++ = 255 - alpha;
					*src++;
				}
			}
		} else {
			BYTE *src = cRGB_data;
			BYTE *RGB = dib_buffer;
			for (DWORD y = 0; y < cRGB->height; ++y) {
				for (DWORD x = 0; x < cRGB->width; ++x) {
					*RGB++ = *src++;
					*RGB++ = *src++;
					*RGB++ = *src++;
				}
			}
		}
		break;
	case 1:
		dib_buffer_length = cRGB->width * cRGB->height * cRGB->bpp / 8;
		dib_buffer = new BYTE[dib_buffer_length];
		if (!dib_buffer)
			return -CUI_EMEM;

		if (cRGB->bpp > 24) {
			BYTE *line_buffer = new BYTE[cRGB->width * 4];
			if (!line_buffer) {
				delete [] dib_buffer;
				return -CUI_EMEM;
			}
			BYTE *src = cRGB_data;
			BYTE *RGB = dib_buffer;
			for (DWORD y = 0; y < cRGB->height; ++y) {
				// RLE decode
				BYTE *dst = line_buffer;
				for (DWORD p = 0; p < 4; ++p) {
					for (int x_count = cRGB->width; x_count > 0; ) {
						BYTE code = *src++;
						DWORD x_copy = code & 0x3f;
						if (code & 0x40)
							x_copy = (x_copy << 8) | *src++;
						x_count -= x_copy;
						if (code & 0x80) {
							BYTE pixel_val = *src++;
							for (DWORD j = 0; j < x_copy; ++j)
								*dst++ = pixel_val;
						} else {
							for (DWORD j = 0; j < x_copy; ++j)
								*dst++ = *src++;
						}
					}
				}
				// alpha blending
				BYTE *alphe_line = line_buffer + cRGB->width * 3;
				BYTE *pixel_line = line_buffer - 1;
				for (DWORD x = 0; x < cRGB->width; ++x) {
					BYTE *pixel = pixel_line + 1;
					BYTE alpha = *alphe_line++;
					for (DWORD k = 0; k < 3; ++k) {
						*RGB++ = alpha * *pixel / 255;
						pixel += cRGB->width;
					}
					*RGB++ = 255 - alpha;
				}
			}
			delete [] line_buffer;
		} else {		// TO test!
			printf("cRGB1 bpp24!\n");exit(0);

			BYTE *line_buffer = new BYTE[cRGB->width * 4];
			if (!line_buffer) {
				delete [] dib_buffer;
				return -CUI_EMEM;
			}
			BYTE *src = cRGB_data;
			BYTE *RGB = dib_buffer;
			// RLE decode
			for (DWORD y = 0; y < cRGB->height; ++y) {
				BYTE *dst = line_buffer;
				for (DWORD p = 0; p < 3; ++p) {
					for (int x_count = cRGB->width; x_count > 0; ) {
						BYTE code = *src++;
						DWORD x_copy = code & 0x3f;
						if (code & 0x40)
							x_copy = (x_copy << 8) | *src++;
						x_count -= x_copy;
						if (code & 0x80) {
							BYTE pixel_val = *src++;
							for (DWORD j = 0; j < x_copy; ++j)
								*dst++ = pixel_val;
						} else {
							for (DWORD j = 0; j < x_copy; ++j)
								*dst++ = *src++;
						}
					}
				}

				BYTE *pixel_line = line_buffer - 1;
				for (DWORD x = 0; x < cRGB->width; ++x) {
					BYTE *pixel = pixel_line + 1;
					for (DWORD k = 0; k < 3; ++k) {
						*RGB++ = *pixel;
						pixel += cRGB->width;
					}
				}
			}
		}		
		break;
	case 2:	// TO test!
		dib_buffer_length = cRGB->width * cRGB->height;
		dib_buffer = new BYTE[dib_buffer_length];
		if (!dib_buffer)
			return -CUI_EMEM;

		{
			BYTE *palette = cRGB_data;
			line_index = (u32 *)(palette + 256 * 4);
			BYTE *line_buffer = (BYTE *)(line_index + cRGB->height);
			BYTE *org_dst = dib_buffer;
			for (DWORD y = 0; y < cRGB->height; ++y) {
				BYTE *src = line_buffer + line_index[y];
				BYTE *dst = dib_buffer + y * cRGB->width;
				BYTE *org_src = src;				

				for (DWORD x = 0; x < cRGB->width; ) {
					BYTE code = *src;
					DWORD x_copy;

					if (code & 2) {
						x_copy = (*(u16 *)src >> 2) & 0x3ff;
						++src;
					} else
						x_copy = (code >> 2) & 3;

					BYTE tmp;
					if (code & 1) {
						tmp = *src++ >> 4;
						if (tmp)
							tmp = (tmp << 4) | 0xF;
						++src;

						for (DWORD j = 0; j < x_copy; ++j) {
							if (x >= cRGB->width)
								break;

							*dst++ = tmp;
							++x;
						}
					} else {
						BYTE xor = 0;

						for (DWORD j = 0; j < x_copy; ++j) {
							if (x >= cRGB->width)
								break;

							xor ^= 1;

							tmp = *src;
							if (xor) {
								tmp >>= 4;
								src += 2;
							} else {
								tmp &= 0xF;
								++src;
							}
							if (tmp)
								tmp = (tmp << 4) | 0xF;
							*dst++ = tmp;
							++x;
						}
						if (!xor)
							++src;
					}
				}
	
				if (y==1) {
				//	printf("%x %x %x %x\n", src-org_src,dst-org_dst,line_index[1],line_index[2]);exit(0);
				}
				*ret_palette = palette;
				*ret_palette_len = 1024;
			}
		}
		break;
	case 3:
		dib_buffer_length = cRGB->width * cRGB->height * cRGB->bpp / 8;		
		dib_buffer = new BYTE[dib_buffer_length];
		if (!dib_buffer)
			return -CUI_EMEM;

		line_index = (u32 *)cRGB_data;
		{
			BYTE *line_buffer = (BYTE *)(line_index + cRGB->height);
			BYTE *dst = dib_buffer;
			if (cRGB->bpp > 24) {
				for (DWORD y = 0; y < cRGB->height; ++y) {
					BYTE *src = line_buffer + line_index[y];
					for (DWORD x = 0; x < cRGB->width; ) {
						BYTE code = *src++;
						DWORD x_copy = code & 0x3f;
						if (code & 0x40)
							x_copy = (x_copy << 8) | *src++;
						x += x_copy;
						if (code & 0x80) {
							for (DWORD j = 0; j < x_copy; ++j) {
								*dst++ = src[3] * src[0] / 255;
								*dst++ = src[3] * src[1] / 255;
								*dst++ = src[3] * src[2] / 255;
								*dst++ = 255 - src[3];							
							}
							src += 4;
						} else {
							for (DWORD j = 0; j < x_copy; ++j) {
								*dst++ = src[3] * src[0] / 255;
								*dst++ = src[3] * src[1] / 255;
								*dst++ = src[3] * src[2] / 255;
								*dst++ = 255 - src[3];
								src += 4;
							}
						}
					}
				}
			} else {
				for (DWORD y = 0; y < cRGB->height; ++y) {
					BYTE *src = line_buffer + line_index[y];
					for (DWORD x = 0; x < cRGB->width; ) {
						BYTE code = *src++;
						DWORD x_copy = code & 0x3f;
						if (code & 0x40)
							x_copy = (x_copy << 8) | *src++;
						x += x_copy;
						if (code & 0x80) {
							for (DWORD j = 0; j < x_copy; ++j) {
								*dst++ = src[0];
								*dst++ = src[1];
								*dst++ = src[2];						
							}
							src += 3;
						} else {
							for (DWORD j = 0; j < x_copy; ++j) {
								*dst++ = *src++;
								*dst++ = *src++;
								*dst++ = *src++;
							}
						}
					}
				}
			}
		}
		break;
	case 4:		// TO test!
		printf("cRGB4!\n");exit(0);
		dib_buffer_length = cRGB->data_length;
		dib_buffer = new BYTE[dib_buffer_length + 3];
		if (!dib_buffer)
			return -CUI_EMEM;
		memcpy(dib_buffer, cRGB_data, cRGB->data_length);
		//42f940(*(u32 *)(a2 + 16), 4 * cRGB->width);
		break;
	default:
		dib_buffer_length = 0;		
		dib_buffer = NULL;
	}
	*ret_uncompr = dib_buffer;
	*ret_uncomprlen = dib_buffer_length;

	return 0;
}

static int SG_decode(BYTE *raw, BYTE **ret_buffer, DWORD *ret_buffer_length)
{
	ags_head_t *head = (ags_head_t *)raw;
	BYTE *act_data;
	DWORD act_data_len;

	if (!memcmp(head->magic, "cJPG", 4)) {
		cJPG_head_t *cJPG = (cJPG_head_t *)raw;
		BYTE *cJPG_data = raw + cJPG->head_size;
		DWORD key[2][32];
		DWORD seed = cJPG->seed;
		for (DWORD i = 0; i < 32; ++i) {
			DWORD _key = 0;
			DWORD _seed = seed;
			for (DWORD j = 0; j < 16; ++j) {
				_key = (_key >> 1) | (u16)((_seed ^ (_seed >> 1)) << 15);
				_seed >>= 2;
			}
			key[0][i] = seed;
			key[1][i] = _key;
			seed = (seed << 1) | (seed >> 31);
		}

		DWORD *dec = (DWORD *)new BYTE[cJPG->data_length];
		if (!dec)
			return -CUI_EMEM;

		DWORD count = cJPG->data_length / 4;
		DWORD *enc = (DWORD *)cJPG_data;
		for (i = 0; i < count; ++i) {
			DWORD _key = key[1][i & 0x1f];
			DWORD flag3 = 3;
			DWORD flag2 = 2;
			DWORD flag1 = 1;
			DWORD result = 0;
			for (DWORD j = 0; j < 16; ++j) {
				DWORD tmp;
				if (_key & 1)
					tmp = 2 * (enc[i] & flag1) | (enc[i] >> 1) & (flag2 >> 1);
				else
					tmp = enc[i] & flag3;
				_key >>= 1;
				result |= tmp;
				flag3 <<= 2;
				flag2 <<= 2;
				flag1 <<= 2;
			}			
			dec[i] = result ^ key[0][i & 0x1f];
		}
		memcpy(dec + count, enc + count, cJPG->data_length & 3);		
		act_data = (BYTE *)dec;
		act_data_len = cJPG->data_length;
	} else if (!memcmp(head->magic, "cRGB", 4)) {
		cRGB_head_t *cRGB = (cRGB_head_t *)raw;		
		BYTE *uncompr, *palette = NULL;
		DWORD uncomprlen, palette_len = 0;

		int ret = cRGB_decode(cRGB, &uncompr, &uncomprlen, &palette, &palette_len);
		if (ret)
			return ret;

		if (uncompr && uncomprlen) {
			if (MyBuildBMPFile(uncompr, uncomprlen, palette, palette_len,
				cRGB->width, cRGB->height, palette ? 8 : cRGB->bpp,
					(BYTE **)&act_data,	&act_data_len,
					my_malloc)) {
				delete [] palette;
				delete [] uncompr;
				return -CUI_EMEM;
			}
			delete [] palette;
			delete [] uncompr;
		} else {
			act_data_len = head->size;
			act_data = new BYTE[head->size];
			if (!act_data)
				return -CUI_EMEM;
		}			
	} else {
		act_data = new BYTE[head->size];
		if (!act_data)
			return -CUI_EMEM;
		memcpy(act_data, raw, head->size);
	}
	*ret_buffer = act_data;
	*ret_buffer_length = act_data_len;

	return 0;
}

/********************* pk *********************/

/* 封包匹配回调函数 */
static int ags_pk_match(struct package *pkg)
{
	ags_head_t pk_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &pk_header, sizeof(pk_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(pk_header.magic, "fPK ", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	u32 pk_size;
	if (pkg->pio->length_of(pkg, &pk_size)) {
		pkg->pio->close(pkg);
		return -CUI_ELEN;
	}

	if (pk_size != pk_header.size) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int ags_pk_extract_directory(struct package *pkg, 
									 struct package_directory *pkg_dir)
{
	cLST_head_t cLST;
	if (pkg->pio->read(pkg, &cLST, sizeof(cLST)))
		return -CUI_EREAD;

	if (strncmp(cLST.magic, "cLST", 4))
		return -CUI_EMATCH;

	cLST.flag &= 1;

	DWORD cLST_index_len = cLST.size - cLST.head_size;
	cLST_entry_t *cLST_index = new cLST_entry_t[cLST.entries];
	if (!cLST_index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, cLST_index, cLST_index_len)) {
		delete [] cLST_index;
		return -CUI_EREAD;
	}

//	cLST_decode(&cLST, (DWORD *)cLST_index, cLST_index_len);

	cNAM_head_t cNAM;
	if (pkg->pio->read(pkg, &cNAM, sizeof(cNAM))) {
		delete [] cLST_index;
		return -CUI_EREAD;
	}

	if (strncmp(cNAM.magic, "cNAM", 4)) {
		delete [] cLST_index;
		return -CUI_EMATCH;
	}


	DWORD cNAM_index_len = cNAM.size - cNAM.head_size;
	char *cNAM_index = new char[cNAM_index_len];
	if (!cNAM_index) {
		delete [] cLST_index;
		return -CUI_EMEM;
	}

	if (pkg->pio->read(pkg, cNAM_index, cNAM_index_len)) {
		delete [] cNAM_index;
		delete [] cLST_index;
		return -CUI_EREAD;
	}

	cDAT_head_t cDAT;
	if (pkg->pio->read(pkg, &cDAT, sizeof(cDAT))) {
		delete [] cNAM_index;
		delete [] cLST_index;
		return -CUI_EREAD;
	}

	if (strncmp(cDAT.magic, "cDAT", 4)) {
		delete [] cNAM_index;
		delete [] cLST_index;
		return -CUI_EMATCH;
	}

	pk_entry_t *index_buffer = new pk_entry_t[cLST.entries];
	if (!index_buffer) {
		delete [] cNAM_index;
		delete [] cLST_index;
		return -CUI_EMEM;
	}

	for (DWORD i = 0; i < cLST.entries; ++i) {
		pk_entry_t &entry = index_buffer[i];
		strcpy(entry.name, &cNAM_index[cLST_index[i].name_offset]);
		entry.offset = cLST_index[i].offset + sizeof(ags_head_t) 
			+ cLST.size + cNAM.size + cDAT.head_size;
		entry.length = cLST_index[i].length;
	}
	delete [] cNAM_index;
	delete [] cLST_index;

	pkg_dir->index_entries = cLST.entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = sizeof(pk_entry_t) * cLST.entries;
	pkg_dir->index_entry_length = sizeof(pk_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int ags_pk_parse_resource_info(struct package *pkg,
									struct package_resource *pkg_res)
{
	pk_entry_t *pk_entry;

	pk_entry = (pk_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, pk_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = pk_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = pk_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int ags_pk_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	BYTE *raw = new BYTE[pkg_res->raw_data_length];
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			delete [] raw;
			return -CUI_EREADVEC;
	}

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = raw;
		return 0;
	}

	if (!memcmp(raw, "fSG ", 4)) {
//printf("%s\n", pkg_res->name);
		DWORD SG_size = pkg_res->raw_data_length;
		SG_size -= sizeof(ags_head_t);
		u32 index_entries = 0;
		DWORD offset = sizeof(ags_head_t);
		while (SG_size) {
			ags_head_t *head = (ags_head_t *)(raw + offset);		
			
			// "cJPG" part is valid when bpp is less than 24
			if (!memcmp(head->magic, "cRGB", 4) || !memcmp(head->magic, "cJPG", 4))
				++index_entries;

			SG_size -= head->size;
			offset += head->size;
		}
		if (index_entries == 1) {
			offset = sizeof(ags_head_t);
			ags_head_t *head = (ags_head_t *)(raw + offset);
			if (!memcmp(head->magic, "cRGB", 4)) {
				pkg_res->flags |= PKG_RES_FLAG_REEXT;
				pkg_res->replace_extension = _T(".bmp");
			} else if (!memcmp(head->magic, "cJPG", 4)) {
				pkg_res->flags |= PKG_RES_FLAG_REEXT;
				pkg_res->replace_extension = _T(".jpg");
			} else
				offset = 0;

			int ret = SG_decode(raw + offset, (BYTE **)&pkg_res->actual_data, 
				&pkg_res->actual_data_length);
			delete [] raw;
			if (ret)
				return ret;
			raw = NULL;
		}
	}
	pkg_res->raw_data = raw;

	return 0;
}

/* 资源保存函数 */
static int ags_pk_save_resource(struct resource *res, 
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
static void ags_pk_release_resource(struct package *pkg, 
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

/* 封包卸载函数 */
static void ags_pk_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation ags_pk_operation = {
	ags_pk_match,					/* match */
	ags_pk_extract_directory,		/* extract_directory */
	ags_pk_parse_resource_info,	/* parse_resource_info */
	ags_pk_extract_resource,		/* extract_resource */
	ags_pk_save_resource,			/* save_resource */
	ags_pk_release_resource,		/* release_resource */
	ags_pk_release					/* release */
};

/********************* px *********************/

/* 封包匹配回调函数 */
static int ags_px_match(struct package *pkg)
{
	ags_head_t px_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &px_header, sizeof(px_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(px_header.magic, "fPX ", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	u32 px_size;
	if (pkg->pio->length_of(pkg, &px_size)) {
		pkg->pio->close(pkg);
		return -CUI_ELEN;
	}

	if (px_size != px_header.size) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int ags_px_extract_directory(struct package *pkg,	
									 struct package_directory *pkg_dir)
{
	u32 px_size;
	if (pkg->pio->readvec(pkg, &px_size, 4, 4, IO_SEEK_SET))
		return -CUI_EREADVEC;

	px_size -= sizeof(ags_head_t);

	u32 index_entries = 0;
	while (px_size) {
		ags_head_t head;		
		if (pkg->pio->read(pkg, &head, sizeof(head)))
			return -CUI_EREAD;

		if (!strncmp(head.magic, "cTRK", 4))
			++index_entries;

		px_size -= head.size;

		if (pkg->pio->seek(pkg, head.size - sizeof(head), IO_SEEK_CUR))
			return -CUI_ESEEK;
	}

	px_entry_t *index_buffer = new px_entry_t[index_entries];
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->seek(pkg, sizeof(ags_head_t), IO_SEEK_SET)) {
		delete [] index_buffer;
		return -CUI_ESEEK;
	}

	char res_name_base[MAX_PATH];
	memset(res_name_base, 0, MAX_PATH);
	unicode2sj(res_name_base, MAX_PATH, pkg->name, -1);
	if (char *ext = strstr(res_name_base, "."))
		*ext = 0;
	DWORD offset = sizeof(ags_head_t);
	for (DWORD i = 0; i < index_entries; ++i) {
		cTRK_head_t cTRK;
		if (pkg->pio->read(pkg, &cTRK, sizeof(cTRK))) {
			delete [] index_buffer;
			return -CUI_EREAD;
		}
		if (pkg->pio->seek(pkg, cTRK.size - cTRK.head_size, IO_SEEK_CUR)) {
			delete [] index_buffer;
			return -CUI_ESEEK;
		}		
		sprintf(index_buffer[i].name, "%s_%04d", 
			res_name_base, cTRK.trk_number);
		index_buffer[i].length = cTRK.size;
		index_buffer[i].offset = offset;
		offset += cTRK.size;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = sizeof(px_entry_t) * index_entries;
	pkg_dir->index_entry_length = sizeof(px_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int ags_px_parse_resource_info(struct package *pkg,
									   struct package_resource *pkg_res)
{
	px_entry_t *px_entry;

	px_entry = (px_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, px_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = px_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = px_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int ags_px_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	BYTE *raw = new BYTE[pkg_res->raw_data_length];
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			delete [] raw;
			return -CUI_EREADVEC;
	}

	cTRK_head_t *cTRK = (cTRK_head_t *)raw;
	if (cTRK->type == 0) {
		DWORD act_data_len = cTRK->Samples * cTRK->Channels * cTRK->BitsPerSample / 8;
		if (MySaveAsWAVE(raw + cTRK->head_size, cTRK->size - cTRK->head_size,
				1, cTRK->Channels, cTRK->SamplesPerSec, cTRK->BitsPerSample, 
				NULL, 0, (BYTE **)&pkg_res->actual_data, 
				&pkg_res->actual_data_length, my_malloc)) {
			delete [] raw;
			return -CUI_EMEM;	
		}
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".wav");
	} else if (cTRK->type == 3) {
		pkg_res->actual_data = new BYTE[cTRK->size - cTRK->head_size];
		if (!pkg_res->actual_data) {
			delete [] raw;
			return -CUI_EMATCH;
		}
		memcpy(pkg_res->actual_data, raw + cTRK->head_size, cTRK->size - cTRK->head_size);
		pkg_res->actual_data_length = cTRK->size - cTRK->head_size;
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".ogg");
	} else {
		delete [] raw;
		return -CUI_EMATCH;
	}
	delete [] raw;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation ags_px_operation = {
	ags_px_match,					/* match */
	ags_px_extract_directory,		/* extract_directory */
	ags_px_parse_resource_info,	/* parse_resource_info */
	ags_px_extract_resource,		/* extract_resource */
	ags_pk_save_resource,			/* save_resource */
	ags_pk_release_resource,		/* release_resource */
	ags_pk_release					/* release */
};

/********************* SG *********************/

/* 封包匹配回调函数 */
static int ags_SG_match(struct package *pkg)
{
	ags_head_t SG_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &SG_header, sizeof(SG_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(SG_header.magic, "fSG ", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}


/* 封包索引目录提取函数 */
static int ags_SG_extract_directory(struct package *pkg,	
									 struct package_directory *pkg_dir)
{
	u32 SG_size;
	if (pkg->pio->readvec(pkg, &SG_size, 4, 4, IO_SEEK_SET))
		return -CUI_EREADVEC;

	SG_size -= sizeof(ags_head_t);

	u32 index_entries = 0;
	while (SG_size) {
		ags_head_t head;		
		if (pkg->pio->read(pkg, &head, sizeof(head)))
			return -CUI_EREAD;
		
		// "cJPG" part is valid when bpp is less than 24
		if (!strncmp(head.magic, "cRGB", 4) || !strncmp(head.magic, "cJPG", 4))
			++index_entries;

		SG_size -= head.size;

		if (pkg->pio->seek(pkg, head.size - sizeof(head), IO_SEEK_CUR))
			return -CUI_ESEEK;
	}

	px_entry_t *index_buffer = new px_entry_t[index_entries];
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->seek(pkg, sizeof(ags_head_t), IO_SEEK_SET)) {
		delete [] index_buffer;
		return -CUI_ESEEK;
	}

	char res_name_base[MAX_PATH];
	memset(res_name_base, 0, MAX_PATH);
	unicode2sj(res_name_base, MAX_PATH, pkg->name, -1);
	if (char *ext = strstr(res_name_base, "."))
		*ext = 0;
	DWORD offset = sizeof(ags_head_t);
	for (DWORD i = 0; i < index_entries; ) {
		ags_head_t head;		
		if (pkg->pio->read(pkg, &head, sizeof(head))) {
			delete [] index_buffer;
			return -CUI_EREAD;
		}

		if (pkg->pio->seek(pkg, head.size - sizeof(head), IO_SEEK_CUR)) {
			delete [] index_buffer;
			return -CUI_ESEEK;
		}

		if (!strncmp(head.magic, "cRGB", 4) || !strncmp(head.magic, "cJPG", 4)) {
			sprintf(index_buffer[i].name, "%s_%04d", res_name_base, i);
			index_buffer[i].length = head.size;
			index_buffer[i].offset = offset;
			++i;
		}
		offset += head.size;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = sizeof(px_entry_t) * index_entries;
	pkg_dir->index_entry_length = sizeof(px_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int ags_SG_parse_resource_info(struct package *pkg,
									   struct package_resource *pkg_res)
{
	px_entry_t *px_entry;

	px_entry = (px_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, px_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = px_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = px_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int ags_SG_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	BYTE *raw = new BYTE[pkg_res->raw_data_length];
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET)) {
		delete [] raw;
		return -CUI_EREADVEC;
	}

	int ret = SG_decode(raw, (BYTE **)&pkg_res->actual_data, 
		&pkg_res->actual_data_length);
	if (ret) {
		delete [] raw;
		return ret;
	}

	ags_head_t *head = (ags_head_t *)raw;
	if (!memcmp(head->magic, "cJPG", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".jpg");
	} else if (!memcmp(head->magic, "cRGB", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp");
	}
	delete [] raw;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation ags_SG_operation = {
	ags_SG_match,				/* match */
	ags_SG_extract_directory,	/* extract_directory */
	ags_SG_parse_resource_info,/* parse_resource_info */
	ags_SG_extract_resource,	/* extract_resource */
	ags_pk_save_resource,		/* save_resource */
	ags_pk_release_resource,	/* release_resource */
	ags_pk_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK ags_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".pk"), NULL, 
		NULL, &ags_pk_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_SUFFIX_SENSE))
			return -1;

	if (callback->add_extension(callback->cui, _T(".px"), NULL, 
		NULL, &ags_px_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".SG"), NULL, 
		NULL, &ags_SG_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
