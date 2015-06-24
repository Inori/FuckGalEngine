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
struct acui_information Willadv_cui_information = {
	_T("WILL"),				/* copyright */
	_T("Willadv"),			/* system */
	_T(".arc"),				/* package */
	_T("1.0.2"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-7-12 15:35"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	u32 types;
} arc_header_t;

typedef struct {
	s8 suffix[4];
	u32 index_entries;		
	u32 index_offset;
} arc_info_t;

typedef struct {
	s8 name[9];
	u32 length;
	u32 offset;
} arc_entry_t;

typedef struct {
	s8 magic[4];	/* "WIPF" */
	u16 files;
	u16 color_depth;
} wipf_header_t;

typedef struct {
	u32 width;
	u32 height;
	u32 orig_x;		/* 左上角为顶点 */
	u32 orig_y;
	u32 orig_z;		// 想象一下Will旗下的制作3D游戏的公司
	u32 length;
} wipf_entry_t;

typedef struct {
	u32 width;
	u32 height;
	u32 comprlen;
} fil_header_t;
#pragma pack ()

typedef struct {
	s8 name[16];
	u32 length;
	u32 offset;
} my_arc_entry_t;

typedef struct {
	TCHAR name[32];
	wipf_entry_t wipf_entry;
	u32 offset;
	u16 color_depth;
} my_wipf_entry_t;


static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static inline unsigned char getbit_le(unsigned char byte, unsigned int pos)
{
	return !!(byte & (1 << pos));
}

static void lzss_decompress(BYTE *uncompr, DWORD uncomprlen, 
							 BYTE *compr, DWORD comprlen)
{
	DWORD act_uncomprlen = 0;
	/* compr中的当前扫描字节 */
	DWORD curbyte = 0;
	DWORD nCurWindowByte = 1;
	DWORD win_size = 4096;
	BYTE win[4096];

	memset(win, 0, sizeof(win));
	while (1) {
		unsigned int curbit;
		BYTE flag;

		flag = compr[curbyte++];
		for (curbit = 0; curbit < 8; curbit++) {
			/* 如果为1, 表示接下来的1个字节原样输出 */
			if (getbit_le(flag, curbit)) {
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

				if (!win_offset && !copy_bytes)
					return;
				
				win_offset <<= 4;
				win_offset |= copy_bytes >> 4;

				copy_bytes &= 0x0f;
				copy_bytes += 2;

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
}

static void rgba_swap(BYTE *buf, BYTE *dib_buf, DWORD width, DWORD height, DWORD cdepth)
{
	BYTE *bb = buf;
	BYTE *gb = bb + width * height;
	BYTE *rb = gb + width * height;
	DWORD line_len = width * cdepth / 8;
	unsigned int k = 0;

	for (unsigned int y = 0; y < height; y++) {
		for (unsigned int x = 0; x < width; x++) {
			dib_buf[y * line_len + x * cdepth / 8 + 0] = bb[k];
			dib_buf[y * line_len + x * cdepth / 8 + 1] = gb[k];
			dib_buf[y * line_len + x * cdepth / 8 + 2] = rb[k++];
		}
	}
}

/********************* arc *********************/

/* 封包匹配回调函数 */
static int Willadv_arc_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包索引目录提取函数 */
static int Willadv_arc_extract_directory(struct package *pkg,
										 struct package_directory *pkg_dir)
{
	arc_header_t arc_header;
	arc_info_t *arc_info;
	arc_entry_t *index_buffer;
	my_arc_entry_t *my_index_buffer;
	DWORD index_entries, index_buffer_len, my_index_buffer_len;
	DWORD i;

	if (pkg->pio->read(pkg, &arc_header, sizeof(arc_header)))
		return -CUI_EREAD;

	arc_info = (arc_info_t *)malloc(arc_header.types * sizeof(arc_info_t));
	if (!arc_info)
		return -CUI_EMEM;		

	if (pkg->pio->read(pkg, arc_info, arc_header.types * sizeof(arc_info_t))) {
		free(arc_info);
		return -CUI_EREAD;
	}

	index_entries = 0;
	for (i = 0; i < arc_header.types; i++)
		index_entries += arc_info[i].index_entries;

	index_buffer_len = index_entries * sizeof(arc_entry_t);
	index_buffer = (arc_entry_t *)malloc(index_buffer_len);
	if (!index_buffer) {
		free(arc_info);
		return -CUI_EREAD;
	}

	my_index_buffer_len = index_entries * sizeof(my_arc_entry_t);
	my_index_buffer = (my_arc_entry_t *)malloc(my_index_buffer_len);
	if (!my_index_buffer) {
		free(index_buffer);
		free(arc_info);
		return -CUI_EMEM;
	}	

	int ret = 0;
	DWORD k = 0;
	for (i = 0; i < arc_header.types; i++) {
		if (pkg->pio->seek(pkg, arc_info[i].index_offset, IO_SEEK_SET)) {
			ret = -CUI_ESEEK;
			break;
		}

		if (pkg->pio->read(pkg, &index_buffer[k], arc_info[i].index_entries * sizeof(arc_entry_t))) {
			ret = -CUI_EREAD;
			break;
		}

		for (DWORD n = 0; n < arc_info[i].index_entries; n++) {
			memset(my_index_buffer[k + n].name, 0, sizeof(my_index_buffer[n].name));
			strncpy(my_index_buffer[k + n].name, index_buffer[k + n].name, 9);
			strcat(my_index_buffer[k + n].name, ".");
			strncat(my_index_buffer[k + n].name, arc_info[i].suffix, 4);
			my_index_buffer[k + n].offset = index_buffer[k + n].offset;
			my_index_buffer[k + n].length = index_buffer[k + n].length;
		}
		
		k += arc_info[i].index_entries;
	}
	free(index_buffer);
	free(arc_info);
	if (i != arc_header.types) {
		free(index_buffer);
		return ret;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = my_index_buffer_len;
	pkg_dir->index_entry_length = sizeof(my_arc_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int Willadv_arc_parse_resource_info(struct package *pkg,
										   struct package_resource *pkg_res)
{
	my_arc_entry_t *my_arc_entry;

	my_arc_entry = (my_arc_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_arc_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_arc_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_arc_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int Willadv_arc_extract_resource(struct package *pkg,
										struct package_resource *pkg_res)
{
	wipf_header_t *wipf_header;

	pkg_res->raw_data = malloc(pkg_res->raw_data_length);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EREADVEC;
	}

	wipf_header = (wipf_header_t *)pkg_res->raw_data;
	if (!strncmp((char *)pkg_res->raw_data, "WIPF", 4) && (wipf_header->files == 1)) {
		wipf_entry_t *wipf_entry;
		DWORD line_len, uncomprlen, comprlen, palette_length;
		BYTE *compr, *uncompr, *palette, *dib_buf;

		wipf_entry = (wipf_entry_t *)(wipf_header + 1);
		compr = (BYTE *)(wipf_entry + 1);
		comprlen = pkg_res->raw_data_length - sizeof(wipf_header_t) - sizeof(wipf_entry_t);
		line_len = (wipf_entry->width * wipf_header->color_depth / 8 + 3) & ~3;
		uncomprlen = line_len * wipf_entry->height;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EMEM;
		}
							
		if (wipf_header->color_depth == 8) {
			palette = compr;
			palette_length = 0x400;
		} else {
			palette = NULL;
			palette_length = 0;
		}
		compr += palette_length;
		comprlen -= palette_length;
		lzss_decompress(uncompr, uncomprlen, compr, comprlen);

		if (wipf_header->color_depth != 8) {
			dib_buf = (BYTE *)malloc(uncomprlen);
			if (!dib_buf) {
				free(uncompr);
				free(pkg_res->raw_data);
				pkg_res->raw_data = NULL;
				return -CUI_EMEM;
			}

			rgba_swap(uncompr, dib_buf, wipf_entry->width, wipf_entry->height, wipf_header->color_depth);
			free(uncompr);
		} else
			dib_buf = uncompr;

		if (MyBuildBMPFile(dib_buf, uncomprlen, palette, palette_length, wipf_entry->width, 
				0 - wipf_entry->height,	wipf_header->color_depth, 
					(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, my_malloc)) {
			free(dib_buf);
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EMEM;
		}
		free(dib_buf);
		pkg_res->replace_extension = _T(".bmp");
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
	} else if (strstr(pkg_res->name, ".WSC") || strstr(pkg_res->name, ".SCR")) {
		for (DWORD i = 0; i < pkg_res->raw_data_length; i++)
			((BYTE *)pkg_res->raw_data)[i] = 
				(((BYTE *)pkg_res->raw_data)[i] >> 2) | (((BYTE *)pkg_res->raw_data)[i] & 3) << 6;
	} else if (strstr(pkg_res->name, ".FIL")) {
		fil_header_t *fil_head = (fil_header_t *)pkg_res->raw_data;
		DWORD uncomprlen = fil_head->width * fil_head->height * 1;
		BYTE *uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EMEM;
		}
		lzss_decompress(uncompr, uncomprlen, (BYTE *)(fil_head + 1), fil_head->comprlen);
		if (MySaveAsBMP(uncompr, uncomprlen, NULL, 0, fil_head->width, 
				0 - fil_head->height, 1, 2, (BYTE **)&pkg_res->actual_data, 
				&pkg_res->actual_data_length, my_malloc)) {
			free(uncompr);
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EMEM;
		}
		free(uncompr);
		pkg_res->replace_extension = _T(".bmp");
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
	}

	return 0;
}

/* 资源保存函数 */
static int Willadv_arc_save_resource(struct resource *res, 
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
static void Willadv_arc_release_resource(struct package *pkg, 
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
static void Willadv_arc_release(struct package *pkg, 
								struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation Willadv_arc_operation = {
	Willadv_arc_match,					/* match */
	Willadv_arc_extract_directory,		/* extract_directory */
	Willadv_arc_parse_resource_info,	/* parse_resource_info */
	Willadv_arc_extract_resource,		/* extract_resource */
	Willadv_arc_save_resource,			/* save_resource */
	Willadv_arc_release_resource,		/* release_resource */
	Willadv_arc_release					/* release */
};


/********************* wipf *********************/

/* 封包匹配回调函数 */
static int Willadv_wipf_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "WIPF", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int Willadv_wipf_extract_directory(struct package *pkg,
										  struct package_directory *pkg_dir)
{
	wipf_entry_t *index_buffer;
	my_wipf_entry_t *my_index_buffer;
	u16 index_entries, color_depth;
	DWORD index_buffer_len, my_index_buffer_len, palette_length;

	if (pkg->pio->read(pkg, &index_entries, sizeof(index_entries)))
		return -CUI_EREAD;

	if (pkg->pio->read(pkg, &color_depth, sizeof(color_depth)))
		return -CUI_EREAD;

	index_buffer_len = index_entries * sizeof(wipf_entry_t);
	index_buffer = (wipf_entry_t *)malloc(index_buffer_len);
	if (!index_buffer)
		return -CUI_EMEM;		

	if (pkg->pio->read(pkg, index_buffer, index_buffer_len)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	my_index_buffer_len = index_entries * sizeof(my_wipf_entry_t);
	my_index_buffer = (my_wipf_entry_t *)malloc(my_index_buffer_len);
	if (!my_index_buffer) {
		free(index_buffer);
		return -CUI_EMEM;
	}

	palette_length = color_depth == 8 ? 0x400 : 0;

	DWORD offset = sizeof(wipf_header_t) + index_buffer_len;
	for (DWORD i = 0; i < index_entries; i++) {
		_stprintf(my_index_buffer[i].name, _T("%s.%04d.bmp"), pkg->name, i);
		my_index_buffer[i].color_depth = color_depth;
		my_index_buffer[i].offset = offset;
		my_index_buffer[i].wipf_entry = index_buffer[i];
		offset += index_buffer[i].length + palette_length;
	}
	free(index_buffer);

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = my_index_buffer_len;
	pkg_dir->index_entry_length = sizeof(my_wipf_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int Willadv_wipf_parse_resource_info(struct package *pkg,
											struct package_resource *pkg_res)
{
	my_wipf_entry_t *my_wipf_entry;

	my_wipf_entry = (my_wipf_entry_t *)pkg_res->actual_index_entry;
	_tcscpy((TCHAR *)pkg_res->name, my_wipf_entry->name);
	pkg_res->flags |= PKG_RES_FLAG_UNICODE;
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_wipf_entry->wipf_entry.length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_wipf_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int Willadv_wipf_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	BYTE *uncompr, *compr;
	my_wipf_entry_t *my_wipf_entry;
	wipf_entry_t *wipf_entry;
	DWORD line_len, uncomprlen, palette_length;
	BYTE *dib_buf, *palette;

	my_wipf_entry = (my_wipf_entry_t *)pkg_res->actual_index_entry;
	wipf_entry = &my_wipf_entry->wipf_entry;
	if (my_wipf_entry->color_depth == 8) {
		palette = (BYTE *)malloc(0x400);
		if (!palette)
			return -CUI_EMEM;

		if (pkg->pio->readvec(pkg, palette, 0x400, pkg_res->offset, IO_SEEK_SET)) {
			free(palette);
			return -CUI_EREADVEC;
		}

		palette_length = 0x400;
	} else {
		palette = NULL;
		palette_length = 0;
	}

	compr = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!compr) {
		if (palette)
			free(palette);
		return -CUI_EMEM;
	}

	if (pkg->pio->readvec(pkg, compr, pkg_res->raw_data_length, pkg_res->offset + palette_length, IO_SEEK_SET)) {
		free(compr);
		if (palette)
			free(palette);
		return -CUI_EREADVEC;
	}

	line_len = (wipf_entry->width * my_wipf_entry->color_depth / 8 + 3) & ~3;
	uncomprlen = line_len * wipf_entry->height;
	uncompr = (BYTE *)malloc(uncomprlen);
	if (!uncompr) {
		free(compr);
		if (palette)
			free(palette);
		return -CUI_EMEM;
	}

	lzss_decompress(uncompr, uncomprlen, compr, pkg_res->raw_data_length);

	free(compr);

	if (my_wipf_entry->color_depth != 8) {
		dib_buf = (BYTE *)malloc(uncomprlen);
		if (!dib_buf) {
			free(uncompr);
			if (palette)
				free(palette);
			return -CUI_EMEM;
		}

		rgba_swap(uncompr, dib_buf, wipf_entry->width, wipf_entry->height, my_wipf_entry->color_depth);
		free(uncompr);
	} else
		dib_buf = uncompr;

	if (MyBuildBMPFile(dib_buf, uncomprlen, palette, palette_length, wipf_entry->width, 
			0 - wipf_entry->height,	my_wipf_entry->color_depth, 
				(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, my_malloc)) {
		free(dib_buf);
		if (palette)
			free(palette);
		return -CUI_EMEM;
	}
	free(dib_buf);
	if (palette)
		free(palette);

	return 0;
}

/* 资源保存函数 */
static int Willadv_wipf_save_resource(struct resource *res, 
									  struct package_resource *pkg_res)
{
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (pkg_res->actual_data && pkg_res->actual_data_length) {
		if (res->rio->write(res, pkg_res->actual_data, pkg_res->actual_data_length)) {
			res->rio->close(res);
			return -CUI_EWRITE;
		}
	}

	res->rio->close(res);
	
	return 0;
}

/* 封包资源释放函数 */
static void Willadv_wipf_release_resource(struct package *pkg, 
										  struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
}

/* 封包卸载函数 */
static void Willadv_wipf_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation Willadv_wipf_operation = {
	Willadv_wipf_match,					/* match */
	Willadv_wipf_extract_directory,		/* extract_directory */
	Willadv_wipf_parse_resource_info,	/* parse_resource_info */
	Willadv_wipf_extract_resource,		/* extract_resource */
	Willadv_wipf_save_resource,			/* save_resource */
	Willadv_wipf_release_resource,		/* release_resource */
	Willadv_wipf_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK Willadv_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".arc"), NULL, 
		NULL, &Willadv_arc_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".WIP"), _T(".WIP.bmp"), 
		NULL, &Willadv_wipf_operation, CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".MSK"), _T(".MSK.bmp"), 
		NULL, &Willadv_wipf_operation, CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
