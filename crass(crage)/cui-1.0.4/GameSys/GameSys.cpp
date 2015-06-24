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
struct acui_information GameSys_cui_information = {
	NULL,					/* copyright */
	NULL,					/* system */
	_T(".pak"),				/* package */
	_T("1.0.3"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-11-23 12:45"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];		/* "PACK" */
	u32 index_entries;
	u32 is_compressed;
	u32 reserved;
} pak_header_t;

typedef struct {	// 0x4c bytes
	s8 name[64];
	u32 uncomprlen;
	u32 comprlen;
	u32 offset;
} pak_entry_t;
#pragma pack ()


static void lzss_uncompress(BYTE *uncompr, DWORD uncomprlen, BYTE *compr, DWORD comprlen)
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
		if (!(flag & 0x0100))
			flag = compr[curbyte++] | 0xff00;

		if (flag & 1) {
			unsigned char data;

			data = compr[curbyte++];
			uncompr[act_uncomprlen++] = data;

			if (act_uncomprlen == uncomprlen)
				break;

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

			for (i = 0; i < copy_bytes; i++) {	
				unsigned char data;

				data = win[(win_offset + i) & (win_size - 1)];
				uncompr[act_uncomprlen++] = data;

				if (act_uncomprlen == uncomprlen)
					return;

				/* 输出的1字节放入滑动窗口 */
				win[nCurWindowByte++] = data;
				nCurWindowByte &= win_size - 1;	
			}
		}
	}
}

/********************* pak *********************/

/* 封包匹配回调函数 */
static int GameSys_pak_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "PACK", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int GameSys_pak_extract_directory(struct package *pkg,
										 struct package_directory *pkg_dir)
{
	pak_header_t pak_header;
	pak_entry_t *index_buffer;
	DWORD index_buffer_length;	

	if (pkg->pio->readvec(pkg, &pak_header, sizeof(pak_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	index_buffer_length = pak_header.index_entries * sizeof(pak_entry_t);
	index_buffer = (pak_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = pak_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(pak_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;
	package_set_private(pkg, pak_header.is_compressed);

	return 0;
}

/* 封包索引项解析函数 */
static int GameSys_pak_parse_resource_info(struct package *pkg,
										   struct package_resource *pkg_res)
{
	pak_entry_t *pak_entry;

	pak_entry = (pak_entry_t *)pkg_res->actual_index_entry;
	if (pak_entry->name[63])
		strncpy(pkg_res->name, pak_entry->name, 64);
	else
		strcpy(pkg_res->name, pak_entry->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = pak_entry->comprlen;
	pkg_res->actual_data_length = pak_entry->uncomprlen;
	pkg_res->offset = pak_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int GameSys_pak_extract_resource(struct package *pkg,
										struct package_resource *pkg_res)
{
	u32 is_compressed = package_get_private(pkg);
	BYTE *compr, *uncompr;

	compr = (BYTE *)pkg->pio->readvec_only(pkg, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET);
	if (!compr)
		return -CUI_EREADVECONLY;

	BYTE *act_data;
	DWORD act_data_len;
	if (is_compressed) {
		uncompr = (BYTE *)malloc(pkg_res->actual_data_length);
		if (!uncompr)
			return -CUI_EMEM;
	
		lzss_uncompress(uncompr, pkg_res->actual_data_length, compr, pkg_res->raw_data_length);
		act_data = uncompr;
		act_data_len = pkg_res->actual_data_length;
	} else {
		uncompr = NULL;
		//pkg_res->actual_length = 0;
		act_data = compr;
		act_data_len = pkg_res->raw_data_length;
	}

	// for .dow
	if (!strncmp((char *)act_data, "RIFF", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".wav");
	} else if (!strncmp((char *)act_data, "OggS", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".ogg");
	} 

	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;

	return 0;
}

/* 资源保存函数 */
static int GameSys_pak_save_resource(struct resource *res, 
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
static void GameSys_pak_release_resource(struct package *pkg, 
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
static void GameSys_pak_release(struct package *pkg, 
								struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation GameSys_pak_operation = {
	GameSys_pak_match,					/* match */
	GameSys_pak_extract_directory,		/* extract_directory */
	GameSys_pak_parse_resource_info,	/* parse_resource_info */
	GameSys_pak_extract_resource,		/* extract_resource */
	GameSys_pak_save_resource,			/* save_resource */
	GameSys_pak_release_resource,		/* release_resource */
	GameSys_pak_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK GameSys_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".pak"), NULL, 
		NULL, &GameSys_pak_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	return 0;
}
