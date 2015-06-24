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
#include <crass/locale.h>

/*
附加，找4D434653，往下就看到了
 */
//Q:\Program Files\埫崟寑応\堹朶偺榝惎乣攋夡偲梸朷偺徴摦乣
//Q:\Program Files\搷怓寑応\彮彈払偺偝偊偢傝
//Q:\Program Files\埫崟寑応\愨朷彮彈亅暅廞偺惈嵸嫵幒亅

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information DAC_cui_information = {
	_T("Exception(エクセプション)"),/* copyright */
	_T("ＤＡＣ"),					/* system */
	_T(".dpk"),						/* package */
	_T("0.8.1"),					/* revision */
	_T("痴漢公賊"),					/* author */
	_T("2009-6-13 8:56"),			/* date */
	NULL,							/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {		// 0x18 bytes
	s8 magic[4];		// "DPK"
	u32 reserved;		// 0
	u32 data_offset;
	u32 dpk_size;
	u32 index_entries;
} dpk_header_t;

typedef struct {
	s8 magic[4];		// "DGC"
	u32 colors;			// 最高字节是flags: bit0 - ?; bit1 - palette; bit2 - mask?; bit3-7 - ??
	u16 width;
	u16 height;
} DGC_header_t;
#pragma pack ()

typedef struct {
	s8 name[MAX_PATH];
	u32 reserved;
	u32 offset;
	u32 length;
} dpk_entry_t;


static const TCHAR *simplified_chinese_strings[] = {
	_T("%s: 没有指定特殊参数game，使用默认解密算法，具体参见DAC.txt。\n"),
};

static const TCHAR *traditional_chinese_strings[] = {
	_T("沒有指定特殊參數game，使用默認解密算法，具體參見DAC.txt。\n"),
};

static const TCHAR *japanese_strings[] = {
	_T("%s: 特殊パラメーターgameが指定されていませんので、デフォルト解読算法を行います、詳しくはDAC.txtを見てください。\n"),
};

static const TCHAR *default_strings[] = {
	_T("%s: don\'t specify the special parameter game to extract, using default decryption method, see DAC.txt for details.\n"),
};

static struct locale_configuration DAC_locale_configurations[4] = {
	{
		936, simplified_chinese_strings, 1
	},
	{
		950, traditional_chinese_strings, 1
	},
	{
		932, japanese_strings, 1
	},
	{
		0, default_strings, 1
	},
};

static int DAC_locale_id;

struct game_config {
	char *name;
	void (*decoder)(char *name, int name_len, BYTE *enc, DWORD enc_len);
} dpk_game_t;

static void default_decoder(char *name, int name_len, BYTE *enc, DWORD enc_len)
{
	int EDX = 0;
	for (int i = name_len - 1; i >= 0; i--)
		EDX += 0x43E78A5C * (name[i] + enc_len) + 0xFF98;
		
	int ESI = 0;
	s32 ECX = ESI * 0x43E78A5C + 0xFF98;
	for (DWORD k = 0; k < enc_len; k++) {
		int EBX = (BYTE)((ECX >> 8) + ECX);
		ECX += 0x43E78A5C;
		enc[k] = (enc[k] ^ EBX) - (BYTE)EDX;
	}
}

static void anti_decoder(char *name, int name_len, BYTE *enc, DWORD enc_len)
{
	int EDX = 0;
	for (int i = name_len - 1; i >= 0; --i)
		EDX += 0xC3BD + (0x577D4861 * (name[i] + enc_len));
		
	int ESI = 0;
	int ECX = 0xC3BD + ESI * 0x577D4861;
	for (DWORD k = 0; k < enc_len; ++k) {
		int EBX = (ECX >> 8) + ECX;
		ECX += 0x577D4861;
		enc[k] = (EBX ^ enc[k]) - (BYTE)EDX;
	}
}

static void tritri_decoder(char *name, int name_len, BYTE *enc, DWORD enc_len)
{
	int EDX = 0;
	for (int i = name_len - 1; i >= 0; --i)
		EDX += 0x9F59 - (0x5DDE9B8D * (name[i] + enc_len));
		
	int ESI = 0;
	int ECX = 0x9F59 - ESI * 0x5DDE9B8D;
	for (DWORD k = 0; k < enc_len; ++k) {
		int EBX = (ECX >> 8) + ECX;
		ECX -= 0x5DDE9B8D;
		enc[k] = (EBX ^ enc[k]) - (BYTE)EDX;
	}
}

static void ingoku_decoder(char *name, int name_len, BYTE *enc, DWORD enc_len)
{
	int EDX = 0;
	for (int i = name_len - 1; i >= 0; --i)
		EDX += 0x4D49 + (0x39712FED * (name[i] + enc_len));
		
	int ESI = 0;
	int ECX = 0x4D49 + ESI * 0x39712FED;
	for (DWORD k = 0; k < enc_len; ++k) {
		int EBX = (ECX >> 8) + ECX;
		ECX += 0x39712FED;
		enc[k] = (EBX ^ enc[k]) - (BYTE)EDX;
	}
}

static struct game_config game_config[] = {
	{
		"ingoku", 
		ingoku_decoder
	},
	{
		"tritri", 
		tritri_decoder
	},
	{
		"anti", 
		anti_decoder
	},
	{
		"wana", 
		default_decoder
	},
	{
		NULL, 
		NULL
	}
};

/********************* dpk *********************/

/* 封包匹配回调函数 */
static int DAC_dpk_match(struct package *pkg)
{
	DWORD cfg = -1;
	const char *game = get_options("game");

	if (!game)
		locale_app_printf(DAC_locale_id, 0, pkg->name);
	else {
		for (cfg = 0; game_config[cfg].name; ++cfg) {
			if (!strcmp(game_config[cfg].name, game))
				break;
		}
		if (!game_config[cfg].name)
			return -CUI_EMATCH;
	}

	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "DPK", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	package_set_private(pkg, cfg);

	return 0;	
}

/* 封包索引目录提取函数 */
static int DAC_dpk_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{
	dpk_header_t dpk_header;

	if (pkg->pio->readvec(pkg, &dpk_header, sizeof(dpk_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	BYTE *enc = (BYTE *)&dpk_header;
	BYTE tmp;
	for (DWORD i = 8; i < 16; i++) {
		tmp = enc[i];
		enc[i] ^= i - 16;
	}	

	for (; i < sizeof(dpk_header_t); i++) {
		DWORD xor = i + tmp;
		tmp = enc[i];
		enc[i] ^= xor;
	}

	DWORD index_length = dpk_header.data_offset - sizeof(dpk_header_t);
	BYTE *index_buffer = new BYTE[index_length];
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_length)) {
		delete [] index_buffer;
		return -CUI_EREADVEC;
	}

	for (; i < dpk_header.data_offset; i++) {
		DWORD xor = i + tmp;
		tmp = index_buffer[i - sizeof(dpk_header_t)];
		index_buffer[i - sizeof(dpk_header_t)] ^= xor;
	}

	DWORD my_index_length = dpk_header.index_entries * sizeof(dpk_entry_t);
	dpk_entry_t *my_index_buffer = new dpk_entry_t[my_index_length]();
	if (!my_index_buffer) {
		delete [] index_buffer;
		return -CUI_EMEM;
	}

	u32 *entry_offset_table = (u32 *)index_buffer;
	for (i = 0; i < dpk_header.index_entries; i++) {
		BYTE *p_entry = index_buffer + 4 * dpk_header.index_entries + entry_offset_table[i];
		my_index_buffer[i].offset = *(u32 *)p_entry + dpk_header.data_offset;
		p_entry += 4;
		my_index_buffer[i].length = *(u32 *)p_entry;
		p_entry += 4;
		my_index_buffer[i].reserved = *(u32 *)p_entry;	// 0
		p_entry += 4;
		strcpy(my_index_buffer[i].name, (char *)p_entry);
	}
	delete [] index_buffer;

	pkg_dir->index_entries = dpk_header.index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = my_index_length;
	pkg_dir->index_entry_length = sizeof(dpk_entry_t);
	//package_set_private(pkg, dpk_header.data_offset);

	return 0;
}

/* 封包索引项解析函数 */
static int DAC_dpk_parse_resource_info(struct package *pkg,
									   struct package_resource *pkg_res)
{
	dpk_entry_t *dpk_entry = (dpk_entry_t *)pkg_res->actual_index_entry;

	strcpy(pkg_res->name, dpk_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = dpk_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = dpk_entry->offset;

	return 0;
}

#if 0
static void DGC_uncompress(BYTE &*uncompr, BYTE *compr, DWORD comprlen, BYTE *palette)
{
 	DWORD count;

	for (DWORD curbyte = 0; j < curbyte < comprlen; ) {
		count = compr[curbyte++];

		if (count) {
			BYTE *pal = palette + 3 * compr[curbyte++];
			BYTE b, g, r;

			b = *pal++;
			g = *pal++;
			r = *pal;
			for (DWORD i = 0; i < count; ++i) {
				*uncompr++ = b;
				*uncompr++ = g;
				*uncompr++ = r;
			}
		} else {
			count = compr[curbyte++];

			if (count) {				
				count += 2;

				for (DWORD i = 0; i < count; ++i) {
					BYTE *pal = palette + 3 * compr[curbyte++];

					*uncompr++ = pal[0];
					*uncompr++ = pal[1];
					*uncompr++ = pal[2];
				}
			} else {
				count = (count << 8) | compr[curbyte++];
				DWORD offset = count >> 6;
				count = (count & 0x3f) + 4;

				for (DWORD i = 0; i < count; ++i) {
					*uncompr = uncompr[-offset + 0];
					++uncompr;
					*uncompr = uncompr[-offset + 1];
					++uncompr;
					*uncompr = uncompr[-offset + 2];
					++uncompr;
				}
			}
		}
	}


}
#endif

/* 封包资源提取函数 */
static int DAC_dpk_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	BYTE *raw = new BYTE[pkg_res->raw_data_length];
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET)) {
		delete [] raw;
		return -CUI_EREADVEC;
	}

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = raw;
		return 0;
	}

	char *name = PathFindFileNameA(pkg_res->name);
	int nlen = strlen(name);

	if (name[nlen - 1] == 'z') {
		name[nlen - 1] = 0;
		nlen--;

		DWORD cfg = package_get_private(pkg);
		if (cfg == -1 || !lstrcmpi(pkg->name, _T("uninstall.dpk"))
				|| !lstrcmpi(pkg->name, _T("install.dpk"))
				|| !lstrcmpi(pkg->name, _T("plugin.dpk"))
				) {	
			default_decoder(name, nlen, raw, pkg_res->raw_data_length);
		} else {
			game_config[cfg].decoder(name, nlen, 
				raw, pkg_res->raw_data_length);
		}
	}

