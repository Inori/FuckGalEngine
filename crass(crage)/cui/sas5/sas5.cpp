#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <utility.h>
#include <stdio.h>

// M:\すたじお緑茶\片恋いの月～体験版～

struct acui_information sas5_cui_information = {
	_T("秋山構平"),			/* copyright */
	_T("Solfa Standard Novel System"),	/* system */
	_T(".iar .war"),		/* package */
	_T("0.5.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-7-5 19:23"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};	

#pragma pack (1)
typedef struct {
	s8 magic[4];		// "war "
	u32 head_size;
	u32 index_entries;
	u32 entry_size;	
} war_header_t;

typedef struct {
	u32 offset;
	u32 length;
	u32 unknown0;	// 0
	u32 unknown1;	// 0
	u32 unknown2;	// -1
	u8 type;		// 0(PCM), 1 or 2(OGG)
	u8 pad[3];
} war_entry_t;

typedef struct {
	s8 magic[4];		// "iar "
	u16 version;
	u16 pad;
	u32 header_size;	
} iar_header_t;

typedef struct {
	u32 info_header_size;
	u32 build_time;
	u32 dir_entries;
	u32 file_entries;
	u32 total_entries;
} iar_info_header_t;

typedef struct {		// 0x40
	u16 flags;			// 低6为不能全为0；bit9 - data是否存在data_offset
	u8 pad;
	u8 is_compressed;	// 0 or 1
	u32 unknown0;		// (0)肯定是一个长度字段
	u32 uncomprlen;
	u32 palette_length;
	
	u32 comprlen;
	u32 unknown1;		// (0)
	/* 文字显示的起始位置的坐标(10,10)视为原点的话，
	 * 那么该这里的坐标是相对于文字显示的起始位置的相对坐标的负值
	 *（背景框为10，表示背景框左上角位于该原点(-10,-10)的位置上），
	 * 右边的方框位置自然都是负值）
	 */
	u32 orig_x;
	u32 orig_y;
	
	s32 width;
	s32 height;
	u32 pitch;
	u32 unknown7;
	// 下面的字段，version 2和3才有
	u32 left_top_x;		// 左上角相对于绘图背景原点的坐标（不是屏幕原点坐标）
	u32 left_top_y;
	u32 right_top_x;	// 右上角相对于绘图背景右上角的坐标（不是屏幕原点坐标）
	u32 right_top_y;
} img_header_t;

typedef struct {
	u32 base_image_id;
	u32 start_line;
	u32 lines;
} img_delta_header_t;

typedef struct {
	u32 info_size;		// 0x10
	u32 data_size;
	u16 FormatTag;
	u16 Channels;
	u32 SamplesPerSec;
	u32 AvgBytesPerSec;
	u16 BlockAlign;
	u16 BitsPerSample;
} au_wav_header_t;
#pragma pack ()
	
typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 length;
} my_iar_entry_t;

static int debug;

static BYTE *sas5_resource;
static char *sas5_current_resource;

#define flag_shift	\
	flag >>= 1;	\
	if (flag <= 0xffff) {	\
		flag = compr[0] | (compr[1] << 8) | 0xffff0000;	\
		compr += 2;	\
	}
	
