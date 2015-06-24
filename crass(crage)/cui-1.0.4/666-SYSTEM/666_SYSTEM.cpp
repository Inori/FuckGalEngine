#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information _666_SYSTEM_cui_information = {
	_T("HEXA"),				/* copyright */
	_T("666-SYSTEM"),		/* system */
	_T(".dat .ml .gl .sl"),	/* package */
	_T("1.0.2"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-7-12 16:15"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];
	u16 version;	// 100
	u32 data_offset;
	u32 entries;
	u32 data_length;
} msg_header_t;

typedef struct {
	s8 magic[4];	// "HSL "
	u16 version;	// 100
	u32 name_offset;// name_offset
	u32 offset_table_offset;	// 每项4字节
	u32 data_offset;
	u16 start_name_length;
	s8 *start;
	u32 entries;
	u16 name_length;
	s8 *name;
} snr_header_t;

typedef struct {
	s8 magic[4];	// "GLNK"
	u16 version;	// 101, 110
	u32 index_entries;
	u32 index_offset;
	u32 index_length;
} glnk_header_t;

typedef struct {
	s8 magic[4];	// "SKMD"
	u16 version;	// 100
	u32 entries;	// 多一项，以0结尾
	u32 data_length;
} skmd_header_t;
#pragma pack ()

typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 length;
} my_msg_entry_t;

typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 length;
} my_glnk_entry_t;

/********************* msg.dat *********************/

/* 封包匹配回调函数 */
static int SYSTEM666_msg_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "HMO ", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int SYSTEM666_msg_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	msg_header_t *msg_header;
	DWORD *offset_table;
	DWORD i;
	BYTE *dat;
	u32 fsize;

	if (pkg->pio->length_of(pkg, &fsize))
		return -CUI_ELEN;

	dat = (BYTE *)malloc(fsize);
	if (!dat)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, dat, fsize, 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	msg_header = (msg_header_t *)dat;
	offset_table = (DWORD *)(msg_header + 1);

	BYTE *raw_data = dat + msg_header->data_offset;
	for (i = 0; i < msg_header->entries; i++) {
		DWORD len;
		BYTE *entry_data;

		entry_data = raw_data + offset_table[i];
		len = *(u32 *)entry_data;
		entry_data += 4;
		for (DWORD k = 0; k < len; k++)
			entry_data[k] = ~entry_data[k];
	}
	pkg_res->raw_data = dat;
	pkg_res->raw_data_length = fsize;

	return 0;
}

/* 资源保存函数 */
static int SYSTEM666_msg_save_resource(struct resource *res, 
									   struct package_resource *pkg_res)
{
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
static void SYSTEM666_msg_release_resource(struct package *pkg, 
										   struct package_resource *pkg_res)
{
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

/* 封包卸载函数 */
static void SYSTEM666_msg_release(struct package *pkg, 
								  struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation SYSTEM666_msg_operation = {
	SYSTEM666_msg_match,			/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	SYSTEM666_msg_extract_resource,	/* extract_resource */
	SYSTEM666_msg_save_resource,	/* save_resource */
	SYSTEM666_msg_release_resource,	/* release_resource */
	SYSTEM666_msg_release			/* release */
};

/********************* msg.dat *********************/

/* 封包匹配回调函数 */
static int SYSTEM666_skmd_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "SKMD", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int SYSTEM666_skmd_extract_resource(struct package *pkg,
										   struct package_resource *pkg_res)
{
	u32 skmd_size;
	if (pkg->pio->length_of(pkg, &skmd_size))
		return -CUI_ELEN;

	BYTE *skmd = (BYTE *)malloc(skmd_size);
	if (!skmd)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, skmd, skmd_size, 0, IO_SEEK_SET)) {
		free(skmd);
		return -CUI_EREADVEC;
	}