#if 1
	if (!strncmp((char *)raw, "DGC", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".DGC");
	}
#else
	if (!strncmp((char *)raw, "DGC", 4)) {
		DGC_header_t *DGC = (DGC_header_t *)raw;
		BYTE flags = DGC->colors >> 24;
		BYTE *uncompr = (BYTE *)malloc(DGC->width * DGC->height * 3);
		if (!uncompr) {
			free(raw);
			return -CUI_EMEM;
		}


		if (flags & 4) {

		} else {

		}

		BYTE *p = raw + sizeof(DGC_header_t);
		BYTE *palette;
		BYTE *act_p = uncompr;

		if ((colors <= 256) && (flags & 2)) {
			BYTE _colors = *p++ + 1;
			palette = p;
			p += _colors * 3;

			do {
				if (flags & 1) {
					s16 comprlen = *(u16 *)p;
					p += 2;

					if (comprlen) {
						if (comprlen > -1) {
							DGC_uncompress(act_p, p, comprlen, palette);
							p += comprlen;
						} else {
							DWORD count = 3 * DGC->width;
							for (DWORD i = 0; i < count; ++i) {
								*act_p = act_p[3 * comprlen];
								++act_p;
							}
						}
					} else {

					}

					


				} else {

				}

		}
	}
#endif
	pkg_res->raw_data = raw;

	return 0;
}

/* 资源保存函数 */
static int DAC_dpk_save_resource(struct resource *res, 
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
static void DAC_dpk_release_resource(struct package *pkg, 
									 struct package_resource *pkg_res)
{
	if (pkg_res->raw_data) {
		delete [] pkg_res->raw_data;
		pkg_res->raw_data = NULL;
	}
}

/* 封包卸载函数 */
static void DAC_dpk_release(struct package *pkg, 
							struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation DAC_dpk_operation = {
	DAC_dpk_match,				/* match */
	DAC_dpk_extract_directory,	/* extract_directory */
	DAC_dpk_parse_resource_info,/* parse_resource_info */
	DAC_dpk_extract_resource,	/* extract_resource */
	DAC_dpk_save_resource,		/* save_resource */
	DAC_dpk_release_resource,	/* release_resource */
	DAC_dpk_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK DAC_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".dpk"), NULL, 
		NULL, &DAC_dpk_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	DAC_locale_id = locale_app_register(DAC_locale_configurations, 3);

	return 0;
}
