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

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information Selene_cui_information = {
	_T(""),		/* copyright */
	_T(""),			/* system */
	_T(".Pack"),	/* package */
	_T("1.0.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-12-30 22:54"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];		// "KCAP"
	u32 index_entries;
} pack_header_t;

typedef struct {
	s8 name[64];
	u32 name_crc;
	u32 data_crc;
	u32 offset;
	u32 length;
	u32 is_encrypted;
} pack_entry_t;
#pragma pack ()

static BYTE decrypt_table[65536];

static void decrypt(BYTE *dat, DWORD len, u16 offset)
{
	for (DWORD i = 0; i < len; ++i) {
		dat[i] ^= decrypt_table[offset];
		++offset;
	}
}

/*********************
   MT realted stuff
*********************/

typedef int				int32_t;
typedef unsigned int	uint32_t;
typedef unsigned char	uint8_t;

struct MT {
	int32_t *next;
	uint32_t items;
	int32_t mt[624];
};

static int32_t MT_getnext(struct MT *MT)
{
	int32_t r;

	if (!--MT->items) {
		int32_t *mt = MT->mt;
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

static void MT_decrypt_init(char *password)
{
	int pwd_len;

	pwd_len = strlen(password);
	if (pwd_len < 8) {
		password = "Selene.Default.Password";
		pwd_len = strlen(password);
	}

	struct MT MT;
	int32_t *mt = MT.mt;

	*mt = crc32(0, (BYTE *)password, pwd_len);
	for (DWORD i = 1; i < 624; ++i)
		mt[i] = i + 0x6C078965 * ((mt[i-1] >> 30) ^ mt[i-1]);
	MT.items = 1;

	for (i = 0; i < 65536; ++i)
		decrypt_table[i] = (BYTE)(password[i % pwd_len] ^ (MT_getnext(&MT) >> 16));
}

/********************* Pack *********************/

/* 封包匹配回调函数 */
static int Selene_Pack_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "KCAP", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int Selene_Pack_extract_directory(struct package *pkg,
										 struct package_directory *pkg_dir)
{
	if (pkg->pio->read(pkg, &pkg_dir->index_entries, 4))
		return -CUI_EREAD;

	DWORD index_buffer_length = pkg_dir->index_entries * sizeof(pack_entry_t);
	pack_entry_t *index_buffer = new pack_entry_t[pkg_dir->index_entries];
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		delete [] index_buffer;
		return -CUI_EREAD;
	}

	pack_entry_t *entry = index_buffer;
	for (DWORD i = 0; i < pkg_dir->index_entries; ++i) {
		if (entry->name_crc != crc32(0, (BYTE *)entry->name, strlen(entry->name)))
			break;

		++entry;
	}
	if (i != pkg_dir->index_entries) {
		delete [] index_buffer;
		return -CUI_EMATCH;
	}

	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(pack_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int Selene_Pack_parse_resource_info(struct package *pkg,
										   struct package_resource *pkg_res)
{
	pack_entry_t *pack_entry;

	pack_entry = (pack_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, pack_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = pack_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = pack_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int Selene_Pack_extract_resource(struct package *pkg,
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

	pack_entry_t *pack_entry = (pack_entry_t *)pkg_res->actual_index_entry;
	if (pack_entry->data_crc != crc32(0, raw, pkg_res->raw_data_length)) {
		delete [] raw;
		return -CUI_EMATCH;
	}

	if (pack_entry->is_encrypted)
		decrypt(raw, pkg_res->raw_data_length, 0);

	pkg_res->raw_data = raw;

	return 0;
}

/* 资源保存函数 */
static int Selene_Pack_save_resource(struct resource *res, 
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
static void Selene_Pack_release_resource(struct package *pkg, 
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
static void Selene_Pack_release(struct package *pkg, 
								struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation Selene_Pack_operation = {
	Selene_Pack_match,					/* match */
	Selene_Pack_extract_directory,		/* extract_directory */
	Selene_Pack_parse_resource_info,		/* parse_resource_info */
	Selene_Pack_extract_resource,		/* extract_resource */
	Selene_Pack_save_resource,			/* save_resource */
	Selene_Pack_release_resource,		/* release_resource */
	Selene_Pack_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK Selene_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".Pack"), NULL, 
		NULL, &Selene_Pack_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	char password[256];

	sprintf(password, "%sdata%dpasya%s%d%sn", "mj", 999, "mada", 2, "zika");
	MT_decrypt_init(password);

	return 0;
}
