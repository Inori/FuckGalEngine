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
#include <vector>

using namespace std;

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information NekoSDK_cui_information = {
	_T("ふわふわ猫しっぽ"),	/* copyright */
	NULL,					/* system */
	_T(".pak .dat"),		/* package */
	_T("1.0.6"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-6-30 22:39"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[10];		/* "NEKOPACK4A" */
	u32 index_length;
} pak_header_t;

typedef struct {
	s8 name[128];
	u32 uncomprlen;
	u32 comprlen;
	u32 offset;
} dat_entry_t;

typedef struct {
	s8 name[260];
	u32 offset;
	u32 uncomprlen;
	u32 comprlen;	
} old_dat_entry_t;
#pragma pack ()

typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 length;
} my_pak_entry_t;

static int inline cal_name_xor_sum(s8 *name, u32 name_len)
{
	int sum = 0;

	for (u32 i = 0; i < name_len; i++)
		sum += name[i];

	return sum;
}

static DWORD lzss_uncompress(BYTE *uncompr, DWORD uncomprlen, 
							 BYTE *compr, DWORD comprlen)
{
	unsigned int act_uncomprlen = 0;
	/* compr中的当前字节中的下一个扫描位的位置 */
	unsigned int curbit = 0;
	/* compr中的当前扫描字节 */
	unsigned int curbyte = 0;
	unsigned int nCurWindowByte = 0xfee;
	unsigned int win_size = 4096;
	BYTE win[4096];
	WORD flag = 0;

	memset(win, 0, nCurWindowByte);
	while (1) {
		flag >>= 1;
		if (!(flag & 0x0100)) {
			flag = compr[curbyte++] | 0xff00;
			if (curbyte > comprlen)
				break;
		}

		if (flag & 1) {
			unsigned char data = compr[curbyte++];
			if (curbyte > comprlen)
				break;
			uncompr[act_uncomprlen++] = data;
			/* 输出的1字节放入滑动窗口 */
			win[nCurWindowByte++] = data;
			nCurWindowByte &= win_size - 1;
		} else {
			unsigned int copy_bytes, win_offset;
			unsigned int i;

			win_offset = compr[curbyte++];
			if (curbyte > comprlen)
				break;
			copy_bytes = compr[curbyte++];
			if (curbyte > comprlen)
				break;
			win_offset |= (copy_bytes & 0xf0) << 4;
			copy_bytes &= 0x0f;
			copy_bytes += 3;

			for (i = 0; i < copy_bytes; i++) {	
				unsigned char data;

				data = win[(win_offset + i) & (win_size - 1)];
				uncompr[act_uncomprlen++] = data;
				/* 输出的1字节放入滑动窗口 */
				win[nCurWindowByte++] = data;
				nCurWindowByte &= win_size - 1;	
			}
		}
	}

	return act_uncomprlen;
}

/********************* pak *********************/

/* 封包匹配回调函数 */
static int NekoSDK_pak_match(struct package *pkg)
{
	s8 magic[10];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "NEKOPACK4A", 10)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int NekoSDK_pak_extract_directory(struct package *pkg,
										 struct package_directory *pkg_dir)
{
	DWORD index_buffer_length;	
	u32 index_length;

	if (pkg->pio->read(pkg, &index_length, 4))
		return -CUI_EREAD;

