#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>
#include <zlib.h>
#include <utility.h>
#include "mt19937int.h"
#include <vector>

using namespace std;

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information System4_cui_information = {
	_T("アリスソフト"),			/* copyright */
	_T("AliceSoft System4.0"),	/* system */
	_T(".ald .ain .AJP .bgi .acx .alk"),	/* package */
	_T("0.9.2"),				/* revision */
	_T("痴漢公賊"),				/* author */
	_T("2009-3-1 14:30"),		/* date */
	NULL,						/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[2];		// "NL"
	// 0x01 0x00 0x01 0x00 0x00 0x00 0x01 0x** 0x00 0x00 0x00 0x00 0x00 0x00 
	s8 unknown0[7];
	u32 index_entries;	/* 真正有效的文件的数量 */
	s8 unknown1[3];
} ald_tailer_t;

typedef struct {
	u32 entry_length;	/* entry头的长度（包含本字段） */
	u32 length;
	u32 unknown[2];
} ald_entry_t;

typedef struct {
	s8 magic[4];		/* "QNT" */
	s32 version;		/* 0, 1, 2 */
} qnt_head_t;

typedef struct {
	u32 orig_x;			/* 0 ? */
	u32 orig_y;			/* 0 ? */
	u32 width;
	u32 height;
	u32 bpp;
	u32 is_compressed;
	u32 dib_length;
	u32 alpha_length;
	u32 dib_unknown[4];	//???
	u32 zero[2];
} qnt_info_t;

typedef struct {
	qnt_head_t head;
	qnt_info_t info;
} qnt0_header_t;

typedef struct {
	qnt_head_t head;
	u32 dib_data_offset;
	qnt_info_t info;
} qnt_header_t;

typedef struct {
	s8 magic[4];		/* "AJP" */
	s32 version;		/* 0 */
	u32 head_size;		// 0x38
	u32 width;
	u32 height;
	u32 data_offset;
	u32 data_length;
	u32 alpha_offset;
	u32 alpha_length;
	u32 unknown[4];
	u32 zero;
} AJP_header_t;

typedef struct {
	s8 magic[2];		/* "PM" */
	u16 version;		/* ?2 */
	u16 head_size;		/* ?0x40 */
	u8 always_0x10;
	u8 pad0;				// 0 or 8
	u8 unknown;
	u8 pad1;
	u16 always_0xffff;	// -1
	u32 pad2;
	u32 unknown1;
	u32 unknown2;
	u32 width;
	u32 height;

} pm_head_t;

typedef struct {
	s8 magic[4];		// "ACX"
	u32 version;		// 0
	u32 comprlen;
	u32 uncomprlen;
} acx_header_t;

typedef struct {
	s8 magic[4];		/* "ALK0" */
	u32 index_entries;
} alk_header_t;

typedef struct {
	u32 offset;
	u32 length;
} alk_entry_t;
#pragma pack ()

typedef struct {
	char name[MAX_PATH];
	unsigned int length;
	unsigned int offset;
} my_ald_entry_t;

extern unsigned long genrand_int32(void);
extern void lsgenrand(unsigned long seed_array[]);

#define bytes3_to_bytes4(x)	((u32)((((x)[0]) << 8) | (((x)[1]) << 16) | (((x)[2]) << 24)))

static void *my_malloc(DWORD len)
{
	return new BYTE[len];
}

