#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>

/* HSP2: 寻找偏移字符串的方法
oniwndp\0data.dpm\0
这个字串的后面就是偏移。

HSP3: 数据段内记录了一些重要的参数，包括解密串、封包偏移等。。。（用AVtype_info做关键字查找）
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
	_T(".dpm .mv .exe .dat"),	/* package */
	_T("2.0.2"),				/* revision */
	_T("绫波微步"),				/* author */
	_T("2009-1-10 16:57"),		/* date */
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
	u32 dpm_fenc;		// 暗号化flag(先頭が'\0'の時のみ有効)
	u32 offset;			/* 相对地址 */
	u32 length;
} dpm_entry_t;
#pragma pack ()

struct HSP_context {
	int hsp_dec;			// (C72)eXceed3rd -JADE PENETRATE-: { 0x84, 0xDF, 0x6F, 0x8C }
	unsigned short hsp_sum;
	long dpm_ofs;
	int do_decrypt;
	/* 资源私有数据 */
	int dpm_fenc;
	int dpm_enc1, dpm_enc2;
	int seed1, seed2;
	unsigned char dpm_lastchr;
};

#define HSP2_HSPRT		4

/* HSP2.x没有定位标识，它的偏移地址是编译时决定的，无法像HSP3那样可以自举出来 */
static DWORD hsp2_hsprt[HSP2_HSPRT] = { 0x1ba00/* 2.6e */, 0x1bc00/* 2.6 */, 0x1be00/* 2.55, 2.55e, 2.61 */, 0x1c400/* 2.61 */, };

static char fpas[]={ 'H'-48,'S'-48,'P'-48,'H'-48,
					 'E'-48,'D'-48,'~'-48,'~'-48 };

static char optmes[] = "HSPHED~~\0_1_________2_________3______";

static BOOL dpm_checksum(BYTE *buf, unsigned int buf_len, int chksum, int deckey)
{
	int sum, sumseed;
	
	sum = 0;
	sumseed = ((deckey >> 24) & 0xff) / 7;
	for (unsigned int i = 0; i < buf_len; i++)
		sum += buf[i] + sumseed;
	
	return (sum & 0xffff) == chksum;	
}

/********************* mv *********************/

/* 封包匹配回调函数 */
static int HSP_mv_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "cIFF", 4) && memcmp(magic, "RIFF", 4) && memcmp(magic, "C~FF", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int HSP_mv_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	u32 fsize;

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
	if (pkg_res->raw_data)
		pkg_res->raw_data = NULL;
}

/* 封包卸载函数 */
static void HSP_mv_release(struct package *pkg, 
						   struct package_directory *pkg_dir)
{
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

/********************* mv ogg *********************/

/* 封包匹配回调函数 */
static int HSP_mv_ogg_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "C~gS", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int HSP_mv_ogg_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	u32 fsize;

	if (pkg->pio->length_of(pkg, &fsize))
		return -CUI_ELEN;

	pkg_res->raw_data = pkg->pio->readvec_only(pkg, fsize - 4, 4, IO_SEEK_SET);
	if (!pkg_res->raw_data)
		return -CUI_EREADVECONLY;
	pkg_res->raw_data_length = fsize - 4;

	return 0;
}

/* 资源保存函数 */
static int HSP_mv_ogg_save_resource(struct resource *res, 
									struct package_resource *pkg_res)
{
	s8 magic[4] = { 'O', 'g', 'g', 'S' };
	
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

/* 封包处理回调函数集合 */
static cui_ext_operation HSP_mv_ogg_operation = {
	HSP_mv_ogg_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	HSP_mv_ogg_extract_resource,/* extract_resource */
	HSP_mv_ogg_save_resource,	/* save_resource */
	HSP_mv_release_resource,	/* release_resource */
	HSP_mv_release				/* release */
};

/********************* exe *********************/

/* 封包匹配回调函数 */
static int HSP_exe_match(struct package *pkg)
{
	s8 magic[4];
	struct HSP_context *context;
	u32 sumsize;
	u32 offset = 0;
	int hsp_dec;
	unsigned short hsp_sum;
	DWORD i;
	int orgexe = 0;
	int a;
	BYTE *dat;
	int do_decrypt;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->length_of(pkg, &sumsize)) {
		pkg->pio->close(pkg);
		return -CUI_ELEN;
	}

	context = (struct HSP_context *)malloc(sizeof(*context));
	if (!context) {
		pkg->pio->close(pkg);
		return -CUI_EMEM;
	}

