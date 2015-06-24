#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>
#include <zlib.h>

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information CatSystem2_cui_information = {
	_T(""),		/* copyright */
	_T("CatSystem2"),			/* system */
	_T(".int .hg2"),		/* package */
	_T(""),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T(""),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)

typedef struct {
	s8 magic[4];		// "HG-2"
	u32 sizeof_header;
	u16 mode;
	u16 pad;
} hg2_header_t;

typedef struct {
	u32 width;
	u32 height;
	u16 bpp;
	u16 key;
	u32 reserved[2];		// 0
	u32 rgb_comprlen;
	u32 rgb_uncomprlen;
	u32 alpha_comprlen;
	u32 uncomprlen2;
} hg2_info_t;

typedef struct {
	u32 sizeof_info2;
	u32 picture_number;		// 从0开始
	u32 width;
	u32 height;
	u32 unknown0[3];
	u32 next_info;			// 最后一个Info2的该字段为0，标识结束
	u32 unknown1[2];
} hg2_info2_t;
#pragma pack ()


static DWORD inline get_lenth(struct bits *bits)
{
	DWORD val = 0;
	for (DWORD i = 0; ; i++) {
		val |= bit_get_high(bits);		
		if (val)
			break;
		val <<= 1;
	}

	DWORD val2 = 0;
	for (DWORD k = 0; k < n; k++) {
		val2 |= bit_get_high(bits);
		val2 <<= 1;
	}

	return val | val2;
}

/********************* int *********************/

/* 封包匹配回调函数 */
static int CatSystem2_int_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "DMF ", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int CatSystem2_int_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
#if 0
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
#endif
	return 0;
}

/* 封包索引项解析函数 */
static int CatSystem2_int_parse_resource_info(struct package *pkg,
									struct package_resource *pkg_res)
{
#if 0
	dat_entry_t *dat_entry;