/* 资源保存函数 */
static int System4_save_resource(struct resource *res, 
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
static void System4_release_resource(struct package *pkg, 
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
static void System4_release(struct package *pkg, 
							struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

static void decode(BYTE *data)
{
	BYTE dec[16] = { 
		0x5D, 0x91, 0xAE, 0x87, 
		0x4A, 0x56, 0x41, 0xCD, 
		0x83, 0xEC, 0x4C, 0x92, 
		0xB5, 0xCB, 0x16, 0x34 
	};

	for (DWORD i = 0; i < 16; ++i)
		data[i] ^= dec[i];
}

/********************* ald *********************/

/* 封包匹配回调函数 */
static int System4_ald_match(struct package *pkg)
{
	s8 magic[2];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->readvec(pkg, magic, sizeof(magic), 
			0 - sizeof(ald_tailer_t), IO_SEEK_END)) {
		pkg->pio->close(pkg);
		return -CUI_EREADVEC;
	}

	if (memcmp(magic, "NL", 2)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int System4_ald_extract_directory(struct package *pkg,
										 struct package_directory *pkg_dir)
{
	ald_tailer_t tailer;
	if (pkg->pio->readvec(pkg, &tailer, sizeof(tailer), 
			0 - sizeof(ald_tailer_t), IO_SEEK_END))
		return -CUI_EREADVEC;

	u8 num_index_offset[3];
	if (pkg->pio->readvec(pkg, num_index_offset, sizeof(num_index_offset), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	u32 offset_index_length = bytes3_to_bytes4(num_index_offset) - 3;
	BYTE *offset_index = new BYTE[offset_index_length];
	if (!offset_index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, offset_index, offset_index_length)) {
		delete [] offset_index;
		return -CUI_EREADVEC;
	}

	DWORD num_index_length = bytes3_to_bytes4(offset_index) - bytes3_to_bytes4(num_index_offset);
	DWORD num_index_entries = num_index_length / 3;	
	DWORD act_num_index_length = 3 * num_index_entries;
	BYTE *num_index = new BYTE[act_num_index_length];
	if (!num_index) {
		delete [] offset_index;
		return -CUI_EMEM;
	}

	if (pkg->pio->read(pkg, num_index, act_num_index_length)) {
		delete [] num_index;
		delete [] offset_index;
		return -CUI_EREADVEC;
	}

	DWORD my_index_len = sizeof(my_ald_entry_t) * tailer.index_entries;
	my_ald_entry_t *my_index = new my_ald_entry_t[tailer.index_entries];
	if (!my_index) {
		delete [] num_index;
		delete [] offset_index;
		return -CUI_EREADVEC;		
	}
	memset(my_index, 0, my_index_len);

	BYTE *p_num_index = num_index;
	BYTE *p_offset_index = offset_index;
	for (DWORD i = 0, k = 0; i < num_index_entries; ++i) {
		u8 ald_id = *p_num_index++;			// xxxxT[A-Z], 从1开始一一对应
		u16 res_id = *(u16 *)p_num_index;	// 从1开始
											// 事实上，这个特性也许能用作升级封包。假设WC.ald里面有ald=1的补丁文件（针对WA.ald的补丁）
		p_num_index += 2;

		if (!ald_id)	// 无效
			continue;

		ald_entry_t entry;
		u32 offset = bytes3_to_bytes4(p_offset_index);
		p_offset_index += 3;

		if (pkg->pio->readvec(pkg, &entry, sizeof(entry), offset, IO_SEEK_SET))
			break;

		if (!memcmp(&entry, "NL", 2)) {
			i = num_index_entries;
			break;
		}

		u32 name_len = entry.entry_length - sizeof(entry);

		if (pkg->pio->read(pkg, my_index[k].name, name_len))
			break;

		my_index[k].name[name_len] = 0;
		my_index[k].length = entry.length;
		my_index[k].offset = offset + entry.entry_length;
		++k;
	}
	delete [] num_index;
	delete [] offset_index;
	if (i != num_index_entries) {
		delete [] my_index;
		return -CUI_EREADVEC;
	}

	pkg_dir->index_entries = tailer.index_entries;
	pkg_dir->directory = my_index;
	pkg_dir->directory_length = my_index_len;
	pkg_dir->index_entry_length = sizeof(my_ald_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int System4_ald_parse_resource_info(struct package *pkg,
										   struct package_resource *pkg_res)
{
	my_ald_entry_t *y_ald_entry;

	y_ald_entry = (my_ald_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, y_ald_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = y_ald_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = y_ald_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int System4_ald_extract_resource(struct package *pkg,
										struct package_resource *pkg_res)
{
	DWORD raw_len = pkg_res->raw_data_length;
	BYTE *raw = new BYTE[raw_len];
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, raw_len,
		pkg_res->offset, IO_SEEK_SET)) {
			delete [] raw;
			return -CUI_EREADVEC;
	}

retry:
	if (!memcmp(raw, "QNT", 3)) {
		if (raw_len < 48) {
			delete [] raw;
			return -CUI_EMEM;
		}

		qnt_head_t *head = (qnt_head_t *)raw;
		u32 dib_data_offset;
		qnt_info_t *info;
		if (!head->version) {
			info = (qnt_info_t *)(head + 1);
			dib_data_offset = 48;
		} else if (head->version <= 2) {
			if (head->version == 1)
				printf("%s: version 1!\n", pkg_res->name);
			++head;
			dib_data_offset = *(u32 *)head;
			info = (qnt_info_t *)((BYTE *)head + 4);
		} else {
			delete [] raw;
			return -CUI_EMEM;
		}

		BYTE *dib_compr;
		if (info->dib_length)
			dib_compr = raw + dib_data_offset;
		else
			dib_compr = NULL;

		BYTE *alpha_compr;
		if (info->alpha_length)
			alpha_compr = raw + dib_data_offset + info->dib_length;
		else
			alpha_compr = NULL;

		if (info->bpp != 24) {
			delete [] raw;
			return -CUI_EMATCH;
		}

		DWORD dib_uncomprlen;
		BYTE *dib_uncompr;
		DWORD bmp24_line_len = 0;
		if (dib_compr) {
			if (info->is_compressed) {		
				info->width += info->width & 1;
				info->height += info->height & 1;
				dib_uncomprlen = info->width * info->height * info->bpp / 8;
				dib_uncompr = new BYTE[dib_uncomprlen];
				if (!dib_uncompr) {
					delete [] raw;
					return -CUI_EMEM;
				}

				if (uncompress(dib_uncompr, &dib_uncomprlen, 
						dib_compr, info->dib_length) != Z_OK) {
					delete [] dib_uncompr;
					delete [] raw;
					return -CUI_EUNCOMPR;				
				}
			} else {
				dib_uncompr = dib_compr;
				dib_uncomprlen = info->dib_length;
			}
			
			DWORD BPP = info->bpp / 8;
			bmp24_line_len = (info->width * BPP + 3) & ~3;
			DWORD dib_len = bmp24_line_len * info->height;
			BYTE *uncompr_dib = new BYTE[dib_len];
			if (!uncompr_dib) {
				if (info->is_compressed)
					delete [] dib_uncompr;
				delete [] raw;
				return -CUI_EMEM;	
			}

			BYTE *src = dib_uncompr;
			BYTE *dst = uncompr_dib;
			for (DWORD p = 0; p < BPP; ++p) {
				for (DWORD y = 0; y < ((info->height + 1) / 2); ++y) {	
					dst = uncompr_dib + y * bmp24_line_len * 2 + p;
					/* 该循环用于填充每2行数据中的B, G, R位置的信息 */
					for (DWORD x = 0; x < ((info->width + 1) / 2); ++x) {				
						dst[bmp24_line_len * 0 + x * BPP * 2 + 0] = *src++;
						dst[bmp24_line_len * 1 + x * BPP * 2 + 0] = *src++;
						dst[bmp24_line_len * 0 + x * BPP * 2 + BPP] = *src++;
						dst[bmp24_line_len * 1 + x * BPP * 2 + BPP] = *src++;
					}
				}
			}

			if (info->is_compressed)
				delete [] dib_uncompr;

			for (DWORD x = 0; x < info->width - 1; ++x) {
				for (p = 0; p < BPP; ++p)
					uncompr_dib[(x + 1) * BPP + p] = 
						uncompr_dib[x * BPP + p] - uncompr_dib[(x + 1) * BPP + p];
			}

			for (DWORD y = 0; y < info->height - 1; ++y) {		
				for (DWORD p = 0; p < BPP; ++p)
					uncompr_dib[(y + 1) * bmp24_line_len + p] = 
						uncompr_dib[y * bmp24_line_len + p] - uncompr_dib[(y + 1) * bmp24_line_len + p];
					
				for (DWORD x = 0; x < info->width - 1; ++x) {
					for (p = 0; p < BPP; ++p) {
						s32 tmp;

						tmp = (uncompr_dib[(y + 1) * bmp24_line_len + x * BPP + p] 
							+ uncompr_dib[y * bmp24_line_len + (x + 1) * BPP + p]) >> 1;
						uncompr_dib[(y + 1) * bmp24_line_len + (x + 1) * BPP + p] = 
							(tmp & 0xff) - uncompr_dib[(y + 1) * bmp24_line_len + (x + 1) * BPP + p];
					}
				}
			}

			dib_uncompr = uncompr_dib;
			dib_uncomprlen = dib_len;
		} else {
			dib_uncompr = NULL;
			dib_uncomprlen = 0;
		}
	
		DWORD alpha_uncomprlen;
		BYTE *alpha_uncompr;
		DWORD alpha_line_len = 0;
		if (alpha_compr) {
			if (info->is_compressed) {
				// お嬢様GA.ald: cg06501.QNT
				alpha_uncomprlen = info->width * (info->height + 1);
				alpha_uncompr = new BYTE[alpha_uncomprlen];
				if (!alpha_uncompr) {
					delete [] dib_uncompr;
					delete [] raw;
					return -CUI_EMEM;
				}

				if (uncompress(alpha_uncompr, &alpha_uncomprlen, alpha_compr, 
						info->alpha_length) != Z_OK) {
					delete [] alpha_uncompr;
					delete [] dib_uncompr;
					delete [] raw;
					return -CUI_EUNCOMPR;
				}
			} else {
				alpha_uncompr = alpha_compr;
				alpha_uncomprlen = info->alpha_length;
			}

			alpha_line_len = info->width;

			for (DWORD x = 0; x < info->width - 1; ++x)
				alpha_uncompr[x + 1] = alpha_uncompr[x] - alpha_uncompr[x + 1];

			for (DWORD y = 0; y < info->height - 1; ++y) {		
				alpha_uncompr[(y + 1) * alpha_line_len] = 
					alpha_uncompr[y * alpha_line_len] - alpha_uncompr[(y + 1) * alpha_line_len];
					
				for (x = 0; x < info->width - 1; ++x) {
					s32 tmp;

					tmp = (alpha_uncompr[(y + 1) * alpha_line_len + x] 
						+ alpha_uncompr[y * alpha_line_len + (x + 1)]) >> 1;
					alpha_uncompr[(y + 1) * alpha_line_len + (x + 1)] = 
							(tmp & 0xff) - alpha_uncompr[(y + 1) * alpha_line_len + (x + 1)];
				}
			}
		} else {
			alpha_uncompr = NULL;
			alpha_uncomprlen = 0;
		}

		if (dib_uncompr && alpha_uncompr) {
			DWORD bmp32_line_len = alpha_line_len * 4;
			BYTE *bmp32_buf = new BYTE[bmp32_line_len * info->height];
			if (!bmp32_buf) {
				if (alpha_compr && info->is_compressed)
					delete [] alpha_uncompr;
				delete [] dib_uncompr;
				delete [] raw;
				return -CUI_EMEM;
			}

			for (DWORD y = 0; y < info->height; ++y) {
				BYTE *src = dib_uncompr + y * bmp24_line_len;
				BYTE *dst = bmp32_buf + (info->height - y - 1) * bmp32_line_len;			
				BYTE *alp = alpha_uncompr + y * alpha_line_len;
				for (DWORD x = 0; x < info->width; ++x) {
					dst[x * 4 + 0] = src[x * 3 + 0] * alp[x] / 255 + 255 - alp[x];
					dst[x * 4 + 1] = src[x * 3 + 1] * alp[x] / 255 + 255 - alp[x];
					dst[x * 4 + 2] = src[x * 3 + 2] * alp[x] / 255 + 255 - alp[x];
					dst[x * 4 + 3] = alp[x];
				}
			}

			if (alpha_compr && info->is_compressed)
				delete [] alpha_uncompr;
			delete [] dib_uncompr;

			if (MySaveAsBMP(bmp32_buf, bmp32_line_len * info->height, 
					NULL, 0, info->width, info->height, 32, 0,
					(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, 
					my_malloc)) {
				delete [] bmp32_buf;
				delete [] raw;
				return -CUI_EMEM;				
			}
			delete [] bmp32_buf;
			delete [] raw;
		} else if (dib_uncompr) {
			if (MySaveAsBMP(dib_uncompr, dib_uncomprlen, 
					NULL, 0, info->width, 0 - info->height, 24, 0,
					(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, 
					my_malloc)) {
				delete [] dib_uncompr;
				delete [] raw;
				return -CUI_EMEM;				
			}
			delete [] dib_uncompr;
			delete [] raw;
		} else if (alpha_uncompr) {
			if (MySaveAsBMP(alpha_uncompr, alpha_uncomprlen, 
					NULL, 0, info->width, 0 - info->height, 8, 0,
					(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, 
					my_malloc)) {
				if (alpha_compr && info->is_compressed)
					delete [] alpha_uncompr;
				delete [] raw;
				return -CUI_EMEM;				
			}
			if (alpha_compr && info->is_compressed)
				delete [] alpha_uncompr;
			delete [] raw;
		} else {
			if (alpha_compr && info->is_compressed)
				delete [] alpha_uncompr;
			delete [] dib_uncompr;
			pkg_res->raw_data = raw;
		}
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".bmp");
	} else if (!memcmp(raw, "PM", 2)) {
#if 0
		pm_head_t *head = (pm_head_t *)raw;
		DWORD line_len = head->width * 3;
		u8 *in = (u8 *)head + data_offset;
		u8 *out = new u8[line_len * head->height];
		if (!out) {
			delete [] raw;
			return -CUI_EMEM;
		}

		for (DWORD y = 0; y < head->height; ++y) {
			for (DWORD x = 0; x < head->width; ) {
				DWORD d = *in++;
				if (d < 0xf8) {
					d |= *in++ << 8;
					*out++ = (((u8)d >> 2) & 7) + ((d & 0x1f) << 3);
					*out++ = (((d >> 3) ^ (d >> 9)) & 3) ^ (d >> 3);
					*out++ = (((d >> 13) ^ (d >> 8)) & 7) ^ (d >> 8);
					++x;
				} else if (d == 0xf8) {
					d = *(u16 *)in;
					in += 2;
					*out++ = ((d & 0x1f) << 3) + (((u8)d >> 2) & 7);
					*out++ = (d >> 3) ^ ((d >> 3) ^ (d >> 9)) & 3;
					*out++ = (d >> 8) ^ ((d >> 8) ^ (d >> 13)) & 7;
					++x;
				} else if (d == 0xf9) {
					DWORD cnt = *in++ + 1;

					BYTE dd[3];
					dd[0] = ((d >> 2) & 7) + ((d & 0x1f) << 3);
					dd[1] = (((d >> 3) ^ (d >> 9)) & 3) ^ (d >> 3);
					dd[2] = (((d >> 13) ^ (d >> 8)) & 7) ^ (d >> 8);

					x += cnt;
					for (DWORD j = 0; j < cnt; ++j) {
						*out++ = dd[0];
						*out++ = dd[1];
						*out++ = dd[2];							
					}			
				} else if (d == 0xfa) {
					u8 *src = out - line_len + 3;
					*out++ = *src++;
					*out++ = *src++;
					*out++ = *src++;
					++x;
				} else if (d == 0xfb) {
					u8 *src = out - line_len - 3;
					*out++ = *src++;
					*out++ = *src++;
					*out++ = *src++;
					++x;				
				} else if (d == 0xfc) {
					DWORD cnt = *in++ + 2;
					u16 dd[2];

					dd[0] = *(u16 *)in;
					in += 2;
					dd[1] = *(u16 *)in;
					in += 2;

					BYTE ddd[6];
					ddd[0] = ((dd[0] & 0x1F) << 3) + (((u8)dd[0] >> 2) & 7);
					ddd[1] = (dd[0] >> 3) ^ (((dd[0] >> 3) ^ (dd[0] >> 9)) & 3;
					ddd[2] = (dd[0] >> 8) ^ (((dd[0] >> 8) ^ (dd[0] >> 13)) & 7);
					ddd[3] = ((dd[1] & 0x1F) << 3) + (((u8)dd[1] >> 2) & 7);
					ddd[4] = (dd[1] >> 3) ^ ((dd[1] >> 3) ^ (dd[1] >> 9)) & 3;
					ddd[5] = (dd[1] >> 8) ^ ((dd[1] >> 8) ^ (dd[1] >> 13)) & 7;

					x += cnt * 2;
					for (DWORD j = 0; j < cnt; ++j) {
						*out++ = ddd[0];
						*out++ = ddd[1];
						*out++ = ddd[2];
						*out++ = ddd[3];
						*out++ = ddd[4];
						*out++ = ddd[5];						
					}	
				} else if (d == 0xfd) {
					DWORD cnt = *in++ + 3;
					d = *(u16 *)in;
					in += 2;

					BYTE dd[3];
					dd[0] = ((d >> 2) & 7) + ((d & 0x1f) << 3);
					dd[1] = (((d >> 3) ^ (d >> 9)) & 3) ^ (d >> 3);
					dd[2] = (((d >> 13) ^ (d >> 8)) & 7) ^ (d >> 8);
					x += cnt;
					for (DWORD j = 0; j < cnt; ++j) {
						*out++ = dd[0];
						*out++ = dd[1];
						*out++ = dd[2];							
					}
				} else if (d == 0xfe) {
					DWORD cnt = *in++ + 2;
					u8 *src = out - 2 * line_len;
					memcpy(out, src, cnt * 3);
					x += cnt;
					out += cnt * 3;
				} else if (d == 0xff) {
					DWORD cnt = *in++ + 2;
					u8 *src = out - line_len;
					memcpy(out, src, cnt * 3);
					x += cnt;
					out += cnt * 3;
				}
			} else {	// 1061a8b

			}


		}
#endif
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".PM");
		pkg_res->raw_data = raw;
		pkg_res->raw_data_length = raw_len;
	} else if (!memcmp(raw, "BM", 2)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".bmp");
		pkg_res->raw_data = raw;
		pkg_res->raw_data_length = raw_len;
	} else if (!memcmp(raw, "ROU", 3)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".ROU");
		pkg_res->raw_data = raw;
		pkg_res->raw_data_length = raw_len;
	} else if (!memcmp(raw, "AJP", 3)) {
		AJP_header_t *ajp_header = (AJP_header_t *)raw;
		if (raw_len < 36 || ajp_header->version) {
			delete [] raw;
			return -CUI_EMATCH;
		}

		if (!ajp_header->data_length || !ajp_header->alpha_length) {
			if (ajp_header->alpha_length > 0 && ajp_header->alpha_offset >= 0 
					&& ajp_header->alpha_offset < raw_len) {
				BYTE *unknown1 = raw + ajp_header->alpha_offset;
				decode(unknown1);
				BYTE *act_data = new BYTE[ajp_header->alpha_length];
				if (!act_data) {
					delete [] raw;
					return -CUI_EMEM;
				}
				memcpy(act_data, unknown1, ajp_header->alpha_length);
				delete [] raw;
				raw = act_data;
				raw_len = ajp_header->alpha_length;
				goto retry;
			}

			if (ajp_header->data_length > 0 && ajp_header->data_offset >= 0 
					&& ajp_header->data_offset < raw_len) {
				BYTE *unknown0 = raw + ajp_header->data_offset;
				decode(unknown0);
				BYTE *act_data = new BYTE[ajp_header->data_length];
				if (!act_data) {
					delete [] raw;
					return -CUI_EMEM;
				}
				memcpy(act_data, unknown0, ajp_header->data_length);
				delete [] raw;
				raw = act_data;
				raw_len = ajp_header->data_length;
				goto retry;
			}
		} else {
			decode(raw + ajp_header->data_offset);
			decode(raw + ajp_header->alpha_offset);
			pkg_res->raw_data = raw;
			pkg_res->raw_data_length = raw_len;			
		}
	} else if (!memcmp(raw, "\xff\xd8\xff\xe0\x00\x10\x4a\x46\x49\x46\x00", 11)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".jpg");
		pkg_res->actual_data = raw;
		pkg_res->actual_data_length = raw_len;
	} else if (!memcmp(raw, "RIFF", 4)) {	// 本该在alk中处理
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".wav");
		pkg_res->actual_data = raw;
		pkg_res->actual_data_length = raw_len;	
	} else {
		pkg_res->actual_data = raw;
		pkg_res->actual_data_length = raw_len;
	}

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation System4_ald_operation = {
	System4_ald_match,				/* match */
	System4_ald_extract_directory,	/* extract_directory */
	System4_ald_parse_resource_info,/* parse_resource_info */
	System4_ald_extract_resource,	/* extract_resource */
	System4_save_resource,			/* save_resource */
	System4_release_resource,		/* release_resource */
	System4_release					/* release */
};

/********************* AJP *********************/

/* 封包匹配回调函数 */
static int System4_AJP_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREADVEC;
	}

	if (memcmp(magic, "AJP", 3)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int System4_AJP_extract_directory(struct package *pkg,
										 struct package_directory *pkg_dir)
{
	AJP_header_t AJP_header;
	if (pkg->pio->readvec(pkg, &AJP_header, sizeof(AJP_header), 
			0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	vector <my_ald_entry_t>index;
	DWORD index_entries = 0;
	my_ald_entry_t entry;
	if (AJP_header.data_length) {
		unicode2sj(entry.name, sizeof(entry.name), pkg->name, -1);
		*strstr(entry.name, ".") = 0;
		strcat(entry.name, "_0");
		entry.length = AJP_header.data_length;
		entry.offset = AJP_header.data_offset;
		index.push_back(entry);
		++index_entries;
	}
	if (AJP_header.alpha_length) {
		unicode2sj(entry.name, sizeof(entry.name), pkg->name, -1);
		*strstr(entry.name, ".") = 0;
		strcat(entry.name, "_1");
		entry.length = AJP_header.alpha_length;
		entry.offset = AJP_header.alpha_offset;
		index.push_back(entry);
		++index_entries;
	}

	my_ald_entry_t *my_index = new my_ald_entry_t[index_entries];
	if (!my_index)
		return -CUI_EMEM;

	for (DWORD i = 0; i < index_entries; ++i) {
		strcpy(my_index[i].name, index[i].name);
		my_index[i].offset = index[i].offset;
		my_index[i].length = index[i].length;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = my_index;
	pkg_dir->directory_length = index_entries * sizeof(my_ald_entry_t);
	pkg_dir->index_entry_length = sizeof(my_ald_entry_t);

	return 0;
}

static cui_ext_operation System4_AJP_operation = {
	System4_AJP_match,				/* match */
	System4_AJP_extract_directory,	/* extract_directory */
	System4_ald_parse_resource_info,/* parse_resource_info */
	System4_ald_extract_resource,	/* extract_resource */
	System4_save_resource,			/* save_resource */
	System4_release_resource,		/* release_resource */
	System4_release					/* release */
};


/********************* alk *********************/

/* 封包匹配回调函数 */
static int System4_alk_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREADVEC;
	}

	if (memcmp(magic, "ALK0", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int System4_alk_extract_directory(struct package *pkg,
										 struct package_directory *pkg_dir)
{
	alk_header_t alk_header;
	if (pkg->pio->readvec(pkg, &alk_header, sizeof(alk_header), 
			0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD index_len = alk_header.index_entries * sizeof(alk_entry_t);
	alk_entry_t *index = new alk_entry_t[alk_header.index_entries];
	if (!index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index, index_len)) {
		free(index);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = alk_header.index_entries;
	pkg_dir->directory = index;
	pkg_dir->directory_length = index_len;
	pkg_dir->index_entry_length = sizeof(alk_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int System4_alk_parse_resource_info(struct package *pkg,
										   struct package_resource *pkg_res)
{
	alk_entry_t *alk_entry;

	alk_entry = (alk_entry_t *)pkg_res->actual_index_entry;
	sprintf(pkg_res->name, "%04d", pkg_res->index_number);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = alk_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = alk_entry->offset;

	return 0;
}

static cui_ext_operation System4_alk_operation = {
	System4_alk_match,				/* match */
	System4_alk_extract_directory,	/* extract_directory */
	System4_alk_parse_resource_info,/* parse_resource_info */
	System4_ald_extract_resource,	/* extract_resource */
	System4_save_resource,			/* save_resource */
	System4_release_resource,		/* release_resource */
	System4_release					/* release */
};

/********************* ain *********************/

/* 封包匹配回调函数 */
static int System4_ain_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包资源提取函数 */
static int System4_ain_extract_resource(struct package *pkg,
										struct package_resource *pkg_res)
{	
	DWORD ain_size = pkg_res->raw_data_length;
	BYTE *ain = new BYTE[ain_size];
	if (!ain)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, ain, ain_size, 0, IO_SEEK_SET)) {
		delete [] ain;
		return -CUI_EREADVEC;
	}

	unsigned long seed = 0x5d3e3;
	unsigned long seed_table[624];
	for (unsigned int i = 0; i < 624; ++i) {
		seed_table[i] = seed;
		seed *= 0x10dcd;
	}
	lsgenrand(seed_table);

	for (i = 0; i < ain_size; ++i)
		ain[i] ^= genrand();
	
	pkg_res->actual_data = ain;
	pkg_res->actual_data_length = ain_size;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation System4_ain_operation = {
	System4_ain_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	System4_ain_extract_resource,	/* extract_resource */
	System4_save_resource,			/* save_resource */
	System4_release_resource,		/* release_resource */
	System4_release					/* release */
};

/********************* bgi *********************/

/* 封包匹配回调函数 */
static int System4_bgi_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包资源提取函数 */
static int System4_bgi_extract_resource(struct package *pkg,
										struct package_resource *pkg_res)
{	
	DWORD bgi_size = pkg_res->raw_data_length;
	BYTE *bgi = new BYTE[bgi_size];
	if (!bgi)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, bgi, bgi_size, 0, IO_SEEK_SET)) {
		delete [] bgi;
		return -CUI_EREADVEC;
	}

	for (unsigned int i = 0; i < bgi_size; ++i)
		bgi[i] = (bgi[i] >> 4) | (bgi[i] << 4);
	
	pkg_res->actual_data = bgi;
	pkg_res->actual_data_length = bgi_size;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation System4_bgi_operation = {
	System4_bgi_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	System4_bgi_extract_resource,	/* extract_resource */
	System4_save_resource,			/* save_resource */
	System4_release_resource,		/* release_resource */
	System4_release					/* release */
};

/********************* acx *********************/

/* 封包匹配回调函数 */
static int System4_acx_match(struct package *pkg)
{
	s8 magic[4];
	u32 version;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, 4)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "ACX", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->pio->read(pkg, &version, 4)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (version) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包资源提取函数 */
static int System4_acx_extract_resource(struct package *pkg,
										struct package_resource *pkg_res)
{
	DWORD acx_size = pkg_res->raw_data_length;
	BYTE *acx = new BYTE[acx_size];
	if (!acx)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, acx, acx_size, 0, IO_SEEK_SET)) {
		delete [] acx;
		return -CUI_EREADVEC;
	}

	acx_header_t *acx_header = (acx_header_t *)acx;
	BYTE *compr = acx + sizeof(acx_header_t);	
	BYTE *uncompr = new BYTE[acx_header->uncomprlen];
	if (!uncompr) {
		delete [] acx;
		return -CUI_EMEM;
	}

	pkg_res->actual_data_length = acx_header->uncomprlen;
	if (uncompress(uncompr, &pkg_res->actual_data_length,
			compr, acx_header->comprlen) != Z_OK) {
		delete [] uncompr;
		delete [] acx;
		return -CUI_EUNCOMPR;
	}
	delete [] acx;	
	pkg_res->actual_data = uncompr;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation System4_acx_operation = {
	System4_acx_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	System4_acx_extract_resource,	/* extract_resource */
	System4_save_resource,			/* save_resource */
	System4_release_resource,		/* release_resource */
	System4_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK System4_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".ald"), NULL, 
		NULL, &System4_ald_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".AJP"), NULL, 
		NULL, &System4_AJP_operation, CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".ain"), _T(".ain_"), 
		NULL, &System4_ain_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".bgi"), _T(".bgi.txt"), 
		NULL, &System4_bgi_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;	

	if (callback->add_extension(callback->cui, _T(".acx"), _T(".acx_"), 
		NULL, &System4_acx_operation, CUI_EXT_FLAG_PKG))
			return -1;	

	if (callback->add_extension(callback->cui, _T(".alk"), NULL, 
		NULL, &System4_alk_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;
	
	return 0;
}