static void iar_uncompress(BYTE *uncompr, BYTE *compr)
{
	u32 flag = 0;

	while (1) {
		u32 offset, copy_bytes;

		flag_shift;
		if (flag & 1)
			*uncompr++ = *compr++;
		else {
			u32 tmp;
			
			flag_shift;
			if (flag & 1) {
				offset = 1;
				flag_shift;
				tmp = flag & 1;
				flag_shift;
				if (!(flag & 1)) {
					offset = 513;
					flag_shift;
					if (!(flag & 1)) {
						offset = 1025;
						flag_shift;
						tmp = (flag & 1) | (tmp << 1);
						flag_shift;
						if (!(flag & 1)) {
							offset = 2049;
							flag_shift;
							tmp = (flag & 1) | (tmp << 1);
							flag_shift;
							if (!(flag & 1)) {
								offset = 4097;
								flag_shift;
								tmp = (flag & 1) | (tmp << 1);
							}
						}
					}
				}
				offset = offset + ((tmp << 8) | *compr++);
				flag_shift;
				if (flag & 1)
					copy_bytes = 3;
				else {
					flag_shift;
					if (flag & 1)
						copy_bytes = 4;
					else {
						flag_shift;
						if (flag & 1)
							copy_bytes = 5;
						else {
							flag_shift;
							if (flag & 1)
								copy_bytes = 6;
							else {
								flag_shift;
								if (flag & 1) {
									flag_shift;
									if (flag & 1)
										copy_bytes = 8;
									else
										copy_bytes = 7;
								} else {
									flag_shift;
									if (flag & 1) {
										copy_bytes = *compr++ + 17;
									} else {
										flag_shift;
										copy_bytes = (flag & 1) << 2;
										flag_shift;
										copy_bytes |= (flag & 1) << 1;
										flag_shift;
										copy_bytes |= flag & 1;
										copy_bytes += 9;
									}
								}
							}
						}
					}
				}
			} else {	// 外while外
				flag_shift;
				copy_bytes = 2;
				if (flag & 1) {
					flag_shift;
					offset = (flag & 1) << 10;
					flag_shift;
					offset |= (flag & 1) << 9;
					flag_shift;
					offset = (offset | *compr++ | ((flag & 1) << 8)) + 256;
				} else {
					offset = *compr++ + 1;
					if (offset == 256)
						break;
				}
			}
			for (DWORD i = 0; i < copy_bytes; i++) {
				*uncompr = *(uncompr - offset);
				uncompr++;
			}
		}
	}
}

static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static void decrypt(BYTE *buf, DWORD len)
{
	BYTE key = 0;
	for (DWORD i = 0; i < len; i++) {
		BYTE v = buf[i];
		buf[i] ^= key;
		key += v + 18;
	}
}

static int __sas5_get_resource_name(DWORD res_load_id, char *name,
									char *pack_name)
{
	int ret = 0;

	if (!sas5_resource) {
		char tmp[MAX_PATH];
		strcpy(tmp, pack_name);
		char *p = strstr(tmp, ".");
		if (p)
			*p = 0;
		sprintf(name, "%s%_%05d", tmp, res_load_id);
		return ret;
	}

	char *resr;
	u32 total_entries;

	if (sas5_current_resource) {
		total_entries = 1;
		resr = sas5_current_resource;
	} else {
		resr = (char *)sas5_resource;
		total_entries = *(u32 *)resr;
		resr += 4;
	}

	for (DWORD i = 0; i < total_entries; ++i) {
		char *res_name = resr;
		resr += strlen(res_name) + 1;
		char *res_type = resr;
		resr += strlen(res_type) + 1;
		char *res_mode = resr;
		resr += strlen(res_mode) + 1;
		u32 res_info_sz = *(u32 *)resr;
		resr += 4;
		char *info = resr;
		char *pkg_name = info;
		info += strlen(pkg_name) + 1;
		u32 res_id = *(u32 *)info;	// 所在的封包的内部id
		resr += res_info_sz;

		if (strstr(pkg_name, pack_name)) {
			if (res_load_id == res_id) {
				strcpy(name, res_name);				
				break;
			}
		}
	}
	if (i != total_entries) {
		sas5_current_resource = resr;
		ret = 1;
	} else {	// not found
		char tmp[MAX_PATH];
		strcpy(tmp, pack_name);
		char *p = strstr(tmp, ".");
		if (p)
			*p = 0;
		sprintf(name, "%s%_%05d", tmp, res_load_id);
		sas5_current_resource = NULL;
		ret = 2;
	}
	return ret;
}

static void sas5_get_resource_name(DWORD res_load_id, char *name,
								   char *pack_name)
{
	int ret = __sas5_get_resource_name(res_load_id, name, pack_name);
	if (ret == 2)
		__sas5_get_resource_name(res_load_id, name, pack_name);		
}

/********************* sec5 *********************/

static int sas5_sec5_match(struct package *pkg)
{
	s8 magic[4];
	u32 version;
	
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "SEC5", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->pio->read(pkg, &version, sizeof(version))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (version - 100000 > 2000) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}
	
	return 0;
}

