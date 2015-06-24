#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <zlib.h>
#include <stdio.h>
#include <utility.h>
#include "camellia.h"

/*
找mbs_tolower的算法：

tools.dll:
01043D67    68 4C010000     PUSH 14C
01043D6C    FF15 C8A10501   CALL DWORD PTR DS:[<&MSVCRT.malloc>]     ; msvcrt.malloc
01043D72    8BF0            MOV ESI,EAX
01043D74    83C4 04         ADD ESP,4
01043D77    85F6            TEST ESI,ESI
01043D79    74 41           JE SHORT tools.01043DBC
01043D7B    B9 53000000     MOV ECX,53
01043D80    33C0            XOR EAX,EAX
01043D82    8BFE            MOV EDI,ESI
01043D84    F3:AB           REP STOS DWORD PTR ES:[EDI]
01043D86    8D4C24 0C       LEA ECX,DWORD PTR SS:[ESP+C]
01043D8A    891E            MOV DWORD PTR DS:[ESI],EBX
01043D8C    51              PUSH ECX
01043D8D    E8 6E070000     CALL tools.mbs_tolower
01043D92    8D56 18         LEA EDX,DWORD PTR DS:[ESI+18]
01043D95    8D4424 10       LEA EAX,DWORD PTR SS:[ESP+10]
01043D99    52              PUSH EDX
01043D9A    50              PUSH EAX
01043D9B    68 80000000     PUSH 80 《-- 关键字
01043DA0    E8 9BE30000     CALL tools.01052140
*/

