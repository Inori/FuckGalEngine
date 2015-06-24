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
struct acui_information CaramelBox_cui_information = {
	_T("キャラメルBOX"),	/* copyright */
	NULL,					/* system */
	_T(".bin .dat .ar3"),		/* package */
	_T("0.8.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-6-18 19:12"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];			/* "arc3" */
	u32 version;			/* 0, 1, 2 */
	u32 chunk_size;
	u32 data_offset;		/* 以块为单位 */
	u32 total_chunk_nr;		/* 整个文件占用的块数 */
	u32 unknown1;
	u32 directory_offset;	/* 以块为单位 */
	u32 directory_length;	/* 实际的字节数 */
	u32 directory_chunk_nr;	/* 目录占用的块数 */
	u32 unknown3;
	u32 unknown4;
	u16 unknown5;
} bin_header_t;

typedef struct {			// 每个资源前面的数据头
	u32 unknown0;
	u32 length0;
	u32 length1;
	u32 unknown1;
	u32 unknown2;
	u32 type;				/* 数据编码模式 */
	u32 pad0;
	u32 pad1;
} resource_header_t;

typedef struct {			// 每个资源前面的数据头
	u32 unknown;			// ??
	u32 comprlen;
	u32 uncomprlen;
	u32 type;				/* 数据编码模式 0, 1(not?), 2(zlib) */
} resource2_header_t;

typedef struct {
	s8 magic[2];			/* "lz" */
	u32 total_uncomprlen;	/* 总解压后的长度 */
} lz_header_t;

typedef struct {
	s8 sync[2];				/* "ze" */
	u16 uncomprlen;			/* 当前块解压后的长度 */
} lz_chunk_t;

typedef struct {
	s8 magic[4];			// "ARC4"
	u32 sync;				// must be 0x00010000
	u32 compr_index_length;	// ARC4_header+lz3_header+(lz3_block_header+lz3_block_data)*N
	u32 chunk_size;			// @ 0c
	u32 index_entries;		// @ 10
	u32 fat_offset;			// 解压后的fat数据的偏移
	u32 fat_size;			// 解压缩后fat表的长度
	u32 name_space_offset;	// 解压后的name_space数据的偏移
	u32 name_space_size;	// 解压缩后资源名表的长度
	u32 index_offset;		// 解压后的index数据的偏移
	u32 index_size;			// 解压缩后index的长度
	u32 unknown;			// @ 2c
} ARC4_header_t;

typedef struct {
	s8 magic[2];			/* "tZ" */
	u32 length;				/* 所有block解压后的总长度 */
} lz3_header_t;

typedef struct {
	s8 header[2];			/* "Zt" or "St" */
	u16 comprlen;			/* 压缩的数据包括fat+name_space+index */
	u16 uncomprlen;			/* 解压后的长度 */
	u16 crc;
} lz3_block_header_t;

typedef struct {
	u8 name_offset[3];
	u8 name_length;
	u8 number;
	u8 offset[3];
} lz3_fat_t;

typedef struct {
	s8 magic[4];	/* "fcb1" */
	u32 width;
	u32 height;
	s32 type;		// 0(lz3), 1(zlib)
} fcb_header_t;

typedef struct {
	s8 magic[8];	/* "_MAPp" */
} map_header_t;
#pragma pack ()

typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 length;
} my_bin_entry_t;

typedef struct {
	s8 name[MAX_PATH];
	u32 offset[255];
	u32 number;
} my_ARC4_entry_t;


static void *my_malloc(DWORD len)
{
	return malloc(len);
}

#define SWAP4(x)	(((x) & 0xff) << 24 | ((x) & 0xff00) << 8 | ((x) & 0xff0000) >> 8 | ((x) & 0xff000000) >> 24)
#define SWAP3(x)	(((x) & 0xff) << 16 | ((x) & 0xff00) | ((x) & 0xff0000) >> 16)
#define SWAP2(x)	((((x) & 0xff) << 8) | (((x) & 0xff00) >> 8))

#define ENCODE_MODE_NORMAL			0
#define ENCODE_MODE_NOT				2

static DWORD lz3_uncompress(BYTE *uncompr, DWORD uncomprlen, BYTE *compr, DWORD comprlen)
{
	BYTE *orig_uncompr = uncompr;
	BYTE flag;

	while ((flag = *compr++)) {
		DWORD count, offset;
		BYTE *src;

		if (flag & 0x80) {
			if (flag & 0x40) {
				BYTE tmp = *compr++;
				if (flag & 0x20) {
					offset = (flag << 16) | (tmp << 8) | *compr++;
					count = (offset & 0x3f) + 4;
					src = uncompr - (((offset >> 6) & 0x7fff) + 1);
				} else {
					offset = (flag << 8) | tmp;
					count = (offset & 7) + 3;
					src = uncompr - (((offset >> 3) & 0x3ff) + 1);
				}
			} else {
				count = (flag & 3) + 2;
				src = uncompr - (((flag >>2) & 0xf) + 1);
			}
		} else {			
			count = flag;
			src = compr;
			compr += count;
		}

		for (DWORD i = 0; i < count; i++)
			*uncompr++ = *src++;
	}

	return uncompr - orig_uncompr;
}