	dat_entry = (dat_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, dat_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = dat_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = dat_entry->offset;
#endif
	return 0;
}

/* 封包资源提取函数 */
static int CatSystem2_int_extract_resource(struct package *pkg,
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
static int CatSystem2_int_save_resource(struct resource *res, 
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
static void CatSystem2_int_release_resource(struct package *pkg, 
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
static void CatSystem2_int_release(struct package *pkg,
								   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation CatSystem2_int_operation = {
	CatSystem2_int_match,					/* match */
	CatSystem2_int_extract_directory,		/* extract_directory */
	CatSystem2_int_parse_resource_info,		/* parse_resource_info */
	CatSystem2_int_extract_resource,		/* extract_resource */
	CatSystem2_int_save_resource,			/* save_resource */
	CatSystem2_int_release_resource,		/* release_resource */
	CatSystem2_int_release					/* release */
};

/********************* hg2 *********************/

/* 封包匹配回调函数 */
static int CatSystem2_hg2_match(struct package *pkg)
{
	hg2_header_t hg2_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &hg2_header, sizeof(hg2_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(hg2_header.magic, "HG-2", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (hg2_header.mode != 0x10 && hg2_header.mode != 0x20 && hg2_header.mode != 0x25) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int CatSystem2_hg2_extract_resource(struct package *pkg,
										   struct package_resource *pkg_res)
{
	u32 hg2_size;
	if (pkg->pio->length_of(pkg, &hg2_size))
		return CUI_ELEN;

	BYTE *hg2 = new BYTE[hg2_size];
	if (!hg2)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, hg2, hg2_size, 0, IO_SEEK_SET)) {
		delete [] hg2;
		return -CUI_EREADVEC;
	}

	hg2_header_t *hg2_header = (hg2_header_t *)hg2;
	hg2_info_t *hg2_info = (hg2_info_t *)(hg2_header + 1);
	hg2_info2_t *hg2_info2 = (hg2_info2_t *)(hg2_info + 1);
	BYTE *compr;
	switch (hg2_header->mode) {
	case 0x25:
		compr = (BYTE *)hg2_info2 + hg2_info2->sizeof_info2;
		break;
	case 0x10:

		break;
	case 0x20:

		break;
	}

	BYTE *uncompr = new BYTE[hg2_info->rgb_uncomprlen + 16];
	if (!uncompr) {
		delete [] hg2;
		return -CUI_EMEM;
	}
	DWORD act_uncomprlen = hg2_info->rgb_uncomprlen + 16;
	if (uncompress(uncompr, &act_uncomprlen, compr, hg2_info->rgb_comprlen) != Z_OK) {
		delete [] uncompr;
		delete [] hg2;
		return -CUI_EUNCOMPR;
	}
	if (act_uncomprlen != hg2_info->rgb_uncomprlen) {
		delete [] uncompr;
		delete [] hg2;
		return -CUI_EUNCOMPR;
	}

	BYTE *uncompr2 = new BYTE[hg2_info->uncomprlen2 + 16];
	if (!uncompr2) {
		delete [] uncompr;
		delete [] hg2;
		return -CUI_EMEM;
	}
	act_uncomprlen = hg2_info->uncomprlen2 + 16;
	compr += hg2_info->rgb_comprlen;
	if (uncompress(uncompr2, &act_uncomprlen, compr, hg2_info->alpha_comprlen) != Z_OK) {
		delete [] uncompr2;
		delete [] uncompr;
		delete [] hg2;
		return -CUI_EUNCOMPR;
	}
	if (act_uncomprlen != hg2_info->uncomprlen2) {
		delete [] uncompr2;
		delete [] uncompr;
		delete [] hg2;
		return -CUI_EUNCOMPR;
	}

	DWORD table[4][256];
	for (DWORD i = 0; i < 256; i++) {
		int dat = (i & 3) | (((i & 0xc) | (((i & 0x30) | ((BYTE)(p & 0xc0) << 6)) << 6)) << 6);
		table[0][i] = dat << 6;
		table[1][i] = dat << 4;
		table[2][i] = dat << 2;
		table[3][i] = dat;
	}

	DWORD bpp = hg2_info->bpp / 8;
	DWORD line_len = hg2_info->width * bpp;
	DWORD dib_size = hg2_info->height * line_len;
	struct bits bits;
	bits_init(&bits, uncompr2, hg2_info->uncomprlen2);
	DWORD flag = bit_get_high(&bits);
	if (!flag) {
		delete [] uncompr2;
		delete [] uncompr;
		delete [] hg2;
		return -CUI_EMATCH;
	}

	DWORD len = get_lenth(&bits);
	BYTE *buffer2 = new BYTE[len](0);
	if (!buffer2) {
		delete [] uncompr2;
		delete [] uncompr;
		delete [] hg2;
		return -CUI_EUNCOMPR;
	}

	for (DWORD i = 0; i < len; i++)
		buffer2[i] = uncompr[i];

	if (len & 3) {
		delete [] buffer2;
		delete [] uncompr2;
		delete [] uncompr;
		delete [] hg2;
		return -CUI_EMATCH;
	}

	DWORD *buffer1 = new DWORD[len / 4];
	if (!buffer1) {
		delete [] buffer2;
		delete [] uncompr2;
		delete [] uncompr;
		delete [] hg2;
		return -CUI_EMEM;
	}

	BYTE *src1 = buffer2;
	BYTE *src2 = buffer2 + len / 2;
	for (i = 0; i < len / 4; i++) {
		buffer1[i] = table[0][*src1] | table[1][src1[len / 4]] | table[2][*src2] | table[3][src2[len / 4]];
		src1++;
		src2++;
	}
	delete [] buffer2;

	BYTE *dib = new BYTE[dib_size];
	if (!dib) {
		delete [] buffer1;
		delete [] uncompr2;
		delete [] uncompr;
		delete [] hg2;
		return -CUI_EMEM;
	}

	BYTE cross_table[256];
	for (i = 0; i < 128; i++) {
		cross_table[i * 2 + 0] = i;
		cross_table[i * 2 + 1] = 255 - i;
	}

	// 输出一行第一个象素
	BYTE *dst = dib + (hg2_info->height - 1) * line_len;
	BYTE *pre = dst;
	BYTE *src = buffer1;
	for (i = 0; i < bpp; i++)
		*dst++ = cross_table[*src++];
	// 输出余下行
	for (DWORD y = 1; y < hg2_info->height; y++) {
		cur = pre - line_len;
		for (DWORD x = 0; x < line_len; x++)
			cur[x] = pre[x] + cross_table[*src++];
		pre -= line_len;
	}
	delete [] buffer1;


	if (hg2_info->key) {
		for (y = 0; y < hg2_info->height; y++) {

		}
	}


	printf("ok\n");exit(0);

	delete [] hg2;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation CatSystem2_hg2_operation = {
	CatSystem2_hg2_match,					/* match */
	NULL,		/* extract_directory */
	NULL,		/* parse_resource_info */
	CatSystem2_hg2_extract_resource,		/* extract_resource */
	CatSystem2_int_save_resource,			/* save_resource */
	CatSystem2_int_release_resource,		/* release_resource */
	CatSystem2_int_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK CatSystem2_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".int"), NULL, 
		NULL, &CatSystem2_int_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".hg2"), NULL, 
		NULL, &CatSystem2_hg2_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	return 0;
}
