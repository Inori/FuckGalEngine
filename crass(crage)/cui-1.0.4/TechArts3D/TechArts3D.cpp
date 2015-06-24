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
struct acui_information TechArts3D_cui_information = {
	_T("Tech Arts 3D"),		/* copyright */
	NULL,					/* system */
	_T(".tah"),				/* package */
	_T("1.0.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-4-16 17:38"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];		// "TAH2"
	u32 index_entries;
	u32 unknown;		// 1
	u32 reserved;		// 0
} tah_header_t;

typedef struct {
	u32 name_hash;
	u32 offset;
} tah_entry_t;
#pragma pack ()

typedef struct {
	char name[MAX_PATH];
	u32 offset;
	u32 length;
} my_tah_entry_t;

extern void init_by_array(unsigned long init_key[], int key_length);
extern unsigned long genrand_int32(void);

static void mt19937ar_lzss_uncompress(BYTE *uncompr, DWORD uncomprlen, 
									   BYTE *compr, DWORD comprlen)
{
	unsigned int act_uncomprlen = 0;
	unsigned int curbit = 0;
	unsigned int curbyte = 0;
	unsigned int nCurWindowByte = 0xff0;
	const unsigned int win_size = 4096;
	BYTE win[4096];
	WORD flag = 0;
	unsigned long init_key[4];
	unsigned char rnd[1024];

	init_key[0] = (uncomprlen | 0x80) >> 5;
	init_key[1] = (uncomprlen << 9) | 6;
	init_key[2] = (uncomprlen << 6) | 4;
	init_key[3] = (uncomprlen | 0x48) >> 3;
	init_by_array(init_key, 4);
	for (DWORD i = 0; i < 1024; i++)
		rnd[i] = (unsigned char)(genrand_int32() >> (i % 7));

	u32 seed = (((uncomprlen / 1000) % 10) + ((uncomprlen / 100) % 10) + ((uncomprlen / 10) % 10) + (uncomprlen % 10)) & 0x31A;

	memset(win, 0, win_size);
	while (act_uncomprlen < uncomprlen) {
		flag >>= 1;
		if (!(flag & 0x0100)) {
			seed = (seed + 1) & 0x3ff;
			flag = (compr[curbyte++] ^ rnd[seed]) | 0xff00;
		}

		seed = (seed + 1) & 0x3ff;

		if (flag & 1) {
			unsigned char data;

			data = compr[curbyte++] ^ rnd[seed];
			uncompr[act_uncomprlen++] = data;
			win[nCurWindowByte++] = data;
			nCurWindowByte &= win_size - 1;
		} else {
			unsigned int copy_bytes, win_offset;
			unsigned int i;

			win_offset = compr[curbyte++] ^ rnd[seed];
			seed = (seed + 1) & 0x3ff;
			copy_bytes = compr[curbyte++] ^ rnd[seed];			
			win_offset |= (copy_bytes & 0xf0) << 4;
			copy_bytes &= 0x0f;
			copy_bytes += 3;

			for (i = 0; i < copy_bytes; i++) {	
				unsigned char data;

				data = win[(win_offset + i) & (win_size - 1)];				
				uncompr[act_uncomprlen++] = data;
				win[nCurWindowByte++] = data;
				nCurWindowByte &= win_size - 1;	
				if (act_uncomprlen >= uncomprlen)
					return;
			}
		}
	}
}

static u32 name_hash(char *name)
{
	u32 key = 0xC8A4E57A;

	for (unsigned int i = 0; i < strlen(name); i++) {
		key = key << 19 | key >> 13;
		key ^= name[i];
	}

	return key ^ (-!!(name[i - 1] & 0x1A));
}

/********************* tah *********************/

/* 封包匹配回调函数 */
static int TechArts3D_tah_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "TAH2", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int TechArts3D_tah_extract_directory(struct package *pkg,
											struct package_directory *pkg_dir)
{
	tah_header_t tah_header;
	u32 arc_size;

	if (pkg->pio->length_of(pkg, &arc_size))
		return -CUI_ELEN;

