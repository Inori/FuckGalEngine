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
#include <utility.h>

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information Seraphim_cui_information = {
	_T("Assemblage Corporation / A.Sumeragi&Fantom"),		/* copyright */
	_T("Seraphim"),			/* system */
	_T(".dat .wav"),		/* package */
	_T("1.0.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-3-24 8:52"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	u32 index_entries;
} ScnPac_header_t;

typedef struct {
	u32 offset;
} ScnPac_entry_t;

typedef struct {
	u32 is_compr;	// ?
	u32 offset;
	u32 length;
} Voice_entry_t;
#pragma pack ()

typedef struct {
	char name[MAX_PATH];
	u32 offset;
	u32 length;
} my_ScnPac_entry_t;

static int ScnPac_rle_uncompress(BYTE **ret_uncompr, DWORD *ret_uncomprlen, 
								 BYTE *compr, DWORD comprlen)
{
	DWORD uncomprlen = *(u32 *)compr;
	BYTE *uncompr = new BYTE[uncomprlen];
	if (!uncompr)
		return -CUI_EMEM;
	DWORD curbyte = 4;
	DWORD act_uncomprlen = 0;
	while (act_uncomprlen < uncomprlen) {
		DWORD copy_bytes;		

		// saint check
		if (curbyte >= comprlen)
			break;
		BYTE flag = compr[curbyte++];
		if (flag & 0x80) {
			// saint check
			if (curbyte >= comprlen)
				break;			
			DWORD tmp = (flag << 8) | compr[curbyte++];
			DWORD offset = ((tmp >> 5) & 0x3FF) + 1;			
			copy_bytes = (tmp & 0x1F) + 1;
//printf("[%x] copy1 %x from %x %x\n", curbyte, copy_bytes, offset, act_uncomprlen);
			BYTE *src = uncompr + act_uncomprlen - offset; 
			do {
				// saint check
				if (act_uncomprlen >= uncomprlen)
					break;
				uncompr[act_uncomprlen++] = *src++;
				--copy_bytes;
			} while (copy_bytes);
		} else {
			copy_bytes = flag + 1;
//printf("[%x] copy2 %x\n", curbyte, copy_bytes);
			do {
				// saint check
				if (curbyte >= comprlen)
					break;
				// saint check
				if (act_uncomprlen >= uncomprlen)
					break;
				uncompr[act_uncomprlen++] = compr[curbyte++];
				--copy_bytes;
			} while (copy_bytes);
		}
	}

	if (curbyte != comprlen || act_uncomprlen != uncomprlen) {
		//printf("rle uncompress failed (%x %x %x %x)\n", 
		//	curbyte, comprlen, act_uncomprlen, uncomprlen);
		delete [] uncompr;
		return -CUI_EMATCH;
	}

	*ret_uncompr = uncompr;
	*ret_uncomprlen = uncomprlen;

	return 0;
}

/* 资源保存函数 */
static int Seraphim_save_resource(struct resource *res, 
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
static void Seraphim_release_resource(struct package *pkg, 
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
static void Seraphim_release(struct package *pkg, 
						   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/********************* ScnPac.dat *********************/

/* 封包匹配回调函数 */
static int Seraphim_ScnPac_match(struct package *pkg)
{
	if (lstrcmpi(pkg->name, _T("ScnPac.Dat")))
		return -CUI_EMATCH;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包索引目录提取函数 */
static int Seraphim_ScnPac_extract_directory(struct package *pkg,
											 struct package_directory *pkg_dir)
{
	if (pkg->pio->read(pkg, &pkg_dir->index_entries, 4))
		return -CUI_EREAD;

	DWORD index_length = sizeof(my_ScnPac_entry_t) * pkg_dir->index_entries;
	my_ScnPac_entry_t *index_buffer = new my_ScnPac_entry_t[pkg_dir->index_entries];
	if (!index_buffer)
		return -CUI_EMEM;

	DWORD orig_index_length = sizeof(u32) * (pkg_dir->index_entries + 1);
	u32 *orig_index_buffer = new u32[pkg_dir->index_entries + 1];
	if (!orig_index_buffer) {
		delete [] index_buffer;
		return -CUI_EMEM;
	}

	if (pkg->pio->read(pkg, orig_index_buffer, orig_index_length)) {
		delete [] orig_index_buffer;
		delete [] index_buffer;
		return -CUI_EREAD;
	}

	for (DWORD i = 0; i < pkg_dir->index_entries; ++i) {
		sprintf(index_buffer[i].name, "%04d", i);
		index_buffer[i].offset = orig_index_buffer[i];
		index_buffer[i].length = orig_index_buffer[i + 1] - orig_index_buffer[i];
	}
	delete [] orig_index_buffer; 

	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_length;
	pkg_dir->index_entry_length = sizeof(my_ScnPac_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int Seraphim_ScnPac_parse_resource_info(struct package *pkg,
											   struct package_resource *pkg_res)
{
	my_ScnPac_entry_t *my_ScnPac_entry;

	my_ScnPac_entry = (my_ScnPac_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_ScnPac_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_ScnPac_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_ScnPac_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int Seraphim_ScnPac_extract_resource(struct package *pkg,
											struct package_resource *pkg_res)
{
	u32 is_compr;
	if (pkg->pio->readvec(pkg, &is_compr, 4, pkg_res->offset, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD comprlen = pkg_res->raw_data_length - 4;
	BYTE *compr = new BYTE[comprlen];
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, pkg_res->offset + 4, IO_SEEK_SET)) {
		delete [] compr;
		return -CUI_EREADVEC;
	}

	BYTE *act_data;
	DWORD act_data_length;
	if (is_compr) {
		DWORD uncomprlen = comprlen * 2;
		BYTE *uncompr = NULL;
		while (1) {
			uncompr = new BYTE[uncomprlen];
			if (!uncompr) {
				delete [] compr;
				return -CUI_EMEM;
			}

			DWORD act_uncomprlen = uncomprlen;
			int ret = uncompress(uncompr, &act_uncomprlen, compr, comprlen);
			if (ret == Z_OK) {
				uncomprlen = act_uncomprlen;
				break;
			}

			delete [] uncompr;
			uncomprlen *= 2;
		}
		delete [] compr;
		act_data = uncompr;
		act_data_length = uncomprlen;
	} else {
		act_data = compr;
		act_data_length = comprlen;
	}

	if (ScnPac_rle_uncompress((BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, 
			act_data, act_data_length)) {
		pkg_res->raw_data = act_data;
		pkg_res->raw_data_length = act_data_length;
	} else
		delete [] act_data;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation Seraphim_ScnPac_operation = {
	Seraphim_ScnPac_match,				/* match */
	Seraphim_ScnPac_extract_directory,	/* extract_directory */
	Seraphim_ScnPac_parse_resource_info,/* parse_resource_info */
	Seraphim_ScnPac_extract_resource,	/* extract_resource */
	Seraphim_save_resource,				/* save_resource */
	Seraphim_release_resource,			/* release_resource */
	Seraphim_release					/* release */
};

/********************* VoiceX.dat *********************/

/* 封包匹配回调函数 */
static int Seraphim_Voice_match(struct package *pkg)
{
	if (!_tcsstr(pkg->name, _T("Voice")))
		return -CUI_EMATCH;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包索引目录提取函数 */
static int Seraphim_Voice_extract_directory(struct package *pkg,
											struct package_directory *pkg_dir)
{
	if (pkg->pio->read(pkg, &pkg_dir->index_entries, 2))
		return -CUI_EREAD;

	DWORD index_length = sizeof(Voice_entry_t) * pkg_dir->index_entries;
	Voice_entry_t *index_buffer = new Voice_entry_t[pkg_dir->index_entries];
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_length)) {
		delete [] index_buffer;
		return -CUI_EREAD;
	}

	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_length;
	pkg_dir->index_entry_length = sizeof(Voice_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int Seraphim_Voice_parse_resource_info(struct package *pkg,
											  struct package_resource *pkg_res)
{
	Voice_entry_t *Voice_entry;

	Voice_entry = (Voice_entry_t *)pkg_res->actual_index_entry;
	sprintf(pkg_res->name, "%04d", pkg_res->index_number);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = Voice_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = Voice_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int Seraphim_Voice_extract_resource(struct package *pkg,
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

	Voice_entry_t *Voice_entry = (Voice_entry_t *)pkg_res->actual_index_entry;
	if (Voice_entry->is_compr) {
		printf("%s: unknown %x\n");
		exit(0);
	} else
		pkg_res->raw_data = raw;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation Seraphim_Voice_operation = {
	Seraphim_Voice_match,				/* match */
	Seraphim_Voice_extract_directory,	/* extract_directory */
	Seraphim_Voice_parse_resource_info,	/* parse_resource_info */
	Seraphim_Voice_extract_resource,	/* extract_resource */
	Seraphim_save_resource,				/* save_resource */
	Seraphim_release_resource,			/* release_resource */
	Seraphim_release					/* release */
};

/********************* ArchPac.dat *********************/

/* 封包匹配回调函数 */
static int Seraphim_ArchPac_match(struct package *pkg)
{
	if (lstrcmpi(pkg->name, _T("ArchPac.dat")))
		return -CUI_EMATCH;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包索引目录提取函数 */
static int Seraphim_ArchPac_extract_directory(struct package *pkg,
											  struct package_directory *pkg_dir)
{
	u32 is_compr;
	if (pkg->pio->read(pkg, &is_compr, 4))
		return -CUI_EREAD;

	if (is_compr) {
		DWORD comprlen = 0x100000;
		BYTE *compr;
		while (1) {
			compr = new BYTE[comprlen];
			if (!compr)
				return -CUI_EMEM;

			if (pkg->pio->readvec(pkg, compr, comprlen, 4, IO_SEEK_SET)) {
				delete [] compr;
				return -CUI_EREADVEC;
			}

			DWORD uncomprlen = comprlen * 2;
			BYTE *uncompr;
			while (1) {
				uncompr = new BYTE[uncomprlen];
				if (!uncompr) {
					delete [] compr;
					return -CUI_EMEM;
				}

				DWORD act_uncomprlen = uncomprlen;
				int ret = uncompress(uncompr, &act_uncomprlen, compr, comprlen);
				if (ret == Z_OK) {
					uncomprlen = act_uncomprlen;
					break;
				}

				delete [] uncompr;
				uncomprlen *= 2;
			}
			MySaveFile(_T("aaa"), uncompr, uncomprlen);exit(0);
		}
	}

	return 0;
}

/* 封包索引项解析函数 */
static int Seraphim_ArchPac_parse_resource_info(struct package *pkg,
												struct package_resource *pkg_res)
{
	Voice_entry_t *Voice_entry;

	Voice_entry = (Voice_entry_t *)pkg_res->actual_index_entry;
	sprintf(pkg_res->name, "%04d", pkg_res->index_number);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = Voice_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = Voice_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int Seraphim_ArchPac_extract_resource(struct package *pkg,
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

	Voice_entry_t *Voice_entry = (Voice_entry_t *)pkg_res->actual_index_entry;
	if (Voice_entry->is_compr) {
		printf("%s: unknown %x\n");
		exit(0);
	} else
		pkg_res->raw_data = raw;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation Seraphim_ArchPac_operation = {
	Seraphim_ArchPac_match,					/* match */
	Seraphim_ArchPac_extract_directory,		/* extract_directory */
	Seraphim_ArchPac_parse_resource_info,	/* parse_resource_info */
	Seraphim_ArchPac_extract_resource,		/* extract_resource */
	Seraphim_save_resource,					/* save_resource */
	Seraphim_release_resource,				/* release_resource */
	Seraphim_release						/* release */
};
/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK Seraphim_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &Seraphim_ArchPac_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR 
		| CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".Dat"), NULL, 
		NULL, &Seraphim_ScnPac_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR 
		| CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), _T(".ogg"), 
		NULL, &Seraphim_Voice_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR 
		| CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	return 0;
}
