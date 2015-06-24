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

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information usi_cui_information = {
	_T(""),					/* copyright */
	_T(""),					/* system */
	_T(".UCG"),				/* package */
	_T("1.0.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-1-25 21:27"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

static void des_decrypt(BYTE *buf, DWORD len, const char *key = "FIREWORK")
{
 	DES_cblock deskey;
	DES_cblock ivec;
	DES_key_schedule ks;

	memset(deskey, 0, 8);
	strncpy((char *)deskey, key, 8);
	memset(ivec, 0, 8);
	DES_set_key_unchecked(&deskey, &ks);
	DES_ncbc_encrypt(buf, buf, len, &ks, &ivec, 0);
}

/********************* UCG *********************/

/* 封包匹配回调函数 */
static int usi_UCG_match(struct package *pkg)
{
	s8 magic[8];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}
	des_decrypt((BYTE *)magic, sizeof(magic));

	if (strncmp(magic, "BM", 2)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int usi_UCG_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	BYTE *buf = new BYTE[pkg_res->raw_data_length];
	if (!buf)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, buf, pkg_res->raw_data_length, 0, IO_SEEK_SET)) {
		delete [] buf;
		return -CUI_EREADVEC;
	}

	des_decrypt(buf, pkg_res->raw_data_length);

	pkg_res->raw_data = buf;

	return 0;
}

/* 资源保存函数 */
static int usi_UCG_save_resource(struct resource *res, 
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
static void usi_UCG_release_resource(struct package *pkg, 
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
static void usi_UCG_release(struct package *pkg, 
							struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation usi_UCG_operation = {
	usi_UCG_match,				/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	usi_UCG_extract_resource,	/* extract_resource */
	usi_UCG_save_resource,		/* save_resource */
	usi_UCG_release_resource,	/* release_resource */
	usi_UCG_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK usi_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".UCG"), _T(".bmp"), 
		NULL, &usi_UCG_operation, CUI_EXT_FLAG_PKG))
			return -1;

	return 0;
}
