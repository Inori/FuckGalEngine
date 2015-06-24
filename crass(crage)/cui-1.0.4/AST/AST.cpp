#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>

struct acui_information AST_cui_information = {
	NULL,				/* copyright */
	_T("AST"),			/* system */
	NULL,				/* package */
	_T("1.1.4"),		/* revision */
	_T("痴漢公賊"),		/* author */
	_T("2008-10-2 22:01"),/* date */
	NULL,				/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

#pragma pack (1)
typedef struct {
	s8 magic[4];				/* "ARC1" or "ARC2" */
	u32 index_entries;
} ARC_header_t;
#pragma pack ()

typedef struct {
	s8 name_length;
	s8 name[MAX_PATH];
	u32 offset;
	u32 comprLen;
	u32 uncomprLen;
} ARC_entry_t;

static inline unsigned char getbit_le(unsigned char byte, unsigned int pos)
{
	return !!(byte & (1 << pos));
}

static inline unsigned char getbyte_le(unsigned char byte)
{
	return byte;
}

static int lzss_decompress(BYTE *uncompr, DWORD uncomprLen,
						   BYTE *compr, DWORD comprLen)
{
	unsigned int act_uncomprlen = 0;
	unsigned int curbyte = 0;		/* compr中的当前扫描字节 */
	unsigned int nCurWindowByte = 0xfee;
	unsigned char window[4096];
	unsigned int win_size = 4096;

	memset(window, 0, win_size);	
	while (1) {
		unsigned char bitmap;

		if (curbyte >= comprLen)
			break;
		
		bitmap = getbyte_le(compr[curbyte++]);
		for (unsigned int curbit_bitmap = 0; curbit_bitmap < 8; curbit_bitmap++) {
			/* 如果为1, 表示接下来的1个字节原样输出 */
			if (getbit_le(bitmap, curbit_bitmap)) {
				unsigned char data;

				if (curbyte >= comprLen)
					goto out;

				if (act_uncomprlen >= uncomprLen)
					goto out;

				data = ~getbyte_le(compr[curbyte++]);
				/* 输出1字节非压缩数据 */
				uncompr[act_uncomprlen++] = data;		
				/* 输出的1字节放入滑动窗口 */
				window[nCurWindowByte++] = data;
				nCurWindowByte &= win_size - 1;
			} else {
				unsigned int copy_bytes, win_offset;
				
				if (curbyte >= comprLen)
					goto out;
				win_offset = getbyte_le(compr[curbyte++]);

				if (curbyte >= comprLen)
					goto out;
				copy_bytes = getbyte_le(compr[curbyte++]);

				win_offset |= (copy_bytes >> 4) << 8;
				copy_bytes &= 0x0f;
				copy_bytes += 3;
				/* 原因很简单：如果拷贝的只有1字节和2字节，那么编码的用途就不大了
				 * 但是copy_bytes就4位，所以0和1表示3和4，所以实际的copy_bytes都要
				 * +3才对.
				 */
				for (unsigned int i = 0; i < copy_bytes; i++) {
					unsigned char data;

					if (act_uncomprlen >= uncomprLen)
						goto out;
					
					data = window[(win_offset + i) & (win_size - 1)];
					uncompr[act_uncomprlen++] = data;		
					/* 输出的1字节放入滑动窗口 */
					window[nCurWindowByte++] = data;
					nCurWindowByte &= win_size - 1;		
				}
			}
		}
	}
out:
	return act_uncomprlen;	
}

/*********************  *********************/

static int AST_match(struct package *pkg)
{
	s8 magic[4];
	unsigned long is_arc2;
	unsigned int index_entries;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (!memcmp(magic, "ARC2", 4))
		is_arc2 = 1;
	else if (!memcmp(magic, "ARC1", 4))
		is_arc2 = 0;
	else {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (pkg->pio->read(pkg, &index_entries, sizeof(index_entries))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (!index_entries) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}
	package_set_private(pkg, (void *)is_arc2);

	return 0;	
}

static int AST_extract_directory(struct package *pkg,
								 struct package_directory *pkg_dir)
{
	int index_entries;
	ARC_entry_t *index_buffer, *entry;
	unsigned int index_buffer_length;	
	unsigned long is_arc2;
	u32 arc_size;

	if (pkg->pio->readvec(pkg, &index_entries, sizeof(index_entries),
			4, IO_SEEK_SET))
		return -CUI_EREAD;

	if (pkg->pio->length_of(pkg, &arc_size))
		return -CUI_ELEN;

	index_buffer_length = index_entries * sizeof(ARC_entry_t);
	index_buffer = (ARC_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;
	memset(index_buffer, 0, index_buffer_length);
	is_arc2 = (unsigned long)package_get_private(pkg);
	
	for (int i = 0; i < index_entries; ++i) {		
		entry = &index_buffer[i];

		if (pkg->pio->read(pkg, &entry->offset, 4))
			break;

		if (pkg->pio->read(pkg, &entry->uncomprLen, 4))
			break;

		if (pkg->pio->read(pkg, &entry->name_length, 1))
			break;

		if (pkg->pio->read(pkg, &entry->name, entry->name_length))
			break;
		
		if (is_arc2) {
			for (int k = 0; k < entry->name_length; k++)
				entry->name[k] = ~entry->name[k];
		}	
		entry->name[entry->name_length] = 0;
	}
	if (i != index_entries) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	// 在第一个超过2G偏移的文件之后的资源,uncomprLen和offset都为0(怀疑是封包器出错)
	i = 0;
	for (int n = 1; n < index_entries; ++n) {
		if (index_buffer[i].offset && index_buffer[n].offset) {
			index_buffer[i].comprLen = index_buffer[n].offset - index_buffer[i].offset;
			i = n;
		}		
	}
	if (index_buffer[n - 1].offset)
		index_buffer[n - 1].comprLen = (u32)(arc_size - index_buffer[n - 1].offset);

	pkg_dir->index_entries = index_entries;
	pkg_dir->index_entry_length = sizeof(ARC_entry_t);
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

static int AST_parse_resource_info(struct package *pkg,
								   struct package_resource *pkg_res)
{
	ARC_entry_t *ARC_entry;

	ARC_entry = (ARC_entry_t *)pkg_res->actual_index_entry;
	strncpy(pkg_res->name, ARC_entry->name, ARC_entry->name_length);
	pkg_res->name_length = ARC_entry->name_length;
	pkg_res->raw_data_length = ARC_entry->comprLen;
	pkg_res->actual_data_length = ARC_entry->uncomprLen;
	pkg_res->offset = ARC_entry->offset;

	return 0;
}

static int AST_extract_resource(struct package *pkg,
								struct package_resource *pkg_res)
{
	unsigned char *compr, *uncompr = NULL;
	unsigned int comprlen, uncomprlen = 0;
	
	comprlen = (unsigned int)pkg_res->raw_data_length;
	compr = (unsigned char *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec64(pkg, compr, comprlen, 0, pkg_res->offset, 0, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	unsigned long is_arc2 = (unsigned long)package_get_private(pkg);
	if (is_arc2) {
		if (!memcmp(compr, "\x76\xaf\xb1\xb8", 4)) {
			for (unsigned int k = 0; k < comprlen; k++)
				compr[k] = ~compr[k];
		} else if (!memcmp(compr, "RIFF", 4)) {
			;
		} else if (!memcmp(compr, "OggS", 4)) {
			;
		} else {
			unsigned int act_uncomprlen;

			uncomprlen = (unsigned int)pkg_res->actual_data_length;
			uncompr = (unsigned char *)malloc(uncomprlen);
			if (!uncompr) {
				free(compr);
				return -CUI_EMEM;
			}
			act_uncomprlen = lzss_decompress(uncompr, uncomprlen, compr, comprlen);
			free(compr);
			compr = NULL;
			if (act_uncomprlen != uncomprlen) {
				free(uncompr);
				return -CUI_EUNCOMPR;				
			}
		}
	}

	pkg_res->raw_data = compr;
	pkg_res->raw_data_length = comprlen;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

static int AST_save_resource(struct resource *res, 
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

static void AST_release_resource(struct package *pkg, 
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

static void AST_release(struct package *pkg, 
						struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	package_set_private(pkg, NULL);
	pkg->pio->close(pkg);
}

static cui_ext_operation AST_operation = {
	AST_match,					/* match */
	AST_extract_directory,		/* extract_directory */
	AST_parse_resource_info,	/* parse_resource_info */
	AST_extract_resource,		/* extract_resource */
	AST_save_resource,			/* save_resource */
	AST_release_resource,		/* release_resource */
	AST_release					/* release */
};

int CALLBACK AST_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, NULL, NULL, 
		NULL, &AST_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NOEXT))
			return -1;

	return 0;
}
