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
struct acui_information pokoten2_cui_information = {
	_T("こどものこえ"),	/* copyright */
	_T("(同人)エロチックゴンドラ ナンバー2 ～恋の見習いウンディーネ～"),	/* system */
	_T(".gpp .bm_ .mus .b"),	/* package */
	_T("1.0.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-1-17 22:31"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];
	u32 index_entries;
} dat_header_t;
#pragma pack ()

static get_4bits_flag = 0;

static BYTE get_4bits(BYTE *src, DWORD &curbyte)
{
	if (get_4bits_flag) {
		get_4bits_flag = 0;
		return src[curbyte++] & 0xf;
	} else {
		get_4bits_flag = 1;
		return src[curbyte] >> 4;
	}
}

static inline void get_pixel(BYTE *ret_pixel1, BYTE *ret_pixel2, BYTE *ret_pixel3, 
							 BYTE *src, DWORD &curbyte)
{
	*ret_pixel1 = get_4bits(src, curbyte) << 4;
	*ret_pixel1 = (get_4bits(src, curbyte) & 0xf) | *ret_pixel1;
	*ret_pixel2 = get_4bits(src, curbyte) << 4;
	*ret_pixel2 = (get_4bits(src, curbyte) & 0xf) | *ret_pixel2;
	*ret_pixel3 = get_4bits(src, curbyte) << 4;
	*ret_pixel3 = (get_4bits(src, curbyte) & 0xf) | *ret_pixel3;
}

static DWORD gpp_uncompress(BYTE *uncompr, DWORD uncomprlen, BYTE *compr, DWORD comprlen)
{
	BYTE step_table[8] = { 9, 4, 7, 2, 6, 8, 3, 5 };
	DWORD act_uncomprlen = 0;

	get_4bits_flag = 0;
	for (DWORD curbyte = 0; curbyte < comprlen; ) {
		BYTE flag = get_4bits(compr, curbyte);
		BYTE pixel[3];
		DWORD idx;

        if (flag & 8) {
			idx = flag & 7;
			for (DWORD i = 0; i < step_table[idx]; ++i) {
				get_pixel(pixel, pixel + 1, pixel + 2, compr, curbyte);
				uncompr[act_uncomprlen++] = pixel[0];
				uncompr[act_uncomprlen++] = pixel[1];
				uncompr[act_uncomprlen++] = pixel[2];
				if (act_uncomprlen >= uncomprlen)
					break;
			}
		} else {
			get_pixel(pixel, pixel + 1, pixel + 2, compr, curbyte);
			idx = flag & 7;
			for (DWORD i = 0; i < step_table[idx]; ++i) {
				uncompr[act_uncomprlen++] = pixel[0];
				uncompr[act_uncomprlen++] = pixel[1];
				uncompr[act_uncomprlen++] = pixel[2];
				if (act_uncomprlen >= uncomprlen)
					break;
			}
        }
	}
	return act_uncomprlen;
}

/********************* gpp *********************/

/* 封包匹配回调函数 */
static int pokoten2_gpp_match(struct package *pkg)
{
	s8 magic[2];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "BM", 2)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包资源提取函数 */
static int pokoten2_gpp_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	u32 gpp_size;
	pkg->pio->length_of(pkg, &gpp_size);

	BYTE *gpp = new BYTE[gpp_size];
	if (!gpp)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, gpp, gpp_size, 0, IO_SEEK_SET)) {
		delete [] gpp;
		return -CUI_EREADVEC;
	}

	BITMAPFILEHEADER *bmfh = (BITMAPFILEHEADER *)gpp;
	BITMAPINFOHEADER *bmiHeader = (BITMAPINFOHEADER *)(bmfh + 1);

	BYTE *bmp = new BYTE[bmfh->bfSize + 256];
	if (!bmp) {
		delete [] gpp;
		return -CUI_EMEM;
	}

	DWORD header_size = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	memcpy(bmp, gpp, header_size);
	gpp_uncompress(bmp + header_size, bmfh->bfSize - header_size, 
		gpp + header_size, gpp_size - header_size);
	pkg_res->actual_data = bmp;
	pkg_res->actual_data_length = bmfh->bfSize;
	delete [] gpp;

	return 0;
}

/* 资源保存函数 */
static int pokoten2_gpp_save_resource(struct resource *res, 
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
static void pokoten2_gpp_release_resource(struct package *pkg, 
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
static void pokoten2_gpp_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation pokoten2_gpp_operation = {
	pokoten2_gpp_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	pokoten2_gpp_extract_resource,	/* extract_resource */
	pokoten2_gpp_save_resource,		/* save_resource */
	pokoten2_gpp_release_resource,	/* release_resource */
	pokoten2_gpp_release			/* release */
};

/********************* mus *********************/

/* 封包匹配回调函数 */
static int pokoten2_mus_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "RIFF", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包资源提取函数 */
static int pokoten2_mus_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	u32 mus_size;
	pkg->pio->length_of(pkg, &mus_size);
	mus_size -= 0x2c;
	BYTE *mus = new BYTE[mus_size];
	if (!mus)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, mus, mus_size, 0x2c, IO_SEEK_SET)) {
		delete [] mus;
		return -CUI_EREADVEC;
	}

	pkg_res->raw_data = mus;
	pkg_res->raw_data_length = mus_size;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation pokoten2_mus_operation = {
	pokoten2_mus_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	pokoten2_mus_extract_resource,	/* extract_resource */
	pokoten2_gpp_save_resource,		/* save_resource */
	pokoten2_gpp_release_resource,	/* release_resource */
	pokoten2_gpp_release			/* release */
};

/********************* b *********************/

/* 封包匹配回调函数 */
static int pokoten2_b_match(struct package *pkg)
{
	if (lstrcmpi(pkg->name, L"config.b"))
		return -CUI_EMATCH;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包资源提取函数 */
static int pokoten2_b_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	u32 b_size;
	pkg->pio->length_of(pkg, &b_size);
	BYTE *b = new BYTE[b_size];
	if (!b)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, b, b_size, 0, IO_SEEK_SET)) {
		delete [] b;
		return -CUI_EREADVEC;
	}

	for (DWORD i = 0; i < b_size; ++i)
		b[i] = -b[i];

	pkg_res->raw_data = b;
	pkg_res->raw_data_length = b_size;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation pokoten2_b_operation = {
	pokoten2_b_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	pokoten2_b_extract_resource,	/* extract_resource */
	pokoten2_gpp_save_resource,		/* save_resource */
	pokoten2_gpp_release_resource,	/* release_resource */
	pokoten2_gpp_release			/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK pokoten2_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".gpp"), _T(".bmp"), 
		NULL, &pokoten2_gpp_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".bm_"), _T(".bmp"), 
		NULL, &pokoten2_gpp_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".mus"), _T(".ogg"), 
		NULL, &pokoten2_mus_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".b"), _T("._b"), 
		NULL, &pokoten2_b_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	return 0;
}
