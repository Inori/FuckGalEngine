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
struct acui_information KScriptEngine_cui_information = {
	_T(""),					/* copyright */
	_T("KScriptEngine"),	/* system */
	_T(".kpc"),				/* package */
	_T("1.0.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-9-28 21:11"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[8];				// "SCRPACK1"
	u32 index_entries;
	u32 index_length;
	s8 package_name[16];		// 与封包名相同的字符串
} kpc_header_t;

typedef struct {
	s8 name[24];
	u32 offset;
	u32 length;
} kpc_entry_t;

typedef struct {
	s8 magic[4];				// "KSLM"
	u8 xor0;
	u8 xor1;
	u16 reserved;
	u32 data_length;
	u32 width;
	u32 height;
} ksl_header_t;

typedef struct {
	s8 magic[4];				// "GRPH"
	u8 xor0;
	u8 xor1;
	u16 reserved;
	u32 data_length;
	u32 is_multi_pic;			// 是否有分图信息
	u32 multi_pic_info_length;	// 分图信息
} kgp_header_t;
#pragma pack ()

static void *my_malloc(DWORD len)
{
	return new BYTE[len];
}

/********************* kpc *********************/

/* 封包匹配回调函数 */
static int KScriptEngine_kpc_match(struct package *pkg)
{
	s8 magic[8];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "SCRPACK1", 8)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int KScriptEngine_kpc_extract_directory(struct package *pkg,
											   struct package_directory *pkg_dir)
{
	kpc_header_t kpc_header;
	if (pkg->pio->readvec(pkg, &kpc_header, sizeof(kpc_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	BYTE *index_buffer = new BYTE[kpc_header.index_length];
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, kpc_header.index_length)) {
		delete [] index_buffer;
		return -CUI_EREAD;
	}

	for (DWORD i = 0; i < kpc_header.index_length; ++i)
		index_buffer[i] ^= 0x45;

	pkg_dir->index_entries = kpc_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = kpc_header.index_length;
	pkg_dir->index_entry_length = sizeof(kpc_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int KScriptEngine_kpc_parse_resource_info(struct package *pkg,
												 struct package_resource *pkg_res)
{
	kpc_entry_t *kpc_entry;

	kpc_entry = (kpc_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, kpc_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = kpc_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = kpc_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int KScriptEngine_kpc_extract_resource(struct package *pkg,
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

	if (!strncmp((char *)raw, "KSLM", 4)) {
		ksl_header_t *ksl = (ksl_header_t *)raw;
		BYTE *p = raw + sizeof(ksl_header_t);

		for (DWORD i = 0; i < ksl->data_length; ++i)
			p[i] ^= ksl->xor0 ^ ksl->xor1;

		if (MyBuildBMPFile(raw + sizeof(ksl_header_t), ksl->data_length, NULL, 0,
				ksl->width, ksl->height, 8, (BYTE **)&pkg_res->actual_data,
				&pkg_res->actual_data_length, my_malloc)) {
			delete [] raw;
			return -CUI_EMEM;
		}

		delete [] raw;
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".bmp");
	} else if (!strncmp((char *)raw, "GRPH", 4)) {
		kgp_header_t *kgp = (kgp_header_t *)raw;
		BYTE *p = raw + sizeof(kgp_header_t);
		// 忽略分图信息
		if (kgp->is_multi_pic && kgp->multi_pic_info_length / 16)
			p += kgp->multi_pic_info_length / 16 * 24;
		
		BYTE *act = new BYTE[kgp->data_length];
		if (!act) {
			delete [] raw;
			return -CUI_EMEM;
		}

		for (DWORD i = 0; i < kgp->data_length; ++i)
			act[i] = p[i] ^ kgp->xor0 ^ kgp->xor1;

		pkg_res->actual_data_length = kgp->data_length;
		pkg_res->actual_data = act;

		delete [] raw;
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".png");
	} else
		pkg_res->raw_data = raw;

	return 0;
}

/* 资源保存函数 */
static int KScriptEngine_kpc_save_resource(struct resource *res, 
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
static void KScriptEngine_kpc_release_resource(struct package *pkg, 
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
static void KScriptEngine_kpc_release(struct package *pkg, 
									  struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation KScriptEngine_kpc_operation = {
	KScriptEngine_kpc_match,				/* match */
	KScriptEngine_kpc_extract_directory,	/* extract_directory */
	KScriptEngine_kpc_parse_resource_info,	/* parse_resource_info */
	KScriptEngine_kpc_extract_resource,		/* extract_resource */
	KScriptEngine_kpc_save_resource,		/* save_resource */
	KScriptEngine_kpc_release_resource,		/* release_resource */
	KScriptEngine_kpc_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK KScriptEngine_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".kpc"), NULL, 
		NULL, &KScriptEngine_kpc_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
