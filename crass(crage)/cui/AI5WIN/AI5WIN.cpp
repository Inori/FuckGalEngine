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
#include <cui_common.h>

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information AI5WIN_cui_information = {
	_T("elf"),				/* copyright */
	_T(""),					/* system */
	_T(".arc .VSD"),		/* package */
	_T("1.0.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-3-29 19:19"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	u32 index_entries;
} arc_header_t;

typedef struct {
	s8 magic[4];		// "MMO "
	u32 orig_x;
	u32 orig_y;
	u32 width;
	u32 height;
	u32 orig_x1;
	u32 orig_y1;
	u32 width1;
	u32 height1;
	u32 alpha_offset;
} mmo_header_t;

typedef struct {
	s8 magic[4];		// "G24n", "G24m", "R24n", "R24m"
	u16 orig_x;			// 图像在显示buffer中的原点x坐标
	u16 orig_y;			// 图像在显示buffer中的原点y坐标
	u16 width;			// 图像宽度
	u16 height;			// 图像高度
	u32 offset;
	u32 length;
} gcc_header_t;

typedef struct {
	s8 magic[4];		// "VSD1"
	u32 offset;
} VSD_header_t;
#pragma pack ()


static const char *game_id;

struct AI5WIN_game_config {
	const TCHAR *name;
	u32 name_length;
	u32 name_xor;
	u32 length_xor;
	u32 offset_xor;
};

struct AI5WIN_game_config brute_config = {
	_T("brute guessing"),
};

struct AI5WIN_game_config DK4_config = {
	_T("DK4"), 256, 0x29, 0x1663E1E9, 0x1BB6625C
};

struct AI5WIN_game_config SHANGRLIA_config = {
	_T("SHANGRLIA"), 32, 0x5f, 0x46831582, 0x17528913
};

struct AI5WIN_game_config LIME2_config = {
	_T("LIME2"), 32, 0x03, 0x14211597, 0x10429634
};

struct AI5WIN_game_config *game_list[] = {
	&brute_config,
	&DK4_config,
	&SHANGRLIA_config,
	&LIME2_config,
	NULL
};


static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static DWORD lzss_uncompress2(BYTE *uncompr, DWORD uncomprlen, 
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
					return act_uncomprlen;
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

	return act_uncomprlen;
}

#if 0
static DWORD gcc_uncompress(gcc_header_t *gcc_header, BYTE *compr)
{
	DWORD i = 0;
	DWORD uncomprlen = gcc_header->width * gcc_header->height * 3;
	DWORD act_uncomprlen = 0;
	BYTE *flag = compr;
	compr += gcc_header->length;
	while (act_uncomprlen != uncomprlen) {
		cur_uncomprlen = uncomprlen - act_uncomprlen;
		if (uncomprlen - act_uncomprlen >= 0xFFFF)
			cur_uncomprlen = 0xFFFF;

		if ((1 << (i & 0x1F)) & flag[4 * (i >> 5)]) {
			compr = sub_446E50(_this, compr, (int)&v12, cur_uncomprlen + 2);
			sub_446BB0(&v13, cur_uncomprlen, v12, a3);
		} else
			compr = sub_447170(_this, compr, cur_uncomprlen, a3);
		act_uncomprlen += cur_uncomprlen;
	}

	return uncomprlen;
}
#endif
/********************* arc *********************/

/* 封包匹配回调函数 */
static int AI5WIN_arc_match(struct package *pkg)
{
	u32 index_entries;
	const TCHAR *game_id;
	struct AI5WIN_game_config **cfg;

	game_id = get_options2(_T("game"));
	if (game_id) {
		for (cfg = game_list; *cfg; cfg++) {
			if (!_tcsicmp((*cfg)->name, game_id))
				break;
		}
		if (!*cfg)
			cfg = game_list;
	} else
		cfg = game_list;

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

	if (!_tcsicmp((*cfg)->name, _T("brute guessing"))) {
		u32 arc_size;

		if (pkg->pio->length_of(pkg, &arc_size)) {
			pkg->pio->close(pkg);
			return -CUI_ELEN;
		}

		for (int nlen = 8; ; nlen++) {
			DWORD entry_size = nlen + 8;
			DWORD index_len = entry_size * index_entries;
			BYTE *index = (BYTE *)malloc(index_len);
			if (!index) {
				pkg->pio->close(pkg);
				return -CUI_EMEM;	
			}

			if (pkg->pio->readvec(pkg, index, index_len, 4, IO_SEEK_SET)) {
				free(index);
				pkg->pio->close(pkg);
				return -CUI_EREAD;
			}

			DWORD xor_offset = 0;
			DWORD xor_length = 0;
			DWORD pre_data_offset, pre_data_length;
			for (DWORD i = 0; i < index_entries; i++) {
				u32 data_length = *(u32 *)&index[entry_size * i + nlen];
				u32 data_offset = *(u32 *)&index[entry_size * i + nlen + 4];

				if (!i) {
					xor_offset = data_offset ^ (4 + index_len);
					data_offset = 4 + index_len;
				} else {
					data_offset ^= xor_offset;
					if (data_offset > arc_size)
						break;

					if (i == 1) {
						xor_length = pre_data_length ^ (data_offset - pre_data_offset);
						pre_data_length = data_offset - pre_data_offset;
					} else
						pre_data_length ^= xor_length;

					if (pre_data_offset + pre_data_length != data_offset)
						break;
				}
				pre_data_offset = data_offset;
				pre_data_length = data_length;
			}
			if (i == index_entries) {
				(*cfg)->length_xor = xor_length;
				(*cfg)->name_length = nlen;
				(*cfg)->name_xor = index[nlen - 1];
				(*cfg)->offset_xor = xor_offset;
				free(index);
				break;
			}
			free(index);
		}		
	}

	package_set_private(pkg, *cfg);

	return 0;	
}

/* 封包索引目录提取函数 */
static int AI5WIN_arc_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	arc_header_t arc_header;
	struct AI5WIN_game_config *cfg;
	DWORD arc_entry_size;

	if (pkg->pio->readvec(pkg, &arc_header, sizeof(arc_header), 
			0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	cfg = (struct AI5WIN_game_config *)package_get_private(pkg);

	arc_entry_size = cfg->name_length + 8;
	DWORD index_buffer_length = arc_header.index_entries * arc_entry_size;
	BYTE *index_buffer = (BYTE *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	BYTE *entry = index_buffer;
	for (DWORD i = 0; i < arc_header.index_entries; i++) {
		for (DWORD k = 0; k < cfg->name_length; k++)
			entry[k] ^= cfg->name_xor;
		entry += cfg->name_length;
		*(u32 *)entry ^= cfg->length_xor;
		entry += 4;
		*(u32 *)entry ^= cfg->offset_xor;
		entry += 4;
	}

	pkg_dir->index_entries = arc_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = arc_entry_size;

	return 0;
}

/* 封包索引项解析函数 */
static int AI5WIN_arc_parse_resource_info(struct package *pkg,
										  struct package_resource *pkg_res)
{
	struct AI5WIN_game_config *cfg;
	BYTE *arc_entry;

	cfg = (struct AI5WIN_game_config *)package_get_private(pkg);
	arc_entry = (BYTE *)pkg_res->actual_index_entry;
	strncpy(pkg_res->name, (char *)arc_entry, cfg->name_length);
	arc_entry += cfg->name_length;
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = *(u32 *)arc_entry;
	arc_entry += 4;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = *(u32 *)arc_entry;

	return 0;
}

/* 封包资源提取函数 */
static int AI5WIN_arc_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	BYTE *raw;

	raw = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length,
			pkg_res->offset, IO_SEEK_SET)) {
		free(raw);
		return -CUI_EREADVEC;
	}

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = raw;
		return 0;
	}

	char *ext = PathFindExtensionA(pkg_res->name);
	if (
		!strcmpi(ext, ".3DM") || !strcmpi(ext, ".A6") 
		|| !strcmpi(ext, ".AX") || !strcmpi(ext, ".BIN") 
		|| !strcmpi(ext, ".CHR") || !strcmpi(ext, ".CSV") 
		|| !strcmpi(ext, ".DAT") || !strcmpi(ext, ".LIB") 
		|| !strcmpi(ext, ".MAM") || !strcmpi(ext, ".MAP")
		|| !strcmpi(ext, ".MES") || !strcmpi(ext, ".MP")
		|| !strcmpi(ext, ".MSK") || !strcmpi(ext, ".SC")
		|| !strcmpi(ext, ".SEQ") || !strcmpi(ext, ".STAGE") 			
		|| !strcmpi(ext, ".X")  
	) {
	//	DWORD uncomprlen = (280000 + 0xffff) & ~0xffff;	// for .mes
		DWORD uncomprlen = pkg_res->raw_data_length * 10;
		BYTE *uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(raw);
			return -CUI_EMEM;
		}
		uncomprlen = lzss_uncompress(uncompr, uncomprlen, raw, 
			pkg_res->raw_data_length);
		pkg_res->actual_data = uncompr;
		pkg_res->actual_data_length = uncomprlen;
	} else if (!strcmpi(ext, ".MMO") && !strncmp((char *)raw, "MMO " ,4)) {
		mmo_header_t *mmo_header = (mmo_header_t *)raw;
		DWORD width = mmo_header->width - mmo_header->orig_x;
		DWORD height = mmo_header->height - mmo_header->orig_y;
		DWORD rgba_size = width * height * 3;
		BYTE *rgba = (BYTE *)malloc(rgba_size);
		if (!rgba) {
			free(raw);
			return -CUI_EMEM;
		}
		lzss_uncompress2(rgba, 
			rgba_size, (BYTE *)(mmo_header + 1), 
			pkg_res->raw_data_length - sizeof(mmo_header_t));

		BYTE *p = rgba + 3;
		for (DWORD x = mmo_header->orig_x + 1; x < mmo_header->width; x++) {
			p[0] += p[-3];
			p[1] += p[-2];
			p[2] += p[-1];
			p += 3;
		}

		BYTE *pp = rgba;
		for (x = width; x < width * height; x++) {
			p[0] += pp[0];
			p[1] += pp[1];
			p[2] += pp[2];
			p += 3;
			pp += 3;
		}

		if (mmo_header->alpha_offset) {
			DWORD alpha_size = width * height;
			BYTE *alpha = (BYTE *)malloc(alpha_size);
			if (!alpha) {
				free(rgba);
				free(raw);
				return -CUI_EMEM;
			}
			lzss_uncompress2(alpha, 
				alpha_size, (BYTE *)(mmo_header + 1) + mmo_header->alpha_offset, 
				pkg_res->raw_data_length - sizeof(mmo_header_t) - mmo_header->alpha_offset);			
			
			rgba_size = width * height * 4;
			BYTE *act_rgba = (BYTE *)malloc(rgba_size);
			if (!act_rgba) {
				free(alpha);
				free(rgba);
				free(raw);
				return -CUI_EMEM;
			}

			for (x = 0; x < width * height; x++) {
				act_rgba[x * 4 + 0] = rgba[x * 3 + 0];
				act_rgba[x * 4 + 1] = rgba[x * 3 + 1];
				act_rgba[x * 4 + 2] = rgba[x * 3 + 2];
				act_rgba[x * 4 + 3] = alpha[x];
			}			
			free(alpha);
			free(rgba);
			rgba = act_rgba;
			if (MyBuildBMPFile(rgba, rgba_size, NULL, 0, width,
					height, 32, (BYTE **)&pkg_res->actual_data,
					&pkg_res->actual_data_length, my_malloc)) {
				free(rgba);
				free(raw);
				return -CUI_EMEM;
			}
		} else {
			if (MyBuildBMPFile(rgba, rgba_size, NULL, 0, width,
					height, 24, (BYTE **)&pkg_res->actual_data,
					&pkg_res->actual_data_length, my_malloc)) {
				free(rgba);
				free(raw);
				return -CUI_EMEM;
			}
		}
		free(rgba);
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp");
	} else if (!strcmpi(ext, ".GCC") && !strncmp((char *)raw, "G24n" ,4)) {
		gcc_header_t *g24n_header = (gcc_header_t *)raw;
		//DWORD width = g24n_header->width - g24n_header->orig_x;
		//DWORD height = g24n_header->height - g24n_header->orig_y;
		DWORD width = g24n_header->width;
		DWORD height = g24n_header->height;
		DWORD rgba_size = width * height * 3;
		BYTE *rgba = (BYTE *)malloc(rgba_size);
		if (!rgba) {
			free(raw);
			return -CUI_EMEM;
		}
		lzss_uncompress2(rgba, 
			rgba_size, (BYTE *)(g24n_header + 1), 
			pkg_res->raw_data_length - sizeof(gcc_header_t));

		if (MyBuildBMPFile(rgba, rgba_size, NULL, 0, width,
				height, 24, (BYTE **)&pkg_res->actual_data,
				&pkg_res->actual_data_length, my_malloc)) {
			free(rgba);
			free(raw);
			return -CUI_EMEM;
		}
		free(rgba);
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp");
	} else if (!strcmpi(ext, ".GCC") && !strncmp((char *)raw, "R24n" ,4)) {
		printf("%s\n", pkg_res->name);


	}

	pkg_res->raw_data = raw;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation AI5WIN_arc_operation = {
	AI5WIN_arc_match,				/* match */
	AI5WIN_arc_extract_directory,	/* extract_directory */
	AI5WIN_arc_parse_resource_info,	/* parse_resource_info */
	AI5WIN_arc_extract_resource,	/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* VSD *********************/

/* 封包匹配回调函数 */
static int AI5WIN_VSD_match(struct package *pkg)
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
static int AI5WIN_VSD_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	u32 VSD_size;
	u32 offset;

	if (pkg->pio->length_of(pkg, &VSD_size))
		return -CUI_ELEN;

	if (pkg->pio->readvec(pkg, &offset, 4, 4, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD mpg_len = VSD_size - (offset + sizeof(VSD_header_t));
	BYTE *mpg = (BYTE *)pkg->pio->readvec_only(pkg, mpg_len, 
		offset + sizeof(VSD_header_t), IO_SEEK_SET);
	if (!mpg)
		return -CUI_EREADVECONLY;

	pkg_res->actual_data = mpg;
	pkg_res->actual_data_length = mpg_len;

	return 0;
}

static void AI5WIN_VSD_release_resource(struct package *pkg, 
										struct package_resource *pkg_res)
{
	if (pkg_res->actual_data)
		pkg_res->actual_data = NULL;
}

/* 封包处理回调函数集合 */
static cui_ext_operation AI5WIN_VSD_operation = {
	AI5WIN_VSD_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	AI5WIN_VSD_extract_resource,/* extract_resource */
	cui_common_save_resource,
	AI5WIN_VSD_release_resource,
	cui_common_release
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK AI5WIN_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".arc"), NULL, 
		NULL, &AI5WIN_arc_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR 
		| CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".VSD"), _T(".mpg"), 
		NULL, &AI5WIN_VSD_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	return 0;
}