static int fcb1_uncompress(BYTE *out, BYTE *in, DWORD width, DWORD height, DWORD format)
{
	BYTE org_rgba[4] = { 128, 128, 128, 255 };

	for (DWORD y = 0; y < height; ++y) {
		BYTE rgba[4];

		rgba[0] = org_rgba[2];
		rgba[1] = org_rgba[1];
		rgba[2] = org_rgba[0];
		rgba[3] = org_rgba[3];

		for (DWORD x = 0; x < width; ++x) {
			BYTE flag = *in++;
			BYTE delta[4];

			if (flag & 0x80) {
				if (flag & 0x40) {
					if (flag & 0x20) {
						if (flag & 0x10) {
							if (flag & 0x08) {
								if (flag == 0xfe) {
									delta[0] = *in++ - 128;
									delta[1] = *in++ - 128;
									delta[2] = *in++ - 128;
									delta[3] = 0;
								} else if (flag == 0xff) {
									delta[0] = *in++ - 128;
									delta[1] = *in++ - 128;
									delta[2] = *in++ - 128;
									delta[3] = *in++ - 128;
								} else
									return -CUI_EUNCOMPR;
							} else {
								DWORD tmp = *in++ | (flag << 8);
								tmp = *in++ | (tmp << 8);
								tmp = *in++ | (tmp << 8);
								delta[0] = (BYTE)(((tmp >> 20) & 0x7F) - 64);
								delta[1] = (BYTE)(((tmp >> 14) & 0x3F) - 32);
								delta[2] = (BYTE)(((tmp >> 8) & 0x3F) - 32);
								delta[3] = (BYTE)(tmp - 128);
							}
						} else {
							DWORD tmp = *in++ | (flag << 8);
							tmp = *in++ | (tmp << 8);
							delta[0] = (BYTE)(((tmp >> 14) & 0x3F) - 32);
							delta[1] = (BYTE)(((tmp >> 10) & 0xF) - 8);
							delta[2] = (BYTE)(((tmp >> 6) & 0xF) - 8);
							delta[3] = (BYTE)((tmp & 0x3F) - 32);
						}
					} else {
						DWORD tmp = *in++ | (flag << 8);
						tmp = *in++ | (tmp << 8);
						delta[0] = (BYTE)(((tmp >> 13) & 0xFF) - 128);
						delta[1] = (BYTE)(((tmp >> 7) & 0x3F) - 32);
						delta[2] = (BYTE)((tmp & 0x7F) - 64);
						delta[3] = 0;
					}
				} else {
					DWORD tmp = *in++ | (flag << 8);
					delta[0] = (BYTE)(((tmp >> 8) & 0x3F) - 32);
					delta[1] = (BYTE)(((tmp >> 4) & 0xF) - 8);
					delta[2] = (BYTE)((tmp & 0xf) - 8);
					delta[3] = 0;
				}
			} else {
				delta[0] = ((flag >> 4) & 7) - 4;
				delta[1] = ((flag >> 2) & 3) - 2;
				delta[2] = (flag & 3) - 2;
				delta[3] = 0;
			}

			rgba[0] += delta[0] + delta[1];
			rgba[1] += delta[0];
			rgba[2] += delta[0] + delta[2];
			rgba[3] += delta[3];

			if (format == 1) {
				*out++ = rgba[0];
				*out++ = rgba[1];
				*out++ = rgba[2];
			} else if (format == 2) {
				*out++ = rgba[0];
				*out++ = rgba[1];
				*out++ = rgba[2];
				*out++ = rgba[3];
			} else if (format == 3) {
				*out++ = rgba[2];
				*out++ = rgba[1];
				*out++ = rgba[0];
				*out++ = rgba[3];
			}

			if (!x) {
				org_rgba[0] = rgba[2];
				org_rgba[1] = rgba[1];
				org_rgba[2] = rgba[0];
				org_rgba[3] = rgba[3];
			}
		}
	}

	return 0;
}

static int lz3_block_uncompress(struct package *pkg, BYTE **ret_data, DWORD *ret_data_len)
{
	lz3_block_header_t block_header;
	if (pkg->pio->read(pkg, &block_header, sizeof(block_header)))
		return -CUI_EREAD;

	if (strncmp(block_header.header, "Zt", 2) && strncmp(block_header.header, "St", 2))
		return -CUI_EMATCH;

	u16 *compr = (u16 *)malloc(block_header.comprlen);
	if (!compr)
		return -CUI_EMEM;
	
	if (pkg->pio->read(pkg, compr, block_header.comprlen)) {
		free(compr);
		return -CUI_EREAD;
	}

	u32 crc = block_header.crc;
	u32 c = 0;
	for (int i = 0; i < block_header.comprlen / 2; i++) {
		crc = 1336793 * crc + 4021;
		compr[i] -= (WORD)(crc >> 16);
		c = compr[i] + 3 * c;	
	}

	if ((c % 0xFFF1) != block_header.crc) {
		free(compr);
		return -CUI_EMATCH;
	}

	BYTE *uncompr = (BYTE *)malloc(block_header.uncomprlen);
	if (!uncompr) {
		free(compr);
		return -CUI_EMEM;
	}

	if (!strncmp(block_header.header, "Zt", 2)) {
		memset(uncompr, 0, block_header.uncomprlen);
		if (lz3_uncompress(uncompr, block_header.uncomprlen, (BYTE *)compr, block_header.comprlen) != block_header.uncomprlen) {
			free(uncompr);
			free(compr);
			return -CUI_EUNCOMPR;
		}
	} else
		memcpy(uncompr, compr, block_header.uncomprlen);

	free(compr);
	*ret_data = uncompr;
	*ret_data_len = block_header.uncomprlen;

	return 0;
}

