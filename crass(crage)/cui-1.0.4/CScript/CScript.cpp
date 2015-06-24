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
struct acui_information CScript_cui_information = {
	_T("Mink"),				/* copyright */
	_T("CScript(WindowsScriptHost)"),/* system */
	_T(".dat"),				/* package */
	_T("1.0.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-1-19 20:33"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	u32 index_entries;
} dat_header_t;

typedef struct {
	s8 name[44];	// 恋の恋是68
	u32 length;
	u32 offset;
} dat_entry_t;

typedef struct {
	s8 magic[4];	/* "gra" */
	u32 width;
	u32 height;
	u32 comprLen;
	u32 uncomprLen;
} gpd_header_t;

typedef struct {
	s8 magic[4];	/* "mas" */
	u32 width;
	u32 height;
	u32 comprLen;
	u32 uncomprLen;
} mas_header_t;

typedef struct {
	s8 magic[4];	/* "sce" */
	u32 reserved[2];
	u32 comprLen;
	u32 uncomprLen;
} sce_header_t;

typedef struct {
	u32 width;
	u32 height;
	u32 frames;
	u32 frame_info_length;
	u32 scale_info_uncomprlen;	// 所有帧的scale解压缩后的总长度
	u32 pixel_offset;			// 第一个差分帧压缩数据的偏移
	u32 pixel_comprlen;			// 所有差分帧的pixel压缩数据的长度
	u32 pixel_uncomprlen;		// 所有差分帧的pixel解压缩数据的长度
	u32 max_pixel_comprlen;		// 最大一个差分帧的pixel压缩数据的长度
	u32 start_pixel_uncomprlen;	// 总帧数1/3起始的那个差分帧的解压缩数据的长度
	u32 init_frame_comprlen;
} __mov_header_t;

typedef struct {
	s8 magic[4];	/* "mov" */
	__mov_header_t header;
} mov_header_t;

typedef struct {
	u32 pixel_comprlen;
	u32 pixel_uncomprlen;
	u32 scale_info_comprlen;
	u32 scale_info_uncomprlen;
	u32 unknown;
} mov_frame_t;

typedef struct {
	u32 position;
	u32 counts;
} mov_scale_t;

typedef struct {
	u32 type;		// 压缩类型: 0(脚本), 1(图像)
	u32 width;		// type0 - 0; type1 - 有效
	u32 height;		// type0 - 0; type1 - 有效
	u32 bpp;		// type0 - 0; type1 - 有效
	u32 comprlen;
	u32 uncomprlen;
} compr_header_t;
#pragma pack ()

typedef struct {
	s8 name[16];
	u32 scale_info_offset;
	u32 pixel_offset;
	u32 pixel_comprlen;
	u32 pixel_uncomprlen;
	u32 scale_info_comprlen;
	u32 scale_info_uncomprlen;
	u32 unknown;
} my_mov_frame_t;


static BYTE *init_pixel_data;
static DWORD init_pixel_data_length;

static void *my_malloc(DWORD len)
{
	return new BYTE[len];
}

#if 0	
/* 这个其实是原本的实现，但是在提取風間愛发生错误：
 * gpd1.dat: 成功提取617 / 711 个资源文件 /
 * gpd2.dat: 成功提取209 / 528 个资源文件 /
 * 所有还是得改成保守实现方式
 */
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
	while (1) {
		flag >>= 1;
		if (!(flag & 0x0100)) {
			flag = compr[curbyte++] | 0xff00;
			if (curbyte > comprlen)
				break;
		}

		if (flag & 1) {
			unsigned char data;

			data = compr[curbyte++];
			if (curbyte > comprlen)
				break;
			uncompr[act_uncomprlen++] = data;
			if (act_uncomprlen > uncomprlen)
				break;
			/* 输出的1字节放入滑动窗口 */
			win[nCurWindowByte++] = data;
			nCurWindowByte &= win_size - 1;
		} else {
			unsigned int copy_bytes, win_offset;
			unsigned int i;

			if (curbyte > comprlen)
				break;
			win_offset = compr[curbyte++];
			if (curbyte > comprlen)
				break;
			copy_bytes = compr[curbyte++];
			win_offset |= (copy_bytes & 0xf0) << 4;
			copy_bytes &= 0x0f;
			copy_bytes += 3;

			if (act_uncomprlen > uncomprlen)
				break;

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
#else
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
			if (act_uncomprlen >= uncomprlen)
				break;
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
					break;
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
#endif

/********************* dat *********************/

/* 封包匹配回调函数 */
static int CScript_dat_match(struct package *pkg)
{
	s32 temp[512 / 4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	u32 index_entries;
	if (pkg->pio->read(pkg, &index_entries, 4)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (!index_entries) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->pio->read(pkg, temp, 512)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	// 忽略文件名部分(假定文件名肯定以4字节全0做结尾)
	for (DWORD n = 1; n < 512 / 4; ++n) {
		if (!temp[n])
			break;
	}
	if (n == 512 / 4) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	// 找到第一个4字节不为0的
	for (; n < 512 / 4; ++n) {
		if (temp[n])
			break;
	}
	if (n == 512 / 4) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if ((temp[n + 1] - 4) % index_entries) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	DWORD entry_size = (temp[n + 1] - 4) / index_entries;

	// 通常不会这么大
	if (entry_size >= sizeof(temp)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->pio->readvec(pkg, temp, 512, 4 + (index_entries - 1) * entry_size, 
			IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	u32 fsize;
	pkg->pio->length_of(pkg, &fsize);

	if (fsize != (u32)temp[entry_size / 4 - 2] + (u32)temp[entry_size / 4 - 1]) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}
	package_set_private(pkg, entry_size);

	return 0;	
}

/* 封包索引目录提取函数 */
static int CScript_dat_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	if (pkg->pio->readvec(pkg, &pkg_dir->index_entries, 4, 0, IO_SEEK_SET))
		return -CUI_EREAD;

	DWORD entry_size = (DWORD)package_get_private(pkg);
	DWORD index_buffer_length = pkg_dir->index_entries * entry_size;
	BYTE *index_buffer = new BYTE[index_buffer_length];
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		delete [] index_buffer;
		return -CUI_EREAD;
	}
MySaveFile(_T("index"), index_buffer, index_buffer_length);
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = entry_size;

	return 0;
}

/* 封包索引项解析函数 */
static int CScript_dat_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	BYTE *dat_entry;

	DWORD entry_size = (DWORD)package_get_private(pkg);
	dat_entry = (BYTE *)pkg_res->actual_index_entry;
	strncpy(pkg_res->name, (char *)dat_entry, entry_size - 8);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = *(u32 *)&dat_entry[entry_size - 8];
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = *(u32 *)&dat_entry[entry_size - 4];

	return 0;
}

/* 封包资源提取函数 */
static int CScript_dat_extract_resource(struct package *pkg,
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

	gpd_header_t *gpd = (gpd_header_t *)raw;
	mas_header_t *mas = (mas_header_t *)raw;
	sce_header_t *sce = (sce_header_t *)raw;
	if (!memcmp(raw, "OggS", 4)) {
		pkg_res->raw_data = raw;
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".ogg");
		return 0;
	} else if (!strncmp(gpd->magic, "gra", 4)) {
		BYTE *uncompr = new BYTE[gpd->uncomprLen];
		if (!uncompr) {
			delete [] raw;
			return -CUI_EMEM;
		}

		BYTE *compr = (BYTE *)(gpd + 1);
		DWORD act_uncomprlen = lzss_uncompress(uncompr, gpd->uncomprLen, 
			compr, gpd->comprLen);
		if (act_uncomprlen != gpd->uncomprLen) {
			delete [] uncompr;
			delete [] raw;
			return -CUI_EMEM;
		}

		if (MyBuildBMPFile(uncompr, act_uncomprlen, NULL, 0, gpd->width, 
				gpd->height, 24, (BYTE **)&pkg_res->actual_data, 
				&pkg_res->actual_data_length, my_malloc)) {
			delete [] uncompr;
			delete [] raw;
			return -CUI_EMEM;			
		}
		delete [] uncompr;
		delete [] raw;
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp");
		return 0;
	} else if (!strncmp(mas->magic, "mas", 4)) {
		BYTE *uncompr = new BYTE[mas->uncomprLen];
		if (!uncompr) {
			delete [] raw;
			return -CUI_EMEM;
		}

		BYTE *compr = (BYTE *)(mas + 1);
		DWORD act_uncomprlen = lzss_uncompress(uncompr, mas->uncomprLen, 
			compr, mas->comprLen);
		if (act_uncomprlen != mas->uncomprLen) {
			delete [] uncompr;
			delete [] raw;
			return -CUI_EMEM;
		}

		if (MyBuildBMPFile(uncompr, act_uncomprlen, NULL, 0, gpd->width, 
				gpd->height, 8, (BYTE **)&pkg_res->actual_data, 
				&pkg_res->actual_data_length, my_malloc)) {
			delete [] uncompr;
			delete [] raw;
			return -CUI_EMEM;			
		}
		delete [] uncompr;
		delete [] raw;
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp");
		return 0;
	} else if (!strncmp(sce->magic, "sce", 4)) {
		BYTE *uncompr = new BYTE[sce->uncomprLen];
		if (!uncompr) {
			delete [] raw;
			return -CUI_EMEM;
		}

		BYTE *compr = (BYTE *)(sce + 1);
		DWORD act_uncomprlen = lzss_uncompress(uncompr, sce->uncomprLen, 
			compr, sce->comprLen);
		if (act_uncomprlen != mas->uncomprLen) {
			delete [] uncompr;
			delete [] raw;
			return -CUI_EMEM;
		}
		delete [] raw;
		pkg_res->actual_data = uncompr;
		pkg_res->actual_data_length = act_uncomprlen;
		return 0;
	}

	compr_header_t *ch = (compr_header_t *)raw;
	DWORD uncomprlen = ch->uncomprlen;
	DWORD comprlen = ch->comprlen;
	BYTE *uncompr;
	if (!ch->type && !ch->width && !ch->height && !ch->bpp) {
		BYTE *compr = raw + sizeof(compr_header_t);
		uncompr = new BYTE[uncomprlen];
		if (!uncompr) {
			delete [] raw;
			return -CUI_EMEM;
		}

		if (lzss_uncompress(uncompr, uncomprlen, compr, comprlen) != uncomprlen) {
			delete [] uncompr;
			delete [] raw;
			return -CUI_EUNCOMPR;
		}
		delete [] raw;
		pkg_res->actual_data = uncompr;
		pkg_res->actual_data_length = uncomprlen;
	} else if ((ch->type == 1) && ch->width && ch->height && ch->bpp) {
		BYTE *compr = raw + sizeof(compr_header_t);
		uncompr = new BYTE[uncomprlen];
		if (!uncompr) {
			delete [] raw;
			return -CUI_EMEM;
		}

		if (lzss_uncompress(uncompr, uncomprlen, compr, comprlen) != uncomprlen) {
			delete [] uncompr;
			delete [] raw;
			return -CUI_EUNCOMPR;
		}

		if (MyBuildBMPFile(uncompr, uncomprlen, NULL, 0,
				ch->width, ch->height, ch->bpp, (BYTE **)&pkg_res->actual_data,
				&pkg_res->actual_data_length, my_malloc)) {
			delete [] uncompr;
			delete [] raw;
			return -CUI_EMEM;
		}
		delete [] uncompr;
		delete [] raw;
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp");
	} else if (!_tcscmp(pkg->name, _T("mov.dat"))) {
		pkg_res->raw_data = raw;
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".mov");
	} else
		pkg_res->raw_data = raw;

	return 0;
}

/* 资源保存函数 */
static int CScript_dat_save_resource(struct resource *res, 
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
static void CScript_dat_release_resource(struct package *pkg, 
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
static void CScript_dat_release(struct package *pkg, 
							 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation CScript_dat_operation = {
	CScript_dat_match,					/* match */
	CScript_dat_extract_directory,		/* extract_directory */
	CScript_dat_parse_resource_info,	/* parse_resource_info */
	CScript_dat_extract_resource,		/* extract_resource */
	CScript_dat_save_resource,			/* save_resource */
	CScript_dat_release_resource,		/* release_resource */
	CScript_dat_release				/* release */
};

/********************* mov *********************/

/* 封包匹配回调函数 */
static int CScript_mov_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "mov", 4)) {
		if (pkg->pio->seek(pkg, 0, IO_SEEK_SET)) {
			pkg->pio->close(pkg);
			return -CUI_ESEEK;
		}
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int CScript_mov_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	u32 old_mov_offset;
	pkg->pio->locate(pkg, &old_mov_offset);

	__mov_header_t mov_header;
	if (pkg->pio->read(pkg, &mov_header, sizeof(mov_header)))
		return -CUI_EREAD;

	mov_frame_t *frame_info = new mov_frame_t[mov_header.frames];
	if (!frame_info)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, frame_info, mov_header.frame_info_length)) {
		delete [] frame_info;
		return -CUI_EREAD;
	}

