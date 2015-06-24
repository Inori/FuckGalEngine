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
struct acui_information PJADV_cui_information = {
	_T("ぼとむれす"),					/* copyright */
	_T("PajamasSoft Adventure Engine"),	/* system */
	_T(".dat .pak .epa .bin"),			/* package */
	_T("1.0.1"),						/* revision */
	_T("痴汉公贼"),						/* author */
	_T("2008-1-25 0:19"),				/* date */
	NULL,								/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[12];		/* "GAMEDAT PACK" or "GAMEDAT PAC2" */
	u32 index_entries;
} pack_header_t;

typedef struct {
	u32 offset;
	u32 length;
} pack_entry_t;

/*
 archive.dat提取出来后有个graphic.def，里面详细定义了每个图像的用途。
 */

/* mode的用途是：
 * 立绘图的人物，通常只有表情变化。因此让mode==2的图是表情图，在它的epa
 * 中记录完整图的epa文件名，并记录下在原图中的位置 */

/* epa解压后最终都会转换为32位位图，没有α字段的图的Alpha填0xff */
typedef struct {
	s8 magic[2];		/* "EP" */
	u8 version;			/* must be 0x01 */
	u8 mode;			/* 0x01, have no XY */
	u8 color_type;		/* 0 - 8bit without alpha; 1 - 24 bit; 2 - 32bit; 4 - 8bit width alpha */
	u8 unknown0;		/* only used in 8bit */	// ？？（8bits模式下有用：0 － 默认256；非0 － 实际的颜色数）
	u8 unknown1;		/* only used in 8bit */	// ？？（8bits模式下有用：0 － 默认256；非0 － 实际的？？）	
	u8 reserved;	
	u32 width;
	u32 height;
} epa1_header_t;

// 0 - with palette; 4 - without palette

typedef struct {
	s8 magic[2];		/* "EP" */
	u8 version;			/* must be 0x01 */
	u8 mode;			/* 0x02, have XY */
	u8 color_type;		/* 0 - 8bit without alpha; 1 - 24 bit; 2 - 32bit; 3 - 16 BIT; 4 - 8bit width alpha */
	u8 unknown0;		/* only used in 8bit */	// ？？（8bits模式下有用：0 － 默认256；非0 － 实际的颜色数）
	u8 unknown1;		/* only used in 8bit */	// ？？（8bits模式下有用：0 － 默认256；非0 － 实际的？？）	
	u8 reserved;		
	u32 width;
	u32 height;
	u32 X;				/* 贴图坐标(左上角) */
	u32 Y;
	s8 name[32];
} epa2_header_t;
#pragma pack ()

typedef struct {
	union {
		epa1_header_t epa1;
		epa2_header_t epa2;	
	};
} epa_header_t;

typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 length;
} my_pack_entry_t;


static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static int epa_decompress(BYTE *uncompr, DWORD *max_uncomprlen, 
		BYTE *compr, DWORD comprlen, u32 width)
{
	unsigned int curbyte = 0;
	unsigned int uncomprlen = *max_uncomprlen;
	unsigned int act_uncomprlen = 0;
	unsigned int step_table[16];

	step_table[0] = 0;
	step_table[1] = 1;
	step_table[2] = width;
	step_table[3] = width + 1;	
	step_table[4] = 2;
	step_table[5] = width - 1;
	step_table[6] = width * 2;
	step_table[7] = 3;	
	step_table[8] = (width + 1) * 2;
	step_table[9] = width + 2;
	step_table[10] = width * 2 + 1;
	step_table[11] = width * 2 - 1;	
	step_table[12] = (width - 1) * 2 ;
	step_table[13] = width - 2;
	step_table[14] = width * 3;
	step_table[15] = 4;	
	
	/* 这里千万不要心血来潮的用memcpy()，原因是memecpy()会根据src和dst
	 * 的位置进行“正确”拷贝。但这个拷贝实际上在这里却是不正确的
	 */
	while (act_uncomprlen < uncomprlen) {
		u8 flag;
		unsigned int copy_bytes;
		
		if (curbyte >= comprlen)
			return -1;
		
		flag = compr[curbyte++];		
		if (!(flag & 0xf0)) {
			copy_bytes = flag;
			if (curbyte + copy_bytes > comprlen)
				return -1;
			if (act_uncomprlen + copy_bytes > uncomprlen)
				return -1;
			for (unsigned int i = 0; i < copy_bytes; i++)
				uncompr[act_uncomprlen++] = compr[curbyte++];
		} else {
			BYTE *src;
			
			if (flag & 8) {
				if (curbyte >= comprlen)
					return -1;				
				copy_bytes = ((flag & 7) << 8) + compr[curbyte++];
			} else
				copy_bytes = flag & 7;
			if (act_uncomprlen + copy_bytes > uncomprlen)
				return -1;
			src = &uncompr[act_uncomprlen] - step_table[flag >> 4];	
			for (unsigned int i = 0; i < copy_bytes; i++)
				uncompr[act_uncomprlen++] = *src++;
		}
	}
	*max_uncomprlen = act_uncomprlen;

	return 0;
}

