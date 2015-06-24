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
struct acui_information Leaf_cui_information = {
	_T("Leaf"),				/* copyright */
	NULL,					/* system */
	_T(".pak .ar1 .ar2 .LAC .wmv"),	/* package */
	_T("0.5.2"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-6-27 13:05"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 head[4];		// "KCAP"	
	u32	famount;	
} PACHEAD;

typedef struct {
	s8 head[4];		// "KCAP"	
	u32 unknown;
	u32	famount;	
} PACHEAD2;

typedef struct {
	u32	type;		
	s8 fname[24];	
	u32	offset;		
	u32	size;		
} PAC_FILE_INFO;

typedef struct {
	s8 head[4];		// "LAC"
	u32	fileCnt;	
} LACHEAD;

typedef struct {
	s8 fname[31];
	s8 bCompress;
	u32	pack_size;
	u32	seekPoint;
} LAC_FILE_INFO;

typedef struct {
	s8 head[4];		// "LAC"
	u32	fileCnt;	
} LACHEAD2;

typedef struct {
	s8 fname[62];
	s8 bCompress;		// 0 - 非压缩; 非0 - 实际pack_size要减去4字节
	s8 unused_count;	// 该值不等于0的资源将不被安装在游戏目录中
	/* while (i < unused && byte_43A4EE + 1 != unused_map[i]))
          ++i;
	 */
	u8 unused_map[12];
	u32	pack_size;	// 0x4c
	u32	file_size;	// 0x50
	u32 zero;		// 0x54
	u32	seekPoint_lo;	// 0x58
	u32	seekPoint_hi;	// 0x5c
	FILETIME CreationTime;
	FILETIME LastAccess;
	FILETIME LastWrite;
} LAC2_FILE_INFO;

typedef struct {
	s8 head[12];		// "TEX PACK0.02"
	u32	fileCnt;
	u8 reserved[16];	// 0
} TEXHEAD;

typedef struct {
	s8 fname[32];
	s32 seekPoint;
	u32	pack_size;
} TEX_FILE_INFO;

typedef struct {
	s8 head[3];		// "lgf"
	u8 bpp;
	u16 width;
	u16 height;
	u32 zero;
} LGFHEAD;

typedef struct {
	s8 head[4];		// "YANI"
	u16	fileCnt;
	u8 unknown0;	// 1
	u8 unknown1;	// 0
	u8 unknown2;	// 0
	u8 bpp;
	u16 width;
	u16 height;
	u16 width0;		// ? 可以是0
	u16 height0;	// ? 可以是0
} ANIHEAD;

typedef struct {
	s32 seekPoint;
	u32	pack_size;
} ANI_FILE_INFO;

typedef struct {
	s8 magic[4];		// "ar10"
	u32 index_entries;	// 76 bytes per entry
	u32 data_offset;
	u8 xor;
} ar1_header_t;

typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 length;
} ar1_entry_t;

typedef struct {
	s8 magic[4];		// "cz10"
	u32 pictures;
} cz1_header_t;

typedef struct {
	u16 width;
	u16 height;
	u32 reserved;
	u32 unknown1;
	u32 bpp;
} cz1_info_t;

typedef struct {
	s8 magic[4];		// "ar20"
	u32 index_entries;
} ar2_header_t;

typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 length;
	u32 key;
} ar2_entry_t;

typedef struct {
	s8 magic[4];		// "cz20"
	u32 pictures;
} cz2_header_t;

typedef struct {
	u16 width;
	u16 height;
	u32 zero;	
	u32 comprlen;
	u32 bpp;
} cz2_info_t;

typedef struct {
	WCHAR name[MAX_PATH];
	u32 offset;
	u32 length;
	u32 width;
	u32 height;
	u32 bpp;
} cz2_entry_t;

typedef struct {
	s8 magic[8];	// "LEAFPACK"
	u16 index_entries;
} LEAFPACK_header_t;

typedef struct {
	s8 fname[12];
	s32 seekPoint;
	u32	file_size;
	u32	next_seekPoint;
} LEAFPACK_FILE_INFO;
#pragma pack ()

typedef struct {
	char name[MAX_PATH];
	DWORD seekPoint;
	DWORD pack_size;
	DWORD is_compr;
} my_ani_entry_t;

static BYTE LEAFPACK_decode_table[11] = {
	0x71, 0x48, 0x6A, 0x55, 
	0x9F, 0x13, 0x58, 0xF7, 
	0xD1, 0x7C, 0x3E
};

