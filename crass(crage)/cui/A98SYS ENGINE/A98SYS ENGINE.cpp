#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>
#include <utility.h>

// TODO: pak("ACPACK32", "@\0", and etc) @ 4258b0

//K:\Program Files\JOKYOUSI
//U:\新建文件夹
//Q:\[old A98]

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information A98SYS_ENGINE_cui_information = {
	_T("GEAR/Active"),		/* copyright */
	_T("A98SYS ENGINE"),	/* system */
	_T(".PAK .edt .ed8 .sal"),				/* package */
	_T("0.8.0"),			/* revision */
	_T("2009-6-6 14:03"),	/* author */
	_T(""),					/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {		// 16字节
	s8 magic[8];		// "ACPACK32" or "ADPACK32"
	u16 minor_version;	// 0
	u16 major_version;	// 1
	u16 index_entries;	// 最后一项是空项
	u16 pad;			// 0
} PAK_header_t;

// 最后一项空项的名字字段全0，crc字段无效，offset等于PAK文件总长度
typedef struct {		// 32字节
	s8 name[26];		// NULL terminated
	u16 name_crc;		// 这个crc是封包时针对63字节的名字而计算出来的，大概是用于快速查找资源用的
	u32 offset;
} PAK_entry_t;

typedef struct {
	s8 name[12];
	u32 offset;
} old_PAK_entry_t;

typedef struct {		// 4字节
	u32 segments;		// 区段数
} HGO_header_t;

typedef struct {		// 36字节
	s8 name[28];		// 区段名称
	u32 type;			// 1, 2(选项) or 3
	u32 offset;
} HGO_entry_t;

typedef struct {			// 34字节
	s8 magic[10];			// ".TRUE峕屗" (.TRUE江戸)
	u32 unknown0;			// 0x0a
	u16 width;				// 0x0e
	u16 height;				// 0x10
	u32 reserved;			// 0x12
	u32 diff_length;		// 0x16
	u32 flag_length;		// 0x1a
	u32 pixel_data_length;	// 0x1e
} edt_header_t;

typedef struct {			// 26字节
	s8 magic[10];			// ".8Bit峕屗" (.8Bit江戸)
	u16 type;				// 0x0a
	u16 unknown1;			// 0x0c
	u16 width;				// 0x0e
	u16 height;				// 0x10
	u16 colors;				// 0x12
	u16 unknown;			// 0x14
	u32 compr_length;		// 0x16
} ed8_header_t;
#pragma pack ()

typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 length;
} my_PAK_entry_t;


static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static char name_table[256] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
		0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
		0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
		0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00, 
		0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 
		0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 
		0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
		0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
		0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
		0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
		0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 
		0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x00, 0x00, 0x00, 
};

static void name_convert(BYTE *name)
{
	unsigned int chr;

	for (chr = *name; chr; chr = *name) {
		// 是否是双字节值
		if (name_table[chr] & 1) {
			if (!*++name)
				return;
			name++;
		} else
			*name++ = toupper(chr);
	}
}

// CCITT CRC routines
static unsigned short CRC16(unsigned char *buf, int length)
{
	unsigned short crc = 0;
	int y, i;

	for (i = 0; i < length; i++) {
		crc ^= buf[i] << 8;

		for (y = 0; y < 8; y++) {
			if (crc & 0x8000)
				crc = crc << 1 ^ 0x1021;
			else
				crc <<= 1;
		}
	}

	return crc;
}

static u16 name_crc(char *_name)
{
	BYTE name[64];

	strncpy((char *)name, _name, 63);
	name[63] = 0;
	name_convert(name);
	return CRC16(name, strlen((char *)name));
}

/* 将10/16进制数数转换为用16进制表示的255进制数 */
static unsigned int decode1(unsigned int data)
{
    return (((data >> 24) & 0xff) * 255 * 255 * 255 + ((data >> 16) & 0xff) * 255 * 255
        + ((data >> 8) & 0xff) * 255 + (data & 0xff) - (255 * 255 * 255 + 255 * 255 + 255 + 1)) / 2;
}

/********************* PAK *********************/