	my_mov_frame_t *my_frame_info = new my_mov_frame_t[mov_header.frames + 1];
	if (!my_frame_info) {
		delete [] frame_info;
		return -CUI_EMEM;
	}

	DWORD offset = old_mov_offset + sizeof(mov_header) + mov_header.frame_info_length;
	for (DWORD i = 0; i < mov_header.frames; ++i) {
		sprintf(my_frame_info[i + 1].name, "%06d", i + 1);
		my_frame_info[i + 1].scale_info_offset = offset;
		offset += frame_info[i].scale_info_comprlen;
		my_frame_info[i + 1].pixel_comprlen = frame_info[i].pixel_comprlen;
		my_frame_info[i + 1].pixel_uncomprlen = frame_info[i].pixel_uncomprlen;
		my_frame_info[i + 1].scale_info_comprlen = frame_info[i].scale_info_comprlen;
		my_frame_info[i + 1].scale_info_uncomprlen = frame_info[i].scale_info_uncomprlen;
		my_frame_info[i + 1].unknown = frame_info[i].unknown;
	}
	delete [] frame_info;

	strcpy(my_frame_info[0].name, "000000");
	my_frame_info[0].scale_info_offset = 0;
	my_frame_info[0].pixel_offset = offset;
	my_frame_info[0].pixel_comprlen = mov_header.init_frame_comprlen;
	my_frame_info[0].pixel_uncomprlen = mov_header.width * mov_header.height * 3;
	my_frame_info[0].scale_info_comprlen = 0;
	my_frame_info[0].scale_info_uncomprlen = 0;
	my_frame_info[0].unknown = 0;

