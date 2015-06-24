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
#include <vector>

using namespace std;

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information VALKYRIA_cui_information = {
	NULL,					/* copyright */
	NULL,					/* system */
	_T(".odn .dat .ini"),	/* package */
	_T("0.9.7"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-8-23 16:40"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];
	u32 index_entries;
} VALKYRIA_header_t;
#pragma pack ()

typedef struct {
	char name[MAX_PATH];
	DWORD offset;
	DWORD length;
} odn_entry_t;

static void *my_malloc(DWORD len)
{
	return new BYTE[len]();
}

static DWORD data_uncompress(BYTE *uncompr, DWORD uncomprLen,
							BYTE *compr, DWORD comprLen)
{
	DWORD act_uncomprLen = 0;
	DWORD curByte = 0;

	while (1) {
		BYTE flag = compr[curByte++];

		for (unsigned int curBit = 0; curBit < 8; curBit++) {
			BYTE b, g, r, a;
			DWORD copy_pixel;

			b = compr[curByte++];
			g = compr[curByte++];
			r = compr[curByte++];
			a = compr[curByte++];

			if (flag & (1 << curBit)) {
				copy_pixel = compr[curByte++] << 8;
				copy_pixel |= compr[curByte++];	
			} else
				copy_pixel = 0;
			copy_pixel++;

			for (DWORD i = 0; i < copy_pixel; i++) {
				uncompr[act_uncomprLen++] = b;	
				uncompr[act_uncomprLen++] = g;	
				uncompr[act_uncomprLen++] = r;
				uncompr[act_uncomprLen++] = a;
			}
			if (act_uncomprLen >= uncomprLen)	// width * height * 4
				return curByte;
		}
	}
	return curByte;
}	

static DWORD back_uncompress(BYTE *uncompr, DWORD uncomprLen,
							BYTE *compr, DWORD comprLen)
{
	DWORD act_uncomprLen = 0;
	DWORD curByte = 0;

	while (1) {
		BYTE flag = compr[curByte++];

		for (unsigned int curBit = 0; curBit < 8; ++curBit) {
			BYTE b, g, r;
			DWORD copy_pixel;

			b = compr[curByte++];
			g = compr[curByte++];
			r = compr[curByte++];

			if (flag & (1 << curBit)) {
				copy_pixel = compr[curByte++] << 8;
				copy_pixel |= compr[curByte++];	
			} else
				copy_pixel = 0;
			++copy_pixel;

			for (DWORD i = 0; i < copy_pixel; ++i) {
				uncompr[act_uncomprLen++] = b;	
				uncompr[act_uncomprLen++] = g;	
				uncompr[act_uncomprLen++] = r;
				if (act_uncomprLen >= uncomprLen)	// 640 * 480 * 3
					return curByte;
			}	
		}
	}
	return curByte;
}	

/********************* odn *********************/

/* 封包匹配回调函数 */
static int VALKYRIA_odn_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
#if 0
	u32 odn_size;
	if (pkg->pio->length_of(pkg, &odn_size)) {
		pkg->pio->close(pkg);
		return -CUI_ELEN;
	}

	if (odn_size == 0x8073) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}
