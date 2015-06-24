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
struct acui_information ags_cui_information = {
	_T("JamCREATION,Inc/ゆい工房"),		/* copyright */
	_T("Anime Script System"),			/* system */
	_T(".dat"),							/* package */
	_T("1.0.0"),						/* revision */
	_T("痴汉公贼"),						/* author */
	_T("2007-3-24 10:12"),				/* date */
	NULL,								/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];		// "pack"
	u16 index_entries;	
} dat_header_t;

/* .dat封包的索引项结构 */
typedef struct {
	s8 name[16];
	u32 offset;			// from file begining
	u32 length;
} dat_entry_t;

typedef struct {
	s8 magic[3];		// "WAV"
	u8 flags;	
} pcm_header_t;

typedef struct {
	u8 flag;		// 16
	u16 width;
	u16 height;
} cg_header_t;

typedef struct {
	u8 flag;
	u16 width;
	u16 height;
} ani_header_t;

typedef struct {
	u16 orig_x;
	u16 orig_y;
	u16 end_x;
	u16 end_y;
} ani_info_t;
#pragma pack ()

typedef struct {
	s8 name[MAX_PATH];
	u32 length;
	u32 offset;
} ani_entry_t;

static BYTE ani_base_image[640 * 360 * 3];

static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static void decompress(BYTE *uncompr, DWORD uncomprlen, BYTE *compr, DWORD width, DWORD height,
					   DWORD orig_x, DWORD end_x, DWORD orig_y, DWORD end_y)
{
	u32 offset[8];
	s16 x_pos[8] = { 0, -1, -3, -2, -1, 0, 1, 2 };
	s16 y_pos[8] = { 0, 0, -1, -1, -1, -1, -1, -1 };
	DWORD line_len;

	line_len = width * 3;
	for (DWORD i = 0; i < 8; i++)
		offset[i] = y_pos[i] * line_len /* 640 * 3 */ + x_pos[i] * 3;

	for (DWORD y = orig_y; y < end_y; y++) {
		DWORD line = y * line_len;
		for (DWORD x = line + orig_x * 3; x < line + end_x * 3; ) {
			BYTE flag = *compr++;
			if (flag & 0x80) {
				if (!(flag & 0x40)) {
					uncompr[x++] = (flag << 2) | ((*compr & 1) << 1);
					uncompr[x++] = *compr++ & 0xfe;
					uncompr[x++] = *compr++;
				} else {
					uncompr[x] = uncompr[x - 3] + ((flag >> 3) & 6) - 2;
					uncompr[x + 1] = uncompr[x - 2] + ((flag >> 1) & 6) - 2;
					uncompr[x + 2] = uncompr[x - 1] + (((flag & 3) + 0x7f) << 1);
					x += 3;
				}
			} else {
				BYTE step = flag >> 4;
				DWORD count = flag & 0xf;

				if (!count) {
					count = flag = *compr++;
					while (flag == 0xff) {
						flag = *compr++;
						count += flag;
					}
					count += 15;
				}

				if (step) {
					BYTE *src = uncompr + x + offset[step];
					for (DWORD i = 0; i < count; i++) {
						uncompr[x++] = *src++;
						uncompr[x++] = *src++;
						uncompr[x++] = *src++;
					}
				} else
					x += count * 3;
			}
		}
	}
}

/********************* dat *********************/

