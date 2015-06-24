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


/* 数据段内记录了一些重要的参数，包括解密串、封包偏移等。。。（用AVtype_info做关键字查找）
004230A9                                      38 36 30 31              8601
004230B9  36 00 00 00 00 00 78 80 02 79 E0 01 64 00 00 73  6.....x�.y?d..s
004230C9  3F 3B 6B 84 DF 6F 8C 5F 00 00 00 F8 07 42 00 00  ?;k勥o宊...?B..
004230D9  00 00 00 2E 3F 41 56 74 79 70 65 5F 69 6E 66 6F  ....?AVtype_info
004230E9  40 40 00 B8 81 41 00 75 98 00 00 73 98 00 00 69  @@.竵A.u?.s?.i
004230F9  67 41 00 30 67 41 00 30 67 41 00 20 05 93 19     gA.0gA.0gA. .?.
*/

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information HSP_cui_information = {
	_T("onion software"),		/* copyright */
	_T("Hot Soup Processor"),	/* system */
	_T(".dpm .mv .exe"),		/* package */
	_T("1.0.0"),				/* revision */
	_T("绫波微步"),				/* author */
	_T("2007-9-29 22:03"),		/* date */
	NULL,						/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];		/* "DPMX" */
	u32 data_offset;
	u32 index_entries;
	u32 name_length;	// name space確保サイズ(ver2.6)
} dpm_header_t;

typedef struct {
	s8 name[16];
	u32 name_offset;	// -1 名前格納ポインタ(先頭が'\0'の時のみ有効)
	u32 dec_key;		// 暗号化flag(先頭が'\0'の時のみ有効)
	u32 offset;			/* 相对地址 */
	u32 length;
} dpm_entry_t;
#pragma pack ()

struct HSP_context {
	u32 dec_key;			// (C72)eXceed3rd -JADE PENETRATE-: { 0x84, 0xDF, 0x6F, 0x8C }
	DWORD package_offset;
	DWORD package_size;
	u32 res_dec_key;
};

static char fpas[]={ 'H'-48,'S'-48,'P'-48,'H'-48,
					 'E'-48,'D'-48,'~'-48,'~'-48 };

//static char optmes[] = "HSPHED~~\0_1_________2_________3______";

static BYTE hsp_wd; 

static int dpm_checksum(BYTE *buf, unsigned int buf_len, int chksum, int deckey)
{
	int sum, sumseed, sumsize;
	
	sum = 0;
	sumsize = 0;
	sumseed = ((deckey >> 24) & 0xff) / 7;
	if (chksum != -1) {
		for (unsigned int i = 0; i < buf_len; i++)
			sum += buf[i] + sumseed;
	}
	
	return sum & 0xffff;	
}
	
static void decode(struct HSP_context *context, BYTE *buf, unsigned int buf_len)
{
	BYTE *dec_key = (BYTE *)&context->dec_key;
	BYTE *key = (BYTE *)&context->res_dec_key;
	unsigned int i;
	BYTE al, cl;

	if (!context->dec_key)
		return;

	cl = (BYTE)((((dec_key[0] * dec_key[2] / 3) ^ context->package_size) & 0xff) + ((key[0] + 0x55) ^ key[2]));
	al = (BYTE)((((dec_key[1] * dec_key[3] / 5) ^ context->package_size) ^ 0xffffffaa) + ((key[1] + 0xaa) ^ key[3]));

	for (i = 0; i < buf_len; i++) {
		hsp_wd += ((buf[i] - al) ^ cl);
		buf[i] = hsp_wd;
	}
}

/********************* mv *********************/

/* 封包匹配回调函数 */
static int HSP_mv_match(struct package *pkg)
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

	if (memcmp(magic, "cIFF", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int HSP_mv_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	SIZE_T fsize;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	if (pkg->pio->length_of(pkg, &fsize))
		return -CUI_ELEN;

	pkg_res->raw_data = pkg->pio->readvec_only(pkg, fsize - 4, 4, IO_SEEK_SET);
	if (!pkg_res->raw_data)
		return -CUI_EREADVECONLY;
	pkg_res->raw_data_length = fsize - 4;

	return 0;
}

/* 资源保存函数 */
static int HSP_mv_save_resource(struct resource *res, 
								 struct package_resource *pkg_res)
{
	s8 magic[4] = { 'R', 'I', 'F', 'F' };

	if (!res || !pkg_res)
		return -CUI_EPARA;
	
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (pkg_res->raw_data && pkg_res->raw_data_length) {
		if (res->rio->write(res, magic, 4)) {
			res->rio->close(res);
			return -CUI_EWRITE;
		}
		if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
			res->rio->close(res);
			return -CUI_EWRITE;
		}
	}

	res->rio->close(res);
	
	return 0;
}

