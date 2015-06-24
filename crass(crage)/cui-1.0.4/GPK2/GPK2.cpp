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
struct acui_information GPK2_cui_information = {
	_T(""),					/* copyright */
	_T(""),					/* system */
	_T(".scb"),				/* package */
	_T("1.0.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-8-16 12:20"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];		// "GPK2"
	u32 index_offset;
} GPK2_header_t;

typedef struct {
	u32 offset;
	u32 length;
	s8 name[128];
} GPK2_entry_t;

typedef struct {
	s8 magic[4];		// "GFB "
	u32 zero;			// ?
	u32 one;			// ?
	u32 comprlen;		// 0表示不压缩
	u32 uncomprlen;
	u32 offset;
	BITMAPINFOHEADER info;
} GFB_header_t;

typedef struct {
	s8 magic[4];		// "GLPK"
	u32 uncomprlen;
	u32 is_crypted;
} scb_header_t;
#pragma pack ()

static void *my_malloc(DWORD len)
{
	return new BYTE[len];
}

static DWORD lzss_uncompress(BYTE *uncompr, DWORD uncomprlen, 
							  BYTE *compr, DWORD comprlen)
{
	unsigned int act_uncomprlen = 0;
	/* compr中的当前字节中的下一个扫描位的位置 */
	unsigned int curbit = 0;
	/* compr中的当前扫描字节 */
	unsigned int curbyte = 0;
	unsigned int nCurWindowByte = 0xfee;
	unsigned int win_size = 4096;
	BYTE win[4096];
	WORD flag = 0;

	memset(win, 0, win_size);
	while (act_uncomprlen < uncomprlen) {
		flag >>= 1;
		if (!(flag & 0x0100))
			flag = compr[curbyte++] | 0xff00;

		if (flag & 1) {
			unsigned char data;

			data = compr[curbyte++];
			uncompr[act_uncomprlen++] = data;
			if (act_uncomprlen >= uncomprlen)
				return act_uncomprlen;
			/* 输出的1字节放入滑动窗口 */
			win[nCurWindowByte++] = data;
			nCurWindowByte &= win_size - 1;
		} else {
			unsigned int copy_bytes, win_offset;
			unsigned int i;

			win_offset = compr[curbyte++];
			copy_bytes = compr[curbyte++];
			win_offset |= (copy_bytes & 0xf0) << 4;
			copy_bytes &= 0x0f;
			copy_bytes += 3;

			for (i = 0; i < copy_bytes; ++i) {	
				unsigned char data;

				data = win[(win_offset + i) & (win_size - 1)];				
				uncompr[act_uncomprlen++] = data;
				/* 输出的1字节放入滑动窗口 */
				win[nCurWindowByte++] = data;
				nCurWindowByte &= win_size - 1;	
			}
		}
	}

	return act_uncomprlen;
}

/********************* GPK2 *********************/

/* 封包匹配回调函数 */
static int GPK2_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "GPK2", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int GPK2_extract_directory(struct package *pkg,
								  struct package_directory *pkg_dir)
{
	u32 index_offset;

	if (pkg->pio->read(pkg, &index_offset, 4))
		return -CUI_EREAD;

	u32 index_entries;
	if (pkg->pio->readvec(pkg, &index_entries, 4, index_offset, IO_SEEK_SET))
		return -CUI_EREADVEC;

	GPK2_entry_t *index = new GPK2_entry_t[index_entries];
	if (!index)
		return -CUI_EMEM;

