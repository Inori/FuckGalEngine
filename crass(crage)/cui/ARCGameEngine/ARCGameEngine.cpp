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

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information ARCGameEngine_cui_information = {
	_T("ARC Software Laboratory"),	/* copyright */
	_T("ARCGameEngine"),			/* system */
	_T(".ALF .BIN .AGF"),			/* package */
	_T("0.9.8"),					/* revision */
	_T("痴漢公賊"),					/* author */
	_T("2009-5-22 23:30"),			/* date */
	NULL,							/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];		// "S3IC", "S3IN", "S4IC", "S4IN"
	s8 version[4];		// "XXX "
	u8 unknown[0x124];	// 可能是一些配置用信息（运行时）
} BIN_header_t;

typedef struct {
	u32	actual_length;
	u32	uncomprlen;
	u32	comprlen;
} BIN_COMPRESSINFO_t;

typedef struct {
	u32 ALF_count;		// ALF封包的个数
} BIN_index_header_t;

typedef struct {
	s8 ALF_name[256];
} BIN_index_entry_t;

typedef struct {
	u32 entries;		// 资源总个数
} BIN_entry_header_t;

typedef struct {
	s8 name[64];
	u32 ALF_id;		// 所属的ALF封包编号（ALF名称命名按照BIN_index_entry_t中的命名）
	u32 id;			// 没掌握到分配规则
	u32 offset;
	u32 length;
} BIN_entry_t;

typedef struct {
	s8 magic[4];	// "ACGF"
	u32 type;		// 1 or 2, etc是非法(把type理解为额外压缩块的个数是否更好些?)
	u32 zero;
	BIN_COMPRESSINFO_t info;
} AGF_header_t;

typedef struct {
	s8 magic[2];	// 0000
	u32 total_size;
	u32 reserved;
	u32 offset;		// 0x36
	u16 pad;
	BITMAPINFOHEADER info;
} BM_header_t;

typedef struct {
	s8 magic[4];	// "ACIF"
	u32 type;		// 2
	u32 zero;
	u32 length;
	u32 width;
	u32 height;
	BIN_COMPRESSINFO_t info;
} ACIF_header_t;

typedef struct {
	s8 magic[4];		// "S3AB", "S4AB"
	u32 unknown0;		// 1
	u32 unknown1;		// 0
	u32 act_length;
	u8 unknown[16];
	BIN_COMPRESSINFO_t info;
} AB_header_t;

typedef struct {
	s8 magic[4];		// "0.03"
	u32 unknown[2];
	u32 index_entries;
} idx_header_t;

typedef struct {
	s8 name[64];
	u32 offset_lo;
	u32 offset_hi;
	u32 length_lo;
	u32 length_hi;
} idx_entry_t;
#pragma pack ()


