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
struct acui_information WGame_cui_information = {
	_T(""),				/* copyright */
	_T("WGame"),		/* system */
	_T(".bmp"),			/* package */
	_T("1.0.0"),		/* revision */
	_T("痴漢公賊"),		/* author */
	_T("2009-4-25 18:28"),				/* date */
	NULL,				/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
#pragma pack ()


static void *my_malloc(DWORD len)
{
	return new BYTE[len];
}

/********************* bmp *********************/

/* 封包匹配回调函数 */
static int WGame_bmp_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	u8 magic[14];
	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp((char *)magic, "zzzzzzzzzzzzzz", sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包资源提取函数 */
static int WGame_bmp_extract_resource(struct package *pkg,
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

	BITMAPFILEHEADER *bmfh = (BITMAPFILEHEADER *)raw;
	bmfh->bfType = 'MB';
	bmfh->bfSize = pkg_res->raw_data_length;
	bmfh->bfReserved1 = 0;
	bmfh->bfReserved2 = 0;
	BITMAPINFOHEADER *bmiHeader = (BITMAPINFOHEADER *)(bmfh + 1);
	bmfh->bfOffBits = sizeof(BITMAPFILEHEADER) + bmiHeader->biSize +
		bmiHeader->biClrUsed * 4;

	pkg_res->raw_data = raw;

	return 0;
}

/* 资源保存函数 */
static int WGame_bmp_save_resource(struct resource *res, 
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
static void WGame_bmp_release_resource(struct package *pkg, 
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
static void WGame_bmp_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation WGame_bmp_operation = {
	WGame_bmp_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	WGame_bmp_extract_resource,	/* extract_resource */
	WGame_bmp_save_resource,		/* save_resource */
	WGame_bmp_release_resource,	/* release_resource */
	WGame_bmp_release			/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK WGame_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".bmp"), _T(".bmp"), 
		NULL, &WGame_bmp_operation, CUI_EXT_FLAG_PKG))
			return -1;

	return 0;
}