struct lze_bits {
	int curbits;
	unsigned int value;
	BYTE *data;
	DWORD curbyte;
	DWORD data_length;
};

static void lze_bits_init(struct lze_bits *lze_bits, BYTE *data, DWORD data_length)
{
	lze_bits->curbits = 0;
	lze_bits->value = 0;
	lze_bits->data = data;
	lze_bits->curbyte = 0;
	lze_bits->data_length = data_length;
}

static int lze_bits_get(struct lze_bits *lze_bits, int req_bits, unsigned int *ret_ARC4a)
{
	int bits_is_not_enough;

	if (!req_bits)
		return 0;

	bits_is_not_enough = lze_bits->curbits < req_bits;
	lze_bits->value <<= req_bits;	
	lze_bits->curbits -= req_bits;
	if (bits_is_not_enough) {
		unsigned int value;

		if (lze_bits->curbyte < lze_bits->data_length) {
			value = SWAP2(*(u16 *)&lze_bits->data[lze_bits->curbyte]);
			lze_bits->curbyte += 2;
		} else
			return -1;
		lze_bits->value |= value << (-lze_bits->curbits);
		lze_bits->curbits += 16;
	}
	*ret_ARC4a = lze_bits->value >> 16;
	lze_bits->value &= 0xffff;

	return 0;
}

/* 首先获得数据长度（通过计算左起连续的bit0的个数+1）,然后返回指定长度的数据 */
static int lze_get_data(struct lze_bits *lze_bits, unsigned int *ret_data)
{
	DWORD total_length = 0;
	unsigned int data = lze_bits->value;
	DWORD curbits = lze_bits->curbits;
	struct lze_bits backup_lze_bits = *lze_bits;

	/* 获得数据长度 */
	while (1) {
		DWORD length = 0;
		
		if (!(data & 0xff00)) {
			length = 8;
			data <<= 8;
		}
		if (!(data & 0xf000)) {
			length += 4;
			data <<= 4;
		}
		if (!(data & 0xc000)) {
			length += 2;
			data <<= 2;
		}
		if (!(data & 0x8000)) {
			length++;
			data <<= 1;
			if (!(data & 0x8000)) {
				length++;
				data <<= 1;
			}
		}

		if (length > curbits)
			length = curbits;

		if (lze_bits_get(lze_bits, length, &data))
			return -1;

		total_length += length;
		curbits -= length;

		if (curbits)
			break;

		backup_lze_bits = *lze_bits;
		if (lze_bits_get(&backup_lze_bits, 16, &data))
			return -1;

		curbits = 16;
	}

	if (lze_bits_get(lze_bits, total_length + 1, &data))
		return -1;

	*ret_data = data;

	return 0;
}

static DWORD lze_uncompress(BYTE *uncompr, DWORD uncomprlen, BYTE *compr,  DWORD comprlen)
{
	struct lze_bits lze_bits;
	DWORD act_uncomprlen = 0;

	lze_bits_init(&lze_bits, compr, comprlen);
	while (1) {
		unsigned int copy_bytes;

		if (lze_get_data(&lze_bits, &copy_bytes))
			break;

		copy_bytes--;
		for (DWORD i = 0; i < copy_bytes; i++) {
			unsigned int data;

			if (lze_bits_get(&lze_bits, 8, &data))
				break;

			if (act_uncomprlen < uncomprlen)
				uncompr[act_uncomprlen++] = (BYTE)data;
		}
		
		if ((i != copy_bytes) || (act_uncomprlen >= uncomprlen))
			break;

		unsigned int offset;
		if (lze_get_data(&lze_bits, &offset))
			break;

		if (lze_get_data(&lze_bits, &copy_bytes))
			break;

		for (i = 0; i < copy_bytes; i++) {
			if (act_uncomprlen < uncomprlen) {
				uncompr[act_uncomprlen] = uncompr[act_uncomprlen - offset];
				act_uncomprlen++;
			}
		}

		if (act_uncomprlen >= uncomprlen)
			break;		
	}

	return lze_bits.curbyte;
}

/********************* bin *********************/

