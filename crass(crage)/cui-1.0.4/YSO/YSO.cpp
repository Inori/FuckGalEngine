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
#include <zlib.h>

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information YSO_cui_information = {
	_T("日本ファルコム株式会社"),/* copyright */
	_T("Ys Origin"),		/* system */
	_T(".na .ni"),			/* package */
	_T("0.9.5"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-3-4 16:55"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];		// "NNI"
	u32 index_entries;
	u32 name_index_length;
	u32 flag;
} ni_header_t;

/* 按照name_hash从小到大顺序排列.
 * 资源查找时，首先将要查找的资源的名字转换为hash,
 * 然后用二分法在ni_entry_t的数组里(.ni文件内存储)
 * 根据hash找到相应的项（对于hash值相同的连续项，最终
 * 二分法查找结束时定位的肯定是连续项中的第一项)。
 */
typedef struct {
	u32 name_hash;
	u32 length;
	u32 offset;			/* 最高位可能表示升级封包中有该资源的更新版本 */
	u32 name_offset;
} ni_entry_t;

typedef struct {
	u32 crc;
	u32 uncomprlen;
} z_header_t;

/* 升级封包(.na+.ni)的升级列表文件 */
typedef struct {
	u32 name_hash;
	u32 unknown[4];
} nu_entry_t;
#pragma pack ()

typedef struct {
	s8 name[MAX_PATH];
	//u32 name_hash;
	u32 offset;
	u32 length;	
	u32 crc;
} my_ni_entry_t;

static void decode(int key, BYTE *data, DWORD data_len)
{
	for (DWORD i = 0; i < data_len; i++) {
		key *= 0x3d09;
		data[i] -= (BYTE)((unsigned int)key >> 16);
	}
}

static DWORD get_hash(char *name)
{
	int i = 0;
	unsigned int hash = 0;

	for (int chr = *name++; chr; chr = *name++) {
		chr -= 32;
		chr &= 0xff;
		chr <<= i;
		i += 5;
		if (i >= 25)
			i = 0;
		hash += chr;
	}

	return hash % 0xfff1;
}

/********************* na *********************/

/* 封包匹配回调函数 */
static int YSO_na_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->open(pkg->lst, IO_READONLY)) {
		pkg->pio->close(pkg);
		return -CUI_EOPEN;
	}

	if (pkg->pio->read(pkg->lst, magic, sizeof(magic))) {
		pkg->pio->close(pkg->lst);
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "NNI", 4)) {
		pkg->pio->close(pkg->lst);
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int YSO_na_extract_directory(struct package *pkg,
									struct package_directory *pkg_dir)
{
	ni_header_t ni_header;
	ni_entry_t *ni_index;
	DWORD ni_index_length;	

	if (pkg->pio->readvec(pkg->lst, &ni_header, sizeof(ni_header_t), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	if (ni_header.flag & 1) {
		printf("ni header flag is 1\n");
		return -CUI_EMATCH;
	}

	ni_index_length = ni_header.index_entries * sizeof(ni_entry_t);
	ni_index = (ni_entry_t *)malloc(ni_index_length);
	if (!ni_index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg->lst, ni_index, ni_index_length)) {
		free(ni_index);
		return -CUI_EREAD;
	}

	decode(0x7C53F961, (BYTE *)ni_index, ni_index_length);

	char *name_index_buffer = (char *)malloc(ni_header.name_index_length);
	if (!name_index_buffer) {
		free(ni_index);
		return -CUI_EMEM;
	}

	if (pkg->pio->read(pkg->lst, name_index_buffer, ni_header.name_index_length)) {
		free(name_index_buffer);
		free(ni_index);
		return -CUI_EREAD;
	}

	decode(0x7C53F961, (BYTE *)name_index_buffer, 
		ni_header.name_index_length);

	DWORD my_index_buffer_length = ni_header.index_entries * sizeof(my_ni_entry_t);
	my_ni_entry_t *my_index_buffer = (my_ni_entry_t *)malloc(my_index_buffer_length);
	if (!my_index_buffer) {
		free(name_index_buffer);
		free(ni_index);
		return -CUI_EMEM;
	}

	memset(my_index_buffer, 0, my_index_buffer_length);
	for (DWORD i = 0; i < ni_header.index_entries; i++) {
		strcpy(my_index_buffer[i].name, 
			&name_index_buffer[ni_index[i].name_offset]);
	//	my_index_buffer[i].name_hash = ni_index[i].name_hash;
		my_index_buffer[i].length = ni_index[i].length;
		my_index_buffer[i].offset = ni_index[i].offset;
	}
	//MySaveFile(_T("ni_index_1100"), ni_index, ni_index_length);

	free(name_index_buffer);
	free(ni_index);

	pkg_dir->index_entries = ni_header.index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = my_index_buffer_length;
	pkg_dir->index_entry_length = sizeof(my_ni_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

static int YSO_na_parse_resource_info(struct package *pkg,
									  struct package_resource *pkg_res)
{
	my_ni_entry_t *my_ni_entry;

	my_ni_entry = (my_ni_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_ni_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_ni_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_ni_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int YSO_na_extract_resource(struct package *pkg,
								   struct package_resource *pkg_res)
{
	z_header_t *z_header;
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;

	pkg_res->raw_data = pkg->pio->readvec_only(pkg, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET);
	if (!pkg_res->raw_data)
		return -CUI_EREADVECONLY;	

	char *pos = strstr(pkg_res->name, ".Z");
	if (pos) {
		z_header = (z_header_t *)(pkg_res->raw_data);

		comprlen = pkg_res->raw_data_length - sizeof(z_header_t);
		compr = (BYTE *)(z_header + 1);
		uncomprlen = z_header->uncomprlen;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr)
			return -CUI_EMEM;

		DWORD act_uncomprlen = uncomprlen;
		if (uncompress(uncompr, &act_uncomprlen, compr, comprlen) != Z_OK 
			|| act_uncomprlen != uncomprlen) {
			free(uncompr);
			return -CUI_EUNCOMPR;
		}

		if (crc32(0, uncompr, uncomprlen) != z_header->crc) {
			free(uncompr);
			return -CUI_EMATCH;
		}

		*pos = 0;
	} else {
		uncompr = NULL;
		uncomprlen = 0;
	}

	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

/* 资源保存函数 */
static int YSO_na_save_resource(struct resource *res, 
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
static void YSO_na_release_resource(struct package *pkg, 
									struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res->raw_data)
		pkg_res->raw_data = NULL;
}

/* 封包卸载函数 */
static void YSO_na_release(struct package *pkg, 
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
static cui_ext_operation YSO_na_operation = {
	YSO_na_match,					/* match */
	YSO_na_extract_directory,		/* extract_directory */
	YSO_na_parse_resource_info,		/* parse_resource_info */
	YSO_na_extract_resource,		/* extract_resource */
	YSO_na_save_resource,			/* save_resource */
	YSO_na_release_resource,		/* release_resource */
	YSO_na_release					/* release */
};

/********************* nya *********************/

/* 封包匹配回调函数 */
static int YSO_nya_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包资源提取函数 */
static int YSO_nya_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	SIZE_T nya_size;
	BYTE *raw, *act;

	if (pkg->pio->length_of(pkg, &nya_size))
		return -CUI_ELEN;

	raw = (BYTE *)pkg->pio->readvec_only(pkg, nya_size,	0, IO_SEEK_SET);
	if (!raw)
		return -CUI_EREADVECONLY;

	act = (BYTE *)malloc(nya_size);
	if (!act)
		return -CUI_EMEM;

	memcpy(act, raw, nya_size);

	decode(0x7C53F961, act, nya_size);

	pkg_res->raw_data = raw;
	pkg_res->raw_data_length = nya_size;
	pkg_res->actual_data = act;
	pkg_res->actual_data_length = nya_size;

	return 0;
}

static void YSO_nya_release(struct package *pkg, 
							struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation YSO_nya_operation = {
	YSO_nya_match,				/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	YSO_nya_extract_resource,	/* extract_resource */
	YSO_na_save_resource,		/* save_resource */
	YSO_na_release_resource,	/* release_resource */
	YSO_nya_release				/* release */
};


/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK YSO_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".na"), NULL, 
		NULL, &YSO_na_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_LST))
			return -1;

	if (callback->add_extension(callback->cui, _T(".nya"), NULL, 
		NULL, &YSO_nya_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	return 0;
}
