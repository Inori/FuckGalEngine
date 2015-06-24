#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information SHSystem_cui_information = {
	_T(""),					/* copyright */
	_T("SHSystem"),			/* system */
	_T(".hxp"),				/* package */
	_T("1.0.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-6-17 21:28"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];		// "Him4"
	u32 index_entries;	// 
} Him4_header_t;

typedef struct {
	s8 magic[4];		// "Him5"
	u32 index_entries;	// 
} Him5_header_t;

typedef struct {
	s8 magic[4];		// "SHS6"
	u32 index_entries;	// must less than 2000
} SHS6_header_t;

typedef struct {
	s8 magic[4];		// "SHS7"
	u32 index_entries;
} SHS7_header_t;

typedef struct {
	u32 entry_size;
	u32 entry_offset;
} SHS7_entry_t;

struct TargaTail_s {
    ULONG eao;		// 扩展区偏移 
    ULONG ddo;		// 开发者区偏移 
    UCHAR info[16];	// TRUEVISION-XFILE 商标字符串 
    UCHAR period;	// 字符"." 
    UCHAR zero;		// 0 
};
#pragma pack ()

/* .dat封包的索引项结构 */
typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 comprlen;		// 为0表示无压缩
	u32 uncomprlen;		// 不为0
} SHS_entry_t;

#define SWAP32(v)	((((v) & 0xff) << 24) | (((v) & 0xff00) << 8) | (((v) & 0xff0000) >> 8) | (((v) & 0xff000000) >> 24))

static int version;

static void shs_uncompress(BYTE *uncompr, DWORD uncomprlen, BYTE *compr)
{
	BYTE flag;
	DWORD act_uncomprlen = 0;

	while (act_uncomprlen < uncomprlen) {
		DWORD copy_bytes, offset;

		flag = *compr++;
		if (flag < 32) {
			if (flag == 29) {
				copy_bytes = *compr++ + 30;
			} else if (flag == 30) {
				copy_bytes = ((compr[0] << 8) | compr[1]) + 286;
				compr += 2;
			} else if (flag == 31) {
				copy_bytes = (compr[0] << 24) | (compr[1] << 16) | (compr[2] << 8) | compr[3];
				compr += 4;
			} else
				copy_bytes = flag + 1;
			offset = 0;
		} else {
			if (!(flag & 0x80)) {
				if ((flag & 0x60) == 0x20) {
					offset = (flag >> 2) & 7;
					copy_bytes = flag & 3;
				} else {
					offset = *compr++;
					if ((flag & 0x60) == 0x40)
						copy_bytes = (flag & 0x1f) + 4;
					else {
						offset |= (flag & 0x1f) << 8;
						flag = *compr++;
						if (flag == 0xfe) {
							copy_bytes = ((compr[0] << 8) | compr[1]) + 258;
							compr += 2;
						} else if (flag == 0xff) {
							copy_bytes = (compr[0] << 24) | (compr[1] << 16) | (compr[2] << 8) | compr[3];
							compr += 4;
						} else 
							copy_bytes = flag + 4;
					}
				}
			} else {
				copy_bytes = (flag >> 5) & 3;
				offset = ((flag & 0x1f) << 8) | *compr++;
			}
			copy_bytes += 3;
			offset++;
		}

		if (!offset) {
			memcpy(&uncompr[act_uncomprlen], compr, copy_bytes);
			act_uncomprlen += copy_bytes;
			compr += copy_bytes;
		} else {
			for (DWORD i = 0; i < copy_bytes; i++) {
				uncompr[act_uncomprlen] = uncompr[act_uncomprlen - offset];
				act_uncomprlen++;
			}
		}
	}
}

/********************* hxp *********************/

