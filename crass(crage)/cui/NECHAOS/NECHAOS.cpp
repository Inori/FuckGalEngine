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
struct acui_information NECHAOS_cui_information = {
	_T(""),		/* copyright */
	_T(""),			/* system */
	_T(".p"),		/* package */
	_T("1.0.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T(""),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[16];		// "PKFileInfo"
	u32 mode;
	u32 index_entries;
} p_header_t;

typedef struct {
	s8 name[64];		// NULL terminated
	u32 offset;
	u32 length;
} p_entry_t;
#pragma pack ()


/********************* p *********************/

/* 封包匹配回调函数 */
static int NECHAOS_p_match(struct package *pkg)
{
	s8 magic[16];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "PKFileInfo", sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int NECHAOS_p_extract_directory(struct package *pkg,
									   struct package_directory *pkg_dir)
{
	p_header_t p_header;
	p_entry_t *index_buffer;
	unsigned int index_buffer_length;	
	unsigned int i;

	if (pkg->pio->readvec(pkg, &p_header, sizeof(p_header), 
			0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	p_header.index_entries ^= 0xE3DF59AC;
	index_buffer_length = p_header.index_entries * sizeof(p_entry_t);
	index_buffer = (p_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	p_entry_t *entry = index_buffer;
	for (i = 0; i < p_header.index_entries; i++) {
		for (DWORD n = 0; n < 63; n++)
			entry->name[n] ^= (i * n + 0x1a) * 3;
		entry->length ^= 0xE3DF59AC;
		entry++;
	}

	pkg_dir->index_entries= p_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(p_entry_t);
	package_set_private(pkg, p_header.mode);

	return 0;
}

/* 封包索引项解析函数 */
static int NECHAOS_p_parse_resource_info(struct package *pkg,
										 struct package_resource *pkg_res)
{
	p_entry_t *p_entry;

	p_entry = (p_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, p_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = p_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = p_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int NECHAOS_p_extract_resource(struct package *pkg,
									  struct package_resource *pkg_res)
{
	BYTE *raw = (BYTE *)pkg->pio->readvec_only(pkg, 
		pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET);
	if (!raw)
		return -CUI_EREADVECONLY;

	int mode = (int)package_get_private(pkg);

	if (mode == 1 || (pkg_res->flags & PKG_RES_FLAG_RAW)) {
		pkg_res->raw_data = raw;
		return 0;
	}

	DWORD enc_len = pkg_res->raw_data_length < 1977 ? 
		pkg_res->raw_data_length : 1977;
	
	if (mode == 2) {
		BYTE *dec = (BYTE *)malloc(pkg_res->raw_data_length);
		if (!dec)
			return -CUI_EMEM;
		memcpy(dec, raw, pkg_res->raw_data_length);

		for (DWORD i = 0; i < enc_len; i++)
			dec[i] ^= 0xca; 
		pkg_res->actual_data = dec;
		pkg_res->actual_data_length = pkg_res->raw_data_length;
	} else {
		BYTE *dec = (BYTE *)malloc(pkg_res->raw_data_length);
		if (!dec)
			return -CUI_EMEM;
		memcpy(dec, raw, pkg_res->raw_data_length);

		int name_len = strlen(pkg_res->name);
		for (DWORD i = 0; i < enc_len; i += name_len) {
			for (int n = 0; n < name_len; n++) {
				if (i + n < enc_len)
					dec[i + n] ^= pkg_res->name[n] + n + i + 3;
			}
		}
		pkg_res->actual_data = dec;
		pkg_res->actual_data_length = pkg_res->raw_data_length;
	}

	pkg_res->raw_data = raw;

	return 0;
}

/* 资源保存函数 */
static int NECHAOS_p_save_resource(struct resource *res, 
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
static void NECHAOS_p_release_resource(struct package *pkg, 
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
static void NECHAOS_p_release(struct package *pkg, 
							  struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation NECHAOS_p_operation = {
	NECHAOS_p_match,				/* match */
	NECHAOS_p_extract_directory,	/* extract_directory */
	NECHAOS_p_parse_resource_info,	/* parse_resource_info */
	NECHAOS_p_extract_resource,		/* extract_resource */
	NECHAOS_p_save_resource,		/* save_resource */
	NECHAOS_p_release_resource,		/* release_resource */
	NECHAOS_p_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK NECHAOS_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".p"), NULL, 
		NULL, &NECHAOS_p_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
