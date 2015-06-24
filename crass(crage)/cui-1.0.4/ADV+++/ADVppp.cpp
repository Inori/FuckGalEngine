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
struct acui_information ADVppp_cui_information = {
	_T("YOX-Project"),		/* copyright */
	_T("ADV+++"),			/* system */
	_T(".dat"),				/* package */
	_T("1.0.0"),			/* revision */
	_T("痴汉公贼"),			/* author */
	_T("2008-2-3 1:21"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {			// 32 bytes
	s8 magic[4];			// "YOX"
	u32 reserved;			// 0
	u32 index_offset;
	u32 index_entries;
	SYSTEMTIME time_stamp;	// 来自.lst文件的时间戳
} dat_header_t;

typedef struct {		// 16 bytes
	s8 magic[4];		// "YOX"
	u32 flag;			// bit1 - zlib or not
	u32 uncomprlen;
	u32 reserved;		// 0
} dat_res_header_t;

typedef struct {
	u32 offset;
	u32 length;			// 2k对齐
} dat_entry_t;
#pragma pack ()

typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 length;
} my_dat_entry_t;

/********************* dat *********************/

/* 封包匹配回调函数 */
static int ADVppp_dat_match(struct package *pkg)
{
	s8 magic[4];

	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "YOX", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int ADVppp_dat_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	dat_header_t dat_header;
	dat_entry_t *index_buffer;
	my_dat_entry_t *my_index_buffer;
	DWORD index_buffer_length, my_index_buffer_length;
	DWORD i;

	if (!pkg || !pkg_dir)
		return -CUI_EPARA;

	if (pkg->pio->readvec(pkg, &dat_header, sizeof(dat_header_t), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	index_buffer_length = dat_header.index_entries * sizeof(dat_entry_t);
	index_buffer = (dat_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, index_buffer, index_buffer_length, dat_header.index_offset, IO_SEEK_SET)) {
		free(index_buffer);
		return -CUI_EREADVEC;
	}

	my_index_buffer_length = dat_header.index_entries * sizeof(my_dat_entry_t);
	my_index_buffer = (my_dat_entry_t *)malloc(my_index_buffer_length);
	if (!my_index_buffer) {
		free(index_buffer);
		return -CUI_EMEM;
	}

	for (i = 0; i < dat_header.index_entries; i++) {
		sprintf(my_index_buffer[i].name, "%08d", i);
		my_index_buffer[i].offset = index_buffer[i].offset;
		my_index_buffer[i].length = index_buffer[i].length;
	}

	pkg_dir->index_entries = dat_header.index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = my_index_buffer_length;
	pkg_dir->index_entry_length = sizeof(my_dat_entry_t);
//	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int ADVppp_dat_parse_resource_info(struct package *pkg,
									struct package_resource *pkg_res)
{
	my_dat_entry_t *my_dat_entry;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	my_dat_entry = (my_dat_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_dat_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_dat_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = my_dat_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int ADVppp_dat_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	dat_res_header_t *res_header;
	BYTE *compr, *uncompr, *act_data;
	DWORD comprlen, uncomprlen, act_data_len;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)pkg->pio->readvec_only(pkg, pkg_res->raw_data_length, 
		pkg_res->offset, IO_SEEK_SET);
	if (!compr)
		return -CUI_EREADVECONLY;

	res_header = (dat_res_header_t *)compr;
	if (!strncmp((char *)res_header, "YOX", 4) && (res_header->flag & 2)) {
		uncomprlen = res_header->uncomprlen;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr)
			return -CUI_EMEM;

		uncompress(uncompr, &uncomprlen, compr + sizeof(dat_res_header_t), comprlen - sizeof(dat_res_header_t));
		act_data = uncompr;
		act_data_len = uncomprlen;
	} else if (!_tcsncmp(pkg->name, _T("card_"), 5) && (pkg_res->index_number == 1 || pkg_res->index_number == 2)) {	// for Moon Strike
		BYTE dec[4] = { 0xba, 0xdc, 0xcd, 0xab };

		uncomprlen = comprlen;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr)
			return -CUI_EMEM;
		for (DWORD i = 0; i < uncomprlen; i++)
			uncompr[i] = compr[i] ^ dec[i & 3];
		act_data = uncompr;
		act_data_len = uncomprlen;
	} else {
		uncomprlen = 0;
		uncompr = NULL;
		act_data = compr;
		act_data_len = comprlen;
	}
	
	if (!strncmp((char *)act_data, "RIFF", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".wav");
	} else if (!strncmp((char *)act_data, "BM", 2)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp");
	}

	pkg_res->raw_data = compr;
	pkg_res->raw_data_length = comprlen;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

/* 资源保存函数 */
static int ADVppp_dat_save_resource(struct resource *res, 
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

/* 封包资源释放函数 */
static void ADVppp_dat_release_resource(struct package *pkg, 
										struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res->raw_data)
		pkg_res->raw_data = NULL;
}

/* 封包卸载函数 */
static void ADVppp_dat_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (!pkg || !pkg_dir)
		return;

	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation ADVppp_dat_operation = {
	ADVppp_dat_match,					/* match */
	ADVppp_dat_extract_directory,		/* extract_directory */
	ADVppp_dat_parse_resource_info,		/* parse_resource_info */
	ADVppp_dat_extract_resource,		/* extract_resource */
	ADVppp_dat_save_resource,			/* save_resource */
	ADVppp_dat_release_resource,		/* release_resource */
	ADVppp_dat_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK ADVppp_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &ADVppp_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
