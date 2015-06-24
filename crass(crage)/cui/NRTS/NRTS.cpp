#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>
#include <vector>

using namespace std;

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information NRTS_cui_information = {
	_T(""),		/* copyright */
	_T(""),					/* system */
	_T(".dat"),				/* package */
	_T("1.0.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T(""),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
#pragma pack ()

typedef struct {
	char name[MAX_PATH];
	DWORD length;
	DWORD offset;
} dat_entry_t;

struct package_info {
	TCHAR *name;
	u32 key;
	DWORD index_number;
};

static struct package_info sav_package_list[] = {
	{
		_T("otome.sav"), 0x7f3e20eb
	},
	{
		NULL, 0
	}
};

static struct package_info wav_package_list[] = {
	{
		_T("bankSS00.dat"), 0xec497da5
	},
	{
		_T("bankSS01.dat"), 0xa6fe3074
	},
	{
		_T("bankSS02.dat"), 0xe46dfd39
	},
	{
		_T("bankSS03.dat"), 0x9e564ba3
	},
	{
		_T("bankSS04.dat"), 0xf59eca67
	},
	{
		_T("bankSS05.dat"), 0x5f37e2c9
	},
	{
		_T("bankSS06.dat"), 0x61da3cb2
	},
	{
		_T("bankSS07.dat"), 0xd70e49f8
	},
	{
		_T("bankSS08.dat"), 0x3842b415
	},
	{
		_T("bankSS09.dat"), 0xbe189d7a
	},
	{
		_T("bankSS10.dat"), 0xaf5d3e51
	},
	{
		_T("bankSS11.dat"), 0xf5363424
	},
	{
		_T("bankSS12.dat"), 0x7b5ec0eb
	},
	{
		_T("bankSS13.dat"), 0xea70e5c7
	},
	{
		_T("bankSS14.dat"), 0x37ffc309
	},
	{
		_T("bankSS15.dat"), 0x09ae78f1
	},
	{
		_T("bankSS16.dat"), 0x7361decc
	},
	{
		_T("bankSS17.dat"), 0xb8746439
	},
	{
		_T("bankSS18.dat"), 0x37a4be16
	},
	{
		_T("bankSSL01.dat"), 0xfeb9ae37
	},
	{
		NULL, 0
	}
};

static struct package_info dat_package_list[] = {
	{
		_T("bankM01.dat"), 0x9afd7521
	},
	{
		_T("bankT01.dat"), 0x78532afe
	},
	{
		_T("bankST01.dat"), 0xa7e415ea
	},
	{
		_T("bankA01.dat"), 0xe9fa3fae
	},
	{
		_T("bankSSN01.dat"), 0xb30f97cb
	},
	{
		NULL, 0
	}
};

static struct package_info fx_package_list[] = {
	{
		_T("bankSHD01.dat"), 0xa78eb1e5
	},
	{
		_T("bankSHD02.dat"), 0xa239fbc1
	},
	{
		_T("bankSHD03.dat"), 0xab76efae
	},
	{
		_T("bankSHD04.dat"), 0x18352792
	},
	{
		NULL, 0
	}
};


static int dat_readvec(struct package *pkg, void *_data, DWORD len, DWORD offset, 
					   DWORD method, u32 key)
{
	BYTE *data = (BYTE *)_data;
	int ret = pkg->pio->readvec(pkg, data, len, offset, method);
	if (ret)
		return ret;

	DWORD align = offset & 3;
	if (align) {
		for (DWORD i = align; ((i < 4) && len); ++i) {
			*data++ ^= ((BYTE *)&key)[i];
			--len;
		}
	}

	u32 *p = (u32 *)data;
	for (DWORD i = 0; i < len / 4; ++i)
		p[i] ^= key;
	data += 4 * i;

	align = len & 3;
	for (i = 0; i < align; ++i)
		data[i] ^= ((BYTE *)&key)[i];

	return 0;
}

static int dat_readvec_last(struct package *pkg, void *_data, DWORD len, DWORD offset, 
							DWORD method, u32 key)
{
	BYTE *data = (BYTE *)_data;
	int ret = pkg->pio->readvec(pkg, data, len, offset, method);
	if (ret)
		return ret;

	DWORD align = offset & 3;
	if (align) {
		for (DWORD i = align; ((i < 4) && len); ++i) {
			*data++ ^= ((BYTE *)&key)[i];
			--len;
		}
	}

	u32 *p = (u32 *)data;
	for (DWORD i = 0; i < len / 4; ++i)
		p[i] ^= key;

	return 0;
}

static int dat_read(struct package *pkg, void *data, DWORD len, u32 key)
{
	u32 offset;
	pkg->pio->locate(pkg, &offset);
	return dat_readvec(pkg, data, len, offset, IO_SEEK_SET, key);
}

/********************* dat *********************/

/* 封包匹配回调函数 */
static int NRTS_dat_match(struct package *pkg)
{
	for (DWORD pid = 0; dat_package_list[pid].name; ++pid) {
		if (!_tcscmp(pkg->name, dat_package_list[pid].name))
			break;
	}
	if (!dat_package_list[pid].name)
		return -CUI_EMATCH;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	package_set_private(pkg, &dat_package_list[pid]);

	return 0;	
}

/* 封包索引目录提取函数 */
static int NRTS_dat_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	u32 fsize;
	if (pkg->pio->length_of(pkg, &fsize))
		return -CUI_ELEN;

	u32 key = ((struct package_info *)package_get_private(pkg))->key;

	vector <dat_entry_t> index;
	if (!_tcscmp(pkg->name, _T("bankSSN01.dat"))) {
		for (DWORD offset = 0; offset + 4 < fsize; ) {
			dat_entry_t entry;
			u32 magic;

			if (dat_readvec(pkg, &magic, 4, offset, IO_SEEK_SET, key))
				return -CUI_EREADVEC;
			
			if (magic == 'FFIR') {
				if (dat_read(pkg, &entry.length, 4, key))
					return -CUI_EREAD;	
				sprintf(entry.name, "%08d", index.size());
				entry.offset = offset;
				entry.length += 8;
				offset += entry.length;
				index.push_back(entry);	
			} else
				++offset;
		}
	} else if (!_tcscmp(pkg->name, _T("bankT01.dat"))) {
		for (DWORD offset = 0; offset + 18 < fsize; ) {
			BYTE tga_header[18];
			dat_entry_t entry;
printf("%x\n", offset);
			if (dat_readvec(pkg, tga_header, 18, offset, IO_SEEK_SET, key))
				return -CUI_EREADVEC;

			if (tga_header[2] == 2) {
				entry.length = 18 + *(u16 *)&tga_header[12] * *(u16 *)&tga_header[14] * tga_header[16] / 8 + 8 + 16 + 2;
				sprintf(entry.name, "%08d", index.size());
				entry.offset = offset;
				offset += entry.length;
				index.push_back(entry);	
			} else if (tga_header[2] == 10) {
				BYTE magic[] = "TRUEVISION-XFILE.";
				entry.offset = offset;
				offset += 18;
				for (DWORD k = 0; k < 18; ) {
					BYTE tmp;

					if (dat_read(pkg, &tmp, 1, key))
						return -CUI_EREAD;

					++offset;

					if (tmp != magic[k])
						k = 0;
					else
						++k;					
				}
				entry.length = offset - entry.offset;
				sprintf(entry.name, "%08d", index.size());
				index.push_back(entry);	
			} else if (!memcmp(tga_header, "DDS ", 4)) {
				offset += 18;

				printf("sdf @ offset %x\n",offset);exit(0);
			}

		}		
	}
	
	DWORD index_buffer_length= index.size() * sizeof(dat_entry_t);
	dat_entry_t *index_buffer = new dat_entry_t[index.size()];
	if (!index_buffer)
		return -CUI_EMEM;

	for (DWORD i = 0; i < index.size(); ++i)
		index_buffer[i] = index[i];

	pkg_dir->index_entries = index.size();
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(dat_entry_t);

	struct package_info *info = (struct package_info *)package_get_private(pkg);
	info->index_number = pkg_dir->index_entries;

	return 0;
}

/* 封包索引项解析函数 */
static int NRTS_dat_parse_resource_info(struct package *pkg,
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

/* 封包资源提取函数 */
static int NRTS_extract_resource(struct package *pkg,
								 struct package_resource *pkg_res)
{
	BYTE *raw = new BYTE[pkg_res->raw_data_length];
	if (!raw)
		return -CUI_EMEM;

	struct package_info *info = (struct package_info *)package_get_private(pkg);
	if (info->index_number != pkg_res->index_number + 1) {
		if (dat_readvec(pkg, raw, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET, info->key)) {
			delete [] raw;
			return -CUI_EREADVEC;
		}
	} else {
		if (dat_readvec(pkg, raw, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET, info->key)) {
			delete [] raw;
			return -CUI_EREADVEC;
		}
		printf("sdf\n");
	}

	if (!memcmp(raw, "RIFF", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".wav");
	} else if (memcmp(raw + 18 + *(u16 *)&raw[12] * *(u16 *)&raw[14] * raw[16] / 8, 
			"TRUEVISION-XFILE.", 18)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".tga");
	}

	pkg_res->raw_data = raw;

	return 0;
}

/* 资源保存函数 */
static int NRTS_save_resource(struct resource *res, 
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
static void NRTS_release_resource(struct package *pkg, 
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
static void NRTS_release(struct package *pkg, 
						 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	package_set_private(pkg, 0);

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation NRTS_dat_operation = {
	NRTS_dat_match,					/* match */
	NRTS_dat_extract_directory,		/* extract_directory */
	NRTS_dat_parse_resource_info,	/* parse_resource_info */
	NRTS_extract_resource,			/* extract_resource */
	NRTS_save_resource,				/* save_resource */
	NRTS_release_resource,			/* release_resource */
	NRTS_release					/* release */
};

/********************* wav *********************/

/* 封包匹配回调函数 */
static int NRTS_wav_match(struct package *pkg)
{
	for (DWORD pid = 0; wav_package_list[pid].name; ++pid) {
		if (!_tcscmp(pkg->name, wav_package_list[pid].name))
			break;
	}
	if (!wav_package_list[pid].name)
		return -CUI_EMATCH;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	package_set_private(pkg, &wav_package_list[pid]);

	return 0;	
}

static cui_ext_operation NRTS_wav_operation = {
	NRTS_wav_match,			/* match */
	NULL,					/* extract_directory */
	NULL,					/* parse_resource_info */
	NRTS_extract_resource,	/* extract_resource */
	NRTS_save_resource,		/* save_resource */
	NRTS_release_resource,	/* release_resource */
	NRTS_release			/* release */
};

/********************* sav *********************/

/* 封包匹配回调函数 */
static int NRTS_sav_match(struct package *pkg)
{
	for (DWORD pid = 0; sav_package_list[pid].name; ++pid) {
		if (!_tcscmp(pkg->name, sav_package_list[pid].name))
			break;
	}
	if (!sav_package_list[pid].name)
		return -CUI_EMATCH;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	package_set_private(pkg, &sav_package_list[pid]);

	return 0;	
}

static cui_ext_operation NRTS_sav_operation = {
	NRTS_sav_match,			/* match */
	NULL,					/* extract_directory */
	NULL,					/* parse_resource_info */
	NRTS_extract_resource,	/* extract_resource */
	NRTS_save_resource,		/* save_resource */
	NRTS_release_resource,	/* release_resource */
	NRTS_release			/* release */
};

/********************* fx *********************/

/* 封包匹配回调函数 */
static int NRTS_fx_match(struct package *pkg)
{
	for (DWORD pid = 0; fx_package_list[pid].name; ++pid) {
		if (!_tcscmp(pkg->name, fx_package_list[pid].name))
			break;
	}
	if (!fx_package_list[pid].name)
		return -CUI_EMATCH;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	package_set_private(pkg, &fx_package_list[pid]);

	return 0;	
}

static cui_ext_operation NRTS_fx_operation = {
	NRTS_fx_match,			/* match */
	NULL,					/* extract_directory */
	NULL,					/* parse_resource_info */
	NRTS_extract_resource,	/* extract_resource */
	NRTS_save_resource,		/* save_resource */
	NRTS_release_resource,	/* release_resource */
	NRTS_release			/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK NRTS_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".dat"), _T(".wav"), 
		NULL, &NRTS_wav_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &NRTS_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), _T(".fx"), 
		NULL, &NRTS_fx_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".sav"), _T(".sav_"), 
		NULL, &NRTS_sav_operation, CUI_EXT_FLAG_PKG))
			return -1;

	return 0;
}
