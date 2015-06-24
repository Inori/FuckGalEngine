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

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information ScenePlayer_cui_information = {
	_T(""),					/* copyright */
	_T(""),					/* system */
	_T(".Pmx .PMP .PMA .PMW"),	/* package */
	_T("0.9.2"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-10-19 12:20"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];
	u32 index_entries;
} dat_header_t;
#pragma pack ()

typedef struct {
	s8 name[256];
	u32 offset;
	u32 length;
} PMA_entry_t;

static int PMA_readvec(struct package *pkg, void *_data, DWORD len, DWORD offset, 
					   DWORD method)
{
	BYTE *data = (BYTE *)_data;
	int ret = pkg->pio->readvec(pkg, data, len, offset, method);
	if (ret)
		return ret;

	DWORD align = offset & 3;
	if (align) {
		for (DWORD i = align; ((i < 4) && len); ++i) {
			*data++ ^= 0x21;
			--len;
		}
	}

	u32 *p = (u32 *)data;
	for (DWORD i = 0; i < len / 4; ++i)
		p[i] ^= 0x21212121;
	data += 4 * i;

	align = len & 3;
	for (i = 0; i < align; ++i)
		data[i] ^= 0x21;

	return 0;
}

static int PMA_read(struct package *pkg, void *data, DWORD len)
{
	u32 offset;
	pkg->pio->locate(pkg, &offset);
	return PMA_readvec(pkg, data, len, offset, IO_SEEK_SET);
}

/********************* Pmx *********************/

/* 封包匹配回调函数 */
static int ScenePlayer_Pmx_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包资源提取函数 */
static int ScenePlayer_Pmx_extract_resource(struct package *pkg,
											struct package_resource *pkg_res)
{
	BYTE *compr = new BYTE[pkg_res->raw_data_length];
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, pkg_res->raw_data_length,
		0, IO_SEEK_SET)) {
			delete [] compr;
			return -CUI_EREADVEC;
	}

	for (DWORD i = 0; i < pkg_res->raw_data_length; ++i)
		compr[i] ^= 0x21;

	DWORD uncomprlen = 0x200000;
	BYTE *uncompr = NULL;
	while (1) {
		uncompr = new BYTE[uncomprlen];
		if (!uncompr) {
			delete [] compr;
			return -CUI_EMEM;
		}

		pkg_res->actual_data_length = uncomprlen;
		int ret = uncompress(uncompr, &pkg_res->actual_data_length, compr, pkg_res->raw_data_length);
		// 失格医師体验版: PMP\EV_CG\HA13RB.PMP
		if (ret == Z_OK)
			break;
		delete [] uncompr;	
		uncomprlen <<= 1;
	}
	delete [] compr;

	pkg_res->actual_data = uncompr;

	return 0;
}

