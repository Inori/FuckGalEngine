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
struct acui_information TonyAlbum_cui_information = {
	_T(""),					/* copyright */
	_T(""),					/* system */
	_T(".arc"),				/* package */
	_T("1.0.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-7-20 12:08"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	u32 index_entries;
} arc_header_t;

typedef struct {
	s8 name[32];
	u32 uncomprlen;
	u32 comprlen;
	u32 offset;
} arc_entry_t;
#pragma pack ()

static void decode(BYTE *data, DWORD len)
{
	const char *key = "Tony";

	for (DWORD i = 0; i < len; ++i)
		data[i] += key[i & 3];
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

	memset(win, 0, nCurWindowByte);
	while (1) {
		flag >>= 1;
		if (!(flag & 0x0100)) {
			flag = compr[curbyte++] | 0xff00;
			if (curbyte == comprlen)
				break;
		}

		if (flag & 1) {
			unsigned char data = compr[curbyte++];
			if (curbyte == comprlen)
				break;
			uncompr[act_uncomprlen++] = data;
			/* 输出的1字节放入滑动窗口 */
			win[nCurWindowByte++] = data;
			nCurWindowByte &= win_size - 1;
		} else {
			unsigned int copy_bytes, win_offset;
			unsigned int i;

			win_offset = compr[curbyte++];
			if (curbyte == comprlen)
				break;

			copy_bytes = compr[curbyte++];
			if (curbyte == comprlen)
				break;

			win_offset |= (copy_bytes & 0xf0) << 4;
			copy_bytes &= 0x0f;
			copy_bytes += 3;

			for (i = 0; i < copy_bytes; i++) {	
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

/********************* arc *********************/

/* 封包匹配回调函数 */
static int TonyAlbum_arc_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	u32 index_entries;

	if (pkg->pio->read(pkg, &index_entries, sizeof(index_entries))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if ((s32)index_entries <= 0) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	arc_entry_t entry;
	if (pkg->pio->read(pkg, &entry, sizeof(entry))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	decode((BYTE *)&entry, sizeof(entry));

	if (entry.offset != sizeof(arc_header_t) + index_entries * sizeof(arc_entry_t)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int TonyAlbum_arc_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{
	u32 index_entries;
	if (pkg->pio->readvec(pkg, &index_entries, 4, 0, IO_SEEK_SET))
		return -CUI_EREADVEC;
	
	arc_entry_t *index = new arc_entry_t[index_entries];
	if (!index)
		return -CUI_EMEM;

	DWORD index_length = index_entries * sizeof(arc_entry_t);
	if (pkg->pio->read(pkg, index, index_length)) {
		delete [] index;
		return -CUI_EREAD;
	}

	decode((BYTE *)index, index_length);

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index;
	pkg_dir->directory_length = index_length;
	pkg_dir->index_entry_length = sizeof(arc_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int TonyAlbum_arc_parse_resource_info(struct package *pkg,
											 struct package_resource *pkg_res)
{
	arc_entry_t *arc_entry;

	arc_entry = (arc_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, arc_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = arc_entry->comprlen;
	pkg_res->actual_data_length = arc_entry->uncomprlen;	/* 数据都是明文 */
	pkg_res->offset = arc_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int TonyAlbum_arc_extract_resource(struct package *pkg,
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

	lzss_uncompress(uncompr, pkg_res->actual_data_length, compr, pkg_res->raw_data_length);

	pkg_res->actual_data = uncompr;
	pkg_res->raw_data = compr;

	return 0;
}

/* 资源保存函数 */
static int TonyAlbum_arc_save_resource(struct resource *res, 
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
static void TonyAlbum_arc_release_resource(struct package *pkg, 
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
static void TonyAlbum_arc_release(struct package *pkg, 
								  struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation TonyAlbum_arc_operation = {
	TonyAlbum_arc_match,				/* match */
	TonyAlbum_arc_extract_directory,	/* extract_directory */
	TonyAlbum_arc_parse_resource_info,	/* parse_resource_info */
	TonyAlbum_arc_extract_resource,		/* extract_resource */
	TonyAlbum_arc_save_resource,		/* save_resource */
	TonyAlbum_arc_release_resource,		/* release_resource */
	TonyAlbum_arc_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK TonyAlbum_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".arc"), NULL, 
		NULL, &TonyAlbum_arc_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
