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
struct acui_information GXTH_cui_information = {
	_T("Experience Inc"),		/* copyright */
	_T("Generation Xth -Code Breaker-"),	/* system */
	_T(".uni .dat .EVT .efc"),	/* package */
	_T("1.0.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-1-11 19:05"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[2];	// "UF"
	u16 width;
	u16 height;
	u8 bpp;
	u8 unknown;		// bpp=3: 1, bpp=4, 2
	u32 header_size;
	u32 length;		// 通常比实际的rgb数据多2字节，但也有和实际值不匹配的情况
} uni_header_t;

typedef struct {
	s8 magic[4];	// "EFC"
	u16 width;
	u16 height;
	u16 bpp;
	u16 unknown;		// bpp=4, 2
	u32 index_entries;
} efc_header_t;

typedef struct {
	s32 uncomprlen;
	s32 uncompr_count;
} compr_header_t;
#pragma pack ()

typedef struct {
	char name[16];
	DWORD offset;
	DWORD length;
} my_entry_t;

static void *my_malloc(DWORD len)
{
	return new BYTE[len];
}

static void __GXTH_uncompress(u16 *uncompr, DWORD uncomprlen, u16 *compr)
{
	DWORD curbyte = 0;
	DWORD act_uncomprlen = 0;
	DWORD act_output = 0;
	DWORD shift = 0;
	u16 flag;

	while (act_output < uncomprlen) {
		if (!shift)
			flag = compr[curbyte++];

		if (flag & (1 << shift)) {
			u16 val = compr[curbyte++];
			DWORD cp = val & 0x7f;
			u16 *src = &uncompr[act_uncomprlen - (val >> 7)];
			u16 *dst = &uncompr[act_uncomprlen];
			for (DWORD i = 0; i < cp; ++i)
				*dst++ = *src++;
			act_uncomprlen += cp;
		} else
			uncompr[act_uncomprlen++] = ~compr[curbyte++];
		shift = ++act_output & 0x0f;
	}
}

static int GXTH_uncompress(BYTE **ret_uncompr, DWORD *ret_uncomprlen,
						   compr_header_t *compr_header)
{
	BYTE *uncompr = new BYTE[compr_header->uncomprlen + 1];
	if (!uncompr)
		return -CUI_EMEM;

	u16 *compr = (u16 *)(compr_header + 1);	
	if (!compr_header->uncompr_count)
		memcpy(uncompr, compr, compr_header->uncomprlen);
	else
		__GXTH_uncompress((u16 *)uncompr, compr_header->uncompr_count, compr);

	*ret_uncompr = (BYTE *)uncompr;
	*ret_uncomprlen = (DWORD)compr_header->uncomprlen;

	return 0;
}

/********************* uni *********************/

/* 封包匹配回调函数 */
static int GXTH_uni_match(struct package *pkg)
{
	s8 magic[2];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "UF", 2)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int GXTH_uni_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	uni_header_t uni;

	if (pkg->pio->readvec(pkg, &uni, sizeof(uni), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD raw_len = ((uni.width * uni.bpp + 3) & ~3) * uni.height;
	BYTE *raw = new BYTE[raw_len];
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, raw_len, sizeof(uni_header_t), IO_SEEK_SET)) {
		delete [] raw;
		return -CUI_EREADVEC;
	}

	u32 fsize;
	pkg->pio->length_of(pkg, &fsize);

