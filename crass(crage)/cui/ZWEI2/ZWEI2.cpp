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
#include <shlwapi.h>

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information ZWEI2_cui_information = {
	_T("Kyoya Yuro"),		/* copyright */
	_T("ラムダ"),			/* system */
	_T(".dat .dmf"),		/* package */
	_T("1.0.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-3-24 8:52"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	u32 type;
	u32 crc1;
	u32 crc2;
} itm_tailer_t;
#pragma pack ()

typedef struct {
	s8 name[256];
	u32 name_length;
	u32 offset;
	u32 length;
} dat_entry_t;

static bool ZWEI2_check_crc(BYTE *enc, DWORD enc_len, char *name, u32 crc1, u32 crc2)
{
	BYTE dec1[16] = "GURUguruMinmIN_";
	BYTE dec2[65] = "!~_Q#xDRgd-d&yfg&'(8)(5(594er2f4asf5f6e5f4s5fvwyjk%fgqTUCFD4568r";
	DWORD _crc1 = 0;
	DWORD _crc2 = 56879;

	strncpy((char *)&dec2[8], _strupr(name), 8);
    for (DWORD i = 0; i < enc_len; ++i) {
		enc[i] =  ~enc[i] ^ (BYTE)(dec2[i & 0x3f] * (dec1[i % 0xf] + (i & 7)));
		_crc1 += enc[i];
		_crc2 ^= enc[i];
    }

	if (crc1 != _crc1)
		printf("crc1 not matched\n");	
	if (crc2 != _crc2)
		printf("crc2 not matched\n");
printf("%x %x\n", _crc1, _crc2);
//	return crc1 != _crc1 || crc2 != _crc2;
	return 0;
}

static void ZWEI2_decode(BYTE *enc, DWORD enc_len)
{
    for (DWORD i = 0; i < enc_len; ++i) {
		BYTE tmp = (BYTE)i ^ ~enc[i];
		enc[i] = tmp >> 4 | tmp << 4;
	}
}

static int ZWEI2_decrypt(BYTE *enc, DWORD enc_len, const TCHAR *name)
{
	char _name[MAX_PATH];
	char *suffix;

	memset(_name, 0, sizeof(_name));
	unicode2acp(_name, MAX_PATH, PathFindFileName(name), -1);
	suffix = strchr(_name, '.');
	if (suffix)
		*suffix = 0;

	enc_len -= sizeof(itm_tailer_t);
	itm_tailer_t *itm = (itm_tailer_t *)(enc + enc_len);
	if (ZWEI2_check_crc(enc, enc_len, _name, itm->crc1, itm->crc2))
		return -CUI_EMATCH;

	if (itm->type == 1)
		ZWEI2_decode(enc, enc_len);

	return itm->type;
}

/********************* itm *********************/

/* 封包匹配回调函数 */
static int ZWEI2_itm_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

#if 0
/* 封包索引目录提取函数 */
static int ZWEI2_itm_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	dat_entry_t *index_buffer;
	unsigned int index_buffer_length;	
	unsigned int i;

	if (pkg->pio->read(pkg, &pkg_dir->index_entries, 4))
		return -CUI_EREAD;

	index_buffer_length = pkg_dir->index_entries * sizeof(dat_entry_t);
	index_buffer = new dat_entry_t[pkg_dir->index_entries];
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
		delete [] index_buffer;
		return -CUI_EREAD;
	}

	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(dat_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int ZWEI2_itm_parse_resource_info(struct package *pkg,
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
#endif

/* 封包资源提取函数 */
static int ZWEI2_itm_extract_resource(struct package *pkg,
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

	int ret = ZWEI2_decrypt(raw, pkg_res->raw_data_length, pkg->name);
	if (ret < 0) {
		delete [] raw;
		return ret;
	}

	if (ret == 0)
		pkg_res->replace_extension = _T(".bmp");	
	else if (ret == 1)
		pkg_res->replace_extension = _T(".txt");
	else if (ret == 3)
		pkg_res->replace_extension = _T(".wav");
	else
		printf("%s: unknown format\n", pkg_res->name);
	pkg_res->flags |= PKG_RES_FLAG_REEXT;
	pkg_res->raw_data = raw;
	pkg_res->raw_data_length -= sizeof(itm_tailer_t);

	return 0;
}

/* 资源保存函数 */
static int ZWEI2_itm_save_resource(struct resource *res, 
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
static void ZWEI2_itm_release_resource(struct package *pkg, 
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
static void ZWEI2_itm_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation ZWEI2_itm_operation = {
	ZWEI2_itm_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	ZWEI2_itm_extract_resource,	/* extract_resource */
	ZWEI2_itm_save_resource,	/* save_resource */
	ZWEI2_itm_release_resource,	/* release_resource */
	ZWEI2_itm_release			/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK ZWEI2_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".itm"), NULL, 
		NULL, &ZWEI2_itm_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	return 0;
}