/* 封包资源释放函数 */
static void HSP_mv_release_resource(struct package *pkg, 
									 struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->raw_data)
		pkg_res->raw_data = NULL;
}

/* 封包卸载函数 */
static void HSP_mv_release(struct package *pkg, 
						   struct package_directory *pkg_dir)
{
	if (!pkg)
		return;

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation HSP_mv_operation = {
	HSP_mv_match,				/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	HSP_mv_extract_resource,	/* extract_resource */
	HSP_mv_save_resource,		/* save_resource */
	HSP_mv_release_resource,	/* release_resource */
	HSP_mv_release				/* release */
};

/********************* exe *********************/

/* 封包匹配回调函数 */
static int HSP_exe_match(struct package *pkg)
{
	s8 magic[4];
	struct HSP_context *context;
	SIZE_T fsize;
	char dpmx_offset_str[10];
	const char *offset_string;
	u32 offset, deckey;

	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->length_of(pkg, &fsize)) {
		pkg->pio->close(pkg);
		return -CUI_ELEN;
	}

	offset_string = get_options("offset");
	if (!offset_string) {
		BYTE *dat = (BYTE *)pkg->pio->read_only(pkg, fsize);
		if (!dat) {
			pkg->pio->close(pkg);
			return -CUI_EREADONLY;
		}

		for (DWORD i = 0; i < fsize; i++) {
			for (DWORD k = 0; k < 8; k++) {
				if (dat[i + k] != fpas[k])
					break;
			}
			if (k == 8)
				break;
		}
		if (i == fsize) {
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}

		offset = i + 8;
		if (pkg->pio->readvec(pkg, dpmx_offset_str, sizeof(dpmx_offset_str), offset + 9, IO_SEEK_SET)) {
			pkg->pio->close(pkg);
			return -CUI_EREADVECONLY;
		}

		//hsp_wd=( *(short *)(dpmx_offset_str+26) );
		//hsp_sum=*(unsigned short *)(dpmx_offset_str+29);
		if (pkg->pio->readvec(pkg, &deckey, 4, offset + 32, IO_SEEK_SET)) {
			pkg->pio->close(pkg);
			return -CUI_EREADVECONLY;
		}
		offset = atoi(dpmx_offset_str) + 0x10000;	
	} else {
		deckey = 0;
		offset = strtoul(offset_string, NULL, 0);
	}

	context = (struct HSP_context *)malloc(sizeof(*context));
	if (!context) {
		pkg->pio->close(pkg);
		return -CUI_EMEM;
	}
	context->dec_key = deckey;
	context->package_offset = offset;
	context->package_size = fsize - context->package_offset;

	if (pkg->pio->seek(pkg, context->package_offset, IO_SEEK_SET)) {
		free(context);
		pkg->pio->close(pkg);
		return -CUI_ESEEK;
	}

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		free(context);
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "DPMX", 4)) {
		free(context);
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}
	package_set_private(pkg, context);

	return 0;	
}