static int sas5_sec5_extract_resource(struct package *pkg,
									  struct package_resource *pkg_res)
{
	BYTE *sec5_data;
	u32 sec5_size;

	if (pkg->pio->length_of(pkg, &sec5_size))
		return -CUI_ELEN;

	sec5_data = (BYTE *)malloc(sec5_size);
	if (!sec5_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, sec5_data, sec5_size, 0, IO_SEEK_SET)) {
		free(sec5_data);
		return -CUI_EREADVEC;
	}

	BYTE *p = sec5_data + 4;
	*(u32 *)p = 2000;
	p += 4;
	while (p < sec5_data + sec5_size) {
		u32 magic = *(u32 *)p;	// block magic
		p += 4;
		u32 seg_len = *(u32 *)p;
		p += 4;

		if (!strncmp((char *)&magic, "ENDS", 4)) {
			break;
		} else if (!strncmp((char *)&magic, "RESR", 4)) {
			// 所有内部资源文件列表
			;
		} else if (!strncmp((char *)&magic, "VARS", 4)) {
			;
		} else if (!strncmp((char *)&magic, "CZIT", 4)) {
			;
		} else if (!strncmp((char *)&magic, "OPTN", 4)) {
			// ms是游戏设置相关的变量和设定值
			;
		} else if (!strncmp((char *)&magic, "CODE", 4)) {
			decrypt(p, seg_len);
		} else if (!strncmp((char *)&magic, "EXPL", 4)) {
			;
		} else if (!strncmp((char *)&magic, "VARA", 4)) {
			;
		} else if (!strncmp((char *)&magic, "RTFC", 4)) {
			// 所有游戏文件列表
			// [0](4)文件数量
			// [4](-1)文件名1
			// ...
			;
		}
		p += seg_len;
	}

	pkg_res->raw_data = sec5_data;
	pkg_res->raw_data_length = sec5_size;

	return 0;
}

static int sas5_sec5_save_resource(struct resource *res, 
								   struct package_resource *pkg_res)
{
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (pkg_res->raw_data && pkg_res->raw_data_length) {
		if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
			res->rio->close(res);
			return -CUI_EWRITE;
		}
	}
	res->rio->close(res);
	
	return 0;
}