/* 封包匹配回调函数 */
static int ags_dat_match(struct package *pkg)
{
	s8 magic[4];

	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "pack", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int ags_dat_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{
	dat_entry_t *index_buffer;
	unsigned int index_buffer_length;	

	if (!pkg || !pkg_dir)
		return -CUI_EPARA;

	if (pkg->pio->read(pkg, &pkg_dir->index_entries, 2))
		return -CUI_EREAD;

	index_buffer_length = pkg_dir->index_entries * sizeof(dat_entry_t);
	index_buffer = (dat_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(dat_entry_t);
//	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int ags_dat_parse_resource_info(struct package *pkg,
									   struct package_resource *pkg_res)
{
	dat_entry_t *dat_entry;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	dat_entry = (dat_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, dat_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = dat_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = dat_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int ags_dat_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)pkg->pio->readvec_only(pkg, comprlen, pkg_res->offset, IO_SEEK_SET);
	if (!compr)
		return -CUI_EREADVECONLY;

	uncompr = NULL;
	uncomprlen = 0;
	if (!strncmp((char *)compr, "WAV", 3)) {
		pcm_header_t *pcm_header = (pcm_header_t *)compr;
		WAVEFORMATEX wav_header;
		DWORD riff_chunk_len, fmt_chunk_len, data_chunk_len;
		char *riff = "RIFF";
		char *id = "WAVE"; 
		char *fmt_chunk_id = "fmt ";
		char *data_chunk_id = "data";
		BYTE *p;

		if (!lstrcmp(pkg->name, _T("bgm.dat"))) {
			wav_header.nChannels = 2;
			wav_header.nAvgBytesPerSec = 0x15888;
			wav_header.nBlockAlign = 4;
		} else {	// voice
			wav_header.nChannels = 1;
			wav_header.nAvgBytesPerSec = 0xac44;
			wav_header.nBlockAlign = 2;
		}
		wav_header.wFormatTag = 1;
		wav_header.nSamplesPerSec = 0x5622;	
		wav_header.wBitsPerSample = 16;
		wav_header.cbSize = 0;

		data_chunk_len = comprlen - 4;
		fmt_chunk_len = sizeof(wav_header) + wav_header.cbSize;
		riff_chunk_len = 4 + (8 + fmt_chunk_len) + (8 + data_chunk_len);

		uncomprlen = sizeof(riff) + sizeof(riff_chunk_len) + sizeof(id) + sizeof(fmt_chunk_id)
			+ sizeof(fmt_chunk_len) + sizeof(wav_header) + sizeof(data_chunk_id) + sizeof(data_chunk_len)
			+ data_chunk_len;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr)
			return -CUI_EMEM;

		p = (BYTE *)uncompr;
		memcpy(p, riff, sizeof(riff));
		p += sizeof(riff);
		memcpy(p, &riff_chunk_len, sizeof(riff_chunk_len));
		p += sizeof(riff_chunk_len);
		memcpy(p, id, sizeof(id));
		p += sizeof(id);
		memcpy(p, fmt_chunk_id, sizeof(fmt_chunk_id));
		p += sizeof(fmt_chunk_id);
		memcpy(p, &fmt_chunk_len, sizeof(fmt_chunk_len));
		p += sizeof(fmt_chunk_len);
		memcpy(p, &wav_header, sizeof(wav_header));
		p += sizeof(wav_header);
		memcpy(p, data_chunk_id, sizeof(data_chunk_id));
		p += sizeof(data_chunk_id);
		memcpy(p, &data_chunk_len, sizeof(data_chunk_len));
		p += sizeof(data_chunk_len);
		memcpy(p, pcm_header + 1, data_chunk_len);

		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".wav");
	} else if (strstr(pkg_res->name, ".cg")) {
		cg_header_t *cg = (cg_header_t *)compr;
		if (cg->flag != 16)
			printf("%x\n", cg->flag);
		uncomprlen = cg->width * cg->height * 3;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr)
			return -CUI_EMEM;
		memset(uncompr, 0, uncomprlen);

		compr += sizeof(cg_header_t);
		decompress(uncompr, uncomprlen, compr, cg->width, cg->height, 0, cg->width, 0, cg->height);

		BYTE *backup = uncompr;
		if (MyBuildBMPFile(uncompr, uncomprlen, NULL, 0, cg->width, 0 - cg->height, 24, 
				&uncompr, &uncomprlen, my_malloc)) {
			free(backup);
			return -CUI_EMEM;
		}
		free(backup);
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".bmp");
	}
	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;
	return 0;
}

/* 资源保存函数 */
static int ags_dat_save_resource(struct resource *res, 
								 struct package_resource *pkg_res)
{
	if (!res || !pkg_res)
		return -CUI_EPARA;
	
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
static void ags_dat_release_resource(struct package *pkg, 
									 struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res->raw_data)
		pkg_res->raw_data = NULL;
}