/* 封包匹配回调函数 */
static int CaramelBox_bin_match(struct package *pkg)
{
	s8 magic[4];
	u32 version;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "arc3", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (pkg->pio->read(pkg, &version, sizeof(version))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (SWAP4(version) > 2) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int CaramelBox_bin_extract_directory(struct package *pkg,
											struct package_directory *pkg_dir)
{
	bin_header_t bin_header;
	u32 bin_size;

	if (pkg->pio->length_of(pkg, &bin_size))
		return -CUI_ELEN;

	if (pkg->pio->readvec(pkg, &bin_header, sizeof(bin_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD directory_length = SWAP4(bin_header.directory_length);
	BYTE *directory = (BYTE *)malloc(directory_length);	
	if (!directory)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, directory, directory_length, 
			SWAP4(bin_header.directory_offset) * SWAP4(bin_header.chunk_size), IO_SEEK_SET)) {
		free(directory);
		return -CUI_EREADVEC;
	}

	DWORD index_entries = 0;
	BYTE *p_dir = directory;
	int is_first_entry = 0;
	while (p_dir < directory + directory_length) {
		int name_len;
		int name_offset;
		char entry_name[16];

		name_len = *p_dir & 0xf;
		name_offset = *p_dir >> 4;
		p_dir++;
		if (name_offset != 0xf) {	// 只有部分名字
			memcpy(&entry_name[name_offset], p_dir, name_len);			
			entry_name[name_offset + name_len] = 0;
			p_dir += name_len;
		} else if (name_len) {
			if (name_len == 0xf)	// 名称序号做累加
				entry_name[strlen(entry_name) - 1]++;
			else {	// 首项
				/* 首项拥有完整的名字 */
				memcpy(entry_name, p_dir, name_len);
				entry_name[name_len] = 0;				
				p_dir += name_len;
				is_first_entry = 1;
			}
		}

		p_dir += 3;	// 忽略数据偏移

		/* 首项还要忽略到下一个首相的directory偏移字段(该字段仅在查找时使用) */
		if (is_first_entry) {
			p_dir += 3;
			is_first_entry = 0;
		}
		index_entries++;
	}

	DWORD index_buffer_length = index_entries * sizeof(my_bin_entry_t);
	my_bin_entry_t *index_buffer = (my_bin_entry_t *)malloc(index_buffer_length);
	if (!index_buffer) {
		free(directory);
		return -CUI_EMEM;
	}

	bin_header.data_offset = SWAP4(bin_header.data_offset);
	bin_header.chunk_size = SWAP4(bin_header.chunk_size);
	p_dir = directory;
	is_first_entry = 0;
	DWORD entry_ARC4a_offset = 0x7fffffff;
	my_bin_entry_t *my_entry = index_buffer;
	my_bin_entry_t *pre_my_entry = my_entry;
	while (p_dir < directory + directory_length) {
		int name_len;
		int name_offset;
		char entry_name[16];

		name_len = *p_dir & 0xf;
		name_offset = *p_dir >> 4;
		p_dir++;
		if (name_offset != 0xf) {	// 只有部分名字
			memcpy(&entry_name[name_offset], p_dir, name_len);			
			entry_name[name_offset + name_len] = 0;
			p_dir += name_len;
		} else if (name_len) {
			if (name_len == 0xf)	// 名称序号做累加
				entry_name[strlen(entry_name) - 1]++;
			else {	// 首项
				/* 首项拥有完整的名字 */
				memcpy(entry_name, p_dir, name_len);
				entry_name[name_len] = 0;				
				p_dir += name_len;
				is_first_entry = 1;
			}
		} else {	// flag == 0xf0 @ 41e080
			u32 cur_ARC4a_offset = SWAP3(*p_dir);

			cur_ARC4a_offset = abs(cur_ARC4a_offset - bin_header.directory_offset);
			if (cur_ARC4a_offset < entry_ARC4a_offset)
				entry_ARC4a_offset = cur_ARC4a_offset;

			pre_my_entry->offset = (entry_ARC4a_offset + bin_header.data_offset) * bin_header.chunk_size;
		}

		entry_ARC4a_offset = SWAP3(*(u32 *)p_dir);
		//entry_ARC4a_offset = abs(entry_ARC4a_offset - bin_header.directory_offset);
		p_dir += 3;

		/* 首项还要忽略到下一个首相的directory偏移字段(该字段仅在查找时使用) */
		if (is_first_entry) {
			p_dir += 3;
			is_first_entry = 0;
		}

		strcpy(my_entry->name, entry_name);
		my_entry->offset = (entry_ARC4a_offset + bin_header.data_offset) * bin_header.chunk_size;
		pre_my_entry = my_entry;
		my_entry++;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(my_bin_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int CaramelBox_bin_parse_resource_info(struct package *pkg,
											  struct package_resource *pkg_res)
{
	my_bin_entry_t *my_bin_entry;
	char name[MAX_PATH];
	int name_len;
	resource_header_t res_header;
	DWORD comprlen, uncomprlen;

	my_bin_entry = (my_bin_entry_t *)pkg_res->actual_index_entry;

	if (pkg->pio->readvec(pkg, &res_header, sizeof(res_header), my_bin_entry->offset, IO_SEEK_SET))
		return -CUI_EREADVEC;
	
	name_len = strlen(my_bin_entry->name);
	for (int k = 0; k < name_len; k++) {
		if (my_bin_entry->name[k] != '*')
			name[k] = my_bin_entry->name[k];
		else
			strcpy(&name[k], "X");
	}
	name[k] = 0;
	strcpy(pkg_res->name, name + 3);
	strcat(pkg_res->name, ".");
	strncat(pkg_res->name, name, 3);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */

	switch (SWAP4(res_header.type)) {
	case 2:
		pkg_res->flags |= 0x80000000;
	case 0:
		comprlen = SWAP4(res_header.length1);
		uncomprlen = 0;
		break;
	case 1:
		printf("%s: unsupported type 1\n", pkg_res->name);
		return -CUI_EMATCH;
		break;
	case 3:
		printf("%s: unsupported type 3\n", pkg_res->name);
		return -CUI_EMATCH;
		break;
	default:
		return -CUI_EMATCH;
	}

	pkg_res->raw_data_length = comprlen;
	pkg_res->actual_data_length = uncomprlen;
	pkg_res->offset = my_bin_entry->offset + sizeof(res_header);

	return 0;
}

/* 封包资源提取函数 */
static int CaramelBox_bin_extract_resource(struct package *pkg,
										   struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	if (pkg_res->flags & 0x80000000) {
		for (DWORD i = 0; i < comprlen; i++)
			compr[i] = ~compr[i];
	}

	uncompr = NULL;
	uncomprlen = 0;

	if (!strncmp((char *)compr, "\x89PNG", 4)) {
		if (!strstr(pkg_res->name, ".png")) {
			pkg_res->flags |= PKG_RES_FLAG_APEXT;
			pkg_res->replace_extension = _T(".png");
		}
	} else if (!strncmp((char *)compr, "RIFF", 4)) {
		if (!strstr(pkg_res->name, ".wav")) {
			pkg_res->flags |= PKG_RES_FLAG_APEXT;
			pkg_res->replace_extension = _T(".wav");
		}
	} else if (!strncmp((char *)compr, "OggS", 4)) {
		if (!strstr(pkg_res->name, ".ogg")) {
			pkg_res->flags |= PKG_RES_FLAG_APEXT;
			pkg_res->replace_extension = _T(".ogg");
		}
	} else if (!strncmp((char *)compr, "lz", 2)) {
		lz_header_t *lz_header = (lz_header_t *)compr;

		uncompr = (BYTE *)malloc(SWAP4(lz_header->total_uncomprlen));
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}

		lz_chunk_t *lz_chunk = (lz_chunk_t *)(lz_header + 1);
		comprlen -= sizeof(lz_header_t) + sizeof(lz_chunk_t);
		for (uncomprlen = 0; uncomprlen < SWAP4(lz_header->total_uncomprlen); ) {
			DWORD act_comprlen;

			if (strncmp(lz_chunk->sync, "ze", 2)) {
				free(uncompr);		
				free(compr);
				return -CUI_EMATCH;	
			}
			act_comprlen = lze_uncompress(uncompr + uncomprlen, SWAP2(lz_chunk->uncomprlen), 
				(BYTE *)(lz_chunk + 1), comprlen);
			uncomprlen += SWAP2(lz_chunk->uncomprlen);
			lz_chunk = (lz_chunk_t *)((BYTE *)(lz_chunk + 1) + act_comprlen);
			comprlen -= sizeof(lz_chunk_t) + act_comprlen;
		}
		if (uncomprlen != SWAP4(lz_header->total_uncomprlen)) {
			free(uncompr);		
			free(compr);
			return -CUI_EUNCOMPR;
		}
	}

	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

/* 资源保存函数 */
static int CaramelBox_bin_save_resource(struct resource *res, 
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
static void CaramelBox_bin_release_resource(struct package *pkg, 
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
static void CaramelBox_bin_release(struct package *pkg, 
								   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation CaramelBox_bin_operation = {
	CaramelBox_bin_match,					/* match */
	CaramelBox_bin_extract_directory,		/* extract_directory */
	CaramelBox_bin_parse_resource_info,		/* parse_resource_info */
	CaramelBox_bin_extract_resource,		/* extract_resource */
	CaramelBox_bin_save_resource,			/* save_resource */
	CaramelBox_bin_release_resource,		/* release_resource */
	CaramelBox_bin_release					/* release */
};

/********************* ARC4 *********************/

/* 封包匹配回调函数 */
static int CaramelBox_ARC4_match(struct package *pkg)
{
	s8 magic[4];
	u32 sync;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "ARC4", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (pkg->pio->read(pkg, &sync, sizeof(sync))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (sync != 0x00010000) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int CaramelBox_ARC4_extract_directory(struct package *pkg,
											struct package_directory *pkg_dir)
{
	ARC4_header_t ARC4_header;
	if (pkg->pio->readvec(pkg, &ARC4_header, sizeof(ARC4_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	lz3_header_t lz3_header;
	if (pkg->pio->read(pkg, &lz3_header, sizeof(lz3_header)))
		return -CUI_EREAD;

	if (strncmp(lz3_header.magic, "tZ", 2))
		return -CUI_EMATCH;

	BYTE *block_data = (BYTE *)malloc(lz3_header.length + sizeof(ARC4_header) + 1);
	if (!block_data)
		return -CUI_EMEM;

	BYTE *b = block_data + sizeof(ARC4_header);
	for (DWORD bsize = 0; bsize < lz3_header.length; ) {
		BYTE *tmp_b;
		DWORD tmp_bsize;
		int ret = lz3_block_uncompress(pkg, &tmp_b, &tmp_bsize);
		if (ret) {
			free(block_data);
			return ret;
		}
		memcpy(b, tmp_b, tmp_bsize);
		b += tmp_bsize;
		bsize += tmp_bsize;
	}
	bsize += sizeof(ARC4_header);
	memcpy(block_data, &ARC4_header, sizeof(ARC4_header));

	if (bsize != sizeof(ARC4_header) + ARC4_header.fat_size 
			+ ARC4_header.name_space_size + ARC4_header.index_size) {
		free(block_data);
		return -CUI_EMATCH;
	}

	DWORD my_index_len = sizeof(my_ARC4_entry_t) * ARC4_header.index_entries;
	my_ARC4_entry_t *my_index = (my_ARC4_entry_t *)malloc(my_index_len);
	if (!my_index) {
		free(block_data);
		return -CUI_EMATCH;
	}

	char *name_space = (char *)block_data + ARC4_header.name_space_offset;
	lz3_fat_t *fat = (lz3_fat_t *)(block_data + sizeof(ARC4_header));
	u8 *index = (u8 *)(block_data + ARC4_header.index_offset);
	DWORD base_offset = (ARC4_header.compr_index_length + ARC4_header.chunk_size - 1) & ~(ARC4_header.chunk_size - 1);
	for (DWORD i = 0; i < ARC4_header.index_entries; ++i) {
		strncpy(my_index[i].name, name_space + SWAP3(*(u32 *)fat->name_offset) * 2, fat->name_length);
		my_index[i].name[fat->name_length] = 0;
		my_index[i].offset[0] = SWAP3(*(u32 *)fat->offset);

		if (fat->number > 1) {
			BYTE *index_base = index + 3 * my_index[i].offset[0];
			for (DWORD j = 0; j < fat->number; ++j) {
				my_index[i].offset[j] = SWAP3(*(u32 *)index_base) * ARC4_header.chunk_size;
				index_base += 3;
			}			
		} else
			 my_index[i].offset[0] *= ARC4_header.chunk_size;

		for (DWORD j = 0; j < fat->number; ++j)
			my_index[i].offset[j] += base_offset;

		my_index[i].number = fat->number;

		++fat;
	}
	free(block_data);

	pkg_dir->index_entries = ARC4_header.index_entries;
	pkg_dir->directory = my_index;
	pkg_dir->directory_length = my_index_len;
	pkg_dir->index_entry_length = sizeof(my_ARC4_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int CaramelBox_ARC4_parse_resource_info(struct package *pkg,
											   struct package_resource *pkg_res)
{
	my_ARC4_entry_t *my_ARC4_entry;

	my_ARC4_entry = (my_ARC4_entry_t *)pkg_res->actual_index_entry;

	resource2_header_t res_header;
	if (pkg->pio->readvec(pkg, &res_header, sizeof(res_header),
		my_ARC4_entry->offset[0], IO_SEEK_SET)) 
			return -CUI_EREADVEC;

	if (SWAP4(res_header.type)) {
		printf("%s: unsupported type %x, unknown %x, %x / %x\n", pkg_res->name,
			SWAP4(res_header.type), 
			SWAP4(res_header.unknown), 
			SWAP4(res_header.comprlen), 
			SWAP4(res_header.uncomprlen));
	}

	pkg_res->raw_data_length = SWAP4(res_header.comprlen);
	pkg_res->actual_data_length = 0;
	pkg_res->offset = my_ARC4_entry->offset[0] + sizeof(res_header);
	strcpy(pkg_res->name, my_ARC4_entry->name);
	pkg_res->name_length = -1;

	return 0;
}

/* 封包资源提取函数 */
static int CaramelBox_ARC4_extract_resource(struct package *pkg,
											struct package_resource *pkg_res)
{
	DWORD raw_len = pkg_res->raw_data_length;
	BYTE *raw = (BYTE *)malloc(raw_len);
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, raw_len, pkg_res->offset, IO_SEEK_SET)) {
		free(raw);
		return -CUI_EREADVEC;
	}

	my_ARC4_entry_t *my_ARC4_entry = (my_ARC4_entry_t *)pkg_res->actual_index_entry;
	int ret;
	for (DWORD j = 1; j < my_ARC4_entry->number; ++j) {
		resource2_header_t res_header;
		if (pkg->pio->readvec(pkg, &res_header, sizeof(res_header),
			my_ARC4_entry->offset[j], IO_SEEK_SET)) {
				ret = -CUI_EREADVEC;
				break;
		}

		if (SWAP4(res_header.type)) {
			printf("%s (%d): unsupported type %x, unknown %x, %x / %x\n", 
				pkg_res->name, j,
				SWAP4(res_header.type), 
				SWAP4(res_header.unknown), 
				SWAP4(res_header.comprlen), 
				SWAP4(res_header.uncomprlen));
		}
		
		DWORD cur_raw_len = SWAP4(res_header.comprlen);
		raw = (BYTE *)realloc(raw, raw_len + cur_raw_len);
		if (!raw) {
			ret = -CUI_EMEM;
			break;
		}
			
		my_ARC4_entry->offset[j] += sizeof(res_header);
		if (pkg->pio->readvec(pkg, raw + raw_len, cur_raw_len, my_ARC4_entry->offset[j], IO_SEEK_SET)) {
			ret = -CUI_EREADVEC;
			break;
		}
		raw_len += cur_raw_len;
	}
	if (j != my_ARC4_entry->number) {
		free(raw);
		return ret;
	}

	if (!strncmp((char *)raw, "tZ", 2)) {
		lz3_header_t *lz3_header = (lz3_header_t *)raw;

		BYTE *block_data = (BYTE *)malloc(lz3_header->length + 1);
		if (!block_data) {
			free(raw);
			return -CUI_EMEM;
		}
		memset(block_data, 0, lz3_header->length + 1);
		DWORD block_data_length = lz3_header->length;

		lz3_block_header_t *block_header = (lz3_block_header_t *)(lz3_header + 1);
		int ret;
		for (DWORD bsize = 0; bsize < lz3_header->length; ) {
			if (strncmp(block_header->header, "Zt", 2) 
					&& strncmp(block_header->header, "St", 2)) {
				ret = -CUI_EMATCH;
				break;
			}

			u16 *compr = (u16 *)(block_header + 1);
			u32 crc = block_header->crc;
			u32 c = 0;
			for (int i = 0; i < block_header->comprlen / 2; i++) {
				crc = 1336793 * crc + 4021;
				compr[i] -= (WORD)(crc >> 16);
				c = compr[i] + 3 * c;	
			}

			if ((c % 0xFFF1) != block_header->crc) {
				ret = -CUI_EMATCH;
				break;
			}

			BYTE *uncompr = block_data + bsize;

			if (!strncmp(block_header->header, "Zt", 2)) {
				
				if (lz3_uncompress(uncompr, block_header->uncomprlen, (BYTE *)compr, block_header->comprlen) != block_header->uncomprlen) {
					ret = -CUI_EUNCOMPR;
					break;
				}
			} else
				memcpy(uncompr, compr, block_header->uncomprlen);

			bsize += block_header->uncomprlen;
			block_header = (lz3_block_header_t *)((u8 *)block_header + block_header->comprlen + sizeof(lz3_block_header_t));
		}
		if (bsize != lz3_header->length) {
			free(block_data);
			free(raw);
			return ret;
		}
		free(raw);

		pkg_res->actual_data = block_data;
		pkg_res->actual_data_length = block_data_length;

		return 0;
	} else if (!strncmp((char *)raw, "lz", 2)) {
		lz_header_t *lz_header = (lz_header_t *)raw;

		lz_header->total_uncomprlen = SWAP4(lz_header->total_uncomprlen);
		BYTE *uncompr = (BYTE *)malloc(lz_header->total_uncomprlen);
		if (!uncompr) {
			free(raw);
			return -CUI_EMEM;
		}

		lz_chunk_t *lz_chunk = (lz_chunk_t *)(lz_header + 1);
		raw_len -= sizeof(lz_header_t) + sizeof(lz_chunk_t);
		for (DWORD uncomprlen = 0; uncomprlen < lz_header->total_uncomprlen; ) {
			DWORD act_comprlen;

			if (strncmp(lz_chunk->sync, "ze", 2)) {
				free(uncompr);		
				free(raw);
				return -CUI_EMATCH;	
			}

			act_comprlen = lze_uncompress(uncompr + uncomprlen, SWAP2(lz_chunk->uncomprlen), 
				(BYTE *)(lz_chunk + 1) + 2, raw_len);
			uncomprlen += SWAP2(lz_chunk->uncomprlen);
			lz_chunk = (lz_chunk_t *)((BYTE *)(lz_chunk + 1) + act_comprlen);
			raw_len -= sizeof(lz_chunk_t) + act_comprlen;
		}
		if (uncomprlen != lz_header->total_uncomprlen) {
			free(uncompr);		
			free(raw);
			return -CUI_EUNCOMPR;
		}
		pkg_res->actual_data = uncompr;
		pkg_res->actual_data_length = lz_header->total_uncomprlen;
		free(raw);

		return 0;
	} else if (!strncmp((char *)raw, "fcb1", 4)) {
		fcb_header_t *fcb_header = (fcb_header_t *)raw;
		lz3_header_t *lz3_header = (lz3_header_t *)(fcb_header + 1);
		DWORD width = fcb_header->width;
		DWORD height = fcb_header->height;
		BYTE *rgba_in;

		if (!fcb_header->type) {
			BYTE *block_data = (BYTE *)malloc(lz3_header->length + 1);
			if (!block_data) {
				free(raw);
				return -CUI_EMEM;
			}
			memset(block_data, 0, lz3_header->length + 1);
			DWORD block_data_length = lz3_header->length;

			lz3_block_header_t *block_header = (lz3_block_header_t *)(lz3_header + 1);
			int ret;
			for (DWORD bsize = 0; bsize < lz3_header->length; ) {
				if (strncmp(block_header->header, "Zt", 2) 
						&& strncmp(block_header->header, "St", 2)) {
					ret = -CUI_EMATCH;
					break;
				}

				u16 *compr = (u16 *)(block_header + 1);
				u32 crc = block_header->crc;
				u32 c = 0;
				for (int i = 0; i < block_header->comprlen / 2; i++) {
					crc = 1336793 * crc + 4021;
					compr[i] -= (WORD)(crc >> 16);
					c = compr[i] + 3 * c;	
				}

				if ((c % 0xFFF1) != block_header->crc) {
					ret = -CUI_EMATCH;
					break;
				}

				BYTE *uncompr = block_data + bsize;

				if (!strncmp(block_header->header, "Zt", 2)) {					
					if (lz3_uncompress(uncompr, block_header->uncomprlen, (BYTE *)compr, block_header->comprlen) != block_header->uncomprlen) {
						ret = -CUI_EUNCOMPR;
						break;
					}
				} else
					memcpy(uncompr, compr, block_header->uncomprlen);

				bsize += block_header->uncomprlen;
				block_header = (lz3_block_header_t *)((u8 *)block_header + block_header->comprlen + sizeof(lz3_block_header_t));
			}
			if (bsize != lz3_header->length) {
				free(block_data);
				free(raw);
				return ret;
			}
			rgba_in = block_data;
		} else if (fcb_header->type == 1) {
			printf("%s: unsupported zlib compression for fcb1 type1\n", pkg_res->name);
			exit(0);
		} else {
			free(raw);
			return -CUI_EMATCH;
		}
		free(raw);

		DWORD rgba_len = width * height * 4;
		BYTE *rgba = (BYTE *)malloc(rgba_len);
		if (!rgba) {
			free(rgba_in);
			return -CUI_EMEM;
		}

		int ret = fcb1_uncompress(rgba, rgba_in, width, height, 2);
		free(rgba_in);
		if (ret) {
			free(rgba);
			return ret;
		}

		alpha_blending(rgba, width, height, 32);

		if (MyBuildBMPFile(rgba, rgba_len, NULL, 0, width, 0-height, 32,
				(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, 
				my_malloc)) {
			free(rgba);
			return -CUI_EMEM;
		}
		free(rgba);
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp");

		return 0;
	} else if (!strncmp((char *)raw, "_MAPp", 5)) {
		map_header_t *map_header = (map_header_t *)raw;
		lz3_header_t *lz3_header = (lz3_header_t *)(map_header + 1);

		BYTE *block_data = (BYTE *)malloc(lz3_header->length + 1);
		if (!block_data) {
			free(raw);
			return -CUI_EMEM;
		}
		memset(block_data, 0, lz3_header->length + 1);
		DWORD block_data_length = lz3_header->length;

		lz3_block_header_t *block_header = (lz3_block_header_t *)(lz3_header + 1);
		int ret;
		for (DWORD bsize = 0; bsize < lz3_header->length; ) {
			if (strncmp(block_header->header, "Zt", 2) 
					&& strncmp(block_header->header, "St", 2)) {
				ret = -CUI_EMATCH;
				break;
			}

			u16 *compr = (u16 *)(block_header + 1);
			u32 crc = block_header->crc;
			u32 c = 0;
			for (int i = 0; i < block_header->comprlen / 2; i++) {
				crc = 1336793 * crc + 4021;
				compr[i] -= (WORD)(crc >> 16);
				c = compr[i] + 3 * c;	
			}

			if ((c % 0xFFF1) != block_header->crc) {
				ret = -CUI_EMATCH;
				break;
			}

			BYTE *uncompr = block_data + bsize;

			if (!strncmp(block_header->header, "Zt", 2)) {
				
				if (lz3_uncompress(uncompr, block_header->uncomprlen, (BYTE *)compr, block_header->comprlen) != block_header->uncomprlen) {
					ret = -CUI_EUNCOMPR;
					break;
				}
			} else
				memcpy(uncompr, compr, block_header->uncomprlen);

			bsize += block_header->uncomprlen;
			block_header = (lz3_block_header_t *)((u8 *)block_header + block_header->comprlen + sizeof(lz3_block_header_t));
		}
		if (bsize != lz3_header->length) {
			free(block_data);
			free(raw);
			return ret;
		}
		free(raw);
		pkg_res->actual_data = block_data;
		pkg_res->actual_data_length = block_data_length;

		return 0;
	}

	pkg_res->raw_data = raw;
	pkg_res->raw_data_length = raw_len;

	return 0;
}

/* 资源保存函数 */
static int CaramelBox_ARC4_save_resource(struct resource *res,
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
static void CaramelBox_ARC4_release_resource(struct package *pkg, 
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
static void CaramelBox_ARC4_release(struct package *pkg, 
									struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation CaramelBox_ARC4_operation = {
	CaramelBox_ARC4_match,				/* match */
	CaramelBox_ARC4_extract_directory,	/* extract_directory */
	CaramelBox_ARC4_parse_resource_info,/* parse_resource_info */
	CaramelBox_ARC4_extract_resource,	/* extract_resource */
	CaramelBox_ARC4_save_resource,		/* save_resource */
	CaramelBox_ARC4_release_resource,	/* release_resource */
	CaramelBox_ARC4_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK CaramelBox_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".bin"), NULL, 
		NULL, &CaramelBox_ARC4_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".データ"), NULL, 
		NULL, &CaramelBox_ARC4_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR
		| CUI_EXT_FLAG_NOEXT))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &CaramelBox_ARC4_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".bin"), NULL, 
		NULL, &CaramelBox_bin_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".ar3"), NULL, 
		NULL, &CaramelBox_bin_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
