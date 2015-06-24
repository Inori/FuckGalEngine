#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <resource.h>
#include <package.h>
#include <cui_error.h>
#include <utility.h>
#include <stdio.h>

struct acui_information ktg_cui_information = {
	_T("工畫堂スタジオ"),	/* copyright */
	_T(""),					/* system */
	_T(".arc"),				/* package */
	_T("0.0.2"),			/* revision */
	_T("痴汉公贼"),			/* author */
	_T("2007.05.12 20:41"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};	

#pragma pack (1)
typedef struct {
	u32 magic;			// 0xa8bcadbe
	u32 version;		// must be 0
	u32 size;
	u32 offset;			// 实际的数据的文件偏移
} arc_header_t;

typedef struct {
	u32 comprlen;
	u32 flags;
	u32 uncomprlen;
} arc_entry_t;

typedef struct {
	//u32 parts;	// 100030466 script, string: 1
	u32 ident;		// script, string: 0/* 资源目录标识符（顶级通常为0) */
	u32 reserved;	// script, string: 0
	u32 entries;
	u32 length;		// 该parts的长度
} arc_info_header_t;

typedef struct {
	u32 offset;
	u32 comprlen;
	u32 flags;
	u32 uncomprlen;
} arc_info_entry_t;

#if 0
typedef struct {
	u32 parts;	// 100030466

	u32 ident;		// script: 0/* 资源目录标识符（顶级通常为0) */
	u32 reserved;	// script: 0
	u32 entries;
	u32 info_length;
	......info		// script: name_segment 

	u32 ident;		/* 资源目录标识符（次级，比如DDS) */
	u32 reserved;
	u32 entries2;
	u32 info_length;
	......info

//script
u32 name_offset;

	size_entries:
	flag
	width
	height

	[entries2个]	// 因为通常entries所在的段表示的不是图像
	offset
	u32 comprlen;
	u32 flags;
	u32 uncomprlen;
	size_index;
} arc_info_t;
#endif
#pragma pack ()

typedef struct {
	WCHAR name[64];
	u32 offset;
	u32 comprlen;
	u32 uncomprlen;
	u32 flags;
} my_arc_entry_t;


static void lzss_uncompress(BYTE *uncompr, DWORD uncomprlen, 
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

	memset(win, 0, sizeof(win));
	while (act_uncomprlen < uncomprlen) {
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
/********************* arc *********************/

static int ktg_arc_match(struct package *pkg)
{
	arc_header_t arc_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &arc_header, sizeof(arc_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	u32 size;
	if (pkg->pio->length_of(pkg, &size)) {
		pkg->pio->close(pkg);
		return -CUI_ELEN;
	}

	if (arc_header.magic != 0xA8BCADBE || arc_header.version || arc_header.size != size) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}
	package_set_private(pkg, arc_header.offset);

	return 0;	
}

static int ktg_arc_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{
	arc_info_header_t *entry_info_buffer;
	BYTE *entry_name_buffer = NULL;
	BYTE *orig_entry_info_buffer = NULL;
	DWORD entry_name_buffer_length, entry_info_buffer_length;
	DWORD parts;
	int err = 0;

	/* 提取名称段和info段 */
	for (DWORD i = 0; i < 2; ++i) {
		arc_entry_t arc_entry;
		if (pkg->pio->read(pkg, &arc_entry, sizeof(arc_entry))) {
			err = -CUI_EREAD;
			break;	
		}

		if (arc_entry.flags > 3) {
			err = -CUI_EMATCH;
			break;	
		}

		DWORD comprlen = arc_entry.comprlen - sizeof(arc_entry);
		BYTE *compr = new BYTE[comprlen];
		if (!compr) {
			err = -CUI_EMEM;
			break;	
		}
		
		DWORD uncomprlen = arc_entry.uncomprlen;
		BYTE *uncompr = new BYTE[uncomprlen];
		if (!uncompr) {
			err = -CUI_EMEM;
			delete [] compr;
			break;	
		}

		if (pkg->pio->read(pkg, compr, comprlen)) {
			err = -CUI_EREAD;
			delete [] uncompr;
			delete [] compr;
			break;	
		}

		if (arc_entry.flags & 1) {
			for (DWORD k = 0; k < comprlen; ++k)
				compr[k] = ~compr[k];
		}

		if (arc_entry.flags & 2)	
			lzss_uncompress(uncompr, uncomprlen, compr, comprlen);
		else
			memcpy(uncompr, compr, uncomprlen);
		delete [] compr;

		if (!i) {
			entry_name_buffer_length = uncomprlen;
			entry_name_buffer = uncompr;
		} else {
			entry_info_buffer_length = uncomprlen - 4;			
			orig_entry_info_buffer = uncompr;
			entry_info_buffer = (arc_info_header_t *)(uncompr + 4);
			parts = *(u32 *)uncompr;
		}
	}
	if (err) {
		delete [] orig_entry_info_buffer;
		delete [] entry_name_buffer;
		return err;
	}	
	
	u32 *name_offset = NULL;
	my_arc_entry_t *index_buffer;
	arc_info_entry_t *entry;
	unsigned int index_length;
	u32 data_offset;

#if 2
TCHAR save_name[MAX_PATH];
	_stprintf(save_name, _T("%s_name"), pkg->name);
	MySaveFile(save_name, entry_name_buffer, entry_name_buffer_length);
_stprintf(save_name, _T("%s_info"), pkg->name);
	MySaveFile(save_name, entry_info_buffer, entry_info_buffer_length);
	exit(0);
#endif

	index_length = entry_info_buffer->entries * sizeof(my_arc_entry_t);
	index_buffer = (my_arc_entry_t *)malloc(index_length);
	if (!index_buffer) {
		free(entry_name_buffer);
		free(entry_info_buffer);
		return -CUI_EMEM;
	}

	data_offset = (u32)package_get_private(pkg);
	unsigned int start_index = 0;
	for (i = 0; i < parts; i++) {
		name_offset = (u32 *)(entry_info_buffer + 1);
		entry = (arc_info_entry_t *)(name_offset + entry_info_buffer->entries);	
		for (unsigned int k = start_index; k < start_index + entry_info_buffer->entries; k++) {
			wcscpy(index_buffer[k].name, (WCHAR *)&entry_name_buffer[name_offset[k]]);
			index_buffer[k].offset = entry[k].offset + data_offset;
			index_buffer[k].comprlen = entry[k].comprlen - 12;
			index_buffer[k].uncomprlen = entry[k].uncomprlen;
			index_buffer[k].flags = entry[k].flags;
		}
		if (i + 1 < parts) {
			entry += entry_info_buffer->entries;
			start_index += entry_info_buffer->entries;
			entry_info_buffer = (arc_info_header_t *)entry;
			index_buffer = (my_arc_entry_t *)realloc(index_buffer, index_length + entry_info_buffer->entries * sizeof(my_arc_entry_t));
			if (!index_buffer)
				break;
			index_length += entry_info_buffer->entries * sizeof(my_arc_entry_t);
		}
	}
	if (i != parts) {
		free(index_buffer);
		free(entry_name_buffer);
		free(orig_entry_info_buffer);
		return -CUI_EMEM;
	}

	free(entry_name_buffer);
	free(orig_entry_info_buffer);

	pkg_dir->index_entries = index_length / sizeof(my_arc_entry_t);
	pkg_dir->index_entry_length = sizeof(my_arc_entry_t);
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_length;	

	return 0;
}

static int ktg_arc_parse_resource_info(struct package *pkg,
									   struct package_resource *pkg_res)
{
	my_arc_entry_t *arc_entry;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	arc_entry = (my_arc_entry_t *)pkg_res->actual_index_entry;
	wcscpy((WCHAR *)pkg_res->name, arc_entry->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = arc_entry->comprlen;
	pkg_res->actual_data_length = arc_entry->uncomprlen;
	pkg_res->offset = arc_entry->offset;
	pkg_res->flags |= PKG_RES_FLAG_UNICODE;

	return 0;
}

static int ktg_arc_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;
	my_arc_entry_t *arc_entry;
	
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	arc_entry = (my_arc_entry_t *)pkg_res->actual_index_entry;
	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	if (arc_entry->flags > 3) {
		free(compr);
		return -CUI_EMATCH;
	}

	if (arc_entry->flags & 1) {
		for (unsigned int k = 0; k < comprlen; k++)
			compr[k] = ~compr[k];
	}

	// core:100028c7c
	if (arc_entry->flags & 2) {	
		uncomprlen = pkg_res->actual_data_length;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}

		lzss_uncompress(uncompr, uncomprlen, compr, comprlen);
		free(compr);
		compr = NULL;
	} else {	/* 1001bf70 */
		//printf("[unsupported %x]\n", arc_entry->flags);
		//exit(0);
		uncomprlen = 0;
		uncompr = NULL;
	}
	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

static int ktg_arc_save_resource(struct resource *res, 
									struct package_resource *pkg_res)
{
	if (!res || !pkg_res)
		return -CUI_EPARA;

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

static void ktg_arc_release_resource(struct package *pkg, 
									 struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}

	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void ktg_arc_release(struct package *pkg, 
							struct package_directory *pkg_dir)
{
	if (!pkg || !pkg_dir)
		return;

	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	package_set_private(pkg, NULL);
	pkg->pio->close(pkg);
}

static cui_ext_operation ktg_arc_operation = {
	ktg_arc_match,					/* match */
	ktg_arc_extract_directory,		/* extract_directory */
	ktg_arc_parse_resource_info,	/* parse_resource_info */
	ktg_arc_extract_resource,		/* extract_resource */
	ktg_arc_save_resource,			/* save_resource */
	ktg_arc_release_resource,		/* release_resource */
	ktg_arc_release					/* release */
};

int CALLBACK ktg_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".arc"), NULL, 
		NULL, &ktg_arc_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
