#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>

/* 支持资源预读 */

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information ARCX_cui_information = {
	NULL,					/* copyright */
	NULL,					/* system */
	_T(".arc"),				/* package */
	_T("1.0.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-1-28 23:59"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];		/* "ARCX" */
	u32 index_entries;
	u32 header_length;
	u32 data_offset;
} arc_header_t;

typedef struct {
	s8 name[100];
	u32 offset;
	u32 comprlen;
	u32 uncomprlen;
	u8 unknown[7];
	u8 is_compressed;
	u32 reserved[2];
} arc_entry_t;
#pragma pack ()

#if 1

static inline unsigned char getbit_le(unsigned char byte, unsigned int pos)
{
	return !!(byte & (1 << pos));
}

static DWORD lzss_uncompress(unsigned char *uncompr, DWORD uncomprlen, 
							unsigned char *compr, DWORD comprlen)
{
	unsigned int act_uncomprlen = 0;
	/* compr中的当前字节中的下一个扫描位的位置 */
	unsigned int curbit = 0;
	/* compr中的当前扫描字节 */
	unsigned int curbyte = 0;
	unsigned int nCurWindowByte = 0xfee;
	unsigned int win_size = 4096;
	BYTE win[4096];
	
	memset(win, 0, sizeof(win));
	while (1) {
		/* 如果为0, 表示接下来的1个字节原样输出 */
		BYTE flag;

		if (curbyte >= comprlen)
			break;

		flag = compr[curbyte++];
		for (curbit = 0; curbit < 8; curbit++) {
			if (getbit_le(flag, curbit)) {
				unsigned char data;

				if (curbyte >= comprlen)
					goto out;

			//	if (act_uncomprlen >= uncomprlen)
			//		goto out;

				data = compr[curbyte++];
				uncompr[act_uncomprlen++] = data;
				/* 输出的1字节放入滑动窗口 */
				win[nCurWindowByte++] = data;
				nCurWindowByte &= win_size - 1;
			} else {
				unsigned int copy_bytes, win_offset;
				unsigned int i;

				if (curbyte >= comprlen)
					goto out;
				win_offset = compr[curbyte++];

				if (curbyte >= comprlen)
					goto out;
				copy_bytes = compr[curbyte++];
				win_offset |= (copy_bytes >> 4) << 8;
				copy_bytes &= 0x0f;
				copy_bytes += 3;

				for (i = 0; i < copy_bytes; i++) {	
					unsigned char data;

			//		if (act_uncomprlen >= uncomprlen)
			//			goto out;

					data = win[(win_offset + i) & (win_size - 1)];
					uncompr[act_uncomprlen++] = data;		
					/* 输出的1字节放入滑动窗口 */
					win[nCurWindowByte++] = data;
					nCurWindowByte &= win_size - 1;	
				}
			}
		}
	}
out:
	return act_uncomprlen;
}
#else
static void lzss_uncompress(unsigned char *uncompr, DWORD uncomprlen, 
							unsigned char *compr, DWORD comprlen)
{
	unsigned int act_uncomprlen = 0;
	/* compr中的当前字节中的下一个扫描位的位置 */
	unsigned int curbit = 0;
	/* compr中的当前扫描字节 */
	unsigned int curbyte = 0;
	unsigned int nCurWindowByte = 0xfee;
	BYTE window[4096];
	WORD flag = 0;
	BYTE data;

	memset(window, 0, nCurWindowByte);
	while (1) {
		flag >>= 1;
		if (!(flag & 0x0100)) {
			if (curbyte >= comprlen)
				break;
			flag = compr[curbyte++] | 0xff00;
		}

		if (!(flag & 1)) {
			WORD offset, copy_bytes;

			if (curbyte + 1 >= comprlen)
				break;

			offset = compr[curbyte++];
			copy_bytes = compr[curbyte++];			
			offset |= (copy_bytes & 0xf0) << 4;
			copy_bytes &= 0x0f;
			copy_bytes += 3;

			for (unsigned int i = 0; i < copy_bytes; i++) {
				data = window[(offset + i) & 0xfff];
				uncompr[act_uncomprlen++] = data;		
				window[nCurWindowByte++] = data;
				nCurWindowByte &= 0xfff;	
			}
		} else {
			if (curbyte >= comprlen)
				break;
			data = compr[curbyte++];
			uncompr[act_uncomprlen++] = data;
			window[nCurWindowByte++] = data;
			nCurWindowByte &= 0xfff;
		}
	}
}
#endif
/********************* arc *********************/

/* 封包匹配回调函数 */
static int ARCX_arc_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "ARCX", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int ARCX_arc_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	arc_header_t arc_header;
	arc_entry_t *index_buffer;
	DWORD index_buffer_length;	

	if (pkg->pio->readvec(pkg, &arc_header, sizeof(arc_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	index_buffer_length = arc_header.index_entries * sizeof(arc_entry_t);
	index_buffer = (arc_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = arc_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(arc_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int ARCX_arc_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	arc_entry_t *arc_entry;

	arc_entry = (arc_entry_t *)pkg_res->actual_index_entry;
	strncpy(pkg_res->name, arc_entry->name, 64);
	pkg_res->name[64] = 0;
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = arc_entry->comprlen;
	if (arc_entry->is_compressed)
		pkg_res->actual_data_length = arc_entry->uncomprlen;
	else
		pkg_res->actual_data_length = 0;	
	pkg_res->offset = arc_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int ARCX_arc_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)pkg->pio->readvec_only(pkg, comprlen, pkg_res->offset, IO_SEEK_SET);
	if (!compr)
		return -CUI_EREADVECONLY;

	uncompr = NULL;
	uncomprlen = pkg_res->actual_data_length;
	if (uncomprlen) {
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr)
			return -CUI_EMEM;

		memset(uncompr, 0xdd, uncomprlen);
		lzss_uncompress(uncompr, uncomprlen, compr, comprlen);
	}
	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;

	return 0;
}

/* 资源保存函数 */
static int ARCX_arc_save_resource(struct resource *res, 
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
static void ARCX_arc_release_resource(struct package *pkg, 
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
static void ARCX_arc_release(struct package *pkg, 
							 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation ARCX_arc_operation = {
	ARCX_arc_match,					/* match */
	ARCX_arc_extract_directory,		/* extract_directory */
	ARCX_arc_parse_resource_info,	/* parse_resource_info */
	ARCX_arc_extract_resource,		/* extract_resource */
	ARCX_arc_save_resource,			/* save_resource */
	ARCX_arc_release_resource,		/* release_resource */
	ARCX_arc_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK ARCX_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".arc"), NULL, 
		NULL, &ARCX_arc_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