static BYTE ar2_decode_table[512] = {
		0x1d, 0x4f, 0xa2, 0x33, 0x2d, 0x17, 0x56, 0xbb, 0xce, 0xf2, 0xc6, 0xdd, 0x01, 0x62, 0x88, 0x51, 
		0x21, 0xf2, 0xd3, 0x2c, 0xc7, 0xf3, 0xc4, 0x36, 0x01, 0xc3, 0x43, 0x39, 0x00, 0x91, 0x35, 0x7e, 
		0x9b, 0x00, 0x21, 0xe9, 0x7f, 0x46, 0x61, 0xc8, 0x5b, 0x75, 0x15, 0x55, 0x45, 0xc9, 0x67, 0x98, 
		0xf8, 0x0b, 0x1c, 0x8f, 0x6c, 0x40, 0x4f, 0x4b, 0xa4, 0xf5, 0xb3, 0x5e, 0xef, 0x08, 0x42, 0x7c, 
		0xde, 0x9e, 0xce, 0x13, 0xe2, 0xe2, 0xd4, 0x91, 0x7a, 0xc6, 0x9a, 0x40, 0x52, 0xdb, 0x7a, 0x2e, 
		0x5f, 0xba, 0x64, 0xc9, 0xb4, 0x61, 0x38, 0xbc, 0x0d, 0xff, 0x31, 0x56, 0x2d, 0xaa, 0x38, 0xbd, 
		0x44, 0xf8, 0xad, 0x6a, 0x40, 0xd7, 0x58, 0x23, 0x36, 0xf9, 0x23, 0xfc, 0x07, 0x33, 0xa5, 0xdd, 
		0xa7, 0x10, 0xfa, 0x32, 0x73, 0x23, 0x99, 0x7a, 0xbb, 0x24, 0xef, 0x92, 0xde, 0x51, 0x6e, 0x7c, 
		0x91, 0x8b, 0x8e, 0x7a, 0x66, 0x2a, 0x06, 0x39, 0x60, 0x48, 0x9b, 0x6b, 0x03, 0x3d, 0xff, 0x8f, 
		0xc6, 0x48, 0x8f, 0x70, 0xb1, 0xca, 0x34, 0x72, 0xf7, 0xf8, 0xfa, 0xb6, 0x49, 0xe3, 0x63, 0x52, 
		0x00, 0x73, 0xff, 0xe7, 0xd5, 0x90, 0x32, 0x1b, 0x87, 0xf1, 0xa0, 0xf8, 0xf1, 0xe2, 0x76, 0x46, 
		0xfe, 0x32, 0x15, 0x7e, 0x1a, 0x7d, 0x60, 0x75, 0x0a, 0xf6, 0x54, 0xe3, 0x02, 0xcc, 0x48, 0x19, 
		0xf6, 0x5f, 0xdd, 0xb9, 0x56, 0x05, 0xa2, 0xee, 0x98, 0x50, 0x4c, 0x43, 0x23, 0x48, 0xbd, 0xd8, 
		0xf0, 0xa5, 0x11, 0x7f, 0x52, 0xde, 0x07, 0xc5, 0x93, 0x82, 0x0d, 0x9a, 0xcc, 0x64, 0x7e, 0x7e, 
		0x63, 0x04, 0x1b, 0x54, 0xe1, 0xce, 0x59, 0x79, 0x6e, 0xb3, 0x79, 0x31, 0xc9, 0xc6, 0x79, 0x01, 
		0xbd, 0xce, 0xf4, 0x5f, 0x0e, 0xde, 0x9d, 0x82, 0x2a, 0x8d, 0xac, 0x74, 0x35, 0x58, 0x69, 0x5d, 
		0x61, 0x7a, 0x1c, 0x3b, 0x16, 0x3e, 0xe4, 0x8e, 0x83, 0xe6, 0xdb, 0x89, 0x08, 0x1d, 0x93, 0xf6, 
		0x95, 0x39, 0xe0, 0xb7, 0x13, 0x81, 0x17, 0xab, 0xf8, 0x8d, 0x17, 0x05, 0x4c, 0x4e, 0x1f, 0xc7, 
		0x4d, 0xcc, 0xb1, 0xc1, 0x61, 0x40, 0xb7, 0x69, 0xef, 0xe7, 0x33, 0x56, 0xf8, 0x0b, 0x71, 0x3f, 
		0xbb, 0x2d, 0xb9, 0xb5, 0xc5, 0x99, 0xe4, 0x78, 0x5c, 0x41, 0xb0, 0x9b, 0x1b, 0xfd, 0xd0, 0x37, 
		0x11, 0xbb, 0xe3, 0x7b, 0x76, 0x81, 0x66, 0xd7, 0x03, 0xc9, 0xcc, 0x59, 0x0e, 0x01, 0x03, 0xf7, 
		0x7c, 0x53, 0xd8, 0x9c, 0x3e, 0xb2, 0xe2, 0x04, 0xda, 0x69, 0x1d, 0xfe, 0xdd, 0xb0, 0x02, 0x90, 
		0x57, 0x6b, 0x42, 0x35, 0x38, 0xdf, 0x33, 0xa2, 0x10, 0xa5, 0x94, 0x49, 0x2d, 0x3c, 0x33, 0x70, 
		0x33, 0x22, 0x2a, 0x12, 0x3b, 0x29, 0xf6, 0xd2, 0x40, 0x53, 0x09, 0x93, 0x8a, 0x9a, 0x8c, 0x2b, 
		0xaa, 0x5c, 0xb3, 0x3f, 0x4b, 0x3d, 0x85, 0xc5, 0x8d, 0x69, 0xf1, 0x37, 0x8a, 0x64, 0xd5, 0x30, 
		0x94, 0xfd, 0xdf, 0x9e, 0x5b, 0x34, 0x3c, 0x44, 0xd0, 0x74, 0x13, 0xa2, 0x43, 0xec, 0xf8, 0x6d, 
		0xe0, 0xbe, 0xb7, 0x9e, 0x5e, 0x67, 0x70, 0x76, 0x85, 0x90, 0xfe, 0x66, 0x0e, 0xcc, 0x94, 0x7b, 
		0xe3, 0x07, 0x63, 0xbc, 0x6d, 0x13, 0x27, 0xfa, 0xc1, 0xab, 0x65, 0x01, 0xa2, 0xb9, 0xd6, 0x40, 
		0x11, 0x95, 0xab, 0x97, 0xce, 0x3a, 0x57, 0xc0, 0x82, 0x84, 0xc9, 0xc5, 0x45, 0xe0, 0x82, 0xcb, 
		0x5b, 0xd9, 0xf3, 0x02, 0x4e, 0x99, 0xa0, 0x7f, 0xbf, 0xf1, 0x25, 0x85, 0xef, 0x88, 0xa7, 0xa9, 
		0x38, 0x25, 0xac, 0xd0, 0x04, 0x8a, 0x15, 0x9e, 0x82, 0x12, 0x79, 0xd3, 0x20, 0xb2, 0x11, 0x13, 
		0xf3, 0x17, 0x14, 0x34, 0x67, 0x8d, 0xf1, 0xd3, 0x22, 0x1f, 0xc8, 0xc5, 0x51, 0x25, 0xd9, 0x27 
};