/* 资源保存函数 */
static int ScenePlayer_Pmx_save_resource(struct resource *res, 
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
static void ScenePlayer_Pmx_release_resource(struct package *pkg, 
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
static void ScenePlayer_Pmx_release(struct package *pkg, 
									struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation ScenePlayer_Pmx_operation = {
	ScenePlayer_Pmx_match,					/* match */
	NULL,
	NULL,
	ScenePlayer_Pmx_extract_resource,		/* extract_resource */
	ScenePlayer_Pmx_save_resource,			/* save_resource */
	ScenePlayer_Pmx_release_resource,		/* release_resource */
	ScenePlayer_Pmx_release					/* release */
};

/********************* PMP *********************/

/* 封包资源提取函数 */
static int ScenePlayer_PMP_extract_resource(struct package *pkg,
											struct package_resource *pkg_res)
{
	BYTE *compr = new BYTE[pkg_res->raw_data_length];
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, pkg_res->raw_data_length,
		0, IO_SEEK_SET)) {
			delete [] compr;
			return -CUI_EREADVEC;
	}

	for (DWORD i = 0; i < pkg_res->raw_data_length; ++i)
		compr[i] ^= 0x21;

	DWORD uncomprlen = 0x200000;
	BYTE *uncompr = NULL;
	while (1) {
		uncompr = new BYTE[uncomprlen];
		if (!uncompr) {
			delete [] compr;
			return -CUI_EMEM;
		}

		pkg_res->actual_data_length = uncomprlen;
		int ret = uncompress(uncompr, &pkg_res->actual_data_length, compr, pkg_res->raw_data_length);
		// 失格医師体验版: PMP\EV_CG\HA13RB.PMP
		if (ret == Z_OK)
			break;

		if (ret == Z_DATA_ERROR && pkg_res->actual_data_length) {
			pkg_res->actual_data_length = *(u32 *)&uncompr[2];
			break;
		}
		delete [] uncompr;	
		uncomprlen <<= 1;
	}
	delete [] compr;

	pkg_res->actual_data = uncompr;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation ScenePlayer_PMP_operation = {
	ScenePlayer_Pmx_match,					/* match */
	NULL,
	NULL,
	ScenePlayer_PMP_extract_resource,		/* extract_resource */
	ScenePlayer_Pmx_save_resource,			/* save_resource */
	ScenePlayer_Pmx_release_resource,		/* release_resource */
	ScenePlayer_Pmx_release					/* release */
};

/********************* PMP *********************/

/* 封包资源提取函数 */
static int ScenePlayer_PMA_extract_resource(struct package *pkg,
											struct package_resource *pkg_res)
{
	BYTE *compr = new BYTE[pkg_res->raw_data_length];
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, pkg_res->raw_data_length,
		0, IO_SEEK_SET)) {
			delete [] compr;
			return -CUI_EREADVEC;
	}

	for (DWORD i = 0; i < pkg_res->raw_data_length; ++i)
		compr[i] ^= 0x21;

	DWORD uncomprlen = 0x200000;
	BYTE *uncompr = NULL;
	while (1) {
		uncompr = new BYTE[uncomprlen];
		if (!uncompr) {
			delete [] compr;
			return -CUI_EMEM;
		}

		pkg_res->actual_data_length = uncomprlen;
		int ret = uncompress(uncompr, &pkg_res->actual_data_length, compr, pkg_res->raw_data_length);
		if (ret == Z_OK)
			break;

		delete [] uncompr;	
		uncomprlen <<= 1;
	}
	delete [] compr;

	pkg_res->actual_data = uncompr;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation ScenePlayer_PMA_operation = {
	ScenePlayer_Pmx_match,					/* match */
	NULL,
	NULL,
	ScenePlayer_PMA_extract_resource,		/* extract_resource */
	ScenePlayer_Pmx_save_resource,			/* save_resource */
	ScenePlayer_Pmx_release_resource,		/* release_resource */
	ScenePlayer_Pmx_release					/* release */
};

/********************* PMA2 *********************/

/* 封包匹配回调函数 */
static int ScenePlayer_PMA2_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包索引目录提取函数 */
static int ScenePlayer_PMA2_extract_directory(struct package *pkg,
											  struct package_directory *pkg_dir)
{
	if (pkg->pio->read(pkg, &pkg_dir->index_entries, 4))
		return -CUI_EREAD;

	DWORD index_buffer_length = pkg_dir->index_entries * sizeof(PMA_entry_t);
	PMA_entry_t *index_buffer = new PMA_entry_t[pkg_dir->index_entries];
	if (!index_buffer)
		return -CUI_EMEM;

	DWORD offset = 5;
	for (DWORD i = 0; i < pkg_dir->index_entries; ++i) {
		// 跨过"BM"标签
		if (pkg->pio->readvec(pkg, &index_buffer[i].length, 4, offset + 2, IO_SEEK_SET))
			break;

		sprintf(index_buffer[i].name, "%08d", i);
		index_buffer[i].offset = offset;
		offset += index_buffer[i].length + 1;	// 每个"BM"前总有1字节值为1的数据
	}
	if (i != pkg_dir->index_entries) {
		delete [] index_buffer;
		return -CUI_EREADVEC;
	}

	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(PMA_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int ScenePlayer_PMA2_parse_resource_info(struct package *pkg,
												struct package_resource *pkg_res)
{
	PMA_entry_t *MA_entry;

	MA_entry = (PMA_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, MA_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = MA_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = MA_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int ScenePlayer_PMA2_extract_resource(struct package *pkg,
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

	pkg_res->raw_data = raw;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation ScenePlayer_PMA2_operation = {
	ScenePlayer_Pmx_match,					/* match */
	ScenePlayer_PMA2_extract_directory,
	ScenePlayer_PMA2_parse_resource_info,
	ScenePlayer_PMA2_extract_resource,		/* extract_resource */
	ScenePlayer_Pmx_save_resource,			/* save_resource */
	ScenePlayer_Pmx_release_resource,		/* release_resource */
	ScenePlayer_Pmx_release					/* release */
};

/********************* PMW *********************/

/* 封包处理回调函数集合 */
static cui_ext_operation ScenePlayer_PMW_operation = {
	ScenePlayer_Pmx_match,					/* match */
	NULL,
	NULL,
	ScenePlayer_PMA_extract_resource,		/* extract_resource */
	ScenePlayer_Pmx_save_resource,			/* save_resource */
	ScenePlayer_Pmx_release_resource,		/* release_resource */
	ScenePlayer_Pmx_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK ScenePlayer_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".Pmx"), _T(".Pmx_"), 
		NULL, &ScenePlayer_Pmx_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".PMP"), _T(".bmp"), 
		NULL, &ScenePlayer_PMP_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".PMA"), _T(".PMA2"), 
		NULL, &ScenePlayer_PMA_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".PMA2"), _T(".bmp"), 
		NULL, &ScenePlayer_PMA2_operation, CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR
		| CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".PMW"), _T(".wav"), 
		NULL, &ScenePlayer_PMW_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	return 0;
}
