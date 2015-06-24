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
#include <zlib.h>

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information ADVEngineMoon_cui_information = {
	NULL,					/* copyright */
	NULL,					/* system */
	_T(""),					/* package */
	_T("0.9.0"),			/* revision */
	_T("绫波微步"),			/* author */
	_T("2008-2-4 13:12"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[8];				// "arc"
	u32 index_entries;			// 资源文件数
	u32 data_offset;			// 数据段的偏移
	u32 compr_index_length;		// 压缩的索引段长度
	u32 uncompr_index_length;	// 解压缩后的索引段长度
} arc_header_t;

typedef struct {
	s8 name[30];
	u32 offset;
	u32 length;
} arc_entry_t;

typedef struct {
	s8 magic[4];		// "GPS"
	u32 unknown0;		// -1
	FILETIME time_stamp;
	u8 compress_method;
	u32 uncomprlen;
	u32 comprlen;
	u32 width;
	u32 height;
	u64 zero;
} gps_header_t;

typedef struct {
	s8 magic[8];		// "ACD"
	u32 unknown;		
} acd_header_t;

typedef struct {
	s8 magic[8];		// "CMP"
	u32 offset;
	u8 unknown[256];
} cmp_header_t;
#pragma pack ()

static DWORD rle_uncompress(BYTE *uncompr, DWORD uncomprlen, 
							BYTE *compr, DWORD comprlen)
{
	unsigned int act_uncomprlen = 0;
	unsigned int curbyte = 0;

	while (1) {
		BYTE data[3];
		BYTE copy_bytes;

		if (curbyte == comprlen)
			break;
		data[0] = compr[curbyte++];
		if (curbyte == comprlen)
			break;
		data[1] = compr[curbyte++];
		if (curbyte == comprlen)
			break;
		data[2] = compr[curbyte++];

		if (curbyte == comprlen)
			break;

		copy_bytes = compr[curbyte++];
		for (DWORD i = 0; i < copy_bytes; i++) {
			if (act_uncomprlen == uncomprlen)
				continue;
			uncompr[act_uncomprlen++] = data[0];

			if (act_uncomprlen == uncomprlen)
				continue;
			uncompr[act_uncomprlen++] = data[1];
			
			if (act_uncomprlen == uncomprlen)
				continue;
			uncompr[act_uncomprlen++] = data[2];
		}
	}
	return act_uncomprlen;
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
			if (act_uncomprlen != uncomprlen)
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
				if (act_uncomprlen != uncomprlen)
					uncompr[act_uncomprlen++] = data;
				/* 输出的1字节放入滑动窗口 */
				win[nCurWindowByte++] = data;
				nCurWindowByte &= win_size - 1;	
			}
		}
	}

	return act_uncomprlen;
}

/********************* *********************/

