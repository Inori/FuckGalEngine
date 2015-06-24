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
struct acui_information rUGP_cui_information = {
	_T("age(アージュ)"),				/* copyright */
	_T("relic Unified Game Platform)"),	/* system */
	_T(".rio .ici"),					/* package */
	_T(""),								/* revision */
	_T(""),								/* author */
	_T(""),								/* date */
	NULL,								/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	u32 length0;	// 加密的数据长度(验证用)
	u32 length;		// 加密的数据长度
} ici_header_t;

typedef struct {
	u8 data[32];
	u16 crc;
} ici_data_t;

typedef struct {
	u32 magic;
	u16 tag0;		// 0x14
	u16 tag1;
} rio_header_t;

typedef struct {
	u16 tag;		// 0xffff, ogg: 0x8058, png: 0x8007, wmv: 0x8251, font: 0x8005, rfr: 0x800c
	u16 version;	// schema
	u16 name_length;
	u8 *name;
} rio_class_t;
#pragma pack ()

/*********************ici *********************/

/* 封包匹配回调函数 */
static int rUGP_ici_match(struct package *pkg)
{
	ici_header_t ici_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &ici_header, sizeof(ici_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (~(ici_header.length0 ^ 0xc92e568b) 
			!= ((ici_header.length ^ 0xc92e568f) >> 3)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包资源提取函数 */
static int rUGP_ici_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	ici_header_t ici_header;

	if (pkg->pio->readvec(pkg, &ici_header, sizeof(ici_header),
		0, IO_SEEK_SET))
			return -CUI_EREADVEC;

	ici_header.length0 = ~(ici_header.length0 ^ 0xc92e568b);
	ici_header.length = (ici_header.length ^ 0xc92e568f) >> 3;

	BYTE *enc = (BYTE *)malloc(ici_header.length);
	if (!enc)
		return -CUI_EMEM;

	DWORD seed = 0xb29d5a0c;
	for (DWORD i = 0; i < ici_header.length / 32; i++) {
		ici_data_t ici_data;
		u16 crc = 0;

		if (pkg->pio->read(pkg, &ici_data, sizeof(ici_data_t)))
			break;

		for (DWORD n = 0; n < 32; n++) {
			ici_data.data[n] ^= seed;			
			crc = (u16)((32 - n) * ici_data.data[n]) + crc;
			seed = ~(((seed >> 15) & 1) + 2 * seed - 0x5c4c8937);
		}
		if (ici_data.crc != crc)
			break;

		memcpy(enc + i * 32, ici_data.data, 32);
	}
	if (i != ici_header.length / 32) {
		free(enc);
		return -CUI_EMATCH;
	}

	BYTE *dec = (BYTE *)malloc(ici_header.length);
	if (!dec) {
		free(enc);
		return -CUI_EMEM;
	}

	BYTE *p = dec;
	DWORD count = ici_header.length / 6;
	for (i = 0; i < count; i++) {
		*p++ = enc[count * 0 + i];
		*p++ = enc[count * 1 + i];
		*p++ = enc[count * 2 + i];
		*p++ = enc[count * 3 + i];
		*p++ = enc[count * 4 + i];
		*p++ = enc[count * 5 + i];
	}

	for (i = 0; i < ici_header.length % 6; i++)
		*p++ = enc[count * 6 + i];

	BYTE d = 0;
	for (i = 0; i < ici_header.length; i++) {
		BYTE tmp = dec[i] - d;
		d = dec[i];
		dec[i] = tmp ^ 0xa5;
	}

	p = enc;
	count = ici_header.length / 5;
	for (i = 0; i < count; i++) {
		*p++ = dec[count * 0 + i];
		*p++ = dec[count * 1 + i];
		*p++ = dec[count * 2 + i];
		*p++ = dec[count * 3 + i];
		*p++ = dec[count * 4 + i];
	}

	for (i = 0; i < ici_header.length % 5; i++)
		*p++ = dec[count * 5 + i];

	d = 0;
	for (int k = ici_header.length - 1; k >= 0; k--) {
		BYTE tmp = enc[k] - d;
		d = enc[k];
		enc[k] = tmp;
	}

	p = dec;
	count = ici_header.length / 3;
	for (i = 0; i < count; i++) {
		*p++ = enc[count * 0 + i] ^ 0x18;
		*p++ = enc[count * 1 + i] ^ 0x3f;
		*p++ = enc[count * 2 + i] ^ 0xe2;
	}

	for (i = 0; i < ici_header.length % 3; i++)
		*p++ = enc[count * 3 + i];

	free(enc);

	pkg_res->actual_data = dec;
	pkg_res->actual_data_length = ici_header.length;

	return 0;
}

/* 资源保存函数 */
static int rUGP_ici_save_resource(struct resource *res, 
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
static void rUGP_ici_release_resource(struct package *pkg, 
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
static void rUGP_ici_release(struct package *pkg, 
							 struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation rUGP_ici_operation = {
	rUGP_ici_match,				/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	rUGP_ici_extract_resource,	/* extract_resource */
	rUGP_ici_save_resource,		/* save_resource */
	rUGP_ici_release_resource,	/* release_resource */
	rUGP_ici_release			/* release */
};

/********************* rio *********************/

/* 封包匹配回调函数 */
static int Lambda_rio_match(struct package *pkg)
{
	u32 magic;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (magic != 0x596E32CD	// SummerDays
			&& magic != 0x32D49E0
			&& magic != 0x1EDB927C 
			&& magic != 0x29F6CBA4
			&& magic != 0x673CE92A // 君が望む永遠～Latest Edition～ .ici
			&& magic != 0xEF137E2D) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}





	return 0;	
}

/* 封包索引目录提取函数 */
static int Lambda_rio_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	dat_entry_t *index_buffer;
	unsigned int index_buffer_length;	
	unsigned int i;

	if (pkg->pio->read(pkg, &pkg_dir->index_entries, 4))
		return -CUI_EREAD;

	index_buffer_length = pkg_dir->index_entries * sizeof(dat_entry_t);
	index_buffer = (dat_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	dat_entry_t *index = index_buffer;
	for (i = 0; i < pkg_dir->index_entries; i++) {
		if (pkg->pio->read(pkg, &index->name_length, 4))
			break;

		if (pkg->pio->read(pkg, index->name, index->name_length))
			break;
		index->name[index->name_length] = 0;

		if (pkg->pio->read(pkg, &index->offset, 4))
			break;

		if (pkg->pio->read(pkg, &index->length, 4))
			break;

		index++;
	}
	if (i != pkg_dir->index_entries) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(dat_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int Lambda_rio_parse_resource_info(struct package *pkg,
									struct package_resource *pkg_res)
{
	dat_entry_t *dat_entry;

	dat_entry = (dat_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, dat_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = dat_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = dat_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int Lambda_rio_extract_resource(struct package *pkg,
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

	// .dst
	if (!strncmp((char *)pkg_res->raw_data, "OggS", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".ogg");
	} else if (!strncmp((char *)pkg_res->raw_data, "RIFF", 4)) {// .dse
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".wav");
	}

	return 0;
}

/* 封包卸载函数 */
static void Lambda_rio_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation Lambda_rio_operation = {
	Lambda_rio_match,				/* match */
	Lambda_rio_extract_directory,	/* extract_directory */
	Lambda_rio_parse_resource_info,	/* parse_resource_info */
	Lambda_rio_extract_resource,	/* extract_resource */
	Lambda_ici_save_resource,		/* save_resource */
	Lambda_ici_release_resource,	/* release_resource */
	Lambda_rio_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK rUGP_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".rio"), NULL, 
		NULL, &rUGP_rio_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".ici"), _T(".ici.dump"), 
		NULL, &rUGP_ici_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	return 0;
}
