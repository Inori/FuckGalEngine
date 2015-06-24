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
#include <cui_common.h>

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information SFA_cui_information = {
	_T("LiLiM"),				/* copyright */
	_T(""),						/* system */
	_T(".aos .abm .scr .msk"),	/* package */
	_T("0.9.0"),				/* revision */
	_T("痴漢公賊"),				/* author */
	_T("2008-4-6 20:30"),		/* date */
	NULL,						/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	u32 handle;			// 0。运行时参数，存放文件句柄
	u32 data_offset;
	u32 index_length;
	s8 name[0x105];		// 封包文件名。运行时参数，存储封包文件全路径	
} aos_header_t;

typedef struct {
	s8 name[32];
	u32 offset;
	u32 length;	
} aos_entry_t;

typedef struct {
	s8 name[16];
	u32 offset;
	u32 length;
	// time stamp
	u16 second;		// 应该不是秒，但也不是星期
	u16 year;
	u8 month;
	u8 day;
	u8 hour;
	u8 minute;
} aos_old_entry_t;

typedef struct {
	u16 unknown1;	// 1
	u16 unknown4;	// 4
	u16 frames;
	u16 unknown0;	// 0
	u32 unknown_length;	// frame偏移表和第一帧数据之间的数据
//	u32 *frame_offset;
} abm1_info_t;
#pragma pack ()

typedef struct {
	char name[MAX_PATH];
	DWORD length;
	DWORD offset;
	DWORD width;
	DWORD height;
} abm1_entry_t;


static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static int huffman_tree_create(struct bits *bits, u16 children[2][255], 
							   unsigned int *index, u16 *retval)
{
	unsigned int bitval;
	u16 child;

	if (bit_get_high(bits, &bitval))
		return -1;
	
	if (bitval) {
		unsigned int parent;

		parent = *index;
		*index = parent + 1;
					
		if (huffman_tree_create(bits, children, index, &child))
			return -1;
		children[0][parent - 256] = child;
		
		if (huffman_tree_create(bits, children, index, &child))
			return -1;
		children[1][parent - 256] = child;

		child = parent;
	} else {
		unsigned int byteval;
		
		if (bits_get_high(bits, 8, &byteval))
			return -1;
		
		child = byteval;			
	}
	*retval = child;
	
	return 0;
}

static int huffman_decompress(unsigned char *uncompr, unsigned long *uncomprlen,
					 unsigned char *compr, unsigned long comprlen)
{
	struct bits bits;
	u16 children[2][255];
	unsigned int index = 256;
	unsigned long max_uncomprlen;
	unsigned long act_uncomprlen;
	unsigned int bitval;
	u16 retval;	
	
	bits_init(&bits, compr, comprlen);
	if (huffman_tree_create(&bits, children, &index, &retval))
		return -1;
	if (retval != 256)
		return -1;

	index = 0;	/* 从根结点开始遍历 */
	act_uncomprlen = 0;
	max_uncomprlen = *uncomprlen;
	while (!bit_get_high(&bits, &bitval)) {
		if (bitval)
			retval = children[1][index];
		else
			retval = children[0][index];
	
		if (retval >= 256)
			index = retval - 256;
		else {
			if (act_uncomprlen >= max_uncomprlen)
				break;
			uncompr[act_uncomprlen++] = (u8)retval;
			index = 0;	/* 返回到根结点 */
		}
	}
	*uncomprlen = act_uncomprlen;

	return 0;
}

static DWORD abm_uncompress(BYTE *uncompr, DWORD uncomprLen,
							BYTE *compr, DWORD comprLen)
{
	DWORD act_uncomprLen = 0;
	DWORD curbyte = 0;

	while (act_uncomprLen < uncomprLen) {
		BYTE flag = compr[curbyte++];
		if (flag) {
			if (flag != 0xff) {
				uncompr[act_uncomprLen++] = compr[curbyte++];
			} else {
				BYTE copy_bytes = compr[curbyte++];
				for (DWORD i = 0; i < copy_bytes; i++) {
					uncompr[act_uncomprLen++] = compr[curbyte++];
				}
			}
		} else {
			BYTE copy_bytes = compr[curbyte++];
			for (DWORD i = 0; i < copy_bytes; i++) {
				uncompr[act_uncomprLen++] = 0;
			}
		}
	}
	return act_uncomprLen;
}

