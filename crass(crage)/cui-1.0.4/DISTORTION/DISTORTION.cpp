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
struct acui_information DISTORTION_cui_information = {
	_T("DISTORTION"),		/* copyright */
	NULL,					/* system */
	_T(".pac"),				/* package */
	_T("1.0.1"),			/* revision */
	_T("痴汉公贼"),			/* author */
	_T("2009-2-26 21:25"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];		// "PACK"
	u32 index_entries;
	u32 is_encrypt;
	u8 decrypt_buffer[32];
} pac_header_t;

typedef struct {
	s8 magic[4];		// "TEYL"
	u32 width;
	u32 height;
} bmp_header_t;
#pragma pack ()

typedef struct {
	char name[MAX_PATH];
	DWORD length;
	DWORD offset;		
} pac_entry_t;


static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static int DISTORTION_pac_extract_directory(struct package *pkg,
											struct package_directory *pkg_dir);

/********************* pac *********************/

/* 封包匹配回调函数 */
static int DISTORTION_pac_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "PACK", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	struct package_directory pkg_dir;
	int ret = DISTORTION_pac_extract_directory(pkg, &pkg_dir);
	if (!ret) {
		free((void *)package_get_private(pkg));
		free(pkg_dir.directory);
	} else
		ret = -CUI_EMATCH;

	return ret;	
}

/* 封包索引目录提取函数 */
static int DISTORTION_pac_extract_directory(struct package *pkg,
											struct package_directory *pkg_dir)
{
	pac_header_t *pac_header;
	pac_entry_t *index_buffer;
	DWORD index_buffer_length;
	u32 offset;
	DWORD i;

	pac_header = (pac_header_t *)malloc(sizeof(pac_header_t));
	if (!pac_header)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pac_header, sizeof(pac_header_t), 0, IO_SEEK_SET)) {
		free(pac_header);
		return -CUI_EREADVEC;
	}

	for (i = 0; i < sizeof(pac_header->decrypt_buffer); i++)
		pac_header->decrypt_buffer[i] ^= 0xab;

	index_buffer_length = pac_header->index_entries * sizeof(pac_entry_t);
	index_buffer = (pac_entry_t *)malloc(index_buffer_length);
	if (!index_buffer) {
		free(pac_header);
		return -CUI_EMEM;
	}

	for (i = 0; i < pac_header->index_entries; i++) {
		u32 name_len;

		if (pac_header->is_encrypt) {
			DWORD k;

			/* 用当前的偏移做解密因子 */
			if (pkg->pio->locate(pkg, &offset)) {
				free(pac_header);
				return -CUI_ELOC;
			}

			if (pkg->pio->read(pkg, &name_len, 4)) {
				free(pac_header);
				return -CUI_EREAD;
			}

			for (k = 0; k < 4; k++) {
				offset &= sizeof(pac_header->decrypt_buffer) - 1;
				((BYTE *)&name_len)[k] ^= pac_header->decrypt_buffer[offset++];
			}

			if (pkg->pio->locate(pkg, &offset)) {
				free(pac_header);
				return -CUI_ELOC;
			}

			if (pkg->pio->read(pkg, index_buffer[i].name, name_len)) {
				free(pac_header);
				return -CUI_EREAD;
			}

			for (k = 0; k < name_len; k++) {
				offset &= sizeof(pac_header->decrypt_buffer) - 1;
				index_buffer[i].name[k] ^= pac_header->decrypt_buffer[offset++];
			}

			if (pkg->pio->locate(pkg, &offset)) {
				free(pac_header);
				return -CUI_ELOC;
			}

			if (pkg->pio->read(pkg, &index_buffer[i].length, 4)) {
				free(pac_header);
				return -CUI_EREAD;
			}

			for (k = 0; k < 4; k++) {
				offset &= sizeof(pac_header->decrypt_buffer) - 1;
				((BYTE *)&index_buffer[i].length)[k] ^= pac_header->decrypt_buffer[offset++];
			}

			if (pkg->pio->locate(pkg, &offset)) {
				free(pac_header);
				return -CUI_ELOC;
			}

			if (pkg->pio->read(pkg, &index_buffer[i].offset, 4)) {
				free(pac_header);
				return -CUI_EREAD;
			}

			for (k = 0; k < 4; k++) {
				offset &= sizeof(pac_header->decrypt_buffer) - 1;
				((BYTE *)&index_buffer[i].offset)[k] ^= pac_header->decrypt_buffer[offset++];
			}
		} else {
			if (pkg->pio->read(pkg, &name_len, 4)) {
				free(pac_header);
				return -CUI_EREAD;
			}

			if (pkg->pio->read(pkg, index_buffer[i].name, name_len)) {
				free(pac_header);
				return -CUI_EREAD;
			}

			if (pkg->pio->read(pkg, &index_buffer[i].length, 4)) {
				free(pac_header);
				return -CUI_EREAD;
			}

			if (pkg->pio->read(pkg, &index_buffer[i].offset, 4)) {
				free(pac_header);
				return -CUI_EREAD;
			}
		}
	}

	pkg_dir->index_entries = pac_header->index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(pac_entry_t);
	package_set_private(pkg, pac_header);

	return 0;
}