static void sas5_sec5_release_resource(struct package *pkg, 
									   struct package_resource *pkg_res)
{
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void sas5_sec5_release(struct package *pkg, 
							 struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

static cui_ext_operation sas5_sec5_operation = {
	sas5_sec5_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	sas5_sec5_extract_resource,	/* extract_resource */
	sas5_sec5_save_resource,	/* save_resource */
	sas5_sec5_release_resource,	/* release_resource */
	sas5_sec5_release			/* release */
};

/********************* iar *********************/

static int sas5_iar_match(struct package *pkg)
{
	iar_header_t iar_header;
	iar_info_header_t iar_info;
	
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->read(pkg, &iar_header, sizeof(iar_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(iar_header.magic, "iar ", 4) 
			|| (iar_header.version != 1 && iar_header.version != 2 && iar_header.version != 3)
			|| iar_header.header_size < sizeof(iar_header)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}
	
	if (pkg->pio->read(pkg, &iar_info, sizeof(iar_info))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}
	
	if (iar_info.info_header_size < sizeof(iar_info) || iar_info.total_entries > iar_info.file_entries) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->lst) {
		int ret = sas5_sec5_match(pkg->lst);
		if (ret) {
			pkg->pio->close(pkg);
			return ret;
		}

		BYTE *sec5_data;
		u32 sec5_size;

		if (pkg->pio->length_of(pkg->lst, &sec5_size)) {
			pkg->pio->close(pkg->lst);
			pkg->pio->close(pkg);
			return -CUI_ELEN;
		}

		sec5_data = (BYTE *)malloc(sec5_size);
		if (!sec5_data) {
			pkg->pio->close(pkg->lst);
			pkg->pio->close(pkg);
			return -CUI_EMEM;
		}

		if (pkg->pio->readvec(pkg->lst, sec5_data, sec5_size, 0, IO_SEEK_SET)) {
			free(sec5_data);
			pkg->pio->close(pkg->lst);
			pkg->pio->close(pkg);
			return -CUI_EREADVEC;
		}

		BYTE *p = sec5_data + 4;
		p += 4;
		while (p < sec5_data + sec5_size) {
			u32 magic = *(u32 *)p;
			p += 4;
			u32 seg_len = *(u32 *)p;
			p += 4;
			if (!strncmp((char *)&magic, "RESR", 4)) {
				sas5_resource = (BYTE *)malloc(seg_len + 1);
				if (!sas5_resource) {
					free(sec5_data);
					pkg->pio->close(pkg->lst);
					pkg->pio->close(pkg);
					return -CUI_EMEM;
				}
				memcpy(sas5_resource, p, seg_len);
				sas5_resource[seg_len - 1] = 0;
				sas5_current_resource = NULL;
				break;
			}
			p += seg_len;
		}
		free(sec5_data);
		pkg->pio->close(pkg->lst);
	}
	
	return 0;
}

static int sas5_iar_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	iar_header_t iar_header;
	iar_info_header_t iar_info;
	my_iar_entry_t *index_buffer;
	DWORD index_length;
	DWORD offset_buffer_length;
	u32 fsize;
	void *offset_buffer;
	
	if (pkg->pio->length_of(pkg, &fsize))
		return -CUI_ELEN;

	if (pkg->pio->readvec(pkg, &iar_header, sizeof(iar_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;
	
	if (pkg->pio->read(pkg, &iar_info, sizeof(iar_info)))
		return -CUI_EREAD;
	
	index_length = iar_info.total_entries * sizeof(my_iar_entry_t);
	index_buffer = (my_iar_entry_t *)malloc(index_length);
	if (!index_buffer)
		return -CUI_EMEM;

	char pkg_name[MAX_PATH];
	unicode2sj(pkg_name, MAX_PATH, pkg->name, -1);
	
	if (iar_header.version < 3) {		
		offset_buffer_length = iar_info.total_entries * 4;
		offset_buffer = malloc(offset_buffer_length);
		if (!offset_buffer) {
			free(index_buffer);
			return -CUI_EMEM;
		}
		
		if (pkg->pio->read(pkg, offset_buffer, offset_buffer_length)) {
			free(offset_buffer);
			free(index_buffer);
			return -CUI_EREAD;
		}
		
		for (DWORD i = 0; i < iar_info.total_entries - 1; ++i) {
			sas5_get_resource_name(i, index_buffer[i].name, pkg_name);
			index_buffer[i].offset = ((u32 *)offset_buffer)[i];
			index_buffer[i].length = ((u32 *)offset_buffer)[i+1] - ((u32 *)offset_buffer)[i];
		}
		sas5_get_resource_name(i, index_buffer[i].name, pkg_name);
		index_buffer[i].offset = ((u32 *)offset_buffer)[i];
		index_buffer[i].length = fsize - ((u32 *)offset_buffer)[i];
	} else {
		offset_buffer_length = iar_info.total_entries * 8;
		offset_buffer = malloc(offset_buffer_length);
		if (!offset_buffer) {
			free(index_buffer);
			return -CUI_EMEM;
		}
		
		if (pkg->pio->read(pkg, offset_buffer, offset_buffer_length)) {
			free(offset_buffer);
			free(index_buffer);
			return -CUI_EREAD;
		}
		
		for (DWORD i = 0; i < iar_info.total_entries - 1; ++i) {
			index_buffer[i].offset = (u32)((u64 *)offset_buffer)[i];
			index_buffer[i].length = (u32)((u64 *)offset_buffer)[i+1] - (u32)((u64 *)offset_buffer)[i];
			sas5_get_resource_name(i, index_buffer[i].name, pkg_name);
		}
		sas5_get_resource_name(i, index_buffer[i].name, pkg_name);
		index_buffer[i].offset = (u32)((u64 *)offset_buffer)[i];
		index_buffer[i].length = fsize - (u32)((u64 *)offset_buffer)[i];
	}
	free(offset_buffer);
	package_set_private(pkg, (void *)iar_header.version);

	pkg_dir->index_entries = iar_info.total_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_length;
	pkg_dir->index_entry_length = sizeof(my_iar_entry_t);

	return 0;
}

static int sas5_iar_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	my_iar_entry_t *my_iar_entry;

	my_iar_entry = (my_iar_entry_t *)pkg_res->actual_index_entry;
	strncpy(pkg_res->name, my_iar_entry->name, 64);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_iar_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_iar_entry->offset;

	return 0;
}

static int sas5_iar_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr, *palette;
	img_header_t header;
	int version;
	DWORD offset;
	int bpp;
#if 0
	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		compr = (BYTE *)malloc(pkg_res->raw_data_length);
		if (!compr)
			return -CUI_EMEM;

		if (pkg->pio->readvec(pkg, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET)) {
			free(compr);
			return -CUI_EREADVEC;
		}
		pkg_res->raw_data = compr;
		return 0;
	}
#endif	
	version = (int)package_get_private(pkg);
	if (version == 1) {
		if (pkg->pio->readvec(pkg, &header, sizeof(header) - 16, pkg_res->offset, IO_SEEK_SET))
			return -CUI_EREADVEC;
		header.left_top_x = 0;
		header.left_top_y = 0;
		header.right_top_x = 0;
		header.right_top_y = 0;
		offset = pkg_res->offset + sizeof(header) - 16;
	} else {
		if (pkg->pio->readvec(pkg, &header, sizeof(header), pkg_res->offset, IO_SEEK_SET))
			return -CUI_EREADVEC;
		offset = pkg_res->offset + sizeof(header);
	}

	if (header.width < 0
		|| header.height < 0
		|| header.width >= 0x20000
		|| header.height >= 0x20000
		|| !header.uncomprlen
		|| !header.comprlen
		|| header.uncomprlen >= 0x4000000
		|| header.comprlen >= 0x4000000
		|| header.unknown0 >= 0x100000
		|| header.palette_length >= 0x100000
		|| header.is_compressed >= 2
		|| !(header.flags & 0x3F) )
    return CUI_EMATCH;

	if (header.flags & 0x0200) {	// 有palette
		palette = (BYTE *)malloc(header.palette_length);
		if (!palette)
			return -CUI_EMEM;
		
		if (pkg->pio->readvec(pkg, palette, header.palette_length, offset, IO_SEEK_SET)) {
			free(palette);
			return -CUI_EREADVEC;
		}
		offset += header.palette_length;
		if (debug)
			printf("%s: palette\n", pkg_res->name);
	} else
		palette = NULL;

	compr = (BYTE *)malloc(header.comprlen);
	if (!compr) {
		free(palette);
		return -CUI_EMEM;
	}
	
	if (pkg->pio->readvec(pkg, compr, header.comprlen, offset, IO_SEEK_SET)) {
		free(compr);	
		free(palette);
		return -CUI_EREADVEC;
	}
	
	uncompr = (BYTE *)malloc(header.uncomprlen);
	if (!uncompr) {
		free(compr);
		free(palette);
		return -CUI_EMEM;
	}

	if (header.is_compressed == 1)
		iar_uncompress(uncompr, compr);
	else if (!header.is_compressed)
		memcpy(uncompr, compr, header.uncomprlen);

	free(compr);

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->actual_data = uncompr;
		pkg_res->actual_data_length = header.uncomprlen;
		return 0;
	}

//	printf("%x %x(%d) %x %x %x @ %p\n", header.width,header.pitch,header.pitch/header.width,header.height,
//		header.palette_length, header.uncomprlen,palette);
//	printf("%x\n", header.flags);
	header.flags &= 0x093f;
	
//	bpp = header.pitch / header.width;
	bpp = 0;
	switch (header.flags) {
	case 0x01:
		bpp = 8;
		if (debug)
			printf("%s: 1\n", pkg_res->name);
		break;
	case 0x02:
		bpp = 8;
		break;
	case 0x1c:
		bpp = 24;
		break;
	case 0x3c:
		bpp = 32;
		break;
	case 0x11c:
		if (debug)
			printf("%s: 11c %x %x %x %x %x %x\n", pkg_res->name, 
				   header.uncomprlen, palette, 
				   header.palette_length, header.width, 
					header.height, bpp);
		break;
	case 0x13c:
		if (debug)
			printf("%s: 13c %x %x %x %x %x %x\n", pkg_res->name, 
				   header.uncomprlen, palette, 
				   header.palette_length, header.width, 
					header.height, bpp);
		break;
	case 0x81c:	// 差分数据
		bpp = 24;
		if (debug)
			printf("%s: 81c %x %x %x %x %x %x\n", pkg_res->name, 
				   header.uncomprlen, palette, 
				   header.palette_length, header.width, 
					header.height, bpp);
		break;
	case 0x83c:	// 差分数据
		bpp = 32;
		break;
	default:
		free(uncompr);
		free(palette);
		return -CUI_EMATCH;
	}

#if 1	// TODO
	if (header.flags == 0x81c || header.flags == 0x83c) {
		WORD Bpp = bpp / 8;
		DWORD pitch = (header.width * Bpp + 3) & ~3;
		DWORD img_size = pitch * header.height;
		BYTE *img = (BYTE *)malloc(img_size);
		if (!img) {
			free(uncompr);
			free(palette);
			return -CUI_EMEM;
		}
		memset(img, 0, img_size);

		img_delta_header_t *delta = (img_delta_header_t *)uncompr;
		BYTE *p = uncompr + sizeof(img_delta_header_t);
		BYTE *cur_line = img + pitch * delta->start_line;

		for (DWORD d = 0; d < delta->lines; ++d) {
			BYTE *dst = cur_line;
			DWORD cnt = *(u16 *)p;
			p += 2;
			for (DWORD i = 0; i < cnt; ++i) {
				u16 pos = *(u16 *)p * Bpp;
				p += 2;
				u16 count = *(u16 *)p * Bpp;
				p += 2;
				dst += pos;
				memcpy(dst, p, count);
				p += count;
				dst += count;
			}
			cur_line += pitch;
		}

		if (MyBuildBMPFile(img, img_size, NULL, 0, header.width, 
				0 - header.height, bpp, (BYTE **)&pkg_res->actual_data, 
				&pkg_res->actual_data_length, my_malloc)) {
			free(img);
			free(uncompr);
			free(palette);
			return -CUI_EMEM;
		}
		free(img);
		free(uncompr);
		free(palette);
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp");
		return 0;
	}
#endif