/* 封包索引目录提取函数 */
static int HSP_exe_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{
	dpm_entry_t *index_buffer;
	unsigned int index_buffer_length;
	u32 data_offset, extra_length;
	struct HSP_context *context;

	if (!pkg || !pkg_dir)
		return -CUI_EPARA;

	context = (struct HSP_context *)package_get_private(pkg);

	if (pkg->pio->read(pkg, &data_offset, 4))
		return -CUI_EREAD;

	if (pkg->pio->read(pkg, &pkg_dir->index_entries, 4))
		return -CUI_EREAD;

	if (pkg->pio->read(pkg, &extra_length, 4))
		return -CUI_EREAD;

	index_buffer_length = pkg_dir->index_entries * sizeof(dpm_entry_t);
	if (data_offset != index_buffer_length + sizeof(dpm_header_t))
		return -CUI_EMATCH;

	index_buffer = (dpm_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(dpm_entry_t);
	context->package_size -= data_offset;
	context->package_offset += data_offset;

	return 0;
}

/* 封包索引项解析函数 */
static int HSP_exe_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	dpm_entry_t *dpm_entry;
	struct HSP_context *context;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	context = (struct HSP_context *)package_get_private(pkg);
	dpm_entry = (dpm_entry_t *)pkg_res->actual_index_entry;
	context->res_dec_key = dpm_entry->dec_key;
	strcpy(pkg_res->name, dpm_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = dpm_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = dpm_entry->offset + context->package_offset;

	return 0;
}

/* 封包资源提取函数 */
static int HSP_exe_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	dpm_entry_t *dpm_entry;
	struct HSP_context *context;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	pkg_res->raw_data = malloc(pkg_res->raw_data_length);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EREADVEC;
	}

	dpm_entry = (dpm_entry_t *)pkg_res->actual_index_entry;
	context = (struct HSP_context *)package_get_private(pkg);
	if (context->dec_key && dpm_entry->dec_key) {
		struct HSP_context *context;
		context = (struct HSP_context *)package_get_private(pkg);
		decode(context, (BYTE *)pkg_res->raw_data, pkg_res->raw_data_length);
	}

	return 0;
}

/* 资源保存函数 */
static int HSP_exe_save_resource(struct resource *res, 
								  struct package_resource *pkg_res)
{
	if (!res || !pkg_res)
		return -CUI_EPARA;
	
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (pkg_res->raw_data && pkg_res->raw_data_length) {
		if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
			res->rio->close(res);
			return -CUI_EWRITE;
		}
	}

	res->rio->close(res);
	
	return 0;
}

/* 封包资源释放函数 */
static void HSP_exe_release_resource(struct package *pkg, 
										struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

/* 封包卸载函数 */
static void HSP_exe_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	struct HSP_context *context;

	if (!pkg || !pkg_dir)
		return;

	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	context = (struct HSP_context *)package_get_private(pkg);
	if (context) {
		free(context);
		package_set_private(pkg, NULL);
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation HSP_exe_operation = {
	HSP_exe_match,					/* match */
	HSP_exe_extract_directory,		/* extract_directory */
	HSP_exe_parse_resource_info,	/* parse_resource_info */
	HSP_exe_extract_resource,		/* extract_resource */
	HSP_exe_save_resource,			/* save_resource */
	HSP_exe_release_resource,		/* release_resource */
	HSP_exe_release					/* release */
};

/********************* dpm *********************/

/* 封包匹配回调函数 */
static int HSP_dpm_match(struct package *pkg)
{
	s8 magic[4];
	struct HSP_context *context;
	SIZE_T fsize;

	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->length_of(pkg, &fsize))
		return -CUI_ELEN;

	context = (struct HSP_context *)malloc(sizeof(*context));
	if (!context) {
		pkg->pio->close(pkg);
		return -CUI_EMEM;
	}

	context->dec_key = 0;
	context->package_offset = 0;
	context->package_size = fsize - context->package_offset;

	if (pkg->pio->seek(pkg, context->package_offset, IO_SEEK_SET)) {
		free(context);
		pkg->pio->close(pkg);
		return -CUI_ESEEK;
	}

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		free(context);
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "DPMX", 4)) {
		free(context);
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}
	package_set_private(pkg, context);

	return 0;	
}

/* 封包处理回调函数集合 */
static cui_ext_operation HSP_dpm_operation = {
	HSP_dpm_match,					/* match */
	HSP_exe_extract_directory,		/* extract_directory */
	HSP_exe_parse_resource_info,	/* parse_resource_info */
	HSP_exe_extract_resource,		/* extract_resource */
	HSP_exe_save_resource,			/* save_resource */
	HSP_exe_release_resource,		/* release_resource */
	HSP_exe_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK HSP_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".mv"), _T(".wav"), 
		NULL, &HSP_mv_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".exe"), NULL, 
		NULL, &HSP_exe_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dpm"), NULL, 
		NULL, &HSP_dpm_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