static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static DWORD lzss_uncompress(BYTE *uncompr, DWORD uncomprlen, BYTE *compr, DWORD comprlen)
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

	memset(win, ' ', nCurWindowByte);
	while (1) {
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

			if (curbyte + 1 >= comprlen)
				break;

			win_offset = compr[curbyte++];
			copy_bytes = compr[curbyte++];
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

static DWORD lzss_uncompress3(BYTE *uncompr, DWORD uncomprlen, BYTE *compr, DWORD comprlen)
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

			if (curbyte + 1 >= comprlen)
				break;

			win_offset = compr[curbyte++];
			copy_bytes = compr[curbyte++];
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

static void lzss_uncompress2(BYTE *uncompr, DWORD uncomprlen, BYTE *compr, DWORD comprlen)
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
			if (curbyte >= comprlen)
				break;
		}

		if (flag & 1) {
			unsigned char data;
			data = compr[curbyte++];
			if (curbyte >= comprlen)
				break;
			uncompr[act_uncomprlen++] = data;
			if (act_uncomprlen >= uncomprlen)
				break;
			/* 输出的1字节放入滑动窗口 */
			win[nCurWindowByte++] = data;
			nCurWindowByte &= win_size - 1;
		} else {
			unsigned int copy_bytes, win_offset;
			unsigned int i;

			win_offset = compr[curbyte++];
			if (curbyte >= comprlen)
				break;
			copy_bytes = compr[curbyte++];
			if (curbyte >= comprlen)
				break;
			win_offset |= (copy_bytes & 0xf0) << 4;
			copy_bytes &= 0x0f;
			copy_bytes += 3;

			for (i = 0; i < copy_bytes; i++) {	
				unsigned char data;

				data = win[(win_offset + i) & (win_size - 1)];
				uncompr[act_uncomprlen++] = data;
				if (act_uncomprlen >= uncomprlen)
					return;
				/* 输出的1字节放入滑动窗口 */
				win[nCurWindowByte++] = data;
				nCurWindowByte &= win_size - 1;	
			}
		}
	}
}

static void BJR_decode(BYTE *out, BYTE *in, char *BJR_name, int width, int height)
{
	int key[3] = { 0, 0x10000, 0 };
	char bjr_chr = *BJR_name++;
printf("%s...\n", BJR_name);
	while (bjr_chr) {
		key[0] += bjr_chr;
		key[1] -= bjr_chr;
		key[2] ^= bjr_chr;
		bjr_chr = *BJR_name++;
	}

	int KEY = key[2] % height;
	if (height < 0)
		height = -height;

    int line_len = width * 3;
    int line_padding = width & 3;
    if (width < 0 && line_padding)
		line_padding = ((line_padding - 1) | ~3) + 1;
    int act_line_len = line_len + line_padding;

	for (int h = 0; h < height; ++h) {
		int y = (KEY + 7) % height;

		KEY = (KEY + 7) % height;
		BYTE *src = &in[y * act_line_len];

		for (int w = 0; w < line_len; ++w) {
			if (!(w & 1))
				*out++ = (0x100FF - *src++ - key[1]) % 256;
			else
				*out++ = (*src++ - key[0] + 0x10000) % 256;
        }
	}
}


/********************* pak *********************/