/********************* dat *********************/

/* 封包匹配回调函数 */
static int PJADV_dat_match(struct package *pkg)
{
	s8 magic[12];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "GAMEDAT PACK", sizeof(magic)) && memcmp(magic, "GAMEDAT PAC2", sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int PJADV_dat_extract_directory(struct package *pkg,
									   struct package_directory *pkg_dir)
{
	pack_header_t pack_header;
	my_pack_entry_t *index_buffer;
	BYTE *raw_index_buffer;
	unsigned int raw_index_buffer_length, name_length, entry_size;
	u32 offset;
	unsigned int i;

	if (pkg->pio->seek(pkg, 0, IO_SEEK_SET))
		return -CUI_ESEEK;

	if (pkg->pio->read(pkg, &pack_header, sizeof(pack_header)))
		return -CUI_EREAD;

	if (!strncmp(pack_header.magic, "GAMEDAT PACK", sizeof(pack_header.magic)))
		name_length = 16;
	else if (!strncmp(pack_header.magic, "GAMEDAT PAC2", sizeof(pack_header.magic)))
		name_length = 32;

	entry_size = name_length + sizeof(pack_entry_t);

	raw_index_buffer_length = entry_size * pack_header.index_entries;
	raw_index_buffer = (BYTE *)malloc(raw_index_buffer_length);
	if (!raw_index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, raw_index_buffer, raw_index_buffer_length)) {
		free(raw_index_buffer);
		return -CUI_EREAD;
	}

	index_buffer = (my_pack_entry_t *)malloc(sizeof(my_pack_entry_t) * pack_header.index_entries);
	if (!index_buffer) {
		free(raw_index_buffer);
		return -CUI_EMEM;
	}

	offset = sizeof(pack_header) + raw_index_buffer_length;
	for (i = 0; i < pack_header.index_entries; i++) {
		my_pack_entry_t *my_entry = &index_buffer[i];
		BYTE *name = raw_index_buffer + i * name_length;
		pack_entry_t *entry = (pack_entry_t *)(raw_index_buffer 
			+ pack_header.index_entries * name_length + i * sizeof(pack_entry_t));

		strncpy(my_entry->name, (char *)name, name_length);
		my_entry->name[name_length] = 0;
		my_entry->length = entry->length;
		my_entry->offset = entry->offset + offset;
	}
	free(raw_index_buffer);

	pkg_dir->index_entries = pack_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = pack_header.index_entries * sizeof(my_pack_entry_t);
	pkg_dir->index_entry_length = sizeof(my_pack_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int PJADV_dat_parse_resource_info(struct package *pkg,
										 struct package_resource *pkg_res)
{
	my_pack_entry_t *my_pack_entry;

	my_pack_entry = (my_pack_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_pack_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_pack_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_pack_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int PJADV_dat_extract_resource(struct package *pkg,
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
static int PJADV_dat_save_resource(struct resource *res, 
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

/* 封包资源释放函数 */
static void PJADV_dat_release_resource(struct package *pkg, 
									   struct package_resource *pkg_res)
{
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

/* 封包卸载函数 */
static void PJADV_dat_release(struct package *pkg, 
							  struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation PJADV_dat_operation = {
	PJADV_dat_match,				/* match */
	PJADV_dat_extract_directory,	/* extract_directory */
	PJADV_dat_parse_resource_info,	/* parse_resource_info */
	PJADV_dat_extract_resource,		/* extract_resource */
	PJADV_dat_save_resource,		/* save_resource */
	PJADV_dat_release_resource,		/* release_resource */
	PJADV_dat_release				/* release */
};

/********************* epa *********************/

/* 封包匹配回调函数 */
static int PJADV_epa_match(struct package *pkg)
{
	s8 magic[2];
	u8 ver;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "EP", sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (pkg->pio->read(pkg, &ver, 1)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (ver != 1) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int PJADV_epa_extract_resource(struct package *pkg,
									  struct package_resource *pkg_res)
{
	epa_header_t epa_header;
	u8 mode;
	u32 color_type, width, height;
	unsigned int compr_offset;
	BYTE *uncompr, *compr, *palette;
	u32 raw_length;
	DWORD uncomprlen, comprlen, act_uncomprlen, palette_length;
	unsigned int pixel_bytes;
	int have_alpha;

	if (pkg->pio->read(pkg, &mode, 1))
		return -CUI_EREAD;

	if (mode != 1 && mode != 2)
		return -CUI_EMATCH;

	if (pkg->pio->length_of(pkg, &raw_length))
		return -CUI_ELEN;

	if (pkg->pio->seek(pkg, 0, IO_SEEK_SET))
		return -CUI_ESEEK;

	if (mode == 1) {
		if (pkg->pio->read(pkg, &epa_header.epa1, sizeof(epa_header.epa1)))
			return -CUI_EREAD;
		
		width = epa_header.epa1.width;
		height = epa_header.epa1.height;
		color_type = epa_header.epa1.color_type;
		compr_offset = sizeof(epa_header.epa1);
	} else {
		if (pkg->pio->read(pkg, &epa_header.epa2, sizeof(epa_header.epa2)))
			return -CUI_EREAD;
		
		width = epa_header.epa2.width;
		height = epa_header.epa2.height;
		color_type = epa_header.epa2.color_type;
		compr_offset = sizeof(epa_header.epa2);
	}

	have_alpha = 0;

	switch (color_type) {
	case 0:
		pixel_bytes = 1;
		palette_length = 768;
		break;
	case 1:
		pixel_bytes = 3;
		palette_length = 0;
		break;
	case 2:
		pixel_bytes = 4;
		palette_length = 0;
		break;
//	case 3:
//		pixel_bytes = 2;
//		palette_length = 0;
//		break;
	case 4:
		pixel_bytes = 1;	// 实际解压时候，比case0的情况多出height * width的数据，这段数据就是α
		palette_length = 768;
		have_alpha = 1;
		break;
	default:
		return -CUI_EMATCH;
	}

	if (palette_length) {
		palette = (BYTE *)malloc(palette_length);
		if (!palette)
			return -CUI_EMEM;

		if (pkg->pio->readvec(pkg, palette, palette_length, compr_offset, IO_SEEK_SET)) {
			free(palette);
			return -CUI_EREADVEC;
		}
	} else
		palette = NULL;

	uncomprlen = width * height * pixel_bytes;
	uncompr = (BYTE *)malloc(uncomprlen);
	if (!uncompr) {
		free(palette);
		return -CUI_EMEM;
	}

	comprlen = raw_length - compr_offset - palette_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr) {
		free(uncompr);
		free(palette);
		return -CUI_EMEM;
	}

	if (pkg->pio->readvec(pkg, compr, comprlen, compr_offset + palette_length, IO_SEEK_SET)) {
		free(compr);
		free(uncompr);
		free(palette);
		return -CUI_EREADVEC;
	}

	act_uncomprlen = uncomprlen;
	if (epa_decompress(uncompr, &act_uncomprlen, compr, comprlen, width)) {
		free(compr);
		free(uncompr);
		free(palette);
		return -CUI_EUNCOMPR;
	}
	free(compr);
	if (act_uncomprlen != uncomprlen) {
		free(uncompr);
		free(palette);
		return -CUI_EUNCOMPR;
	}

	/* 将RGBA数据重新整理 */
	if (pixel_bytes > 1) {
		BYTE *tmp = (BYTE *)malloc(uncomprlen);
		if (!tmp) {
			free(uncompr);
			free(palette);
			return -CUI_EMEM;
		}
	
		unsigned int line_len = width * pixel_bytes;
		unsigned int i = 0;
		for (unsigned int p = 0; p < pixel_bytes; p++) {
			for (unsigned int y = 0; y < height; y++) {
				BYTE *line = &tmp[y * line_len];
				for (unsigned int x = 0; x < width; x++) {
					BYTE *pixel = &line[x * pixel_bytes];
					pixel[p] = uncompr[i++];
				}
			}
		}
		free(uncompr);	
		uncompr = tmp;
	}

	if (pixel_bytes == 2) {
		if (MyBuildBMP16File(uncompr, uncomprlen, width, height, (BYTE **)&pkg_res->actual_data, 
				&pkg_res->actual_data_length, RGB555, NULL, my_malloc)) {
			free(uncompr);
			free(palette);
			return -CUI_EMEM;
		}
	} else if (MyBuildBMPFile(uncompr, uncomprlen, palette, palette_length,
			width, 0 - height, pixel_bytes * 8,	(BYTE **)&pkg_res->actual_data, 
				&pkg_res->actual_data_length, my_malloc)) {
		free(uncompr);
		free(palette);
		return -CUI_EMEM;	
	}
	free(uncompr);
	free(palette);

	pkg_res->raw_data = NULL;	
	pkg_res->raw_data_length = raw_length;

	return 0;
}

/* 资源保存函数 */
static int PJADV_epa_save_resource(struct resource *res, 
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
static void PJADV_epa_release_resource(struct package *pkg, 
									   struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
}

/* 封包卸载函数 */
static void PJADV_epa_release(struct package *pkg, 
							  struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation PJADV_epa_operation = {
	PJADV_epa_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	PJADV_epa_extract_resource,	/* extract_resource */
	PJADV_epa_save_resource,	/* save_resource */
	PJADV_epa_release_resource,	/* release_resource */
	PJADV_epa_release			/* release */
};

/********************* textdata.bin *********************/

/* 封包匹配回调函数 */
static int PJADV_bin_match(struct package *pkg)
{
	s8 magic[12];

	if (lstrcmp(pkg->name, _T("textdata.bin")))
		return -CUI_EMATCH;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (!strncmp(magic, "PJADV_TF0001", 12)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包资源提取函数 */
static int PJADV_bin_extract_resource(struct package *pkg,
									  struct package_resource *pkg_res)
{
	u32 scenario_size;
	BYTE *scenario;

	if (pkg->pio->length_of(pkg, &scenario_size))
		return -CUI_ELEN;

	scenario = (BYTE *)malloc(scenario_size);
	if (!scenario)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, scenario, scenario_size, 0, IO_SEEK_SET)) {
		free(scenario);
		return -CUI_EREAD;
	}

	BYTE xor = 0xc5;
	for (DWORD i = 0; i < scenario_size; i++) {
		scenario[i] ^= xor;
		xor += 0x5c;
	}

	pkg_res->raw_data = NULL;	
	pkg_res->raw_data_length = scenario_size;
	pkg_res->actual_data = scenario;	
	pkg_res->actual_data_length = scenario_size;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation PJADV_bin_operation = {
	PJADV_bin_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	PJADV_bin_extract_resource,	/* extract_resource */
	PJADV_epa_save_resource,	/* save_resource */
	PJADV_epa_release_resource,	/* release_resource */
	PJADV_epa_release			/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK PJADV_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &PJADV_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".pak"), NULL, 
		NULL, &PJADV_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".epa"), _T(".bmp"), 
		NULL, &PJADV_epa_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".bin"), _T(".bin.out"), 
		NULL, &PJADV_bin_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;	

	return 0;
}