static DWORD abm_uncompress32(BYTE *uncompr, DWORD uncomprLen,
							  BYTE *compr, DWORD comprLen)
{
	DWORD act_uncomprLen = 0;
	DWORD curbyte = 0;
	DWORD pixel = 0;

	while (act_uncomprLen < uncomprLen) {
		BYTE flag = compr[curbyte++];
		if (flag) {
			if (flag != 0xff) {
				uncompr[act_uncomprLen++] = compr[curbyte++];
				if (++pixel == 3) {
					uncompr[act_uncomprLen++] = flag;
					pixel = 0;
				}
			} else {
				BYTE copy_bytes = compr[curbyte++];
				for (DWORD i = 0; i < copy_bytes; i++) {
					uncompr[act_uncomprLen++] = compr[curbyte++];
					if (++pixel == 3) {
						uncompr[act_uncomprLen++] = 0xff;
						pixel = 0;
					}
				}
			}
		} else {
			BYTE copy_bytes = compr[curbyte++];
			for (DWORD i = 0; i < copy_bytes; i++) {
				uncompr[act_uncomprLen++] = 0;
				if (++pixel == 3) {
					uncompr[act_uncomprLen++] = 0;
					pixel = 0;
				}
			}
		}
	}
	return act_uncomprLen;
}

/********************* aos *********************/