/* 封包匹配回调函数 */
static int Leaf_pak_match(struct package *pkg)
{
	s8 head[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, head, sizeof(head))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(head, "KCAP", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int Leaf_pak_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	PACHEAD head1;
	PACHEAD2 head2;
	PAC_FILE_INFO info1, info2;

	if (pkg->pio->readvec(pkg, &head1, sizeof(head1), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	if (pkg->pio->read(pkg, &info1, sizeof(info1)))
		return -CUI_EREAD;

	if (pkg->pio->readvec(pkg, &head2, sizeof(head2), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	if (pkg->pio->read(pkg, &info2, sizeof(info2)))
		return -CUI_EREAD;

	DWORD head_size;
	u32	famount;
	if (info2.offset == sizeof(head2) + head2.famount * sizeof(PAC_FILE_INFO)) {
		head_size = sizeof(head2);
		famount = head2.famount;
	} else if (info1.offset == sizeof(head1) + head1.famount * sizeof(PAC_FILE_INFO)) {
		head_size = sizeof(head1);
		famount = head1.famount;
	} else
		return -CUI_EMATCH;

	DWORD index_buffer_length = famount * sizeof(PAC_FILE_INFO);
	PAC_FILE_INFO *index_buffer = (PAC_FILE_INFO *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, index_buffer, index_buffer_length, head_size, IO_SEEK_SET)) {
		free(index_buffer);
		return -CUI_EREADVEC;
	}

	pkg_dir->index_entries = famount;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(PAC_FILE_INFO);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int Leaf_pak_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	PAC_FILE_INFO *pak_entry;

	pak_entry = (PAC_FILE_INFO *)pkg_res->actual_index_entry;
	if (pak_entry->fname[23])
		pkg_res->name_length = 24;
	else
		pkg_res->name_length = strlen(pak_entry->fname);
	strncpy(pkg_res->name, pak_entry->fname, pkg_res->name_length);
	pkg_res->name[pkg_res->name_length] = 0;
	pkg_res->raw_data_length = pak_entry->size;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = pak_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int Leaf_pak_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	PAC_FILE_INFO *pak_entry;
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)pkg->pio->readvec_only(pkg, comprlen, pkg_res->offset, IO_SEEK_SET);
	if (!compr)
		return -CUI_EREADVECONLY;

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = compr;
		return 0;		
	}

	BYTE *act_data;
	DWORD act_data_len;
	pak_entry = (PAC_FILE_INFO *)pkg_res->actual_index_entry;
	if (!pak_entry->type) {
		uncompr = NULL;
		uncomprlen = 0;
		act_data = compr;
		act_data_len = comprlen;
	} else {
		uncomprlen = *(u32 *)(compr + 4);
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr)
			return -CUI_EMEM;
		comprlen = *(u32 *)compr;
		if (lzss_uncompress(uncompr, uncomprlen, compr + 8, comprlen - 8) != uncomprlen) {
			free(uncompr);
			return -CUI_EUNCOMPR;
		}
		act_data = uncompr;
		act_data_len = uncomprlen;
	}	

	if (strstr(pkg_res->name, ".BJR")) {
		BITMAPFILEHEADER *bmfh = (BITMAPFILEHEADER *)act_data;
		BITMAPINFOHEADER *bmiHeader = (BITMAPINFOHEADER *)(bmfh + 1);
		BYTE *BJR_out = (BYTE *)malloc(act_data_len);
		if (!BJR_out) {
			free(uncompr);
			return -CUI_EMEM;
		}
		memcpy(BJR_out, act_data, bmfh->bfOffBits);
		BJR_decode(BJR_out + bmfh->bfOffBits, act_data + bmfh->bfOffBits,
			pkg_res->name, bmiHeader->biWidth, bmiHeader->biHeight);
		free(uncompr);
		uncompr = BJR_out;
		uncomprlen = act_data_len;
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".BMP");
	}

	pkg_res->raw_data = compr;
	pkg_res->raw_data_length = comprlen;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

/* 资源保存函数 */
static int Leaf_pak_save_resource(struct resource *res, 
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
static void Leaf_pak_release_resource(struct package *pkg, 
									  struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res->raw_data)
		pkg_res->raw_data = NULL;
}

/* 封包卸载函数 */
static void Leaf_pak_release(struct package *pkg, 
							 struct package_directory *pkg_dir)
{
	void *priv = (void *)package_get_private(pkg);
	if (priv) {
		free(priv);
		package_set_private(pkg, NULL);
	}

	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation Leaf_pak_operation = {
	Leaf_pak_match,					/* match */
	Leaf_pak_extract_directory,		/* extract_directory */
	Leaf_pak_parse_resource_info,	/* parse_resource_info */
	Leaf_pak_extract_resource,		/* extract_resource */
	Leaf_pak_save_resource,			/* save_resource */
	Leaf_pak_release_resource,		/* release_resource */
	Leaf_pak_release				/* release */
};

/********************* ar1 *********************/

/* 封包匹配回调函数 */
static int Leaf_ar1_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "ar10", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int Leaf_ar1_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	ar1_header_t ar1_header;
	DWORD index_buffer_length;	
	u32 ar1_size;

	if (pkg->pio->length_of(pkg, &ar1_size))
		return -CUI_ELEN;

	if (pkg->pio->readvec(pkg, &ar1_header, sizeof(ar1_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	BYTE *index_buffer = (BYTE *)malloc(ar1_header.data_offset - sizeof(ar1_header));
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, ar1_header.data_offset - sizeof(ar1_header))) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	index_buffer_length = ar1_header.index_entries * sizeof(ar1_entry_t);
	ar1_entry_t *my_index_buffer = (ar1_entry_t *)malloc(index_buffer_length);
	if (!my_index_buffer) {
		free(index_buffer);
		return -CUI_EMEM;
	}

	DWORD ar1_index_length = 0;
	BYTE *p = index_buffer;
	for (DWORD i = 0; i < ar1_header.index_entries; i++) {
		u8 name_len;

		name_len = strlen((char *)p);
		strncpy(my_index_buffer[i].name, (char *)p, name_len);
		my_index_buffer[i].name[name_len] = 0;
		p += name_len + 1;
		my_index_buffer[i].offset = *(u32 *)p + ar1_header.data_offset;
		p += 4;
	}
	free(index_buffer);

	for (i = 0; i < ar1_header.index_entries - 1; i++)
		my_index_buffer[i].length = my_index_buffer[i+1].offset - my_index_buffer[i].offset; 		
	my_index_buffer[i].length = ar1_size - my_index_buffer[i].offset; 

	pkg_dir->index_entries = ar1_header.index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(ar1_entry_t);
	package_set_private(pkg, ar1_header.xor);

	return 0;
}

/* 封包索引项解析函数 */
static int Leaf_ar1_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	ar1_entry_t *ar1_entry;

	ar1_entry = (ar1_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, ar1_entry->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = ar1_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = ar1_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int Leaf_ar1_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	BYTE *raw, *act;
	BYTE xor;

	raw = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET)) {
		free(raw);
		return -CUI_EREADVEC;
	}

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = raw;
		return 0;		
	}

	DWORD act_len = *(u32 *)raw;
	act = (BYTE *)malloc(act_len);
	if (!act) {
		free(raw);
		return -CUI_EMEM;
	}
	xor = (BYTE)package_get_private(pkg);
	BYTE xor_len = raw[4] ^ xor;
	BYTE *dec = raw + 4 + 1;
	BYTE *enc = dec + xor_len;
	for (DWORD i = 0, k = 0; i < act_len; i++) {
		act[i] = enc[i] ^ dec[k++];
		if (k == xor_len)
			k = 0;
	}

	pkg_res->raw_data = raw;
	pkg_res->actual_data = act;
	pkg_res->actual_data_length = act_len;

	return 0;
}

