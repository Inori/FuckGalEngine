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
struct acui_information AI6WIN_cui_information = {
	_T("elf"),				/* copyright */
	_T(""),					/* system */
	_T(".arc .rmt .VSD .akb"),	/* package */
	_T("1.1.2"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-5-1 14:22"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	u32 index_entries;
} arc_header_t;

typedef struct {
	s8 name[260];
	u32 comprlen;
	u32 uncomprlen;
	u32 offset;
} arc_entry_t;

typedef struct {
	s8 magic[4];		// "RMT "
	u32 orig_x;
	u32 orig_y;
	u32 width;
	u32 height;
} rmt_header_t;

typedef struct {
	s8 magic[4];		// "AKB "
	u16 width;
	u16 height;
	u32 flags;
	u32 mask;			// 合成用屏蔽掩码
	u32 orig_x;			// 有效的显示区域，与flags有关
	u32 orig_y;			// flags & 0x80000000: (x0, y0) - (x1, y1) 需要和基准图做合成，这里给出的是合成区域
	u32 end_x;			// !(flags & 0x80000000): (0, 0) - (width, height) 全部都是有效的显示区域
	u32 end_y;			// flags & 0x40000000: 
} akb_header_t;

typedef struct {
	s8 magic[4];		// "VSD1"
	u32 offset;
} VSD_header_t;
#pragma pack ()

#define SWAP4(x)	(((x) & 0xff) << 24 | ((x) & 0xff00) << 8 | ((x) & 0xff0000) >> 8 | ((x) & 0xff000000) >> 24)


static BYTE *init_picture;
static char init_picture_name_pattern[MAX_PATH];

static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static void lzss_uncompress(BYTE *uncompr, DWORD uncomprlen, 
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
		if (!(flag & 0x0100))
			flag = compr[curbyte++] | 0xff00;

		if (flag & 1) {
			unsigned char data;

			data = compr[curbyte++];
			uncompr[act_uncomprlen++] = data;
			/* 输出的1字节放入滑动窗口 */
			win[nCurWindowByte++] = data;
			nCurWindowByte &= win_size - 1;
		} else {
			unsigned int copy_bytes, win_offset;
			unsigned int i;

			win_offset = compr[curbyte++];
			copy_bytes = compr[curbyte++];
			win_offset |= (copy_bytes & 0xf0) << 4;
			copy_bytes &= 0x0f;
			copy_bytes += 3;

			for (i = 0; i < copy_bytes; i++) {	
				unsigned char data;

				if (act_uncomprlen >= uncomprlen)
					return;
				data = win[(win_offset + i) & (win_size - 1)];				
				uncompr[act_uncomprlen++] = data;
				/* 输出的1字节放入滑动窗口 */
				win[nCurWindowByte++] = data;
				nCurWindowByte &= win_size - 1;	
			}
		}
	}
}