/* 封包卸载函数 */
static void ags_dat_release(struct package *pkg, 
							struct package_directory *pkg_dir)
{
	if (!pkg || !pkg_dir)
		return;

	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation ags_dat_operation = {
	ags_dat_match,					/* match */
	ags_dat_extract_directory,		/* extract_directory */
	ags_dat_parse_resource_info,	/* parse_resource_info */
	ags_dat_extract_resource,		/* extract_resource */
	ags_dat_save_resource,			/* save_resource */
	ags_dat_release_resource,		/* release_resource */
	ags_dat_release					/* release */
};

/********************* ani *********************/

/* 封包匹配回调函数 */
static int ags_ani_match(struct package *pkg)
{
	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包索引目录提取函数 */
static int ags_ani_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{
	DWORD index_buffer_length;
	DWORD *index_buffer;
	SIZE_T fsize;
	ani_entry_t *ani_index_buffer;

	if (!pkg || !pkg_dir)
		return -CUI_EPARA;

	if (pkg->pio->length_of(pkg, &fsize))
		return -CUI_ELEN;

	if (pkg->pio->read(pkg, &index_buffer_length, 4))
		return -CUI_EREAD;

	pkg_dir->index_entries = index_buffer_length / 4;
	index_buffer = (DWORD *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, index_buffer, index_buffer_length, 0, IO_SEEK_SET)) {
		free(index_buffer);
		return -CUI_EREADVEC;
	}

	index_buffer_length = pkg_dir->index_entries * sizeof(ani_entry_t);
	ani_index_buffer = (ani_entry_t *)malloc(index_buffer_length + sizeof(ani_entry_t));
	if (!ani_index_buffer) {
		free(index_buffer);
		return -CUI_EMEM;
	}
	ani_index_buffer[pkg_dir->index_entries].offset = fsize;
	ani_index_buffer[pkg_dir->index_entries].length = 1;

	for (DWORD i = 0; i < pkg_dir->index_entries; i++) {
		BYTE flag;

		sprintf(ani_index_buffer[i].name, "%08d.ani.bmp", i);
		ani_index_buffer[i].offset = index_buffer[i];
		if (pkg->pio->readvec(pkg, &flag, 1, ani_index_buffer[i].offset, IO_SEEK_SET)) {
			free(index_buffer);
			return -CUI_EREADVEC;
		}
		if (flag == 1)
			ani_index_buffer[i].length = 0;
		else
			ani_index_buffer[i].length = 1;
	}

	for (i = 0; i < pkg_dir->index_entries; i++) {
		if (ani_index_buffer[i].length) {
			/* 找到前一项 */
			DWORD offset = fsize;
			for (DWORD k = 0; k < pkg_dir->index_entries + 1; k++) {
				if (ani_index_buffer[k].length) {
					if (ani_index_buffer[k].offset > ani_index_buffer[i].offset && ani_index_buffer[k].offset < offset)
						offset = ani_index_buffer[k].offset;
				}
			}
			ani_index_buffer[i].length = offset - ani_index_buffer[i].offset;
		}
	}
//	for (i = 0; i < pkg_dir->index_entries; i++) {
//		printf("%s: %x @ %x\n", pkg_res->name, ani_index_buffer[i].length, ani_index_buffer[i].offset);
//	}

	pkg_dir->directory = ani_index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(ani_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int ags_ani_parse_resource_info(struct package *pkg,
									   struct package_resource *pkg_res)
{
	ani_entry_t *ani_entry;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	ani_entry = (ani_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, ani_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = ani_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = ani_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int ags_ani_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	ani_header_t *ani_header;
	BYTE *compr, *uncompr, *data;
	DWORD comprlen, uncomprlen;
	DWORD orig_x, end_x, orig_y, end_y;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	ani_header = (ani_header_t *)compr;
	data = (BYTE *)(ani_header + 1);

//	printf("%s: flag %02x width %x height %x\n", pkg_res->name, ani_header->flag, ani_header->width, ani_header->height);

#if 1
	uncomprlen = ani_header->width * ani_header->height * 3;
	uncompr = (BYTE *)malloc(uncomprlen);
	if (!uncompr) {
		free(compr);
		return -CUI_EMEM;
	}
	memset(uncompr, 0, uncomprlen);

	if ((ani_header->flag & 0xf) == 1) {
		printf("1! %x exit ...\n", ani_header->flag);
		exit(0);
	}

	if (!((ani_header->flag >> 4) & 1)) {
		printf("compressed! %x...\n", ani_header->flag);
		exit(0);
	}

	/* 描述了相对于源图（ani的第一张）的区域坐标 */
	if (ani_header->flag & 0xf) {
		ani_info_t *ani_info = (ani_info_t *)data;

		data += sizeof(ani_info_t);
		orig_x = ani_info->orig_x;
		orig_y = ani_info->orig_y;
		end_x = ani_info->end_x;
		end_y = ani_info->end_y;
//		printf("%s: orig_x %x pitch_x %x orig_y %x pitch_y %x\n", 
//			pkg_res->name, ani_info->orig_x, ani_info->pitch_x, ani_info->orig_y, ani_info->pitch_y);
	} else {
		orig_x = 0;
		orig_y = 0;
		end_x = ani_header->width;
		end_y = ani_header->height;
	}

	if ((ani_header->flag >> 4) & 1)
		decompress(uncompr, uncomprlen, data, ani_header->width, ani_header->height, 
			orig_x, end_x, orig_y, end_y);
#if 1
	if (!pkg_res->index_number)
		memcpy(ani_base_image, uncompr, uncomprlen);
	else {
#if 1
		for (DWORD i = 0; i < uncomprlen; i++) {
			if (!uncompr[i])
				uncompr[i] = ani_base_image[i];
		}
#else
		for (DWORD y = 0; y < ani_header->height; y++) {
			for (DWORD x = 0; x < ani_header->width; x++) {
				if (!uncompr[y * ani_header->width * 3 + x * 3] && !uncompr[y * ani_header->width * 3 + x * 3 + 1]
					&& !uncompr[y * ani_header->width * 3 + x * 3 + 2]) {
						uncompr[y * ani_header->width * 3 + x * 3] = ani_base_image[y * ani_header->width * 3 + x * 3];
						uncompr[y * ani_header->width * 3 + x * 3 + 1] = ani_base_image[y * ani_header->width * 3 + x * 3 + 1];
						uncompr[y * ani_header->width * 3 + x * 3 + 2] = ani_base_image[y * ani_header->width * 3 + x * 3 + 2];
				}
			}
		}
#endif
		memcpy(ani_base_image, uncompr, uncomprlen);
	}
#endif
	BYTE *backup = uncompr;
	if (MyBuildBMPFile(uncompr, uncomprlen, NULL, 0, ani_header->width, 0 - ani_header->height, 24, 
			&uncompr, &uncomprlen, my_malloc)) {
		free(backup);
		return -CUI_EMEM;
	}
	free(backup);
#endif
	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

/* 资源保存函数 */
static int ags_ani_save_resource(struct resource *res, 
								 struct package_resource *pkg_res)
{
	if (!res || !pkg_res)
		return -CUI_EPARA;
	
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
static void ags_ani_release_resource(struct package *pkg, 
									 struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res->raw_data)
		pkg_res->raw_data = NULL;
}

/* 封包卸载函数 */
static void ags_ani_release(struct package *pkg, 
							struct package_directory *pkg_dir)
{
	if (!pkg || !pkg_dir)
		return;

	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation ags_ani_operation = {
	ags_ani_match,					/* match */
	ags_ani_extract_directory,		/* extract_directory */
	ags_ani_parse_resource_info,	/* parse_resource_info */
	ags_ani_extract_resource,		/* extract_resource */
	ags_ani_save_resource,			/* save_resource */
	ags_ani_release_resource,		/* release_resource */
	ags_ani_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK ags_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &ags_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".ani"), NULL, 
		NULL, &ags_ani_operation, CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
