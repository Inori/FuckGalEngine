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
struct acui_information ARCG_cui_information = {
	NULL,						/* copyright */
	NULL,						/* system */
	_T(".arc .vpk .bmx .scb .wsm"),	/* package */
	_T("0.9.6"),				/* revision */
	_T("痴漢公賊"),				/* author */
	_T("2009-7-20 16:02"),		/* date */
	NULL,						/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];		// "ARCG"
	u32 version;		// 0x00010000
	u32 index_offset;
	u32 index_length;
	u16 dir_entries;	// 目录项个数(包含根目录)
	u32 file_entries;	// 文件项个数
	u8 resrved[10];		// 0
} ARCG_header_t;

#if 0
typedef struct {		// 用于描述和建立一个2级的目录和文件结构的通用数据结构
	u8 name_length;		// 本字段和名字长度(以NULL结尾并包含)的总长度
	s8 *name;			// 以NULL结尾	
	u32 offset;			// 对于根目录项(且包含子目录项),指向第一个文件项之前4个字节
						// (感觉应该是指向准备建立但没建立的最新一个空目录项)
						// 对于根目录项(且不包含子目录)或子目录项,指向第一个文件项
						// 对于文件项，指向其数据开始的位置
	u32 length;			// 对于根目录项(且包含子目录项),总为0
						// 对于根目录项(且不包含子目录)或子目录项,表示包含的文件数
						// 对于文件项,表示数据长度
} ARCG_entry_t;
#endif

typedef struct {
	s8 magic[4];		// "TX04"
	u16 pitch;
	u16 height;
} TX04_header_t;

typedef struct {
	s8 magic[4];			// "MBF0"
	u32 index_entries;
	u32 data_offset;		// 第一个文件项数据的位置
	u32 flag;
	u32 mbf_size;
	u32 reserved[3];		// 0
} mbf_header_t;

typedef struct {
	s8 magic[4];		// "WSM4"
	u32 data_offset;
	u32 header_size;
	u32 index_entries;
	u32 info_offset;
	u32 info_entries;
	u8 resrved[40];		// 0
} wsm_header_t;

typedef struct {		// 0x1a8字节
	u32 index_number;	// 从1开始计数
	s8 name[64];
	s8 description[128];
	u32 unknown0;		// 1
	u32 unknown1;		// 1
	u32 unknown2;		// 0x63
	u32 id;				// 从0开始计数
	u32 offset;
	u32 length;
	u32 unknown3;		// 3
	u8 unknown[200];	// 0
} wsm_name_entry_t;

typedef struct {		// 0x24字节
	u32 offset;
	u32 length;
	u32 zero;
	u32 pcm_size;		// 6164a0,69c840
	u16 FormatTag;
	u16 Channels;
	u32 SamplesPerSec;
	u32 AvgBytesPerSec;
	u16 BlockAlign;
	u16 wBitsPerSample;
	u32 reserved;		// 0
} wsm_info_entry_t;
#pragma pack ()

typedef struct {
	char name[MAX_PATH];
	DWORD offset;
	DWORD length;
} ARCG_entry_t;


static void *my_malloc(DWORD len)
{
	return new BYTE[len];
}

