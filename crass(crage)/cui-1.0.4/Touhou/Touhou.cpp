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

extern void init_genrand(unsigned long s);
extern unsigned long genrand_int32(void);

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information Touhou_cui_information = {
	_T("黄昏フロンティア"),	/* copyright */
	NULL,					/* system */
	_T(".dat"),				/* package */
	_T("1.0.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-5-25 21:02"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	u16 index_entries;
	u32 index_length;
} dat_header_t;

typedef struct {
	u8 bpp;
	u32 width; 
    u32 height; 
    u32 pitch;		// 对齐了的宽度
    u32 unknown_dib_length; 
} cv2_header_t;

typedef struct {
	u16 FormatTag; 
    u16 Channels; 
    u32 SamplesPerSec; 
    u32 AvgBytesPerSec; 
    u16 BlockAlign; 
    u16 BitsPerSample; 
    u16 cbSize; 
	u32 data_length;
} cv3_header_t;
#pragma pack ()

typedef struct {
	char name[MAX_PATH];
	int name_length;
	u32 offset;
	u32 length;
} dat_entry_t;

static void *my_malloc(DWORD len)
{
	return new BYTE[len];
}

/********************* dat *********************/

/* 封包匹配回调函数 */
static int Touhou105_dat_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	dat_header_t dat_header;
	if (pkg->pio->read(pkg, &dat_header, sizeof(dat_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	u32 dat_size;
	if (pkg->pio->length_of(pkg, &dat_size)) {
		pkg->pio->close(pkg);
		return -CUI_ELEN;
	}

	if (!dat_header.index_entries || dat_header.index_length >= dat_size) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->pio->seek(pkg, 0, IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_ESEEK;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int Touhou105_dat_extract_directory(struct package *pkg,
										  struct package_directory *pkg_dir)
{
	dat_header_t dat_header;
	if (pkg->pio->read(pkg, &dat_header, sizeof(dat_header)))
		return -CUI_EREAD;

	BYTE *index = new BYTE[dat_header.index_length];
	if (!index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index, dat_header.index_length))
		return -CUI_EREAD;

	init_genrand(dat_header.index_length + 6);
	for (DWORD i = 0; i < dat_header.index_length; ++i)
		index[i] ^= genrand_int32();

	DWORD index_buffer_length = sizeof(dat_entry_t) * dat_header.index_entries;
	dat_entry_t *index_buffer = new dat_entry_t[dat_header.index_entries];
	if (!index_buffer) {
		delete [] index;
		return -CUI_EMEM;
	}

	BYTE key0 = 0xc5;
	BYTE key1 = 0x83;
	for (i = 0; i < dat_header.index_length; ++i) {
		index[i] ^= key0;
		key0 += key1;
		key1 += 0x53;
	}

	BYTE *p = index;
	for (i = 0; i < dat_header.index_entries; i++) {
		dat_entry_t &dat_entry = index_buffer[i];

		dat_entry.offset = *(u32 *)p;
		p += 4;
		dat_entry.length = *(u32 *)p;
		p += 4;
		dat_entry.name_length = *p++;
		strncpy(dat_entry.name, (char *)p, dat_entry.name_length);
		p += dat_entry.name_length;
	}
	delete [] index;

	pkg_dir->index_entries = dat_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(dat_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int Touhou105_dat_parse_resource_info(struct package *pkg,
									struct package_resource *pkg_res)
{
	dat_entry_t *dat_entry;

	dat_entry = (dat_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, dat_entry->name);
	pkg_res->name_length = dat_entry->name_length;
	pkg_res->raw_data_length = dat_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = dat_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int Touhou105_dat_extract_resource(struct package *pkg,
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

	u8 key = (u8)(pkg_res->offset >> 1) | 0x23;
	for (DWORD i = 0; i < pkg_res->raw_data_length; i++)
		raw[i] ^= key;
	
	if (strstr(pkg_res->name, ".cv0") || strstr(pkg_res->name, ".cv1")) {
		BYTE xor0 = 0x8b, xor1 = 0x71;
		for (DWORD i = 0; i < pkg_res->raw_data_length; i++) {
			raw[i] ^= xor0;
			xor0 += xor1;
			xor1 += 0x95;
		}
		pkg_res->raw_data = raw;
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".txt");
	} else if (strstr(pkg_res->name, ".cv2")) {
		cv2_header_t *cv2_header = (cv2_header_t *)raw;	
		BYTE *dib_data = (BYTE *)(cv2_header + 1);
		DWORD dib_data_len;
		if (cv2_header->unknown_dib_length)
			dib_data_len = cv2_header->unknown_dib_length;
		else if (cv2_header->bpp < 24)
			dib_data_len = cv2_header->pitch * cv2_header->height * cv2_header->bpp / 8;
		else {
			dib_data_len = cv2_header->pitch * cv2_header->height * 4;
			cv2_header->bpp = 32;
		}
	
		if (cv2_header->unknown_dib_length) {
			printf("%s: width %x pitch %x bpp %x palette_length %x\n", 
				pkg_res->name, cv2_header->width, cv2_header->pitch,
				cv2_header->bpp, cv2_header->unknown_dib_length);
		}
		if (MySaveAsBMP(dib_data, dib_data_len, 
				NULL, 0, cv2_header->pitch, 0 - cv2_header->height,
				cv2_header->bpp, 0, (BYTE **)&pkg_res->actual_data, 
				&pkg_res->actual_data_length, my_malloc)) {
			delete [] raw;
			return -CUI_EMEM;
		}
		delete [] raw;
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp");
	} else if (strstr(pkg_res->name, ".cv3")) {
		cv3_header_t *cv3_header = (cv3_header_t *)raw;
		BYTE *pcm_data = (BYTE *)(cv3_header + 1);
		if (MySaveAsWAVE(pcm_data, cv3_header->data_length, 
				cv3_header->FormatTag, cv3_header->Channels, 
				cv3_header->SamplesPerSec, cv3_header->BitsPerSample, 
				NULL, 0, (BYTE **)&pkg_res->actual_data, 
				&pkg_res->actual_data_length, my_malloc)) {
			delete [] raw;
			return -CUI_EMEM;
		}
		delete [] raw;
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".wav");
	} else
		pkg_res->raw_data = raw;

#if 0
	// [C72]東方緋想天 ～ Scarlet Weather Rhapsody 体験版
	// 的所有资源扩展名都是.cnv,而且图像资源比较多,所以放
	// 到最后一个比较分支里.
	if (strstr(pkg_res->name, ".cv1") || (strstr(pkg_res->name, "csv") && strstr(pkg_res->name, ".cnv"))) {
		BYTE xor0 = 0x66, xor1 = 0x65;
		for (DWORD i = 0; i < pkg_res->raw_data_length; i++) {
			raw[i] ^= xor0;
			xor0 += xor1;
			xor1 += 0x4f;
		}

printf("dfs\n");
		pkg_res->raw_data = raw;
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".txt");
	} else 
#endif

	return 0;
}

/* 资源保存函数 */
static int Touhou105_dat_save_resource(struct resource *res, 
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
static void Touhou105_dat_release_resource(struct package *pkg, 
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
static void Touhou105_dat_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation Touhou105_dat_operation = {
	Touhou105_dat_match,					/* match */
	Touhou105_dat_extract_directory,		/* extract_directory */
	Touhou105_dat_parse_resource_info,		/* parse_resource_info */
	Touhou105_dat_extract_resource,		/* extract_resource */
	Touhou105_dat_save_resource,			/* save_resource */
	Touhou105_dat_release_resource,		/* release_resource */
	Touhou105_dat_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK Touhou_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &Touhou105_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	return 0;
}
