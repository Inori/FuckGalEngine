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
#include <zlib.h>

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information FVP_cui_information = {
	_T("Favorite"),			/* copyright */
	_T("FAVORITE VIEW POINT SYSTEM VER 2.5"),					/* system */
	_T(".bin"),				/* package */
	_T("1.1.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-4-4 23:22"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE | ACUI_ATTRIBUTE_PRELOAD
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s32 index_entries;
	s32 name_index_length;
} bin_header_t;

typedef struct {
	u32 name_offset;
	u32 data_offset;
	u32 data_length;
} bin_entry_t;

typedef struct {
	s8 magic[4];			// "hzc1"
	u32 uncomprlen;
	u32 parameters_length;	// 32
} hzc_header_t;

typedef struct {
	s8 magic[4];		// "NVSG"
	u16 unknown0;		// 0x100
	u16 color_type;
	u16 width;
	u16 height;
	u16 unknown2;		// [c]
	u16 unknown3;		// [e]
	u16 unknown4;		// [10]
	u16 unknown5;		// [12]
	u32 picts;			// [14]
	u32 reserved[2];	// 0
} nvsg_header_t;
#pragma pack ()

/* .bin封包的索引项结构 */
typedef struct {
	s8 name[256];
	u32 name_length;
	u32 offset;
	u32 length;
} my_bin_entry_t;

typedef struct {
	s8 name[32];
	u32 offset;
	u32 length;
	u16 width;
	u16 height;
} my_nvsg_entry_t;


static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static int FVP_bin_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir);

/********************* bin *********************/

/* 封包匹配回调函数 */
static int FVP_bin_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	struct package_directory pkg_dir;
	int ret = FVP_bin_extract_directory(pkg, &pkg_dir);
	if (!ret)
		free(pkg_dir.directory);

	return ret;	
}

