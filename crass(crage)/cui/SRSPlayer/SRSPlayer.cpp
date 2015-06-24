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

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information SRSPlayer_cui_information = {
	_T("Sagiri no Miyako"),	/* copyright */
	_T("Sagiri Resource Set Gameplayer"),	/* system */
	_T(".pak"),				/* package */
	_T("1.0.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-1-18 17:42"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	u32 index_length;
} pak_header_t;

typedef struct {
	u32 crc;
	s8 *name;
	u32 comprlen;
	u32 uncomprlen;
	u32 offset;
} pak_entry_t;
#pragma pack ()

typedef struct {
	char name[MAX_PATH];
	DWORD offset;
	DWORD comprlen;
	DWORD uncomprlen;
	DWORD crc;
} my_pak_entry_t;

static DWORD crc_table[256];

static void make_crc_table(void)
{
	DWORD crc = 0;

	for (DWORD j = 0; j < 256; ++j) {
		crc = j << 24;
		for (DWORD k = 0; k < 8; ++k) {			
			if (crc & 1UL << 31) {
				crc <<= 1;
				crc ^= 0x4C11DB7;
			} else
				crc <<= 1;
        }
		crc_table[j] = crc;
	}
}

static DWORD __crc32(BYTE *dat, DWORD len)
{
	DWORD crc = -1;

	for (DWORD i = 0; i < len; ++i)
		crc = crc_table[dat[i] ^ (crc >> 24)] ^ (crc << 8);

	return ~crc;
}

/*********************
   MT realted stuff
*********************/

typedef unsigned char		uint8_t;
typedef unsigned int		uint32_t;

struct MT {
	uint32_t *next;
	uint32_t items;
	uint32_t mt[624];
};

static uint32_t MT_getnext(struct MT *MT)
{
	uint32_t r;

	if (!--MT->items) {
		uint32_t *mt = MT->mt;
		unsigned int i;

		MT->items = 624;
		MT->next = mt;

		for (i=0; i<227; i++)
			mt[i] = ((((mt[i] ^ mt[i+1])&0x7ffffffe)^mt[i])>>1)^((0-(mt[i+1]&1))&0x9908b0df)^mt[i+397];
		for (; i<623; i++)
			mt[i] = ((((mt[i] ^ mt[i+1])&0x7ffffffe)^mt[i])>>1)^((0-(mt[i+1]&1))&0x9908b0df)^mt[i-227];
		mt[623] = ((((mt[623] ^ mt[0])&0x7ffffffe)^mt[623])>>1)^((0-(mt[0]&1))&0x9908b0df)^mt[i-227];
	}

	r = *(MT->next++);
	r ^= (r >> 11);
	r ^= ((r & 0xff3a58ad) << 7);
	r ^= ((r & 0xffffdf8c) << 15);
	r ^= (r >> 18);
	return r;
}

static void MT_decrypt(uint8_t *buf, unsigned int size, uint32_t seed)
{
	struct MT MT;
	unsigned int i;
	uint32_t *mt = MT.mt;

	*mt=seed;
	for(i=1; i<624; i++)
		mt[i] = i+0x6c078965*((mt[i-1]>>30)^mt[i-1]);
	MT.items = 1;

	while(size--) {
		*buf++ ^= MT_getnext(&MT);
	}
}

static void MT_decrypt32(uint32_t *buf, unsigned int size, uint32_t seed)
{
	struct MT MT;
	unsigned int i;
	uint32_t *mt = MT.mt;

	*mt=seed;
	for(i=1; i<624; i++)
		mt[i] = i+0x6c078965*((mt[i-1]>>30)^mt[i-1]);
	MT.items = 1;

	while(size--)
		*buf++ ^= MT_getnext(&MT);
}

/********************* pak *********************/

static int SRSPlayer_pak_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir);

/* 封包匹配回调函数 */
static int SRSPlayer_pak_match(struct package *pkg)
{
	u32 index_length;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &index_length, 4)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (index_length <= 4) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	struct package_directory pkg_dir;
	int ret = SRSPlayer_pak_extract_directory(pkg, &pkg_dir);
	if (ret) {
		pkg->pio->close(pkg);
		return ret;
	}
	delete [] pkg_dir.directory;

	return 0;	
}

