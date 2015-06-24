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
struct acui_information NEJII_cui_information = {
	NULL,					/* copyright */
	_T("NEJII"),			/* system */
	_T(".cdt .vdt .ovd .pdt .dat"),				/* package */
	_T("1.0.2"),			/* revision */
	_T("痴汉公贼"),			/* author */
	_T("2009-3-21 14:01"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];		// "RK1"
	u32 index_entries;
	u32 index_offset; 
} Xdt_header_t;

typedef struct {
	s8 name[16];
	u32 comprlen;
	u32 uncomprlen; 
	u32 is_compressed;
	u32 offset;
} Xdt_entry_t;

typedef struct {
	u32 index_entries;
} mk_header_t;

typedef struct {
	u32 length;
	u32 width;
	u32 height;
} map_header_t;
#pragma pack ()

static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static inline unsigned char getbit_le(unsigned char byte, unsigned int pos)
{
	return !!(byte & (1 << pos));
}

static void lzss_decompress(unsigned char *uncompr, DWORD *uncomprlen, 
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

				if (act_uncomprlen >= *uncomprlen)
					goto out;

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

					if (act_uncomprlen >= *uncomprlen)
						goto out;

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
	*uncomprlen = act_uncomprlen;
}

/*********************  *********************/

/* 封包匹配回调函数 */
static int NEJII_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->seek(pkg, 0 - sizeof(Xdt_header_t), IO_SEEK_END)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (pkg->pio->read(pkg, magic, 4)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "RK1", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int NEJII_extract_directory(struct package *pkg,
								   struct package_directory *pkg_dir)
{
	Xdt_entry_t *index_buffer;
	u32 index_entries;
	u32 index_offset; 
	unsigned int index_buffer_length;	

	if (pkg->pio->read(pkg, &index_entries, 4))
		return -CUI_EREAD;

	if (pkg->pio->read(pkg, &index_offset, 4))
		return -CUI_EREAD;

	index_buffer_length = index_entries * sizeof(Xdt_entry_t);
	index_buffer = (Xdt_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, index_buffer, index_buffer_length, index_offset, IO_SEEK_SET)) {
		free(index_buffer);
		return -CUI_EREAD;
	}
	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(Xdt_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int NEJII_parse_resource_info(struct package *pkg,
									 struct package_resource *pkg_res)
{
	Xdt_entry_t *Xdt_entry;

	Xdt_entry = (Xdt_entry_t *)pkg_res->actual_index_entry;
	strncpy(pkg_res->name, Xdt_entry->name, 16);
	pkg_res->name[16] = 0;
	pkg_res->name_length = strlen(pkg_res->name);		/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = Xdt_entry->comprlen;
	pkg_res->actual_data_length = Xdt_entry->uncomprlen;	/* 数据都是明文 */
	pkg_res->offset = Xdt_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int NEJII_extract_resource(struct package *pkg,
								  struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	Xdt_entry_t *Xdt_entry;

	compr = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(compr);
			return -CUI_EREADVEC;
	}

	BYTE *act_data;
	Xdt_entry = (Xdt_entry_t *)pkg_res->actual_index_entry;
	if (Xdt_entry->is_compressed) {
		DWORD act_uncomprlen;

		uncompr = (BYTE *)malloc(pkg_res->actual_data_length);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}
		act_uncomprlen = pkg_res->actual_data_length;
		lzss_decompress(uncompr, &act_uncomprlen, compr, pkg_res->raw_data_length);
		free(compr);
		compr = NULL;
		if (act_uncomprlen != pkg_res->actual_data_length) {
			free(uncompr);
			return -CUI_EUNCOMPR;
		}
		act_data = uncompr;
	} else {
		uncompr = NULL;
		act_data = compr;
	}

	if (!strncmp((char *)act_data, "BM", 2)) {
		BITMAPFILEHEADER *bmh = (BITMAPFILEHEADER *)act_data;
		BITMAPINFOHEADER *bmif = (BITMAPINFOHEADER *)(bmh + 1);
		BYTE *rgba = act_data + bmh->bfOffBits;
		alpha_blending(rgba, bmif->biWidth, bmif->biHeight, bmif->biBitCount);
		// ビリビリパニック ぶる×かる的Im32.cdt
		if (bmif->biCompression == 3)
			bmif->biCompression = BI_RGB;
	}

	pkg_res->actual_data = uncompr;
	pkg_res->raw_data = compr;

	return 0;
}

/* 资源保存函数 */
static int NEJII_save_resource(struct resource *res, 
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
static void NEJII_release_resource(struct package *pkg, 
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
static void NEJII_release(struct package *pkg, 
						  struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation NEJII_operation = {
	NEJII_match,				/* match */
	NEJII_extract_directory,	/* extract_directory */
	NEJII_parse_resource_info,	/* parse_resource_info */
	NEJII_extract_resource,		/* extract_resource */
	NEJII_save_resource,		/* save_resource */
	NEJII_release_resource,		/* release_resource */
	NEJII_release				/* release */
};

/********************* mk *********************/

/* 封包匹配回调函数 */
static int NEJII_mk_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	u32 index_entries;
	if (pkg->pio->read(pkg, &index_entries, 4)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (index_entries < 1 || index_entries >= 32) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	u32 fsize;
	pkg->pio->length_of(pkg, &fsize);
	if (fsize != sizeof(mk_header_t) + 32 + index_entries * (800 * 600)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int NEJII_mk_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	u32 index_entries;
	if (pkg->pio->readvec(pkg, &index_entries, 4, 0, IO_SEEK_SET))
		return -CUI_EREAD;

	BYTE *index = (BYTE *)malloc(32);
	if (!index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index, 32)) {
		free(index);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index;
	pkg_dir->directory_length = index_entries;
	pkg_dir->index_entry_length = 1;

	return 0;
}

/* 封包索引项解析函数 */
static int NEJII_mk_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	BYTE *mk_entry;

	mk_entry = (BYTE *)pkg_res->actual_index_entry;
	sprintf(pkg_res->name, "%02d", *mk_entry);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = 800 * 600;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = sizeof(mk_header_t) + 32 + pkg_res->index_number * (800 * 600);

	return 0;
}

/* 封包资源提取函数 */
static int NEJII_mk_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	BYTE *raw = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(raw);
			return -CUI_EREADVEC;
	}

	if (MyBuildBMPFile(raw, pkg_res->raw_data_length, NULL, 0, 800, 600, 8, 
			(BYTE **)&pkg_res->actual_data,	&pkg_res->actual_data_length, my_malloc)) {
		free(raw);
		return -CUI_EMEM;
	}

	pkg_res->raw_data = raw;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation NEJII_mk_operation = {
	NEJII_mk_match,					/* match */
	NEJII_mk_extract_directory,		/* extract_directory */
	NEJII_mk_parse_resource_info,	/* parse_resource_info */
	NEJII_mk_extract_resource,		/* extract_resource */
	NEJII_save_resource,			/* save_resource */
	NEJII_release_resource,			/* release_resource */
	NEJII_release					/* release */
};

#if 0
/********************* map *********************/

static int NEJII_map_extract_directory(struct package *pkg,
									   struct package_directory *pkg_dir);

/* 封包匹配回调函数 */
static int NEJII_map_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	struct package_directory pkg_dir;
	int ret = NEJII_map_extract_directory(pkg, &pkg_dir);
	if (!ret)
		free(pkg_dir.directory);

	return ret;	
}

/* 封包索引目录提取函数 */
static int NEJII_map_extract_directory(struct package *pkg,
									   struct package_directory *pkg_dir)
{
	u32 fsize;
	pkg->pio->length_of(pkg, &fsize);
	DWORD offset = 0;
	for (DWORD i = 0; offset < fsize; ++i) {
		map_header_t header;

		if (pkg->pio->readvec(pkg, &header, sizeof(header), offset, IO_SEEK_SET))
			return -CUI_EREADVEC;
		
		offset += sizeof(header) + header.length;
	}
	if (offset != fsize)
		return -CUI_EMATCH;

	DWORD index_len = i * sizeof(map_header_t);
	map_header_t *index = (map_header_t *)malloc(index_len);
	if (!index)
		return -CUI_EMEM;

	DWORD index_entries = i;
	offset = 0;
	for (i = 0; i < index_entries; ++i) {
		if (pkg->pio->readvec(pkg, index + i, sizeof(map_header_t), offset, IO_SEEK_SET))
			break;
		
		offset += sizeof(map_header_t) + index[i].length;
	}
	if (i != index_entries) {
		free(index);
		return -CUI_EREADVEC;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index;
	pkg_dir->directory_length = index_len;
	pkg_dir->index_entry_length = sizeof(map_header_t);
	package_set_private(pkg, 0);

	return 0;
}

/* 封包索引项解析函数 */
static int NEJII_map_parse_resource_info(struct package *pkg,
										 struct package_resource *pkg_res)
{
	map_header_t *entry;

	u32 base_offset = (u32)package_get_private(pkg);
	entry = (map_header_t *)pkg_res->actual_index_entry;
	sprintf(pkg_res->name, "%02d", pkg_res->index_number);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = base_offset;
	package_set_private(pkg, base_offset + entry->length + sizeof(map_header_t));

	return 0;
}

/* 封包资源提取函数 */
static int NEJII_map_extract_resource(struct package *pkg,
									  struct package_resource *pkg_res)
{
	BYTE *raw = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(raw);
			return -CUI_EREADVEC;
	}

	map_header_t *entry = (map_header_t *)pkg_res->actual_index_entry;
	if (MyBuildBMPFile(raw, pkg_res->raw_data_length, NULL, 0, entry->width, entry->height, 
			entry->length * 8 / entry->width / entry->height, (BYTE **)&pkg_res->actual_data,	
			&pkg_res->actual_data_length, my_malloc)) {
		free(raw);
		return -CUI_EMEM;
	}

	pkg_res->raw_data = raw;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation NEJII_map_operation = {
	NEJII_map_match,				/* match */
	NEJII_map_extract_directory,	/* extract_directory */
	NEJII_map_parse_resource_info,	/* parse_resource_info */
	NEJII_map_extract_resource,		/* extract_resource */
	NEJII_save_resource,			/* save_resource */
	NEJII_release_resource,			/* release_resource */
	NEJII_release					/* release */
};
#endif

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK NEJII_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".cdt"), NULL, 
		NULL, &NEJII_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".vdt"), NULL, 
		NULL, &NEJII_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".ovd"), NULL, 
		NULL, &NEJII_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".pdt"), NULL, 
		NULL, &NEJII_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &NEJII_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

//	if (callback->add_extension(callback->cui, _T(".map"), _T(".bmp"), 
//		NULL, &NEJII_map_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
//			return -1;

	return 0;
}
