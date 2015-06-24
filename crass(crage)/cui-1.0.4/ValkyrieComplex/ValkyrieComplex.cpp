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

struct acui_information ValkyrieComplex_cui_information = 
{
	_T("CIRCUS"),			/* copyright */
	NULL,					/* system */
	_T(".pak .pac"),		/* package */
	_T("1.0.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-5-30 1:06"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[24];		// "ＶＣ体験版" or "ＶＣ製品版"
	u32 index_entries;
	u32 index_length;
} pak_header_t;

typedef struct {
	s8 *name;			// 0(运行时使用)
	u32 name_length;
	u32 offset;
	u32 length;
} pak_entry_t;

typedef struct {
	u32 always_1;
	u32 index_entries;
	u32 data_offset;
	u32 file_size;
	u8 reserved[16];	// 0
} pac_header_t;

typedef struct {
	s8 name[32];
	u32 length;
	u32 offset;
	FILETIME time;
	u32 reserved[2];	// 0
} pac_entry_t;

typedef struct {
	u32 always_0;
	u32 index_entries;
	u32 data_length;
	u32 bpp;
	u32 always_1;
} lpc_header_t;

typedef struct {
	u32 width;
	u32 height;
	u32 length;
} lpc_info_t;
#pragma pack ()


static void *my_malloc(DWORD len)
{
	return new BYTE[len];
}

static void decrypt(BYTE *dat, DWORD len, DWORD key)
{
	for (DWORD i = 0; i < len; ++i)
		dat[i] ^= key;
}

static void cps_decrypt(BYTE *in, DWORD in_len)
{
	if (in_len < 776)
		return;

	DWORD len = in_len - 8;
	BYTE *dat = in + 8;
    DWORD key = len ? dat[len - 1] : 0;
    *(u32 *)(in + 4) ^= key;
	key = (key >> 22) & 0xFF;
    if (key < 128)
		key ^= 0xFF;

    BYTE *a = &dat[len - key];
    BYTE *b = &dat[len / 512] + key;
    BYTE tmp = *b;
    *b = *a;
    *a = tmp;

    a = &dat[key + 2 * (len / 512)];
    b = &dat[len - 2 * key];
    tmp = *a;
    *a = *b;
    *b = tmp;
}

static void cps_decompress0(BYTE *out, DWORD out_len, BYTE *in, DWORD in_len)
{
	cps_decrypt(in, in_len);
	memcpy(out, in + 4, out_len);
}

static void cps_decompress1(BYTE *out, DWORD out_len, BYTE *in, DWORD in_len)
{
	cps_decrypt(in, in_len);

	in += 4;
	DWORD b = 0;
	BYTE *e_out = out + out_len;
	while (out < e_out) {
		DWORD is_set, cnt = 0;

		for (DWORD k = 0; k < 4; ++k) {	// get 4 bits
			is_set = (*in >> b++) & 1;
			in += b / 8;
			b &= 7;
			cnt <<= 1;
			cnt |= is_set;
		}

		is_set = (*in >> b++) & 1;
		in += b / 8;
		b &= 7;

		if (is_set) {
			for (DWORD i = 0; i < cnt + 1; ++i) {
				BYTE v = 0;
				for (DWORD k = 0; k < 8; ++k) {	// get 8 bits
					is_set = (*in >> b++) & 1;
					in += b / 8;
					b &= 7;
					v <<= 1;
					v |= is_set;
				}
				*out++ = v;
			}
		} else {
			DWORD v = 0;
			for (DWORD k = 0; k < 8; ++k) {	// get 8 bits
				is_set = (*in >> b++) & 1;
				in += b / 8;
				b &= 7;
				v <<= 1;
				v |= is_set;
			}
			if (!cnt)
				continue;
			memset(out, v, cnt);
			out += cnt;
		}
	}

}

static void cps_decompress2(BYTE *out, DWORD out_len, BYTE *in, DWORD in_len)
{
	cps_decrypt(in, in_len);

	BYTE *compr = in + 38;
	out[out_len - 1] = in[36];
	DWORD b = 0;
	for (DWORD i = 0; i < out_len / 2; ++i) {
		BYTE is_set = (*compr >> b++) & 1;
		compr += b / 8;
		b &= 7;
		u16 v = 0;
		if (is_set) {
			for (DWORD k = 0; k < 16; ++k) {	// get 16 bits
				is_set = (*compr >> b++) & 1;
				compr += b / 8;
				b &= 7;
				v <<= 1;
				v |= is_set;
			}
		} else {
			for (DWORD k = 0; k < 15; ++k) {
				is_set = (*compr >> b++) & 1;
				compr += b / 8;
				b &= 7;
				if (is_set)
					break;
			}
			v = *(u16 *)&in[4 + 2 * k];
		}
		*(u16 *)out = v;
		out += 2;
	}
}

static void cps_decompress3(BYTE *out, DWORD out_len, BYTE *in, DWORD in_len)
{
	cps_decrypt(in, in_len);

	in += 4;
	if (out_len <= 128)
		memcpy(out, in, out_len);
	else {
		memcpy(out, in, 128);
		in += 128;
		out += 128;

		DWORD b = 0;
		BYTE *e_out = out + out_len - 128;

		while (out < e_out) {
			BYTE is_set = (*in >> b++) & 1;
			in += b / 8;
			b &= 7;

			if (is_set) {
				DWORD off = 0;
				for (DWORD k = 0; k < 7; ++k) {	// get 7 bits
					is_set = (*in >> b++) & 1;
					in += b / 8;
					b &= 7;
					off <<= 1;
					off |= is_set;
				}

				DWORD cnt = 0;
				for (k = 0; k < 4; ++k) {	// get 4 bits
					is_set = (*in >> b++) & 1;
					in += b / 8;
					b &= 7;
					cnt <<= 1;
					cnt |= is_set;
				}
				cnt += 2;

				BYTE *src = out - off - 1;
				for (DWORD i = 0; i < cnt; ++i)
					*out++ = *src++;
			} else {
				BYTE v = 0;
				for (DWORD k = 0; k < 8; ++k) {	// get 8 bits
					is_set = (*in >> b++) & 1;
					in += b / 8;
					b &= 7;
					v <<= 1;
					v |= is_set;
				}
				*out++ = v;
			}
		}
	}
}

static int cps_decompress(BYTE **out, DWORD *out_len, BYTE *in, DWORD in_len)
{
	u32 mode = *(u32 *)in >> 28;

	if (mode > 3)
		return -CUI_EMATCH;

	u32 uncomprlen = (*(u32 *)in ^ 0xA415FCF) & 0xfffffff;
	BYTE *uncompr = new BYTE[uncomprlen + 4096];
	if (!uncompr)
		return -CUI_EMEM;
	memset(uncompr, 0, uncomprlen);

	switch (mode) {
	case 0:
		cps_decompress0(uncompr, uncomprlen, in, in_len);
		break;
	case 1:
		cps_decompress1(uncompr, uncomprlen, in, in_len);
		break;
	case 2:
		cps_decompress2(uncompr, uncomprlen, in, in_len);
		break;
	case 3:
		cps_decompress3(uncompr, uncomprlen, in, in_len);
		break;
	default:
		break;
	}

	*out = uncompr;
	*out_len = uncomprlen;

	return 0;
}

static u32 lpc_palette[256];

static void lpc_decompress32(BYTE *out, BYTE *in, DWORD width, DWORD height)
{
	DWORD line_len = width * 4;
	for (DWORD y = 0; y < height; ++y) {
		DWORD x = 0;
		while (1) {
			if (*in) {
				if (x < width) {
					u8 *dst = out + 4 * x;
					*(u32 *)dst = lpc_palette[in[1]];
					dst[3] = *in;
				}
				++x;
				in += 2;
			} else {
				++in;
				x += *in;
				if (!*in++)
					break;
			}
		}
		out += line_len;
	}
}

static void lpc_decompress24(BYTE *out, BYTE *in, DWORD width, DWORD height)
{
	DWORD line_len = width * 3;
	for (DWORD y = 0; y < height; ++y) {
		DWORD x = 0;
		while (1) {
			if (*in) {
				if (x < width) {
					u8 *dst = out + 3 * x;
					*(u32 *)dst = lpc_palette[in[1]];
				}
				++x;
				in += 2;
			} else {
				++in;
				x += *in;
				if (!*in++)
					break;
			}
		}
		out += line_len;
	}
}

static void lpc_decompress(BYTE *out, BYTE *in, DWORD width, DWORD height, DWORD bpp)
{
	if (bpp == 24)
		lpc_decompress24(out, in, width, height);
	else
		lpc_decompress32(out, in, width, height);
}

/********************* pak *********************/

/* 封包匹配回调函数 */
static int ValkyrieComplex_pak_match(struct package *pkg)
{
	pak_header_t header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &header, sizeof(header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	int is_retail;
	if (!strcmp(header.magic, "倁俠懱尡斉"))
		is_retail = 0;
	else if (!strcmp(header.magic, "倁俠惢昳斉"))
		is_retail = 1;
	else {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}
	package_set_private(pkg, is_retail);

	return 0;	
}

/* 封包索引目录提取函数 */
static int ValkyrieComplex_pak_extract_directory(struct package *pkg,
												 struct package_directory *pkg_dir)
{
	pak_header_t header;
	if (pkg->pio->readvec(pkg, &header, sizeof(header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	int is_retail = (int)package_get_private(pkg);

	if (is_retail)
		decrypt((BYTE *)&header, sizeof(header), 0x58);
	else
		decrypt((BYTE *)&header, sizeof(header), 0x38);	

	BYTE *index = new BYTE[header.index_length];
	if (!index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index, header.index_length)) {
		delete [] index;
		return -CUI_EREAD;
	}

	if (is_retail)
		decrypt(index, header.index_length, 0x58);
	else
		decrypt(index, header.index_length, 0x38);

	pak_entry_t *p = (pak_entry_t *)index;
	char *name = (char *)(index + sizeof(pak_entry_t) * header.index_entries);
	for (DWORD i = 0; i < header.index_entries; ++i) {
		p[i].name = name;
		name += p[i].name_length + 1;
	}

	pkg_dir->index_entries = header.index_entries;
	pkg_dir->directory = index;
	pkg_dir->directory_length = sizeof(pak_entry_t) * header.index_entries;
	pkg_dir->index_entry_length = sizeof(pak_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int ValkyrieComplex_pak_parse_resource_info(struct package *pkg,
												   struct package_resource *pkg_res)
{
	pak_entry_t *pak_entry;

	pak_entry = (pak_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, pak_entry->name);
	pkg_res->name_length = pak_entry->name_length;
	pkg_res->raw_data_length = pak_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = pak_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int ValkyrieComplex_pak_extract_resource(struct package *pkg,
												struct package_resource *pkg_res)
{
	BYTE *raw = new BYTE[pkg_res->raw_data_length];
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			delete [] raw;
			return -CUI_EREADVEC;
	}

	int is_retail = (int)package_get_private(pkg);

	if (is_retail)
		decrypt(raw, pkg_res->raw_data_length, 0x24);
	else
		decrypt(raw, pkg_res->raw_data_length, 0x25);

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = raw;
		return 0;
	}

	if (strstr( ".CS.CSV", pkg_res->name)) {	// for script
		for (DWORD i = 0; i < pkg_res->raw_data_length; ++i)
			--raw[i];
	} else if (strstr(pkg_res->name, ".CPS")) {
		int ret = cps_decompress((BYTE **)&pkg_res->actual_data, 
			&pkg_res->actual_data_length, raw, pkg_res->raw_data_length);
		if (ret) {
			delete [] raw;
			return -CUI_EMEM;
		}

		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		if (!strncmp((char *)pkg_res->actual_data, "BM", 2))			
			pkg_res->replace_extension = _T(".bmp");
		else
			pkg_res->replace_extension = _T(".tga");
	}

	pkg_res->raw_data = raw;

	return 0;
}

/* 资源保存函数 */
static int ValkyrieComplex_pak_save_resource(struct resource *res, 
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
static void ValkyrieComplex_pak_release_resource(struct package *pkg, 
												 struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		delete [] pkg_res->actual_data;
		pkg_res->actual_data = NULL;
	}
	if (pkg_res->raw_data) {
		delete [] pkg_res->raw_data;
		pkg_res->raw_data = NULL;
	}
}

/* 封包卸载函数 */
static void ValkyrieComplex_pak_release(struct package *pkg, 
										struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation ValkyrieComplex_pak_operation = {
	ValkyrieComplex_pak_match,				/* match */
	ValkyrieComplex_pak_extract_directory,	/* extract_directory */
	ValkyrieComplex_pak_parse_resource_info,/* parse_resource_info */
	ValkyrieComplex_pak_extract_resource,	/* extract_resource */
	ValkyrieComplex_pak_save_resource,		/* save_resource */
	ValkyrieComplex_pak_release_resource,	/* release_resource */
	ValkyrieComplex_pak_release				/* release */
};

/********************* pac *********************/

/* 封包匹配回调函数 */
static int ValkyrieComplex_pac_match(struct package *pkg)
{
	pac_header_t header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &header, sizeof(header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	u32 fsize;
	pkg->pio->length_of(pkg, &fsize);

	if (header.always_1 != 1 || header.file_size != fsize) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int ValkyrieComplex_pac_extract_directory(struct package *pkg,
												 struct package_directory *pkg_dir)
{
	pac_header_t header;
	if (pkg->pio->readvec(pkg, &header, sizeof(header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD index_len = sizeof(pac_entry_t) * header.index_entries;
	pac_entry_t *index = new pac_entry_t[header.index_entries];
	if (!index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index, index_len)) {
		delete [] index;
		return -CUI_EREAD;
	}

	for (DWORD i = 0; i < header.index_entries; ++i)
		index[i].offset += header.data_offset;

	pkg_dir->index_entries = header.index_entries;
	pkg_dir->directory = index;
	pkg_dir->directory_length = index_len;
	pkg_dir->index_entry_length = sizeof(pac_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int ValkyrieComplex_pac_parse_resource_info(struct package *pkg,
												   struct package_resource *pkg_res)
{
	pac_entry_t *pac_entry;

	pac_entry = (pac_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, pac_entry->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = pac_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = pac_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int ValkyrieComplex_pac_extract_resource(struct package *pkg,
												struct package_resource *pkg_res)
{
	BYTE *raw = new BYTE[pkg_res->raw_data_length];
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			delete [] raw;
			return -CUI_EREADVEC;
	}

	pkg_res->raw_data = raw;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation ValkyrieComplex_pac_operation = {
	ValkyrieComplex_pac_match,				/* match */
	ValkyrieComplex_pac_extract_directory,	/* extract_directory */
	ValkyrieComplex_pac_parse_resource_info,/* parse_resource_info */
	ValkyrieComplex_pac_extract_resource,	/* extract_resource */
	ValkyrieComplex_pak_save_resource,		/* save_resource */
	ValkyrieComplex_pak_release_resource,	/* release_resource */
	ValkyrieComplex_pak_release				/* release */
};

/********************* lpc *********************/

/* 封包匹配回调函数 */
static int ValkyrieComplex_lpc_match(struct package *pkg)
{
	lpc_header_t header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &header, sizeof(header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (header.always_0 || header.always_1 != 1) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int ValkyrieComplex_lpc_extract_directory(struct package *pkg,
												 struct package_directory *pkg_dir)
{
	lpc_header_t header;
	if (pkg->pio->readvec(pkg, &header, sizeof(header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	if (pkg->pio->read(pkg, lpc_palette, 1024))
		return -CUI_EREAD;

	u32 *offset_table = new u32[384];
	if (!offset_table)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, offset_table, 1536)) {
		delete [] offset_table;
		return -CUI_EREAD;
	}

	for (DWORD i = 0; i < header.index_entries + 1; ++i)
		offset_table[i] += sizeof(header) + 1024 + 1536;

	pkg_dir->index_entries = header.index_entries;
	pkg_dir->directory = offset_table;
	pkg_dir->directory_length = (header.index_entries) * 4;
	pkg_dir->index_entry_length = 4;
	package_set_private(pkg, header.bpp);

	return 0;
}

/* 封包索引项解析函数 */
static int ValkyrieComplex_lpc_parse_resource_info(struct package *pkg,
												   struct package_resource *pkg_res)
{
	u32 *offset = (u32 *)pkg_res->actual_index_entry;

	sprintf(pkg_res->name, "%03d", pkg_res->index_number);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = offset[1] - offset[0];
	pkg_res->actual_data_length = 0;
	pkg_res->offset = *offset;

	return 0;
}

/* 封包资源提取函数 */
static int ValkyrieComplex_lpc_extract_resource(struct package *pkg,
												struct package_resource *pkg_res)
{
	lpc_info_t info;
	if (pkg->pio->readvec(pkg, &info, sizeof(info), pkg_res->offset, IO_SEEK_SET))
		return -CUI_EREADVEC;

	BYTE *compr = new BYTE[info.length];
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, info.length,
		pkg_res->offset + sizeof(info), IO_SEEK_SET)) {
			delete [] compr;
			return -CUI_EREADVEC;
	}

	DWORD bpp = (DWORD)package_get_private(pkg);
	DWORD uncomprlen = info.width * info.height * bpp / 8;
	BYTE *uncompr = new BYTE[uncomprlen + 1];
	if (!uncompr) {
		delete [] compr;
		return -CUI_EMEM;
	}
	memset(uncompr, 0, uncomprlen);

	lpc_decompress(uncompr, compr, info.width, info.height, bpp);

	if (MyBuildBMPFile(uncompr, uncomprlen, NULL, 0, info.width, 0-info.height, bpp,
			(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, my_malloc)) {
		delete [] uncompr;
		delete [] compr;
		return -CUI_EMEM;	
	}
	delete [] uncompr;
	pkg_res->raw_data = compr;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation ValkyrieComplex_lpc_operation = {
	ValkyrieComplex_lpc_match,				/* match */
	ValkyrieComplex_lpc_extract_directory,	/* extract_directory */
	ValkyrieComplex_lpc_parse_resource_info,/* parse_resource_info */
	ValkyrieComplex_lpc_extract_resource,	/* extract_resource */
	ValkyrieComplex_pak_save_resource,		/* save_resource */
	ValkyrieComplex_pak_release_resource,	/* release_resource */
	ValkyrieComplex_pak_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK ValkyrieComplex_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".pak"), NULL, 
		NULL, &ValkyrieComplex_pak_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
		return -1;

	if (callback->add_extension(callback->cui, _T(".pac"), NULL, 
		NULL, &ValkyrieComplex_pac_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
		return -1;

	if (callback->add_extension(callback->cui, _T(".lpc"), _T(".bmp"), 
		NULL, &ValkyrieComplex_lpc_operation, CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
		return -1;

	return 0;
}