struct acui_information MalieSystem_cui_information = {
	_T("GREEN WOOD"),		/* copyright */
	_T("MalieSystem"),		/* system */
	_T(".lib .mgf .mls"),	/* package */
	_T("3.1.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-6-1 16:54"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};	

#pragma pack (1)
typedef struct {
	s8 magic[4];	// "LIB" or "LIBU"
	u32 version;	// 0x00010000
	u32 index_entries;
	u32 reserved;	// 0
} lib_header_t;

typedef struct {
	s8 name[36];
	u32 length;
	u32 offset;
	u32 reserved;		// 0
} lib_entry_t;

typedef struct {
	WCHAR name[34];
	u32 length;
	u32 offset;
	u32 reserved;		// 0
} libu_entry_t;

typedef struct {
	s8 magic[8];		// "MalieGF"
} mgf_header_t;

typedef struct {
	s8 magic[13];		// "MalieScenario"
} mls_header_t;
#pragma pack ()

static int is_libu, is_encryption;

static unsigned int keyTable[CAMELLIA_TABLE_WORD_LEN];

static void rotate16bytes(u32 offset, DWORD *cipher)
{
	BYTE shifts = ((offset >> 4) & 0x0f) + 16;

	cipher[0] = _lrotl(cipher[0], shifts);
	cipher[1] = _lrotr(cipher[1], shifts);
	cipher[2] = _lrotl(cipher[2], shifts);
	cipher[3] = _lrotr(cipher[3], shifts);
}

static void *memstr(void *_dst, int len, char *src)
{
	BYTE *dst = (BYTE *)_dst;
	for (int i = 0; i < len; ++i) {
		for (int k = 0; src[k]; ++k) {
			if (dst[i + k] != src[k])
				break;
		}
		if (!src[k])
			break;
	}

	return i == len ? NULL : dst + i;
}

static int MalieSystem_readvec(struct package *pkg, void *_data, DWORD data_len, DWORD offset)
{
	if (!data_len)
		return 0;

	BYTE *data = (BYTE *)_data;

	if (!is_encryption)
		return pkg->pio->readvec(pkg, data, data_len, offset, IO_SEEK_SET);

	DWORD aligned = offset & 0xf;
	offset -= aligned;
	DWORD data_offset = (16 - aligned) & 0xf;

	BYTE cipher[16];
	BYTE plain[16];
	if (aligned) {
		if (pkg->pio->readvec(pkg, cipher, sizeof(cipher), offset, IO_SEEK_SET))
			return -CUI_EREADVEC;
		rotate16bytes((unsigned int)pkg->fior->base_offset + offset, (DWORD *)cipher);
		Camellia_DecryptBlock(128, cipher, keyTable, plain);
		DWORD act_data_len = data_len > data_offset ? data_offset : data_len;
		memcpy(data, plain + aligned, act_data_len);
		data_len -= act_data_len;
		if (!data_len)
			return 0;
		offset += 16;
	}

	DWORD cipher_len = data_len & ~0xf;
	BYTE *_cipher = data + data_offset;
	if (pkg->pio->readvec(pkg, _cipher, cipher_len, offset, IO_SEEK_SET))
		return -CUI_EREADVEC;

	cipher_len /= 16;
	for (DWORD i = 0; i < cipher_len; ++i) {
		rotate16bytes((unsigned int)pkg->fior->base_offset + offset, (DWORD *)_cipher);
		Camellia_DecryptBlock(128, _cipher, keyTable, plain);
		memcpy(_cipher, plain, 16);
		_cipher += 16;
		offset += 16;
	}

	aligned = data_len & 0xf;
	if (aligned) {
		if (pkg->pio->readvec(pkg, cipher, 16, offset, IO_SEEK_SET))
			return -CUI_EREADVEC;

		rotate16bytes((unsigned int)pkg->fior->base_offset + offset, (DWORD *)cipher);
		Camellia_DecryptBlock(128, cipher, keyTable, plain);
		memcpy(_cipher, plain, aligned);
	}

	return 0;
}

static void empty_key(BYTE *key)
{
	memset(key, 0, 16);
}

static void mbs_tolower(BYTE *key)
{
	BYTE orig_key[16] = {
		0xfd, 0xfe, 0xff, 0x00,
		0xf9, 0xfa, 0xfb, 0xfc,
		0xf5, 0xf6, 0xf7, 0xf8,
		0xf1, 0xf2, 0xf3, 0xf4,
	};

	for (DWORD i = 0; i < 16; ++i)
		key[i] = -orig_key[i];
}

static void mbs_tolower2(BYTE *key)
{
	BYTE orig_key[16] = {
		0xAC, 0xCA, 0x8B, 0x96, 
		0x9A, 0x8F, 0xC7, 0x99, 
		0xAA, 0xAE, 0x88, 0xAF, 
		0x9D, 0xA9, 0xA8, 0xB6,
	};

	for (DWORD i = 0; i < 16; ++i)
		key[i] = -orig_key[i];
}

static void mbs_tolower3(BYTE *key)
{
	BYTE orig_key[16] = {
		0xAF, 0xB3, 0xCA, 0xCB, 
		0x8A, 0x92, 0xB9, 0x8B, 
		0xBE, 0xB5, 0x91, 0x9D, 
		0xBA, 0xAB, 0xB4, 0xB8,
	};

	for (DWORD i = 0; i < 16; ++i)
		key[i] = -orig_key[i];
}

static void mbs_tolower4(BYTE *key)
{
	BYTE orig_key[16] = {
		0xB2, 0x95, 0xCC, 0xAA, 
		0xBD, 0xBB, 0xC9, 0x86, 
		0xA8, 0xAC, 0xCD, 0xD0, 
		0x8A, 0xB1, 0xA6, 0xB4
	};

	for (DWORD i = 0; i < 16; ++i)
		key[i] = -orig_key[i];
}

static void mbs_tolower5(BYTE *key)
{
	BYTE orig_key[16] = {
		0xAF, 0xB3, 0xCA, 0xCB, 
		0x8A, 0x92, 0xB9, 0x8B, 
		0xBE, 0xB5, 0x91, 0x9D, 
		0xBA, 0xAB, 0xB4, 0xB8
	};

	for (DWORD i = 0; i < 16; ++i)
		key[i] = -orig_key[i];
}

static void mbs_tolower6(BYTE *key)
{
	BYTE orig_key[16] = {
		0x8F, 0x9D, 0xB9, 0x9E, 
		0x94, 0xAF, 0xAD, 0xBB, 
		0x9C, 0xA8, 0x8B, 0x92, 
		0x9F, 0xB3, 0x97, 0xBA
	};

	for (DWORD i = 0; i < 16; ++i)
		key[i] = -orig_key[i];
}

static void (*genkey_function[])(BYTE *) = {
	mbs_tolower6,
	mbs_tolower5,
	mbs_tolower4,
	mbs_tolower3,
	mbs_tolower2,
	mbs_tolower,
	empty_key,
	NULL,
};


/********************* lib *********************/

static int MalieSystem_lib_match(struct package *pkg)
{
	lib_header_t lib_header;
	lib_header_t *plain_lib_header;	

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &lib_header, sizeof(lib_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}
	plain_lib_header = &lib_header;

	if (!memcmp(lib_header.magic, "LIB", 4)) {
		is_libu = 0;
		is_encryption = 0;
	} else if (!memcmp(lib_header.magic, "LIBU", 4)) {
		is_libu = 1;
		is_encryption = 0;	
	} else {
		BYTE plain[32], cipher[32];

		if (pkg->pio->readvec(pkg, cipher, sizeof(cipher), 0, IO_SEEK_SET)) {
			pkg->pio->close(pkg);
			return -CUI_EREADVEC;
		}

		if (!pkg->fior->base_offset) {
			void (**func)(BYTE *);
			for (func = genkey_function; *func; ++func) {
				unsigned char rawKey[16];
				BYTE _cipher[16];
				(*func)(rawKey);
				memset(keyTable, 0, sizeof(keyTable));
				Camellia_Ekeygen(128, rawKey, keyTable);
				memcpy(_cipher, cipher, 16);
				rotate16bytes(0, (DWORD *)_cipher);
				Camellia_DecryptBlock(128, _cipher, keyTable, plain);
	
				// 因为"LIB"的索引项不是16字节对齐,所以只有LIBU型才可能有加密
				if (!memcmp(plain, "LIBU", 4)) {		
					is_libu = 1;
					is_encryption = 1;
					break;
				}
			}
			if (!*func) {
				pkg->pio->close(pkg);
				return -CUI_EMATCH;
			}
		} else {
			rotate16bytes((unsigned int)pkg->fior->base_offset, (DWORD *)cipher);
			Camellia_DecryptBlock(128, cipher, keyTable, plain);
			rotate16bytes((unsigned int)pkg->fior->base_offset + 16, (DWORD *)(cipher + 16));
			Camellia_DecryptBlock(128, cipher + 16, keyTable, plain + 16);

			// 因为"LIB"的索引项不是16字节对齐,所以只有LIBU型才可能有加密
			plain_lib_header = (lib_header_t *)memstr(plain, sizeof(plain), "LIBU");
			if (plain_lib_header) {		
				is_libu = 1;
				is_encryption = 1;
			} else {
				pkg->pio->close(pkg);
				return -CUI_EMATCH;	
			}
		}
	}
	if (!plain_lib_header->index_entries) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int MalieSystem_lib_extract_directory(struct package *pkg,
											 struct package_directory *pkg_dir)
{
	BYTE cipher[32];
	lib_header_t *lib_header;

	if (MalieSystem_readvec(pkg, cipher, sizeof(cipher), 0))
		return -CUI_EREADVEC;

	if (is_encryption) {
		BYTE *p = cipher;
		while (p = (BYTE *)memstr(p, sizeof(cipher), "LIBU")) {
			lib_header = (lib_header_t *)p;
			p += 4;
		}
	} else
		lib_header = (lib_header_t *)cipher;

	DWORD entry_size;
	if (is_libu)
		entry_size = sizeof(libu_entry_t);
	else
		entry_size = sizeof(lib_entry_t);

	pkg_dir->index_entries = lib_header->index_entries;
	DWORD index_buffer_length = pkg_dir->index_entries * entry_size; 
	BYTE *index_buffer = new BYTE[index_buffer_length];
	if (!index_buffer)
		return -CUI_EMEM;

	DWORD offset_base = (BYTE *)lib_header - cipher;
	DWORD offset = offset_base + sizeof(lib_header_t);
	if (MalieSystem_readvec(pkg, index_buffer, index_buffer_length, offset)) {
		delete [] index_buffer;
		return -CUI_EREAD;
	}

	if (is_libu) {
		libu_entry_t *libu_entry = (libu_entry_t *)index_buffer;
		for (DWORD i = 0; i < pkg_dir->index_entries; ++i)
			libu_entry[i].offset += offset_base;
	} else {
		lib_entry_t *lib_entry = (lib_entry_t *)index_buffer;
		for (DWORD i = 0; i < pkg_dir->index_entries; ++i)
			lib_entry[i].offset += offset_base;
	}

	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = entry_size;
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

static int MalieSystem_lib_parse_resource_info(struct package *pkg,
											   struct package_resource *pkg_res)
{
	if (is_libu) {
		libu_entry_t *libu_entry = (libu_entry_t *)pkg_res->actual_index_entry;
		unicode2acp(pkg_res->name, MAX_PATH, libu_entry->name, -1);
		pkg_res->raw_data_length = libu_entry->length;
		pkg_res->offset = libu_entry->offset;
	} else {
		lib_entry_t *lib_entry = (lib_entry_t *)pkg_res->actual_index_entry;
		strcpy(pkg_res->name, lib_entry->name);
		pkg_res->raw_data_length = lib_entry->length;
		pkg_res->offset = lib_entry->offset;
	}
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */	
	pkg_res->actual_data_length = 0;

	return 0;
}

static int MalieSystem_lib_extract_resource(struct package *pkg,
											struct package_resource *pkg_res)
{
	s8 magic[4];

	if (MalieSystem_readvec(pkg, magic, 4, pkg_res->offset))
		return -CUI_EREADVEC;

	if ((!memcmp(magic, "LIB", 4) || !memcmp(magic, "LIBU", 4))
			&& (pkg_res->raw_data_length > sizeof(lib_header_t))) {
		pkg_res->flags |= PKG_RES_FLAG_FIO | PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".lib");
		if (is_encryption) {
			DWORD aligned = pkg_res->offset & 0xf;
			pkg_res->offset -= aligned;
			pkg_res->raw_data_length += aligned;
			pkg_res->raw_data_length = (pkg_res->raw_data_length + 15) & ~0xf;
		}
		return 0;
	}

	BYTE *raw = new BYTE[pkg_res->raw_data_length];
	if (!raw)
		return -CUI_EMEM;

	if (MalieSystem_readvec(pkg, raw, pkg_res->raw_data_length, pkg_res->offset)) {
		delete [] raw;
		return -CUI_EREADVEC;
	}

	if (!memcmp(raw, "OggS", 4)) {
		pkg_res->replace_extension = _T(".ogg");
		pkg_res->flags = PKG_RES_FLAG_REEXT;
	}
	pkg_res->raw_data = raw;

	return 0;
}

static int MalieSystem_lib_save_resource(struct resource *res, 
										 struct package_resource *pkg_res)
{
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}

	res->rio->close(res);
	
	return 0;
}

static void MalieSystem_lib_release_resource(struct package *pkg, 
											 struct package_resource *pkg_res)
{
	if (pkg_res->raw_data) {
		delete [] pkg_res->raw_data;
		pkg_res->raw_data = NULL;
	}
}

static void MalieSystem_lib_release(struct package *pkg, 
									struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;		
	}
	pkg->pio->close(pkg);
}