/* 封包索引目录提取函数 */
static int SRSPlayer_pak_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{
	u32 index_length;

	if (pkg->pio->readvec(pkg, &index_length, 4, 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	index_length -= 4;
	BYTE *index = new BYTE[index_length];
	if (!index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index, index_length)) {
		delete [] index;
		return -CUI_EREAD;
	}

	MT_decrypt(index, index_length, index_length + 4);

	make_crc_table();

	if (__crc32(index + 4, index_length - 4) != *(u32 *)index) {
		delete [] index;
		return -CUI_EMATCH;
	}

	DWORD offset = 4;
	for (DWORD i = 0; offset < index_length; ++i) {
		offset += strlen((char *)index + offset) + 1 + 8;
		*(u32 *)&index[offset] += index_length + 4;
		offset += 4 + 1 + 4;		
	}

	pkg_dir->index_entries = i;
	pkg_dir->directory = index;
	pkg_dir->directory_length = index_length;
	pkg_dir->flags = PKG_DIR_FLAG_VARLEN;

	return 0;
}

/* 封包索引项解析函数 */
static int SRSPlayer_pak_parse_resource_info(struct package *pkg,
											 struct package_resource *pkg_res)
{
	BYTE *pak_entry;

	pak_entry = (BYTE *)pkg_res->actual_index_entry + 4;
	pkg_res->name_length = strlen((char *)pak_entry);
	strcpy(pkg_res->name, (char *)pak_entry);
	pak_entry += pkg_res->name_length + 1;
	pkg_res->raw_data_length = *(u32 *)pak_entry;
	pak_entry += 4;
	pkg_res->actual_data_length = *(u32 *)pak_entry;
	pak_entry += 4;
	pkg_res->offset = *(u32 *)pak_entry;
	pkg_res->actual_index_entry_length = pkg_res->name_length + 1 + 12 + 1 + 4;

	return 0;
}

/* 封包资源提取函数 */
static int SRSPlayer_pak_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	DWORD act_raw_len = pkg_res->raw_data_length;
	DWORD raw_len = (act_raw_len + 3) & ~3;
	BYTE *raw = new BYTE[raw_len];
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, act_raw_len, pkg_res->offset, IO_SEEK_SET)) {
		delete [] raw;
		return -CUI_EREADVEC;
	}

	BYTE *pak_entry = (BYTE *)pkg_res->actual_index_entry + 4;
	u32 seed = *(u32 *)&pak_entry[pkg_res->name_length + 1 + 12 + 1];
	u32 dec_table[16];
	memset(dec_table, 0, sizeof(dec_table));
	MT_decrypt32(dec_table, 16, seed);
	for (DWORD i = 0; i < raw_len / 4; ++i)
		((u32 *)raw)[i] ^= dec_table[i & 0xf];

	if (!pak_entry[pkg_res->name_length + 1 + 12]) {
		BYTE *uncompr = new BYTE[pkg_res->actual_data_length];
		if (!uncompr) {
			delete [] raw;
			return -CUI_EMEM;
		}

		if (uncompress(uncompr, &pkg_res->actual_data_length, raw, act_raw_len) != Z_OK) {
			delete [] uncompr;
			delete [] raw;
			return -CUI_EUNCOMPR;
		}	
		delete [] raw;
		raw = NULL;
		pkg_res->actual_data = uncompr;
	}	
	pkg_res->raw_data = raw;

	if (strstr(pkg_res->name, ".avd")) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".mpg");
	} else if (strstr(pkg_res->name, ".b32")) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".bmp");
	}

	return 0;
}

/* 资源保存函数 */
static int SRSPlayer_pak_save_resource(struct resource *res, 
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
static void SRSPlayer_pak_release_resource(struct package *pkg, 
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
static void SRSPlayer_pak_release(struct package *pkg, 
								  struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation SRSPlayer_pak_operation = {
	SRSPlayer_pak_match,				/* match */
	SRSPlayer_pak_extract_directory,	/* extract_directory */
	SRSPlayer_pak_parse_resource_info,	/* parse_resource_info */
	SRSPlayer_pak_extract_resource,		/* extract_resource */
	SRSPlayer_pak_save_resource,		/* save_resource */
	SRSPlayer_pak_release_resource,		/* release_resource */
	SRSPlayer_pak_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK SRSPlayer_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".pak"), NULL, 
		NULL, &SRSPlayer_pak_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