/* 封包匹配回调函数 */
static int SFA_aos_match(struct package *pkg)
{
	aos_header_t aos_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &aos_header, sizeof(aos_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	char pkg_name[MAX_PATH];
	unicode2sj(pkg_name, MAX_PATH, pkg->name, -1);
	if (aos_header.handle || strcmpi(aos_header.name, pkg_name)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int SFA_aos_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{
	aos_header_t aos_header;

	if (pkg->pio->readvec(pkg, &aos_header, sizeof(aos_header), 
			0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	pkg_dir->index_entries = aos_header.index_length / sizeof(aos_entry_t);
	aos_entry_t *index_buffer = (aos_entry_t *)malloc(aos_header.index_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, aos_header.index_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	aos_entry_t *entry = index_buffer;
	for (DWORD i = 0; i < pkg_dir->index_entries; i++) {
		entry->offset += aos_header.data_offset;
		entry++;
	}

	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = aos_header.index_length;
	pkg_dir->index_entry_length = sizeof(aos_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int SFA_aos_parse_resource_info(struct package *pkg,
									   struct package_resource *pkg_res)
{
	aos_entry_t *aos_entry;

	aos_entry = (aos_entry_t *)pkg_res->actual_index_entry;
	if (!aos_entry->name[31])
		strcpy(pkg_res->name, aos_entry->name);
	else {
		strncpy(pkg_res->name, aos_entry->name, 32);
		pkg_res->name[31] = 0;
	}
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = aos_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = aos_entry->offset;

	return 0;
}

#define BIG_BUFFER_SIZE		(64 * 1024 * 1024)

/* 封包资源提取函数 */
static int SFA_aos_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	BYTE magic[16];

	if (pkg_res->raw_data_length < BIG_BUFFER_SIZE) {
		void *raw = malloc(pkg_res->raw_data_length);
		if (!raw)
			return -CUI_EMEM;

		if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length,
			pkg_res->offset, IO_SEEK_SET)) {
				free(raw);
				return -CUI_EREADVEC;
		}
		pkg_res->raw_data = raw;
		memcpy(magic, raw, 16);
	} else {
		if (pkg->pio->readvec(pkg, magic, 16,
			pkg_res->offset, IO_SEEK_SET))
				return -CUI_EREADVEC;
		pkg_res->raw_data = pkg;
	}

	if (!strncmp((char *)magic, "RIFF", 4) 
			&& !strncmp((char *)&magic[8], "AVI ", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".avi");
	}

	return 0;
}

static int SFA_aos_save_resource(struct resource *res, 
								 struct package_resource *pkg_res)
{
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (pkg_res->raw_data_length < BIG_BUFFER_SIZE) {
		if (pkg_res->raw_data && pkg_res->raw_data_length) {
			if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
				res->rio->close(res);
				return -CUI_EWRITE;
			}
		}
	} else {
		struct package *pkg = (struct package *)pkg_res->raw_data;
		void *buf = malloc(BIG_BUFFER_SIZE);
		if (!buf) {
			res->rio->close(res);
			return -CUI_EMEM;
		}
		DWORD offset = pkg_res->offset;
		for (DWORD i = 0; i < pkg_res->raw_data_length / BIG_BUFFER_SIZE; i++) {
			if (pkg->pio->readvec(pkg, buf, BIG_BUFFER_SIZE, offset, IO_SEEK_SET)) {
				free(buf);
				res->rio->close(res);
				return -CUI_EREADVEC;
			}
			if (res->rio->write(res, buf, BIG_BUFFER_SIZE)) {
				free(buf);
				res->rio->close(res);
				return -CUI_EWRITE;
			}
			offset += BIG_BUFFER_SIZE;			
		}

		DWORD rem_sz = pkg_res->raw_data_length - i * BIG_BUFFER_SIZE;
		if (rem_sz) {
			if (pkg->pio->readvec(pkg, buf, rem_sz, offset, IO_SEEK_SET)) {
				free(buf);
				res->rio->close(res);
				return -CUI_EREADVEC;
			}
			if (res->rio->write(res, buf, rem_sz)) {
				free(buf);
				res->rio->close(res);
				return -CUI_EWRITE;
			}
		}
		free(buf);
		pkg_res->raw_data = NULL;
	}

	res->rio->close(res);
	
	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation SFA_aos_operation = {
	SFA_aos_match,				/* match */
	SFA_aos_extract_directory,	/* extract_directory */
	SFA_aos_parse_resource_info,/* parse_resource_info */
	SFA_aos_extract_resource,	/* extract_resource */
	SFA_aos_save_resource,		/* save_resource */
	cui_common_release_resource,
	cui_common_release
};

/********************* aos *********************/

/* 封包匹配回调函数 */
static int SFA_aos_old_match(struct package *pkg)
{
	aos_old_entry_t aos_entry;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &aos_entry, sizeof(aos_entry))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (pkg->pio->readvec(pkg, &aos_entry, sizeof(aos_entry), 
			aos_entry.offset, IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	for (DWORD i = 0; i < sizeof(aos_entry.name); i++) {
		if (aos_entry.name[i] != -1 && aos_entry.name[i]) {
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int SFA_aos_old_extract_directory(struct package *pkg,
										 struct package_directory *pkg_dir)
{
	aos_old_entry_t aos_entry;
	u32 aos_size;

	if (pkg->pio->length_of(pkg, &aos_size))
		return -CUI_ELEN;

	if (pkg->pio->seek(pkg, 0, IO_SEEK_SET))
		return -CUI_ESEEK;

	DWORD index_entries = 0;
	DWORD offset = 0;
	while (1) {
		if (pkg->pio->readvec(pkg, &aos_entry, sizeof(aos_entry),
				offset, IO_SEEK_SET))
			return -CUI_EREADVEC;

		for (DWORD i = 0; i < sizeof(aos_entry.name); i++) {
			if (aos_entry.name[i] != -1)
				break;
		}
		offset += sizeof(aos_entry);
		if (i != sizeof(aos_entry.name)){
#if 0
			printf("%s: @ %x %d-%d-%d %02d:%02d:%02d\n",
				aos_entry.name, offset+aos_entry.offset,
				aos_entry.year,aos_entry.month,
				aos_entry.day,aos_entry.hour,aos_entry.minute,
				aos_entry.second);
			if (aos_entry.month > 12 || !aos_entry.month) {
				printf("err month %d\n", aos_entry.month);
				exit(0);
			}
			if (aos_entry.day > 31 || !aos_entry.day) {
				printf("err day %d\n", aos_entry.day);
				exit(0);
			}
			if (aos_entry.hour > 23) {
				printf("err hour %d\n", aos_entry.hour);
				exit(0);
			}
			if (aos_entry.minute > 59) {
				printf("err minute %d\n", aos_entry.minute);
				exit(0);
			}
			if (aos_entry.second > 59) {
				printf("err second %d\n", aos_entry.second);
				exit(0);
			}
#endif
			index_entries++;
		}

		if (!((offset + aos_entry.offset) & 15)) {
			if (offset + aos_entry.offset + aos_entry.length + 16 >= aos_size)
				break;		
		} else {
			if ((((offset + aos_entry.offset + aos_entry.length) + 15) & ~15) >= aos_size)
				break;
		}

		if (i == sizeof(aos_entry.name))		
			offset += aos_entry.offset;	
	}

	if (pkg->pio->seek(pkg, 0, IO_SEEK_SET))
		return -CUI_ESEEK;	
		
	DWORD index_length = index_entries * sizeof(aos_entry);
	aos_old_entry_t *index_buffer = (aos_old_entry_t *)malloc(index_length);
	if (!index_buffer)
		return -CUI_EMEM;

	offset = 0;
	for (DWORD n = 0; n < index_entries; ) {
		aos_old_entry_t *entry = &index_buffer[n];
		if (pkg->pio->readvec(pkg, entry, sizeof(aos_old_entry_t),
				offset, IO_SEEK_SET)) {
			free(index_buffer);
			return -CUI_EREADVEC;
		}

		for (DWORD i = 0; i < sizeof(entry->name); i++) {
			if (entry->name[i] != -1)
				break;
		}
		offset += sizeof(aos_old_entry_t);
		if (i == sizeof(entry->name))		
			offset += entry->offset;	
		else {
			entry->offset += offset;
			n++;
		}
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_length;
	pkg_dir->index_entry_length = sizeof(aos_old_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int SFA_aos_old_parse_resource_info(struct package *pkg,
										   struct package_resource *pkg_res)
{
	aos_old_entry_t *aos_entry;

	aos_entry = (aos_old_entry_t *)pkg_res->actual_index_entry;
	if (!aos_entry->name[15])
		strcpy(pkg_res->name, aos_entry->name);
	else {
		strncpy(pkg_res->name, aos_entry->name, 16);
		pkg_res->name[31] = 0;
	}
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = aos_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = aos_entry->offset;

	return 0;
}

static int SFA_aos_old_extract_resource(struct package *pkg,
										struct package_resource *pkg_res)
{
	void *raw = malloc(pkg_res->raw_data_length);
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
static cui_ext_operation SFA_aos_old_operation = {
	SFA_aos_old_match,				/* match */
	SFA_aos_old_extract_directory,	/* extract_directory */
	SFA_aos_old_parse_resource_info,/* parse_resource_info */
	SFA_aos_old_extract_resource,	/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* abm *********************/

/* 封包匹配回调函数 */
static int SFA_abm_match(struct package *pkg)
{
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmiHeader;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &bmfh, sizeof(bmfh))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (bmfh.bfType != 0x4d42) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (pkg->pio->read(pkg, &bmiHeader, sizeof(bmiHeader))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (bmiHeader.biBitCount == 1) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int SFA_abm_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	u32 abm_size;

	if (pkg->pio->length_of(pkg, &abm_size))
		return -CUI_ELEN;

	BYTE *abm = (BYTE *)malloc(abm_size);
	if (!abm)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, abm, abm_size, 0, IO_SEEK_SET)) {
		free(abm);
		return -CUI_EREADVEC;
	}

	BITMAPFILEHEADER *bmfh;
	BITMAPINFOHEADER *bmiHeader;
			
	bmfh = (BITMAPFILEHEADER *)abm;
	bmiHeader = (BITMAPINFOHEADER *)(bmfh + 1);

	s16 bpp = bmiHeader->biBitCount;
	u16 _bpp = bpp < 0 ? -bpp : bpp;
	DWORD uncomprLen;
	BYTE *uncompr;
	if (bpp == 2)
		_bpp = 32;
	uncomprLen = bmiHeader->biWidth * abs(bmiHeader->biHeight) * _bpp / 8;
	uncompr = (BYTE *)malloc(uncomprLen + bmfh->bfOffBits);
	if (!uncompr) {
		free(abm);
		return -CUI_EMEM;;
	}

	BYTE *compr = (BYTE *)abm + bmfh->bfOffBits;
	DWORD comprLen = abm_size - bmfh->bfOffBits;
	if (bpp == 24) {
		BYTE *src = compr;
		BYTE *dst = uncompr + bmfh->bfOffBits;
		for (int x = 0; x < bmiHeader->biWidth * abs(bmiHeader->biHeight); x++) {
			*dst++ = *src++;
			*dst++ = *src++;
			*dst++ = *src++;
			*dst++ = 0xff;
		}
	} else if (bpp == 32) {
		DWORD act_uncomprlen = abm_uncompress32(uncompr + bmfh->bfOffBits, uncomprLen, compr, comprLen);
		if (act_uncomprlen != uncomprLen) {
			free(uncompr);
			free(abm);
			return -CUI_EUNCOMPR;
		}
	} else if (bpp == -8) {
		DWORD act_uncomprlen = abm_uncompress(uncompr + bmfh->bfOffBits, uncomprLen, compr, comprLen);
		if (act_uncomprlen != uncomprLen) {
			free(uncompr);
			free(abm);
			return -CUI_EUNCOMPR;
		}
	} else if (bpp == 8)
		memcpy(uncompr + bmfh->bfOffBits, compr, uncomprLen);
	else if (bpp == 2 || bpp == 3) {
		printf("%s: unsupported bpp %x\n", pkg_res->name, bpp);
		free(uncompr);
		pkg_res->raw_data = abm;
		pkg_res->raw_data_length = abm_size;
		return 0;
	//	free(abm);
	//	return -CUI_EMATCH;
	}
	
	if (!(bmiHeader->biHeight & 0x80000000))
		bmiHeader->biHeight = 0 - bmiHeader->biHeight;
	bmiHeader->biBitCount = _bpp;
	memcpy(uncompr, bmfh, bmfh->bfOffBits);

	pkg_res->raw_data = abm;
	pkg_res->raw_data_length = abm_size;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprLen + bmfh->bfOffBits;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation SFA_abm_operation = {
	SFA_abm_match,				/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	SFA_abm_extract_resource,	/* extract_resource */
	CUI_COMMON_OPERATION		/* release */
};

/********************* abm1 *********************/

/* 封包匹配回调函数 */
static int SFA_abm1_match(struct package *pkg)
{
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmiHeader;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &bmfh, sizeof(bmfh))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (bmfh.bfType != 0x4d42) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (pkg->pio->read(pkg, &bmiHeader, sizeof(bmiHeader))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (bmiHeader.biBitCount != 1) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int SFA_abm1_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmiHeader;
	abm1_info_t info;
	u32 abm1_size;

	if (pkg->pio->length_of(pkg, &abm1_size))
		return -CUI_ELEN;

	if (pkg->pio->seek(pkg, 0, IO_SEEK_SET))
		return -CUI_ESEEK;

	if (pkg->pio->read(pkg, &bmfh, sizeof(bmfh)))
		return -CUI_EREAD;

	if (pkg->pio->read(pkg, &bmiHeader, sizeof(bmiHeader)))
		return -CUI_EREAD;

	if (pkg->pio->read(pkg, &info, sizeof(info)))
		return -CUI_EREAD;

	u32 *frame_offset_table = (u32 *)malloc(info.frames * 4);
	if (!frame_offset_table)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, frame_offset_table, info.frames * 4)) {
		free(frame_offset_table);
		return -CUI_EREAD;
	}

	DWORD index_length = info.frames * sizeof(abm1_entry_t);
	abm1_entry_t *index_buffer = (abm1_entry_t *)malloc(index_length);
	if (!index_buffer) {
		free(frame_offset_table);
		return -CUI_EMEM;
	}

	abm1_entry_t *entry = index_buffer;
	for (DWORD i = 0; i < info.frames; i++) {
		sprintf(entry->name, "%04d", i);
		entry->offset = frame_offset_table[i];
		entry->width = bmiHeader.biWidth;
		entry->height = bmiHeader.biHeight;
		entry++;
	}
	free(frame_offset_table);

	entry = index_buffer;
	for (i = 0; i < (DWORD)info.frames - 1; i++) {
		entry->length = entry[1].offset - entry->offset;
		entry++;
	}
	entry->length = abm1_size - entry->offset;

	pkg_dir->index_entries = info.frames;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_length;
	pkg_dir->index_entry_length = sizeof(abm1_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int SFA_abm1_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	abm1_entry_t *abm1_entry;

	abm1_entry = (abm1_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, abm1_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = abm1_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = abm1_entry->offset;

	return 0;
}
/* 封包资源提取函数 */
static int SFA_abm1_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	BYTE *abm1 = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!abm1)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, abm1, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET)) {
		free(abm1);
		return -CUI_EREADVEC;
	}

	abm1_entry_t *abm1_entry = (abm1_entry_t *)pkg_res->actual_index_entry;
	DWORD uncomprLen = abm1_entry->width * abm1_entry->height * 4;
	BYTE *uncompr = (BYTE *)malloc(uncomprLen);
	if (!uncompr) {
		free(abm1);
		return -CUI_EMEM;;
	}
		
	if (pkg_res->index_number == 0) {
		BYTE *p = uncompr;
		BYTE *cp = abm1;
		for (DWORD x = 0; x < abm1_entry->width * abm1_entry->height; x++) {
			*p++ = *cp++;
			*p++ = *cp++;
			*p++ = *cp++;
			*p++ = 0xff;
		}
	} else {
		if (abm_uncompress32(uncompr, uncomprLen, abm1, uncomprLen) != uncomprLen) {
			free(uncompr);
			free(abm1);
			return -CUI_EUNCOMPR;
		}
	}

	if (MyBuildBMPFile(uncompr, uncomprLen, NULL, 0, abm1_entry->width,
			0 - abm1_entry->height, 32, (BYTE **)&pkg_res->actual_data, 
			&pkg_res->actual_data_length, my_malloc)) {
		free(uncompr);
		free(abm1);
		return -CUI_EMEM;
	}
	free(uncompr);
	pkg_res->raw_data = abm1;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation SFA_abm1_operation = {
	SFA_abm1_match,					/* match */
	SFA_abm1_extract_directory,		/* extract_directory */
	SFA_abm1_parse_resource_info,	/* parse_resource_info */
	SFA_abm1_extract_resource,		/* extract_resource */
	CUI_COMMON_OPERATION			/* release */
};

/********************* scr *********************/

/* 封包匹配回调函数 */
static int SFA_scr_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包资源提取函数 */
static int SFA_scr_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	u32 scr_size;

	if (pkg->pio->length_of(pkg, &scr_size))
		return -CUI_ELEN;

	void *scr = malloc(scr_size);
	if (!scr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, scr, scr_size, 0, IO_SEEK_SET)) {
		free(scr);
		return -CUI_EREADVEC;
	}

	u32 uncomprlen = *(u32 *)scr;
	BYTE *uncompr = (BYTE *)malloc(uncomprlen);
	if (!uncompr) {
		free(scr);
		return -CUI_EMEM;
	}
	BYTE *compr = (BYTE *)scr + 4;
	DWORD comprlen = scr_size - 4;
	DWORD act_uncomprlen = uncomprlen;
	if (huffman_decompress(uncompr, &act_uncomprlen, compr, comprlen)) {
		free(uncompr);
		free(scr);
		return -CUI_EUNCOMPR;
	}
	if (uncomprlen != act_uncomprlen) {
		free(uncompr);
		free(scr);
		return -CUI_EUNCOMPR;
	}
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = act_uncomprlen;
	pkg_res->raw_data = scr;
	pkg_res->raw_data_length = scr_size;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation SFA_scr_operation = {
	SFA_scr_match,				/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	SFA_scr_extract_resource,	/* extract_resource */
	CUI_COMMON_OPERATION		/* release */
};

/********************* msk *********************/

/* 封包匹配回调函数 */
static int SFA_msk_match(struct package *pkg)
{
	s8 magic[2];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (magic[0] != 'B' || magic[1] != 'M') {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包资源提取函数 */
static int SFA_msk_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	u32 msk_size;

	if (pkg->pio->length_of(pkg, &msk_size))
		return -CUI_ELEN;

	void *msk = malloc(msk_size);
	if (!msk)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, msk, msk_size, 0, IO_SEEK_SET)) {
		free(msk);
		return -CUI_EREADVEC;
	}
	pkg_res->raw_data = msk;
	pkg_res->raw_data_length = msk_size;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation SFA_msk_operation = {
	SFA_msk_match,				/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	SFA_msk_extract_resource,	/* extract_resource */
	CUI_COMMON_OPERATION		/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK SFA_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".aos"), NULL, 
		NULL, &SFA_aos_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".aos"), NULL, 
		NULL, &SFA_aos_old_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".abm"), _T(".bmp"), 
		NULL, &SFA_abm_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".abm"), _T(".bmp"), 
		NULL, &SFA_abm1_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".scr"), _T(".txt"), 
		NULL, &SFA_scr_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".msk"), _T(".msk.bmp"), 
		NULL, &SFA_msk_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	return 0;
}
