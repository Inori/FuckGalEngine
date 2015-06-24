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

struct acui_information ESCARC_cui_information = {
	_T("エスクード"),			/* copyright */
	NULL,						/* system */
	_T(".bin .c .h .dat .bmp .txt .mot .db .dif .zip"),	/* package */
	_T("1.0.1"),				/* revision */
	_T("痴汉公贼"),				/* author */
	_T("2009-1-10 12:31"),		/* date */
	NULL,						/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

#pragma pack (1)
typedef struct {
	s8 magic[8];	// "ACPXPK01"
	u32 entries;
} old_bin_header_t;

typedef struct {
	s8 name[32];
	u32 offset;
	u32 length;
} old_bin_entry_t;

typedef struct {
	s8 magic[8];
	u32 key;
	u32 entries;
} bin_header1_t;

typedef struct {
	s8 magic[8];
	u32 key;
	u32 entries;
	u32 name_table_length;
} bin_header2_t;

typedef struct {
	s8 name[128];
	u32 offset;
	u32 length;
} bin_entry_t;

typedef struct {
	u32 name_offset;
	u32 offset;
	u32 length;
} bin_entry2_t;

typedef struct {
	s8 magic[4];
	u32 uncomprlen;
} acp_header_t;
#pragma pack ()

static ESCARC_version = 1;

#define SWAP16(v)	((((v) & 0xff) << 8) | (((v) >> 8) & 0xff))
#define SWAP32(v)	((((v) & 0xff) << 24) | (((v) & 0xff00) << 8) | (((v) & 0xff0000) >> 8) | (((v) & 0xff000000) >> 24))

static u32 update_key(u32 key)
{
	key ^= 0x65ac9365;
	key = (((key >> 1) ^ key) >> 3) ^ (((key << 1) ^ key) << 3) ^ key;
	
	return key;
}

static BYTE lzw_dict_letter[35023];
static DWORD lzw_dict_prefix_code[35023];

static int lzw_decode(BYTE *uncompr, DWORD uncomprlen,
					  BYTE *compr, DWORD comprlen)
{
	struct bits bits;
	bits_init(&bits, compr, comprlen);
	DWORD act_uncomprlen = 0;
	while (1) {
		DWORD code_bits = 9;
		DWORD cur_pos = 259;
		unsigned int prefix_code;
		if (bits_get_high(&bits, code_bits, &prefix_code))
			return act_uncomprlen;

		if (prefix_code == 256)	// END_OF_STREAM
			break;

		uncompr[act_uncomprlen++] = prefix_code;
		if (act_uncomprlen > uncomprlen)
			return -CUI_EUNCOMPR;
		
		BYTE last_code = prefix_code;
		while (1) {
			unsigned int suffix_code;
			if (bits_get_high(&bits, code_bits, &suffix_code))
				return act_uncomprlen;

			if (suffix_code == 257)	// INCREAM_BITS
				++code_bits;
			else if (suffix_code == 258)	// CLEAN_DICT
				break;
			else if (suffix_code == 256)	// END_OF_STREAM
				return act_uncomprlen;
			else {
				DWORD phrase_len;
				DWORD code;
				BYTE phrase[4096];
				if (suffix_code < cur_pos) {	// hit
					phrase_len = 0;
					code = suffix_code;
				} else {
					phrase[0] = last_code;
					code = prefix_code;
					phrase_len = 1;
				}

				while (code > 255) {
					phrase[phrase_len++] = lzw_dict_letter[code];
					code = lzw_dict_prefix_code[code];
				}
				phrase[phrase_len++] = (BYTE)code;
				last_code = (BYTE)code;

				if (act_uncomprlen + phrase_len > uncomprlen)
					return -CUI_EUNCOMPR;

				for (int i = phrase_len - 1; i >= 0; --i)
					uncompr[act_uncomprlen++] = phrase[i];
				
				lzw_dict_prefix_code[cur_pos] = prefix_code;
				lzw_dict_letter[cur_pos++] = (BYTE)code;
				prefix_code = suffix_code;
			}
		}
	}

	return act_uncomprlen;
}

/********************* bin *********************/

/* 封包匹配回调函数 */
static int ESCARC_bin_match(struct package *pkg)
{
	s8 magic[8];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (!strncmp(magic, "ESC-ARC1", 8))
		ESCARC_version = 1;
	else if (!strncmp(magic, "ESC-ARC2", 8)) 
		ESCARC_version = 2;
	else {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}	

	return 0;	
}

static int ESCARC_bin_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	u32 key, index_entries;
	u32 name_table_length;

