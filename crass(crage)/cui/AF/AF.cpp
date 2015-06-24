// 提取 BlueImpact 公司的游戏 Angel's Feather 资源文件的插件

#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>

struct acui_information AF_cui_information = 
{
		NULL,					/* copyright */
		NULL,					/* system */
		_T(".dat"),				/* package */
		_T("1.0.0"),			/* revision */
		_T("joyful"),			/* author */
		_T("2007-8-21 16:22"),	/* date */
		NULL,					/* notion */
		ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
/* .dat封包的索引项结构 */
typedef struct {
	u32 idx_length; // 该项索引项的长度
	u32 unknown; // 用途不明,好像都是0 ?
	u32 offset;
	u32 length;
	s8 name[128];
} dat_entry_t;
#pragma pack ()

/********************* dat *********************/

/* 封包匹配回调函数 */
// 本游戏无magic
static int AF_dat_match(struct package *pkg)
{

	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包索引目录提取函数 */
static int AF_dat_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	dat_entry_t *index_buffer;
	unsigned int index_buffer_length;	
	unsigned int i;
	unsigned int total_index_length; // 这个长度是封包中指出的索引段的长度
	if (!pkg || !pkg_dir)
		return -CUI_EPARA;

	if (pkg->pio->read(pkg, &total_index_length, 4))
		return -CUI_EREAD;

	if (total_index_length == 0)
		return -CUI_EMATCH;

	for (i = 0; total_index_length > 0; i++) {
		dat_entry_t index;

		if (pkg->pio->read(pkg, &index.idx_length, 4))
			break;
		if (pkg->pio->read(pkg, &index.unknown, 4))
			break;
		if (pkg->pio->read(pkg, &index.offset, 4))
			break;
		if (pkg->pio->read(pkg, &index.length, 4))
			break;
		if (pkg->pio->read(pkg, index.name, index.idx_length - 16))
			break;
		total_index_length -= index.idx_length;
	}
	if (total_index_length)
		return -CUI_EREAD;

	if (pkg->pio->seek(pkg, 4, IO_SEEK_SET))
		return -CUI_ESEEK;

	pkg_dir->index_entries = i;
	index_buffer_length = pkg_dir->index_entries * sizeof(dat_entry_t);
	index_buffer = (dat_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	for (i = 0; i < pkg_dir->index_entries; i++) {
		dat_entry_t *index = &index_buffer[i];

		if (pkg->pio->read(pkg, &index->idx_length, 4))
			break;
		if (pkg->pio->read(pkg, &index->unknown, 4))
			break;
		if (pkg->pio->read(pkg, &index->offset, 4))
			break;
		if (pkg->pio->read(pkg, &index->length, 4))
			break;
		u32 name_length = index->idx_length - 16;
		if (pkg->pio->read(pkg, index->name, name_length))
			break;
		index->name[name_length] = 0;
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
static int AF_dat_parse_resource_info(struct package *pkg,
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
static int AF_dat_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	pkg_res->raw_data = VirtualAlloc(NULL, (DWORD)pkg_res->raw_data_length, MEM_COMMIT, PAGE_READWRITE);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			VirtualFree(pkg_res->raw_data, 0, MEM_RELEASE);
			pkg_res->raw_data = NULL;
			return -CUI_EREADVEC;
		}

		return 0;
}

/* 资源保存函数 */
static int AF_dat_save_resource(struct resource *res, 
								struct package_resource *pkg_res)
{
	if (!res || !pkg_res)
		return -CUI_EPARA;

	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}
	res->rio->close(res);

	return 0;
}

/* 封包资源释放函数 */
static void AF_dat_release_resource(struct package *pkg, 
									struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->raw_data) {
		VirtualFree(pkg_res->raw_data, 0, MEM_RELEASE);
		pkg_res->raw_data = NULL;
	}
}

/* 封包卸载函数 */
static void AF_dat_release(struct package *pkg, 
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
static cui_ext_operation AF_dat_operation = {
		AF_dat_match,					/* match */
		AF_dat_extract_directory,		/* extract_directory */
		AF_dat_parse_resource_info,		/* parse_resource_info */
		AF_dat_extract_resource,		/* extract_resource */
		AF_dat_save_resource,			/* save_resource */
		AF_dat_release_resource,		/* release_resource */
		AF_dat_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK AF_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &AF_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NO_MAGIC))
		return -1;

	return 0;
}