	// 先尝试HSP2.x
	for (i = 0; i < HSP2_HSPRT; i++) {
		if (!pkg->pio->readvec(pkg, magic, sizeof(magic), hsp2_hsprt[i], IO_SEEK_SET)) {
			if (!strncmp(magic, "DPMX", 4)) {
				hsp_dec = -1;
				hsp_sum = -1;
				offset = hsp2_hsprt[i];
				do_decrypt = 0;
				goto hsp2;
			}
		}
	}

	// 尝试HSP3.x
	dat = (BYTE *)pkg->pio->readvec_only(pkg, sumsize, 0, IO_SEEK_SET);
	if (!dat) {
		free(context);
		pkg->pio->close(pkg);
		return -CUI_EREADVECONLY;
	}

	for (i = 0; i < sumsize; ++i) {
		for (DWORD k = 0; k < 8; ++k) {
			if (dat[i + k] != fpas[k])
				break;
		}
		if (k == 8)
			break;
	}
	if (i == sumsize) {
		free(context);
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->pio->readvec(pkg, optmes, sizeof(optmes), i + 8, IO_SEEK_SET)) {
		free(context);
		pkg->pio->close(pkg);
		return -CUI_EREADVECONLY;
	}

	for (a = 0; a < 8; ++a) {
		if (optmes[a] - 48 == fpas[a])
			++orgexe;
	}

	if (!orgexe) {
		offset = atoi(optmes + 9) + 0x10000;			// customized EXE mode else original EXE mode
		//	hsp_wd = *(short *)(optmes + 26);
		hsp_sum = *(u16 *)(optmes + 29);
		hsp_dec = *(u32 *)(optmes + 32);
	} else {
		hsp_sum = -1;
		hsp_dec = -1;
		offset = 0;
	}

	do_decrypt = 1;

hsp2:
	context->hsp_dec = hsp_dec;
	context->hsp_sum = hsp_sum;
	context->dpm_ofs = offset;
	context->do_decrypt = do_decrypt;
	sumsize -= offset;

	if (pkg->pio->seek(pkg, context->dpm_ofs, IO_SEEK_SET)) {
		free(context);
		pkg->pio->close(pkg);
		return -CUI_ESEEK;
	}

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		free(context);
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "DPMX", 4)) {
		free(context);
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (hsp_sum != (unsigned short)-1) {
		dpm_header_t *dpm_header;
		BYTE *chk_sum;
		int hedsize;

		chk_sum = (BYTE *)pkg->pio->readvec_only(pkg, sumsize, context->dpm_ofs, IO_SEEK_SET);
		if (!chk_sum) {
			free(context);
			pkg->pio->close(pkg);
			return -CUI_EREADVECONLY;				
		}

		dpm_header = (dpm_header_t *)chk_sum;
		hedsize = sizeof(dpm_header_t) + dpm_header->name_length + dpm_header->index_entries * sizeof(dpm_entry_t);

		if (!dpm_checksum(chk_sum, sumsize, hsp_sum, hsp_dec)) {
			free(context);
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}

		sumsize -= hedsize;
	}

	if (hsp_dec == -1) {
		context->seed1 = 0xaa;
		context->seed2 = 0x55;
	} else {
		context->seed1 = (sumsize ^ (((hsp_dec>>16)&0xff) * (hsp_dec&0xff) / 3)) & 0xff;
		context->seed2 = ((((hsp_dec>>8)&0xff) * ((hsp_dec>>24)&0xff) / 5) ^ sumsize ^ 0xaa) & 0xff;
	}
	package_set_private(pkg, context);

	return 0;	
}