	if (pkg->pio->read(pkg, &key, 4))
		return -CUI_EREAD;

	if (pkg->pio->read(pkg, &index_entries, 4))
		return -CUI_EREAD;

	key = update_key(key);
	index_entries ^= key;

	if (ESCARC_version == 2) {		
		if (pkg->pio->read(pkg, &name_table_length, 4))
			return -CUI_EREAD;

		key = update_key(key);
		name_table_length ^= key;
	}

	DWORD index_buffer_length;
	void *index_buffer;
	if (ESCARC_version == 1) {
		index_buffer_length = index_entries * sizeof(bin_entry_t);
		index_buffer = new bin_entry_t[index_entries];
	} else if (ESCARC_version == 2) {
		index_buffer_length = index_entries * sizeof(bin_entry2_t);
		index_buffer = new bin_entry2_t[index_entries];
	}
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		delete [] index_buffer;
		return -CUI_EREAD;
	}

	u32 *p = (u32 *)index_buffer;
	for (DWORD i = 0; i < index_buffer_length / 4; ++i) {
		key = update_key(key);
		p[i] ^= key;
	}

	if (ESCARC_version == 2) {
		char *name_table = new char[name_table_length];
		if (!name_table) {
			delete [] index_buffer;
			return -CUI_EMEM;
		}

		if (pkg->pio->read(pkg, name_table, name_table_length)) {
			delete [] name_table;
			delete [] index_buffer;
			return -CUI_EREAD;
		}

		index_buffer_length = index_entries * sizeof(bin_entry_t);
		bin_entry_t *real_index_buffer = new bin_entry_t[index_entries];
		if (!real_index_buffer) {
			delete [] name_table;
			delete [] index_buffer;
			return -CUI_EMEM;	
		}

		bin_entry2_t *p = (bin_entry2_t *)index_buffer;
		for (DWORD i = 0; i < index_entries; ++i) {
			strcpy(real_index_buffer[i].name, &name_table[p[i].name_offset]);
			real_index_buffer[i].offset = p[i].offset;
			real_index_buffer[i].length = p[i].length;
		}
		delete [] name_table;
		delete [] index_buffer;
		index_buffer = real_index_buffer;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->index_entry_length = sizeof(bin_entry_t);
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;

	return 0;
}

static int ESCARC_bin_parse_resource_info(struct package *pkg,
										  struct package_resource *pkg_res)
{
	bin_entry_t *bin_entry;

	bin_entry = (bin_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, bin_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = bin_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = bin_entry->offset;

	return 0;
}

static int ESCARC_bin_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	DWORD comprlen = pkg_res->raw_data_length;
	BYTE *compr = new BYTE[comprlen];
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, pkg_res->offset, IO_SEEK_SET)) {
		delete [] compr;
		return -CUI_EREADVEC;
	}
	pkg_res->raw_data = compr;

	return 0;
}

static int ESCARC_bin_save_resource(struct resource *res, 
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
static void ESCARC_bin_release_resource(struct package *pkg, 
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

static void ESCARC_bin_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg);
}

static cui_ext_operation ESCARC_bin_operation = {
	ESCARC_bin_match,				/* match */
	ESCARC_bin_extract_directory,	/* extract_directory */
	ESCARC_bin_parse_resource_info,	/* parse_resource_info */
	ESCARC_bin_extract_resource,	/* extract_resource */
	ESCARC_bin_save_resource,		/* save_resource */
	ESCARC_bin_release_resource,	/* release_resource */
	ESCARC_bin_release				/* release */
};

/********************* old bin *********************/

/* 封包匹配回调函数 */
static int ESCARC_old_bin_match(struct package *pkg)
{
	s8 magic[8];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "ACPXPK01", 8)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}	

	return 0;	
}