	offset = mov_header.pixel_offset;
	for (i = 1; i < mov_header.frames + 1; ++i) {
		my_frame_info[i].pixel_offset = offset;
		offset += my_frame_info[i].pixel_comprlen;
	}
	pkg_dir->index_entries = mov_header.frames + 1;
	pkg_dir->directory = my_frame_info;
	pkg_dir->directory_length = (mov_header.frames + 1) * sizeof(my_mov_frame_t);
	pkg_dir->index_entry_length = sizeof(my_mov_frame_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	__mov_header_t *__mov_header = new __mov_header_t;
	if (!__mov_header) {
		delete [] my_frame_info;
		return -CUI_EMEM;
	}
	memcpy(__mov_header, &mov_header, sizeof(__mov_header_t));
	package_set_private(pkg, __mov_header);

	return 0;
}

/* 封包索引项解析函数 */
static int CScript_mov_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	my_mov_frame_t *mov_frame;

	mov_frame = (my_mov_frame_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, mov_frame->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = mov_frame->pixel_comprlen;
	pkg_res->actual_data_length = mov_frame->pixel_uncomprlen;
	pkg_res->offset = mov_frame->pixel_offset;

	return 0;
}

/* 封包资源提取函数 */
static int CScript_mov_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	BYTE *compr = new BYTE[pkg_res->raw_data_length];
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			delete [] compr;
			return -CUI_EREADVEC;
	}

	BYTE *uncompr = new BYTE[pkg_res->actual_data_length + 1];
	if (!uncompr) {
		delete [] compr;
		return -CUI_EMEM;
	}

	if (lzss_uncompress(uncompr, pkg_res->actual_data_length, 
			compr, pkg_res->raw_data_length) != pkg_res->actual_data_length) {
		delete [] uncompr;
		delete [] compr;
		return -CUI_EUNCOMPR;
	}
	delete [] compr;

	__mov_header_t *mov_header = (__mov_header_t *)package_get_private(pkg);
	if (!strcmp(pkg_res->name, "000000")) {
		init_pixel_data_length = pkg_res->actual_data_length;
		if (MyBuildBMPFile(uncompr, init_pixel_data_length, NULL, 0,
				mov_header->width, mov_header->height, 24, (BYTE **)&pkg_res->actual_data,
				&pkg_res->actual_data_length, my_malloc)) {
			delete [] uncompr;
			return -CUI_EMEM;
		}

		init_pixel_data = new BYTE[init_pixel_data_length];
		if (!init_pixel_data) {
			delete [] pkg_res->actual_data;
			delete [] uncompr;
			return -CUI_EMEM;			
		}
		memcpy(init_pixel_data, uncompr, init_pixel_data_length);
		delete [] uncompr;
	} else {
		my_mov_frame_t *mov_frame = (my_mov_frame_t *)pkg_res->actual_index_entry;
		BYTE *scale_info_compr = new BYTE[mov_frame->scale_info_comprlen];
		if (!scale_info_compr) {
			delete [] uncompr;
			return -CUI_EMEM;
		}

		if (pkg->pio->readvec(pkg, scale_info_compr, mov_frame->scale_info_comprlen,
			mov_frame->scale_info_offset, IO_SEEK_SET)) {
				delete [] scale_info_compr;
				delete [] uncompr;
				return -CUI_EREADVEC;
		}

		BYTE *scale_info_uncompr = new BYTE[mov_frame->scale_info_uncomprlen];
		if (!scale_info_uncompr) {
			delete [] scale_info_compr;
			delete [] uncompr;
			return -CUI_EMEM;
		}

		if (lzss_uncompress(scale_info_uncompr, mov_frame->scale_info_uncomprlen, 
				scale_info_compr, mov_frame->scale_info_comprlen) 
				!= mov_frame->scale_info_uncomprlen) {
			delete [] scale_info_uncompr;
			delete [] scale_info_compr;
			delete [] uncompr;
			return -CUI_EUNCOMPR;
		}
		delete [] scale_info_compr;

		mov_scale_t *scale_index = (mov_scale_t *)scale_info_uncompr;
		DWORD scales = mov_frame->scale_info_uncomprlen / sizeof(mov_scale_t);
		BYTE *delta = uncompr;
		for (DWORD i = 0; i < scales; ++i) {
			memcpy(&init_pixel_data[scale_index[i].position], delta, scale_index[i].counts);
			delta += scale_index[i].counts;			
		}
		delete [] scale_info_uncompr;
		delete [] uncompr;

		if (MyBuildBMPFile(init_pixel_data, init_pixel_data_length, NULL, 0,
				mov_header->width, mov_header->height, 24, (BYTE **)&pkg_res->actual_data,
				&pkg_res->actual_data_length, my_malloc))
			return -CUI_EMEM;
	}

	return 0;
}

/* 封包卸载函数 */
static void CScript_mov_release(struct package *pkg, 
							 struct package_directory *pkg_dir)
{
	void *priv = (void *)package_get_private(pkg);
	if (priv) {
		delete [] priv;
		package_set_private(pkg, NULL);
	}

	if (init_pixel_data) {
		delete [] init_pixel_data;
		init_pixel_data = NULL;
	}
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}


/* 封包处理回调函数集合 */
static cui_ext_operation CScript_mov_operation = {
	CScript_mov_match,					/* match */
	CScript_mov_extract_directory,		/* extract_directory */
	CScript_mov_parse_resource_info,	/* parse_resource_info */
	CScript_mov_extract_resource,		/* extract_resource */
	CScript_dat_save_resource,			/* save_resource */
	CScript_dat_release_resource,		/* release_resource */
	CScript_mov_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK CScript_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &CScript_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".mov"), _T(".bmp"), 
		NULL, &CScript_mov_operation, CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