/* 封包匹配回调函数 */
static int ADVEngineMoon_match(struct package *pkg)
{
	s8 magic[8];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "arc", 8)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int ADVEngineMoon_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	arc_header_t arc_header;

	if (pkg->pio->readvec(pkg, &arc_header, sizeof(arc_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	BYTE *compr = (BYTE *)malloc(arc_header.compr_index_length);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, compr, arc_header.compr_index_length)) {
		free(compr);
		return -CUI_EREAD;
	}

	BYTE *uncompr = (BYTE *)malloc(arc_header.uncompr_index_length);
	if (!uncompr) {
		free(compr);
		return -CUI_EMEM;
	}

	lzss_uncompress(uncompr, arc_header.uncompr_index_length, compr, arc_header.compr_index_length);
	free(compr);

	arc_entry_t *entry = (arc_entry_t *)uncompr;
	for (DWORD i = 0; i < arc_header.index_entries; i++) {
		entry->offset += arc_header.data_offset;
		entry++;
	}

	pkg_dir->index_entries = arc_header.index_entries;
	pkg_dir->directory = uncompr;
	pkg_dir->directory_length = arc_header.uncompr_index_length;
	pkg_dir->index_entry_length = sizeof(arc_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int ADVEngineMoon_parse_resource_info(struct package *pkg,
									struct package_resource *pkg_res)
{
	arc_entry_t *arc_entry;

	arc_entry = (arc_entry_t *)pkg_res->actual_index_entry;
	if (!arc_entry->name[29])
		strcpy(pkg_res->name, arc_entry->name);
	else
		strncpy(pkg_res->name, arc_entry->name, 30);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = arc_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = arc_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int ADVEngineMoon_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr, *act_data;
	DWORD comprlen, uncomprlen, act_data_len;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)pkg->pio->readvec_only(pkg, comprlen, pkg_res->offset, IO_SEEK_SET);
	if (!compr)
		return -CUI_EREADVECONLY;

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = compr;
		return 0;
	}
	
	if (!strcmp((char *)compr, "GPS")) {
		gps_header_t *gps_header = (gps_header_t *)compr;
		BYTE *tmp_uncompr;
		DWORD tmp_uncomprlen;

		uncomprlen = gps_header->uncomprlen;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr)
			return -CUI_EMEM;

		switch (gps_header->compress_method) {
		case 0:
			memcpy(uncompr, (BYTE *)(gps_header + 1), gps_header->uncomprlen);
			break;
		case 1:	// RLE
			uncomprlen = rle_uncompress(uncompr, uncomprlen, (BYTE *)(gps_header + 1), gps_header->comprlen);
			break;
		case 2:	// LZSS
			uncomprlen = lzss_uncompress(uncompr, uncomprlen, (BYTE *)(gps_header + 1), gps_header->comprlen);
			break;
		case 3:	// LZSS+RLE	
			tmp_uncomprlen = uncomprlen * 2;
			tmp_uncompr = (BYTE *)malloc(tmp_uncomprlen);
			if (!tmp_uncompr) {
				free(uncompr);
				return -CUI_EMEM;
			}
			tmp_uncomprlen = lzss_uncompress(tmp_uncompr, tmp_uncomprlen, (BYTE *)(gps_header + 1), gps_header->comprlen);
			uncomprlen = rle_uncompress(uncompr, uncomprlen, tmp_uncompr, tmp_uncomprlen);
			free(tmp_uncompr);
			break;
		}
		act_data = uncompr;
		act_data_len = uncomprlen;
	} else if (!strcmp((char *)compr, "ACD")) {
		acd_header_t *acd_header = (acd_header_t *)compr;
		BYTE *p = (BYTE *)(acd_header + 1);

		uncomprlen = comprlen - sizeof(acd_header_t);
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr)
			return -CUI_EMEM;
		
		for (DWORD i = 0; i < uncomprlen; i++)
			uncompr[i] = 0xff - p[i];		
	} else if (!strcmp((char *)compr, "CMP")) {
		cmp_header_t *cmp_header = (cmp_header_t *)compr;
		gps_header_t *gps_header = (gps_header_t *)(compr + cmp_header->offset);
		BYTE *tmp_uncompr;
		DWORD tmp_uncomprlen;

		uncomprlen = gps_header->uncomprlen;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr)
			return -CUI_EMEM;

		switch (gps_header->compress_method) {
		case 0:
			memcpy(uncompr, (BYTE *)(gps_header + 1), gps_header->uncomprlen);
			break;
		case 1:	// RLE
			uncomprlen = rle_uncompress(uncompr, uncomprlen, (BYTE *)(gps_header + 1), gps_header->comprlen);
			break;
		case 2:	// LZSS
			lzss_uncompress(uncompr, uncomprlen, (BYTE *)(gps_header + 1), gps_header->comprlen);
			break;
		case 3:	// LZSS+RLE
			tmp_uncomprlen = uncomprlen;
			tmp_uncompr = (BYTE *)malloc(tmp_uncomprlen);
			if (!tmp_uncompr) {
				free(uncompr);
				return -CUI_EMEM;
			}
			lzss_uncompress(uncompr, uncomprlen, (BYTE *)(gps_header + 1), gps_header->comprlen);
			tmp_uncomprlen = rle_uncompress(tmp_uncompr, tmp_uncomprlen, uncompr, uncomprlen);
			free(uncompr);
			uncompr = tmp_uncompr;
			uncomprlen = tmp_uncomprlen;
			break;
		}
		act_data = uncompr;
		act_data_len = uncomprlen;		
	} else {
		uncompr = NULL;
		uncomprlen = 0;
		act_data = compr;
		act_data_len = comprlen;
	}

	if (!strncmp((char *)act_data, "BM", 2)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp");
	}

	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

/* 资源保存函数 */
static int ADVEngineMoon_save_resource(struct resource *res, 
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
static void ADVEngineMoon_release_resource(struct package *pkg, 
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
static void ADVEngineMoon_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation ADVEngineMoon_operation = {
	ADVEngineMoon_match,					/* match */
	ADVEngineMoon_extract_directory,		/* extract_directory */
	ADVEngineMoon_parse_resource_info,		/* parse_resource_info */
	ADVEngineMoon_extract_resource,		/* extract_resource */
	ADVEngineMoon_save_resource,			/* save_resource */
	ADVEngineMoon_release_resource,		/* release_resource */
	ADVEngineMoon_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK ADVEngineMoon_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, NULL, NULL, 
		NULL, &ADVEngineMoon_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NOEXT))
			return -1;

	return 0;
}
