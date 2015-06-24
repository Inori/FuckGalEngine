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
struct acui_information bakuretsu_cui_information = {
	_T("WHITE SNOW PROJECT"),	/* copyright */
	_T("(C75)(同人)爆裂生徒会 体験版 ver. 1.02"),	/* system */
	_T(".snr"),				/* package */
	_T("1.0.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-1-11 20:03"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/********************* snr *********************/

/* 封包匹配回调函数 */
static int bakuretsu_snr_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包资源提取函数 */
static int bakuretsu_snr_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	u32 snr_size;
	pkg->pio->length_of(pkg, &snr_size);

	BYTE *snr = new BYTE[snr_size];
	if (!snr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, snr, snr_size, 0, IO_SEEK_SET)) {
		delete [] snr;
		return -CUI_EREADVEC;
	}

	for (DWORD i = 0; i < snr_size; ++i)
		if (snr[i] != '\n')
			snr[i] ^= 8;

	pkg_res->raw_data = snr;
	pkg_res->raw_data_length = snr_size;

	return 0;
}

/* 资源保存函数 */
static int bakuretsu_snr_save_resource(struct resource *res, 
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
static void bakuretsu_snr_release_resource(struct package *pkg, 
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
static void bakuretsu_snr_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation bakuretsu_snr_operation = {
	bakuretsu_snr_match,			/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	bakuretsu_snr_extract_resource,	/* extract_resource */
	bakuretsu_snr_save_resource,	/* save_resource */
	bakuretsu_snr_release_resource,	/* release_resource */
	bakuretsu_snr_release			/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK bakuretsu_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".snr"), NULL, 
		NULL, &bakuretsu_snr_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	return 0;
}