/* 封包索引目录提取函数 */
static int HSP_exe_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{
	dpm_header_t dpm_header;
	dpm_entry_t *index_buffer;
	unsigned int index_buffer_length;
	struct HSP_context *context;

	context = (struct HSP_context *)package_get_private(pkg);

	if (pkg->pio->readvec(pkg, &dpm_header, sizeof(dpm_header), context->dpm_ofs, IO_SEEK_SET))
		return -CUI_EREADVEC;

	index_buffer_length = dpm_header.index_entries * sizeof(dpm_entry_t);
	if (dpm_header.data_offset != sizeof(dpm_header_t) + dpm_header.name_length + index_buffer_length)
		return -CUI_EMATCH;

	index_buffer = (dpm_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = dpm_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(dpm_entry_t);
	context->dpm_ofs += dpm_header.data_offset;

	return 0;
}

/* 封包索引项解析函数 */
static int HSP_exe_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	dpm_entry_t *dpm_entry;
	struct HSP_context *context;

	context = (struct HSP_context *)package_get_private(pkg);
	dpm_entry = (dpm_entry_t *)pkg_res->actual_index_entry;	
	strcpy(pkg_res->name, dpm_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = dpm_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = dpm_entry->offset + context->dpm_ofs;

	if (dpm_entry->dpm_fenc) {
		context->dpm_enc1 = ((dpm_entry->dpm_fenc & 0xff) + 0x55) ^ ((dpm_entry->dpm_fenc >> 16) & 0xff);
		context->dpm_enc2 = (((dpm_entry->dpm_fenc >> 8) & 0xff) + 0xaa) ^ ((dpm_entry->dpm_fenc >> 24) & 0xff);
	} else {
		context->dpm_enc1 = 0;
		context->dpm_enc2 = 0;
	}
	context->dpm_fenc = dpm_entry->dpm_fenc;
	context->dpm_lastchr = 0;

	return 0;
}

/* 封包资源提取函数 */
static int HSP_exe_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	struct HSP_context *context;

	pkg_res->raw_data = malloc(pkg_res->raw_data_length);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EREADVEC;
	}

	context = (struct HSP_context *)package_get_private(pkg);
	//if (context->do_decrypt) {	// 原来认为HSP完全无加密,后碰到1个2.61游戏有加密	
		BYTE *mem = (BYTE *)pkg_res->raw_data;
		if (context->dpm_fenc) {
			for (DWORD i = 0; i < pkg_res->raw_data_length; ++i) {
				context->dpm_lastchr += (context->dpm_enc1 + context->seed1) ^ (*mem - (context->seed2 + context->dpm_enc2));
				*mem++ = context->dpm_lastchr;
			}
		}
	//}

	return 0;
}

/* 资源保存函数 */
static int HSP_exe_save_resource(struct resource *res, 
								  struct package_resource *pkg_res)
{
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
	u32 fsize;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->length_of(pkg, &fsize))
		return -CUI_ELEN;

	context = (struct HSP_context *)malloc(sizeof(*context));
	if (!context) {
		pkg->pio->close(pkg);
		return -CUI_EMEM;
	}
#if 1
	context->hsp_dec = -1;
	context->hsp_sum = -1;
	context->dpm_ofs = 0;
	context->seed1 = 0xaa;
	context->seed2 = 0x55;
#else

	context->dpm_ofs = 0;
	context->hsp_sum = -1;
context->hsp_dec = 0x284b7729;
	if (context->hsp_dec == -1) {
		context->seed1 = 0xaa;
		context->seed2 = 0x55;
	} else {
		u32 sumsize = 0;
		u32 hsp_dec = context->hsp_dec;
		context->seed1 = (sumsize ^ (((hsp_dec>>16)&0xff) * (hsp_dec&0xff) / 3)) & 0xff;
		context->seed2 = ((((hsp_dec>>8)&0xff) * ((hsp_dec>>24)&0xff) / 5) ^ sumsize ^ 0xaa) & 0xff;
	}
printf("%x %x\n",context->seed1,context->seed2);
	exit(0);
#endif

	if (pkg->pio->seek(pkg, context->dpm_ofs, IO_SEEK_SET)) {
		free(context);
		pkg->pio->close(pkg);
		return -CUI_ESEEK;
	}

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		free(context);
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "DPMX", 4)) {
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

/********************* dat *********************/

/* 封包匹配回调函数 */
static int HSP_dat_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包资源提取函数 */
static int HSP_dat_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	DWORD raw_len = pkg_res->raw_data_length;
	DWORD *raw = (DWORD *)malloc(raw_len);
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, (BYTE *)raw, raw_len, 0, IO_SEEK_SET)) {
		free(raw);
		return -CUI_EREADVEC;
	}

	raw_len -= 4;
	DWORD dec_len = raw_len > 1000 ? 1000 : raw_len;

	u32 xor = *(u32 *)((BYTE *)raw + raw_len);
	for (DWORD i = 0; i < dec_len / 4; ++i)
		raw[i] ^= xor;

	for (DWORD j = 0; j < (dec_len & 3); ++j)
		((BYTE *)&raw[i])[j] ^= ((BYTE *)&xor)[j];

	if (!memcmp(raw, "\xff\xd8\xff\xe0\x00\x10JFIF", 10)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".jpg");
	} else if (!memcmp(raw, "BM", 2)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp");
	} else if (!memcmp(raw, "\x89PNG", 2)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".png");
	}

	pkg_res->raw_data = raw;
	pkg_res->raw_data_length = raw_len;

	return 0;
}

static cui_ext_operation HSP_dat_operation = {
	HSP_dat_match,					/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	HSP_dat_extract_resource,		/* extract_resource */
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

	if (callback->add_extension(callback->cui, _T(".mv"), _T(".ogg"), 
		NULL, &HSP_mv_ogg_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".exe"), NULL, 
		NULL, &HSP_exe_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dpm"), NULL, 
		NULL, &HSP_dpm_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &HSP_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	return 0;
}