static DWORD lzss_uncompress(BYTE *compr, DWORD comprlen, BYTE *uncompr)
{
	unsigned int act_uncomprlen = 0;
	/* compr中的当前字节中的下一个扫描位的位置 */
	unsigned int curbit = 0;
	/* compr中的当前扫描字节 */
	unsigned int curbyte = 0;
	unsigned int nCurWindowByte = 0xfee;
	unsigned int win_size = 4096;
	BYTE win[4096];
	DWORD flag = 0;

	memset(win, 0, nCurWindowByte);
	while (1) {
		flag >>= 1;
		if (!(flag & 0x0100)) {
			if (curbyte >= comprlen)
				break;
			flag = compr[curbyte++] | 0xff00;
		}

		if (flag & 1) {
			unsigned char data;

			if (curbyte >= comprlen)
				break;

			data = compr[curbyte++];
			uncompr[act_uncomprlen++] = data;
			/* 输出的1字节放入滑动窗口 */
			win[nCurWindowByte++] = data;
			nCurWindowByte &= win_size - 1;
		} else {
			unsigned int copy_bytes, win_offset;
			unsigned int i;

			if (curbyte >= comprlen)
				break;
			win_offset = compr[curbyte++];

			if (curbyte >= comprlen)
				break;
			copy_bytes = compr[curbyte++];
			win_offset |= (copy_bytes & 0xf0) << 4;
			copy_bytes &= 0x0f;
			copy_bytes += 3;

			for (i = 0; i < copy_bytes; i++) {	
				unsigned char data;

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

/********************* ALF *********************/

/* 封包匹配回调函数 */
static int ARCGameEngine_ALF_match(struct package *pkg)
{
	s8 magic[4];
	s8 version[4];

	if (pkg->pio->open(pkg->lst, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg->lst, magic, sizeof(magic))) {
		pkg->pio->close(pkg->lst);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "S3IC", 4) && strncmp(magic, "S3IN", 4)
			&& strncmp(magic, "S4IC", 4) && strncmp(magic, "S4IN", 4)) {
		pkg->pio->close(pkg->lst);
		return -CUI_EMATCH;	
	}

	if (pkg->pio->read(pkg->lst, version, sizeof(version))) {
		pkg->pio->close(pkg->lst);
		return -CUI_EREAD;
	}

	s8 *check = "000 ";
	for (DWORD i = 0; i < 4; i++) {
		if (version[i] < check[i])
			break;
	}
	if (i != 4) {
		pkg->pio->close(pkg->lst);
		return -CUI_EMATCH;
	}

	if (pkg->pio->open(pkg, IO_READONLY)) {
		pkg->pio->close(pkg->lst);
		return -CUI_EOPEN;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int ARCGameEngine_ALF_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	BIN_header_t BIN_header;
	BYTE *pindex;
	DWORD pindex_len;

	if (pkg->pio->readvec(pkg->lst, &BIN_header, sizeof(BIN_header), 
			0, IO_SEEK_SET)) 
		return -CUI_EREADVEC;

	if (!strncmp(BIN_header.magic, "S3IC", 4) || !strncmp(BIN_header.magic, "S4IC", 4)) {
		BIN_COMPRESSINFO_t BIN_COMPRESSINFO;

		if (pkg->pio->read(pkg->lst, &BIN_COMPRESSINFO, sizeof(BIN_COMPRESSINFO))) 
			return -CUI_EREAD;
	
		BYTE *compr = (BYTE *)malloc(BIN_COMPRESSINFO.comprlen);
		if (!compr)
			return -CUI_EMEM;

		if (pkg->pio->read(pkg->lst, compr,	BIN_COMPRESSINFO.comprlen)) {
			free(compr);
			return -CUI_EREAD;
		}

		if (BIN_COMPRESSINFO.comprlen != BIN_COMPRESSINFO.actual_length) {
			BYTE *uncompr = (BYTE *)malloc(BIN_COMPRESSINFO.uncomprlen + 2);
			if (!uncompr) {
				free(compr);
				return -CUI_EMEM;
			}

			lzss_uncompress(compr, BIN_COMPRESSINFO.comprlen, uncompr);
			free(compr);
			pindex = uncompr;
		} else
			pindex = compr;
		pindex_len = BIN_COMPRESSINFO.actual_length;
	} else {	// "S3IN", "S4IN"
		u32 size;
		if (pkg->pio->length_of(pkg->lst, &size))
			return -CUI_ELEN;

		size -= sizeof(BIN_header);
		BYTE *raw = (BYTE *)malloc(size);
		if (!raw)
			return -CUI_EMEM;

		if (pkg->pio->read(pkg->lst, raw, size)) {
			free(raw);
			return -CUI_EREAD;
		}		
		pindex = raw;
		pindex_len = size;
	}

	BYTE *p = pindex;
	u32 ALF_count = *(u32 *)p;
	p += 4;
	BIN_index_entry_t *ALF_index_entry = (BIN_index_entry_t *)p;

	char ALF_name[MAX_PATH];
	unicode2sj(ALF_name, MAX_PATH, pkg->name, -1);
	for (DWORD i = 0; i < ALF_count; i++) {
		if (!strcmp(ALF_name, ALF_index_entry[i].ALF_name))
			break;
	}
	if (i == ALF_count) {
		free(pindex);
		return -CUI_EMATCH;
	}

	DWORD current_ALF_id = i;

	p += ALF_count * sizeof(BIN_index_entry_t);
	u32 total_index_entries = *(u32 *)p;
	p += 4;

	BIN_entry_t *entry = (BIN_entry_t *)p;
	DWORD index_entries = 0;
	for (i = 0; i < total_index_entries; ++i) {
#if 0
		printf("%s: ALFid %x, id %x, %x @ %x\n",
			entry->name, entry->ALF_id, entry->id,
			entry->length,entry->offset);
#endif
		if (entry->ALF_id == current_ALF_id)
			++index_entries;
		++entry;		
	}

	DWORD index_len = index_entries * sizeof(BIN_entry_t);	
	BIN_entry_t *index_buffer = (BIN_entry_t *)malloc(index_len);
	if (!index_buffer) {
		free(pindex);
		return -CUI_EMEM;
	}

	entry = (BIN_entry_t *)p;
	DWORD k = 0;
	for (i = 0; i < total_index_entries; ++i) {
		if (entry->ALF_id == current_ALF_id)
			index_buffer[k++] = *entry;
		++entry;		
	}
	free(pindex);

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_len;
	pkg_dir->index_entry_length = sizeof(BIN_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int ARCGameEngine_ALF_parse_resource_info(struct package *pkg,
												 struct package_resource *pkg_res)
{
	BIN_entry_t *ALF_entry;

	ALF_entry = (BIN_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, ALF_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = ALF_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = ALF_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int ARCGameEngine_ALF_extract_resource(struct package *pkg,
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

/* 资源保存函数 */
static int ARCGameEngine_ALF_save_resource(struct resource *res, 
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
static void ARCGameEngine_ALF_release_resource(struct package *pkg, 
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
static void ARCGameEngine_ALF_release(struct package *pkg, 
									  struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg->lst);
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation ARCGameEngine_ALF_operation = {
	ARCGameEngine_ALF_match,				/* match */
	ARCGameEngine_ALF_extract_directory,	/* extract_directory */
	ARCGameEngine_ALF_parse_resource_info,	/* parse_resource_info */
	ARCGameEngine_ALF_extract_resource,		/* extract_resource */
	ARCGameEngine_ALF_save_resource,		/* save_resource */
	ARCGameEngine_ALF_release_resource,		/* release_resource */
	ARCGameEngine_ALF_release				/* release */
};

/********************* ALF_idx *********************/

/* 封包匹配回调函数 */
static int ARCGameEngine_ALF_idx_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg->lst, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg->lst, magic, sizeof(magic))) {
		pkg->pio->close(pkg->lst);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "0.03", 4)) {
		pkg->pio->close(pkg->lst);
		return -CUI_EMATCH;	
	}

	if (pkg->pio->open(pkg, IO_READONLY)) {
		pkg->pio->close(pkg->lst);
		return -CUI_EOPEN;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int ARCGameEngine_ALF_idx_extract_directory(struct package *pkg,
												   struct package_directory *pkg_dir)
{
	idx_header_t idx_header;

	if (pkg->pio->readvec(pkg->lst, &idx_header, sizeof(idx_header), 
			0, IO_SEEK_SET)) 
		return -CUI_EREADVEC;

	DWORD index_len = sizeof(idx_entry_t) * idx_header.index_entries;
	idx_entry_t *index = (idx_entry_t *)malloc(index_len);
	if (!index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg->lst, index, index_len)) {
		free(index);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = idx_header.index_entries;
	pkg_dir->directory = index;
	pkg_dir->directory_length = index_len;
	pkg_dir->index_entry_length = sizeof(idx_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int ARCGameEngine_ALF_idx_parse_resource_info(struct package *pkg,
													 struct package_resource *pkg_res)
{
	idx_entry_t *idx_entry;

	idx_entry = (idx_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, idx_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = idx_entry->length_lo;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = idx_entry->offset_lo;

	return 0;
}
/* 封包处理回调函数集合 */
static cui_ext_operation ARCGameEngine_ALF_idx_operation = {
	ARCGameEngine_ALF_idx_match,				/* match */
	ARCGameEngine_ALF_idx_extract_directory,	/* extract_directory */
	ARCGameEngine_ALF_idx_parse_resource_info,	/* parse_resource_info */
	ARCGameEngine_ALF_extract_resource,			/* extract_resource */
	ARCGameEngine_ALF_save_resource,			/* save_resource */
	ARCGameEngine_ALF_release_resource,			/* release_resource */
	ARCGameEngine_ALF_release					/* release */
};

/********************* BIN *********************/

/* 封包匹配回调函数 */
static int ARCGameEngine_AB_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg->lst);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "S3AB", 4) && strncmp(magic, "S4AB", 4)) {
		pkg->pio->close(pkg->lst);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int ARCGameEngine_AB_extract_resource(struct package *pkg,
											  struct package_resource *pkg_res)
{
	AB_header_t AB_header;
	if (pkg->pio->readvec(pkg, &AB_header, sizeof(AB_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	BYTE *compr = (BYTE *)malloc(AB_header.info.comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, compr, AB_header.info.comprlen)) {
		free(compr);
		return -CUI_EREAD;
	}

	for (DWORD i = 0; i < AB_header.info.comprlen; ++i)
		compr[i] = ~compr[i];

	if (AB_header.info.actual_length != AB_header.info.comprlen) {
		BYTE *uncompr = (BYTE *)malloc(AB_header.info.uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}

		lzss_uncompress(compr, AB_header.info.comprlen, uncompr);
		free(compr);
		pkg_res->actual_data = uncompr;
		pkg_res->actual_data_length = AB_header.info.uncomprlen;		
	} else {
		pkg_res->raw_data = compr;
		pkg_res->raw_data_length = AB_header.info.comprlen;
	}

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation ARCGameEngine_AB_operation = {
	ARCGameEngine_AB_match,				/* match */
	NULL,								/* extract_directory */
	NULL,								/* parse_resource_info */
	ARCGameEngine_AB_extract_resource,	/* extract_resource */
	ARCGameEngine_ALF_save_resource,	/* save_resource */
	ARCGameEngine_ALF_release_resource,	/* release_resource */
	ARCGameEngine_ALF_release			/* release */
};

/********************* AGF *********************/

/* 封包匹配回调函数 */
static int ARCGameEngine_AGF_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包资源提取函数 */
static int ARCGameEngine_AGF_extract_resource(struct package *pkg,
											  struct package_resource *pkg_res)
{
	u32 AGF_size;
	if (pkg->pio->length_of(pkg, &AGF_size))
		return -CUI_ELEN;
	
	BYTE *raw = (BYTE *)malloc(AGF_size);
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, AGF_size, 0, IO_SEEK_SET)) {
		free(raw);
		return -CUI_EREADVEC;
	}

	if (!memcmp(raw, "\x00\x00\x01\xba", 4)) {
		pkg_res->raw_data = raw;
		pkg_res->raw_data_length = AGF_size;
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".mpg");
	} else if (!strncmp((char *)raw, "ACGF", 4) || memcmp(raw, "BM", 2)) {
		AGF_header_t *AGF = (AGF_header_t *)raw;
		if ((AGF->type != 1 && AGF->type != 2) || AGF->zero) {
			free(raw);
			return -CUI_EMATCH;
		}

		BYTE *compr = (BYTE *)(AGF + 1);
		BYTE *uncompr = (BYTE *)malloc(AGF->info.uncomprlen + 2);
		if (!uncompr) {
			free(raw);
			return -CUI_EMEM;
		}

		DWORD act_uncomprlen;
		if (AGF->info.actual_length != AGF->info.comprlen)
			act_uncomprlen = lzss_uncompress(compr, AGF->info.comprlen, uncompr);
		else {
			memcpy(uncompr, compr, AGF->info.uncomprlen);
			act_uncomprlen = AGF->info.uncomprlen;
		}

		BM_header_t *bmp_header = (BM_header_t *)uncompr;
		u32 *pal = (u32 *)(bmp_header + 1);
		BIN_COMPRESSINFO_t *info = (BIN_COMPRESSINFO_t *)(compr + AGF->info.comprlen);
		compr = (BYTE *)(info + 1);
		uncompr = (BYTE *)malloc(info->uncomprlen + 2);
		if (!uncompr) {
			free(bmp_header);
			free(raw);
			return -CUI_EMEM;
		}

		if (info->actual_length != info->comprlen)
			act_uncomprlen = lzss_uncompress(compr, info->comprlen, uncompr);
		else {
			memcpy(uncompr, compr, info->uncomprlen);
			act_uncomprlen = info->uncomprlen;
		}

		BYTE *dib = uncompr;
		BYTE *mask = NULL;
		if (AGF->type == 2) {
			ACIF_header_t *ACIF_header = (ACIF_header_t *)(compr + info->comprlen);
			BYTE *ACIF_compr = (BYTE *)(ACIF_header + 1);
			BYTE *ACIF_uncompr = (BYTE *)malloc(ACIF_header->info.uncomprlen + 2);
			if (!ACIF_uncompr) {
				free(dib);
				free(bmp_header);
				free(raw);
				return -CUI_EMEM;
			}

			if (ACIF_header->info.actual_length != ACIF_header->info.comprlen)
				act_uncomprlen = lzss_uncompress(ACIF_compr, ACIF_header->info.comprlen, ACIF_uncompr);
			else {
				memcpy(ACIF_uncompr, ACIF_compr, ACIF_header->info.uncomprlen);	
				act_uncomprlen = ACIF_header->info.uncomprlen;
			}
			bmp_header->info.biSizeImage = bmp_header->info.biWidth * bmp_header->info.biHeight * 4;
			bmp_header->offset = 0x36;
			bmp_header->total_size = bmp_header->info.biSizeImage + bmp_header->offset;
			mask = ACIF_uncompr;
		}

		BYTE *act_data = (BYTE *)malloc(bmp_header->total_size + 3);
		if (!act_data) {
			free(mask);
			free(dib);
			free(bmp_header);
			free(raw);
			return -CUI_EMEM;
		}

		if (AGF->type == 2) {
			BYTE *s = dib;
			BYTE *d = act_data + bmp_header->offset;	
			// mask是不对齐的
			DWORD m_line_len = bmp_header->info.biWidth;
			BYTE *m = mask + (bmp_header->info.biHeight - 1) * m_line_len;
			DWORD align;
			if (bmp_header->info.biBitCount == 24) {
				align = (4 - ((bmp_header->info.biWidth * 3) & 3)) & 3;
				for (int y = 0; y < bmp_header->info.biHeight; ++y) {
					for (int x = 0; x < bmp_header->info.biWidth; ++x) {
#if 0	// 不做blending 因为某些图效果反而不好了
						BYTE a = *m++;
						d[0] = s[0] * a / 255 + (255 - a);
						d[1] = s[1] * a / 255 + (255 - a);
						d[2] = s[2] * a / 255 + (255 - a);
						d[3] = a;
#else
						d[0] = s[0];
						d[1] = s[1];
						d[2] = s[2];
						d[3] = *m++;
#endif
						s += 3;
						d += 4;
					}
					s += align;
					m -= m_line_len * 2;
				}
			} else if (bmp_header->info.biBitCount == 4) {				
				align = ((((bmp_header->info.biWidth * bmp_header->info.biBitCount + 7) & ~7) / 8 + 3) & ~3)
					- bmp_header->info.biWidth / 2;
				for (int y = 0; y < bmp_header->info.biHeight; ++y) {
					for (int x = 0; x < bmp_header->info.biWidth; ++x) {
						if (x & 1)
							*(u32 *)d = pal[(*s++ & 0xf0) >> 4];
						else
							*(u32 *)d = pal[(*s & 0xf)];
						d[3] = *m++;
						d += 4;
					}
					s += align;
					m -= m_line_len * 2;
				}
			} else {
				//u32 *pal = (u32 *)(act_data + bmp_header->offset);
				align = ((bmp_header->info.biWidth + 3) & ~3) - bmp_header->info.biWidth;
				for (int y = 0; y < bmp_header->info.biHeight; ++y) {
					for (int x = 0; x < bmp_header->info.biWidth; ++x) {
						*(u32 *)d = pal[*s++];
						d[3] = *m++;
						d += 4;
					}
					s += align;
					m -= m_line_len * 2;
				}
			}
			bmp_header->info.biBitCount = 32;
		} else
			memcpy(act_data + bmp_header->offset, dib, bmp_header->total_size - bmp_header->offset);
		memcpy(act_data, bmp_header, 14);
		*(u16 *)act_data = 'MB';
		memcpy(act_data + 14, &bmp_header->info, bmp_header->offset - 14);
		pkg_res->actual_data = act_data;
		pkg_res->actual_data_length = bmp_header->total_size;
		free(mask);
		free(dib);
		free(bmp_header);
		free(raw);
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp");
	} else {
		pkg_res->raw_data = raw;
		pkg_res->raw_data_length = AGF_size;
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp");
		//BITMAPINFOHEADER *info = (BITMAPINFOHEADER *)((BITMAPFILEHEADER *)raw + 1);
		//alpha_blending((BYTE *)(info + 1), info->biWidth, info->biHeight, info->biBitCount);
	}

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation ARCGameEngine_AGF_operation = {
	ARCGameEngine_AGF_match,				/* match */
	NULL,									/* extract_directory */
	NULL,									/* parse_resource_info */
	ARCGameEngine_AGF_extract_resource,		/* extract_resource */
	ARCGameEngine_ALF_save_resource,		/* save_resource */
	ARCGameEngine_ALF_release_resource,		/* release_resource */
	ARCGameEngine_ALF_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK ARCGameEngine_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".ALF"), NULL, NULL, 
		&ARCGameEngine_ALF_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR 
		| CUI_EXT_FLAG_LST | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".ALF"), NULL, NULL, 
		&ARCGameEngine_ALF_idx_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR 
		| CUI_EXT_FLAG_LST | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".BIN"), _T(".exe"), NULL, 
		&ARCGameEngine_AB_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".AGF"), NULL, NULL, 
		&ARCGameEngine_AGF_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES 
		| CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	return 0;
}
