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

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information G2EngineSystem_cui_information = {
	_T("YAMANA Hideaki"),	/* copyright */
	_T("G2EngineSystem"),	/* system */
	_T(".pak"),				/* package */
	_T("1.0.0"),			/* revision */
	_T("痴汉公贼"),			/* author */
	_T("2007-3-24 10:12"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];	// "GCEX"
	u32 zeroed;		// must be 0
	u32 offset_lo;
	u32 offset_hi;
} gcex_header_t;

typedef struct {
	s8 magic[4];	// "GCE3"
	u32 flags;		// 0x11
	u32 length_lo;	// 包括文件头本身
	u32 length_hi;
	u32 unknown1;	// 0
	u32 crc;
	u32 unknown2;
	u32 unknown3;
} gce3_header_t;
#pragma pack ()

/********************* pak *********************/

/* 封包匹配回调函数 */
static int G2EngineSystem_pak_match(struct package *pkg)
{
	s8 magic[4];
	u32 zeroed;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "GCEX", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (pkg->pio->read(pkg, &zeroed, sizeof(zeroed))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (zeroed) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int G2EngineSystem_pak_extract_directory(struct package *pkg,
												struct package_directory *pkg_dir)
{
	u32 offset_lo;
	if (pkg->pio->read(pkg, &offset_lo, sizeof(offset_lo)))
		return -CUI_EREAD;

	if (pkg->pio->seek(pkg, offset_lo, IO_SEEK_SET))
		return -CUI_ESEEK;

	gcex_header_t gcex_header;
	if (pkg->pio->read(pkg, &gcex_header, sizeof(gcex_header)))
		return -CUI_EREAD;

	if (!strncmp(gcex_header.magic, "GCE3", 4)) {
		if (pkg->pio->seek(pkg, offset_lo, IO_SEEK_SET))
			return -CUI_ESEEK;

		if (pkg->pio->read(pkg, &gce3_header, sizeof(gce3_header)))
			return -CUI_EREAD;



	} else if (!strncmp(gcex_header.magic, "GCE1", 4)) {

	} else if (!strncmp(gcex_header.magic, "GCE0", 4)) {

	} else
		return -CUI_EMATCH;
	

	dat_entry_t *index_buffer;
	unsigned int index_buffer_length;	
	unsigned int i;	
	gce3_header_t gce3_header；






	if (pkg->pio->read(pkg, &gce3_header, sizeof(gce3_header)))
		return -CUI_EREAD;

	if (strncmp(gce3_header.magic, "GCE3", 4))
		return -CUI_EMATCH;

	if (gce3_header.length_lo < 32)
		return -CUI_EMATCH;

	gce3_data_len = gce3_header.length_lo - sizeof(gce3_header);
	gce3_data = (BYTE *)malloc(gce3_data_len);
	if (!gce3_data)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, gce3_data, gce3_data_len)) {
		free(gce3_data);
		return -CUI_EREAD;
	}
	
	crc = crc32(0L, &gce3_header, sizeof(gce3_header));
	if (crc32(crc, gce3_data, gce3_data_len) != gce3_header.crc) {
		free(gce3_data);
		return -CUI_EMATCH;
	}





	index_buffer_length = pkg_dir->index_entries * sizeof(dat_entry_t);
	index_buffer = (dat_entry_t *)malloc(index_buffer_length);
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
		free(index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(dat_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int G2EngineSystem_pak_parse_resource_info(struct package *pkg,
									struct package_resource *pkg_res)
{
	dat_entry_t *dat_entry;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	dat_entry = (dat_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, dat_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = dat_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = dat_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int G2EngineSystem_pak_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	pkg_res->raw_data = malloc(pkg_res->raw_data_length);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EREADVEC;
	}

	return 0;
}

/* 资源保存函数 */
static int G2EngineSystem_pak_save_resource(struct resource *res, 
									struct package_resource *pkg_res)
{
	if (!res || !pkg_res)
		return -CUI_EPARA;
	
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
static void G2EngineSystem_pak_release_resource(struct package *pkg, 
										struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

/* 封包卸载函数 */
static void G2EngineSystem_pak_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (!pkg || !pkg_dir)
		return;

	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation G2EngineSystem_pak_operation = {
	G2EngineSystem_pak_match,					/* match */
	G2EngineSystem_pak_extract_directory,		/* extract_directory */
	G2EngineSystem_pak_parse_resource_info,		/* parse_resource_info */
	G2EngineSystem_pak_extract_resource,		/* extract_resource */
	G2EngineSystem_pak_save_resource,			/* save_resource */
	G2EngineSystem_pak_release_resource,		/* release_resource */
	G2EngineSystem_pak_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK G2EngineSystem_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &G2EngineSystem_pak_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