static void lzss_uncompress2(BYTE *uncompr, DWORD uncomprlen, 
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
	while (curbyte < comprlen) {
		flag >>= 1;
		if (!(flag & 0x0100))
			flag = compr[curbyte++] | 0xff00;

		if (flag & 1) {
			unsigned char data;

			data = compr[curbyte++];
			uncompr[act_uncomprlen++] = data;
			/* 输出的1字节放入滑动窗口 */
			win[nCurWindowByte++] = data;
			nCurWindowByte &= win_size - 1;
		} else {
			unsigned int copy_bytes, win_offset;
			unsigned int i;

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
}

/********************* arc *********************/

/* 封包匹配回调函数 */
static int AI6WIN_arc_match(struct package *pkg)
{
	u32 index_entries; 

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

	return 0;	
}

/* 封包索引目录提取函数 */
static int AI6WIN_arc_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	arc_header_t arc_header;

	if (pkg->pio->readvec(pkg, &arc_header, sizeof(arc_header), 
			0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD index_buffer_length = arc_header.index_entries * sizeof(arc_entry_t);
	arc_entry_t *index_buffer = (arc_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	arc_entry_t *entry = index_buffer;
	for (DWORD i = 0; i < arc_header.index_entries; ++i) {
		int name_len = strlen(entry->name);
		for (int k = 0; k < name_len; k++)
			entry->name[k] -= name_len + 1 - k;
		entry->comprlen = SWAP4(entry->comprlen);
		entry->uncomprlen = SWAP4(entry->uncomprlen);
		entry->offset = SWAP4(entry->offset);
		++entry;
	}

	pkg_dir->index_entries = arc_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(arc_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int AI6WIN_arc_parse_resource_info(struct package *pkg,
										  struct package_resource *pkg_res)
{
	arc_entry_t *arc_entry;

	arc_entry = (arc_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, arc_entry->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = arc_entry->comprlen;
	pkg_res->actual_data_length = arc_entry->uncomprlen;
	pkg_res->offset = arc_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int AI6WIN_arc_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD uncomprlen;

	compr = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, pkg_res->raw_data_length,
			pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = compr;
		return 0;
	}

	if (pkg_res->raw_data_length != pkg_res->actual_data_length) {
		uncomprlen = pkg_res->actual_data_length;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}
		lzss_uncompress2(uncompr, uncomprlen, compr, pkg_res->raw_data_length);
	} else {
		uncompr = NULL;
		uncomprlen = 0;
	}

	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;
	pkg_res->raw_data = compr;

	return 0;
}

/* 资源保存函数 */
static int AI6WIN_arc_save_resource(struct resource *res, 
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
static void AI6WIN_arc_release_resource(struct package *pkg, 
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
static void AI6WIN_arc_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation AI6WIN_arc_operation = {
	AI6WIN_arc_match,				/* match */
	AI6WIN_arc_extract_directory,	/* extract_directory */
	AI6WIN_arc_parse_resource_info,	/* parse_resource_info */
	AI6WIN_arc_extract_resource,	/* extract_resource */
	AI6WIN_arc_save_resource,		/* save_resource */
	AI6WIN_arc_release_resource,	/* release_resource */
	AI6WIN_arc_release				/* release */
};

/********************* rmt *********************/

/* 封包匹配回调函数 */
static int AI6WIN_rmt_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "RMT ", sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int AI6WIN_rmt_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	BYTE *rmt;
	u32 rmt_size;

	if (pkg->pio->length_of(pkg, &rmt_size))
		return -CUI_ELEN;

	rmt = (BYTE *)malloc(rmt_size);
	if (!rmt)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, rmt, rmt_size, 0, IO_SEEK_SET)) {
		free(rmt);
		return -CUI_EREADVEC;
	}

	rmt_header_t *rmt_header = (rmt_header_t *)rmt;
	DWORD rgba_size = rmt_header->width * rmt_header->height * 4;
	BYTE *rgba = (BYTE *)malloc(rgba_size);
	if (!rgba) {
		free(rmt);
		return -CUI_EMEM;
	}

	lzss_uncompress(rgba, rgba_size, (BYTE *)(rmt_header + 1), 
		rmt_size - sizeof(rmt_header_t));

	BYTE *p = rgba + 4;
	for (DWORD x = 1; x < rmt_header->width; x++) {
		p[0] += p[-4];
		p[1] += p[-3];
		p[2] += p[-2];
		p[3] += p[-1];
		p += 4;
	}

	BYTE *pp = rgba;
	for (x = rmt_header->width; x < rmt_header->width * rmt_header->height; x++) {
		p[0] += pp[0];
		p[1] += pp[1];
		p[2] += pp[2];
		p[3] += pp[3];
		p += 4;
		pp += 4;
	}

	if (MyBuildBMPFile(rgba, rgba_size, NULL, 0, rmt_header->width,
			rmt_header->height, 32, (BYTE **)&pkg_res->actual_data,
			&pkg_res->actual_data_length, my_malloc)) {
		free(rgba);
		free(rmt);
		return -CUI_EMEM;
	}
	free(rgba);
	pkg_res->raw_data = rmt;
	pkg_res->raw_data_length = rmt_size;

	return 0;
}

/* 封包卸载函数 */
static void AI6WIN_rmt_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation AI6WIN_rmt_operation = {
	AI6WIN_rmt_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	AI6WIN_rmt_extract_resource,/* extract_resource */
	AI6WIN_arc_save_resource,	/* save_resource */
	AI6WIN_arc_release_resource,/* release_resource */
	AI6WIN_rmt_release			/* release */
};

/********************* akb *********************/

/* 封包匹配回调函数 */
static int AI6WIN_akb_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "AKB ", sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int AI6WIN_akb_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	BYTE *akb;
	u32 akb_size;

	if (pkg->pio->length_of(pkg, &akb_size))
		return -CUI_ELEN;

	akb = (BYTE *)malloc(akb_size);
	if (!akb)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, akb, akb_size, 0, IO_SEEK_SET)) {
		free(akb);
		return -CUI_EREADVEC;
	}

	akb_header_t *akb_header = (akb_header_t *)akb;
	DWORD width = akb_header->end_x - akb_header->orig_x;
	DWORD height = akb_header->end_y - akb_header->orig_y;	

	if (!width || !height)
		return 0;

//wcprintf(pkg->name);
//printf(": [%x %x] <%x / %x> (%x, %x) - (%x, %x) \n", akb_header->flags,akb_header->mask,
//	   akb_header->width,akb_header->height,
//	   akb_header->orig_x,akb_header->orig_y,
//	   akb_header->end_x, akb_header->end_y);

	if (!(akb_header->flags & 0x40000000)) {
		DWORD rgba_size = width * height * 4;
		BYTE *rgba = (BYTE *)malloc(rgba_size);
		if (!rgba) {
			free(akb);
			return -CUI_EMEM;
		}

		lzss_uncompress2(rgba, rgba_size, (BYTE *)(akb_header + 1), 
			akb_size - sizeof(akb_header_t));

		BYTE *p = rgba + rgba_size - width * 4 + 4;
		for (DWORD x = 1; x < width; ++x) {
			p[0] += p[-4];
			p[1] += p[-3];
			p[2] += p[-2];
			p[3] += p[-1];
			p += 4;
		}

		BYTE *pp = rgba + rgba_size - width * 4;
		p = pp - width * 4;
		for (DWORD y = 1; y < height; ++y) {
			for (x = 0; x < width; ++x) {
				p[0] += pp[0];
				p[1] += pp[1];
				p[2] += pp[2];
				p[3] += pp[3];
				p += 4;
				pp += 4;
			}
			pp -= width * 4 * 2;
			p -= width * 4 * 2;
		}

		if (MyBuildBMPFile(rgba, rgba_size, NULL, 0, width,
				height, 32, (BYTE **)&pkg_res->actual_data,
				&pkg_res->actual_data_length, my_malloc)) {
			free(rgba);
			free(akb);
			return -CUI_EMEM;
		}
		delete [] rgba;
	} else {
		DWORD rgb_size = width * height * 3;
		BYTE *rgb = (BYTE *)malloc(rgb_size);
		if (!rgb) {
			free(akb);
			return -CUI_EMEM;
		}

		lzss_uncompress2(rgb, rgb_size, (BYTE *)(akb_header + 1), 
			akb_size - sizeof(akb_header_t));

		BYTE *p = rgb + rgb_size - width * 3 + 3;
		for (DWORD x = 1; x < width; ++x) {
			p[0] += p[-3];
			p[1] += p[-2];
			p[2] += p[-1];
			p += 3;
		}

		BYTE *pp = rgb + rgb_size - width * 3;
		p = pp - width * 3;
		for (DWORD y = 1; y < height; ++y) {
			for (x = 0; x < width; ++x) {
				p[0] += pp[0];
				p[1] += pp[1];
				p[2] += pp[2];
				p += 3;
				pp += 3;
			}
			pp -= width * 3 * 2;
			p -= width * 3 * 2;
		}

#if 1
		if (MyBuildBMPFile(rgb, rgb_size, NULL, 0, width,
				height, 24, (BYTE **)&pkg_res->actual_data,
				&pkg_res->actual_data_length, my_malloc)) {
			free(rgb);
			free(akb);
			return -CUI_EMEM;
		}
		free(rgb);

		if (akb_header->flags & 0x80000000) {
			TCHAR *name = (TCHAR *)pkg_res->name;
			TCHAR *p = _tcsstr(name, _T(".akb"));
			_stprintf(p, _T("_%d-%d_%d-%d.akb"), akb_header->orig_x,
				akb_header->orig_y, akb_header->end_x, akb_header->end_y);
		}
#else
		if (!(akb_header->flags & 0x80000000)) {
			if (MyBuildBMPFile(rgb, rgb_size, NULL, 0, width,
					height, 24, (BYTE **)&pkg_res->actual_data,
					&pkg_res->actual_data_length, my_malloc)) {
				free(rgb);
				free(akb);
				return -CUI_EMEM;
			}

			if (init_picture)
				delete [] init_picture;
			init_picture = rgb;
			//printf("setup base\n");
		} else {
			DWORD picture_len = akb_header->width * akb_header->height * 3;
			BYTE *picture = new BYTE[picture_len + 1];
			if (!picture) {
				free(rgb);
				free(akb);
				return -CUI_EMEM;			
			}

			BYTE *dst;
			if (!init_picture || _tcsncmp(pkg->name, _T("ev"), 2)) {
				//printf("no base\n");
				dst = picture;
				DWORD pixels = akb_header->width * akb_header->height;
				for (DWORD p = 0; p < pixels; ++p) {
					*(u32 *)dst = akb_header->mask;
					dst += 3;
				}
			} else {
				//printf("use base\n");
				memcpy(picture, init_picture, picture_len);
			}

			DWORD line_len = akb_header->width * 3;
			dst = picture + (akb_header->height - akb_header->end_y) * line_len + akb_header->orig_x * 3;
			BYTE *__rgb = rgb;
			for (DWORD y = akb_header->orig_y; y < akb_header->end_y; ++y) {
				for (DWORD x = akb_header->orig_x; x < akb_header->end_x; ++x) {
					DWORD bgr = __rgb[0] | __rgb[1] << 8 | __rgb[2] << 16 | (akb_header->flags & 0xff) << 24;
					if (bgr != akb_header->mask) {
						dst[0] = __rgb[0];
						dst[1] = __rgb[1];
						dst[2] = __rgb[2];
					}
					dst += 3;
					__rgb += 3;
				}
				dst += line_len - width * 3;
			}
			free(rgb);

			if (MyBuildBMPFile(picture, picture_len, NULL, 0, akb_header->width,
					akb_header->height, 24, (BYTE **)&pkg_res->actual_data,
					&pkg_res->actual_data_length, my_malloc)) {
				free(picture);
				free(akb);
				return -CUI_EMEM;
			}
			free(picture);
		}
#endif
	}

	pkg_res->raw_data = akb;
	pkg_res->raw_data_length = akb_size;

	return 0;
}

/* 封包卸载函数 */
static void AI6WIN_akb_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation AI6WIN_akb_operation = {
	AI6WIN_akb_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	AI6WIN_akb_extract_resource,/* extract_resource */
	AI6WIN_arc_save_resource,	/* save_resource */
	AI6WIN_arc_release_resource,/* release_resource */
	AI6WIN_akb_release			/* release */
};

/********************* VSD *********************/

/* 封包匹配回调函数 */
static int AI6WIN_VSD_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "VSD1", sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int AI6WIN_VSD_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	u32 VSD_size;
	u32 offset;

	if (pkg->pio->length_of(pkg, &VSD_size))
		return -CUI_ELEN;

	if (pkg->pio->readvec(pkg, &offset, 4, 4, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD mpg_len = VSD_size - (offset + sizeof(VSD_header_t));
	BYTE *mpg = (BYTE *)malloc(mpg_len);
	if (!mpg)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, mpg, mpg_len, 
			offset + sizeof(VSD_header_t), IO_SEEK_SET)) {
		free(mpg);
		return -CUI_EREADVEC;
	}

	pkg_res->raw_data = mpg;
	pkg_res->raw_data_length = mpg_len;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation AI6WIN_VSD_operation = {
	AI6WIN_VSD_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	AI6WIN_VSD_extract_resource,/* extract_resource */
	AI6WIN_arc_save_resource,	/* save_resource */
	AI6WIN_arc_release_resource,/* release_resource */
	AI6WIN_rmt_release			/* release */
};

/********************* bin *********************/

/* 封包匹配回调函数 */
static int AI6WIN_bin_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包资源提取函数 */
static int AI6WIN_bin_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	u32 bin_size;

	if (pkg->pio->length_of(pkg, &bin_size))
		return -CUI_ELEN;

	BYTE *bin = new BYTE[bin_size];
	if (!bin)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, bin, bin_size, 0, IO_SEEK_SET)) {
		delete [] bin;
		return -CUI_EREADVEC;
	}

	if (MyBuildBMPFile(bin, bin_size, NULL, 0, 800, 600, 8, 
			(BYTE **)&pkg_res->actual_data,
			&pkg_res->actual_data_length, my_malloc)) {
		delete [] bin;
		return -CUI_EMEM;
	}
	delete [] bin;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation AI6WIN_bin_operation = {
	AI6WIN_bin_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	AI6WIN_bin_extract_resource,/* extract_resource */
	AI6WIN_arc_save_resource,	/* save_resource */
	AI6WIN_arc_release_resource,/* release_resource */
	AI6WIN_rmt_release			/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK AI6WIN_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".arc"), NULL, 
		NULL, &AI6WIN_arc_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".akb"), _T(".bmp"), 
		NULL, &AI6WIN_akb_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".rmt"), _T(".bmp"), 
		NULL, &AI6WIN_rmt_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".VSD"), _T(".mpg"), 
		NULL, &AI6WIN_VSD_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".bin"), _T(".bmp"), 
		NULL, &AI6WIN_bin_operation, CUI_EXT_FLAG_RES | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	return 0;
}
