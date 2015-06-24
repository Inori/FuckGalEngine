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

//M:\Program Files\嬌僼僃儘\堹宄楆昉僄儗僲儚

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information SPack_cui_information = {
	NULL,					/* copyright */
	NULL,					/* system */
	_T(".dat"),				/* package */
	_T("0.7.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-12-7 21:53"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[6];		// "SPack"
	u16 version;		// 1
	u32 payload_size;	// 除去首部和索引段的纯数据长度
	u32 reserved0;
	u32 index_entries;
	u32 reserved1;
} dat_header_t;

typedef struct {
	s8 name[32];
	u32 offset;	
	u32 uncomprlen;
	u32 comprlen;
	u8 mode;
	u8 unknown;			// 1
	u16 hash;
	u8 pad[8];
} dat_entry_t;
#pragma pack ()

static u16 dec_table[256];

static void init_dec_table(DWORD seed)
{
	seed = (-(seed != 1) & 0xFFFFC407) + 0xC001;
	for (DWORD i = 0; i < 256; ++i) {
		u16 hash;

		if (i & 1)
			hash = (u16)seed ^ (u16)(i >> 1);
		else
			hash = (u16)i >> 1;
		if (hash & 1)
			hash = (u16)seed ^ (u16)(hash >> 1);
		else
			hash = (u16)hash >> 1;
		if (hash & 1)
			hash = (u16)seed ^ (u16)(hash >> 1);
		else
			hash = (u16)hash >> 1;
		if (hash & 1)
			hash = (u16)seed ^ (u16)(hash >> 1);
		else
			hash = (u16)hash >> 1;
		if (hash & 1)
			hash = (u16)seed ^ (u16)(hash >> 1);
		else
			hash = (u16)hash >> 1;
		if (hash & 1)
			hash = (u16)seed ^ (u16)(hash >> 1);
		else
			hash = (u16)hash >> 1;
		if (hash & 1)
			hash = (u16)seed ^ (u16)(hash >> 1);
		else
			hash = (u16)hash >> 1;
		if (hash & 1)
			hash = (u16)seed ^ (u16)(hash >> 1);
		else
			hash = (u16)hash >> 1;
		dec_table[i] = hash;
	}
}

static u16 get_hash(BYTE *buf, DWORD buf_len)
{
	u8 seed[2] = { 0xff, 0xff };
	for (DWORD i = 0; i < buf_len; ++i)
		*(u16 *)seed = seed[1] ^ (u16)dec_table[buf[i] ^ seed[0]];

	return seed[0] | seed[1] << 8;
}

static void mode2_uncompress(BYTE *uncompr, DWORD uncomprlen,
							 BYTE *compr, DWORD comprlen)
{
	DWORD curbyte = 0;
	DWORD act_uncomprlen = 0;
	DWORD flag = 0;
	DWORD mask = 0;

	while (!(act_uncomprlen >= uncomprlen || curbyte > comprlen)) {
		if (!mask) {
			flag = *(u32 *)&compr[curbyte];
			curbyte += 4;
			mask = 0x80000000;
		}
		
		if (flag & mask) {
			DWORD copy_bytes, offset;

			offset = compr[curbyte++];
			copy_bytes = offset >> 4;
			offset &= 0x0f;
			if (copy_bytes == 15) {
				copy_bytes = *(u16 *)&compr[curbyte];
				curbyte += 2;
			} else if (copy_bytes == 14)
				copy_bytes = compr[curbyte++];
			else
				++copy_bytes;

			if (offset < 10)
				offset++;
			else
				offset = ((offset - 10) << 8) | compr[curbyte++];

			for (DWORD i = 0; i < copy_bytes; ++i) {
				uncompr[act_uncomprlen] = uncompr[act_uncomprlen - offset];
				act_uncomprlen++;
			}
		} else
			uncompr[act_uncomprlen++] = compr[curbyte++];

		mask >>= 1;
	}
//	printf("%x VS %x / %x VS %x\n",
//		curbyte, comprlen, act_uncomprlen,
//		uncomprlen);
}

/********************* dat *********************/

/* 封包匹配回调函数 */
static int SPack_dat_match(struct package *pkg)
{
	dat_header_t dat_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &dat_header, sizeof(dat_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(dat_header.magic, "SPack", 6)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (dat_header.version != 1) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int SPack_dat_extract_directory(struct package *pkg,
									   struct package_directory *pkg_dir)
{
	dat_header_t dat_header;
	if (pkg->pio->readvec(pkg, &dat_header, sizeof(dat_header), 
			0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD index_buffer_length = dat_header.index_entries * sizeof(dat_entry_t);
	dat_entry_t *index_buffer = new dat_entry_t[dat_header.index_entries];
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, index_buffer, index_buffer_length, 
			sizeof(dat_header_t) + dat_header.payload_size, IO_SEEK_SET)) {
		delete [] index_buffer;
		return -CUI_EREADVEC;
	}

	for (DWORD i = 0; i < dat_header.index_entries; ++i)
		index_buffer[i].offset += sizeof(dat_header_t);

	pkg_dir->index_entries = dat_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(dat_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int SPack_dat_parse_resource_info(struct package *pkg,
										 struct package_resource *pkg_res)
{
	dat_entry_t *dat_entry;

	dat_entry = (dat_entry_t *)pkg_res->actual_index_entry;
	if (!dat_entry->name[31])
		strcpy(pkg_res->name, dat_entry->name);
	else
		strncpy(pkg_res->name, dat_entry->name, 32);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = dat_entry->comprlen;
	pkg_res->actual_data_length = dat_entry->uncomprlen;
	pkg_res->offset = dat_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int SPack_dat_extract_resource(struct package *pkg,
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

	dat_entry_t *dat_entry = (dat_entry_t *)pkg_res->actual_index_entry;
	if (dat_entry->hash != get_hash(raw, dat_entry->comprlen)) {
		delete [] raw;
		return -CUI_EMATCH;
	}

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = raw;
		return 0;
	}

	BYTE *act_data;
//	printf("%s: %x / %x, mode %x, unknown %x, pad: %x %x %x %x %x %x %x %x\n", pkg_res->name,
//		dat_entry->comprlen, dat_entry->uncomprlen, dat_entry->mode, dat_entry->unknown,
//		dat_entry->pad[0], dat_entry->pad[1], dat_entry->pad[2], dat_entry->pad[3], 
//		dat_entry->pad[4], dat_entry->pad[5], dat_entry->pad[6], dat_entry->pad[7]
//		);
	if (dat_entry->mode == 2) {
		BYTE *uncompr = new BYTE[dat_entry->uncomprlen + 1];
		if (!uncompr) {
			delete [] raw;
			return -CUI_EMEM;
		}
		mode2_uncompress(uncompr, dat_entry->uncomprlen, raw, pkg_res->raw_data_length);
		delete [] raw;
		pkg_res->actual_data = uncompr;
		act_data = uncompr;
	} else if (dat_entry->mode == 1) {
		for (DWORD i = 0; i < dat_entry->comprlen; ++i)
			raw[i] = ~raw[i];
		pkg_res->raw_data = raw;
		act_data = raw;
	} else {
		pkg_res->raw_data = raw;
		act_data = raw;
	}

	if (!strncmp((char *)act_data, "OggS", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".ogg");
	}

	return 0;
}

/* 资源保存函数 */
static int SPack_dat_save_resource(struct resource *res, 
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
static void SPack_dat_release_resource(struct package *pkg, 
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
static void SPack_dat_release(struct package *pkg, 
							  struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation SPack_dat_operation = {
	SPack_dat_match,				/* match */
	SPack_dat_extract_directory,	/* extract_directory */
	SPack_dat_parse_resource_info,	/* parse_resource_info */
	SPack_dat_extract_resource,		/* extract_resource */
	SPack_dat_save_resource,		/* save_resource */
	SPack_dat_release_resource,		/* release_resource */
	SPack_dat_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK SPack_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &SPack_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	init_dec_table(0);

	return 0;
}