	DWORD index_length = sizeof(GPK2_entry_t) * index_entries;
	if (pkg->pio->read(pkg, index, index_length)) {
		delete [] index;
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index;
	pkg_dir->directory_length = index_length;
	pkg_dir->index_entry_length = sizeof(GPK2_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int GPK2_parse_resource_info(struct package *pkg,
									struct package_resource *pkg_res)
{
	GPK2_entry_t *GPK2_entry;

	GPK2_entry = (GPK2_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, GPK2_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = GPK2_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = GPK2_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int GPK2_extract_resource(struct package *pkg,
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

	if (!memcmp(raw, "GFB ", 4)) {
		GFB_header_t *GFB = (GFB_header_t *)raw;
		BYTE *palette;
		DWORD palette_size;
		if (GFB->offset == sizeof(GFB_header_t)) {
			palette = NULL;
			palette_size = 0;
		} else {
			palette = raw + sizeof(GFB_header_t);
			palette_size = GFB->offset - sizeof(GFB_header_t);
		}
		if (GFB->comprlen) {
			BYTE *uncompr = new BYTE[GFB->uncomprlen];
			if (!uncompr) {
				delete [] raw;
				return -CUI_EMEM;
			}
			BYTE *compr = raw + GFB->offset;
			lzss_uncompress(uncompr, GFB->uncomprlen, compr, GFB->comprlen);

			if (MyBuildBMPFile(uncompr, GFB->uncomprlen, palette, palette_size,
					GFB->info.biWidth, GFB->info.biHeight, GFB->info.biBitCount,
					(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length,
					my_malloc)) {
				delete [] uncompr;
				delete [] raw;
				return -CUI_EMEM;
			}
			delete [] uncompr;
		} else {
			if (MyBuildBMPFile(raw + GFB->offset, GFB->uncomprlen, palette, palette_size,
					GFB->info.biWidth, GFB->info.biHeight, GFB->info.biBitCount,
					(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length,
					my_malloc)) {
				delete [] raw;
				return -CUI_EMEM;
			}
		}
		delete [] raw;
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp");
	} else
		pkg_res->raw_data = raw;

	return 0;
}

/* 资源保存函数 */
static int GPK2_save_resource(struct resource *res, 
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
static void GPK2_release_resource(struct package *pkg, 
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
static void GPK2_release(struct package *pkg, 
						 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation GPK2_operation = {
	GPK2_match,					/* match */
	GPK2_extract_directory,		/* extract_directory */
	GPK2_parse_resource_info,	/* parse_resource_info */
	GPK2_extract_resource,		/* extract_resource */
	GPK2_save_resource,			/* save_resource */
	GPK2_release_resource,		/* release_resource */
	GPK2_release				/* release */
};

/********************* scb *********************/

/* 封包匹配回调函数 */
static int GPK2_scb_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "GLPK", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int GPK2_scb_extract_resource(struct package *pkg,
								 struct package_resource *pkg_res)
{
	u32 uncomprlen, is_crypted;

	if (pkg->pio->read(pkg, &uncomprlen, sizeof(uncomprlen)))
		return -CUI_EREAD;

	if (pkg->pio->read(pkg, &is_crypted, sizeof(is_crypted)))
		return -CUI_EREAD;
	
	DWORD comprlen = pkg_res->raw_data_length - sizeof(scb_header_t);
	BYTE *compr = new BYTE[comprlen];
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, compr, comprlen)) {
		delete [] compr;
		return -CUI_EREAD;
	}

	if (is_crypted) {
		for (DWORD i = 0; i < comprlen; ++i)
			compr[i] = ~compr[i];
	}

	BYTE *uncompr = new BYTE[uncomprlen];
	if (!uncompr) {
		delete [] compr;
		return -CUI_EMEM;
	}

	lzss_uncompress(uncompr, uncomprlen, compr, comprlen);
	delete [] compr;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation GPK2_scb_operation = {
	GPK2_scb_match,				/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	GPK2_scb_extract_resource,	/* extract_resource */
	GPK2_save_resource,			/* save_resource */
	GPK2_release_resource,		/* release_resource */
	GPK2_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK GPK2_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, NULL, NULL, 
		NULL, &GPK2_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NOEXT))
			return -1;

	if (callback->add_extension(callback->cui, _T(".scb"),  _T(".txt"), 
		NULL, &GPK2_scb_operation, CUI_EXT_FLAG_PKG))
			return -1;

	return 0;
}