/* 封包索引项解析函数 */
static int DISTORTION_pac_parse_resource_info(struct package *pkg,
											  struct package_resource *pkg_res)
{
	pac_entry_t *pac_entry;

	pac_entry = (pac_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, pac_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = pac_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = pac_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int DISTORTION_pac_extract_resource(struct package *pkg,
										   struct package_resource *pkg_res)
{
	pac_header_t *pac_header;
	BYTE *raw, *actual;
	DWORD actual_length;
	SIZE_T offset;

	pac_header = (pac_header_t *)package_get_private(pkg);

	raw = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw,  pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(raw);
			return -CUI_EREADVEC;
	}

	if (pac_header->is_encrypt) {
		offset = pkg_res->offset;
		for (DWORD k = 0; k < pkg_res->raw_data_length; k++) {
			offset &= sizeof(pac_header->decrypt_buffer) - 1;
			raw[k] ^= pac_header->decrypt_buffer[offset++];
		}
	}

	if (!memcmp(raw, "TEYL", 4)) {
		bmp_header_t *bmp_header = (bmp_header_t  *)raw;

		if (MyBuildBMPFile((BYTE *)(bmp_header + 1), pkg_res->raw_data_length - sizeof(bmp_header_t),
				NULL, 0, bmp_header->width, 0 - bmp_header->height, 32, 
					&actual, &actual_length, my_malloc)) {
			free(raw);
			return -CUI_EMEM;
		}		
	} else {
		actual = NULL;
		actual_length = 0;
	}

	pkg_res->raw_data = raw;
	pkg_res->actual_data = actual;
	pkg_res->actual_data_length = actual_length;

	return 0;
}

/* 资源保存函数 */
static int DISTORTION_pac_save_resource(struct resource *res, 
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
static void DISTORTION_pac_release_resource(struct package *pkg, 
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
static void DISTORTION_pac_release(struct package *pkg, 
								   struct package_directory *pkg_dir)
{
	pac_header_t *pac_header;

	pac_header = (pac_header_t *)package_get_private(pkg);
	if (pac_header) {
		free(pac_header);
		package_set_private(pkg, NULL);
	}

	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation DISTORTION_pac_operation = {
	DISTORTION_pac_match,					/* match */
	DISTORTION_pac_extract_directory,		/* extract_directory */
	DISTORTION_pac_parse_resource_info,		/* parse_resource_info */
	DISTORTION_pac_extract_resource,		/* extract_resource */
	DISTORTION_pac_save_resource,			/* save_resource */
	DISTORTION_pac_release_resource,		/* release_resource */
	DISTORTION_pac_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK DISTORTION_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".pac"), NULL, 
		NULL, &DISTORTION_pac_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
