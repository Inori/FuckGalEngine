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
struct acui_information NGS_cui_information = {
	_T(""),		/* copyright */
	_T(""),			/* system */
	_T(".npf"),				/* package */
	_T(""),			/* revision */
	_T("痴汉公贼"),			/* author */
	_T(""),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

static long key0 = 0x12345;
static long key1 = 0x3575d485;

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];		// "PACK"
	u32 unknown0;		// 4
	u32 unknown1;		// 1

	s8 fat_magic[4];	// "FAT "
	u32 fat_length;		// 12

	u32 index_entries;
	u32 name_offset;
	u32 data_offset;
} npf_header_t;

typedef struct {
	u32 name_offset;
	u32 data_offset;
	u32 key;
	u32 name_length;
	u32 data_length;
} npf_entry_t;

// huffman encoding
typedef struct {
	s8 magic[4];		// "IMGX"
	u32 length;

} img_header_t;
#pragma pack ()

typedef struct {
	char name[MAX_PATH];
	DWORD name_length;
	DWORD offset;
	DWORD length;
	u32 key;
} my_npf_entry_t;

static void set_key(long seed)
{
	if (!seed)
		seed = 0x67895;
	key0 = seed;
	key1 = ((seed << 18) ^ (seed >> 12)) - 0x579E2B8D;
}

static void decrypt(BYTE *dat, DWORD dat_length)
{
	long tmp1, tmp2;
	
	for (DWORD i = 0; i < dat_length; i++) {
		tmp1 = (key1 << 18) ^ (key1 >> 12);
		tmp2 = (key0 << 14) ^ (key0 >> 10);
		key1 += tmp2 + tmp1 + 0xEA9CC9B7;
		dat[i] ^= key1;
	}
}

#define SWAP32(v)		(~((v << 16) | ((v >> 16) & 0xFFFF)))

/********************* img *********************/

/* 封包匹配回调函数 */
static int NGS_img_match(struct package *pkg)
{
	s8 magic[4];

	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "IMGX", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int NGS_img_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	my_npf_entry_t *my_npf_entry;
	u32 uncomprlen;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	if (pkg->pio->read(pkg, &uncomprlen, 4))
		return -CUI_EREAD;

	uncomprlen = SWAP32(uncomprlen);

	req_bits = 9;
	init_dict();
	while (1) {		
		bits_get(&flag, req_bits);
		if (flag != 0x100) {
			bits_put(flag, 8);
			bits_get(&flag, req_bits);
			if (flag == 0x101) {	/* 增加code位数 */
				req_bits++;
				bits_put(flag, 8);
				bits_get(&flag, ++req_bits);
			}
			if (flag == 0x102) {	/* 字典满 */	
				init_dict();
				continue;
			}
			if (flag != 0x103) {

			}
		} else {

		}
	}

	pkg_res->raw_data = malloc(pkg_res->raw_data_length);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EREADVEC;
	}

	my_npf_entry = (my_npf_entry_t *)pkg_res->actual_index_entry;
	set_key(my_npf_entry->key);
	decrypt((BYTE *)pkg_res->raw_data, pkg_res->raw_data_length);

	return 0;
}

/* 资源保存函数 */
static int NGS_img_save_resource(struct resource *res, 
								 struct package_resource *pkg_res)
{
	if (!res || !pkg_res)
		return -CUI_EPARA;
	
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (pkg_res->raw_data && pkg_res->raw_data_length) {
		if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
			res->rio->close(res);
			return -CUI_EWRITE;
		}
	}

	res->rio->close(res);
	
	return 0;
}

