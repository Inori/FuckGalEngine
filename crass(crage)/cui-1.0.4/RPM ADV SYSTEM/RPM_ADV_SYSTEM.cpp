#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <utility.h>
#include <crass/locale.h>
#include <stdio.h>

/* 密码发现方法：
 * 找字符串引用while的代码，找到使用while作为参数的函数的地址，将该地址
 * 作为常量搜索，另外一处调用的参数就是code。
 */

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information RPM_ADV_SYSTEM_cui_information = {
	_T("rpm ENTERTAINMENT CREATIONAL STUDIO"),	/* copyright */
	_T("RPM ADV SYSTEM"),	/* system */
	_T(".arc .bmp"),		/* package */
	_T("1.2.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-3-4 23:13"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	u32 index_entries;
	u32 is_compressed; 
} arc_header_t;

typedef struct {
	s8 name[32];
	u32 uncomprLen;
	u32 comprLen;
	u32 offset;
} arc_entry_t;

typedef struct {
	s8 name[24];
	u32 uncomprLen;
	u32 comprLen;
	u32 offset;
} arc_entry_old_t;
#pragma pack ()


static const TCHAR *simplified_chinese_strings[] = {
	_T("自动猜测结果: code=%s\n"),
	_T("自动猜测结果: code=%s,old\n"),
};

static const TCHAR *traditional_chinese_strings[] = {
	_T("自動猜測結果: code=%s\n"),
	_T("自動猜測結果: code=%s,old\n"),
};

static const TCHAR *japanese_strings[] = {
	_T("自動推測結果: code=%s\n"),
	_T("自動推測結果: code=%s,old\n"),
};

static const TCHAR *default_strings[] = {
	_T("automatical guessing: code=%s\n"),
	_T("automatical guessing: code=%s,old\n"),
};

static struct locale_configuration app_locale_configurations[4] = {
	{
		936, simplified_chinese_strings, 2
	},
	{
		950, traditional_chinese_strings, 2
	},
	{
		932, japanese_strings, 2
	},
	{
		0, default_strings, 2
	},
};

static int app_locale_id;

static void *my_malloc(DWORD len)
{
	return new BYTE[len];
}

static int old_version = 0;
static char decode_string[256];

static inline unsigned char getbit_le(unsigned char byte, unsigned int pos)
{
	return !!(byte & (1 << pos));
}

static void lzss_decompress(unsigned char *uncompr, DWORD *uncomprlen, 
							unsigned char *compr, DWORD comprlen)
{
	unsigned int act_uncomprlen = 0;
	/* compr中的当前字节中的下一个扫描位的位置 */
	unsigned int curbit = 0;
	/* compr中的当前扫描字节 */
	unsigned int curbyte = 0;
	unsigned int nCurWindowByte = 0xfee;
	unsigned int win_size = 4096;
	BYTE win[4096];
	
	memset(win, 0, sizeof(win));
	while (1) {
		/* 如果为0, 表示接下来的1个字节原样输出 */
		BYTE flag;

		if (curbyte >= comprlen)
			break;

		flag = compr[curbyte++];
		for (curbit = 0; curbit < 8; curbit++) {
			if (getbit_le(flag, curbit)) {
				unsigned char data;

				if (curbyte >= comprlen)
					goto out;

				if (act_uncomprlen >= *uncomprlen)
					goto out;

				data = compr[curbyte++];
				uncompr[act_uncomprlen++] = data;
				/* 输出的1字节放入滑动窗口 */
				win[nCurWindowByte++] = data;
				nCurWindowByte &= win_size - 1;
			} else {
				unsigned int copy_bytes, win_offset;
				unsigned int i;

				if (curbyte >= comprlen)
					goto out;
				win_offset = compr[curbyte++];

				if (curbyte >= comprlen)
					goto out;
				copy_bytes = compr[curbyte++];
				win_offset |= (copy_bytes >> 4) << 8;
				copy_bytes &= 0x0f;
				copy_bytes += 3;

				for (i = 0; i < copy_bytes; i++) {	
					unsigned char data;

					if (act_uncomprlen >= *uncomprlen)
						goto out;

					data = win[(win_offset + i) & (win_size - 1)];
					uncompr[act_uncomprlen++] = data;		
					/* 输出的1字节放入滑动窗口 */
					win[nCurWindowByte++] = data;
					nCurWindowByte &= win_size - 1;	
				}
			}
		}
	}
out:
	*uncomprlen = act_uncomprlen;
}

static void decode(BYTE *buf, DWORD buf_len)
{
	unsigned int code_len = strlen(decode_string);

	for (unsigned int i = 0; i < buf_len; i++)
		buf[i] += decode_string[i % code_len];
}

static int __get_code(const BYTE *buf, DWORD buf_len, const BYTE *plain)
{
	const BYTE *crypt = &buf[buf_len - 4];
	BYTE code[4];
	BYTE r_code[4];

	for (int i = 0; i < 4; ++i) {
		code[i] = plain[i] - crypt[i];
		r_code[i] = 0 - code[i];
	}

	DWORD name_len = buf_len - 12;
	DWORD pos[2];
	int find = 0;
	for (i = name_len - 4; i >= 0; --i) {
		if (!memcmp(buf + i, r_code, 4)) {
			pos[find++] = i;
			if (find == 2)
				break;
		}
	}
	if (find != 2)
		return -1;

	int code_len = pos[0] - pos[1];
	for (i = 0; i < code_len; ++i)
		decode_string[(pos[1] + i) % code_len] = 0 - buf[pos[1] + i];
	decode_string[i] = 0;

	return 0;
}

static int get_code(const BYTE *buf, DWORD index_entries)
{
	DWORD entry_size[] = { 44, 36, 0 }; 

	for (DWORD j = 0; entry_size[j]; ++j) {
		u32 offset = sizeof(arc_header_t) + index_entries * entry_size[j];
		if (!__get_code(buf, entry_size[j], (u8 *)&offset))
			break;
	}
	if (!entry_size[j])
		return -1;
	old_version = !j ? 0 : 1;
	return 0;
}

/********************* arc *********************/

/* 封包匹配回调函数 */
static int RPM_ADV_SYSTEM_arc_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	arc_header_t arc_header;
	if (pkg->pio->read(pkg, &arc_header, sizeof(arc_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	BYTE entry[44];	// 最大项长度
	if (pkg->pio->read(pkg, entry, sizeof(entry))) {
		if (pkg->pio->read(pkg, entry, 36)) {
			pkg->pio->close(pkg);
			return -CUI_EREAD;
		}
		memset(entry + 36, 0, 8);
	}

	old_version = !!get_options("old");
	const char *dec_string = get_options("code");
	if (!dec_string) {
		if (!lstrcmp(pkg->name, _T("instdata.arc")))
			strcpy(decode_string, "inst");
		else if (!lstrcmp(pkg->name, _T("system.arc")))
			strcpy(decode_string, "while");
		else if (!get_code(entry, arc_header.index_entries)) {
			TCHAR str[MAX_PATH];

			acp2unicode(decode_string, -1, str, SOT(str));
			if (!old_version)
				locale_app_printf(app_locale_id, 0, str);
			else
				locale_app_printf(app_locale_id, 1, str);
		} else {
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}
	} else
		strcpy(decode_string, dec_string);

	return 0;	
}

/* 封包索引目录提取函数 */
static int RPM_ADV_SYSTEM_arc_extract_directory(struct package *pkg,
												struct package_directory *pkg_dir)
{
	arc_header_t arc_header;
	arc_entry_t *index_buffer;
	unsigned int index_buffer_len;	
	unsigned int i;

	if (pkg->pio->readvec(pkg, &arc_header, sizeof(arc_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	if (old_version)
		index_buffer_len = arc_header.index_entries * sizeof(arc_entry_old_t);
	else
		index_buffer_len = arc_header.index_entries * sizeof(arc_entry_t);
	index_buffer = (arc_entry_t *)malloc(index_buffer_len);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_len)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	decode((BYTE *)index_buffer, index_buffer_len);

	if (old_version) {
		arc_entry_t *_new;
		arc_entry_old_t *old = (arc_entry_old_t *)index_buffer;

		index_buffer_len = arc_header.index_entries * sizeof(arc_entry_t);
		_new = (arc_entry_t *)malloc(index_buffer_len);
		if (!_new) {
			free(index_buffer);
			return -CUI_EMEM;
		}

		for (i = 0; i < arc_header.index_entries; i++) {
			strcpy(_new[i].name, old[i].name);
			_new[i].uncomprLen = old[i].uncomprLen;
			_new[i].comprLen = old[i].comprLen;
			_new[i].offset = old[i].offset;
		}
		free(old);
		index_buffer = _new;
	}
	pkg_dir->index_entries = arc_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_len;
	pkg_dir->index_entry_length = sizeof(arc_entry_t);
	package_set_private(pkg, (void *)arc_header.is_compressed);

	return 0;
}

/* 封包索引项解析函数 */
static int RPM_ADV_SYSTEM_arc_parse_resource_info(struct package *pkg,
												  struct package_resource *pkg_res)
{
	arc_entry_t *arc_entry;

	arc_entry = (arc_entry_t *)pkg_res->actual_index_entry;
	if (old_version) {
		if (!arc_entry->name[23])
			strcpy(pkg_res->name, arc_entry->name);
		else
			strncpy(pkg_res->name, arc_entry->name, 24);
	} else {
		if (!arc_entry->name[31])
			strcpy(pkg_res->name, arc_entry->name);
		else
			strncpy(pkg_res->name, arc_entry->name, 32);
	}
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = arc_entry->comprLen;
	pkg_res->actual_data_length = arc_entry->uncomprLen;
	pkg_res->offset = arc_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int RPM_ADV_SYSTEM_arc_extract_resource(struct package *pkg,
											   struct package_resource *pkg_res)
{
	u32 is_compressed;
	BYTE *uncompr, *compr, *actdata;
	DWORD actlen;

	is_compressed = (u32)package_get_private(pkg);

	compr = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	if (is_compressed) {
		uncompr = (BYTE *)malloc(pkg_res->actual_data_length);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}

		actlen = pkg_res->actual_data_length; 
		lzss_decompress(uncompr, &actlen, compr, pkg_res->raw_data_length);
		if (actlen != pkg_res->actual_data_length) {
			free(uncompr);
			free(compr);
			return -CUI_EUNCOMPR;
		}
		free(compr);
		compr = NULL;
		actdata = uncompr;
	} else {
		uncompr = NULL;
		actlen = pkg_res->raw_data_length;
		actdata = compr;
	}

	/* 对于较长的资源名称，后缀名有时候会被揭短 */
	if (!strncmp((char *)actdata, "BM", 2)) {
		BITMAPFILEHEADER *bmfh = (BITMAPFILEHEADER *)actdata;	
		BITMAPINFOHEADER *bmif = (BITMAPINFOHEADER *)(bmfh + 1);
		if (bmif->biBitCount == 24) {
			DWORD bmp32_size = bmif->biWidth * bmif->biHeight * 4 
					+ sizeof(BITMAPINFOHEADER) + sizeof(BITMAPFILEHEADER) + 2;
			DWORD bmp24_size = ((bmif->biWidth * 3 + 3) & ~3) * 3
					+ sizeof(BITMAPINFOHEADER) + sizeof(BITMAPFILEHEADER) + 2;
			if (bmp24_size > actlen && bmp32_size >= actlen) {
				DWORD rgba_size = bmif->biWidth * bmif->biHeight * 4;
				BYTE *rgba = new BYTE[rgba_size];
				if (!rgba) {
					free(actdata);
					return -CUI_EMEM;
				}

				DWORD align = (4 - ((bmif->biWidth * 3) & 3)) & 3;
				BYTE *src = actdata + bmfh->bfOffBits;
				BYTE *dst = rgba;
				BYTE *a = actdata + bmfh->bfSize;
				for (int y = 0; y < bmif->biHeight; ++y) {
					for (int x = 0; x < bmif->biWidth; ++x) {
						*dst++ = *src++;
						*dst++ = *src++;
						*dst++ = *src++;
						*dst++ = *a++;
					}
					src += align;
				}

				BYTE *ret;
				DWORD ret_size;
				if (MyBuildBMPFile(rgba, rgba_size, NULL, 0, bmif->biWidth,
						bmif->biHeight, 32, &ret, &ret_size, my_malloc)) {
					delete [] rgba;
					delete [] actdata;
					return -CUI_EMEM;
				}
				delete [] rgba;
				delete [] actdata;
				uncompr = ret;
				pkg_res->actual_data_length = ret_size;
			}
		}
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".BMP");
	}

	if (strstr(pkg_res->name, ".lib")) {
		const char *key = "偰偡偲";	// てすと

		for (DWORD i = 0, k = 0; i < actlen; i++) {
			actdata[i] -= key[k++];
			if (k <= strlen(key))
				k = 0;
		}
	}

	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;

	return 0;
}

/* 资源保存函数 */
static int RPM_ADV_SYSTEM_arc_save_resource(struct resource *res, 
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
static void RPM_ADV_SYSTEM_arc_release_resource(struct package *pkg, 
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
static void RPM_ADV_SYSTEM_arc_release(struct package *pkg, 
									   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation RPM_ADV_SYSTEM_arc_operation = {
	RPM_ADV_SYSTEM_arc_match,					/* match */
	RPM_ADV_SYSTEM_arc_extract_directory,		/* extract_directory */
	RPM_ADV_SYSTEM_arc_parse_resource_info,		/* parse_resource_info */
	RPM_ADV_SYSTEM_arc_extract_resource,		/* extract_resource */
	RPM_ADV_SYSTEM_arc_save_resource,			/* save_resource */
	RPM_ADV_SYSTEM_arc_release_resource,		/* release_resource */
	RPM_ADV_SYSTEM_arc_release					/* release */
};

/********************* bmp *********************/

/* 封包匹配回调函数 */
static int RPM_ADV_SYSTEM_bmp_match(struct package *pkg)
{
	s8 magic[2];
	u32 mpg_magic;
	u32 replace_bytes;
	u32 bmp_size;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "BM", 2)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->pio->read(pkg, &replace_bytes, 4)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (pkg->pio->length_of(pkg, &bmp_size)) {
		pkg->pio->close(pkg);
		return -CUI_ELEN;
	}
	
	if (pkg->pio->readvec(pkg, &mpg_magic, 4, bmp_size - replace_bytes, IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_EREADVEC;
	}
	
	if (mpg_magic != 0xba010000 && mpg_magic != 0xb3010000) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包资源提取函数 */
static int RPM_ADV_SYSTEM_bmp_extract_resource(struct package *pkg,
											   struct package_resource *pkg_res)
{
	u32 replace_bytes;
	u32 bmp_size;
	BYTE *raw, *act;

	if (pkg->pio->length_of(pkg, &bmp_size))
		return -CUI_ELEN;

	if (pkg->pio->readvec(pkg, &replace_bytes, 4, 2, IO_SEEK_SET))
		return -CUI_EREADVEC;

	act = (BYTE *)malloc(replace_bytes);
	if (!act)
		return -CUI_EMEM;

	bmp_size -= replace_bytes;
	if (pkg->pio->readvec(pkg, act, replace_bytes, bmp_size, IO_SEEK_SET)) {
		free(act);
		return -CUI_EREADVEC;
	}

	bmp_size -= replace_bytes;
	raw = (BYTE *)pkg->pio->readvec_only(pkg, bmp_size, replace_bytes, IO_SEEK_SET);
	if (!raw) {
		free(act);
		return -CUI_EREADVECONLY;
	}

	pkg_res->raw_data = raw;
	pkg_res->raw_data_length = bmp_size;
	pkg_res->actual_data = act;
	pkg_res->actual_data_length = replace_bytes;

	return 0;
}

/* 资源保存函数 */
static int RPM_ADV_SYSTEM_bmp_save_resource(struct resource *res, 
											struct package_resource *pkg_res)
{
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (res->rio->write(res, pkg_res->actual_data, pkg_res->actual_data_length)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}

	if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}

	res->rio->close(res);
	
	return 0;
}

/* 封包资源释放函数 */
static void RPM_ADV_SYSTEM_bmp_release_resource(struct package *pkg, 
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
static void RPM_ADV_SYSTEM_bmp_release(struct package *pkg, 
									   struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation RPM_ADV_SYSTEM_bmp_operation = {
	RPM_ADV_SYSTEM_bmp_match,				/* match */
	NULL,									/* extract_directory */
	NULL,									/* parse_resource_info */
	RPM_ADV_SYSTEM_bmp_extract_resource,	/* extract_resource */
	RPM_ADV_SYSTEM_bmp_save_resource,		/* save_resource */
	RPM_ADV_SYSTEM_bmp_release_resource,	/* release_resource */
	RPM_ADV_SYSTEM_bmp_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK RPM_ADV_SYSTEM_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".arc"), NULL, 
		NULL, &RPM_ADV_SYSTEM_arc_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".bmp"), _T(".mpg"), 
		NULL, &RPM_ADV_SYSTEM_bmp_operation, CUI_EXT_FLAG_PKG))
			return -1;

	app_locale_id = locale_app_register(app_locale_configurations, 3);

	return 0;
}