	if (bpp) {
		if (MyBuildBMPFile(uncompr, header.uncomprlen, palette, header.palette_length, header.width, 
				0 - header.height, bpp, (BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, my_malloc)) {
			free(uncompr);
			free(palette);
			return -CUI_EMEM;
		}
		free(uncompr);
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp");
	} else {
		pkg_res->actual_data = uncompr;
		pkg_res->actual_data_length = header.uncomprlen;
	}	
	free(palette);

	return 0;
}

static int sas5_iar_save_resource(struct resource *res, 
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

static void sas5_iar_release_resource(struct package *pkg, 
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

static void sas5_iar_release(struct package *pkg, 
							 struct package_directory *pkg_dir)
{
	if (sas5_resource) {
		free(sas5_resource);
		sas5_resource = NULL;
		sas5_current_resource = NULL;
	}
	if (pkg_dir->directory) {		
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg);
}

static cui_ext_operation sas5_iar_operation = {
	sas5_iar_match,					/* match */
	sas5_iar_extract_directory,		/* extract_directory */
	sas5_iar_parse_resource_info,	/* parse_resource_info */
	sas5_iar_extract_resource,		/* extract_resource */
	sas5_iar_save_resource,			/* save_resource */
	sas5_iar_release_resource,		/* release_resource */
	sas5_iar_release				/* release */
};

/********************* war *********************/

static int sas5_war_match(struct package *pkg)
{
	war_header_t war_header;
	
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->read(pkg, &war_header, sizeof(war_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(war_header.magic, "war ", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->lst) {
		int ret = sas5_sec5_match(pkg->lst);
		if (ret) {
			pkg->pio->close(pkg);
			return ret;
		}

		BYTE *sec5_data;
		u32 sec5_size;

		if (pkg->pio->length_of(pkg->lst, &sec5_size)) {
			pkg->pio->close(pkg->lst);
			pkg->pio->close(pkg);
			return -CUI_ELEN;
		}

		sec5_data = (BYTE *)malloc(sec5_size);
		if (!sec5_data) {
			pkg->pio->close(pkg->lst);
			pkg->pio->close(pkg);
			return -CUI_EMEM;
		}

		if (pkg->pio->readvec(pkg->lst, sec5_data, sec5_size, 0, IO_SEEK_SET)) {
			free(sec5_data);
			pkg->pio->close(pkg->lst);
			pkg->pio->close(pkg);
			return -CUI_EREADVEC;
		}

		BYTE *p = sec5_data + 4;
		p += 4;
		while (p < sec5_data + sec5_size) {
			u32 magic = *(u32 *)p;
			p += 4;
			u32 seg_len = *(u32 *)p;
			p += 4;
			if (!strncmp((char *)&magic, "RESR", 4)) {
				sas5_resource = (BYTE *)malloc(seg_len + 1);
				if (!sas5_resource) {
					free(sec5_data);
					pkg->pio->close(pkg->lst);
					pkg->pio->close(pkg);
					return -CUI_EMEM;
				}
				memcpy(sas5_resource, p, seg_len);
				sas5_resource[seg_len - 1] = 0;
				sas5_current_resource = NULL;
				break;
			}
			p += seg_len;
		}
		free(sec5_data);
		pkg->pio->close(pkg->lst);
	}
	
	return 0;
}

static int sas5_war_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	war_header_t war_header;
	if (pkg->pio->readvec(pkg, &war_header, sizeof(war_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;
	
	DWORD index_length = war_header.index_entries * sizeof(war_entry_t);
	war_entry_t *index_buffer = (war_entry_t *)malloc(index_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = war_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_length;
	pkg_dir->index_entry_length = sizeof(war_entry_t);

	return 0;
}

static int sas5_war_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	war_entry_t *war_entry = (war_entry_t *)pkg_res->actual_index_entry;
	char pkg_name[MAX_PATH];
	unicode2sj(pkg_name, MAX_PATH, pkg->name, -1);
	sas5_get_resource_name(pkg_res->index_number, 
		pkg_res->name, pkg_name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = war_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = war_entry->offset;
	if (debug)
		printf("%s: %x %x %x %x %x %x\n",pkg_res->name,
			war_entry->length, war_entry->offset,
			war_entry->unknown0, war_entry->unknown1,
			war_entry->unknown2, war_entry->type);

	return 0;
}

static int sas5_war_extract_resource(struct package *pkg,
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

	war_entry_t *war_entry = (war_entry_t *)pkg_res->actual_index_entry;
	if (war_entry->type == 2) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".ogg");
		pkg_res->raw_data = raw;
	} else if (war_entry->type == 0) {
		au_wav_header_t *wav = (au_wav_header_t *)raw;
		BYTE *pcm = raw + sizeof(au_wav_header_t);
		if (MySaveAsWAVE(pcm, wav->data_size, wav->FormatTag, 
				wav->Channels, wav->SamplesPerSec, wav->BitsPerSample, 
				NULL, 0, (BYTE **)&pkg_res->actual_data, 
				&pkg_res->actual_data_length, my_malloc)) {
			free(raw);
			return -CUI_EMEM;
		}
		free(raw);
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".wav");
	} else
		pkg_res->raw_data = raw;

	return 0;
}

static cui_ext_operation sas5_war_operation = {
	sas5_war_match,					/* match */
	sas5_war_extract_directory,		/* extract_directory */
	sas5_war_parse_resource_info,	/* parse_resource_info */
	sas5_war_extract_resource,		/* extract_resource */
	sas5_iar_save_resource,			/* save_resource */
	sas5_iar_release_resource,		/* release_resource */
	sas5_iar_release				/* release */
};

int CALLBACK sas5_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".iar"), _T(".bmp"), 
		NULL, &sas5_iar_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".war"), NULL, 
		NULL, &sas5_war_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".sec5"), _T(".sec5_"), 
		NULL, &sas5_sec5_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (get_options("debug"))
		debug = 1;
	else
		debug = 0;
	sas5_resource = NULL;
	sas5_current_resource = NULL;

	return 0;
}