	if (pkg->pio->readvec(pkg, &tah_header, sizeof(tah_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD index_buffer_length = tah_header.index_entries * sizeof(tah_entry_t);
	tah_entry_t *index_buffer = new tah_entry_t[tah_header.index_entries];
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		delete [] index_buffer;
		return -CUI_EREAD;
	}

	/* 解压只含.tbn资源的名称表 */
	DWORD uncomprlen;
	if (pkg->pio->read(pkg, &uncomprlen, 4)) {
		delete [] index_buffer;
		return -CUI_EREAD;
	}

	DWORD comprlen = index_buffer[0].offset - sizeof(tah_header) - index_buffer_length;
	BYTE *compr = new BYTE[comprlen];
	if (!compr) {
		delete [] index_buffer;
		return -CUI_EMEM;
	}

	if (pkg->pio->read(pkg, compr, comprlen)) {
		delete [] compr;
		delete [] index_buffer;
		return -CUI_EREAD;
	}

	BYTE *uncompr = new BYTE[uncomprlen]();
	if (!uncompr) {
		delete [] compr;
		delete [] index_buffer;
		return -CUI_EMEM;
	}

	mt19937ar_lzss_uncompress(uncompr, uncomprlen, compr, comprlen);
	delete [] compr;

	DWORD my_index_buffer_length = tah_header.index_entries * sizeof(my_tah_entry_t);
	my_tah_entry_t *my_index_buffer = new my_tah_entry_t[tah_header.index_entries]();
	if (!my_index_buffer) {
		delete [] uncompr;
		delete [] index_buffer;
		return -CUI_EMEM;
	}

	char *name = (char *)uncompr;
	char dir_path[MAX_PATH];
	while (*name) {
		if (strstr(name, "/"))
			strcpy(dir_path, name);
		else {
			char name_path[MAX_PATH];

			strcpy(name_path, dir_path);
			strcat(name_path, name);
			CharLowerA(name_path);
			u32 hash = name_hash(name_path);
			for (DWORD i = 0; i < tah_header.index_entries; i++) {
				if (!my_index_buffer[i].name[0]) {
					if (hash == index_buffer[i].name_hash) {
						strcpy(my_index_buffer[i].name, name_path);
						break;
					}
				}
			}
			if (i == tah_header.index_entries)
				my_index_buffer[i].name[0] = 0;
		}
		name += strlen(name) + 1;
	}
	
	for (DWORD i = 0; i < tah_header.index_entries; i++) {
		if (!my_index_buffer[i].name[0])
			sprintf(my_index_buffer[i].name, "%08d", i);
		my_index_buffer[i].offset = index_buffer[i].offset;
	}
	delete [] uncompr;

	for (i = 0; i < tah_header.index_entries - 1; i++)
		my_index_buffer[i].length = index_buffer[i+1].offset - index_buffer[i].offset;
	my_index_buffer[i].length = arc_size - index_buffer[i].offset;

	pkg_dir->index_entries = tah_header.index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = my_index_buffer_length;
	pkg_dir->index_entry_length = sizeof(my_tah_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int TechArts3D_tah_parse_resource_info(struct package *pkg,
											  struct package_resource *pkg_res)
{
	my_tah_entry_t *my_tah_entry;

	my_tah_entry = (my_tah_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_tah_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_tah_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_tah_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int TechArts3D_tah_extract_resource(struct package *pkg,
										   struct package_resource *pkg_res)
{
	DWORD raw_len = pkg_res->raw_data_length;
	BYTE *raw = new BYTE[raw_len];
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, raw_len, pkg_res->offset, IO_SEEK_SET)) {
		delete [] raw;
		return -CUI_EREADVEC;
	}
	
	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = raw;
		return 0;
	}

	u32 uncomprlen = *(u32 *)raw;
	BYTE *compr = raw + 4;
	DWORD comprlen = raw_len - 4;
	BYTE *uncompr = new BYTE[uncomprlen]();
	if (!uncompr) {
		delete [] raw;
		return -CUI_EMEM;
	}

	mt19937ar_lzss_uncompress(uncompr, uncomprlen, compr, comprlen);

	if (!strncmp((char *)uncompr, "8BPS", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".psd");
	} else if (!strncmp((char *)uncompr, "TMO1", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".tmo");
	} else if (!strncmp((char *)uncompr, "OggS", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".ogg");
	} else if (!strncmp((char *)uncompr, "TSO1", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".tso");
	} else if (!strncmp((char *)uncompr, "BBBB", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".tbn");
	} else {	// data/shader/toonshader.cgfx
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".cgfx");
	}

	pkg_res->raw_data = raw;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

/* 资源保存函数 */
static int TechArts3D_tah_save_resource(struct resource *res, 
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
static void TechArts3D_tah_release_resource(struct package *pkg, 
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
static void TechArts3D_tah_release(struct package *pkg, 
								   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation TechArts3D_tah_operation = {
	TechArts3D_tah_match,				/* match */
	TechArts3D_tah_extract_directory,	/* extract_directory */
	TechArts3D_tah_parse_resource_info,	/* parse_resource_info */
	TechArts3D_tah_extract_resource,	/* extract_resource */
	TechArts3D_tah_save_resource,		/* save_resource */
	TechArts3D_tah_release_resource,	/* release_resource */
	TechArts3D_tah_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK TechArts3D_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".tah"), NULL, 
		NULL, &TechArts3D_tah_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
