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
#include <vector>

using namespace std;
using std::vector;

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information kagura_cui_information = {
	_T(""),		/* copyright */
	_T(""),			/* system */
	_T(".pak"),	/* package */
	_T("1.0.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-7-22 19:03"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];		// "PAK"
	u16 index_info_offset;
	u16 unknown0;		// 6
	u16 resreved0;		// 0
	u16 zero;			// 0
	u32 resreved1;		// 0
} pak_header_t;

typedef struct {	
	u32 index_info_size;
	u32 reserved0;		// 0
	u32 dir_entries;	// 1 ?
	u32 uncomprlen;
	u32 comprlen;
	u32 reserved1;		// 0
} pak_idx_info_t;

typedef struct {	
	u64 offset;			// from 0
	u64 uncomprlen;
	u64 comprlen;
	u32 flags;
	u64 CreateTime;
	u64 LastAccess;
	u64 LastWrite;
	//s8 *name;
} pak_entry_t;
#pragma pack ()

typedef struct {
	char name[MAX_PATH];
	DWORD uncomprlen;
	DWORD comprlen;
	DWORD offset;
} my_pak_entry_t;

static int pak_decompress(Bytef *dest, uLongf destLen, 
						  const Bytef *source, uLong sourceLen)
{
    z_stream stream;
    int err;

	memset(&stream, 0, sizeof(stream));
    err = inflateInit2(&stream, -15);
    if (err != Z_OK)
		return err;
    
	stream.next_in = (Bytef*)source;
    stream.avail_in = (uInt)sourceLen;
    stream.next_out = dest;
    stream.avail_out = (uInt)destLen;

    inflate(&stream, Z_FINISH);

	return destLen - stream.avail_out;
}

static void collect_index_entries(BYTE *&pindex, DWORD index_entries, 
								  vector<my_pak_entry_t> &my_index,
								  char *root_dir)
{
	char parent_dir[MAX_PATH];

	for (DWORD i = 0; i < index_entries; ++i) {
		pak_entry_t *entry = (pak_entry_t *)pindex;
		char *name = (char *)(entry + 1);
		int nlen = strlen(name) + 1;
		pindex += sizeof(pak_entry_t) + nlen;

		if (entry->flags & 0x10) {
			sprintf(parent_dir, "%s\\%s\\", root_dir, name);
			collect_index_entries(pindex, entry->uncomprlen, my_index, parent_dir);
		} else {
			my_pak_entry_t my_entry;

			sprintf(my_entry.name, "%s\\%s", root_dir, name);
			my_entry.uncomprlen = entry->uncomprlen;
			my_entry.comprlen = entry->comprlen;
			my_entry.offset = entry->offset;
			my_index.push_back(my_entry);
		}
	}
}

/********************* pak *********************/

/* 封包匹配回调函数 */
static int kagura_pak_match(struct package *pkg)
{
	pak_header_t header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &header, sizeof(header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(header.magic, "PAK", 4) || header.zero) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int kagura_pak_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	pak_header_t header;
	if (pkg->pio->readvec(pkg, &header, sizeof(header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	pak_idx_info_t info;
	if (pkg->pio->readvec(pkg, &info, sizeof(info), header.index_info_offset, IO_SEEK_SET))
		return -CUI_EREADVEC;

	BYTE *compr = new BYTE[info.comprlen];
	if (!compr)
		return -CUI_EMEM;

	BYTE *uncompr = new BYTE[info.uncomprlen];
	if (!uncompr) {
		delete [] compr;
		return -CUI_EMEM;
	}

	if (pkg->pio->readvec(pkg, compr, info.comprlen, 
			header.index_info_offset + info.index_info_size, IO_SEEK_SET)) {
		delete [] uncompr;
		delete [] compr;
		return -CUI_EREADVEC;
	}

	if (pak_decompress(uncompr, info.uncomprlen, compr, info.comprlen) != info.uncomprlen) {
		delete [] uncompr;
		delete [] compr;
		return -CUI_EUNCOMPR;
	}
	delete [] compr;

	vector<my_pak_entry_t> my_index;
	char parent_dir[MAX_PATH] = ".";
	BYTE *pindex = uncompr;

	collect_index_entries(pindex, info.dir_entries, my_index, parent_dir);
	delete [] uncompr;

	my_pak_entry_t *index = new my_pak_entry_t[my_index.size()];
	if (!index) {
		delete [] index;
		return -CUI_EMEM;
	}

	for (DWORD i = 0; i < my_index.size(); ++i) {
		index[i] = my_index[i];
		index[i].offset += sizeof(pak_header_t) + sizeof(pak_idx_info_t)
			+ info.comprlen;
	}

	pkg_dir->index_entries = my_index.size();
	pkg_dir->directory = index;
	pkg_dir->directory_length = my_index.size() * sizeof(my_pak_entry_t);
	pkg_dir->index_entry_length = sizeof(my_pak_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int kagura_pak_parse_resource_info(struct package *pkg,
										  struct package_resource *pkg_res)
{
	my_pak_entry_t *my_pak_entry;

	my_pak_entry = (my_pak_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_pak_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_pak_entry->comprlen;
	pkg_res->actual_data_length = my_pak_entry->uncomprlen;	/* 数据都是明文 */
	pkg_res->offset = my_pak_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int kagura_pak_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	BYTE *compr = new BYTE[pkg_res->raw_data_length];
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			delete [] compr;
			return -CUI_EREADVEC;
	}

	BYTE *uncompr = new BYTE[pkg_res->actual_data_length];
	if (!uncompr) {
		delete [] compr;
		return -CUI_EMEM;
	}

	if (pak_decompress(uncompr, pkg_res->actual_data_length, 
			compr, pkg_res->raw_data_length) != pkg_res->actual_data_length) {
		delete [] uncompr;
		delete [] compr;
		return -CUI_EUNCOMPR;
	}

	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;

	return 0;
}

/* 资源保存函数 */
static int kagura_pak_save_resource(struct resource *res, 
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
static void kagura_pak_release_resource(struct package *pkg, 
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
static void kagura_pak_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation kagura_pak_operation = {
	kagura_pak_match,				/* match */
	kagura_pak_extract_directory,	/* extract_directory */
	kagura_pak_parse_resource_info,	/* parse_resource_info */
	kagura_pak_extract_resource,	/* extract_resource */
	kagura_pak_save_resource,		/* save_resource */
	kagura_pak_release_resource,	/* release_resource */
	kagura_pak_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK kagura_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".pak"), NULL, 
		NULL, &kagura_pak_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