/* 封包匹配回调函数 */
static int A98SYS_ENGINE_PAK_match(struct package *pkg)
{
	s8 magic[8];
	u16 minor_version;
	u16 major_version;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "ADPACK32", 8) && strncmp(magic, "ACPACK32", 8)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (pkg->pio->read(pkg, &minor_version, 2)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (pkg->pio->read(pkg, &major_version, 2)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (minor_version != 0 || major_version != 1) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int A98SYS_ENGINE_PAK_extract_directory(struct package *pkg,
											   struct package_directory *pkg_dir)
{
	u32 index_entries;
	PAK_entry_t *index_buffer;
	DWORD index_buffer_length;	

	if (pkg->pio->read(pkg, &index_entries, 4))
		return -CUI_EREAD;

	index_entries &= 0xffff;
	index_buffer_length = index_entries * sizeof(PAK_entry_t);
	index_buffer = (PAK_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	index_entries--;
	DWORD my_index_buffer_length = index_entries * sizeof(my_PAK_entry_t);
	my_PAK_entry_t *my_index_buffer = (my_PAK_entry_t *)malloc(my_index_buffer_length);
	if (!my_index_buffer) {
		free(index_buffer);
		return -CUI_EMEM;
	}

	my_PAK_entry_t *entry = my_index_buffer;
	int k = 0;
	for (DWORD i = 0; i < index_entries; i++) {		
		strcpy(entry->name, index_buffer[i].name);

		if (!index_buffer[i].name_crc) {
			int nlen = index_buffer[i].name[25] ? 26 : strlen(index_buffer[i].name);
			strncpy(entry->name, index_buffer[i].name, nlen);
			entry->name[nlen] = 0;
		} else if (name_crc(entry->name) != index_buffer[i].name_crc) {
			char *ext = PathFindExtensionA(entry->name);
			char *_ext;

			if (ext && *ext && (_ext = strstr(".HGO.ed8.edt.ogg.wav.ENV.bmp.exe.WAV.TAG.ED8.BAT.ITW.SUE.sal", ext))) {
				strncpy(ext, _ext, 4);
				ext[4] = 0;
				if (name_crc(entry->name) != index_buffer[i].name_crc)
					break;
			} else {
				ext = strstr(entry->name, ".");

				if (ext) {					
					const char *suffix_list[] = {
						".HGO", ".ed8", ".edt", 
						".ogg", ".wav", ".ENV", 
						".bmp",	".exe", ".WAV", 
						".TAG", ".ED8",	".BAT",
						".ITW", ".SUE", ".sal",
						NULL
					};

					for (int k = 0; suffix_list[k]; k++) {					
						strcpy(ext, suffix_list[k]);
						if (name_crc(entry->name) == index_buffer[i].name_crc)
							break;
						strcpy(entry->name, index_buffer[i].name);
					}
					/* 资源名称过长 */
					if (!suffix_list[k]) {
						strcpy(entry->name, index_buffer[i].name);
						printf("%s: name too long!!\n", index_buffer[i].name);
					}
				} else
					printf("%s: name too long!\n", entry->name);
			}
		}	
		entry->offset = index_buffer[i].offset;
		entry->length = index_buffer[i+1].offset - index_buffer[i].offset;		
		entry++;
	}
	free(index_buffer);
	if (i != index_entries) {
		free(my_index_buffer);
		return -CUI_EMATCH;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = my_index_buffer_length;
	pkg_dir->index_entry_length = sizeof(my_PAK_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int A98SYS_ENGINE_PAK_parse_resource_info(struct package *pkg,
												 struct package_resource *pkg_res)
{
	my_PAK_entry_t *my_PAK_entry;

	my_PAK_entry = (my_PAK_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_PAK_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_PAK_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_PAK_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int A98SYS_ENGINE_PAK_extract_resource(struct package *pkg,
											  struct package_resource *pkg_res)
{
	pkg_res->raw_data = malloc(pkg_res->raw_data_length);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EREADVEC;
	}

	return 0;
}

/* 资源保存函数 */
static int A98SYS_ENGINE_save_resource(struct resource *res, 
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
static void A98SYS_ENGINE_release_resource(struct package *pkg, 
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
static void A98SYS_ENGINE_release(struct package *pkg, 
								  struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation A98SYS_ENGINE_PAK_operation = {
	A98SYS_ENGINE_PAK_match,				/* match */
	A98SYS_ENGINE_PAK_extract_directory,	/* extract_directory */
	A98SYS_ENGINE_PAK_parse_resource_info,	/* parse_resource_info */
	A98SYS_ENGINE_PAK_extract_resource,		/* extract_resource */
	A98SYS_ENGINE_save_resource,			/* save_resource */
	A98SYS_ENGINE_release_resource,			/* release_resource */
	A98SYS_ENGINE_release					/* release */
};

/********************* old_PAK *********************/

/* 封包匹配回调函数 */
static int A98SYS_ENGINE_old_PAK_match(struct package *pkg)
{
	u16 index_entries;
	old_PAK_entry_t entry;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &index_entries, sizeof(index_entries))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (!index_entries) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->pio->read(pkg, &entry, sizeof(entry))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (entry.offset != sizeof(index_entries) + index_entries * sizeof(entry)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int A98SYS_ENGINE_old_PAK_extract_directory(struct package *pkg,
												   struct package_directory *pkg_dir)
{
	u16 index_entries;

	if (pkg->pio->readvec(pkg, &index_entries, 2, 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD index_buffer_length = index_entries * sizeof(old_PAK_entry_t);
	old_PAK_entry_t *index_buffer = (old_PAK_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	--index_entries;
	DWORD my_index_buffer_length = index_entries * sizeof(my_PAK_entry_t);
	my_PAK_entry_t *my_index_buffer = (my_PAK_entry_t *)malloc(my_index_buffer_length);
	if (!my_index_buffer) {
		free(index_buffer);
		return -CUI_EMEM;
	}
	memset(my_index_buffer, 0, my_index_buffer_length);

	my_PAK_entry_t *entry = my_index_buffer;
	for (DWORD i = 0; i < index_entries; ++i) {
		for (int n = 7; n > 0; --n) {
			if (index_buffer[i].name[n] != ' ')
				break;
		}

		strncpy(entry->name, index_buffer[i].name, n + 1);
		strcat(entry->name, ".");
		strcat(entry->name, index_buffer[i].name + 8);
		entry->offset = index_buffer[i].offset;
		entry->length = index_buffer[i+1].offset - index_buffer[i].offset;		
		++entry;
	}
	free(index_buffer);
	if (i != index_entries) {
		free(my_index_buffer);
		return -CUI_EMATCH;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = my_index_buffer_length;
	pkg_dir->index_entry_length = sizeof(my_PAK_entry_t);
	//pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation A98SYS_ENGINE_old_PAK_operation = {
	A98SYS_ENGINE_old_PAK_match,			/* match */
	A98SYS_ENGINE_old_PAK_extract_directory,/* extract_directory */
	A98SYS_ENGINE_PAK_parse_resource_info,	/* parse_resource_info */
	A98SYS_ENGINE_PAK_extract_resource,		/* extract_resource */
	A98SYS_ENGINE_save_resource,			/* save_resource */
	A98SYS_ENGINE_release_resource,			/* release_resource */
	A98SYS_ENGINE_release					/* release */
};

/********************* edt *********************/

/* 封包匹配回调函数 */
static int A98SYS_ENGINE_edt_match(struct package *pkg)
{
	edt_header_t header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &header, sizeof(header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(header.magic, ".TRUE峕屗", 10)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

struct bits_struct {
	int bitmap;
	int mask;
	int *cur;
	int *end;
	int count;
};

static void get_bits_init(struct bits_struct *bits, BYTE *buf, int buf_len)
{
	bits->cur = (int *)buf;
	bits->count = buf_len / 4;
	bits->end = bits->cur + bits->count;	
	bits->mask = 1;
	bits->bitmap = *bits->cur++;	
}

static int get_bit(struct bits_struct *bits)
{
	int bitval;

	bitval = 0;
	if (bits->mask & bits->bitmap)
		bitval = 1;

	bits->mask <<= 1;
	if (!bits->mask) {
		bits->bitmap = *bits->cur++;
		bits->mask = 1;
	}

	return bitval;
}

static int get_bits(struct bits_struct *bits, int req)
{
	int bitval = 0;
	
	do {
		bitval <<= 1;
		bitval += get_bit(bits);
		--req;
	} while (req);

	return bitval;
}

static int get_pixel(struct bits_struct *bits)
{
	int bitval = get_bit(bits);

	if (bitval) {
		int len = get_bits(bits, 2) + 1;
		if (len & 1)
			bitval = (len + 1) / -2;
		else
			bitval = len / 2;
	}

	return bitval;
}

static const int edt_count_table[] = {
	0x0, 0x1, 0x3, 0x7,
	0x0f, 0x1f, 0x3f, 0x7f,
	0x0ff, 0x1ff, 0x3ff, 0x7ff,
	0x0fff, 0x1fff, 0x3fff, 0x7fff,
	0x0ffff, 0x1ffff, 0x3ffff, 0x7ffff,
	0x0fffff, 0x1fffff, 0x3fffff, 0xffffffff,
};

static int get_ffns(struct bits_struct *bits)
{
	int count = 0;

	while (get_bit(bits))
		++count;

	if (count)
		count = edt_count_table[count] + get_bits(bits, count);

	return count;
}

/* 封包资源提取函数 */
static int A98SYS_ENGINE_edt_extract_resource(struct package *pkg,
											  struct package_resource *pkg_res)
{
	u32 edt_size;
	pkg->pio->length_of(pkg, &edt_size);

	BYTE *edt = (BYTE *)malloc(edt_size);
	if (!edt)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, edt, edt_size, 0, IO_SEEK_SET)) {
		free(edt);
		return -CUI_EREADVEC;
	}

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = edt;
		return 0;
	}

	edt_header_t *header = (edt_header_t *)edt;
	BYTE *diff = edt + sizeof(edt_header_t);
	BYTE *flag = diff + header->diff_length;
	BYTE *compr = flag + header->flag_length;

	if ((header->diff_length == 0x4e) && !strcmp((char *)diff, ".EDT_DIFF")) {
		//diff += 10;	// point to mask value, 4字节，通常是0xff（蓝色）
		//接下来是基础图的文件名（无扩展名）
	}

	DWORD uncomprlen = 3 * header->width * header->height;
	BYTE *uncompr = (BYTE *)malloc(uncomprlen + 256);
	if (!uncompr) {
		free(edt);
		return -CUI_EMEM;
	}
	memset(uncompr, 0, uncomprlen);

	//  sub_4357D0
	struct bits_struct flag_bits;
	BYTE *p_compr = compr;

	int offset_table[32];
	for (int k = 0; k < 28; ) {
		int step = header->width * (k / 7 - 4);

		offset_table[k++] = 3 * step - 9;
		offset_table[k++] = 3 * step - 6;
		offset_table[k++] = 3 * step - 3;
		offset_table[k++] = 3 * step + 0;
		offset_table[k++] = 3 * step + 3;
		offset_table[k++] = 3 * step + 6;
		offset_table[k++] = 3 * step + 9;
	}
	offset_table[k++] = -12;
	offset_table[k++] = -9;
	offset_table[k++] = -6;
	offset_table[k] = -3;

	get_bits_init(&flag_bits, flag, header->flag_length);

	uncompr[0] = *p_compr++;
	uncompr[1] = *p_compr++;
	uncompr[2] = *p_compr++;

	for (DWORD act_uncomprlen = 3; act_uncomprlen < uncomprlen; ) {
		if (get_bit(&flag_bits)) {
			if (get_bit(&flag_bits)) {
				BYTE B, G, R;
				if (get_bit(&flag_bits)) {
					int off;

					switch (get_bits(&flag_bits, 2)) {
					case 0:
						off = offset_table[24];
						break;
					case 1:
						off = offset_table[23];
						break;
					case 2:
						off = offset_table[25];
						break;
					case 3:
						off = offset_table[17];
						break;
					}

					if (uncompr[act_uncomprlen + off + 0] <= 2)
						B = 2;
					else if (uncompr[act_uncomprlen + off + 0] > 253)
						B = 253;
					else
						B = uncompr[act_uncomprlen + off + 0];

					if (uncompr[act_uncomprlen + off + 1] <= 2)
						G = 2;
					else if (uncompr[act_uncomprlen + off + 1] > 253)
						G = 253;
					else
						G = uncompr[act_uncomprlen + off + 1];

					if (uncompr[act_uncomprlen + off + 2] <= 2)
						R = 2;
					else if (uncompr[act_uncomprlen + off + 2] > 253)
						R = 253;
					else
						R = uncompr[act_uncomprlen + off + 2];

					uncompr[act_uncomprlen++] = B + (BYTE)get_pixel(&flag_bits);
					uncompr[act_uncomprlen++] = G + (BYTE)get_pixel(&flag_bits);
					uncompr[act_uncomprlen++] = R + (BYTE)get_pixel(&flag_bits);
				} else {
					if (uncompr[act_uncomprlen - 3] <= 2)
						B = 2;
					else if (uncompr[act_uncomprlen - 3] > 253)
						B = 253;
					else
						B = uncompr[act_uncomprlen - 3];

					if (uncompr[act_uncomprlen - 2] <= 2)
						G = 2;
					else if (uncompr[act_uncomprlen - 2] > 253)
						G = 253;
					else
						G = uncompr[act_uncomprlen - 2];

					if (uncompr[act_uncomprlen - 1] <= 2)
						R = 2;
					else if (uncompr[act_uncomprlen - 1] > 253)
						R = 253;
					else
						R = uncompr[act_uncomprlen - 1];

					uncompr[act_uncomprlen++] = B + (BYTE)get_pixel(&flag_bits);
					uncompr[act_uncomprlen++] = G + (BYTE)get_pixel(&flag_bits);
					uncompr[act_uncomprlen++] = R + (BYTE)get_pixel(&flag_bits);
				}
			} else {
				unsigned int offset, count;

				offset = get_bits(&flag_bits, 5);
				count = (get_ffns(&flag_bits) + 1) * 3;

				BYTE *p = uncompr + act_uncomprlen;
				for (DWORD i = 0; i < count; ++i) {
					*p = p[offset_table[offset]];
					++p;
				}
				act_uncomprlen += count;
			}
		} else {
			uncompr[act_uncomprlen++] = *p_compr++;
			uncompr[act_uncomprlen++] = *p_compr++;
			uncompr[act_uncomprlen++] = *p_compr++;
		}
	}

	if (MyBuildBMPFile(uncompr, act_uncomprlen, NULL, 0, 
			header->width, -header->height, 24, (BYTE **)&pkg_res->actual_data,
			&pkg_res->actual_data_length, my_malloc)) {
		free(uncompr);
		free(edt);
		return -CUI_EMEM;
	}

	free(uncompr);
	free(edt);

	pkg_res->raw_data_length = edt_size;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation A98SYS_ENGINE_edt_operation = {
	A98SYS_ENGINE_edt_match,			/* match */
	NULL,								/* extract_directory */
	NULL,								/* parse_resource_info */
	A98SYS_ENGINE_edt_extract_resource,	/* extract_resource */
	A98SYS_ENGINE_save_resource,		/* save_resource */
	A98SYS_ENGINE_release_resource,		/* release_resource */
	A98SYS_ENGINE_release				/* release */
};

/********************* ed8 *********************/

/* 封包匹配回调函数 */
static int A98SYS_ENGINE_ed8_match(struct package *pkg)
{
	ed8_header_t header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &header, sizeof(header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(header.magic, ".8Bit峕屗", 10)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int A98SYS_ENGINE_ed8_extract_resource(struct package *pkg,
											  struct package_resource *pkg_res)
{
	u32 ed8_size;
	pkg->pio->length_of(pkg, &ed8_size);

	BYTE *ed8 = (BYTE *)malloc(ed8_size);
	if (!ed8)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, ed8, ed8_size, 0, IO_SEEK_SET)) {
		free(ed8);
		return -CUI_EREADVEC;
	}

	// ed8_process_42BBF0
	ed8_header_t *ed8_header = (ed8_header_t *)ed8;
	BYTE *palette = ed8 + sizeof(ed8_header_t);
	BYTE *unknown;
	if (ed8_header->unknown)
		unknown = palette + ed8_header->colors * 3;
	else
		unknown = NULL;

	BYTE *compr = palette + ed8_header->colors * 3 + ed8_header->unknown;

	if (ed8_header->unknown == 4) {
		unknown = palette + ed8_header->colors * 3; 
	} else {
		ed8_header->unknown1 = 0;
	}
	int offset_table[14];

	offset_table[0] = -1;
	offset_table[1] = -ed8_header->width;
	offset_table[2] = -2;
	offset_table[3] = -(ed8_header->width + 1);
	offset_table[4] = -(ed8_header->width - 1);
	offset_table[5] = -(ed8_header->width * 2);
	offset_table[6] = -(ed8_header->width + 2);
	offset_table[7] = -(ed8_header->width - 2);
	offset_table[8] = -(ed8_header->width + 1) * 2;
	offset_table[9] = -(ed8_header->width * 2 + 1);
	offset_table[10] = -(ed8_header->width * 2 - 1);
	offset_table[11] = -(ed8_header->width - 1) * 2;
	offset_table[12] = -(ed8_header->width * 3);
	offset_table[13] = -(ed8_header->width * 3 + 1);

	DWORD uncomprlen = ed8_header->width * ed8_header->height;
	BYTE *uncompr = (BYTE *)malloc(uncomprlen + 3 * ed8_header->height);
	if (!uncompr) {
		free(ed8);
		return -CUI_EMEM;
	}

	if (ed8_header->type == 256) {
		struct bits_struct flag_bits;
		DWORD act_uncomprlen;

		get_bits_init(&flag_bits, compr, ed8_header->compr_length);
		uncompr[0] = get_bits(&flag_bits, 8);
		act_uncomprlen = 1;

		while (act_uncomprlen < uncomprlen) {
			if (!get_bit(&flag_bits)) {
				int _offset = -1;

				do {
					int offset, count;

					if (get_bit(&flag_bits)) {
						if (get_bit(&flag_bits))
							offset = get_bits(&flag_bits, 3) + 6;
						else
							offset = get_bits(&flag_bits, 2) + 2;
					} else
						offset = get_bit(&flag_bits);

					if (offset == _offset)
						break;

					_offset = offset;

					count = get_ffns(&flag_bits) + 1;
					if (offset >= 2)
						++count;
					
					BYTE *p = uncompr + act_uncomprlen;
					for (int i = 0; i < count; ++i) {
						*p = p[offset_table[offset]];
						++p;
					}
					act_uncomprlen += count;					
				} while (act_uncomprlen < uncomprlen);
			}
			uncompr[act_uncomprlen++] = get_bits(&flag_bits, 8);
		}

		if (MyBuildBMPFile(uncompr, act_uncomprlen, palette, ed8_header->colors * 3, 
				ed8_header->width, -ed8_header->height, 8, (BYTE **)&pkg_res->actual_data,
				&pkg_res->actual_data_length, my_malloc)) {
			free(uncompr);
			free(ed8);
			return -CUI_EMEM;
		}
		free(uncompr);
		free(ed8);
	} else {
		printf("%s: unsupported ed8!\n", pkg_res->name);
		free(uncompr);
		free(ed8);
		return -CUI_EMATCH;
	}

	pkg_res->raw_data_length = ed8_size;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation A98SYS_ENGINE_ed8_operation = {
	A98SYS_ENGINE_ed8_match,			/* match */
	NULL,								/* extract_directory */
	NULL,								/* parse_resource_info */
	A98SYS_ENGINE_ed8_extract_resource,	/* extract_resource */
	A98SYS_ENGINE_save_resource,		/* save_resource */
	A98SYS_ENGINE_release_resource,		/* release_resource */
	A98SYS_ENGINE_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK A98SYS_ENGINE_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".PAK"), NULL, 
		NULL, &A98SYS_ENGINE_PAK_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".edt"), _T(".edt.bmp"), 
		NULL, &A98SYS_ENGINE_edt_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".ed8"), _T(".ed8.bmp"), 
		NULL, &A98SYS_ENGINE_ed8_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".sal"), _T(".sal.bmp"), 
		NULL, &A98SYS_ENGINE_ed8_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".PAK"), NULL, 
		NULL, &A98SYS_ENGINE_old_PAK_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;


	return 0;
}
