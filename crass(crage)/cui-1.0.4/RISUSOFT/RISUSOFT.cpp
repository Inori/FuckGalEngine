#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>
#include <openssl/des.h>
#include <utility.h>

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information RISUSOFT_cui_information = {
	_T("RISUSOFT"),		/* copyright */
	_T(""),				/* system */
	_T(".properties"),	/* package */
	_T("1.0.0"),		/* revision */
	_T("痴漢公賊"),		/* author */
	_T("2009-4-18 21:07"),				/* date */
	NULL,				/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];
	u32 index_entries;
} dat_header_t;
#pragma pack ()

static void des_decrypt(BYTE *buf, DWORD len, const char *key = "risu.sakura.ne.jp")
{
 	DES_cblock deskey;
	DES_cblock ivec;
	DES_key_schedule ks;

	memset(deskey, 0, sizeof(deskey));
	strcpy((char *)deskey, key);
	memset(ivec, 0, sizeof(ivec));
	DES_set_key_unchecked(&deskey, &ks);
	for (DWORD i = 0; i < len / 8; ++i)
		DES_encrypt1((unsigned long *)buf + i*2, &ks, 0);
}

/*********************  *********************/

/* 封包匹配回调函数 */
static int RISUSOFT_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	u8 magic[8];	// at least 8 bytes 
	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	des_decrypt(magic, sizeof(magic));

	if (!strncmp((char *)magic, "OggS", 4))
		;
	else if (!strncmp((char *)magic, "RIFF", 4))
		;
	else if (!strncmp((char *)magic, "\x89PNG", 4))
		;
	else if (!strncmp((char *)magic, "<?xml ", 6))
		;
	else {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包资源提取函数 */
static int RISUSOFT_extract_resource(struct package *pkg,
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

	des_decrypt(raw, pkg_res->raw_data_length);

	if (!strncmp((char *)raw, "OggS", 4)) {
		pkg_res->replace_extension = _T(".ogg");
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
	} else if (!strncmp((char *)raw, "RIFF", 4)) {
		pkg_res->replace_extension = _T(".wav");
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
	} else if (!strncmp((char *)raw, "\x89PNG", 4)) {
		pkg_res->replace_extension = _T(".png");
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
	} else if (!strncmp((char *)raw, "<?xml ", 6)) {
		pkg_res->replace_extension = _T(".xml");
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
	}

	pkg_res->raw_data = raw;

	return 0;
}

/* 资源保存函数 */
static int RISUSOFT_save_resource(struct resource *res, 
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
static void RISUSOFT_release_resource(struct package *pkg, 
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
static void RISUSOFT_release(struct package *pkg, 
							 struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation RISUSOFT_operation = {
	RISUSOFT_match,				/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	RISUSOFT_extract_resource,	/* extract_resource */
	RISUSOFT_save_resource,		/* save_resource */
	RISUSOFT_release_resource,	/* release_resource */
	RISUSOFT_release			/* release */
};

/********************* properties *********************/

/* 封包匹配回调函数 */
static int RISUSOFT_properties_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包处理回调函数集合 */
static cui_ext_operation RISUSOFT_properties_operation = {
	RISUSOFT_properties_match,	/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	RISUSOFT_extract_resource,	/* extract_resource */
	RISUSOFT_save_resource,		/* save_resource */
	RISUSOFT_release_resource,	/* release_resource */
	RISUSOFT_release			/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK RISUSOFT_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, NULL, NULL, 
		NULL, &RISUSOFT_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_NOEXT))
			return -1;

	if (callback->add_extension(callback->cui, _T(".properties"), NULL, 
		NULL, &RISUSOFT_properties_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	return 0;
}
