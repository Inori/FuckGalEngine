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
struct acui_information AiSystem_cui_information = {
	_T("Studio AERO"),		/* copyright */
	_T("AiSystem ver 0.01"),/* system */
	_T(".ttd .bmp .stf"),	/* package */
	_T("1.0.2"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-3-13 21:31"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];		// ".FRC"
	u32 index_key;		// 解密索引段key
	u32 index_crc;		// 加密的索引段crc
	u32 index_entries;	// 资源数
	u32 sync;			// must be 5(header的4字节个数?)
} ttd_header_t;

typedef struct {
	u32 length;
	u32 offset;
	u32 ttd_id;	// （运行时参数）.ttd封包的数量是硬编码的；所属的ttd封包id也是事先定好的
	s8 name[32];
} ttd_entry_t;

typedef struct {
	s8 magic[4];		// "DSFF"
	u32 uncomprlen;
} dsff_header_t;
#pragma pack ()

// AUTODIN II是标准的以太网CRC校验多项式
extern u32 crc32(const u8 *buf, int len);

static void decode(BYTE *buf, unsigned int len, u32 key)
{
	for (unsigned int i = 0; i < (len / 4); i++)
		((u32 *)buf)[i] ^= key;
}


static void lzss_uncompress(BYTE *uncompr, DWORD uncomprlen, 
							BYTE *compr, DWORD comprlen)
{
	unsigned int act_uncomprlen = 0;
	/* compr中的当前字节中的下一个扫描位的位置 */
	unsigned int curbit = 0;
	/* compr中的当前扫描字节 */
	unsigned int curbyte = 0;
	unsigned int nCurWindowByte = 0xff0;
	unsigned int win_size = 4096;
	BYTE win[4096];
	WORD flag = 0;

	memset(win, 0, win_size);
	while (curbyte < comprlen) {
		flag >>= 1;
		if (!(flag & 0x0100))
			flag = compr[curbyte++] | 0xff00;

		if (flag & 1) {
			unsigned char data;

			data = compr[curbyte++];
			uncompr[act_uncomprlen++] = data;
			win[nCurWindowByte++] = data;
			nCurWindowByte &= win_size - 1;
		} else {
			unsigned int copy_bytes, win_offset;
			unsigned int i;

			win_offset = compr[curbyte++];

			if (curbyte >= comprlen)
				break;

			copy_bytes = compr[curbyte++];
			win_offset |= (copy_bytes & 0xf0) << 4;
			copy_bytes &= 0x0f;
			copy_bytes += 3;

			for (i = 0; i < copy_bytes; i++) {	
				unsigned char data;

				data = win[(win_offset + i) & (win_size - 1)];
				uncompr[act_uncomprlen++] = data;
				win[nCurWindowByte++] = data;
				nCurWindowByte &= win_size - 1;	
			}
		}
	}
}

/********************* ttd *********************/

/* 封包匹配回调函数 */
static int AiSystem_ttd_match(struct package *pkg)
{
	ttd_header_t ttd_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &ttd_header, sizeof(ttd_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(ttd_header.magic, ".FRC", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (ttd_header.sync != 5) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int AiSystem_ttd_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	ttd_header_t ttd_header;
	ttd_entry_t *index_buffer;
	unsigned int index_buffer_length;	

	if (pkg->pio->readvec(pkg, &ttd_header, sizeof(ttd_header), 
			0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	index_buffer_length = ttd_header.index_entries * sizeof(ttd_entry_t);
	index_buffer = (ttd_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	if (ttd_header.index_crc != crc32((u8 *)index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EMATCH;
	}

	decode((u8 *)index_buffer, index_buffer_length, ttd_header.index_key);

	pkg_dir->index_entries = ttd_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(ttd_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int AiSystem_ttd_parse_resource_info(struct package *pkg,
									struct package_resource *pkg_res)
{
	ttd_entry_t *ttd_entry;

	ttd_entry = (ttd_entry_t *)pkg_res->actual_index_entry;
	if (!ttd_entry->name[31])
		strcpy(pkg_res->name, ttd_entry->name);
	else
		strncpy(pkg_res->name, ttd_entry->name, 32);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = ttd_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = ttd_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int AiSystem_ttd_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	pkg_res->raw_data = malloc(pkg_res->raw_data_length);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EREADVEC;
	}

	return 0;
}

/* 资源保存函数 */
static int AiSystem_ttd_save_resource(struct resource *res, 
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
static void AiSystem_ttd_release_resource(struct package *pkg, 
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
static void AiSystem_ttd_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation AiSystem_ttd_operation = {
	AiSystem_ttd_match,					/* match */
	AiSystem_ttd_extract_directory,		/* extract_directory */
	AiSystem_ttd_parse_resource_info,		/* parse_resource_info */
	AiSystem_ttd_extract_resource,		/* extract_resource */
	AiSystem_ttd_save_resource,			/* save_resource */
	AiSystem_ttd_release_resource,		/* release_resource */
	AiSystem_ttd_release					/* release */
};

/********************* bmp *********************/

/* 封包匹配回调函数 */
static int AiSystem_bmp_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "DSFF", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int AiSystem_bmp_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;
	u32 bmp_size;

	if (pkg->pio->length_of(pkg, &bmp_size))
		return -CUI_ELEN;

	{
		comprlen = bmp_size - sizeof(dsff_header_t);
		compr = (BYTE *)malloc(comprlen);
		if (!compr)
			return -CUI_EMEM;

		if (pkg->pio->readvec(pkg, &uncomprlen, 4, 4, IO_SEEK_SET)) {
			free(compr);
			return -CUI_EREADVEC;
		}

		if (pkg->pio->readvec(pkg, compr, comprlen, 8, IO_SEEK_SET)) {
			free(compr);
			return -CUI_EREADVEC;
		}

		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}

		lzss_uncompress(uncompr, uncomprlen, compr, comprlen);
	}

	pkg_res->raw_data = compr;
	pkg_res->raw_data_length = comprlen;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

/* 封包卸载函数 */
static void AiSystem_bmp_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation AiSystem_bmp_operation = {
	AiSystem_bmp_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	AiSystem_bmp_extract_resource,	/* extract_resource */
	AiSystem_ttd_save_resource,		/* save_resource */
	AiSystem_ttd_release_resource,	/* release_resource */
	AiSystem_bmp_release			/* release */
};

/********************* bmp16 *********************/

/* 封包匹配回调函数 */
static int AiSystem_bmp16_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	BITMAPFILEHEADER bh;
	if (pkg->pio->read(pkg, &bh, sizeof(bh))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (bh.bfType != 'MB') {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	BITMAPINFOHEADER bi;
	if (pkg->pio->read(pkg, &bi, sizeof(bi))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (bi.biBitCount != 16 || bh.bfSize <= sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)
			+ ((bi.biWidth * 2 + 3) & ~3) * bi.biHeight) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包资源提取函数 */
static int AiSystem_bmp16_extract_resource(struct package *pkg,
										   struct package_resource *pkg_res)
{
	u32 bmp_size;

	if (pkg->pio->length_of(pkg, &bmp_size))
		return -CUI_ELEN;

	BYTE *bmp = (BYTE *)malloc(bmp_size);
	if (!bmp)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, bmp, bmp_size, 0, IO_SEEK_SET)) {
		free(bmp);
		return -CUI_EREADVEC;
	}

	BITMAPFILEHEADER *bh = (BITMAPFILEHEADER *)bmp;
	BITMAPINFOHEADER *bi = (BITMAPINFOHEADER *)(bh + 1);
	bi->biBitCount = 24;

	pkg_res->raw_data = bmp;
	pkg_res->raw_data_length = bmp_size;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation AiSystem_bmp16_operation = {
	AiSystem_bmp16_match,			/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	AiSystem_bmp16_extract_resource,/* extract_resource */
	AiSystem_ttd_save_resource,		/* save_resource */
	AiSystem_ttd_release_resource,	/* release_resource */
	AiSystem_bmp_release			/* release */
};

/********************* stf *********************/

/* 封包匹配回调函数 */
static int AiSystem_stf_match(struct package *pkg)
{
	s8 magic[4];
	unsigned long is_compressed;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (!strncmp(magic, "DSFF", 4))
		is_compressed  = 1;
	else
		is_compressed  = 0;

	package_set_private(pkg, is_compressed);

	return 0;	
}

/* 封包资源提取函数 */
static int AiSystem_stf_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;
	u32 stf_size;
	unsigned long is_compressed;

	is_compressed = package_get_private(pkg);

	if (pkg->pio->length_of(pkg, &stf_size))
		return -CUI_ELEN;

	if (!is_compressed) {
		comprlen = stf_size;
		compr = (BYTE *)malloc(comprlen);
		if (!compr)
			return -CUI_EMEM;

		if (pkg->pio->readvec(pkg, compr, comprlen, 0, IO_SEEK_SET)) {
			free(compr);
			return -CUI_EREADVEC;
		}

		uncomprlen = stf_size; 
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}
		memcpy(uncompr, compr, uncomprlen);
	} else {
		comprlen = stf_size - sizeof(dsff_header_t);
		compr = (BYTE *)malloc(comprlen);
		if (!compr)
			return -CUI_EMEM;

		if (pkg->pio->readvec(pkg, &uncomprlen, 4, 4, IO_SEEK_SET)) {
			free(compr);
			return -CUI_EREADVEC;
		}

		if (pkg->pio->readvec(pkg, compr, comprlen, 8, IO_SEEK_SET)) {
			free(compr);
			return -CUI_EREADVEC;
		}

		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}

		lzss_uncompress(uncompr, uncomprlen, compr, comprlen);
	}

	for (DWORD i = 0; i < uncomprlen; i++)
		uncompr[i] = ~uncompr[i];

	pkg_res->raw_data = compr;
	pkg_res->raw_data_length = comprlen;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}


/* 封包处理回调函数集合 */
static cui_ext_operation AiSystem_stf_operation = {
	AiSystem_stf_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	AiSystem_stf_extract_resource,	/* extract_resource */
	AiSystem_ttd_save_resource,		/* save_resource */
	AiSystem_ttd_release_resource,	/* release_resource */
	AiSystem_bmp_release			/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK AiSystem_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".bmp"), NULL, 
		NULL, &AiSystem_bmp16_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".ttd"), NULL, 
		NULL, &AiSystem_ttd_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".bmp"), NULL, 
		NULL, &AiSystem_bmp_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".stf"), NULL, 
		NULL, &AiSystem_stf_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	return 0;
}