	skmd_header_t *skmd_head = (skmd_header_t *)skmd;
	BYTE *data = skmd + sizeof(skmd_header_t) + (skmd_head->entries + 1) * 4;
	for (DWORD i = 0; i < skmd_head->data_length; ++i)
		data[i] = ~data[i];
	pkg_res->raw_data = skmd;
	pkg_res->raw_data_length = skmd_size;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation SYSTEM666_skmd_operation = {
	SYSTEM666_skmd_match,			/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	SYSTEM666_skmd_extract_resource,/* extract_resource */
	SYSTEM666_msg_save_resource,	/* save_resource */
	SYSTEM666_msg_release_resource,	/* release_resource */
	SYSTEM666_msg_release			/* release */
};

/********************* .dat *********************/

/* 封包匹配回调函数 */
static int SYSTEM666_glnk_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "GLNK", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int SYSTEM666_glnk_extract_directory(struct package *pkg,
											struct package_directory *pkg_dir)
{
	glnk_header_t glnk_header;
	DWORD index_buffer_length;	
	DWORD i;
	my_glnk_entry_t *index_buffer;
	BYTE *index;

	if (pkg->pio->readvec(pkg, &glnk_header, sizeof(glnk_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	index = (BYTE *)malloc(glnk_header.index_length);
	if (!index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index, glnk_header.index_length)) {
		free(index);
		return -CUI_EREAD;
	}

	index_buffer_length = glnk_header.index_entries * sizeof(my_glnk_entry_t);
	index_buffer = (my_glnk_entry_t *)malloc(index_buffer_length);
	if (!index_buffer) {
		free(index);
		return -CUI_EMEM;
	}

	BYTE *p = index;
	for (i = 0; i < glnk_header.index_entries; i++) {
		BYTE name_len;
		
		name_len = *p++;
		strncpy(index_buffer[i].name, (char *)p, name_len);
		index_buffer[i].name[name_len] = 0;
		p += name_len;
		index_buffer[i].offset = *(DWORD *)p;
		p += 4;
		index_buffer[i].length = *(DWORD *)p;
		p += 4;
		if (glnk_header.version == 110)
			p += 4;	/* 忽略length_hi */
	}
	free(index);

	pkg_dir->index_entries = glnk_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(my_glnk_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int SYSTEM666_glnk_parse_resource_info(struct package *pkg,
											  struct package_resource *pkg_res)
{
	my_glnk_entry_t *my_glnk_entry;

	my_glnk_entry = (my_glnk_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_glnk_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_glnk_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_glnk_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int SYSTEM666_glnk_extract_resource(struct package *pkg,
										   struct package_resource *pkg_res)
{
	pkg_res->raw_data = malloc(pkg_res->raw_data_length);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EREADVEC;
	}

	return 0;
}

/* 资源保存函数 */
static int SYSTEM666_glnk_save_resource(struct resource *res, 
										struct package_resource *pkg_res)
{
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
static void SYSTEM666_glnk_release_resource(struct package *pkg, 
											struct package_resource *pkg_res)
{
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

/* 封包卸载函数 */
static void SYSTEM666_glnk_release(struct package *pkg, 
								   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation SYSTEM666_glnk_operation = {
	SYSTEM666_glnk_match,				/* match */
	SYSTEM666_glnk_extract_directory,	/* extract_directory */
	SYSTEM666_glnk_parse_resource_info,	/* parse_resource_info */
	SYSTEM666_glnk_extract_resource,	/* extract_resource */
	SYSTEM666_glnk_save_resource,		/* save_resource */
	SYSTEM666_glnk_release_resource,	/* release_resource */
	SYSTEM666_glnk_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK _666_SYSTEM_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &SYSTEM666_glnk_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".ml"), NULL, 
		NULL, &SYSTEM666_glnk_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".gl"), NULL, 
		NULL, &SYSTEM666_glnk_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".sl"), NULL, 
		NULL, &SYSTEM666_glnk_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), _T(".dat_"), 
		NULL, &SYSTEM666_msg_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), _T(".dat_"), 
		NULL, &SYSTEM666_skmd_operation, CUI_EXT_FLAG_PKG))
			return -1;

	return 0;
}