/* 封包匹配回调函数 */
static int SHSystem_hxp_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (!strncmp(magic, "Him4", 4))
		version = 4;
	else if (!strncmp(magic, "Him5", 4))
		version = 5;
	else if (!strncmp(magic, "SHS6", 4))
		version = 6;
	else if (!strncmp(magic, "SHS7", 4))
		version = 7;
	else {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int SHSystem_hxp_extract_directory(struct package *pkg,
										  struct package_directory *pkg_dir)
{
	SHS_entry_t *index_buffer;
	unsigned int index_buffer_length;	
	unsigned int i;
	u32 *offset_buffer;

	if (pkg->pio->read(pkg, &pkg_dir->index_entries, 4))
		return -CUI_EREAD;

	if (version == 4 || version == 6) {
		index_buffer_length = pkg_dir->index_entries * sizeof(SHS_entry_t);
		index_buffer = (SHS_entry_t *)malloc(index_buffer_length);
		if (!index_buffer)
			return -CUI_EMEM;

		offset_buffer = (u32 *)malloc(4 * pkg_dir->index_entries);
		if (!offset_buffer) {
			free(index_buffer);
			return -CUI_EMEM;
		}

		if (pkg->pio->read(pkg, offset_buffer, 4 * pkg_dir->index_entries)) {
			free(offset_buffer);
			free(index_buffer);
			return -CUI_EREAD;
		}

		for (i = 0; i < pkg_dir->index_entries; i++) {
			sprintf(index_buffer[i].name, "%08d", i);
			index_buffer[i].offset = offset_buffer[i] + 8;

			if (pkg->pio->readvec(pkg, &index_buffer[i].comprlen, 4, offset_buffer[i], IO_SEEK_SET))
				break;

			if (pkg->pio->read(pkg, &index_buffer[i].uncomprlen, 4))
				break;
		}
		free(offset_buffer);
		if (i != pkg_dir->index_entries) {
			free(index_buffer);
			return -CUI_EREAD;
		}
	} else if (version == 5 || version == 7) {
		u32 act_entries;
		SHS7_entry_t *index_buffer7;

		index_buffer_length = pkg_dir->index_entries * sizeof(SHS7_entry_t);
		index_buffer7 = (SHS7_entry_t *)malloc(index_buffer_length);
		if (!index_buffer7)
			return -CUI_EMEM;	
		
		if (pkg->pio->read(pkg, index_buffer7, index_buffer_length)) {
			free(index_buffer7);
			return -CUI_EREAD;
		}

		int ret = -1;
		act_entries = 0;
		for (i = 0; i < pkg_dir->index_entries; i++) {
			if (!index_buffer7[i].entry_size)
				continue;

			BYTE *entry = (BYTE *)malloc(index_buffer7[i].entry_size);
			if (!entry) {
				ret = -CUI_EMEM;
				break;
			}

			if (pkg->pio->readvec(pkg, entry, index_buffer7[i].entry_size, 
					index_buffer7[i].entry_offset, IO_SEEK_SET)) {
				free(entry);
				ret = -CUI_EREADVEC;
				break;
			}

			BYTE *p = entry;
			int len = *p;
			while (len) {
				p += len;
				len = *p;
				act_entries++;
			}
			free(entry);
		}
		if (i != pkg_dir->index_entries) {
			free(index_buffer7);
			return ret;
		}

		index_buffer_length = act_entries * sizeof(SHS_entry_t);
		index_buffer = (SHS_entry_t *)malloc(index_buffer_length);
		if (!index_buffer) {
			free(index_buffer7);
			return -CUI_EMEM;
		}

		DWORD k = 0;
		for (i = 0; i < pkg_dir->index_entries; i++) {
			if (!index_buffer7[i].entry_size)
				continue;

			BYTE *entry = (BYTE *)malloc(index_buffer7[i].entry_size);
			if (!entry) {
				ret = -CUI_EMEM;
				break;
			}

			if (pkg->pio->readvec(pkg, entry, index_buffer7[i].entry_size, 
					index_buffer7[i].entry_offset, IO_SEEK_SET)) {
				free(entry);
				ret = -CUI_EREADVEC;
				break;
			}

			BYTE *p = entry;
			int len = *p;
			while (len) {
				index_buffer[k].offset = SWAP32(*(u32 *)&p[1]);
				if (pkg->pio->readvec(pkg, &index_buffer[k].comprlen, 4, index_buffer[k].offset, IO_SEEK_SET)) {
					ret = -CUI_EREADVEC;
					break;
				}
				if (pkg->pio->read(pkg, &index_buffer[k].uncomprlen, 4)) {
					ret = -CUI_EREAD;
					break;
				}
				index_buffer[k].offset += 8;
				strcpy(index_buffer[k].name, (char *)&p[5]);
				p += len;
				len = *p;
				k++;				
			}
			free(entry);
			if (len)
				break;
		}
		free(index_buffer7);
		if (i != pkg_dir->index_entries) {
			free(index_buffer);
			return ret;
		}

		pkg_dir->index_entries = act_entries;
	}

	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(SHS_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int SHSystem_hxp_parse_resource_info(struct package *pkg,
											struct package_resource *pkg_res)
{
	SHS_entry_t *SHS_entry;

	SHS_entry = (SHS_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, SHS_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = SHS_entry->comprlen;
	pkg_res->actual_data_length = SHS_entry->uncomprlen;
	if (!pkg_res->raw_data_length) {
		pkg_res->raw_data_length = SHS_entry->uncomprlen;
		pkg_res->actual_data_length = 0;
	}
	pkg_res->offset = SHS_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int SHSystem_hxp_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr, *dat;
	DWORD *dat_len;
	struct TargaTail_s tga_tailer;

	compr = (BYTE *)malloc(pkg_res->raw_data_length + sizeof(tga_tailer));
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, pkg_res->raw_data_length, 
			pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	if (pkg_res->actual_data_length) {
		uncompr = (BYTE *)malloc(pkg_res->actual_data_length + sizeof(tga_tailer));
		if (!uncompr)
			return -CUI_EMEM;
		shs_uncompress(uncompr, pkg_res->actual_data_length, compr);
		dat = uncompr;
		dat_len = &pkg_res->actual_data_length;
	} else {
		uncompr = NULL;
		dat = compr;
		dat_len = &pkg_res->raw_data_length;
	}

	if (!strncmp((char *)dat, "BM", 2)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".bmp");
	} else if (!strncmp((char *)dat, "SHSysSC", 8)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".sc");
	} else if (!strncmp((char *)dat, "OggS", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".ogg");
	} else if (!strncmp((char *)dat, "RIFF", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".wav");
	} else if (!strncmp((char *)dat, "Himauri", 8)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".hst");
	} else if (!strncmp((char *)dat + *dat_len - 18, "TRUEVISION-XFILE.", 18)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".tga");
	} else if (*dat_len == (DWORD)(*(u16 *)&dat[12] * *(u16 *)&dat[14] * dat[16] / 8 + 18)) {
		tga_tailer.eao = 0;
		tga_tailer.ddo = 0;
		memcpy(tga_tailer.info, "RUEVISION-XFILE", 16);
		tga_tailer.period = '.';
		tga_tailer.zero = 0;
		memcpy(dat + *dat_len, &tga_tailer, sizeof(tga_tailer));
		*dat_len +=  sizeof(tga_tailer); 
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".tga");
	} else if (!lstrcmpi(pkg->name, L"koi_face.hxp")) {

		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".tga");
	}	
	// else if (MultiByteToWideChar(932, MB_ERR_INVALID_CHARS, (LPCSTR)dat, *dat_len, NULL, 0)) {
	//	pkg_res->flags |= PKG_RES_FLAG_APEXT;
	//	pkg_res->replace_extension = _T(".txt");
	//}

	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;

	return 0;
}

/* 资源保存函数 */
static int SHSystem_hxp_save_resource(struct resource *res, 
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
static void SHSystem_hxp_release_resource(struct package *pkg, 
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
static void SHSystem_hxp_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation SHSystem_hxp_operation = {
	SHSystem_hxp_match,					/* match */
	SHSystem_hxp_extract_directory,		/* extract_directory */
	SHSystem_hxp_parse_resource_info,	/* parse_resource_info */
	SHSystem_hxp_extract_resource,		/* extract_resource */
	SHSystem_hxp_save_resource,			/* save_resource */
	SHSystem_hxp_release_resource,		/* release_resource */
	SHSystem_hxp_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK SHSystem_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".hxp"), NULL, 
		NULL, &SHSystem_hxp_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
