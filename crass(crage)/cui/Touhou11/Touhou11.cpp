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
struct acui_information Touhou11_cui_information = {
	_T("上海アリス幻樂団"),	/* copyright */
	NULL,					/* system */
	_T(".dat"),				/* package */
	_T("0.7.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-8-17 15:23"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];
	u32 index_length;
	u32 compr_index_length;
	u32 index_entries;
} dat_header_t;

#if 0
typedef struct {
	char *name;		// 向上4字节对齐
	u32 offset;
	u32 length;
	u32 unknown;	// 0
} dat_entry_t;
#endif

struct resource_info {
	u8 key0;
	u8 key1;
	u8 unknown[2];
	u32 block_length;
	u32 limit_length;
};

typedef struct {
	s8 magic[4];		// "TH11"
	u32 file_size;
	u32 version;		// ? 4
	u32 game_version;	// 0x100(1.00)
	u32 comprlen;
	u32 uncomprlen;
} score_dat_header_t;

typedef struct {		// 该格式在程序里完全没有任何访问
	s8 magic[4];		// "ZWAV"
	u8 unknown0;		// 1
	u32 unknown1;		// 0
	u32 index_entries;
	u8 unknown2[3];		// 0
} zwav_dat_header_t;

/* 为了让音乐循环播放且平滑过渡，
 * length长度的音乐实际上包含了一部分开头的数据，
 * 所以从smooth_offset开始的部分数据延伸到smooth_length长度。
 */
typedef struct {
	s8 name[16];
	u32 offset;
	u32 smooth_length;
	u32 smooth_offset;
	u32 length;
	u16 FormatTag; 
    u16 Channels; 
    u32 SamplesPerSec; 
    u32 AvgBytesPerSec; 
    u16 BlockAlign; 
    u16 BitsPerSample; 
    u32 cbSize;
} zwav_entry_t;
#pragma pack ()

typedef struct {
	char name[MAX_PATH];
	u32 offset;
	u32 comprlen;
	u32 uncomprlen;
	u32 unknown;
} dat_entry_t;


static struct resource_info resource_info[8] = {
	{
		0x1b, 0x37, 0xaa, 0x00, 0x00000040, 0x00002800,
	},
	{
		0x51, 0xe9, 0xbb, 0x00, 0x00000040, 0x00003000,
	},
	{
		0xc1, 0x51, 0xcc, 0x00, 0x00000080, 0x00003200,
	},
	{
		0x03, 0x19, 0xdd, 0x00, 0x00000400, 0x00007800,
	},
	{
		0xab, 0xcd, 0xee, 0x00, 0x00000200, 0x00002800,
	},
	{
		0x12, 0x34, 0xff, 0x00, 0x00000080, 0x00003200,
	},
	{
		0x35, 0x97, 0x11, 0x00, 0x00000080, 0x00002800,
	},
	{
		0x99, 0x37, 0x77, 0x00, 0x00000400, 0x00002000,
	}
};

static void *my_malloc(DWORD len)
{
	return new BYTE[len];
}

static int decode(BYTE *buf, DWORD len, DWORD blen, DWORD llen, 
				  BYTE key0, BYTE key1)
{
	DWORD rem_len = len % blen;
	if (rem_len >= blen / 4)
		rem_len = 0;

	DWORD max_len;
	if (llen <= len)
		max_len = llen;
	else
		max_len = len;

	BYTE *tmp = new BYTE[max_len];
	if (!tmp)
		return -CUI_EMEM;
	memcpy(tmp, buf, max_len);
	BYTE *src = tmp;

	rem_len = len - (rem_len + (len & 1));
	for (DWORD i = 0; i < rem_len && i < llen; ) {
		if (rem_len < blen)	// 最后一块
			blen = rem_len;

		BYTE *dst = &buf[blen - 1];
		DWORD cnt = (blen + 1) / 2;
		for (DWORD j = 0; j < cnt; ++j) {
			*dst = key0 ^ *src++;
			key0 += key1;
			dst -= 2;
		}
        cnt = blen / 2;
		dst = &buf[blen - 2];
        for (j = 0; j < cnt; ++j) {
			*dst = key0 ^ *src++;
			key0 += key1;
			dst -= 2;
        }
		buf += blen;
		rem_len -= blen;
		llen -= blen;
	}
	free(tmp);

	return 0;
}

