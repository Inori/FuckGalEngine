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
using std::vector;

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information DDSystem_cui_information = {
	NULL,					/* copyright */
	_T("DDSystem"),			/* system */
	_T(".dat"),				/* package */
	_T("1.0.2"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-7-4 23:18"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];	// "DDP0" or "DDP1" or "DDP2" or "DDP3"
	u32 index_entries;
} DDP_header_t;

typedef struct {
	u32 size;
	u32 offset;
} DDP1_hash_entry_t;

// 文件的最后4字节是文件的大小
typedef struct {
	s8 magic[4];	// "DDP2"
	u32 index_entries;
	u32 data_offset;
	u32 unknown[5];	// 0
} DDP2_header_t;

typedef struct {
	u32 offset;
	u32 uncomprlen;
	u32 comprlen;
	u32 unknown;	// 0
} DDP2_entry_t;

// 文件的最后4字节是文件的大小
typedef struct {
	s8 magic[4];	// "DDP3"
	u32 hash_entries;
	u32 index_offset;
	u32 unknown[5];	// 0
} DDP3_header_t;

typedef struct {
	s8 magic[8];	// "DDSxHXB"
	u8 length[3];
	u8 flag;
	u32 unknown;	// 0
} HXB_header_t;
#pragma pack ()

/* .dat封包的索引项结构 */
typedef struct {
	char name[MAX_PATH];
	DWORD offset;
	DWORD comprlen;
	DWORD uncomprlen;
} my_dat_entry_t;

#define SWAP32(v)	((((v) & 0xff) << 24) | (((v) & 0xff00) << 8) | (((v) & 0xff0000) >> 8) | (((v) & 0xff000000) >> 24))

static DWORD do_hash(BYTE *name)
{
	DWORD hash = 0;
	DWORD seed = hash + 1;
	for (DWORD i = 0; name[i]; ++i) {
		hash ^= name[i] * seed;
		seed += 0x1f3;
	}
	return (hash >> 11) ^ hash;
}

static void ddp_uncompress(BYTE *uncompr, DWORD uncomprlen,
						   BYTE *compr, DWORD comprlen)
{
	DWORD curbyte = 0;
	DWORD act_uncomprlen = 0;
	while (act_uncomprlen < uncomprlen) {
		BYTE flag = compr[curbyte++];
		DWORD offset, copy_len;

		if (flag < 0x1d) {
			copy_len = flag + 1;
			offset = 0;
		} else if (flag == 0x1d) {
			copy_len = compr[curbyte++] + 0x1e;
			offset = 0;
		} else if (flag == 0x1e) {
			copy_len = ((compr[curbyte] << 8) | compr[curbyte + 1]) + 0x11e;
			curbyte += 2;
			offset = 0;			
		} else if (flag == 0x1f) {
			copy_len = (compr[curbyte] << 24) | (compr[curbyte + 1] << 16) 
				| (compr[curbyte + 2] << 8) | compr[curbyte + 3];
			curbyte += 4;
			offset = 0;
		} else {
			if (flag < 0x80) {
				if ((flag & 0x60) == 0x20) {
					copy_len = flag & 3;
					offset = (flag >> 2) & 7;
				} else if ((flag & 0x60) == 0x40) {
					copy_len = (flag & 0x1f) + 4;
					offset = compr[curbyte++];
				} else {
					offset = ((flag & 0x1f) << 8) | compr[curbyte++];
					flag = compr[curbyte++];
					switch (flag) {
					case 0xfe:
						copy_len = ((compr[curbyte] << 8) | compr[curbyte + 1]) + 0x102;
						curbyte += 2;
						break;
					case 0xff:
						copy_len = (compr[curbyte] << 24) | (compr[curbyte + 1] << 16) 
							| (compr[curbyte + 2] << 8) | compr[curbyte + 3];
						curbyte += 4;
						break;
					default:
						copy_len = flag + 4;
					}
				}
			} else {
				copy_len = (flag >> 5) & 3;
				offset = ((flag & 0x1f) << 8) | compr[curbyte++];
			}
			++offset;
			copy_len += 3;
		}

		if (offset) {
			for (DWORD i = 0; i < copy_len; ++i) {
				uncompr[act_uncomprlen] = uncompr[act_uncomprlen - offset];
				++act_uncomprlen;
			}
		} else {
			for (DWORD i = 0; i < copy_len; ++i)
				uncompr[act_uncomprlen++] = compr[curbyte++];
		}
	}
}

/********************* dat *********************/

/* 封包匹配回调函数 */
static int DDSystem_DDP0_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "DDP0", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int DDSystem_DDP0_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{
	u32 index_entries;

	if (pkg->pio->read(pkg, &index_entries, 4))
		return -CUI_EREAD;

	if (index_entries < 2)
		return -CUI_EMATCH;

	u32 *offset_buffer = (u32 *)malloc(index_entries * 4);
	if (!offset_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, offset_buffer, index_entries * 4)) {
		free(offset_buffer);
		return -CUI_EREAD;
	}

	DWORD my_index_buffer_len = index_entries * sizeof(my_dat_entry_t);
	my_dat_entry_t *my_index_buffer = (my_dat_entry_t *)malloc(my_index_buffer_len);
	if (!my_index_buffer) {
		free(offset_buffer);
		return -CUI_EMEM;
	}

	my_dat_entry_t *entry = my_index_buffer;
	for (DWORD i = 0; i < index_entries; ++i) {
		sprintf(entry->name, "%08d", i);		

		if (pkg->pio->readvec(pkg, &entry->comprlen, 4, 
				offset_buffer[i], IO_SEEK_SET))
			break;

		if (pkg->pio->read(pkg, &entry->uncomprlen, 4))
			break;

		if (!entry->comprlen) {
			entry->comprlen = entry->uncomprlen;
			entry->uncomprlen = 0;
		}
		entry->offset = offset_buffer[i] + 8;
		++entry;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = my_index_buffer_len;
	pkg_dir->index_entry_length = sizeof(my_dat_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int DDSystem_DDP0_parse_resource_info(struct package *pkg,
											 struct package_resource *pkg_res)
{
	my_dat_entry_t *dat_entry;

	dat_entry = (my_dat_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, dat_entry->name);
	pkg_res->name_length = -1;
	if (dat_entry->comprlen) {
		pkg_res->raw_data_length = dat_entry->comprlen;
		pkg_res->actual_data_length = dat_entry->uncomprlen;
	} else {
		pkg_res->raw_data_length = dat_entry->uncomprlen;
		pkg_res->actual_data_length = 0;
	}
	pkg_res->raw_data_length = dat_entry->comprlen == 0 ? dat_entry->uncomprlen : dat_entry->comprlen;
	pkg_res->actual_data_length = dat_entry->uncomprlen;
	pkg_res->offset = dat_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int DDSystem_DDP0_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	BYTE *compr = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(compr);
			return -CUI_EREADVEC;
	}	

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = compr;
		return 0;
	}

	BYTE *act_data;
	DWORD act_data_len;
	if (pkg_res->actual_data_length) {
		BYTE *uncompr = (BYTE *)malloc(pkg_res->actual_data_length);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}
		ddp_uncompress(uncompr, pkg_res->actual_data_length, 
			compr, pkg_res->raw_data_length);
		free(compr);
		pkg_res->actual_data = uncompr;
		act_data = uncompr;
		act_data_len = pkg_res->actual_data_length;
	} else {
		pkg_res->raw_data = compr;
		act_data = compr;
		act_data_len = pkg_res->raw_data_length;
	}

	if (!memcmp(act_data, "DDSxHXB", 8)) {
		HXB_header_t *HXB = (HXB_header_t *)act_data;

		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".HXB");

		int seed = HXB->length[0] << 16 | HXB->length[1] << 8 | HXB->length[2];
		if (act_data_len < 16 || seed != act_data_len)
			return 0;

		int key = (((seed << 5) ^ 0xa5) * (seed + 0x6f349)) ^ 0x34A9B129;
		u32 *p = (u32 *)(HXB + 1);
		for (int i = 0; i < (seed - 13) / 4; ++i)
			p[i] ^= key;

		if (HXB->flag) {
			u8 *p = (u8 *)(HXB + 1);
			// skip 2 strings
			while (*p++);
			while (*p++);		
		}		
	} else if (!memcmp(act_data, "DDSxSpt", 8)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".Spt");
	} else if (!memcmp(act_data, "BM", 2)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".bmp");
	} else if (!memcmp(act_data, "RIFF", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".wav");
	} else if (!memcmp(act_data, "OggS", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".ogg");
	} else if (!memcmp(act_data + act_data_len - 0x12, "TRUEVISION-XFILE.", 0x12)
			/* for no signature */	
			|| (*(u16 *)&act_data[12] * *(u16 *)&act_data[14] * 4 + 0x12 == (int)act_data_len)
			) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".tga");
	}

	return 0;
}

