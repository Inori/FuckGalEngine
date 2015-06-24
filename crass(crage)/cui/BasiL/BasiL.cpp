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
struct acui_information BasiL_cui_information = {
	_T(""),		/* copyright */
	_T(""),			/* system */
	_T(".ng3 .nss .MIF"),	/* package */
	_T(""),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T(""),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];	// "NG3"
	u32 width;
	u32 height;
} ng3_header_t;

typedef struct {
	s8 magic[4];	// "MIF"
	u32 index_entries;
} MIF_header_t;

typedef struct {
	s8 name[16];
	u32 offset;
	u32 length;
} MIF_entry_t;
#pragma pack ()

static void *my_malloc(DWORD len)
{
	return new BYTE[len];
}

/********************* ng3 *********************/

/* 封包匹配回调函数 */
static int BasiL_ng3_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "NG3", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int BasiL_ng3_extract_resource(struct package *pkg,
									  struct package_resource *pkg_res)
{
	u32 ng3_size;
	pkg->pio->length_of(pkg, &ng3_size);

	BYTE *ng3 = new BYTE[ng3_size];
	if (!ng3)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, ng3, ng3_size, 0, IO_SEEK_SET)) {
		delete [] ng3;
		return -CUI_EREADVEC;
	}

	ng3_header_t *header = (ng3_header_t *)ng3;
	DWORD uncomprlen = header->width * header->height * 3;
	BYTE *uncompr = new BYTE[uncomprlen];
	if (!uncompr) {
		delete [] ng3;
		return -CUI_EMEM;
	}

	BYTE *compr_data = (BYTE *)(header + 1);
	BYTE *compr = compr_data + 0x300;
	BYTE *dst = uncompr;

	for (DWORD i = 0; i < uncomprlen; ) {
		BYTE *src;

		if (compr[0] == 1) {
			src = compr_data + compr[1] * 3;
			*dst++ = *src++;
			*dst++ = *src++;
			*dst++ = *src++;
			compr += 2;
			i += 3;
		} else if (compr[0] == 2) {
			for (DWORD j = 0; j < compr[2]; ++j) {
				src = compr_data + compr[1] * 3;
				*dst++ = *src++;
				*dst++ = *src++;
				*dst++ = *src++;
				i += 3;
			}
			compr += 3;
		} else {
			*dst++ = *compr++;
			*dst++ = *compr++;
			*dst++ = *compr++;
			i += 3;
		}
	}

	if (MyBuildBMPFile(uncompr, uncomprlen, NULL, 0, header->width, header->height,
			24, (BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, my_malloc)) {
		delete [] uncompr;
		delete [] ng3;
		return -CUI_EMEM;
	}
	delete [] uncompr;
	delete [] ng3;

	return 0;
}

/* 资源保存函数 */
static int BasiL_ng3_save_resource(struct resource *res, 
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
static void BasiL_ng3_release_resource(struct package *pkg, 
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
static void BasiL_ng3_release(struct package *pkg, 
							  struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation BasiL_ng3_operation = {
	BasiL_ng3_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	BasiL_ng3_extract_resource,		/* extract_resource */
	BasiL_ng3_save_resource,		/* save_resource */
	BasiL_ng3_release_resource,		/* release_resource */
	BasiL_ng3_release				/* release */
};

/********************* nss *********************/

/* 封包匹配回调函数 */
static int BasiL_nss_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包资源提取函数 */
static int BasiL_nss_extract_resource(struct package *pkg,
									  struct package_resource *pkg_res)
{
	u32 nss_size;
	pkg->pio->length_of(pkg, &nss_size);

	BYTE *nss = new BYTE[nss_size];
	if (!nss)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, nss, nss_size, 0, IO_SEEK_SET)) {
		delete [] nss;
		return -CUI_EREADVEC;
	}

	for (DWORD i = 0; i < nss_size; ++i)
		nss[i] ^= 4;

	pkg_res->raw_data = nss;
	pkg_res->raw_data_length = nss_size;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation BasiL_nss_operation = {
	BasiL_nss_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	BasiL_nss_extract_resource,		/* extract_resource */
	BasiL_ng3_save_resource,		/* save_resource */
	BasiL_ng3_release_resource,		/* release_resource */
	BasiL_ng3_release				/* release */
};

/********************* MIF *********************/

/* 封包匹配回调函数 */
static int BasiL_MIF_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, 4)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "MIF", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int BasiL_MIF_extract_directory(struct package *pkg,
									   struct package_directory *pkg_dir)
{
	if (pkg->pio->read(pkg, &pkg_dir->index_entries, 4))
		return -CUI_EREAD;

	++pkg_dir->index_entries;
	DWORD index_length = pkg_dir->index_entries * sizeof(MIF_entry_t);
	MIF_entry_t *index = new MIF_entry_t[pkg_dir->index_entries];
	if (!index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index, index_length)) {
		delete [] index;
		return -CUI_EREAD;
	}

	pkg_dir->directory = index;
	pkg_dir->directory_length = index_length;
	pkg_dir->index_entry_length = sizeof(MIF_entry_t);
	//pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int BasiL_MIF_parse_resource_info(struct package *pkg,
										 struct package_resource *pkg_res)
{
	MIF_entry_t *MIF_entry;

	MIF_entry = (MIF_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, MIF_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = MIF_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = MIF_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int BasiL_MIF_extract_resource(struct package *pkg,
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
static cui_ext_operation BasiL_MIF_operation = {
	BasiL_MIF_match,				/* match */
	BasiL_MIF_extract_directory,	/* extract_directory */
	BasiL_MIF_parse_resource_info,	/* parse_resource_info */
	BasiL_MIF_extract_resource,		/* extract_resource */
	BasiL_ng3_save_resource,		/* save_resource */
	BasiL_ng3_release_resource,		/* release_resource */
	BasiL_ng3_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK BasiL_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".ng3"), _T(".bmp"), 
		NULL, &BasiL_ng3_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".nss"), _T(".txt"), 
		NULL, &BasiL_nss_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".MIF"), NULL, 
		NULL, &BasiL_MIF_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
