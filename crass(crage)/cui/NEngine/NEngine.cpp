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
struct acui_information NEngine_cui_information = {
	_T(""),		/* copyright */
	_T(""),			/* system */
	_T(".aoi .s6scn .scn"),		/* package */
	_T(""),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T(""),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];		// "AOI1"
	u32 uncomprlen;
	u32 comprlen;
	u32 unknown;
} aoi1_header_t;

typedef struct {
	s8 magic[4];		// "AOI2"
	u32 width;
	u32 height;
	u8 format;
	u8 pad[3];
} aoi2_header_t;

typedef struct {
	s8 magic[4];		// "AOI3"
	u32 uncomprlen;
	u32 comprlen;
	u32 unknown;
} aoi3_header_t;
#pragma pack ()

/* .dat封包的索引项结构 */
typedef struct {
	s8 name[256];
	u32 name_length;
	u32 offset;
	u32 length;
} dat_entry_t;

/********************* aoi2 *********************/

/* 封包匹配回调函数 */
static int NEngine_aoi2_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "AOI2", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int NEngine_aoi2_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{

	dat_entry_t *index_buffer;
	unsigned int index_buffer_length;	
	unsigned int i;

	if (pkg->pio->read(pkg, &pkg_dir->index_entries, 4))
		return -CUI_EREAD;

	index_buffer_length = pkg_dir->index_entries * sizeof(dat_entry_t);
	index_buffer = new dat_entry_t[pkg_dir->index_entries];
	if (!index_buffer)
		return -CUI_EMEM;

	dat_entry_t *index = index_buffer;
	for (i = 0; i < pkg_dir->index_entries; i++) {
		if (pkg->pio->read(pkg, &index->name_length, 4))
			break;

		if (pkg->pio->read(pkg, index->name, index->name_length))
			break;
		index->name[index->name_length] = 0;

		if (pkg->pio->read(pkg, &index->offset, 4))
			break;

		if (pkg->pio->read(pkg, &index->length, 4))
			break;

		index++;
	}
	if (i != pkg_dir->index_entries) {
		delete [] index_buffer;
		return -CUI_EREAD;
	}

	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(dat_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int NEngine_aoi2_parse_resource_info(struct package *pkg,
									struct package_resource *pkg_res)
{
	dat_entry_t *dat_entry;

	dat_entry = (dat_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, dat_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = dat_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = dat_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int NEngine_aoi2_extract_resource(struct package *pkg,
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

	// .dst
	if (!strncmp((char *)raw, "OggS", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".ogg");
	} else if (!strncmp((char *)raw, "RIFF", 4)) {// .dse
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".wav");
	}

	pkg_res->raw_data = raw;

	return 0;
}

/* 资源保存函数 */
static int NEngine_aoi2_save_resource(struct resource *res, 
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
static void NEngine_aoi2_release_resource(struct package *pkg, 
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
static void NEngine_aoi2_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation NEngine_aoi2_operation = {
	NEngine_aoi2_match,					/* match */
	NEngine_aoi2_extract_directory,		/* extract_directory */
	NEngine_aoi2_parse_resource_info,		/* parse_resource_info */
	NEngine_aoi2_extract_resource,		/* extract_resource */
	NEngine_aoi2_save_resource,			/* save_resource */
	NEngine_aoi2_release_resource,		/* release_resource */
	NEngine_aoi2_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK NEngine_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".aoi"), NULL, 
		NULL, &NEngine_aoi2_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".s6scn"), NULL, 
		NULL, &NEngine_aoi1_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".scn"), NULL, 
		NULL, &NEngine_aoi3_operation, CUI_EXT_FLAG_PKG))
			return -1;

	return 0;
}
