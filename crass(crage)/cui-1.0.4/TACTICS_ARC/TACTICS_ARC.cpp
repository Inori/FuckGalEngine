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
struct acui_information TACTICS_ARC_cui_information = {
	_T(""),					/* copyright */
	_T(""),					/* system */
	_T(".arc"),				/* package */
	_T("1.0.2"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-12-27 10:35"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[16];		// "TACTICS_ARC_FILE"
	u32 comprlen;
	u32 uncomprlen;
	u32 index_entries;
	u32 unknown;
} arc_header_t;
#pragma pack ()

typedef struct {
	u32 offset;
	u32 comprlen;
	u32 uncomprlen;
	u32 name_length;
	s8 name[256];
} my_arc_entry_t;

static void lzss_decompress(BYTE *uncompr, DWORD uncomprlen, 
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
		if (!(flag & 0x0100))
			flag = compr[curbyte++] | 0xff00;
		if (flag & 1) {
			unsigned char data;

			data = compr[curbyte++];
			uncompr[act_uncomprlen++] = data;
			if (act_uncomprlen == uncomprlen)
				return;
			/* 输出的1字节放入滑动窗口 */
			win[nCurWindowByte++] = data;
			nCurWindowByte &= win_size - 1;
		} else {
			unsigned int copy_bytes, win_offset;
			unsigned int i;

			win_offset = compr[curbyte++];
			copy_bytes = compr[curbyte++];
			win_offset |= (copy_bytes >> 4) << 8;
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

/********************* arc *********************/

/* 封包匹配回调函数 */
static int TACTICS_ARC_arc_match(struct package *pkg)
{
	s8 magic[16];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "TACTICS_ARC_FILE", 16)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int TACTICS_ARC_arc_extract_directory(struct package *pkg,
											 struct package_directory *pkg_dir)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;
	DWORD index_entries, unknown, my_arc_index_buffer_len;
	my_arc_entry_t *my_arc_index_buffer;
	char *decrypt_string;
	int decrypt_string_len;

	if (pkg->pio->read(pkg, &comprlen, sizeof(comprlen)))
		return -CUI_EREAD;

	if (pkg->pio->read(pkg, &uncomprlen, sizeof(uncomprlen)))
		return -CUI_EREAD;

	if (pkg->pio->read(pkg, &index_entries, sizeof(index_entries)))
		return -CUI_EREAD;

	if (pkg->pio->read(pkg, &unknown, sizeof(unknown)))
		return -CUI_EREAD;

	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, compr, comprlen)) {
		free(compr);
		return -CUI_EREADONLY;
	}

	for (DWORD i = 0; i < comprlen; i++)
		compr[i] = ~compr[i];

	uncompr = (BYTE *)malloc(uncomprlen);
	if (!uncompr) {
		free(compr);
		return -CUI_EMEM;
	}

	lzss_decompress(uncompr, uncomprlen, compr, comprlen);
	free(compr);
MySaveFile(_T("dfsd"), uncompr, uncomprlen);
	decrypt_string_len = strlen((char *)uncompr) + 1;
	decrypt_string = (char *)malloc(decrypt_string_len);
	if (!decrypt_string) {
		free(uncompr);
		return -CUI_EMEM;
	}
	memcpy(decrypt_string, uncompr, decrypt_string_len);

	my_arc_index_buffer_len = sizeof(my_arc_entry_t) * index_entries;
	my_arc_index_buffer = (my_arc_entry_t *)malloc(my_arc_index_buffer_len);
	if (!my_arc_index_buffer) {
		free(uncompr);
		return -CUI_EMEM;
	}

	BYTE *p_index = uncompr + decrypt_string_len;
	u32 len = *(u32 *)(p_index + 4);
	u32 nlen = *(u32 *)(p_index + 12);
	int is_old;
	if (*(u32 *)(p_index + 16 + nlen) == len)
		is_old = 1;
	else
		is_old = 0;

	for (i = 0; i < index_entries; i++) {
		my_arc_entry_t *my_arc_entry = &my_arc_index_buffer[i];
		u32 name_length;

		my_arc_entry->offset = *(u32 *)p_index + comprlen + sizeof(arc_header_t);
		p_index += 4;
		my_arc_entry->comprlen = *(u32 *)p_index;
		p_index += 4;
		my_arc_entry->uncomprlen = *(u32 *)p_index;
		p_index += 4;
		name_length = *(u32 *)p_index;
		p_index += 4;
		if (!is_old)
			p_index += 8;	// unknown
		strncpy(my_arc_entry->name, (char *)p_index, name_length);
		my_arc_entry->name[name_length] = 0;
		p_index += name_length;
	}
	free(uncompr);

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = my_arc_index_buffer;
	pkg_dir->directory_length = my_arc_index_buffer_len;
	pkg_dir->index_entry_length = sizeof(my_arc_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;
	package_set_private(pkg, decrypt_string);
	return 0;
}

/* 封包索引项解析函数 */
static int TACTICS_ARC_arc_parse_resource_info(struct package *pkg,
											   struct package_resource *pkg_res)
{
	my_arc_entry_t *my_arc_entry;

	my_arc_entry = (my_arc_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_arc_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_arc_entry->comprlen;
	pkg_res->actual_data_length = my_arc_entry->uncomprlen;
	pkg_res->offset = my_arc_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int TACTICS_ARC_arc_extract_resource(struct package *pkg,
											struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	char *decrypt_string;
	int decrypt_string_len;

	decrypt_string = (char *)package_get_private(pkg);
	decrypt_string_len = strlen(decrypt_string);

	compr = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	int k = 0;
	for (DWORD i = 0; i < pkg_res->raw_data_length; i++) {
		if (k == decrypt_string_len)
			k = 0;
		compr[i] ^= decrypt_string[k++];
	}

	if (pkg_res->actual_data_length) {
		uncompr = (BYTE *)malloc(pkg_res->actual_data_length);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}

		lzss_decompress(uncompr, pkg_res->actual_data_length, 
			compr, pkg_res->raw_data_length);
	} else
		uncompr = NULL;

	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;

	return 0;
}

/* 资源保存函数 */
static int TACTICS_ARC_arc_save_resource(struct resource *res, 
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
static void TACTICS_ARC_arc_release_resource(struct package *pkg, 
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
static void TACTICS_ARC_arc_release(struct package *pkg,
									struct package_directory *pkg_dir)
{
	char *decrypt_string;

	decrypt_string = (char *)package_get_private(pkg);
	if (decrypt_string) {
		free(decrypt_string);
		package_set_private(pkg, NULL);	
	}

	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation TACTICS_ARC_arc_operation = {
	TACTICS_ARC_arc_match,				/* match */
	TACTICS_ARC_arc_extract_directory,	/* extract_directory */
	TACTICS_ARC_arc_parse_resource_info,/* parse_resource_info */
	TACTICS_ARC_arc_extract_resource,	/* extract_resource */
	TACTICS_ARC_arc_save_resource,		/* save_resource */
	TACTICS_ARC_arc_release_resource,	/* release_resource */
	TACTICS_ARC_arc_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK TACTICS_ARC_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".arc"), NULL, 
		NULL, &TACTICS_ARC_arc_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