	BYTE *index_buffer = (BYTE *)malloc(index_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	BYTE *p = index_buffer;
	for (DWORD index_entries = 0; *(u32 *)p; index_entries++) {
		u32 name_length = *(u32 *)p;	/* include NULL */
		p += 4 + name_length + 8;
	};
	if (!index_entries) {
		free(index_buffer);
		return -CUI_EMATCH;
	}

	index_buffer_length = index_entries * sizeof(my_pak_entry_t);
	my_pak_entry_t *my_index_buffer = (my_pak_entry_t *)malloc(index_buffer_length);
	if (!my_index_buffer) {
		free(index_buffer);
		return -CUI_EMEM;
	}

	p = index_buffer;
	for (DWORD i = 0; i < index_entries; i++) {
		u32 name_length;
		int xor_sum;
		
		name_length = *(u32 *)p;
		p += 4;
		strncpy(my_index_buffer[i].name, (char *)p, name_length);
		xor_sum = cal_name_xor_sum((char *)p, name_length);
		p += name_length;
		my_index_buffer[i].offset = *(u32 *)p ^ xor_sum;
		p += 4;
		my_index_buffer[i].length = *(u32 *)p ^ xor_sum;
		p += 4;
	}
	free(index_buffer);

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(my_pak_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int NekoSDK_pak_parse_resource_info(struct package *pkg,
										   struct package_resource *pkg_res)
{
	my_pak_entry_t *my_pak_entry;

	my_pak_entry = (my_pak_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_pak_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_pak_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = my_pak_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int NekoSDK_pak_extract_resource(struct package *pkg,
										struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	/* .alp是对应的同名bmp文件的alpha信息。
	 * 因为处理上不太方便，所以就不提取了。
	 */
	BYTE xor_magic;

	xor_magic = (BYTE)(comprlen / 8 + 0x22);
	for (unsigned int b = 0; b < 32; b++) {
		if (b > comprlen)
			break;

		compr[b] ^= xor_magic;
		xor_magic <<= 3;
	}

	uncomprlen = comprlen;
	while (1) {
		uncomprlen <<= 1;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}

		if (uncompress(uncompr, &uncomprlen, compr, comprlen) == Z_OK)
			break;
		free(uncompr);
	}

	pkg_res->raw_data = compr;
	pkg_res->raw_data_length = comprlen;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

/* 资源保存函数 */
static int NekoSDK_pak_save_resource(struct resource *res, 
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
static void NekoSDK_pak_release_resource(struct package *pkg, 
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
static void NekoSDK_pak_release(struct package *pkg, 
								struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation NekoSDK_pak_operation = {
	NekoSDK_pak_match,					/* match */
	NekoSDK_pak_extract_directory,		/* extract_directory */
	NekoSDK_pak_parse_resource_info,	/* parse_resource_info */
	NekoSDK_pak_extract_resource,		/* extract_resource */
	NekoSDK_pak_save_resource,			/* save_resource */
	NekoSDK_pak_release_resource,		/* release_resource */
	NekoSDK_pak_release					/* release */
};

/********************* pak *********************/

/* 封包匹配回调函数 */
static int NekoSDK_paks_match(struct package *pkg)
{
	s8 magic[10];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "NEKOPACK4S", 10)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引项解析函数 */
static int NekoSDK_paks_parse_resource_info(struct package *pkg,
											struct package_resource *pkg_res)
{
	u32 name_length, offset, length;

	if (pkg->pio->read(pkg, &name_length, 4))
		return -CUI_EREAD;

	char *name = (char *)malloc(name_length);
	if (!name)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, name, name_length)) {
		free(name);
		return -CUI_EREAD;
	}

	if (pkg->pio->read(pkg, &offset, 4)) {
		free(name);
		return -CUI_EREAD;
	}

	if (pkg->pio->read(pkg, &length, 4)) {
		free(name);
		return -CUI_EREAD;
	}

	int xor_sum = cal_name_xor_sum(name, name_length);
	offset ^= xor_sum;
	length ^= xor_sum;

	strcpy(pkg_res->name, name);
	free(name);
	pkg_res->name_length = name_length;
	pkg_res->raw_data_length = length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = offset;

	return 0;
}

/* 封包资源提取函数 */
static int NekoSDK_paks_extract_resource(struct package *pkg,
										struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREAD;
	}

	BYTE xor_magic;

	xor_magic = (BYTE)(comprlen / 8 + 0x22);
	for (unsigned int b = 0; b < 32; b++) {
		if (b > comprlen)
			break;

		compr[b] ^= xor_magic;
		xor_magic <<= 3;
	}

	uncomprlen = comprlen;
	while (1) {
		uncomprlen <<= 1;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}

		if (uncompress(uncompr, &uncomprlen, compr, comprlen) == Z_OK)
			break;
		free(uncompr);
	}

	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

/* 资源保存函数 */
static int NekoSDK_paks_save_resource(struct resource *res, 
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
static void NekoSDK_paks_release_resource(struct package *pkg, 
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
static void NekoSDK_paks_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation NekoSDK_paks_operation = {
	NekoSDK_paks_match,					/* match */
	NULL,								/* extract_directory */
	NekoSDK_paks_parse_resource_info,	/* parse_resource_info */
	NekoSDK_paks_extract_resource,		/* extract_resource */
	NekoSDK_paks_save_resource,			/* save_resource */
	NekoSDK_paks_release_resource,		/* release_resource */
	NekoSDK_paks_release				/* release */
};

/********************* dat *********************/

/* 封包匹配回调函数 */
static int NekoSDK_dat_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包索引目录提取函数 */
static int NekoSDK_dat_extract_directory(struct package *pkg,
										 struct package_directory *pkg_dir)
{
	vector<dat_entry_t> dat_index;
	while (1) {
		dat_entry_t entry;

		if (pkg->pio->read(pkg, &entry, sizeof(entry)))
			return -CUI_EREAD;

		if (!entry.offset && !entry.comprlen && !entry.uncomprlen)
			break;

		entry.offset ^= 0xcacaca;
		entry.comprlen ^= 0xcacaca;
		entry.uncomprlen ^= 0xcacaca;
		dat_index.push_back(entry);
	}

	dat_entry_t *index_buffer = new dat_entry_t[dat_index.size()];
	if (!index_buffer)
		return -CUI_EMEM;

	for (DWORD i = 0; i < dat_index.size(); ++i)
		index_buffer[i] = dat_index[i];

	pkg_dir->index_entries = dat_index.size();
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = sizeof(dat_entry_t) * pkg_dir->index_entries;
	pkg_dir->index_entry_length = sizeof(dat_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int NekoSDK_dat_parse_resource_info(struct package *pkg,
										   struct package_resource *pkg_res)
{
	dat_entry_t *dat_entry;

	dat_entry = (dat_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, dat_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = dat_entry->comprlen;
	pkg_res->actual_data_length = dat_entry->uncomprlen;
	pkg_res->offset = dat_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int NekoSDK_dat_extract_resource(struct package *pkg,
										struct package_resource *pkg_res)
{
	DWORD comprlen = pkg_res->raw_data_length;
	BYTE *compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, pkg_res->offset, IO_SEEK_SET)) {
		delete [] compr;
		return -CUI_EREADVEC;
	}

	if (pkg_res->actual_data_length) {
		DWORD uncomprlen = pkg_res->actual_data_length;
		BYTE *uncompr = new BYTE[uncomprlen];
		if (!uncompr) {
			delete [] compr;
			return -CUI_EMEM;
		}
		DWORD act_uncomprlen = lzss_uncompress(uncompr, 
			uncomprlen, compr, comprlen);
		delete [] compr;
		if (act_uncomprlen != uncomprlen) {
			delete [] uncompr;
			return -CUI_EUNCOMPR;
		}
		pkg_res->actual_data = uncompr;
		pkg_res->actual_data_length = uncomprlen;
	} else {
		pkg_res->raw_data = compr;
		pkg_res->raw_data_length = comprlen;
	}

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation NekoSDK_dat_operation = {
	NekoSDK_dat_match,					/* match */
	NekoSDK_dat_extract_directory,		/* extract_directory */
	NekoSDK_dat_parse_resource_info,	/* parse_resource_info */
	NekoSDK_dat_extract_resource,		/* extract_resource */
	NekoSDK_pak_save_resource,			/* save_resource */
	NekoSDK_pak_release_resource,		/* release_resource */
	NekoSDK_pak_release					/* release */
};

/********************* old dat *********************/

/* 封包匹配回调函数 */
static int NekoSDK_old_dat_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	old_dat_entry_t first_entry;

	if (pkg->pio->readvec(pkg, &first_entry, sizeof(first_entry), 0, IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_EREADVEC;
	}

	DWORD index_entries = first_entry.offset / sizeof(old_dat_entry_t) - 1;
	DWORD last_entry_offset = (index_entries - 1) * sizeof(old_dat_entry_t);

	old_dat_entry_t last_entry;
	if (pkg->pio->readvec(pkg, &last_entry, sizeof(last_entry), last_entry_offset, IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_EREADVEC;
	}

	u32 fsize;
	pkg->pio->length_of(pkg, &fsize);
	if (last_entry.offset + last_entry.comprlen != fsize) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int NekoSDK_old_dat_extract_directory(struct package *pkg,
											 struct package_directory *pkg_dir)
{
	old_dat_entry_t first_entry;

	if (pkg->pio->readvec(pkg, &first_entry, sizeof(first_entry), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD index_entries = first_entry.offset / sizeof(old_dat_entry_t) - 1;
	DWORD index_len = index_entries * sizeof(old_dat_entry_t);
	old_dat_entry_t *index_buffer = new old_dat_entry_t[index_entries];
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, index_buffer, index_len, 0, IO_SEEK_SET)) {
		delete [] index_buffer;
		return -CUI_EREADVEC;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_len;
	pkg_dir->index_entry_length = sizeof(old_dat_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int NekoSDK_old_dat_parse_resource_info(struct package *pkg,
											   struct package_resource *pkg_res)
{
	old_dat_entry_t *old_dat_entry;

	old_dat_entry = (old_dat_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, old_dat_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = old_dat_entry->comprlen;
	pkg_res->actual_data_length = old_dat_entry->uncomprlen;
	pkg_res->offset = old_dat_entry->offset;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation NekoSDK_old_dat_operation = {
	NekoSDK_old_dat_match,				/* match */
	NekoSDK_old_dat_extract_directory,	/* extract_directory */
	NekoSDK_old_dat_parse_resource_info,/* parse_resource_info */
	NekoSDK_dat_extract_resource,		/* extract_resource */
	NekoSDK_pak_save_resource,			/* save_resource */
	NekoSDK_pak_release_resource,		/* release_resource */
	NekoSDK_pak_release					/* release */
};


/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK NekoSDK_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".pak"), NULL, 
		NULL, &NekoSDK_pak_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".pak"), NULL, 
		NULL, &NekoSDK_paks_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &NekoSDK_old_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &NekoSDK_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR 
			| CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	return 0;
}