/* 封包索引目录提取函数 */
static int FVP_bin_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{
	bin_header_t bin_header;
	bin_entry_t *index_buffer;
	my_bin_entry_t *my_index_buffer;
	BYTE *name_buffer;
	unsigned int index_buffer_length;	
	int i;

	if (pkg->pio->readvec(pkg, &bin_header, sizeof(bin_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	if (bin_header.index_entries <= 0 || bin_header.name_index_length <= 0)
		return -CUI_EMATCH;

	index_buffer_length = bin_header.index_entries * sizeof(bin_entry_t);
	index_buffer = (bin_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	u32 fsize;
	pkg->pio->length_of(pkg, &fsize);
	if (index_buffer[bin_header.index_entries - 1].data_offset + 
			index_buffer[bin_header.index_entries - 1].data_length != fsize) {
		free(index_buffer);
		return -CUI_EMATCH;
	}

	name_buffer = (BYTE *)malloc(bin_header.name_index_length);
	if (!name_buffer) {
		free(index_buffer);
		return -CUI_EMEM;
	}

	if (pkg->pio->read(pkg, name_buffer, bin_header.name_index_length)) {
		free(name_buffer);
		free(index_buffer);
		return -CUI_EREAD;
	}

	index_buffer_length = bin_header.index_entries * sizeof(my_bin_entry_t);
	my_index_buffer = (my_bin_entry_t *)malloc(index_buffer_length);
	if (!my_index_buffer) {
		free(name_buffer);
		free(index_buffer);
		return -CUI_EMEM;
	}

	for (i = 0; i < bin_header.index_entries; i++) {
		my_index_buffer[i].length = index_buffer[i].data_length;
		my_index_buffer[i].offset = index_buffer[i].data_offset;
		strncpy(my_index_buffer[i].name, (char *)&name_buffer[index_buffer[i].name_offset], 
			sizeof(my_index_buffer[i].name));
	}
	free(name_buffer);
	free(index_buffer);

	pkg_dir->index_entries = bin_header.index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(my_bin_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int FVP_bin_parse_resource_info(struct package *pkg,
									   struct package_resource *pkg_res)
{
	my_bin_entry_t *my_bin_entry;

	my_bin_entry = (my_bin_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_bin_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_bin_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_bin_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int FVP_bin_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)pkg->pio->readvec_only(pkg, comprlen, pkg_res->offset, IO_SEEK_SET);
	if (!compr)
		return -CUI_EREADVECONLY;

	uncompr = NULL;
	uncomprlen = 0;

	if (!memcmp(compr, "RIFF", 4)) {
		pkg_res->replace_extension = _T(".wav");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
	} else if (!memcmp(compr, "OggS", 4)) {
		pkg_res->replace_extension = _T(".ogg");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
	} else if (!memcmp(compr, "hzc1", 4)) {
		hzc_header_t *hzc = (hzc_header_t *)compr;
		BYTE *zlib_compr;
		DWORD zlib_comprlen;
		BYTE *act_uncompr;

		uncompr = (BYTE *)malloc(hzc->uncomprlen + hzc->parameters_length);
		if (!uncompr)
			return -CUI_EMEM;

		zlib_compr = (BYTE *)(hzc + 1) + hzc->parameters_length;
		zlib_comprlen = comprlen - sizeof(hzc_header_t) - hzc->parameters_length;
		uncomprlen = hzc->uncomprlen;
		if (uncompress(uncompr + hzc->parameters_length, &uncomprlen, zlib_compr, zlib_comprlen) != Z_OK) {
			free(uncompr);
			return -CUI_EUNCOMPR;
		}
		if (uncomprlen != hzc->uncomprlen) {
			free(uncompr);
			return -CUI_EUNCOMPR;
		}
		memcpy(uncompr, hzc + 1, hzc->parameters_length);
		compr = NULL;

		if (!memcmp(uncompr, "NVSG", 4)) {
			nvsg_header_t *nvsg = (nvsg_header_t *)uncompr;

			if (nvsg->color_type == 0) {
				if (MyBuildBMPFile((BYTE *)(nvsg + 1), uncomprlen, NULL, 0, nvsg->width, 
						0 - nvsg->height, 24, &act_uncompr, &uncomprlen, my_malloc)) {
					free(uncompr);
					return -CUI_EMEM;			
				}
				free(uncompr);
				uncompr = act_uncompr;
				pkg_res->replace_extension = _T(".bmp");
				pkg_res->flags |= PKG_RES_FLAG_REEXT;
			} else if (nvsg->color_type == 1) {
				alpha_blending((BYTE *)(nvsg + 1), nvsg->width, nvsg->height, 32);
				if (MyBuildBMPFile((BYTE *)(nvsg + 1), uncomprlen, NULL, 0, nvsg->width, 
						0 - nvsg->height, 32, &act_uncompr, &uncomprlen, my_malloc)) {
					free(uncompr);
					return -CUI_EMEM;			
				}
				free(uncompr);
				uncompr = act_uncompr;
				pkg_res->replace_extension = _T(".bmp");
				pkg_res->flags |= PKG_RES_FLAG_REEXT;
			} else if (nvsg->color_type == 3) {
				if (MyBuildBMPFile((BYTE *)(nvsg + 1), uncomprlen, NULL, 0, nvsg->width, 
						0 - nvsg->height, 8, &act_uncompr, &uncomprlen, my_malloc)) {
					free(uncompr);
					return -CUI_EMEM;			
				}
				free(uncompr);
				uncompr = act_uncompr;
				pkg_res->replace_extension = _T(".bmp");
				pkg_res->flags |= PKG_RES_FLAG_REEXT;
			} else if (nvsg->color_type == 2) {
				uncomprlen += sizeof(nvsg_header_t);
				pkg_res->replace_extension = _T(".nvsg");
				pkg_res->flags |= PKG_RES_FLAG_REEXT;
			} else if (nvsg->color_type == 4) {
				BYTE *p = (BYTE *)(nvsg + 1);
				for (DWORD i = 0; i < uncomprlen; ++i)
					if (p[i])
						p[i] = 0xff;

				if (MyBuildBMPFile((BYTE *)(nvsg + 1), uncomprlen, NULL, 0, nvsg->width, 
						0 - nvsg->height, 8, &act_uncompr, &uncomprlen, my_malloc)) {
					free(uncompr);
					return -CUI_EMEM;			
				}
				free(uncompr);
				uncompr = act_uncompr;			
				pkg_res->replace_extension = _T(".bmp");
				pkg_res->flags |= PKG_RES_FLAG_REEXT;
			} else {
				free(uncompr);
				return -CUI_EMATCH;
			}
		}
	} 

	pkg_res->raw_data = compr;
	pkg_res->raw_data_length = comprlen;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

/* 资源保存函数 */
static int FVP_bin_save_resource(struct resource *res, 
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
static void FVP_bin_release_resource(struct package *pkg, 
									 struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res->raw_data)
		pkg_res->raw_data = NULL;
}

/* 封包卸载函数 */
static void FVP_bin_release(struct package *pkg, 
							struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation FVP_bin_operation = {
	FVP_bin_match,					/* match */
	FVP_bin_extract_directory,		/* extract_directory */
	FVP_bin_parse_resource_info,	/* parse_resource_info */
	FVP_bin_extract_resource,		/* extract_resource */
	FVP_bin_save_resource,			/* save_resource */
	FVP_bin_release_resource,		/* release_resource */
	FVP_bin_release					/* release */
};

/********************* nvsg *********************/

/* 封包匹配回调函数 */
static int FVP_nvsg_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "NVSG", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int FVP_nvsg_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	nvsg_header_t nvsg_header;
	my_nvsg_entry_t *index_buffer;
	DWORD index_buffer_length;	
	DWORD bmp_length, offset;

	if (pkg->pio->readvec(pkg, &nvsg_header, sizeof(nvsg_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	index_buffer_length = nvsg_header.picts * sizeof(my_nvsg_entry_t);
	index_buffer = (my_nvsg_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	bmp_length = nvsg_header.width * nvsg_header.height * 4;
	offset = sizeof(nvsg_header);
	for (DWORD i = 0; i < nvsg_header.picts; i++) {
		sprintf(index_buffer[i].name, "%04d.bmp", i);
		index_buffer[i].length = bmp_length;
		index_buffer[i].offset = offset;
		offset += bmp_length;
		index_buffer[i].width = nvsg_header.width;
		index_buffer[i].height = nvsg_header.height;
	}

	pkg_dir->index_entries = nvsg_header.picts;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(my_nvsg_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int FVP_nvsg_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	my_nvsg_entry_t *my_nvgs_entry;

	my_nvgs_entry = (my_nvsg_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_nvgs_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_nvgs_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_nvgs_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int FVP_nvsg_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	my_nvsg_entry_t *my_nvgs_entry;

	pkg_res->raw_data = malloc(pkg_res->raw_data_length);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, pkg_res->raw_data_length, 
			pkg_res->offset, IO_SEEK_SET)) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
		return -CUI_EREADVEC;
	}

	my_nvgs_entry = (my_nvsg_entry_t *)pkg_res->actual_index_entry;
	if (MyBuildBMPFile((BYTE *)pkg_res->raw_data, pkg_res->raw_data_length, NULL, 0, my_nvgs_entry->width, 
			0 - my_nvgs_entry->height, 32, (BYTE **)&pkg_res->actual_data, 
			&pkg_res->actual_data_length, my_malloc)) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
		return -CUI_EMEM;			
	}

	return 0;
}

/* 资源保存函数 */
static int FVP_nvsg_save_resource(struct resource *res, 
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
static void FVP_nvsg_release_resource(struct package *pkg, 
									  struct package_resource *pkg_res)
{
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

/* 封包卸载函数 */
static void FVP_nvsg_release(struct package *pkg, 
							 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation FVP_nvsg_operation = {
	FVP_nvsg_match,					/* match */
	FVP_nvsg_extract_directory,		/* extract_directory */
	FVP_nvsg_parse_resource_info,	/* parse_resource_info */
	FVP_nvsg_extract_resource,		/* extract_resource */
	FVP_nvsg_save_resource,			/* save_resource */
	FVP_nvsg_release_resource,		/* release_resource */
	FVP_nvsg_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK FVP_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".bin"), NULL, 
		NULL, &FVP_bin_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".nvsg"), NULL, 
		NULL, &FVP_nvsg_operation, CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