/* 封包资源释放函数 */
static void NGS_img_release_resource(struct package *pkg, 
									 struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

/* 封包卸载函数 */
static void NGS_img_release(struct package *pkg, 
							struct package_directory *pkg_dir)
{
	if (!pkg)
		return;

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation NGS_img_operation = {
	NGS_img_match,				/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	NGS_img_extract_resource,	/* extract_resource */
	NGS_img_save_resource,		/* save_resource */
	NGS_img_release_resource,	/* release_resource */
	NGS_img_release				/* release */
};

/********************* npf *********************/

/* 封包匹配回调函数 */
static int NGS_npf_match(struct package *pkg)
{
	s8 magic[4];

	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "PACK", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (pkg->pio->seek(pkg, 0, IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_ESEEK;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int NGS_npf_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{
	npf_header_t npf_header;
	npf_entry_t *index_buffer;
	BYTE *name_buffer;
	my_npf_entry_t *my_index_buffer;
	unsigned int index_buffer_length, name_buffer_length, i;	

	if (!pkg || !pkg_dir)
		return -CUI_EPARA;

	if (pkg->pio->read(pkg, &npf_header, sizeof(npf_header)))
		return -CUI_EREAD;

	set_key(0x46415420);	// "FAT "

	decrypt((BYTE *)npf_header.fat_magic, 8);

	if (strncmp(npf_header.fat_magic, "FAT ", 4))
		return -CUI_EMATCH;

	decrypt((BYTE *)&npf_header.index_entries, 12);

	index_buffer_length = npf_header.index_entries * sizeof(npf_entry_t);
	index_buffer = (npf_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	set_key(npf_header.index_entries);

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	decrypt((BYTE *)index_buffer, index_buffer_length);	

	name_buffer_length = npf_header.data_offset - npf_header.name_offset;
	name_buffer = (BYTE *)malloc(name_buffer_length);
	if (!name_buffer) {
		free(index_buffer);
		return -CUI_EMEM;
	}

	if (pkg->pio->readvec(pkg, name_buffer, name_buffer_length, npf_header.name_offset, IO_SEEK_SET)) {
		free(name_buffer);
		free(index_buffer);
		return -CUI_EREADVEC;
	}

	index_buffer_length = sizeof(my_npf_entry_t) * npf_header.index_entries;
	my_index_buffer = (my_npf_entry_t *)malloc(index_buffer_length);
	if (!my_index_buffer) {
		free(name_buffer);
		free(index_buffer);
		return -CUI_EMEM;
	}

	BYTE *p_name_buffer = name_buffer;
	for (i = 0; i < npf_header.index_entries; i++) {
		if (index_buffer[i].key) {
			set_key(index_buffer[i].key);
			decrypt(p_name_buffer, index_buffer[i].name_length);
		}
		strncpy(my_index_buffer[i].name, (char *)p_name_buffer, index_buffer[i].name_length);
		my_index_buffer[i].name[index_buffer[i].name_length] = 0;
		p_name_buffer += index_buffer[i].name_length;
		my_index_buffer[i].name_length = index_buffer[i].name_length;
		my_index_buffer[i].length = index_buffer[i].data_length;
		my_index_buffer[i].offset = index_buffer[i].data_offset;
		my_index_buffer[i].key = index_buffer[i].key;
	}
	free(name_buffer);
	free(index_buffer);

	pkg_dir->index_entries = npf_header.index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(my_npf_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int NGS_npf_parse_resource_info(struct package *pkg,
									   struct package_resource *pkg_res)
{
	my_npf_entry_t *my_npf_entry;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	my_npf_entry = (my_npf_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_npf_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_npf_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_npf_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int NGS_npf_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	my_npf_entry_t *my_npf_entry;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	pkg_res->raw_data = malloc(pkg_res->raw_data_length);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EREADVEC;
	}

	my_npf_entry = (my_npf_entry_t *)pkg_res->actual_index_entry;
	set_key(my_npf_entry->key);
	decrypt((BYTE *)pkg_res->raw_data, pkg_res->raw_data_length);

	return 0;
}

/* 资源保存函数 */
static int NGS_npf_save_resource(struct resource *res, 
								 struct package_resource *pkg_res)
{
	if (!res || !pkg_res)
		return -CUI_EPARA;
	
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (pkg_res->raw_data && pkg_res->raw_data_length) {
		if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
			res->rio->close(res);
			return -CUI_EWRITE;
		}
	}

	res->rio->close(res);
	
	return 0;
}

/* 封包资源释放函数 */
static void NGS_npf_release_resource(struct package *pkg, 
									 struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

/* 封包卸载函数 */
static void NGS_npf_release(struct package *pkg, 
							struct package_directory *pkg_dir)
{
	if (!pkg || !pkg_dir)
		return;

	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation NGS_npf_operation = {
	NGS_npf_match,					/* match */
	NGS_npf_extract_directory,		/* extract_directory */
	NGS_npf_parse_resource_info,	/* parse_resource_info */
	NGS_npf_extract_resource,		/* extract_resource */
	NGS_npf_save_resource,			/* save_resource */
	NGS_npf_release_resource,		/* release_resource */
	NGS_npf_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK NGS_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".npf"), NULL, 
		NULL, &NGS_npf_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, NULL, _T(".bmp"), 
		NULL, &NGS_img_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES | CUI_EXT_FLAG_NOEXT))
			return -1;

	return 0;
}