static int ESCARC_old_bin_extract_directory(struct package *pkg,
											struct package_directory *pkg_dir)
{
	u32 index_entries;

	if (pkg->pio->read(pkg, &index_entries, 4))
		return -CUI_EREAD;

	DWORD index_buffer_length = index_entries * sizeof(old_bin_entry_t);
	old_bin_entry_t *index_buffer = new old_bin_entry_t[index_entries];
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		delete [] index_buffer;
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->index_entry_length = sizeof(old_bin_entry_t);
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;

	return 0;
}

static int ESCARC_old_bin_parse_resource_info(struct package *pkg,
											  struct package_resource *pkg_res)
{
	old_bin_entry_t *bin_entry;

	bin_entry = (old_bin_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, bin_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = bin_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = bin_entry->offset;

	return 0;
}

static int ESCARC_old_bin_extract_resource(struct package *pkg,
										   struct package_resource *pkg_res)
{
	DWORD comprlen = pkg_res->raw_data_length;
	BYTE *compr = new BYTE[comprlen];
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, pkg_res->offset, IO_SEEK_SET)) {
		delete [] compr;
		return -CUI_EREADVEC;
	}
	pkg_res->raw_data = compr;

	return 0;
}

static cui_ext_operation ESCARC_old_bin_operation = {
	ESCARC_old_bin_match,				/* match */
	ESCARC_old_bin_extract_directory,	/* extract_directory */
	ESCARC_old_bin_parse_resource_info,	/* parse_resource_info */
	ESCARC_old_bin_extract_resource,	/* extract_resource */
	ESCARC_bin_save_resource,		/* save_resource */
	ESCARC_bin_release_resource,	/* release_resource */
	ESCARC_bin_release				/* release */
};

/********************* acp *********************/

/* 封包匹配回调函数 */
static int ESCARC_acp_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "acp", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int ESCARC_acp_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	u32 acp_size;
	if (pkg->pio->length_of(pkg, &acp_size))
		return -CUI_ELEN;

	acp_header_t acp_header;
	if (pkg->pio->readvec(pkg, &acp_header, sizeof(acp_header_t), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD comprlen = acp_size - sizeof(acp_header);
	BYTE *compr = new BYTE[comprlen];
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, sizeof(acp_header_t), IO_SEEK_SET)) {
		delete [] compr;
		return -CUI_EREADVEC;
	}

	DWORD uncomprlen = SWAP32(acp_header.uncomprlen);
	BYTE *uncompr = new BYTE[uncomprlen];
	if (!uncompr) {
		delete [] compr;
		return -CUI_EMEM;
	}

	int ret = lzw_decode(uncompr, uncomprlen, compr, comprlen);
	delete [] compr;
	if (ret < 0) {
		delete [] uncompr;
		return ret;
	}
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = ret;

	return 0;
}

static cui_ext_operation ESCARC_acp_operation = {
	ESCARC_acp_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	ESCARC_acp_extract_resource,/* extract_resource */
	ESCARC_bin_save_resource,	/* save_resource */
	ESCARC_bin_release_resource,/* release_resource */
	ESCARC_bin_release			/* release */
};

int CALLBACK ESCARC_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".bin"), NULL, 
		NULL, &ESCARC_bin_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".c"), _T(".c.txt"), 
		NULL, &ESCARC_acp_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".h"), _T(".h.txt"), 
		NULL, &ESCARC_acp_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".bin"), _T(".bin.dump"), 
		NULL, &ESCARC_acp_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), _T(".dat.txt"), 
		NULL, &ESCARC_acp_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".bmp"), _T(".dump.bmp"), 
		NULL, &ESCARC_acp_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".txt"), _T(".dump.txt"), 
		NULL, &ESCARC_acp_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".mot"), _T(".dump.mot"), 
		NULL, &ESCARC_acp_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".db"), _T(".dump.db"), 
		NULL, &ESCARC_acp_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dif"), _T(".dump.dif"), 
		NULL, &ESCARC_acp_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".zip"), _T(".dump.zip"), 
		NULL, &ESCARC_acp_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".bin"), NULL, 
		NULL, &ESCARC_old_bin_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}