static void __bc_uncompress(BYTE *uncompr, DWORD uncomprlen, 
							BYTE *compr, DWORD comprlen, DWORD pitch)
{
	DWORD curbyte = 0;
	DWORD act_uncomprlen = 0;

	uncompr[act_uncomprlen++] = compr[curbyte++];
	uncompr[act_uncomprlen++] = compr[curbyte++];
	while ((act_uncomprlen + 1 < uncomprlen) && (curbyte + 1 < comprlen)) {
		BYTE flag = compr[curbyte++];
		if ((flag & 0xe0) == 0xe0) {
			DWORD cp = (flag & 0x1f) + 1;
			if (act_uncomprlen + cp > uncomprlen)
				cp = uncomprlen - act_uncomprlen;
			for (DWORD i = 0; i < cp; ++i)
				uncompr[act_uncomprlen++] = compr[curbyte++];
		} else if ((flag & 0xe0) == 0xc0) {
#if 1
			// plus flag? should be or (flag & 0x1f) ???
			//DWORD offset = ((compr[curbyte++] << 5) + flag) & 0x1f;
			DWORD offset = (compr[curbyte++] << 5) | (flag & 0x1f);
			DWORD cp = compr[curbyte++];
#else
			// you bastard!
			DWORD offset = flag & 0x1f;
			DWORD cp = compr[(++curbyte)++];
#endif
			BYTE *src = uncompr + act_uncomprlen - 1 - offset;
			if (act_uncomprlen + cp > uncomprlen)
				cp = uncomprlen - act_uncomprlen;
			for (DWORD i = 0; i < cp; ++i)
				uncompr[act_uncomprlen++] = *src++;
		} else if ((flag & 0xc0) == 0x00) {
			DWORD offset = (flag >> 3) & 7;
			DWORD cp = flag & 7;
			if (cp != 7)
				cp += 2;
			else
				cp = compr[curbyte++];
			BYTE *src = uncompr + act_uncomprlen - 1 - offset;
			if (act_uncomprlen + cp > uncomprlen)
				cp = uncomprlen - act_uncomprlen;
			for (DWORD i = 0; i < cp; ++i)
				uncompr[act_uncomprlen++] = *src++;
		} else if ((flag & 0xc0) == 0x40) {
			DWORD offset = (flag >> 2) & 0xf;
			DWORD cp = flag & 3;
			if (cp != 3)
				cp += 2;
			else
				cp = compr[curbyte++];
			BYTE *src = uncompr + act_uncomprlen - (pitch + 8) + offset;
			if (act_uncomprlen + cp > uncomprlen)
				cp = uncomprlen - act_uncomprlen;
			for (DWORD i = 0; i < cp; ++i)
				uncompr[act_uncomprlen++] = *src++;
		} else {
			DWORD offset = (flag >> 2) & 0xf;
			DWORD cp = flag & 3;
			if (cp != 3)
				cp += 2;
			else
				cp = compr[curbyte++];
			BYTE *src = uncompr + act_uncomprlen - (pitch * 2 + 8) + offset;
			if (act_uncomprlen + cp > uncomprlen)
				cp = uncomprlen - act_uncomprlen;
			for (DWORD i = 0; i < cp; ++i)
				uncompr[act_uncomprlen++] = *src++;	
		}
	}
}

static int bc_uncompress(void *data, DWORD data_len, BYTE **ret_uncompr, DWORD *ret_uncomprlen)
{
	BITMAPFILEHEADER *bmfh = (BITMAPFILEHEADER *)data;
	BITMAPINFOHEADER *bmiHeader = (BITMAPINFOHEADER *)(bmfh + 1);
	char *tx = (char *)data + bmfh->bfOffBits;
	DWORD palette_size = bmfh->bfOffBits - sizeof(BITMAPFILEHEADER) - sizeof(BITMAPINFOHEADER);
	BYTE *palette = palette_size ? (BYTE *)(bmiHeader + 1) : NULL;
	int ret = -CUI_EMATCH;

	if (!strncmp(tx, "TX04", 4)) {
		TX04_header_t *tx04 = (TX04_header_t *)tx;

		DWORD uncomprlen = tx04->pitch * tx04->height;
		BYTE *uncompr = new BYTE[uncomprlen];
		if (!uncompr)
			return -CUI_EMEM;

		__bc_uncompress(uncompr, uncomprlen, (BYTE *)(tx04 + 1), 
			data_len - ((BYTE *)(tx04 + 1) - data), tx04->pitch);

		// defiltering
		if (bmiHeader->biBitCount > 8) {
			DWORD bpp = bmiHeader->biBitCount / 8;
			BYTE *curline = uncompr;
			for (int y = 0; y < bmiHeader->biHeight; ++y) {
				BYTE *cur = curline + bpp;
				BYTE *pre = cur - bpp;
				for (int x = 1; x < bmiHeader->biWidth; ++x) {				
					for (DWORD p = 0; p < bpp; ++p)
						*cur++ += *pre++;
				}
				curline += tx04->pitch;
			}
		}

		if (MyBuildBMPFile(uncompr, uncomprlen, palette, palette_size, bmiHeader->biWidth,
				bmiHeader->biHeight, bmiHeader->biBitCount, ret_uncompr, 
				ret_uncomprlen, my_malloc)) {
			delete [] uncompr;
			return -CUI_EMEM;
		}
		delete [] uncompr;
		ret = 0;
	}

	return ret;
}

/********************* *********************/