	if (MyBuildBMPFile(raw, raw_len, NULL, 0, uni.width, uni.height, uni.bpp * 8,
		(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, my_malloc)) {
		delete [] raw;
		return -CUI_EMEM;
	}
	delete [] raw;

	return 0;
}

/* 资源保存函数 */
static int GXTH_uni_save_resource(struct resource *res, 
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
static void GXTH_uni_release_resource(struct package *pkg, 
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
static void GXTH_uni_release(struct package *pkg, 
							 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation GXTH_uni_operation = {
	GXTH_uni_match,				/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	GXTH_uni_extract_resource,	/* extract_resource */
	GXTH_uni_save_resource,		/* save_resource */
	GXTH_uni_release_resource,	/* release_resource */
	GXTH_uni_release			/* release */
};

/********************* dat *********************/

/* 封包匹配回调函数 */
static int GXTH_dat_match(struct package *pkg)
{
	s8 magic[36];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic + 32, "OggS", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int GXTH_dat_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	u32 fsize;
	pkg->pio->length_of(pkg, &fsize);
	fsize -= 32;

	BYTE *ogg = new BYTE[fsize];
	if (!ogg)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, ogg, fsize, 32, IO_SEEK_SET)) {
		delete [] ogg;
		return -CUI_EREADVEC;
	}

	pkg_res->raw_data = ogg;
	pkg_res->raw_data_length = fsize;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation GXTH_dat_operation = {
	GXTH_dat_match,				/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	GXTH_dat_extract_resource,	/* extract_resource */
	GXTH_uni_save_resource,		/* save_resource */
	GXTH_uni_release_resource,	/* release_resource */
	GXTH_uni_release			/* release */
};

/********************* compr *********************/

/* 封包匹配回调函数 */
static int GXTH_compr_match(struct package *pkg)
{
	if (!lstrcmpi(pkg->name, L"tutorial.dat"))
		return -CUI_EMATCH;

	compr_header_t header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &header, sizeof(header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (header.uncomprlen <= 0 || header.uncompr_count < 0) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int GXTH_compr_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	u32 fsize;
	pkg->pio->length_of(pkg, &fsize);

	BYTE *compr = new BYTE[fsize];
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, fsize, 0, IO_SEEK_SET)) {
		delete [] compr;
		return -CUI_EREADVEC;
	}

	int ret = GXTH_uncompress((BYTE **)&pkg_res->actual_data, 
		&pkg_res->actual_data_length, (compr_header_t *)compr);
	if (ret) {
		delete [] compr;
		return ret;		
	}
	delete [] compr;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation GXTH_compr_operation = {
	GXTH_compr_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	GXTH_compr_extract_resource,/* extract_resource */
	GXTH_uni_save_resource,		/* save_resource */
	GXTH_uni_release_resource,	/* release_resource */
	GXTH_uni_release			/* release */
};

/********************* tutorial.dat *********************/

/* 封包匹配回调函数 */
static int GXTH_tutorial_match(struct package *pkg)
{
	if (lstrcmpi(pkg->name, L"tutorial.dat"))
		return -CUI_EMATCH;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	s32 index_entries;

	if (pkg->pio->read(pkg, &index_entries, sizeof(index_entries))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (index_entries <= 0) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int GXTH_tutorial_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{
	u32 index_entries;

	if (pkg->pio->readvec(pkg, &index_entries, sizeof(index_entries), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	my_entry_t *my_index = new my_entry_t[index_entries];
	if (!my_index)
		return -CUI_EMEM;

	u32 *offset_index = new u32[index_entries];
	if (!offset_index) {
		delete [] my_index;
		return -CUI_EMEM;
	}

	if (pkg->pio->read(pkg, offset_index, index_entries * 4)) {
		delete [] offset_index;
		delete [] my_index;
		return -CUI_EREAD;
	}

	my_entry_t *entry = my_index;
	for (DWORD i = 0; i < index_entries; ++i) {
		sprintf(entry->name, "%06d", i);
		entry->offset = offset_index[i];
		++entry;
	}
	delete [] offset_index;

	entry = my_index;
	for (i = 0; i < index_entries - 1; ++i) {
		entry->length = (entry+1)->offset - entry->offset;
		++entry;
	}

	u32 fsize;
	pkg->pio->length_of(pkg, &fsize);
	entry->length = fsize - entry->offset;

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = my_index;
	pkg_dir->directory_length = index_entries * sizeof(my_entry_t);
	pkg_dir->index_entry_length = sizeof(my_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int GXTH_tutorial_parse_resource_info(struct package *pkg,
											 struct package_resource *pkg_res)
{
	my_entry_t *my_entry;

	my_entry = (my_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int GXTH_tutorial_extract_resource(struct package *pkg,
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
static cui_ext_operation GXTH_tutorial_operation = {
	GXTH_tutorial_match,				/* match */
	GXTH_tutorial_extract_directory,	/* extract_directory */
	GXTH_tutorial_parse_resource_info,	/* parse_resource_info */
	GXTH_tutorial_extract_resource,		/* extract_resource */
	GXTH_uni_save_resource,				/* save_resource */
	GXTH_uni_release_resource,			/* release_resource */
	GXTH_uni_release					/* release */
};

/********************* efc *********************/

/* 封包匹配回调函数 */
static int GXTH_efc_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "EFC", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int GXTH_efc_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	efc_header_t *header = new efc_header_t;
	if (!header)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, header, sizeof(efc_header_t), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	my_entry_t *my_index = new my_entry_t[header->index_entries];
	if (!my_index)
		return -CUI_EMEM;

	u32 *offset_index = new u32[header->index_entries];
	if (!offset_index) {
		delete [] my_index;
		return -CUI_EMEM;
	}

	if (pkg->pio->read(pkg, offset_index, header->index_entries * 4)) {
		delete [] offset_index;
		delete [] my_index;
		return -CUI_EREAD;
	}

	my_entry_t *entry = my_index;
	for (DWORD i = 0; i < header->index_entries; ++i) {
		sprintf(entry->name, "%06d", i);
		entry->offset = offset_index[i];
		++entry;
	}
	delete [] offset_index;

	entry = my_index;
	for (i = 0; i < header->index_entries - 1; ++i) {
		entry->length = (entry+1)->offset - entry->offset;
		++entry;
	}

	u32 fsize;
	pkg->pio->length_of(pkg, &fsize);
	entry->length = fsize - entry->offset;

	pkg_dir->index_entries = header->index_entries;
	pkg_dir->directory = my_index;
	pkg_dir->directory_length = header->index_entries * sizeof(my_entry_t);
	pkg_dir->index_entry_length = sizeof(my_entry_t);
	package_set_private(pkg, header);

	return 0;
}

/* 封包索引项解析函数 */
static int GXTH_efc_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	my_entry_t *my_entry;

	my_entry = (my_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int GXTH_efc_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	BYTE *compr = new BYTE[pkg_res->raw_data_length];
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, pkg_res->raw_data_length, 
			pkg_res->offset, IO_SEEK_SET)) {
		delete [] compr;
		return -CUI_EREADVEC;
	}

	int ret = GXTH_uncompress((BYTE **)&pkg_res->actual_data, 
		&pkg_res->actual_data_length, (compr_header_t *)compr);
	if (ret) {
		delete [] compr;
		return ret;		
	}
	delete [] compr;

	efc_header_t *header = (efc_header_t *)package_get_private(pkg);
	BYTE *uncompr = (BYTE *)pkg_res->actual_data;
	if (MyBuildBMPFile(uncompr, pkg_res->actual_data_length, 
			NULL, 0, header->width, header->height, header->bpp * 8,
			(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, my_malloc)) {
		delete [] uncompr;
		pkg_res->actual_data = NULL;
		return -CUI_EMEM;
	}
	delete [] uncompr;

	return 0;
}

static void GXTH_efc_release(struct package *pkg, 
							 struct package_directory *pkg_dir)
{
	void *priv = (void *)package_get_private(pkg);
	if (priv) {
		delete [] priv;
		package_set_private(pkg, NULL);
	}

	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation GXTH_efc_operation = {
	GXTH_efc_match,					/* match */
	GXTH_efc_extract_directory,		/* extract_directory */
	GXTH_efc_parse_resource_info,	/* parse_resource_info */
	GXTH_efc_extract_resource,		/* extract_resource */
	GXTH_uni_save_resource,			/* save_resource */
	GXTH_uni_release_resource,		/* release_resource */
	GXTH_efc_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK GXTH_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".uni"), _T(".bmp"), 
		NULL, &GXTH_uni_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), _T(".ogg"), 
		NULL, &GXTH_dat_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), _T("._dat"), 
		NULL, &GXTH_compr_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".EVT"), _T("._EVT"), 
		NULL, &GXTH_compr_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &GXTH_tutorial_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".efc"), _T(".bmp"), 
		NULL, &GXTH_efc_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