#endif
	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (!memcmp(magic, "\x00\x00\x01\xba", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;		
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int VALKYRIA_odn_extract_directory(struct package *pkg,
										  struct package_directory *pkg_dir)
{
	u32 odn_size;
	if (pkg->pio->length_of(pkg, &odn_size))
		return -CUI_ELEN;

	char name_24[25];
	if (pkg->pio->readvec(pkg, name_24, 24, 0, IO_SEEK_SET))
		return -CUI_EREADVEC;
	name_24[24] = 0;

	char name_16[17];
	memcpy(name_16, name_24, 16);
	name_16[16] = 0;

	if (pkg->pio->seek(pkg, 0, IO_SEEK_SET))
		return -CUI_ESEEK;

	u32 res_id, offset, unknown;
	vector<odn_entry_t> odn_index;
	if (memcmp(&name_24[16], "data", 4) && sscanf(name_24, "data%4d%8x%8x", &res_id, &offset, &unknown) == 3) {
		while (offset != odn_size) {
			if (pkg->pio->read(pkg, name_24, 24))
				return -CUI_EREAD;
			
			sscanf(name_24, "data%4d%8x%8x", &res_id, &offset, &unknown);
			odn_entry_t odn_entry;
			memcpy(odn_entry.name, name_24, 8);
			odn_entry.name[8] = 0;
			odn_entry.offset = offset;
			odn_index.push_back(odn_entry);
		}
	} else if ((sscanf(name_16, "back%4d%8x", &res_id, &offset) == 2)
			|| (sscanf(name_16, "hime%4d%8x", &res_id, &offset) == 2)
			|| (sscanf(name_16, "phii%4d%8x", &res_id, &offset) == 2)
			) {
		while (1) {
			if (pkg->pio->read(pkg, name_16, 16))
				return -CUI_EREAD;

			if (!memcmp(name_16, "END_ffffffffffff", 16))
				break;
			
			//sscanf(name_16, "back%4d%8x", &res_id, &offset);
			if (sscanf(name_16, "back%4d%8x", &res_id, &offset) != 2)
				if (sscanf(name_16, "hime%4d%8x", &res_id, &offset) != 2)
					sscanf(name_16, "phii%4d%8x", &res_id, &offset);

			odn_entry_t odn_entry;
			memcpy(odn_entry.name, name_16, 8);
			odn_entry.name[8] = 0;
			odn_entry.offset = offset;
			odn_index.push_back(odn_entry);
		}
		// make a dummy entry
		odn_entry_t odn_entry;
		odn_entry.offset = odn_size;
		odn_index.push_back(odn_entry);
		DWORD index_len = odn_index.size() * 16;
		for (DWORD i = 0; i < odn_index.size() - 1; ++i)
			odn_index[i].offset += index_len;
	} else if (sscanf(name_16, "data%4d%8x", &res_id, &offset) == 2) {
		DWORD index_entries = offset / 16;
		for (DWORD i = 0; i < index_entries; ++i) {
			if (pkg->pio->read(pkg, name_16, 16))
				return -CUI_EREAD;
			
			sscanf(name_16, "data%4d%8x", &res_id, &offset);
			odn_entry_t odn_entry;
			memcpy(odn_entry.name, name_16, 8);
			odn_entry.name[8] = 0;
			odn_entry.offset = offset;
			odn_index.push_back(odn_entry);
		}
		if (odn_index[odn_index.size() - 1].offset != odn_size) {
			// make a dummy entry
			odn_entry_t odn_entry;
			odn_entry.offset = odn_size;
			odn_index.push_back(odn_entry);
		}			
	} else if (sscanf(name_16, "hime%4d%8x", &res_id, &offset) == 2) {
		if (offset) {
			DWORD index_entries = offset / 16;
			for (DWORD i = 0; i < index_entries; i++) {
				if (pkg->pio->read(pkg, name_16, 16))
					return -CUI_EREAD;

				if (!memcmp(name_16, "HIME_END", 8)) {
					break;
				}
				
				sscanf(name_16, "hime%4d%8x", &res_id, &offset);
				odn_entry_t odn_entry;
				memcpy(odn_entry.name, name_16, 8);
				odn_entry.name[8] = 0;
				odn_entry.offset = offset;
				odn_index.push_back(odn_entry);
			}
			// make a dummy entry
			odn_entry_t odn_entry;
			odn_entry.offset = odn_size;
			odn_index.push_back(odn_entry);
		} else {
			while (1) {
				if (pkg->pio->read(pkg, name_16, 16))
					return -CUI_EREAD;

				if (!memcmp(name_16, "HIME_END", 8))
					break;
				
				sscanf(name_16, "hime%4d%8x", &res_id, &offset);
				odn_entry_t odn_entry;
				memcpy(odn_entry.name, name_16, 8);
				odn_entry.name[8] = 0;
				odn_entry.offset = offset;
				odn_index.push_back(odn_entry);
			}
			DWORD index_len = odn_index.size() * 16 + 8;
			for (DWORD i = 0; i < odn_index.size() - 1; ++i)
				odn_index[i].offset += index_len;
		}
	} else {
		BYTE key = 0xff;
		char name[16];
		memcpy(name, name_16, 16);
		for (DWORD i = 0; i < 16; i++)
			name[i] ^= key--;

		if (sscanf(name, "scrp%04d%8x", &res_id, &offset) == 2) {
			BYTE key = 0xff;
			while (1) {
				if (pkg->pio->read(pkg, name, 16))
					return -CUI_EREAD;

				for (DWORD i = 0; i < 16; ++i)
					name[i] ^= key--;

				if (!memcmp(name, "ffffffffffffffff", 16))
					break;
				
				sscanf(name, "scrp%4d%8x", &res_id, &offset);
				odn_entry_t odn_entry;
				memcpy(odn_entry.name, name, 8);
				odn_entry.name[8] = 0;
				odn_entry.offset = offset;
				odn_index.push_back(odn_entry);
			}
			// make a dummy entry
			odn_entry_t odn_entry;
			odn_entry.offset = odn_size;
			odn_index.push_back(odn_entry);
			DWORD index_len = odn_index.size() * 16;
			for (DWORD i = 0; i < odn_index.size() - 1; ++i)
				odn_index[i].offset += index_len;
		} else if (sscanf(name, "menu%04d%8x", &res_id, &offset) == 2) {
			BYTE key = 0xff;
			while (1) {
				if (pkg->pio->read(pkg, name, 16))
					return -CUI_EREAD;

				for (DWORD i = 0; i < 16; ++i)
					name[i] ^= key--;

				if (!memcmp(name, "ffffffffffffffff", 16))
					break;
				
				sscanf(name, "menu%4d%8x", &res_id, &offset);
				odn_entry_t odn_entry;
				memcpy(odn_entry.name, name, 8);
				odn_entry.name[8] = 0;
				odn_entry.offset = offset;
				odn_index.push_back(odn_entry);
			}
			// make a dummy entry
			odn_entry_t odn_entry;
			odn_entry.offset = odn_size;
			odn_index.push_back(odn_entry);
			DWORD index_len = odn_index.size() * 16;
			for (DWORD i = 0; i < odn_index.size() - 1; ++i)
				odn_index[i].offset += index_len;
		} else {
			printf("not supported\n");
			//exit(0);
			return -CUI_EMATCH;
		}
	}
	
	for (DWORD i = 0; i < odn_index.size(); ++i)
		odn_index[i].length = odn_index[i + 1].offset - odn_index[i].offset;

	DWORD index_buffer_length = sizeof(odn_entry_t) * (odn_index.size() - 1);
	odn_entry_t *index_buffer = new odn_entry_t[odn_index.size() - 1];
	if (!index_buffer)
		return -CUI_EMEM;

	for (i = 0; i < odn_index.size() - 1; ++i)
		index_buffer[i] = odn_index[i];

	pkg_dir->index_entries = odn_index.size() - 1;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(odn_entry_t);
	//pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int VALKYRIA_odn_parse_resource_info(struct package *pkg,
									struct package_resource *pkg_res)
{
	odn_entry_t *odn_entry;

	odn_entry = (odn_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, odn_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = odn_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = odn_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int VALKYRIA_odn_extract_resource(struct package *pkg,
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

	if (!strncmp((char *)raw, "OggS", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".ogg");
	} else if (!strncmp((char *)raw, "Bjj^", 4)) {
		for (DWORD i = 0; i < pkg_res->raw_data_length; ++i)
			raw[i] ^= 0x0d;
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".ogg");
	} else if (!strncmp(pkg_res->name, "scrp", 4) || !strncmp(pkg_res->name, "menu", 4)) {
		BYTE key = ~(pkg_res->offset & 0xff);
		for (DWORD i = 0; i < pkg_res->raw_data_length; ++i)		
			raw[i] ^= key--;
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".txt");
	} else if (!strncmp(pkg_res->name, "back", 4) || !strncmp(pkg_res->name, "phii", 4)) {
		DWORD width[2] = { 640, 800 };
		DWORD height[2] = { 480, 600 };

		for (DWORD k = 0; k < 2; ++k) {
			DWORD uncomprlen = width[k] * height[k] * 3;
			BYTE *uncompr = new BYTE[uncomprlen];
			if (!uncompr) {
				delete [] raw;
				return -CUI_EMEM;
			}

			DWORD curbyte = back_uncompress(uncompr, uncomprlen, 
				raw, pkg_res->raw_data_length);
			if (curbyte == pkg_res->raw_data_length || curbyte + 1 == pkg_res->raw_data_length) {
				// 不确定这个处理是否在640 * 480时有过
				for (DWORD i = 0; i < uncomprlen; ++i)
					uncompr[i] = (uncompr[i] * 255) >> 8;

				if (MyBuildBMPFile(uncompr, uncomprlen, NULL, 0, 
						width[k], height[k], 24, (BYTE **)&pkg_res->actual_data,
						&pkg_res->actual_data_length, my_malloc)) {
					delete [] uncompr;
					delete [] raw;
					return -CUI_EMEM;				
				}
				delete [] uncompr;
				pkg_res->flags |= PKG_RES_FLAG_REEXT;
				pkg_res->replace_extension = _T(".bmp");
				break;
			}
			delete [] uncompr;
		}
		if (k == 2) {
			delete [] raw;
			return -CUI_EMATCH;
		}
	} else if (!strncmp(pkg_res->name, "data", 4)) {
		DWORD width[2] = { 640, 800 };
		DWORD height[2] = { 480, 600 };

		for (DWORD k = 0; k < 2; ++k) {
			DWORD uncomprlen = width[k] * height[k] * 4;
			BYTE *uncompr = new BYTE[uncomprlen];
			if (!uncompr) {
				delete [] raw;
				return -CUI_EMEM;
			}

			DWORD curbyte = data_uncompress(uncompr, uncomprlen, 
				raw, pkg_res->raw_data_length);
			if (curbyte == pkg_res->raw_data_length || curbyte + 1 == pkg_res->raw_data_length) {
				// 不确定这个处理是否在640 * 480时有过
				for (DWORD i = 0; i < uncomprlen; ++i)
					uncompr[i] = (uncompr[i] * 255) >> 8;

				if (MyBuildBMPFile(uncompr, uncomprlen, NULL, 0, 
						width[k], height[k], 32, (BYTE **)&pkg_res->actual_data,
						&pkg_res->actual_data_length, my_malloc)) {
					delete [] uncompr;
					delete [] raw;
					return -CUI_EMEM;				
				}
				delete [] uncompr;
				pkg_res->flags |= PKG_RES_FLAG_REEXT;
				pkg_res->replace_extension = _T(".bmp");
				break;
			}
			delete [] uncompr;
		}
		if (k == 2) {
			delete [] raw;
			return -CUI_EMATCH;
		}
	} else if (!strncmp(pkg_res->name, "hime", 4)) {
		const TCHAR *freq_str = get_options2(_T("wav_freq"));
		DWORD freq = 44100;
		if (freq_str) {
			if (!_tcsicmp(freq_str, _T("22K")))
				freq = 22080;
		}

		if (MySaveAsWAVE(raw, pkg_res->raw_data_length, 
				1, 1, freq, 16, NULL, 0, (BYTE **)&pkg_res->actual_data,
				&pkg_res->actual_data_length, my_malloc)) {
			delete [] raw;
			return -CUI_EMEM;			
		}
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".wav");
	}
	pkg_res->raw_data = raw;

	return 0;
}

/* 资源保存函数 */
static int VALKYRIA_odn_save_resource(struct resource *res, 
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
static void VALKYRIA_odn_release_resource(struct package *pkg, 
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
static void VALKYRIA_odn_release(struct package *pkg, 
							struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation VALKYRIA_odn_operation = {
	VALKYRIA_odn_match,					/* match */
	VALKYRIA_odn_extract_directory,		/* extract_directory */
	VALKYRIA_odn_parse_resource_info,	/* parse_resource_info */
	VALKYRIA_odn_extract_resource,		/* extract_resource */
	VALKYRIA_odn_save_resource,			/* save_resource */
	VALKYRIA_odn_release_resource,		/* release_resource */
	VALKYRIA_odn_release				/* release */
};

/********************* ini *********************/

/* 封包匹配回调函数 */
static int VALKYRIA_ini_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包资源提取函数 */
static int VALKYRIA_ini_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	char length_str[5];
	DWORD offset;
	u32 length;
	BYTE *orig, *act;

	orig = (BYTE *)pkg->pio->readvec_only(pkg, pkg_res->raw_data_length, 0, IO_SEEK_SET);
	if (!orig)
		return -CUI_EREADVECONLY;

	act = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!act)
		return -CUI_EMEM;

	length_str[4] = 0;
	memcpy(length_str, orig, 4);
	length = atoi(length_str);
	*(u32 *)act = length;
	offset = 4;
	// title
	for (DWORD i = 0; i < length; i++)
		act[offset + i] = orig[offset + i] ^ 0xd1;
	offset += length;

	memcpy(length_str, &orig[offset], 4);
	length = atoi(length_str);
	*(u32 *)&act[offset] = length;
	offset += 4;
	// title
	for (i = 0; i < length; i++)
		act[offset + i] = orig[offset + i] ^ 0xd1;
	offset += length;

	if ((char)orig[offset] <= 0)
		; // 640 x 480
	else
		; // 800 x 600
	act[offset] = orig[offset];
	offset++;

	act[offset] = orig[offset] - '0';	// ??
	offset++;
	memcpy(&act[offset], &orig[offset], 48);
	offset += 48;			// ??

	pkg_res->raw_data = orig;
	pkg_res->actual_data = act;
	pkg_res->actual_data_length = offset;

	return 0;
}

/* 资源保存函数 */
static int VALKYRIA_ini_save_resource(struct resource *res, 
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
static void VALKYRIA_ini_release_resource(struct package *pkg, 
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
static void VALKYRIA_ini_release(struct package *pkg, 
							struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation VALKYRIA_ini_operation = {
	VALKYRIA_ini_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	VALKYRIA_ini_extract_resource,	/* extract_resource */
	VALKYRIA_ini_save_resource,		/* save_resource */
	VALKYRIA_ini_release_resource,	/* release_resource */
	VALKYRIA_ini_release			/* release */
};

#if 0
/********************* save *********************/

/* 封包匹配回调函数 */
static int VALKYRIA_save_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	u32 odn_size;
	if (pkg->pio->length_of(pkg, &odn_size)) {
		pkg->pio->close(pkg);
		return -CUI_ELEN;
	}

	if (odn_size != 0x8073) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包资源提取函数 */
static int VALKYRIA_save_extract_resource(struct package *pkg,
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

	for (DWORD i = 0; i < 20; ++i)
		raw[i] ^= 0x31;
	for (DWORD k = 0; k < 20; ++k)
		raw[i++] ^= 0x31;
	raw[i++] ^= 0x4d;
	raw[i++] ^= 0x4d;
	raw[i++] ^= 0x4d;
	for (k = 0; k < 32 * 1024; ++k)
		raw[i++] ^= 0x4d;
	raw[i] = !!(raw[i] ^= 0x4d);
	i++;
	raw[i] = !!(raw[i] ^= 0x53);
	i++;
	BYTE xor = 0x46;
	for (k = 0; k < 64; ++k)
		raw[i++] ^= xor++;
	raw[i++] ^= xor;
	raw[i] = !!(raw[i] ^= xor);
	i++;
	raw[i] = !!(raw[i] ^= xor);
	i++;
	raw[i++] = !!raw[i];
	raw[i++] = !!raw[i];
	raw[i] = !!raw[i];

	pkg_res->raw_data = raw;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation VALKYRIA_save_operation = {
	VALKYRIA_save_match,				/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	VALKYRIA_save_extract_resource,	/* extract_resource */
	VALKYRIA_odn_save_resource,		/* save_resource */
	VALKYRIA_odn_release_resource,	/* release_resource */
	VALKYRIA_odn_release				/* release */
};


/* 封包资源提取函数 */
static int VALKYRIA_setup_extract_resource(struct package *pkg,
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

	BYTE key = 13;
	for (DWORD i = 0; i < 0x6000; ++i) {
		raw[(i / 4) + (i % 4096)] ^= key;
		key += 13;
	}

	// read 0x2280
	// read 1f400
}

/* 封包处理回调函数集合 */
static cui_ext_operation VALKYRIA_setup_operation = {
	VALKYRIA_setup_match,			/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	VALKYRIA_setup_extract_resource,/* extract_resource */
	VALKYRIA_odn_save_resource,		/* save_resource */
	VALKYRIA_odn_release_resource,	/* release_resource */
	VALKYRIA_odn_release			/* release */
};
#endif

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK VALKYRIA_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".odn"), NULL, 
		NULL, &VALKYRIA_odn_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

//	if (callback->add_extension(callback->cui, _T(".odn"), _T(".sav"), 
//		NULL, &VALKYRIA_save_operation, CUI_EXT_FLAG_PKG))
//			return -1;

	if (callback->add_extension(callback->cui, _T(".ini"), _T(".ini.dump"), 
		NULL, &VALKYRIA_ini_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &VALKYRIA_odn_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".file"), NULL, 
		NULL, &VALKYRIA_odn_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	return 0;
}
