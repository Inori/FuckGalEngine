#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>
#include <cui_common.h>

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information TEATIME_cui_information = {
	_T("TEATIME"),					/* copyright */
	_T(""),							/* system */
	_T(".bmp .tga .wav. ODF .TEA .txt .pak .idt .ODA"),	/* package */
	_T("1.0.4"),					/* revision */
	_T("痴漢公賊"),					/* author */
	_T("2009-6-27 13:37"),			/* date */
	NULL,							/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	u32 index_entries;
} pak_header_t;

typedef struct {
	u32 offset;
	u32 length;
	s8 name[260];
} pak_entry_t;
#pragma pack ()


static BYTE decrypt_table[15] = {
	0xFF, 0xFF, 0xFF, 0x15, 0x50, 0x0F, 0xFF, 0xFF, 
	0xFF, 0xCA, 0x02, 0xFF, 0x15, 0x50, 0xFF
};

/********************* bmp *********************/

/* 封包匹配回调函数 */
static int TEATIME_bmp_match(struct package *pkg)
{
	u8 magic[2];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (magic[0] != 0xbd || magic[1] != 0xb2) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int TEATIME_bmp_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	u32 bmp_size;

	if (pkg->pio->length_of(pkg, &bmp_size))
		return -CUI_ELEN;

	BYTE *dec = (BYTE *)malloc(bmp_size);
	if (!dec)
		return -CUI_EMEM;	

	if (pkg->pio->readvec(pkg, dec, bmp_size, 0, IO_SEEK_SET)) {
		free(dec);
		return -CUI_EREADVEC;
	}

	for (DWORD i = 0; i < bmp_size; i++)
		dec[i] ^= decrypt_table[i % 15];

	pkg_res->actual_data = dec;
	pkg_res->actual_data_length = bmp_size;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation TEATIME_bmp_operation = {
	TEATIME_bmp_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	TEATIME_bmp_extract_resource,/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* tga *********************/

/* 封包匹配回调函数 */
static int TEATIME_tga_match(struct package *pkg)
{
	u8 magic[2];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (magic[0] != 0xff || magic[1] != 0xff) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包处理回调函数集合 */
static cui_ext_operation TEATIME_tga_operation = {
	TEATIME_tga_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	TEATIME_bmp_extract_resource,/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* wav *********************/

/* 封包匹配回调函数 */
static int TEATIME_wav_match(struct package *pkg)
{
	u8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (magic[0] !=  0xad || magic[1] != 0xb6 || magic[2] != 0xb9 || magic[3] != 0x53) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包处理回调函数集合 */
static cui_ext_operation TEATIME_wav_operation = {
	TEATIME_wav_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	TEATIME_bmp_extract_resource,/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* ODF *********************/

/* 封包匹配回调函数 */
static int TEATIME_ODF_match(struct package *pkg)
{
	s8 magic[3];
	DWORD is_crypt;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "ODF", 3)) {
		u32 ODF_size;

		if (pkg->pio->length_of(pkg, &ODF_size)) {
			pkg->pio->close(pkg);
			return -CUI_ELEN;
		}

		BYTE decrypt_table[3][5] = {
			{ 0xAA, 0x5A, 0xFF, 0x81, 0xD9 },
			{ 0xAA, 0x8F, 0xFF, 0x81, 0xD9 },
			{ 0xAA, 0x5A, 0xFF, 0xAF, 0xF0 },
		};

		for (DWORD i = 0; i < 3; i++)
			magic[i] ^= decrypt_table[ODF_size % 3][i % 5] + i;

		if (strncmp(magic, "ODF", 3) && strncmp(magic, "ODA", 3) ) {
			pkg->pio->close(pkg);
			return -CUI_EMATCH;			
		}

		is_crypt = 1;
	} else
		is_crypt = 0;
	package_set_private(pkg, is_crypt);

	return 0;	
}

static int TEATIME_ODF_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	u32 ODF_size;
	DWORD is_crypt = package_get_private(pkg);

	if (pkg->pio->length_of(pkg, &ODF_size))
		return -CUI_ELEN;

	BYTE *dec = (BYTE *)malloc(ODF_size);
	if (!dec)
		return -CUI_EMEM;	

	if (pkg->pio->readvec(pkg, dec, ODF_size, 0, IO_SEEK_SET)) {
		free(dec);
		return -CUI_EREADVEC;
	}

	if (is_crypt) {
		BYTE decrypt_table[3][5] = {
			{ 0xAA, 0x5A, 0xFF, 0x81, 0xD0 },
			{ 0xAA, 0x8F, 0xFF, 0x81, 0xD0 },
			{ 0xAA, 0x5A, 0xFF, 0xAF, 0xF0 }
		};

		for (DWORD i = 0; i < ODF_size; i++)
			dec[i] ^= (decrypt_table[ODF_size % 3][i % 5] + i) & 0xff;
	}

	pkg_res->actual_data = dec;
	pkg_res->actual_data_length = ODF_size;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation TEATIME_ODF_operation = {
	TEATIME_ODF_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	TEATIME_ODF_extract_resource,/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* TEA *********************/

/* 封包匹配回调函数 */
static int TEATIME_TEA_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

static int TEATIME_TEA_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	u32 TEA_size;

	if (pkg->pio->length_of(pkg, &TEA_size))
		return -CUI_ELEN;

	BYTE *dec = (BYTE *)malloc(TEA_size);
	if (!dec)
		return -CUI_EMEM;	

	if (pkg->pio->readvec(pkg, dec, TEA_size, 0, IO_SEEK_SET)) {
		free(dec);
		return -CUI_EREADVEC;
	}
	
	if (dec[TEA_size - 1]) {
		BYTE decrypt_table[31] = {
			0xAA, 0x55, 0x1F, 0x68, 0x3D, 0xCC, 0x01, 0x40, 0xAA, 0xAB, 0xC1, 0xE1, 0x0F, 0xFF, 0xFF, 0xAA,
			0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0xAA, 0xAA, 0xAA, 0xAA, 0x00
		};
		int nlen = strlen((char *)decrypt_table);
		for (DWORD i = 0; i < TEA_size; i++)
			dec[i] ^= decrypt_table[i % nlen] + ((i * 31) & 0x7f);
	}


	pkg_res->actual_data = dec;
	pkg_res->actual_data_length = TEA_size;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation TEATIME_TEA_operation = {
	TEATIME_TEA_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	TEATIME_TEA_extract_resource,/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* txt *********************/

/* 封包匹配回调函数 */
static int TEATIME_txt_match(struct package *pkg)
{
	u32 txt_size;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->length_of(pkg, &txt_size)) {
		pkg->pio->close(pkg);
		return -CUI_ELEN;
	}

	BYTE flag;
	if (pkg->pio->readvec(pkg, &flag, 1, -1, IO_SEEK_END)) {
		pkg->pio->close(pkg);
		return -CUI_EREADVEC;
	}

	if (flag != '1') {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int TEATIME_txt_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	u32 txt_size;

	if (pkg->pio->length_of(pkg, &txt_size))
		return -CUI_ELEN;

	BYTE *dec = (BYTE *)malloc(txt_size);
	if (!dec)
		return -CUI_EMEM;	

	if (pkg->pio->readvec(pkg, dec, txt_size, 0, IO_SEEK_SET)) {
		free(dec);
		return -CUI_EREADVEC;
	}
	
	for (DWORD i = 0; i < txt_size - 1; i++)
		dec[i] = ~dec[i] ;
	dec[i] = '0';

	pkg_res->actual_data = dec;
	pkg_res->actual_data_length = txt_size;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation TEATIME_txt_operation = {
	TEATIME_txt_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	TEATIME_txt_extract_resource,/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* pak *********************/

/* 封包匹配回调函数 */
static int TEATIME_pak_match(struct package *pkg)
{
	u32 index_entries;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &index_entries, sizeof(index_entries))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (!index_entries) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int TEATIME_pak_extract_directory(struct package *pkg,
										 struct package_directory *pkg_dir)
{
	pak_header_t pak_header;
	unsigned int index_buffer_length;	

	if (pkg->pio->readvec(pkg, &pak_header, sizeof(pak_header),
			0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	index_buffer_length = pak_header.index_entries * sizeof(pak_entry_t);
	pak_entry_t *index_buffer = (pak_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = pak_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(pak_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int TEATIME_pak_parse_resource_info(struct package *pkg,
										   struct package_resource *pkg_res)
{
	pak_entry_t *pak_entry;

	pak_entry = (pak_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, pak_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = pak_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = pak_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int TEATIME_pak_extract_resource(struct package *pkg,
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

/* 封包处理回调函数集合 */
static cui_ext_operation TEATIME_pak_operation = {
	TEATIME_pak_match,					/* match */
	TEATIME_pak_extract_directory,		/* extract_directory */
	TEATIME_pak_parse_resource_info,	/* parse_resource_info */
	TEATIME_pak_extract_resource,		/* extract_resource */
	CUI_COMMON_OPERATION
};


/********************* idt *********************/

/* 封包匹配回调函数 */
static int TEATIME_idt_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包资源提取函数 */
static int TEATIME_idt_extract_resource(struct package *pkg,
										struct package_resource *pkg_res)
{
	DWORD data_len = pkg_res->raw_data_length;
	BYTE *enc = (BYTE *)pkg->pio->readvec_only(pkg, data_len, 0, IO_SEEK_SET);
	if (!enc)
		return -CUI_EREADVECONLY;

	BYTE *dec = (BYTE *)malloc(data_len);
	if (!dec)
		return -CUI_EMEM;
	
	for (DWORD i = 0; i < data_len; ++i) {
		BYTE dec_table[8];
		BYTE tmp = enc[i];

		for (DWORD j = 0; j < 8; ++j) {
			dec_table[j] = tmp << j;
			dec_table[j] = dec_table[j] >> 7;
			dec_table[j] <<= j;
		}
		tmp = 0;
		for (j = 0; j < 8; ++j)
			tmp |= dec_table[j];
		dec[i] = tmp;
	}

	pkg_res->actual_data = dec;
	pkg_res->actual_data_length = data_len;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation TEATIME_idt_operation = {
	TEATIME_idt_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	TEATIME_idt_extract_resource,	/* extract_resource */
	CUI_COMMON_OPERATION
};


/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK TEATIME_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".tga"), NULL, 
		NULL, &TEATIME_tga_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".bmp"), NULL, 
		NULL, &TEATIME_bmp_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".wav"), NULL, 
		NULL, &TEATIME_wav_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".ODA"), NULL, 
		NULL, &TEATIME_ODF_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".ODF"), NULL, 
		NULL, &TEATIME_ODF_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".TEA"), NULL, 
		NULL, &TEATIME_TEA_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".pak"), NULL, 
		NULL, &TEATIME_pak_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".txt"), NULL, 
		NULL, &TEATIME_txt_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".idt"), NULL, 
		NULL, &TEATIME_idt_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	return 0;
}