/* 封包资源释放函数 */
static void Leaf_ar1_release_resource(struct package *pkg, 
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
static void Leaf_ar1_release(struct package *pkg, 
							 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation Leaf_ar1_operation = {
	Leaf_ar1_match,					/* match */
	Leaf_ar1_extract_directory,		/* extract_directory */
	Leaf_ar1_parse_resource_info,	/* parse_resource_info */
	Leaf_ar1_extract_resource,		/* extract_resource */
	Leaf_pak_save_resource,			/* save_resource */
	Leaf_ar1_release_resource,		/* release_resource */
	Leaf_ar1_release				/* release */
};

/********************* ar2 *********************/

/* 封包匹配回调函数 */
static int Leaf_ar2_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "ar20", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int Leaf_ar2_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	DWORD index_buffer_length;	
	u32	index_entries;

	if (pkg->pio->read(pkg, &index_entries, 4))
		return -CUI_EREAD;

	index_buffer_length = index_entries * sizeof(ar2_entry_t);
	ar2_entry_t *index_buffer = (ar2_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	DWORD ar2_index_length = 0;
	for (DWORD i = 0; i < index_entries; i++) {
		u8 name_len;

		if (pkg->pio->read(pkg, &name_len, 1))
			break;

		if (pkg->pio->read(pkg, index_buffer[i].name, name_len))
			break;

		for (DWORD k = 0; k < name_len; k++)
			index_buffer[i].name[k] ^= name_len;
		index_buffer[i].name[name_len] = 0;

		if (pkg->pio->read(pkg, &index_buffer[i].offset, 4))
			break;

		if (pkg->pio->read(pkg, &index_buffer[i].length, 4))
			break;

		if (pkg->pio->read(pkg, &index_buffer[i].key, 4))
			break;

		ar2_index_length += 1 + name_len + 12;
	}
	if (i != index_entries) {
		free(index_buffer);
		return -CUI_EREAD;
	}
	for (i = 0; i < index_entries; i++)
		index_buffer[i].offset += sizeof(ar2_header_t) + ar2_index_length;

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(ar2_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int Leaf_ar2_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	ar2_entry_t *ar2_entry;

	ar2_entry = (ar2_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, ar2_entry->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = ar2_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = ar2_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int Leaf_ar2_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	BYTE *compr;
	DWORD comprlen;
	ar2_entry_t *ar2_entry;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = compr;
		return 0;		
	}

	ar2_entry = (ar2_entry_t *)pkg_res->actual_index_entry;
	for (DWORD i = 0; i < comprlen; i++)
		compr[i] ^= ar2_decode_table[(ar2_entry->key & 0xff) + (i & 0xff)];

	pkg_res->raw_data = compr;
	pkg_res->raw_data_length = comprlen;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation Leaf_ar2_operation = {
	Leaf_ar2_match,					/* match */
	Leaf_ar2_extract_directory,		/* extract_directory */
	Leaf_ar2_parse_resource_info,	/* parse_resource_info */
	Leaf_ar2_extract_resource,		/* extract_resource */
	Leaf_pak_save_resource,			/* save_resource */
	Leaf_ar1_release_resource,		/* release_resource */
	Leaf_pak_release				/* release */
};

/********************* cz2 *********************/

/* 封包匹配回调函数 */
static int Leaf_cz2_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "cz20", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int Leaf_cz2_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	DWORD index_buffer_length;	
	u32	index_entries;

	if (pkg->pio->read(pkg, &index_entries, 4))
		return -CUI_EREAD;

	index_entries &= 0xff;
	cz2_info_t *index_buffer = (cz2_info_t *)malloc(index_entries * sizeof(cz2_info_t));
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_entries * sizeof(cz2_info_t))) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	index_buffer_length = index_entries * sizeof(cz2_entry_t);
	cz2_entry_t *my_index_buffer = (cz2_entry_t *)malloc(index_buffer_length);
	if (!my_index_buffer) {
		free(index_buffer);
		return -CUI_EMEM;
	}

	DWORD offset = sizeof(cz2_header_t) + index_entries * sizeof(cz2_info_t);
	for (DWORD i = 0; i < index_entries; i++) {
		swprintf(my_index_buffer[i].name, L"%s.%03d.bmp", pkg->name, i);
		my_index_buffer[i].length = index_buffer[i].comprlen;
		my_index_buffer[i].width = index_buffer[i].width;
		my_index_buffer[i].height = index_buffer[i].height;
		my_index_buffer[i].bpp = index_buffer[i].bpp;
		my_index_buffer[i].offset = offset;

		offset += index_buffer[i].comprlen;
	}
	free(index_buffer);

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(cz2_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int Leaf_cz2_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	cz2_entry_t *cz2_entry;

	cz2_entry = (cz2_entry_t *)pkg_res->actual_index_entry;
	wcscpy((WCHAR *)pkg_res->name, cz2_entry->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = cz2_entry->length;
	pkg_res->actual_data_length = cz2_entry->width * cz2_entry->height * cz2_entry->bpp;
	pkg_res->offset = cz2_entry->offset;
	pkg_res->flags |= PKG_RES_FLAG_UNICODE;

	return 0;
}

/* 封包资源提取函数 */
static int Leaf_cz2_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;
	cz2_entry_t *cz2_entry;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = compr;
		return 0;		
	}

	uncomprlen = pkg_res->actual_data_length;
	uncompr = (BYTE *)malloc(uncomprlen);
	if (!uncompr) {
		free(compr);
		return -CUI_EMEM;
	}

	if (lzss_uncompress3(uncompr, uncomprlen, compr, comprlen) != uncomprlen) {
		free(uncompr);
		free(compr);
		return -CUI_EUNCOMPR;
	}

	cz2_entry = (cz2_entry_t *)pkg_res->actual_index_entry;
	for (DWORD i = cz2_entry->bpp; i < uncomprlen; i++)
		uncompr[i] = uncompr[i - cz2_entry->bpp] - uncompr[i];
	
	if (MyBuildBMPFile(uncompr, uncomprlen, NULL, 0, cz2_entry->width, 
			0 - cz2_entry->height, cz2_entry->bpp * 8, (BYTE **)&pkg_res->actual_data,
			&pkg_res->actual_data_length, my_malloc)) {
		free(uncompr);
		free(compr);
		return -CUI_EMEM;
	}
	free(uncompr);

	pkg_res->raw_data = compr;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation Leaf_cz2_operation = {
	Leaf_cz2_match,					/* match */
	Leaf_cz2_extract_directory,		/* extract_directory */
	Leaf_cz2_parse_resource_info,	/* parse_resource_info */
	Leaf_cz2_extract_resource,		/* extract_resource */
	Leaf_pak_save_resource,			/* save_resource */
	Leaf_ar1_release_resource,		/* release_resource */
	Leaf_pak_release				/* release */
};

/********************* lac *********************/

/* 封包匹配回调函数 */
static int Leaf_lac_match(struct package *pkg)
{
	s8 head[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, head, sizeof(head))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(head, "LAC", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int Leaf_lac_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	DWORD index_buffer_length;	
	u32	famount;

	if (pkg->pio->read(pkg, &famount, 4))
		return -CUI_EREAD;

	index_buffer_length = famount * sizeof(LAC_FILE_INFO);
	LAC_FILE_INFO *index_buffer = (LAC_FILE_INFO *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = famount;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(LAC_FILE_INFO);

	return 0;
}

/* 封包索引项解析函数 */
static int Leaf_lac_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	LAC_FILE_INFO *lac_entry;

	lac_entry = (LAC_FILE_INFO *)pkg_res->actual_index_entry;
	if (lac_entry->fname[30])
		pkg_res->name_length = 31;
	else
		pkg_res->name_length = strlen(lac_entry->fname);
	for (int i = 0; i < pkg_res->name_length; i++)
		pkg_res->name[i] = ~lac_entry->fname[i];
	pkg_res->raw_data_length = lac_entry->pack_size;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = lac_entry->seekPoint;

	return 0;
}

/* 封包资源提取函数 */
static int Leaf_lac_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	LAC_FILE_INFO *lac_entry;
	BYTE *compr;
	DWORD comprlen;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (pkg->pio->readvec(pkg, compr, comprlen, pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = compr;
		return 0;		
	}

	lac_entry = (LAC_FILE_INFO *)pkg_res->actual_index_entry;
	BYTE *act_data;
	DWORD act_data_len;
	if (!lac_entry->bCompress) {
		act_data = compr;
		act_data_len = comprlen;
	} else {
		u32 extra_size, uncomprlen;

		if (*(u32 *)compr == comprlen) {	// old format
			uncomprlen = *(u32 *)(compr + 4);
			comprlen = *(u32 *)compr;
			extra_size = 8;
		} else {
			uncomprlen = *(u32 *)compr;
			extra_size = 4;
		}

		BYTE *uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}

		lzss_uncompress2(uncompr, uncomprlen, compr + extra_size, comprlen - extra_size);
		free(compr);
		act_data = uncompr;
		act_data_len = uncomprlen;
	}	

	if (!memcmp(act_data, "lgf", 3)) {
		LGFHEAD *lgf = (LGFHEAD *)act_data;
		BYTE *rgb = (BYTE *)(lgf + 1);
		BYTE *palette = NULL;
		DWORD palette_size = 0;

		if (lgf->bpp == 9) {
			lgf->bpp = 8;
			palette = rgb;
			palette_size = 1024;
			rgb += 1024;
		}

		DWORD rbg_len = lgf->width * lgf->height * lgf->bpp / 8;

		if (MyBuildBMPFile(rgb, rbg_len, palette, palette_size, 
				lgf->width, -lgf->height, lgf->bpp, (BYTE **)&pkg_res->actual_data,
				&pkg_res->actual_data_length, my_malloc)) {
			free(act_data);
			return -CUI_EMEM;
		}
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp");
	}

	pkg_res->raw_data = act_data;
	pkg_res->raw_data_length = act_data_len;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation Leaf_lac_operation = {
	Leaf_lac_match,					/* match */
	Leaf_lac_extract_directory,		/* extract_directory */
	Leaf_lac_parse_resource_info,	/* parse_resource_info */
	Leaf_lac_extract_resource,		/* extract_resource */
	Leaf_pak_save_resource,			/* save_resource */
	Leaf_ar1_release_resource,		/* release_resource */
	Leaf_pak_release				/* release */
};

/********************* LAC *********************/

/* 封包匹配回调函数 */
static int Leaf_LAC_match(struct package *pkg)
{
	s8 head[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, head, sizeof(head))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(head, "LAC", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int Leaf_LAC_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	DWORD index_buffer_length;	
	u32	famount;

	if (pkg->pio->read(pkg, &famount, 4))
		return -CUI_EREAD;

	index_buffer_length = famount * sizeof(LAC2_FILE_INFO);
	LAC2_FILE_INFO *index_buffer = (LAC2_FILE_INFO *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = famount;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(LAC2_FILE_INFO);

	return 0;
}

/* 封包索引项解析函数 */
static int Leaf_LAC_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	LAC2_FILE_INFO *lac_entry;

	lac_entry = (LAC2_FILE_INFO *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, lac_entry->fname);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = lac_entry->pack_size;
	pkg_res->actual_data_length = lac_entry->file_size;
	pkg_res->offset = lac_entry->seekPoint_lo;

	return 0;
}

/* 封包资源提取函数 */
static int Leaf_LAC_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	LAC2_FILE_INFO *lac_entry = (LAC2_FILE_INFO *)pkg_res->actual_index_entry;
	DWORD comprlen = lac_entry->pack_size;
	BYTE *compr = (BYTE *)pkg->pio->readvec_only64(pkg, comprlen, 0,
		lac_entry->seekPoint_lo, lac_entry->seekPoint_hi, IO_SEEK_SET);
	if (!compr)
		return -CUI_EREADVECONLY;

	if (lac_entry->bCompress) {	// 待测试
		DWORD uncomprlen = *(u32 *)compr;
		comprlen -= 4;
		BYTE *uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr)
			return -CUI_EMEM;

		lzss_uncompress2(uncompr, uncomprlen, compr + 4, comprlen);
		pkg_res->actual_data = uncompr;
		pkg_res->actual_data_length = uncomprlen;
	}
	pkg_res->raw_data = compr;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation Leaf_LAC_operation = {
	Leaf_LAC_match,					/* match */
	Leaf_LAC_extract_directory,		/* extract_directory */
	Leaf_LAC_parse_resource_info,	/* parse_resource_info */
	Leaf_LAC_extract_resource,		/* extract_resource */
	Leaf_pak_save_resource,			/* save_resource */
	Leaf_pak_release_resource,		/* release_resource */
	Leaf_pak_release				/* release */
};

/********************* wmv *********************/

/* 封包匹配回调函数 */
static int Leaf_wmv_match(struct package *pkg)
{
	s8 head[10];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, head, sizeof(head))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(head, "\x00\x00\x00\x00\x00\x00\x00\x00\xa6\xd9", 10)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int Leaf_wmv_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	u32 fsize;

	pkg->pio->length_of(pkg, &fsize);

	BYTE *raw = (BYTE *)malloc(fsize);
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, fsize, 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	memcpy(raw, "\x30\x26\xb2\x75\x8e\x66\xcf\x11", 8);

	pkg_res->raw_data = raw;
	pkg_res->raw_data_length = fsize;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation Leaf_wmv_operation = {
	Leaf_wmv_match,				/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	Leaf_wmv_extract_resource,	/* extract_resource */
	Leaf_pak_save_resource,		/* save_resource */
	Leaf_ar1_release_resource,	/* release_resource */
	Leaf_pak_release			/* release */
};

/********************* TEX *********************/

/* 封包匹配回调函数 */
static int Leaf_TEX_match(struct package *pkg)
{
	TEXHEAD head;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &head, sizeof(head))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(head.head, "TEX PACK0.02", 12) || !head.fileCnt) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int Leaf_TEX_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	DWORD index_buffer_length;	
	TEXHEAD head;

	if (pkg->pio->readvec(pkg, &head, sizeof(head), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	index_buffer_length = head.fileCnt * sizeof(TEX_FILE_INFO);
	TEX_FILE_INFO *index_buffer = (TEX_FILE_INFO *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	for (DWORD i = 0; i < head.fileCnt; ++i)
		index_buffer[i].seekPoint += sizeof(TEXHEAD) + index_buffer_length;

	pkg_dir->index_entries = head.fileCnt;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(TEX_FILE_INFO);

	return 0;
}

/* 封包索引项解析函数 */
static int Leaf_TEX_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	TEX_FILE_INFO *tex_entry;

	tex_entry = (TEX_FILE_INFO *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, tex_entry->fname);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = tex_entry->pack_size;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = tex_entry->seekPoint;

	return 0;
}

/* 封包资源提取函数 */
static int Leaf_TEX_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	BYTE *raw = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length, 
			pkg_res->offset, IO_SEEK_SET)) {
		free(raw);
		return -CUI_EREADVEC;
	}

	pkg_res->raw_data = raw;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation Leaf_TEX_operation = {
	Leaf_TEX_match,					/* match */
	Leaf_TEX_extract_directory,		/* extract_directory */
	Leaf_TEX_parse_resource_info,	/* parse_resource_info */
	Leaf_TEX_extract_resource,		/* extract_resource */
	Leaf_pak_save_resource,			/* save_resource */
	Leaf_ar1_release_resource,		/* release_resource */
	Leaf_pak_release				/* release */
};

/********************* ANI *********************/

/* 封包匹配回调函数 */
static int Leaf_ANI_match(struct package *pkg)
{
	s8 head[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, head, sizeof(head))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(head, "YANI", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int Leaf_ANI_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	ANIHEAD *head = (ANIHEAD *)malloc(sizeof(ANIHEAD));
	if (!head)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, head, sizeof(ANIHEAD), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD index_buffer_length = head->fileCnt * sizeof(ANI_FILE_INFO);
	ANI_FILE_INFO *index_buffer = (ANI_FILE_INFO *)malloc(index_buffer_length);
	if (!index_buffer) {
		free(head);
		return -CUI_EMEM;
	}

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		free(head);
		return -CUI_EREAD;
	}

	index_buffer_length = head->fileCnt * sizeof(my_ani_entry_t);
	my_ani_entry_t *my_index = (my_ani_entry_t *)malloc(index_buffer_length);
	if (!head) {
		free(index_buffer);
		free(head);
		return -CUI_EMEM;
	}

	for (DWORD i = 0; i < head->fileCnt; ++i) {
		sprintf(my_index[i].name, "%06d", i);
		my_index[i].is_compr = !(index_buffer[i].pack_size & 0x80000000);
		my_index[i].pack_size = index_buffer[i].pack_size & ~0x80000000;
		my_index[i].seekPoint = index_buffer[i].seekPoint;
	}
	free(index_buffer);

	pkg_dir->index_entries = head->fileCnt;
	pkg_dir->directory = my_index;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(my_ani_entry_t);
	package_set_private(pkg, head);

	return 0;
}

/* 封包索引项解析函数 */
static int Leaf_ANI_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	my_ani_entry_t *ani_entry;

	ani_entry = (my_ani_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, ani_entry->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = ani_entry->pack_size;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = ani_entry->seekPoint;

	return 0;
}

/* 封包资源提取函数 */
static int Leaf_ANI_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	DWORD comprlen = pkg_res->raw_data_length;
	BYTE *compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, 
			pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}
	
	DWORD act_data_len;
	BYTE *act_data;
	my_ani_entry_t *ani_entry = (my_ani_entry_t *)pkg_res->actual_index_entry;
	if (ani_entry->is_compr) {
		u32 uncomprlen = *(u32 *)compr;
		BYTE *uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}

		lzss_uncompress2(uncompr, uncomprlen, compr + 4, comprlen - 4);
		free(compr);
		act_data = uncompr;
		act_data_len = uncomprlen;
	} else {
		act_data = compr;
		act_data_len = comprlen;
	}

	ANIHEAD *head = (ANIHEAD *)package_get_private(pkg);

	BYTE *palette = NULL;
	DWORD palette_size = 0;
	if (head->bpp == 8) {
		DWORD offset = sizeof(ANIHEAD) + head->fileCnt * sizeof(ANI_FILE_INFO) 
			+ pkg_res->index_number * 1024;

		palette = (BYTE *)malloc(1024);
		if (!palette) {
			free(act_data);
			return -CUI_EMEM;			
		}

		if (pkg->pio->readvec(pkg, palette, 1024, offset, IO_SEEK_SET)) {
			free(act_data);
			return -CUI_EREADVEC;
		}
		palette_size = 1024;
	} else if (head->bpp == 32) {
		BYTE *p = act_data;
		for (int i = 0; i < head->width * head->height; ++i) {
			p[0] = p[0] * p[3] / 255;
			p[1] = p[1] * p[3] / 255;
			p[2] = p[2] * p[3] / 255;
			p += 4;
		}			
	}

	if (MyBuildBMPFile(act_data, act_data_len, palette, palette_size, head->width, 
			head->height, head->bpp, (BYTE **)&pkg_res->actual_data,
			&pkg_res->actual_data_length, my_malloc)) {
		free(palette);
		free(act_data);
		return -CUI_EMEM;
	}
	free(palette);
	free(act_data);

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation Leaf_ANI_operation = {
	Leaf_ANI_match,					/* match */
	Leaf_ANI_extract_directory,		/* extract_directory */
	Leaf_ANI_parse_resource_info,	/* parse_resource_info */
	Leaf_ANI_extract_resource,		/* extract_resource */
	Leaf_pak_save_resource,			/* save_resource */
	Leaf_ar1_release_resource,		/* release_resource */
	Leaf_pak_release				/* release */
};

