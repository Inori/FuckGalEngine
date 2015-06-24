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

// M:\CLOVER\MOLDAVITE
// N:\性奴隷未亡人

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information Game_Script_Player_cui_information = {
	_T("HyperWorks"),		/* copyright */
	_T("Game Script Player for Win32"),			/* system */
	_T(".pak"),		/* package */
	_T(""),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T(""),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];		// "pac2" or "pack" or "PACK"
	u32 index_length;
} pak_header_t;
#pragma pack ()

/* .dat封包的索引项结构 */
typedef struct {
	s8 name[256];
	u32 name_length;
	u32 offset;
	u32 length;
} dat_entry_t;

static int is_encrypt;

static BYTE decript_table[32] = {
	0xa0, 0x25, 0x53, 0x30, 0x8c, 0x30, 0x6f, 0x30,
	0x42, 0x57, 0x2c, 0x67, 0x43, 0x53, 0x0b, 0x5c,
	0x6e, 0x30, 0x57, 0x84, 0x5c, 0x4f, 0x69, 0x72,
	0x67, 0x30, 0x42, 0x30, 0x8b, 0x30, 0x00, 0x00
};

// 输入的data_len必须4字节对齐, offset不能超过32
static void decrypt(BYTE *data, int data_len, int offset)
{

	if (data_len <= 0)
		return;

	while (data_len > 0) {
		int dec_len = 32 - offset;
		int *dec = (int *)&decript_table[offset];
		if (data_len < 32 - offset)
			dec_len = data_len;
		offset = 0;
		data_len -= dec_len;
		for (int i = 0; i < (((dec_len + 3) & ~3) / 4); ++i) {
			*(int *)data ^= *dec++;
			data += 4;	
		}
	}
}

/********************* pak *********************/

/* 封包匹配回调函数 */
static int Game_Script_Player_pak_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (!strncmp(magic, "pac2", 4))
		is_encrypt = 1;
	else if (!strncmp(magic, "pack", 4) || !strncmp(magic, "PACK", 4))
		is_encrypt = 0;
	else {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int Game_Script_Player_pak_extract_directory(struct package *pkg,
													struct package_directory *pkg_dir)
{
	u32 index_length;	

	if (pkg->pio->read(pkg, &index_length, 4))
		return -CUI_EREAD;

	BYTE *index = new BYTE[index_length];
	if (!index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index, index_length)) {
		delete [] index;
		return -CUI_EREAD;
	}

	if (is_encrypt)
		decrypt(index, index_length, 0);

	BYTE *p = index;
	for (DWORD i = 0; ; ++i) {
		p += 8 + ((8 + 1 + p[8]) & ~7);
		if (!*(u32 *)p)
			break;
	}

	pkg_dir->index_entries = i;
	pkg_dir->directory = index;
	pkg_dir->directory_length = index_length;
	pkg_dir->flags = PKG_DIR_FLAG_VARLEN;

	return 0;
}

/* 封包索引项解析函数 */
static int Game_Script_Player_pak_parse_resource_info(struct package *pkg,
									struct package_resource *pkg_res)
{
	BYTE *entry = (BYTE *)pkg_res->actual_index_entry;
	BYTE nlen = entry[8];
	pkg_res->actual_index_entry_length = 8 + ((8 + 1 + nlen) & ~7);
	strncpy(pkg_res->name, (char *)entry + 9, nlen);
	pkg_res->name_length = nlen;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = *(u32 *)&entry[4];
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = *(u32 *)entry;

	return 0;
}

/* 封包资源提取函数 */
static int Game_Script_Player_pak_extract_resource(struct package *pkg,
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

/* 资源保存函数 */
static int Game_Script_Player_pak_save_resource(struct resource *res, 
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
static void Game_Script_Player_pak_release_resource(struct package *pkg, 
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
static void Game_Script_Player_pak_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation Game_Script_Player_pak_operation = {
	Game_Script_Player_pak_match,					/* match */
	Game_Script_Player_pak_extract_directory,		/* extract_directory */
	Game_Script_Player_pak_parse_resource_info,		/* parse_resource_info */
	Game_Script_Player_pak_extract_resource,		/* extract_resource */
	Game_Script_Player_pak_save_resource,			/* save_resource */
	Game_Script_Player_pak_release_resource,		/* release_resource */
	Game_Script_Player_pak_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK Game_Script_Player_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".pak"), NULL, 
		NULL, &Game_Script_Player_pak_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
