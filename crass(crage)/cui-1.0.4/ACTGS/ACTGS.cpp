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

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information ACTGS_cui_information = {
	_T("ACTRESS"),			/* copyright */
	_T("ACTRESS Game System"),	/* system */
	_T(".dat .cg .scr. wav"),	/* package */
	_T("1.1.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-7-18 12:58"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	u32 index_entries;
	u32 reserved[3];
} dat_header_t;

typedef struct {
	u32 offset;
	u32 length;
	s8 name[24];
} dat_entry_t;

typedef struct {
	s8 magic[4];	// "PAK "
	u32 comprlen;
	u32 uncomprlen;
} pak_header_t;
#pragma pack ()

static const char *key1 = "\x22\x33\x41\x11\x05\x54\x16";

static const char *old_key_ring[] = {
	"ACTGS ACTRESS Game System",
	"\x10\xD3\x27\x53\x10\xD3\x27\x53\xC6\x08\x33\x12\x51\x88\x19\x21\xA1",
	NULL,
};

static void decode(BYTE *enc, DWORD enc_len, const char *key)
{
	DWORD key_len = strlen((char *)key);

	for (DWORD i = 0; i < enc_len; ++i)
		enc[i] ^= key[i % key_len];
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
			if (curbyte == comprlen)
				break;
			flag = compr[curbyte++] | 0xff00;
		}

		if (flag & 1) {
			unsigned char data;

			if (curbyte == comprlen)
				break;

			data = compr[curbyte++];
			uncompr[act_uncomprlen++] = data;
			/* 输出的1字节放入滑动窗口 */
			win[nCurWindowByte++] = data;
			nCurWindowByte &= win_size - 1;
		} else {
			unsigned int copy_bytes, win_offset;
			unsigned int i;

			if (curbyte == comprlen)
				break;
			win_offset = compr[curbyte++];

			if (curbyte == comprlen)
				break;
			copy_bytes = compr[curbyte++];
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

/********************* dat *********************/

/* 封包匹配回调函数 */
static int ACTGS_dat_match(struct package *pkg)
{
	dat_header_t header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &header, sizeof(header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if ((int)header.index_entries <= 0) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	dat_entry_t entry;

	if (pkg->pio->read(pkg, &entry, sizeof(entry))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	const char *dec_key;
	if (entry.offset == (sizeof(header) + header.index_entries * sizeof(entry)))
		dec_key = NULL;
	else {
		decode((BYTE *)&entry, sizeof(entry), key1);
		if (entry.offset == (sizeof(header) + header.index_entries * sizeof(entry)))
			dec_key = key1;
		else {
			pkg->pio->close(pkg);
			return -CUI_EMATCH;		
		}
	}

	package_set_private(pkg, dec_key);

	return 0;	
}

/* 封包索引目录提取函数 */
static int ACTGS_dat_extract_directory(struct package *pkg,
									   struct package_directory *pkg_dir)
{
	dat_header_t header;

	if (pkg->pio->readvec(pkg, &header, sizeof(header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD index_buffer_length = header.index_entries * sizeof(dat_entry_t);
	dat_entry_t *index_buffer = new dat_entry_t[header.index_entries];
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		delete [] index_buffer;
		return -CUI_EREAD;
	}

	const char *dec_key = (const char *)package_get_private(pkg);
	if (dec_key)
		decode((BYTE *)index_buffer, index_buffer_length, dec_key);

	pkg_dir->index_entries = header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(dat_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int ACTGS_dat_parse_resource_info(struct package *pkg,
										 struct package_resource *pkg_res)
{
	dat_entry_t *dat_entry;

	dat_entry = (dat_entry_t *)pkg_res->actual_index_entry;
	memset(pkg_res->name, 0, 25);
	strcpy(pkg_res->name, dat_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = dat_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = dat_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int ACTGS_dat_extract_resource(struct package *pkg,
									  struct package_resource *pkg_res)
{
	BYTE *data;
	DWORD data_len;

	data_len = pkg_res->raw_data_length;
	data = (BYTE *)malloc(data_len);
	if (!data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, data, data_len, pkg_res->offset, IO_SEEK_SET)) {
		free(data);
		return -CUI_EREADVEC;
	}

	const char *dec_key = (const char *)package_get_private(pkg);
	if (dec_key) {
		if (strstr(pkg_res->name, ".psd") || strstr(pkg_res->name, ".bmp") || strstr(pkg_res->name, ".BMP")) {
			if (!strncmp((char *)data, "PAK ", 4)) {
				pak_header_t *pak = (pak_header_t *)data;
				BYTE *uncompr = (BYTE *)malloc(pak->uncomprlen + 4096);
				if (!uncompr) {
					free(data);
					return -CUI_EMEM;
				}

				lzss_uncompress(uncompr, pak->uncomprlen, 
					data + sizeof(pak_header_t), pak->comprlen);
				pkg_res->actual_data = uncompr;
				pkg_res->actual_data_length = pak->uncomprlen;
			} else
				decode(data, 32, dec_key);
		} else if (strstr(pkg_res->name, ".scr")) {
			if (data[0] == 'X') {
				decode(data + 1, data_len - 1, dec_key);
				data[0] = 'N';
			}
		}
	}

	pkg_res->raw_data = data;

	return 0;
}

/* 资源保存函数 */
static int ACTGS_dat_save_resource(struct resource *res, 
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
static void ACTGS_dat_release_resource(struct package *pkg, 
									   struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}

	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

/* 封包卸载函数 */
static void ACTGS_dat_release(struct package *pkg, 
							  struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation ACTGS_dat_operation = {
	ACTGS_dat_match,				/* match */
	ACTGS_dat_extract_directory,	/* extract_directory */
	ACTGS_dat_parse_resource_info,	/* parse_resource_info */
	ACTGS_dat_extract_resource,		/* extract_resource */
	ACTGS_dat_save_resource,		/* save_resource */
	ACTGS_dat_release_resource,		/* release_resource */
	ACTGS_dat_release				/* release */
};

/* 封包匹配回调函数 */
static int ACTGS_old_match(struct package *pkg)
{
	dat_header_t header;
	dat_entry_t entry;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &header, sizeof(header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (pkg->pio->read(pkg, &entry, sizeof(entry))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	DWORD i;
	const char *dec_key;
	if (((int)header.index_entries > 0) && 
			(entry.offset == (sizeof(header) + header.index_entries * sizeof(entry)))) {
		dec_key = NULL;
		goto out;
	}

	for (i = 0; old_key_ring[i]; ++i) {
		if (pkg->pio->readvec(pkg, &header, sizeof(header), 0, IO_SEEK_SET)) {
			pkg->pio->close(pkg);
			return -CUI_EREADVEC;
		}

		decode((BYTE *)&header, sizeof(header), old_key_ring[i]);

		if ((int)header.index_entries <= 0)
			continue;

		if (pkg->pio->read(pkg, &entry, sizeof(entry))) {
			pkg->pio->close(pkg);
			return -CUI_EREAD;
		}

		decode((BYTE *)&entry, sizeof(entry), old_key_ring[i]);

		if (entry.offset == (sizeof(header) + header.index_entries * sizeof(entry))) {
			dec_key = old_key_ring[i];
			break;
		}
	}
	if (!old_key_ring[i]) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;		
	}
out:
	package_set_private(pkg, dec_key);

	return 0;	
}

/* 封包索引目录提取函数 */
static int ACTGS_old_extract_directory(struct package *pkg,
									   struct package_directory *pkg_dir)
{
	dat_header_t header;

	if (pkg->pio->readvec(pkg, &header, sizeof(header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	const char *dec_key = (const char *)package_get_private(pkg);
	if (dec_key)
		decode((BYTE *)&header, sizeof(header), dec_key);

	DWORD index_buffer_length = header.index_entries * sizeof(dat_entry_t);
	dat_entry_t *index_buffer = new dat_entry_t[header.index_entries];
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		delete [] index_buffer;
		return -CUI_EREAD;
	}

	if (dec_key)
		decode((BYTE *)index_buffer, index_buffer_length, dec_key);

	pkg_dir->index_entries = header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(dat_entry_t);

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation ACTGS_old_operation = {
	ACTGS_old_match,				/* match */
	ACTGS_old_extract_directory,	/* extract_directory */
	ACTGS_dat_parse_resource_info,	/* parse_resource_info */
	ACTGS_dat_extract_resource,		/* extract_resource */
	ACTGS_dat_save_resource,		/* save_resource */
	ACTGS_dat_release_resource,		/* release_resource */
	ACTGS_dat_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK ACTGS_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &ACTGS_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".cg"), NULL, 
		NULL, &ACTGS_old_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".scr"), NULL, 
		NULL, &ACTGS_old_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".wav"), NULL, 
		NULL, &ACTGS_old_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
