#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>
#include <bzlib.h>
#include <vector>
#include "blowfish.h"

using namespace std;

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information Lua_cui_information = {
	_T("PUC-Rio"),			/* copyright */
	_T("Lua"),				/* system */
	_T(".pck"),				/* package */
	_T("1.0.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-5-28 19:52"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[12];		// "PACK_FILE001"
	u32 index_entries;
	u32 index_length;	// headsize
} pck_header_t;
#pragma pack ()

typedef struct {
	char name[MAX_PATH];
	DWORD offset;
	DWORD comprlen;
	DWORD uncomprlen;
} pck_entry_t;

static BYTE blowfish_key[] = "\x9F\xC0\xC1\x9F\x91";

static void blowfish_key_init(void)
{
	for (DWORD i = 0; i < 5; i++)
		blowfish_key[i] ^= 0xf0;
}

static void pck_decrypt(DWORD *cipher, DWORD cipher_length)
{
	BLOWFISH_CTX ctx;
	DWORD i;
	
	Blowfish_Init(&ctx, blowfish_key, 5);
	for (i = 0; i < cipher_length / 8; i++)
		Blowfish_Decrypt(&ctx, &cipher[i * 2 + 0], &cipher[i * 2 + 1]);
}

static void pck_decrypt2(DWORD *cipher, DWORD cipher_length, 
						 BYTE *key, DWORD key_length)
{
	BLOWFISH_CTX ctx;
	DWORD i;
	
	Blowfish_Init(&ctx, key, key_length);
	for (i = 0; i < cipher_length / 8; i++)
		Blowfish_Decrypt(&ctx, &cipher[i * 2 + 0], &cipher[i * 2 + 1]);
}

/********************* pck *********************/

/* 封包匹配回调函数 */
static int Lua_pck_match(struct package *pkg)
{
	s8 magic[12];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "PACK_FILE001", 12)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int Lua_pck_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{
	pck_header_t pck_header;
	if (pkg->pio->readvec(pkg, &pck_header, sizeof(pck_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	BYTE *index = new BYTE[pck_header.index_length];
	if (!index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index, pck_header.index_length)) {
		delete [] index;
		return -CUI_EREAD;
	}

	pck_decrypt((DWORD *)index, pck_header.index_length);

	pck_entry_t *pck_index_buffer = new pck_entry_t[pck_header.index_entries];
	if (!pck_index_buffer)
		return -CUI_EMEM;

	BYTE *p = index;
	DWORD offset = sizeof(pck_header_t) + pck_header.index_length;
	for (DWORD i = 0; i < pck_header.index_entries; ++i) {
		pck_entry_t &entry = pck_index_buffer[i];
		entry.uncomprlen = *(u32 *)p;
		p += 4;
		strcpy(entry.name, (char *)p);		
		p += strlen((char *)p) + 1;
		entry.comprlen = *(u32 *)p;
		p += 4;
		entry.offset = offset;
		offset += entry.comprlen + 1;
	}
	delete [] index;	

	pkg_dir->index_entries = pck_header.index_entries;
	pkg_dir->directory = pck_index_buffer;
	pkg_dir->directory_length = pck_header.index_entries * sizeof(pck_entry_t);
	pkg_dir->index_entry_length = sizeof(pck_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int Lua_pck_parse_resource_info(struct package *pkg,
									   struct package_resource *pkg_res)
{
	pck_entry_t *pck_entry;

	pck_entry = (pck_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, pck_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = pck_entry->comprlen;
	pkg_res->actual_data_length = pck_entry->uncomprlen;
	pkg_res->offset = pck_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int Lua_pck_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	u8 filetype;
	if (pkg->pio->readvec(pkg, &filetype, 1, pkg_res->offset, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD comprlen = pkg_res->raw_data_length;
	BYTE *compr = new BYTE[comprlen];
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen,	pkg_res->offset + 1, IO_SEEK_SET)) {
		delete [] compr;
		return -CUI_EREADVEC;
	}

	/*
	 * 0 - music (.wav, .ogg, .mp3)
	 * 2 - image (.png, .dds)
	 * 3 - script (.lua, .txt)
	 */
	if (filetype) {
		pck_decrypt((DWORD *)compr, comprlen);

		if (filetype == 3) {	// 3 - script (.lua, .txt)
			DWORD uncomprlen = pkg_res->actual_data_length;
			BYTE *uncompr = new BYTE[uncomprlen];
			if (!uncompr) {
				delete [] compr;
				return -CUI_EMEM;			
			}
			unsigned int act_uncomprlen = uncomprlen;
			if (BZ2_bzBuffToBuffDecompress((char *)uncompr, &act_uncomprlen,
					(char *)compr, comprlen, 0, 0) != BZ_OK) {
				delete [] uncompr;
				delete [] compr;
				return -CUI_EUNCOMPR;
			}
			pkg_res->actual_data = uncompr;
			pkg_res->actual_data_length = act_uncomprlen;
		}
	}
	pkg_res->raw_data = compr;

	return 0;
}

/* 资源保存函数 */
static int Lua_pck_save_resource(struct resource *res, 
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
static void Lua_pck_release_resource(struct package *pkg, 
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
static void Lua_pck_release(struct package *pkg, 
							struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation Lua_pck_operation = {
	Lua_pck_match,					/* match */
	Lua_pck_extract_directory,		/* extract_directory */
	Lua_pck_parse_resource_info,	/* parse_resource_info */
	Lua_pck_extract_resource,		/* extract_resource */
	Lua_pck_save_resource,			/* save_resource */
	Lua_pck_release_resource,		/* release_resource */
	Lua_pck_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK Lua_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".pck"), NULL, 
		NULL, &Lua_pck_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	blowfish_key_init();

	return 0;
}
