#include <windows.h>
#include <tchar.h>
#include <Wincrypt.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <zlib.h>
#include <stdio.h>

struct acui_information ego_cui_information = {
	_T("Studio e.go!"),		/* copyright */
	NULL,					/* system */
	_T(".dat"),				/* package */
	_T("1.1.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-3-26 20:24"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE | ACUI_ATTRIBUTE_PRELOAD 
};	

#pragma pack (1)
typedef struct {
	s8 magic[4];		// "PAK0"
	u32 data_offset;
	u32 dir_numbers;	// 包括根目录
	u32 index_entries;
} dat_header_t;

typedef struct {
	u32 parent_dir;		// 所属的dir。-1表示没有父目录（因为根目录没有父目录)
	u32 last_file;		// 该目录下最后1个文件的索引号(基于1)
} dat_dir_entry_t;

typedef struct {
	u32 offset;
	u32 length;
	u32 flags;
	u32 time;			// time_t
} dat_entry_t;

typedef struct {
	s8 magic[4];		// "ECC "
	u32 length;
	u32 mode;
	u32 key;
} ecc_header_t;

typedef struct {
	s8 magic[4];		// "EDB2"
	u32 length;
	u32 mode;
	u32 key;
} edb2_header_t;

typedef struct {	// 接在后面的是hash_length长的cipher hash值，然后是cipher数据
	s8 maigc[4];	// "TUTA"
	u32 data_length;
	u32 hash_length;
	u32 cipher_length;
} tuta_header_t;

typedef struct {
	s8 maigc[4];	// "FLD0"
	u32 data_length;
} fld0_header_t;

typedef struct {	// JumpFile
	s8 maigc[4];	// "SCR "
	u32 version;
	u32 mode;
	u32 key;
	u32 cipher_length;
} scr_header_t;
#pragma pack ()

typedef struct {
	char name[MAX_PATH];
	DWORD offset;
	DWORD length;
	DWORD flags;	
} my_dat_entry_t;


static char *dat_get_subdir_name(char *pname, DWORD idx, BYTE *ret_name_len)
{
	BYTE name_len;

	for (DWORD d = 0; d < idx; d++) {
		name_len = *pname++;
		pname += name_len;
	}
	name_len = *pname;
	if (ret_name_len)
		*ret_name_len = name_len;
	return pname;
}

static void dat_get_dir_name(char *name_index, dat_dir_entry_t *dir_index, DWORD dir, char *ret_dir_name)
{
	DWORD dir_idx[256];
	DWORD idx = 0;
	
	/* 遍历到根目录 */
	while (dir_index[dir].parent_dir != -1) {
		dir_idx[idx++] = dir;
		dir = dir_index[dir].parent_dir;
	}

	for (int i = idx - 1; i >= 0; i--) {
		char *sub_dir_name;
		BYTE sub_dir_name_len;
		
		sub_dir_name = dat_get_subdir_name(name_index, dir_idx[i] - 1, &sub_dir_name_len) + 1;
		strncat(ret_dir_name, sub_dir_name, sub_dir_name_len);
		strcat(ret_dir_name, "/");
	}
}

/********************* dat *********************/

static int ego_dat_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "PAK0", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int ego_dat_extract_directory(struct package *pkg,
											 struct package_directory *pkg_dir)
{
	dat_header_t dat_header;
	dat_dir_entry_t *dir_index;
	dat_entry_t *entry_index;
	char *name_index;
	my_dat_entry_t *my_index_buffer;
	DWORD dir_index_length, entry_index_length, name_index_length, my_index_buffer_length;