/* 资源保存函数 */
static int DDSystem_DDP0_save_resource(struct resource *res, 
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
static void DDSystem_DDP0_release_resource(struct package *pkg, 
										   struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

/* 封包卸载函数 */
static void DDSystem_DDP0_release(struct package *pkg, 
								  struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation DDSystem_DDP0_operation = {
	DDSystem_DDP0_match,				/* match */
	DDSystem_DDP0_extract_directory,	/* extract_directory */
	DDSystem_DDP0_parse_resource_info,	/* parse_resource_info */
	DDSystem_DDP0_extract_resource,		/* extract_resource */
	DDSystem_DDP0_save_resource,		/* save_resource */
	DDSystem_DDP0_release_resource,		/* release_resource */
	DDSystem_DDP0_release				/* release */
};

/********************* dat *********************/

/* 封包匹配回调函数 */
static int DDSystem_DDP1_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "DDP1", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int DDSystem_DDP1_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{
	u32 hash_entries;

	if (pkg->pio->read(pkg, &hash_entries, 4))
		return -CUI_EREAD;

	DWORD hash_index_length = hash_entries * sizeof(DDP1_hash_entry_t);
	DDP1_hash_entry_t *hash_index = (DDP1_hash_entry_t *)malloc(hash_index_length);
	if (!hash_index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, hash_index, hash_index_length)) {
		free(hash_index);
		return -CUI_EREAD;
	}

	vector<my_dat_entry_t> dat_index;
	int ret = 0;
	for (DWORD i = 0; i < hash_entries; ++i) {
		if (!hash_index[i].size)	// 没有资源缓存在该hash中
			continue;

		// 为缓存在该hash中的所有资源项分配内存
		BYTE *info_entry = (BYTE *)malloc(hash_index[i].size);
		if (!info_entry) {
			ret = -CUI_EMEM;
			break;
		}

		if (pkg->pio->readvec(pkg, info_entry, hash_index[i].size, hash_index[i].offset, IO_SEEK_SET)) {
			free(info_entry);
			ret = -CUI_EREAD;
			break;
		}

		DWORD len = 0;
		BYTE *p = info_entry;
		while (p < info_entry + hash_index[i].size) {
			DWORD entrys_size = *p++;
			if (!entrys_size)
				break;

			my_dat_entry_t dat_entry;
			dat_entry.offset = SWAP32(*(u32 *)p);
			p += 4;
			if (pkg->pio->readvec(pkg, &dat_entry.comprlen, 
					4, dat_entry.offset, IO_SEEK_SET)) {
				ret = -CUI_EREADVEC;
				break;
			}
			if (pkg->pio->read(pkg, &dat_entry.uncomprlen, 4)) {
				ret = -CUI_EREAD;
				break;
			}
			if (!dat_entry.comprlen) {
				dat_entry.comprlen = dat_entry.uncomprlen;
				dat_entry.uncomprlen = 0;
			}
			dat_entry.offset += 8;
			int name_len = entrys_size - 5;
			strncpy(dat_entry.name, (char *)p, name_len);
			dat_entry.name[name_len] = 0;
			p += name_len;
			dat_index.push_back(dat_entry);
		}	
		free(info_entry);
		if (ret)
			break;
	}
	free(hash_index);
	if (ret)
		return ret;		

	DWORD my_index_buffer_len = dat_index.size() * sizeof(my_dat_entry_t);
	my_dat_entry_t *my_index_buffer = (my_dat_entry_t *)malloc(my_index_buffer_len);
	if (!my_index_buffer)
		return -CUI_EMEM;

	for (i = 0; i < dat_index.size(); ++i) {
		strcpy(my_index_buffer[i].name, dat_index[i].name);
		my_index_buffer[i].offset = dat_index[i].offset;
		my_index_buffer[i].uncomprlen = dat_index[i].uncomprlen;
		my_index_buffer[i].comprlen = dat_index[i].comprlen;
	}

	pkg_dir->index_entries = dat_index.size();
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = my_index_buffer_len;
	pkg_dir->index_entry_length = sizeof(my_dat_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int DDSystem_DDP1_parse_resource_info(struct package *pkg,
											 struct package_resource *pkg_res)
{
	my_dat_entry_t *dat_entry;

	dat_entry = (my_dat_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, dat_entry->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = dat_entry->comprlen;
	pkg_res->actual_data_length = dat_entry->uncomprlen;
	pkg_res->offset = dat_entry->offset;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation DDSystem_DDP1_operation = {
	DDSystem_DDP1_match,				/* match */
	DDSystem_DDP1_extract_directory,	/* extract_directory */
	DDSystem_DDP1_parse_resource_info,	/* parse_resource_info */
	DDSystem_DDP0_extract_resource,		/* extract_resource */
	DDSystem_DDP0_save_resource,		/* save_resource */
	DDSystem_DDP0_release_resource,		/* release_resource */
	DDSystem_DDP0_release				/* release */
};

/********************* dat *********************/

/* 封包匹配回调函数 */
static int DDSystem_DDP2_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "DDP2", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int DDSystem_DDP2_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{
	DDP2_header_t header;

	if (pkg->pio->readvec(pkg, &header, sizeof(header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD index_length = header.index_entries * sizeof(DDP2_entry_t);
	DDP2_entry_t *index = (DDP2_entry_t *)malloc(index_length);
	if (!index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index, index_length)) {
		free(index);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = header.index_entries;
	pkg_dir->directory = index;
	pkg_dir->directory_length = index_length;
	pkg_dir->index_entry_length = sizeof(DDP2_entry_t);
	pkg_dir->flags= PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int DDSystem_DDP2_parse_resource_info(struct package *pkg,
											 struct package_resource *pkg_res)
{
	DDP2_entry_t *entry;

	entry = (DDP2_entry_t *)pkg_res->actual_index_entry;
	sprintf(pkg_res->name, "%06d", pkg_res->index_number);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = entry->comprlen;
	pkg_res->actual_data_length = entry->uncomprlen;
	pkg_res->offset = entry->offset;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation DDSystem_DDP2_operation = {
	DDSystem_DDP2_match,				/* match */
	DDSystem_DDP2_extract_directory,	/* extract_directory */
	DDSystem_DDP2_parse_resource_info,	/* parse_resource_info */
	DDSystem_DDP0_extract_resource,		/* extract_resource */
	DDSystem_DDP0_save_resource,		/* save_resource */
	DDSystem_DDP0_release_resource,		/* release_resource */
	DDSystem_DDP0_release				/* release */
};

/********************* dat *********************/

/* 封包匹配回调函数 */
static int DDSystem_DDP3_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "DDP3", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int DDSystem_DDP3_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{
	DDP3_header_t header;

	if (pkg->pio->readvec(pkg, &header, sizeof(header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD hash_index_length = header.hash_entries * sizeof(DDP1_hash_entry_t);
	DDP1_hash_entry_t *hash_index = (DDP1_hash_entry_t *)malloc(hash_index_length);
	if (!hash_index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, hash_index, hash_index_length)) {
		free(hash_index);
		return -CUI_EREAD;
	}

	vector<my_dat_entry_t> dat_index;
	int ret = 0;
	for (DWORD i = 0; i < header.hash_entries; ++i) {
		if (!hash_index[i].size)	// 没有资源缓存在该hash中
			continue;

		// 为缓存在该hash中的所有资源项分配内存
		BYTE *info_entry = (BYTE *)malloc(hash_index[i].size);
		if (!info_entry) {
			ret = -CUI_EMEM;
			break;
		}

		if (pkg->pio->readvec(pkg, info_entry, hash_index[i].size, hash_index[i].offset, IO_SEEK_SET)) {
			free(info_entry);
			ret = -CUI_EREAD;
			break;
		}

		DWORD len = 0;
		BYTE *p = info_entry;
		while (p < info_entry + hash_index[i].size) {
			DWORD entrys_size = *p++;
			if (!entrys_size)
				break;

			my_dat_entry_t dat_entry;
			dat_entry.offset = *(u32 *)p;
			p += 4;
			dat_entry.uncomprlen = *(u32 *)p;
			p += 4;	
			dat_entry.comprlen = *(u32 *)p;
			p += 4;
			if (!dat_entry.comprlen) {
				dat_entry.comprlen = dat_entry.uncomprlen;
				dat_entry.uncomprlen = 0;
			}
			// skip unknown(0)
			p += 4;

			int name_len = entrys_size - 17;
			strncpy(dat_entry.name, (char *)p, name_len);
			dat_entry.name[name_len] = 0;
			p += name_len;
			dat_index.push_back(dat_entry);
		}	
		free(info_entry);
		if (ret)
			break;
	}
	free(hash_index);
	if (ret)
		return ret;		

	DWORD my_index_buffer_len = dat_index.size() * sizeof(my_dat_entry_t);
	my_dat_entry_t *my_index_buffer = (my_dat_entry_t *)malloc(my_index_buffer_len);
	if (!my_index_buffer)
		return -CUI_EMEM;

	for (i = 0; i < dat_index.size(); ++i) {
		strcpy(my_index_buffer[i].name, dat_index[i].name);
		my_index_buffer[i].offset = dat_index[i].offset;
		my_index_buffer[i].uncomprlen = dat_index[i].uncomprlen;
		my_index_buffer[i].comprlen = dat_index[i].comprlen;
	}

	pkg_dir->index_entries = dat_index.size();
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = my_index_buffer_len;
	pkg_dir->index_entry_length = sizeof(my_dat_entry_t);

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation DDSystem_DDP3_operation = {
	DDSystem_DDP3_match,				/* match */
	DDSystem_DDP3_extract_directory,	/* extract_directory */
	DDSystem_DDP1_parse_resource_info,	/* parse_resource_info */
	DDSystem_DDP0_extract_resource,		/* extract_resource */
	DDSystem_DDP0_save_resource,		/* save_resource */
	DDSystem_DDP0_release_resource,		/* release_resource */
	DDSystem_DDP0_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK DDSystem_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &DDSystem_DDP3_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &DDSystem_DDP2_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &DDSystem_DDP1_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &DDSystem_DDP0_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
