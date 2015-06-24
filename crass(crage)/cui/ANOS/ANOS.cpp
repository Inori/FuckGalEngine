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
struct acui_information ANOS_cui_information = {
	_T("YOX-Project"),		/* copyright */
	_T("ANOS"),				/* system */
	_T(".dat"),				/* package */
	_T("1.0.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-7-26 18:02"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[8];	// "DMKHARC"
	u32 index_entries;
	u32 index_offset;
} dat_header_t;

typedef struct {
	s8 *name;
	u32 offset;
	u32 length;
} dat_entry_t;

typedef struct {
	s8 magic[4];		// "DIMG"
	u32 width;
	u32 height;
	u32 unknown0;
	u32 unknown1;
	u32 unknown2;
	u32 unknown3;
	u32 pictures;
} img_header_t;
#pragma pack ()

typedef struct {
	s8 name[256];
	u32 offset;
	u32 length;
	u32 width;
	u32 height;
} ANOS_entry_t;

static void *my_malloc(DWORD len)
{
	return new BYTE[len];
}

static BYTE decode_table[512];

static void decode_init(char *name)
{
	int key = 0;
	for (int i = 0; name[i]; ++i)
		key += 137 * name[i];	
	
	BYTE *dec = decode_table;
	for (i = 0; i < 64; ++i) {
		dec[0] = dec[256] = (BYTE)key;
		key = (key << 1) | (((key ^ (key >> 12)) >> 17) & 1);
		dec[1] = dec[257] = (BYTE)key;
		key = (key << 1) | (((key ^ (key >> 12)) >> 17) & 1);
		dec[2] = dec[258] = (BYTE)key;
		key = (key << 1) | (((key ^ (key >> 12)) >> 17) & 1);
		dec[3] = dec[259] = (BYTE)key;
		key = (key << 1) | (((key ^ (key >> 12)) >> 17) & 1);
		dec += 4;
	}
}

static void decode(BYTE *data, DWORD len, DWORD offset)
{
	BYTE *dec = &decode_table[offset];

	for (DWORD i = 0; i < len; ++i)
		data[i] ^= dec[i & 0xff];
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
	while (act_uncomprlen < uncomprlen) {
		flag >>= 1;
		if (!(flag & 0x0100)) {
			if (curbyte >= comprlen)
				break;
			flag = compr[curbyte++] | 0xff00;
		}

		if (flag & 1) {
			unsigned char data;

			if (curbyte >= comprlen)
				break;
			data = compr[curbyte++];
			uncompr[act_uncomprlen++] = data;
			/* 输出的1字节放入滑动窗口 */
			win[nCurWindowByte++] = data;
			nCurWindowByte &= win_size - 1;
		} else {
			unsigned int copy_bytes, win_offset;
			unsigned int i;

			if (curbyte >= comprlen)
				break;
			win_offset = compr[curbyte++];
			if (curbyte >= comprlen)
				break;
			copy_bytes = compr[curbyte++];
			win_offset |= (copy_bytes & 0xf0) << 4;
			copy_bytes &= 0x0f;
			copy_bytes += 3;

			for (i = 0; i < copy_bytes; i++) {	
				unsigned char data;

				if (act_uncomprlen >= uncomprlen)
					goto out;
				data = win[(win_offset + i) & (win_size - 1)];					
				uncompr[act_uncomprlen++] = data;
				/* 输出的1字节放入滑动窗口 */
				win[nCurWindowByte++] = data;
				nCurWindowByte &= win_size - 1;	
			}
		}
	}
out:
	return act_uncomprlen;
}

/********************* dat *********************/

/* 封包匹配回调函数 */
static int ANOS_dat_match(struct package *pkg)
{
	dat_header_t dat_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &dat_header, sizeof(dat_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	char pkg_name[MAX_PATH];
	unicode2sj(pkg_name, MAX_PATH, pkg->name, -1);
	CharUpperA(pkg_name);
	decode_init(pkg_name);
	decode((BYTE *)&dat_header, sizeof(dat_header), 0xf0);

	if (strncmp(dat_header.magic, "DMKHARC", 8)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	u32 dat_size;
	if (pkg->pio->length_of(pkg, &dat_size)) {
		pkg->pio->close(pkg);
		return -CUI_ELEN;
	}

	if (dat_size <= dat_header.index_offset) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int ANOS_dat_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	dat_header_t dat_header;

	if (pkg->pio->readvec(pkg, &dat_header, sizeof(dat_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	decode((BYTE *)&dat_header, sizeof(dat_header), 0xf0);

	u32 dat_size;
	if (pkg->pio->length_of(pkg, &dat_size)) 
		return -CUI_ELEN;

	DWORD index_len = dat_size - dat_header.index_offset;
	BYTE *index = new BYTE[index_len];
	if (!index)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, index, index_len, dat_header.index_offset, IO_SEEK_SET)) {
		delete [] index;
		return -CUI_EREADVEC;
	}
	decode(index, index_len, 0);

	pkg_dir->index_entries = dat_header.index_entries;
	pkg_dir->directory = index;
	pkg_dir->directory_length = index_len;
	pkg_dir->flags = PKG_DIR_FLAG_VARLEN;

	return 0;
}

/* 封包索引项解析函数 */
static int ANOS_dat_parse_resource_info(struct package *pkg,
									struct package_resource *pkg_res)
{
	BYTE *dat_entry = (BYTE *)pkg_res->actual_index_entry;
	DWORD nlen = strlen((char *)dat_entry) + 1;
	memcpy(pkg_res->name, dat_entry, nlen);
	dat_entry += nlen;
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->offset = *(u32 *)dat_entry;
	dat_entry += 4;
	pkg_res->raw_data_length = *(u32 *)dat_entry;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->actual_index_entry_length = nlen + 4 + 4;

	return 0;
}

/* 封包资源提取函数 */
static int ANOS_dat_extract_resource(struct package *pkg,
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

	decode(raw, pkg_res->raw_data_length, 0);

	int is_img;
	BYTE *compr;
	DWORD comprlen;
	img_header_t img_header;
	if (!memcmp(raw, "DIMG", 4)) {
		memcpy(&img_header, raw, sizeof(img_header_t));
		is_img = 1;
		compr = raw + sizeof(img_header_t);
		comprlen = pkg_res->raw_data_length - sizeof(img_header_t);
	} else {
		is_img = 0;
		compr = raw;
		comprlen = pkg_res->raw_data_length;
	}

	BYTE *actual_data;
	DWORD actual_data_length;
	if (!memcmp(compr, "COMP", 4)) {
		DWORD uncomprlen = *(u32 *)(compr + 4);
		BYTE *uncompr = new BYTE[uncomprlen];
		if (!uncompr) {
			delete [] raw;
			return -CUI_EMEM;
		}

		lzss_uncompress(uncompr, uncomprlen, compr + 8, comprlen - 8);
		delete [] raw;
		actual_data = uncompr;
		actual_data_length = uncomprlen;
	} else {
		actual_data = compr;
		actual_data_length = comprlen;
	}

	if (is_img) {
		if (img_header.pictures > 1) {
			pkg_res->raw_data = new BYTE[sizeof(img_header_t) + actual_data_length];
			if (!pkg_res->raw_data) {
				delete [] actual_data;
				return -CUI_EMEM;			
			}
			memcpy((BYTE *)pkg_res->raw_data, &img_header, sizeof(img_header_t));
			memcpy((BYTE *)pkg_res->raw_data + sizeof(img_header_t), actual_data, actual_data_length);
			delete [] actual_data;

			return 0;
		}

		if (MyBuildBMPFile(actual_data, actual_data_length, NULL, 0, 
				img_header.width, 0 - img_header.height, 32, (BYTE **)&pkg_res->actual_data,
				&pkg_res->actual_data_length, my_malloc)) {
			delete [] actual_data;
			return -CUI_EMEM;
		}
		delete [] actual_data;
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp");
	} else {
		pkg_res->actual_data = actual_data;
		pkg_res->actual_data_length = actual_data_length;
	}
	
	return 0;
}

/* 资源保存函数 */
static int ANOS_dat_save_resource(struct resource *res, 
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
static void ANOS_dat_release_resource(struct package *pkg, 
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
static void ANOS_dat_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation ANOS_dat_operation = {
	ANOS_dat_match,					/* match */
	ANOS_dat_extract_directory,		/* extract_directory */
	ANOS_dat_parse_resource_info,		/* parse_resource_info */
	ANOS_dat_extract_resource,		/* extract_resource */
	ANOS_dat_save_resource,			/* save_resource */
	ANOS_dat_release_resource,		/* release_resource */
	ANOS_dat_release					/* release */
};

/********************* IMG *********************/

/* 封包匹配回调函数 */
static int ANOS_IMG_match(struct package *pkg)
{
	img_header_t img_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &img_header, sizeof(img_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(img_header.magic, "DIMG", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int ANOS_IMG_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	img_header_t img_header;

	if (pkg->pio->readvec(pkg, &img_header, sizeof(img_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	ANOS_entry_t *index = new ANOS_entry_t[img_header.pictures];
	if (!index)
		return -CUI_EMEM;

	char pkg_name[MAX_PATH];
	unicode2sj(pkg_name, MAX_PATH, pkg->name, -1);
	*strstr(pkg_name, ".IMG") = 0;
	
	u32 length = img_header.width * img_header.height * 4;
	
	for (DWORD i = 0; i < img_header.pictures; ++i) {
		sprintf(index[i].name, "%s_%04d", pkg_name, i);
		index[i].length = length;
		index[i].offset = i * length;
		index[i].width = img_header.width;
		index[i].height = img_header.height;
	}

	pkg_dir->index_entries = img_header.pictures;
	pkg_dir->directory = index;
	pkg_dir->directory_length = sizeof(ANOS_entry_t) * img_header.pictures;
	pkg_dir->index_entry_length = sizeof(ANOS_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int ANOS_IMG_parse_resource_info(struct package *pkg,
									struct package_resource *pkg_res)
{
	ANOS_entry_t *entry = (ANOS_entry_t *)pkg_res->actual_index_entry;

	strcpy(pkg_res->name, entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->offset = entry->offset;
	pkg_res->raw_data_length = entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */

	return 0;
}

/* 封包资源提取函数 */
static int ANOS_IMG_extract_resource(struct package *pkg,
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

	ANOS_entry_t *entry = (ANOS_entry_t *)pkg_res->actual_index_entry;

	if (MyBuildBMPFile(raw, pkg_res->raw_data_length, NULL, 0, 
			entry->width, 0 - entry->height, 32, (BYTE **)&pkg_res->actual_data,
			&pkg_res->actual_data_length, my_malloc)) {
		delete [] raw;
		return -CUI_EMEM;
	}
	delete [] raw;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation ANOS_IMG_operation = {
	ANOS_IMG_match,					/* match */
	ANOS_IMG_extract_directory,		/* extract_directory */
	ANOS_IMG_parse_resource_info,	/* parse_resource_info */
	ANOS_IMG_extract_resource,		/* extract_resource */
	ANOS_dat_save_resource,			/* save_resource */
	ANOS_dat_release_resource,		/* release_resource */
	ANOS_dat_release				/* release */
};


/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK ANOS_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &ANOS_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".IMG"), _T(".BMP"), 
		NULL, &ANOS_IMG_operation, CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
