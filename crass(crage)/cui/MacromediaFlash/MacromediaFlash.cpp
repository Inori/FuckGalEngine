#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>
#include <zlib.h>
#include <utility.h>

// http://school.cfan.com.cn/pro/pother/2006-09-12/1158030322d16493.shtml
// http://topic.csdn.net/t/20050707/20/4130073.html
// http://www.warpfield.cn/blog/?p=146

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information MacromediaFlash_cui_information = {
	_T("Macromedia"),		/* copyright */
	_T("MacromediaFlash"),	/* system */
	_T(".swf"),				/* package */
	_T("0.0.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T(""),					/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[3];	// "FWS"
	u8 version;
	u32 length;		// flash文件的大小
} swf_header_t;

typedef struct {	// Flash 6开始支持
	s8 magic[3];	// "CWS"
	u8 version;		// 7, 8
	u32 uncomprlen;	// flash文件的原始大小
} cswf_header_t;

typedef struct {
	RECT frame_size;	// 以twip为单位(1/20像素)记录帧的尺寸 (共8字节)
	u16 frame_rate;
	u16 frame_count;
} swf_info_t;

#pragma pack ()

typedef struct {
	char name[MAX_PATH];
	DWORD name_length;
	DWORD offset;
	DWORD length;
} my_cswf_entry_t;

/********************* cswf *********************/

/* 封包匹配回调函数 */
static int MacromediaFlash_cswf_match(struct package *pkg)
{
	cswf_header_t cswf_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &cswf_header, sizeof(cswf_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(cswf_header.magic, "CWS", 3) || cswf_header.version < 7
				|| cswf_header.uncomprlen < 8) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}
	package_set_private(pkg, cswf_header.version);

	return 0;	
}

/* 封包索引目录提取函数 */
static int MacromediaFlash_cswf_extract_directory(struct package *pkg,
												  struct package_directory *pkg_dir)
{
	cswf_header_t cswf_header;

	if (pkg->pio->readvec(pkg, &cswf_header, sizeof(cswf_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	u32 comprlen;
	pkg->pio->length_of(pkg, &comprlen);
	comprlen -= sizeof(cswf_header);

	BYTE *compr = new BYTE[comprlen];
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, compr, comprlen)) {
		delete [] compr;
		return -CUI_EREAD;
	}

	BYTE *uncompr = new BYTE[cswf_header.uncomprlen - 8];
	if (!uncompr) {
		delete [] compr;
		return -CUI_EMEM;
	}

	DWORD act_uncomprlen = cswf_header.uncomprlen - 8;
	if (uncompress(uncompr, &act_uncomprlen, compr, comprlen) != Z_OK) {
		delete [] uncompr;
		delete [] compr;
		return -CUI_EUNCOMPR;		
	}
printf("%x %x\n", act_uncomprlen, cswf_header.uncomprlen);
	MySaveFile(_T("swf"), uncompr, act_uncomprlen);
#if 0

	pkg_dir->index_entries = k;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = k * sizeof(my_cswf_entry_t);
	pkg_dir->index_entry_length = sizeof(my_cswf_entry_t);
#endif
	return 0;
}

/* 封包索引项解析函数 */
static int MacromediaFlash_cswf_parse_resource_info(struct package *pkg,
													struct package_resource *pkg_res)
{
#if 0
	my_cswf_entry_t *my_cswf_entry;

	my_cswf_entry = (my_cswf_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_cswf_entry->name);
	pkg_res->name_length = my_cswf_entry->name_length;
	pkg_res->raw_data_length = my_cswf_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_cswf_entry->offset;
#endif
	return 0;
}

/* 封包资源提取函数 */
static int MacromediaFlash_cswf_extract_resource(struct package *pkg,
												 struct package_resource *pkg_res)
{
	char *p;

	pkg_res->raw_data = malloc(pkg_res->raw_data_length);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EREADVEC;
	}

	p = (char *)pkg_res->raw_data;
	if (!memcmp(p, "RIFF", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".wav");
	}

	return 0;
}

/* 资源保存函数 */
static int MacromediaFlash_cswf_save_resource(struct resource *res, 
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
static void MacromediaFlash_cswf_release_resource(struct package *pkg, 
										struct package_resource *pkg_res)
{
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

/* 封包卸载函数 */
static void MacromediaFlash_cswf_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg->lst);
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation MacromediaFlash_cswf_operation = {
	MacromediaFlash_cswf_match,					/* match */
	MacromediaFlash_cswf_extract_directory,		/* extract_directory */
	MacromediaFlash_cswf_parse_resource_info,	/* parse_resource_info */
	MacromediaFlash_cswf_extract_resource,		/* extract_resource */
	MacromediaFlash_cswf_save_resource,			/* save_resource */
	MacromediaFlash_cswf_release_resource,		/* release_resource */
	MacromediaFlash_cswf_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK MacromediaFlash_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".swf"), NULL, 
		NULL, &MacromediaFlash_cswf_operation, CUI_EXT_FLAG_PKG 
		| CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