static cui_ext_operation MalieSystem_lib_operation = {
	MalieSystem_lib_match,				/* match */
	MalieSystem_lib_extract_directory,	/* extract_directory */
	MalieSystem_lib_parse_resource_info,/* parse_resource_info */
	MalieSystem_lib_extract_resource,	/* extract_resource */
	MalieSystem_lib_save_resource,		/* save_resource */
	MalieSystem_lib_release_resource,	/* release_resource */
	MalieSystem_lib_release				/* release */
};

/********************* mgf *********************/

static int MalieSystem_mgf_match(struct package *pkg)
{
	mgf_header_t mgf;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_ECREATE;

	if (pkg->pio->read(pkg, mgf.magic, sizeof(mgf.magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(mgf.magic, "MalieGF", sizeof(mgf.magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int MalieSystem_mgf_extract_resource(struct package *pkg,
											struct package_resource *pkg_res)
{
	const s8 png_header[8] = { '\x89', 'P', 'N', 'G', '\x0d',
		'\x0a', '\x1a', '\x0a' };

	pkg_res->raw_data = malloc((unsigned int)pkg_res->raw_data_length);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, pkg_res->raw_data_length,
			pkg_res->offset, IO_SEEK_SET)) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
		return -CUI_EREADVEC;
	}
	memcpy(pkg_res->raw_data, png_header, 8);

	return 0;
}

static int MalieSystem_mgf_save_resource(struct resource *res, 
									struct package_resource *pkg_res)
{
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}
	res->rio->close(res);
	
	return 0;
}

static void MalieSystem_mgf_release_resource(struct package *pkg, 
											 struct package_resource *pkg_res)
{
	if (pkg_res && pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void MalieSystem_mgf_release(struct package *pkg, 
									struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

static cui_ext_operation MalieSystem_mgf_operation = {
	MalieSystem_mgf_match,					/* match */
	NULL,									/* extract_directory */
	NULL,
	MalieSystem_mgf_extract_resource,		/* extract_resource */
	MalieSystem_mgf_save_resource,			/* save_resource */
	MalieSystem_mgf_release_resource,		/* release_resource */
	MalieSystem_mgf_release					/* release */
};

/********************* mls *********************/

static int MalieSystem_mls_match(struct package *pkg)
{
	mls_header_t mls;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, mls.magic, sizeof(mls.magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(mls.magic, "MalieScenario", sizeof(mls.magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int MalieSystem_mls_extract_resource(struct package *pkg,
											struct package_resource *pkg_res)
{
	unsigned int uncomprlen, comprlen;
	unsigned char *uncompr, *compr;

	comprlen = (unsigned int)(pkg_res->raw_data_length - sizeof(mls_header_t));
	compr = (unsigned char *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, sizeof(mls_header_t), 
			IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	int retry = 0;
	uncomprlen = comprlen;
	unsigned long act_uncomprlen;
	while (1) {
		uncomprlen <<= 1;
		uncompr = (unsigned char *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}

		act_uncomprlen = uncomprlen;
		if (uncompress(uncompr, &act_uncomprlen, compr, comprlen) == Z_OK)
			break;
		free(uncompr);
	}
	free(compr);
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = act_uncomprlen;

	return 0;
}

static int MalieSystem_mls_save_resource(struct resource *res,
										 struct package_resource *pkg_res)
{
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (res->rio->write(res, pkg_res->actual_data, pkg_res->actual_data_length)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}
	res->rio->close(res);
	
	return 0;
}

static void MalieSystem_mls_release_resource(struct package *pkg, 
											 struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
}

static void MalieSystem_mls_release(struct package *pkg, 
									struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

static cui_ext_operation MalieSystem_mls_operation = {
	MalieSystem_mls_match,					/* match */
	NULL,									/* extract_directory */
	NULL,
	MalieSystem_mls_extract_resource,		/* extract_resource */
	MalieSystem_mls_save_resource,			/* save_resource */
	MalieSystem_mls_release_resource,		/* release_resource */
	MalieSystem_mls_release					/* release */
};

int CALLBACK MalieSystem_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".lib"), NULL, 
		NULL, &MalieSystem_lib_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES 
		| CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_RECURSION))
			return -1;

	if (callback->add_extension(callback->cui, _T(".mgf"), _T(".png"), 
		NULL, &MalieSystem_mgf_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".mls"), _T(".txt"), 
		NULL, &MalieSystem_mls_operation, CUI_EXT_FLAG_PKG))
			return -1;

	return 0;
}