/********************* LEAFPACK *********************/

/* 封包匹配回调函数 */
static int Leaf_LEAFPACK_match(struct package *pkg)
{
	s8 head[8];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, head, sizeof(head))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(head, "LEAFPACK", 8)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int Leaf_LEAFPACK_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{
	DWORD index_buffer_length;	
	u16	famount;

	if (pkg->pio->read(pkg, &famount, 2))
		return -CUI_EREAD;

	index_buffer_length = famount * sizeof(LEAFPACK_FILE_INFO);
	LEAFPACK_FILE_INFO *index_buffer = (LEAFPACK_FILE_INFO *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	u32 fsize;
	pkg->pio->length_of(pkg, &fsize);

	if (pkg->pio->readvec(pkg, index_buffer, index_buffer_length, fsize - index_buffer_length, IO_SEEK_SET)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	BYTE *p = (BYTE *)index_buffer;
	for (int i = 0; i < index_buffer_length; ++i)
		p[i] -= LEAFPACK_decode_table[i % sizeof(LEAFPACK_decode_table)];

	pkg_dir->index_entries = famount;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(LEAFPACK_FILE_INFO);

	return 0;
}

/* 封包索引项解析函数 */
static int Leaf_LEAFPACK_parse_resource_info(struct package *pkg,
											 struct package_resource *pkg_res)
{
	LEAFPACK_FILE_INFO *entry;

	entry = (LEAFPACK_FILE_INFO *)pkg_res->actual_index_entry;
	if (entry->fname[11])
		pkg_res->name_length = 12;
	else
		pkg_res->name_length = strlen(entry->fname);
	strncpy(pkg_res->name, entry->fname, pkg_res->name_length);
	pkg_res->raw_data_length = entry->file_size;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = entry->seekPoint;

	return 0;
}

/* 封包资源提取函数 */
static int Leaf_LEAFPACK_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	LAC_FILE_INFO *lac_entry;
	BYTE *compr;
	DWORD comprlen;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (pkg->pio->readvec(pkg, compr, comprlen, pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = compr;
		return 0;		
	}

	for (int i = 0; i < comprlen; ++i)
		compr[i] -= LEAFPACK_decode_table[i % sizeof(LEAFPACK_decode_table)];

	pkg_res->raw_data = compr;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation Leaf_LEAFPACK_operation = {
	Leaf_LEAFPACK_match,				/* match */
	Leaf_LEAFPACK_extract_directory,	/* extract_directory */
	Leaf_LEAFPACK_parse_resource_info,	/* parse_resource_info */
	Leaf_LEAFPACK_extract_resource,		/* extract_resource */
	Leaf_pak_save_resource,				/* save_resource */
	Leaf_ar1_release_resource,			/* release_resource */
	Leaf_pak_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK Leaf_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".pak"), NULL, 
		NULL, &Leaf_pak_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".pak"), NULL, 
		NULL, &Leaf_lac_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".LAC"), NULL, 
		NULL, &Leaf_LAC_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".ar1"), NULL, 
		NULL, &Leaf_ar1_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".ar2"), NULL, 
		NULL, &Leaf_ar2_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".cz2"), NULL, 
		NULL, &Leaf_cz2_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".wmv"), NULL, 
		NULL, &Leaf_wmv_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".TEX"), _T(".DDS"), 
		NULL, &Leaf_TEX_operation, CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".ANI"), _T(".bmp"), 
		NULL, &Leaf_ANI_operation, CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".pak"), NULL, 
		NULL, &Leaf_LEAFPACK_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