static DWORD Touhou11_uncompress(BYTE *uncompr, DWORD uncomprlen,
								 BYTE *compr, DWORD comprlen)
{
	struct bits bits;
	bits_init(&bits, compr, comprlen);
	BYTE window[8192];
	DWORD win_pos = 1;
	DWORD act_uncomprlen = 0;
	while (1) {
		unsigned int flag;
		bit_get_high(&bits, &flag);
		if (flag) {
			unsigned int data;
			bits_get_high(&bits, 8, &data);
			uncompr[act_uncomprlen++] = data;
			window[win_pos] = data;
			win_pos = (win_pos + 1) & 0x1fff;
		} else {
			unsigned int offset = 0;
			bits_get_high(&bits, 13, &offset);
			if (!offset)
				return act_uncomprlen;
			unsigned int count;
			bits_get_high(&bits, 4, &count);
			count += 3;
			for (DWORD i = 0; i < count; ++i) {
				BYTE data = window[(i + offset) & 0x1fff];
				uncompr[act_uncomprlen++] = data;
				window[win_pos] = data;
				win_pos = (win_pos + 1) & 0x1fff;
			}
		}
	}

	return act_uncomprlen;
}

/********************* dat *********************/

/* 封包匹配回调函数 */
static int Touhou11_dat_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	dat_header_t dat_header;
	if (pkg->pio->read(pkg, &dat_header, sizeof(dat_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (decode((BYTE *)&dat_header, sizeof(dat_header), 16, 16, 0x1b, 0x37)) {
		pkg->pio->close(pkg);
		return -CUI_EMEM;
	}

	if (strncmp(dat_header.magic, "THA1", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMEM;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int Touhou11_dat_extract_directory(struct package *pkg,
										  struct package_directory *pkg_dir)
{
	dat_header_t dat_header;
	if (pkg->pio->readvec(pkg, &dat_header, sizeof(dat_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	if (decode((BYTE *)&dat_header, sizeof(dat_header), 16, 16, 0x1b, 0x37))
		return -CUI_EMEM;

	dat_header.index_length -= 0x075BCD15;
	dat_header.compr_index_length -= 0x3ADE68B1;
	dat_header.index_entries += 0xF7E7F8AC;

	u32 fsize;
	if (pkg->pio->length_of(pkg, &fsize))
		return -CUI_ELEN;

	fsize -= dat_header.compr_index_length;
	if (pkg->pio->seek(pkg, fsize, IO_SEEK_SET))
		return -CUI_ESEEK;

	BYTE *compr_index = new BYTE[dat_header.compr_index_length ];
	if (!compr_index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, compr_index, dat_header.compr_index_length)) {
		delete [] compr_index;
		return -CUI_EREAD;
	}

	if (decode(compr_index, dat_header.compr_index_length, 128, dat_header.compr_index_length, 
			0x3e, 0x9b)) {
		delete [] compr_index;
		return -CUI_EMEM;
	}

	BYTE *uncompr_index = new BYTE[dat_header.index_length];
	if (!uncompr_index) {
		delete [] compr_index;
		return -CUI_EMEM;
	}

	DWORD act_uncomprlen = Touhou11_uncompress(uncompr_index, dat_header.index_length,
		compr_index, dat_header.compr_index_length);
	delete [] compr_index;
	if (act_uncomprlen != dat_header.index_length) {
		delete [] uncompr_index;
		return -CUI_EUNCOMPR;
	}

	dat_entry_t *index_buffer = new dat_entry_t[dat_header.index_entries];
	if (!index_buffer) {
		delete [] uncompr_index;
		return -CUI_EMEM;
	}
	BYTE *p = uncompr_index;
	for (DWORD i = 0; i < dat_header.index_entries; ++i) {
		int nlen = strlen((char *)p);
		strcpy(index_buffer[i].name, (char *)p);
		nlen = (nlen + 4) & ~3;
		p += nlen;
		index_buffer[i].offset = *(u32 *)p;
		p += 4;
		index_buffer[i].uncomprlen = *(u32 *)p;
		p += 4;
		index_buffer[i].unknown = *(u32 *)p;
		p += 4;
	}
	delete [] uncompr_index;	

	for (i = 0; i < dat_header.index_entries - 1; ++i)
		index_buffer[i].comprlen = index_buffer[i + 1].offset - index_buffer[i].offset;
	index_buffer[i].comprlen = fsize - index_buffer[i].offset;

	pkg_dir->index_entries = dat_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = dat_header.index_entries * sizeof(dat_entry_t);
	pkg_dir->index_entry_length = sizeof(dat_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int Touhou11_dat_parse_resource_info(struct package *pkg,
											struct package_resource *pkg_res)
{
	dat_entry_t *dat_entry;

	dat_entry = (dat_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, dat_entry->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = dat_entry->comprlen;
	pkg_res->actual_data_length = dat_entry->uncomprlen;
	pkg_res->offset = dat_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int Touhou11_dat_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	BYTE *compr = new BYTE[pkg_res->raw_data_length];
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			delete [] compr;
			return -CUI_EREADVEC;
	}

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = compr;
		return 0;
	}

	int nlen = strlen(pkg_res->name);
	int sum = 0;
	for (int i = 0; i < nlen; ++i)
		sum += pkg_res->name[i];

	struct resource_info *rinfo = &resource_info[sum & 7];
	if (decode(compr, pkg_res->raw_data_length, rinfo->block_length, 
			rinfo->limit_length, rinfo->key0, rinfo->key1)) {
		delete [] compr;
		return -CUI_EMEM;
	}

	if (pkg_res->raw_data_length != pkg_res->actual_data_length) {
		BYTE *uncompr = new BYTE[pkg_res->actual_data_length];
		if (!uncompr) {
			delete [] compr;
			return -CUI_EMEM;			
		}
		DWORD act_uncomprlen = Touhou11_uncompress(uncompr, 
			pkg_res->actual_data_length, compr, pkg_res->raw_data_length);
		delete [] compr;
		if (act_uncomprlen != pkg_res->actual_data_length) {
			delete [] uncompr;
			return -CUI_EUNCOMPR;
		}
		pkg_res->actual_data = uncompr;
	} else
		pkg_res->raw_data = compr;

	return 0;
}

/* 资源保存函数 */
static int Touhou11_dat_save_resource(struct resource *res, 
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
static void Touhou11_dat_release_resource(struct package *pkg, 
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
static void Touhou11_dat_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation Touhou11_dat_operation = {
	Touhou11_dat_match,					/* match */
	Touhou11_dat_extract_directory,		/* extract_directory */
	Touhou11_dat_parse_resource_info,		/* parse_resource_info */
	Touhou11_dat_extract_resource,		/* extract_resource */
	Touhou11_dat_save_resource,			/* save_resource */
	Touhou11_dat_release_resource,		/* release_resource */
	Touhou11_dat_release					/* release */
};

/********************* score dat *********************/

/* 封包匹配回调函数 */
static int Touhou11_score_dat_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	score_dat_header_t dat_header;
	if (pkg->pio->read(pkg, &dat_header, sizeof(dat_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(dat_header.magic, "TH11", 4) || dat_header.version != 4) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包资源提取函数 */
static int Touhou11_score_dat_extract_resource(struct package *pkg,
											   struct package_resource *pkg_res)
{
	BYTE *raw = new BYTE[pkg_res->raw_data_length];
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length, 0, IO_SEEK_SET)) {
		delete [] raw;
		return -CUI_EREADVEC;
	}

	score_dat_header_t *dat_header = (score_dat_header_t *)raw;
	BYTE *compr = raw + sizeof(score_dat_header_t);
	DWORD comprlen = dat_header->comprlen;
	if (decode(compr, comprlen, 16, comprlen, 0xac, 0x35)) {
		delete [] raw;
		return -CUI_EMEM;
	}

	BYTE *uncompr = new BYTE[dat_header->uncomprlen];
	if (!uncompr) {
		delete [] raw;
		return -CUI_EMEM;			
	}
	DWORD act_uncomprlen = Touhou11_uncompress(uncompr, 
		dat_header->uncomprlen, compr, comprlen);
	delete [] raw;
	if (act_uncomprlen != dat_header->uncomprlen) {
		delete [] uncompr;
		return -CUI_EUNCOMPR;
	}
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = act_uncomprlen;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation Touhou11_score_dat_operation = {
	Touhou11_score_dat_match,			/* match */
	NULL,								/* extract_directory */
	NULL,								/* parse_resource_info */
	Touhou11_score_dat_extract_resource,/* extract_resource */
	Touhou11_dat_save_resource,			/* save_resource */
	Touhou11_dat_release_resource,		/* release_resource */
	Touhou11_dat_release				/* release */
};

/********************* zwav dat *********************/

/* 封包匹配回调函数 */
static int Touhou11_zwav_dat_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	zwav_dat_header_t dat_header;
	if (pkg->pio->read(pkg, &dat_header, sizeof(dat_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(dat_header.magic, "ZWAV", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int Touhou11_zwav_dat_extract_directory(struct package *pkg,
											   struct package_directory *pkg_dir)
{
	int ret = Touhou11_dat_match(pkg->lst);
	if (ret)
		return ret;

	struct package_directory lst_dir;
	memset(&lst_dir, 0, sizeof(lst_dir));
	ret = Touhou11_dat_extract_directory(pkg->lst, &lst_dir);
	if (ret) {
		Touhou11_dat_release(pkg->lst, &lst_dir);
		return ret;
	}

	for (DWORD i = 0; i < lst_dir.index_entries; ++i) {
		dat_entry_t *entry = ((dat_entry_t *)lst_dir.directory) + i;
		if (!strcmp(entry->name, "thbgm.fmt")) {
			struct package_resource zwav_fmt_res;

			memset(&zwav_fmt_res, 0, sizeof(zwav_fmt_res));
			strcpy(zwav_fmt_res.name, entry->name);
			zwav_fmt_res.name_length = -1;
			zwav_fmt_res.raw_data_length = entry->comprlen;
			zwav_fmt_res.actual_data_length = entry->uncomprlen;
			zwav_fmt_res.offset = entry->offset;
			ret = Touhou11_dat_extract_resource(pkg->lst, &zwav_fmt_res);
			if (ret)
				break;

			BYTE *zwav_fmt;
			DWORD zwav_fmt_size;
			if (zwav_fmt_res.actual_data) {
				zwav_fmt = (BYTE *)zwav_fmt_res.actual_data;
				zwav_fmt_size = zwav_fmt_res.actual_data_length;
			} else {
				zwav_fmt = (BYTE *)zwav_fmt_res.raw_data;
				zwav_fmt_size = zwav_fmt_res.raw_data_length;
			}
			zwav_entry_t *zwav_fmt_list = (zwav_entry_t *)zwav_fmt;
			for (DWORD j = 0; ; ++j) {
				if (!zwav_fmt_list[j].name[0])
					break;
			}
			Touhou11_dat_release(pkg->lst, &lst_dir);
			pkg_dir->index_entries = j;
			pkg_dir->directory = zwav_fmt;
			pkg_dir->directory_length = j * sizeof(zwav_entry_t);
			pkg_dir->index_entry_length = sizeof(zwav_entry_t);	
			return 0;
		}
	}
	Touhou11_dat_release(pkg->lst, &lst_dir);
	if (i == lst_dir.index_entries)
		return -CUI_EMATCH;
	return ret;
}

/* 封包索引项解析函数 */
static int Touhou11_zwav_dat_parse_resource_info(struct package *pkg,
												 struct package_resource *pkg_res)
{
	zwav_entry_t *zwav_entry;

	zwav_entry = (zwav_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, zwav_entry->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = zwav_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = zwav_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int Touhou11_zwav_dat_extract_resource(struct package *pkg,
											  struct package_resource *pkg_res)
{
	BYTE *raw = new BYTE[pkg_res->raw_data_length];
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET)) {
		delete [] raw;
		return -CUI_EREADVEC;
	}

	zwav_entry_t *zwav_entry = (zwav_entry_t *)pkg_res->actual_index_entry;
	if (MySaveAsWAVE(raw, pkg_res->raw_data_length, 
			zwav_entry->FormatTag, zwav_entry->Channels, zwav_entry->SamplesPerSec, 
			zwav_entry->BitsPerSample, NULL, 0, (BYTE **)&pkg_res->actual_data,
			&pkg_res->actual_data_length, my_malloc)) {
		delete [] raw;
		return -CUI_EMEM;		
	}

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation Touhou11_zwav_dat_operation = {
	Touhou11_zwav_dat_match,				/* match */
	Touhou11_zwav_dat_extract_directory,	/* extract_directory */
	Touhou11_zwav_dat_parse_resource_info,	/* parse_resource_info */
	Touhou11_zwav_dat_extract_resource,		/* extract_resource */
	Touhou11_dat_save_resource,				/* save_resource */
	Touhou11_dat_release_resource,			/* release_resource */
	Touhou11_dat_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK Touhou11_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &Touhou11_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &Touhou11_score_dat_operation, CUI_EXT_FLAG_PKG))
			return -1;

	// 具体的音乐分块信息在th11.dat里的thbgm.fmt中
	if (callback->add_extension(callback->cui, _T(".dat"), _T(".wav"), 
		NULL, &Touhou11_zwav_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_LST))
			return -1;

	return 0;
}