	if (pkg->pio->readvec(pkg, &dat_header, sizeof(dat_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;
	
	dir_index_length = dat_header.dir_numbers * sizeof(dat_dir_entry_t);
	dir_index = (dat_dir_entry_t *)malloc(dir_index_length);
	if (!dir_index)
		return -CUI_EMEM;
	
	if (pkg->pio->read(pkg, dir_index, dir_index_length)) {
		free(dir_index);
		return -CUI_EREAD;
	}
	
	entry_index_length = dat_header.index_entries * sizeof(dat_entry_t);
	entry_index = (dat_entry_t *)malloc(entry_index_length);
	if (!entry_index) {
		free(dir_index);
		return -CUI_EMEM;
	}

	if (pkg->pio->read(pkg, entry_index, entry_index_length)) {
		free(entry_index);
		free(dir_index);
		return -CUI_EREAD;
	}

	name_index_length = dat_header.data_offset - sizeof(dat_header_t) - dir_index_length - entry_index_length;
	name_index = (char *)malloc(name_index_length);
	if (!name_index) {
		free(entry_index);
		free(dir_index);
		return -CUI_EMEM;
	}

	if (pkg->pio->read(pkg, name_index, name_index_length)) {
		free(name_index);
		free(entry_index);
		free(dir_index);
		return -CUI_EREAD;
	}

	my_index_buffer_length = dat_header.index_entries * sizeof(my_dat_entry_t);
	my_index_buffer = (my_dat_entry_t *)malloc(my_index_buffer_length);
	if (!my_index_buffer) {
		free(name_index);
		free(entry_index);
		free(dir_index);
		return -CUI_EMEM;
	}

	char *pename = dat_get_subdir_name(name_index, dat_header.dir_numbers - 1, NULL);
	DWORD idx = 0;
	for (DWORD d = 0; d < dat_header.dir_numbers; d++) {
		char parent_dir_name[256];
	
		if (dir_index[d].parent_dir == -1)
			continue;

		memset(parent_dir_name, 0, sizeof(parent_dir_name));
		dat_get_dir_name(name_index, dir_index, d, parent_dir_name);

		for (DWORD i = idx; i < dir_index[d].last_file; i++) {
			BYTE name_len;
			
			memset(my_index_buffer[i].name, 0, sizeof(my_index_buffer[i].name));
			strcpy(my_index_buffer[i].name, parent_dir_name);	
			name_len = *pename++;
			strncat(my_index_buffer[i].name, pename, name_len);
			pename += name_len;
			my_index_buffer[i].length = entry_index[i].length;
			my_index_buffer[i].offset = entry_index[i].offset;
			my_index_buffer[i].flags = entry_index[i].flags;
		}
		idx = dir_index[d].last_file;
	}
	free(name_index);
	free(entry_index);
	free(dir_index);
	
	pkg_dir->index_entries = dat_header.index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = my_index_buffer_length;
	pkg_dir->index_entry_length = sizeof(my_dat_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

static int ego_dat_parse_resource_info(struct package *pkg,
											   struct package_resource *pkg_res)
{
	my_dat_entry_t *my_dat_entry;

	my_dat_entry = (my_dat_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_dat_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_dat_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_dat_entry->offset;

	return 0;
}

static int ego_dat_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen,	pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREAD;
	}

	uncompr = NULL;
	uncomprlen = 0;

	if (!strncmp((char *)compr, "ECC ", 4)) {
		ecc_header_t *ecc_header = (ecc_header_t *)compr;
		DWORD cipher_len = comprlen - sizeof(ecc_header_t);
		DWORD loop = cipher_len / 4;
		DWORD key = ecc_header->key;
		DWORD *cipher = (DWORD *)(ecc_header + 1);
		DWORD i;

		switch (ecc_header->mode) {
		case 0:	/* 明文 */
			break;
		case 1:
			for (i = 0; i < loop; i++) {
				if (!(i & 0xff))
					key = !!key;
				key += 0x7654321;
				cipher[i] ^= key;
			}
			break;
		case 2:
			for (i = 0; i < loop; i++) {
				if (!(i & 0xff))
					key = ~key;
				key += 0x7654321;
				cipher[i] ^= key;
			}
			break;
		case 3:
			for (i = 0; i < loop; i++) {
				if (!(i & 0xff))
					key = ~key;
				key += cipher_len;
				cipher[i] ^= key;
			}
			break;
		}
	} else if (!strncmp((char *)compr, "EDB2", 4)) {
		edb2_header_t *ecc_header = (edb2_header_t *)compr;
		DWORD cipher_len = ecc_header->length;
		DWORD loop = cipher_len / 4;
		DWORD key = ecc_header->key;
		DWORD *cipher = (DWORD *)(ecc_header + 1);
		DWORD i;

		switch (ecc_header->mode) {
		case 0:	/* 明文 */
			break;
		case 1:
			for (i = 0; i < loop; i++) {
				if (!(i & 0xff))
					key = !!key;
				key += 0x7654321;
				cipher[i] ^= key;
			}
			break;
		case 2:
			for (i = 0; i < loop; i++) {
				if (!(i & 0xff))
					key = ~key;
				key += 0x7654321;
				cipher[i] ^= key;
			}
			break;
		case 3:
			for (i = 0; i < loop; i++) {
				if (!(i & 0xff))
					key = ~key;
				key += cipher_len;
				cipher[i] ^= key;
			}
			break;
		default:
			free(compr);
			return -CUI_EMATCH;
		}
	} if (!strncmp((char *)compr, "TSCR", 4)) {
		ecc_header_t *ecc_header = (ecc_header_t *)compr;
		DWORD cipher_len = comprlen - sizeof(ecc_header_t);
		DWORD loop = cipher_len / 4;
		DWORD key = ecc_header->key;
		DWORD *cipher = (DWORD *)(ecc_header + 1);
		DWORD i;

		switch (ecc_header->mode) {
		case 0:	/* 明文 */
		default:
			break;
		case 1:
			for (i = 0; i < loop; i++) {
				if (!(i & 0xff))
					key = !!key;
				key += 0x7654321;
				cipher[i] ^= key;
			}
			break;
		case 2:
			for (i = 0; i < loop; i++) {
				if (!(i & 0xff))
					key = ~key;
				key += 0x7654321;
				cipher[i] ^= key;
			}
			break;
		case 3:
			for (i = 0; i < loop; i++) {
				if (!(i & 0xff))
					key = ~key;
				key += cipher_len;
				cipher[i] ^= key;
			}
			break;
		case 4:
			for (i = 0; i < loop; i++) {
				if (i & 3) {
					if (key & 0x80000000)
						key = (key << 1) | 1;
					else
						key = (key << 1) & 0xFFFFFFFE;
				}
				key = cipher_len + ~key;
				cipher[i] = (cipher[i] >> 24) | (((cipher[i] >> 16) & 0xff) << 8) | (((cipher[i] >> 8) & 0xff) << 16) | (cipher[i] << 24);
				cipher[i] ^= key;
			}
			break;
		case 5:
			for (i = 0; i < loop; i++) {
				if (i & 3) {
					if (key & 0x80000000)
						key = (key << 1) | 1;
					else
						key = (key << 1) & 0xFFFFFFFE;
				}
				key = 0x7654321 + ~key;
				cipher[i] = (cipher[i] >> 24) | (((cipher[i] >> 16) & 0xff) << 8) | (((cipher[i] >> 8) & 0xff) << 16) | (cipher[i] << 24);
				cipher[i] ^= key;
			}
			break;
		}

		uncomprlen = cipher_len;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;	
		}
		memcpy(uncompr, cipher, cipher_len);
	} else if (!strncmp((char *)compr, "TCRP", 4)) {
		DWORD cipher_len = comprlen - 8;
		HCRYPTPROV hCryptProv;   // 定义一个CSP模块的句柄
		HCRYPTHASH hHash;
		HCRYPTKEY hKey;
	//	BYTE passwd = 0;
		BYTE *cipher = compr + 8;

		//这个函数是获取有某个容器的CSP模块的指针，成功返回TRUE
		if (CryptAcquireContext(&hCryptProv, _T("FLD"), _T("Microsoft Base Cryptographic Provider v1.0"),
				PROV_RSA_FULL, 0) == FALSE) {
			free(compr);
			return -CUI_EMATCH;		
		}

		if (CryptCreateHash(hCryptProv, CALG_SHA, 0, 0, &hHash) == FALSE) {
			CryptReleaseContext(hCryptProv, 0);
			free(compr);
			return -CUI_EMATCH;
		}
#if 0
	//	if (CryptHashData(hHash, &passwd, 0, 0) == FALSE) {
		if (CryptHashData(hHash, NULL, 0, 0) == FALSE) {
			CryptDestroyHash(hHash);
			CryptReleaseContext(hCryptProv, 0);
			free(compr);
			return -CUI_EMATCH;		
		}
#endif
		// 获取对话密码
		if (CryptDeriveKey(hCryptProv, CALG_RC4, hHash, 0, &hKey) == FALSE) {
			CryptDestroyHash(hHash);
			CryptReleaseContext(hCryptProv, 0);
			free(compr);
			return -CUI_EMATCH;		
		}

		uncomprlen = cipher_len;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			CryptDestroyHash(hHash);
			CryptReleaseContext(hCryptProv, 0);
			free(compr);
			return -CUI_EMEM;	
		}
		memcpy(uncompr, cipher, cipher_len);
		
		if (CryptDecrypt(hKey, NULL, TRUE, 0, uncompr, &uncomprlen) == FALSE) {
			free(uncompr);
			CryptDestroyHash(hHash);
			CryptReleaseContext(hCryptProv, 0);
			free(compr);
			return -CUI_EMATCH;		
		}

		CryptDestroyKey(hKey);
		CryptReleaseContext(hCryptProv, 0);
		if (uncomprlen != cipher_len) {
			free(uncompr);
			free(compr);
			return -CUI_EMATCH;	
		}
	}

	if (!strncmp((char *)compr, "TUTA", 4) || (uncompr && !strncmp((char *)uncompr, "TUTA", 4))) {
		tuta_header_t *tuta_header;
		HCRYPTPROV hCryptProv;
		BYTE *cipher;
		HCRYPTHASH hHash;

		if (!strncmp((char *)compr, "TUTA", 4))
			tuta_header = (tuta_header_t *)compr;
		else
			tuta_header = (tuta_header_t *)uncompr;			

		if (CryptAcquireContext(&hCryptProv, NULL, _T("Microsoft Base Cryptographic Provider v1.0"),
				PROV_RSA_FULL, CRYPT_VERIFYCONTEXT) == FALSE) {
			free(uncompr);
			free(compr);
			return -CUI_EMATCH;		
		}

		if (CryptCreateHash(hCryptProv, CALG_SHA, 0, 0, &hHash) == FALSE) {
			CryptReleaseContext(hCryptProv, 0);
			free(uncompr);
			free(compr);
			return -CUI_EMATCH;
		}

		cipher = ((BYTE *)(tuta_header + 1)) + tuta_header->hash_length;
		if (CryptHashData(hHash, cipher, tuta_header->cipher_length, 0) == FALSE) {
			CryptDestroyHash(hHash);
			CryptReleaseContext(hCryptProv, 0);
			free(uncompr);
			free(compr);
			return -CUI_EMATCH;		
		}

		DWORD HashDataLen = 0;
		if (CryptGetHashParam(hHash, HP_HASHVAL, NULL, &HashDataLen, 0) == FALSE) {
			CryptDestroyHash(hHash);
			CryptReleaseContext(hCryptProv, 0);
			free(uncompr);
			free(compr);
			return -CUI_EMATCH;	
		}

		if (HashDataLen != tuta_header->hash_length) {
			CryptDestroyHash(hHash);
			CryptReleaseContext(hCryptProv, 0);
			free(uncompr);
			free(compr);
			return -CUI_EMATCH;	
		}

		BYTE *HashData = (BYTE *)malloc(HashDataLen);
		if (!HashData) {
			CryptDestroyHash(hHash);
			CryptReleaseContext(hCryptProv, 0);
			free(uncompr);
			free(compr);
			return -CUI_EMATCH;	
		}

		if (CryptGetHashParam(hHash, HP_HASHVAL, HashData, &HashDataLen, 0) == FALSE) {
			free(HashData);
			CryptDestroyHash(hHash);
			CryptReleaseContext(hCryptProv, 0);
			free(uncompr);
			free(compr);
			return -CUI_EMATCH;	
		}

		if (memcmp(tuta_header + 1, HashData, HashDataLen)) {
			free(HashData);
			CryptDestroyHash(hHash);
			CryptReleaseContext(hCryptProv, 0);
			free(uncompr);
			free(compr);
			return -CUI_EMATCH;	
		}
		free(HashData);
		CryptDestroyHash(hHash);
		CryptReleaseContext(hCryptProv, 0);

		if (!strncmp((char *)cipher, "FLD0", 4)) {
			fld0_header_t *fld0_header = (fld0_header_t *)cipher;
//			DWORD *fld0_data;
		}
	}

	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

static int ego_dat_save_resource(struct resource *res, 
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

static void ego_dat_release_resource(struct package *pkg, 
											 struct package_resource *pkg_res)
{
	if (pkg_res && pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res && pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void ego_dat_release(struct package *pkg, 
									struct package_directory *pkg_dir)
{
	if (pkg_dir->directory && pkg_dir->directory_length) {		
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg);
}

static cui_ext_operation ego_dat_operation = {
	ego_dat_match,					/* match */
	ego_dat_extract_directory,		/* extract_directory */
	ego_dat_parse_resource_info,	/* parse_resource_info */
	ego_dat_extract_resource,		/* extract_resource */
	ego_dat_save_resource,			/* save_resource */
	ego_dat_release_resource,		/* release_resource */
	ego_dat_release					/* release */
};

/********************* old dat *********************/

static int ego_old_dat_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	u32 index_length;
	if (pkg->pio->read(pkg, &index_length, 4)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (!index_length) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int ego_old_dat_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{
	u32 index_length;
	if (pkg->pio->readvec(pkg, &index_length, 4, 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	BYTE *index = new BYTE[index_length];
	if (!index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index, index_length)) {
		delete [] index;
		return -CUI_EREAD;
	}

	DWORD index_entries = 0;
	BYTE *p = index;
	while (p < index + index_length) {
		p += *(u32 *)p;
		++index_entries;
	}

	my_dat_entry_t *index_buffer = new my_dat_entry_t[index_entries];
	if (!index_buffer) {
		delete [] index;
		return -CUI_EMEM;
	}

	p = index;
	for (DWORD i = 0; i < index_entries; ++i) {
		my_dat_entry_t &entry = index_buffer[i];
		p += 4;
		entry.flags = *(u32 *)p;
		p += 4;
		entry.offset = *(u32 *)p;
		p += 4;
		entry.length = *(u32 *)p;
		p += 4;
		strcpy(entry.name, (char *)p);
		p += strlen((char *)p) + 1;
	}
	delete [] index;
	
	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_entries * sizeof(my_dat_entry_t);
	pkg_dir->index_entry_length = sizeof(my_dat_entry_t);
//	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

static int ego_old_dat_parse_resource_info(struct package *pkg,
										   struct package_resource *pkg_res)
{
	my_dat_entry_t *my_dat_entry;

	my_dat_entry = (my_dat_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_dat_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_dat_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_dat_entry->offset;

	return 0;
}

static int ego_old_dat_extract_resource(struct package *pkg,
										struct package_resource *pkg_res)
{
	DWORD comprlen = pkg_res->raw_data_length;
	BYTE *compr = new BYTE[comprlen];
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, 
			pkg_res->offset, IO_SEEK_SET)) {
		delete [] compr;
		return -CUI_EREADVEC;
	}

	pkg_res->raw_data = compr;

	return 0;
}

static cui_ext_operation ego_old_dat_operation = {
	ego_old_dat_match,					/* match */
	ego_old_dat_extract_directory,		/* extract_directory */
	ego_old_dat_parse_resource_info,	/* parse_resource_info */
	ego_old_dat_extract_resource,		/* extract_resource */
	ego_dat_save_resource,			/* save_resource */
	ego_dat_release_resource,		/* release_resource */
	ego_dat_release					/* release */
};

int CALLBACK ego_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &ego_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &ego_old_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	return 0;
}