/* 封包匹配回调函数 */
static int ARCG_match(struct package *pkg)
{
	ARCG_header_t ARCG_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &ARCG_header, sizeof(ARCG_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(ARCG_header.magic, "ARCG", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (ARCG_header.version != 0x00010000) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int ARCG_extract_directory(struct package *pkg,
								  struct package_directory *pkg_dir)
{
	ARCG_header_t ARCG_header;
	if (pkg->pio->readvec(pkg, &ARCG_header, sizeof(ARCG_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	BYTE *index = new BYTE[ARCG_header.index_length];
	if (!index)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, index, ARCG_header.index_length, ARCG_header.index_offset, IO_SEEK_SET)) {
		delete [] index;
		return -CUI_EREADVEC;
	}

	ARCG_entry_t *ARCG_index = new ARCG_entry_t[ARCG_header.file_entries];
	if (!ARCG_index) {
		delete [] index;
		return -CUI_EMEM;
	}

	// 因为所有封包都是同名
	char base_dir[MAX_PATH];
	unicode2acp(base_dir, MAX_PATH, pkg->extension + 1, -1);
	strcat(base_dir, "/");

	DWORD i = 0;
	BYTE *p = index;
	for (DWORD d = 0; d < ARCG_header.dir_entries; ++d) {
		char *dir_name = (char *)(p + 1);
		p += *p;
		u32 file_index_offset = *(u32 *)p;
		p += 4;
		u32 files = *(u32 *)p;
		p += 4;
		BYTE *fp = index + file_index_offset - ARCG_header.index_offset;
		for (DWORD f = 0; f < files; ++f) {
			strcpy(ARCG_index[i].name, base_dir);
			strcat(ARCG_index[i].name, dir_name);
			strcat(ARCG_index[i].name, "/");
			strcat(ARCG_index[i].name, (char *)(fp + 1));
			fp += *fp;
			ARCG_index[i].offset = *(u32 *)fp;
			fp += 4;
			ARCG_index[i].length = *(u32 *)fp;
			fp += 4;
			++i;
		}
	}
	delete [] index;
	if (d != ARCG_header.dir_entries || i != ARCG_header.file_entries) {
		delete [] ARCG_index;
		return -CUI_EMATCH;
	}

	pkg_dir->index_entries = ARCG_header.file_entries;
	pkg_dir->directory = ARCG_index;
	pkg_dir->directory_length = ARCG_header.file_entries * sizeof(ARCG_entry_t);
	pkg_dir->index_entry_length = sizeof(ARCG_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int ARCG_parse_resource_info(struct package *pkg,
									struct package_resource *pkg_res)
{
	ARCG_entry_t *ARCG_entry;
	int name_len;

	ARCG_entry = (ARCG_entry_t *)pkg_res->actual_index_entry;
	name_len = strlen(ARCG_entry->name);
	strncpy(pkg_res->name, ARCG_entry->name, name_len);
	// sight_select\P_桑原孝広?
	if (pkg_res->name[name_len - 1] == '?')
		pkg_res->name[name_len - 1] = '_';
	pkg_res->name_length = name_len;
	pkg_res->raw_data_length = ARCG_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = ARCG_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int ARCG_extract_resource(struct package *pkg,
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

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = raw;
		return 0;
	}

	if (!strncmp((char *)raw, "RIFF", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".wav");
	} else if (!strncmp((char *)raw, "OggS", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".ogg");
	} else if (!strncmp((char *)raw, "\xff\xd8\xff\xe0\x00\x10JFIF", 10)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".jpg");
	} else if (!strncmp((char *)raw, "BC", 2)) {
		int ret = bc_uncompress(raw, pkg_res->raw_data_length, 
			(BYTE **)&pkg_res->actual_data,	&pkg_res->actual_data_length);
		if (ret) {
			delete [] raw;
			return ret;			
		}
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".bmp");
	} else if (!strncmp((char *)raw, "BM", 2)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".bmp");
	}

	pkg_res->raw_data = raw;

	return 0;
}

/* 资源保存函数 */
static int ARCG_save_resource(struct resource *res, 
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
static void ARCG_release_resource(struct package *pkg, 
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
static void ARCG_release(struct package *pkg, 
						 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation ARCG_operation = {
	ARCG_match,					/* match */
	ARCG_extract_directory,		/* extract_directory */
	ARCG_parse_resource_info,	/* parse_resource_info */
	ARCG_extract_resource,		/* extract_resource */
	ARCG_save_resource,			/* save_resource */
	ARCG_release_resource,		/* release_resource */
	ARCG_release				/* release */
};

/********************* mbf *********************/

/* 封包匹配回调函数 */
static int ARCG_mbf_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "MBF0", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int ARCG_mbf_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	mbf_header_t mbf_header;
	if (pkg->pio->readvec(pkg, &mbf_header, sizeof(mbf_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD index_len = mbf_header.data_offset - sizeof(mbf_header);
	BYTE *index = new BYTE[index_len];
	if (!index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index, index_len)) {
		delete [] index;
		return -CUI_EREAD;
	}

	BYTE *p = index;

	// base项,意义不明
	if (mbf_header.flag & 1)
		p += *p;

	ARCG_entry_t *mbf_index = new ARCG_entry_t[mbf_header.index_entries];
	if (!mbf_index) {
		delete [] index;
		return -CUI_EMEM;
	}

	DWORD offset = mbf_header.data_offset;
	int ret = 0;
	for (DWORD i = 0; i < mbf_header.index_entries; ++i) {
		strcpy(mbf_index[i].name, (char *)p + 2);
		p += *p;

		s8 magic[2];
		if (pkg->pio->readvec(pkg, magic, sizeof(magic), 
			offset, IO_SEEK_SET)) {
			ret = -CUI_EREADVEC;
			break;
		}

		if (!strncmp(magic, "BC", 2)) {
			BITMAPFILEHEADER bmfh;
			if (pkg->pio->readvec(pkg, &bmfh, sizeof(bmfh), offset, IO_SEEK_SET)) {
				ret = -CUI_EREADVEC;
				break;
			}	
			mbf_index[i].offset = offset;
			mbf_index[i].length = bmfh.bfSize;
			offset += bmfh.bfSize;
		} else {
			ret = -CUI_EMATCH;
			break;
		}
	}
	delete [] index;
	if (i != mbf_header.index_entries) {
		delete [] mbf_index;
		return ret;
	}

	pkg_dir->index_entries = mbf_header.index_entries;
	pkg_dir->directory = mbf_index;
	pkg_dir->directory_length = mbf_header.index_entries * sizeof(ARCG_entry_t);
	pkg_dir->index_entry_length = sizeof(ARCG_entry_t);

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation ARCG_mbf_operation = {
	ARCG_mbf_match,				/* match */
	ARCG_mbf_extract_directory,	/* extract_directory */
	ARCG_parse_resource_info,	/* parse_resource_info */
	ARCG_extract_resource,		/* extract_resource */
	ARCG_save_resource,			/* save_resource */
	ARCG_release_resource,		/* release_resource */
	ARCG_release				/* release */
};

/********************* wsm *********************/

static int ARCG_wsm_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "WSM", 3)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int ARCG_wsm_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	wsm_header_t wsm_header;
	if (pkg->pio->readvec(pkg, &wsm_header, sizeof(wsm_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	wsm_name_entry_t *name_index = new wsm_name_entry_t[wsm_header.index_entries];
	if (!name_index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, name_index,
			wsm_header.index_entries * sizeof(wsm_name_entry_t))) {
		delete [] name_index;
		return -CUI_EREAD;
	}

	wsm_info_entry_t *info_index = new wsm_info_entry_t[wsm_header.info_entries];
	if (!info_index) {
		delete [] name_index;
		return -CUI_EMEM;
	}

	if (pkg->pio->read(pkg, info_index, 
			wsm_header.info_entries * sizeof(wsm_info_entry_t))) {
		delete [] info_index;
		delete [] name_index;
		return -CUI_EREAD;
	}

	ARCG_entry_t *ARCG_index = new ARCG_entry_t[wsm_header.index_entries];
	if (!ARCG_index) {
		delete [] info_index;
		delete [] name_index;
		return -CUI_EMEM;
	}

	for (DWORD i = 0; i < wsm_header.index_entries; ++i) {
		strcpy(ARCG_index[i].name, name_index[i].name);
		ARCG_index[i].offset = info_index[i].offset;
		ARCG_index[i].length = info_index[i].length;
	}
	delete [] info_index;
	delete [] name_index;

	pkg_dir->index_entries = wsm_header.index_entries;
	pkg_dir->directory = ARCG_index;
	pkg_dir->directory_length = wsm_header.index_entries * sizeof(ARCG_entry_t);
	pkg_dir->index_entry_length = sizeof(ARCG_entry_t);

	return 0;
}

static cui_ext_operation ARCG_wsm_operation = {
	ARCG_wsm_match,				/* match */
	ARCG_wsm_extract_directory,	/* extract_directory */
	ARCG_parse_resource_info,	/* parse_resource_info */
	ARCG_extract_resource,		/* extract_resource */
	ARCG_save_resource,			/* save_resource */
	ARCG_release_resource,		/* release_resource */
	ARCG_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK ARCG_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".arc"), NULL, 
		NULL, &ARCG_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".scb"), NULL, 
		NULL, &ARCG_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".bmx"), NULL, 
		NULL, &ARCG_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".vpk"), NULL, 
		NULL, &ARCG_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".wsm"), NULL, 
		NULL, &ARCG_wsm_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".mbf"), NULL, 
		NULL, &ARCG_mbf_operation, CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
