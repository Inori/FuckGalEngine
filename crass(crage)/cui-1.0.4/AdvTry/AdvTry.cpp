#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <utility.h>
#include <stdio.h>
#include <crass/locale.h>

/* 寻找offset和key的方法
 现搜索字符串"ARCHIVE",再往上找下面的数据
 第一次读取8字节的位置就是init_offset
 读取0x24字节header的位置就是sec_offset
 2次期间的xor用的就是key

用这个关键字在exe中定位key：
  iskanji:unknown Kanji code type(在其下面几行）
或者key字段写为ARCHIVE，逆推出string

00417DC2  |.  6A 04         PUSH    4
00417DC4  |.  51            PUSH    ECX
00417DC5  |.  E8 33150300   CALL    Adv.004492FD 《－－读code
00417DCA  |.  53            PUSH    EBX                              ; |Arg4
00417DCB  |.  6A 24         PUSH    24                               ; |Arg3 = 00000024
00417DCD  |.  8D9424 8C0000>LEA     EDX, DWORD PTR SS:[ESP+8C]       ; |
00417DD4  |.  6A 01         PUSH    1                                ; |Arg2 = 00000001
00417DD6  |.  52            PUSH    EDX                              ; |Arg1
00417DD7  |.  E8 21150300   CALL    Adv.004492FD                     ; \读header
00417DDC  |.  8B3D FCD74700 MOV     EDI, DWORD PTR DS:[47D7FC]       ;  《－key
 */
struct acui_information AdvTry_cui_information = {
	_T(""),					/* copyright */
	_T(""),					/* system */
	_T(".dat"),				/* package */
	_T("1.1.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-6-30 22:20"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};	

#pragma pack (1)
typedef struct {
	s8 magic[12];		/* "ARCHIVE" */
	u32 header_length;
	u32 index_offset;		
	u32 index_entry_length;
	u32 reserved[2];
	u32 index_entries;
} dat_header_t;

typedef struct {
	u8 zero;
	s8 name[263];
	u32 offset;
	u32 length;
	u32 reserved;
} dat_entry_t;
#pragma pack ()

typedef struct {
	dat_header_t dat_header;
	BYTE code[8];
	const char key[8];
	DWORD init_offset;
	DWORD sec_offset;
} AdvTry_game_context_t;

static const TCHAR *simplified_chinese_strings[] = {
	_T("自动猜测结果: key=%s, init_offset=0x%x, sec_offset=0x%x\n"),
};

static const TCHAR *traditional_chinese_strings[] = {
	_T("自動猜測結果: key=%s, init_offset=0x%x, sec_offset=0x%x\n"),
};

static const TCHAR *japanese_strings[] = {
	_T("自動推測結果: key=%s, init_offset=0x%x, sec_offset=0x%x\n"),
};

static const TCHAR *default_strings[] = {
	_T("automatical guessing: key=%s, init_offset=0x%x, sec_offset=0x%x\n"),
};

static struct locale_configuration app_locale_configurations[4] = {
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

static int app_locale_id;

#if 0
#define AdvTry_games		11

/*
每个游戏需要提供3个参数才能正确提取：init_offset、sec_offset和key。前2个是2个偏移值，最后一个是解密字符串。
本cui目前已经集成了几个游戏的上述参数，所以在提取以下游戏时，无需再提供这3个参数。
·Clover Point
init_offset=0x11,sec_offset=0x46,key=ClOVeRrE
·Clover Point 体验版
init_offset=0x1c,sec_offset=0x45,key=CVPTTRY1
·ちょこっと☆ばんぱいあ! 
init_offset=0x35,sec_offset=0x51,key=CHOKOPAI
·ちょこっと☆ばんぱいあ! 体验版
init_offset=0x11,sec_offset=0x49,key=CPTRYVER
·恋Q！～恋とHの乱れ射ち～ 体验版
init_offset=0x08,sec_offset=0x10,key=KOIQ_WEB
·恋Q！～恋とHの乱れ射ち～
init_offset=0x08,sec_offset=0x10,key=RE_MAJIQ
·神樹の館 体验版
init_offset=0x00,sec_offset=0x08,key=METEOR01
·月と魔法と太陽と
init_offset=0x08,sec_offset=0x10,key=TrpAdTRY
·綾瀬家のオンナ ～淫華の血脈～
init_offset=0x0c,sec_offset=0x15,key=-AYASEKE

如果本文档没有记录你要提取的游戏，可以指定这3个参数来提取。

0 - Clover Point
1 - Clover Point 体验版
2 - ちょこっと☆ばんぱいあ! 体验版
3 - 恋Q！～恋とHの乱れ射ち～ 体验版
4 - 恋Q！～恋とHの乱れ射ち～
5 - 神樹の館 体验版
6 - 月と魔法と太陽と 体验版
7 - 綾瀬家のオンナ ～淫華の血脈～
8 - ちょこっと☆ばんぱいあ! 
9 - 代々木人妻専門学院 ～乳妻たちのアンニュイ～ 体験版 第2弾
*/
static DWORD AdvTry_game_init_offset[AdvTry_games] = { 0x11, 0x1c, 0x11, 0x08, 0x08, 0x00, 0x08, 0x0c, 0x35, 0x11, 0x08, };
static DWORD AdvTry_game_sec_offset[AdvTry_games] = { 0x46, 0x45, 0x49, 0x10, 0x10, 0x08, 0x10, 0x15, 0x51, 0x1e, 0x10, };
static char *AdvTry_game_key[AdvTry_games] = { "ClOVeRrE", "CVPTTRY1", "CPTRYVER", "KOIQ_WEB", 
	"RE_MAJIQ", "METEOR01", "TrpAdTRY", "-AYASEKE", "CHOKOPAI", "UMakeMe!", "SETSUEI-" };


static int AdvTry_game_id;

static int AdvTry_dat_check(struct package *pkg, DWORD init_offset, DWORD sec_offset, BYTE *key, 
		dat_header_t *dat_header, BYTE *code)
{
	if (pkg->pio->readvec(pkg, code, 8, init_offset, IO_SEEK_SET)) 
		return -CUI_EREADVEC;

	xor_decode(code, 8, key, 0);
	
	if (pkg->pio->readvec(pkg, dat_header, sizeof(dat_header_t), sec_offset, IO_SEEK_SET)) 
		return -CUI_EREADVEC;
	
	xor_decode((BYTE *)dat_header, sizeof(dat_header_t), code, 0);
	return strncmp("ARCHIVE", dat_header->magic, 8) == 0 ? 0 : -CUI_EMATCH;
}
#endif

static void inline xor_decode(BYTE *buf, DWORD buf_len, 
							  BYTE *key, DWORD offset)
{
	for (unsigned int i = 0; i < buf_len; i++)
		buf[i] ^= key[(offset + i) & 7];
}

/********************* dat *********************/

static int AdvTry_dat_extract_directory(struct package *pkg,	
										struct package_directory *pkg_dir);

#if 1
static int AdvTry_dat_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	BYTE buf[128];
	if (pkg->pio->read(pkg, buf, sizeof(buf))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}
	
	for (DWORD c = 0; c < sizeof(buf) - 16; ++c) {
		BYTE key[8], code[8];

		for (DWORD k = c + 8; k < sizeof(buf) - 8; ++k) {
			memcpy(code, buf + c, 8);
			memcpy(key, buf + k, 8);
			xor_decode(code, 8, (BYTE *)"ARCHIVE", 0);
			xor_decode(key, 8, code, 0);
			for (DWORD i = 0; i < 8; ++i) {
				if (key[i] > '~' || key[i] < '!')
					break;
			}
			if (i == 8) {
				AdvTry_game_context_t *AdvTry_game_context = 
					(AdvTry_game_context_t *)malloc(sizeof(AdvTry_game_context_t));
				if (!AdvTry_game_context) {
					pkg->pio->close(pkg);
					return -CUI_EMEM;
				}
				strncpy((char *)AdvTry_game_context->key, (char *)key, 8);
				memcpy(AdvTry_game_context->code, buf + c, 8);
				xor_decode(AdvTry_game_context->code, 8, key, 0);
				memcpy(&AdvTry_game_context->dat_header, buf + k, sizeof(dat_header_t));
				xor_decode((BYTE *)&AdvTry_game_context->dat_header, sizeof(dat_header_t), 
					AdvTry_game_context->code, 0);

				if (AdvTry_game_context->dat_header.index_entry_length != sizeof(dat_entry_t)) {
					free(AdvTry_game_context);
					continue;
				}

				AdvTry_game_context->init_offset = c;
				AdvTry_game_context->sec_offset = k;			
				package_set_private(pkg, AdvTry_game_context);

				struct package_directory pkg_dir;
				if (AdvTry_dat_extract_directory(pkg, &pkg_dir)) {
					free(AdvTry_game_context);
					continue;
				}
				free(pkg_dir.directory);

				TCHAR _key[9];
				acp2unicode((char *)key, 8, _key, SOT(_key));
				_key[8] = 0;
				locale_app_printf(app_locale_id, 0, _key, c, k);

				return 0;
			}
		}
	}

	return -CUI_EMATCH;
}
#else
static int AdvTry_dat_match(struct package *pkg)
{
	AdvTry_game_context_t *AdvTry_game_context;
	DWORD i;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	AdvTry_game_context = (AdvTry_game_context_t *)malloc(sizeof(AdvTry_game_context_t));
	if (!AdvTry_game_context) {
		pkg->pio->close(pkg);
		return -CUI_EMEM;
	}
	
	for (i = 0; i < AdvTry_games; i++) {
		int ret = AdvTry_dat_check(pkg, AdvTry_game_init_offset[i], AdvTry_game_sec_offset[i], (BYTE *)AdvTry_game_key[i], 
				&AdvTry_game_context->dat_header, AdvTry_game_context->code);
		if (!ret)
			break;
		if (ret != -CUI_EMATCH)
			return -CUI_EMATCH;
	}
	if (i != AdvTry_games) {
		AdvTry_game_context->key = AdvTry_game_key[i];
		AdvTry_game_context->init_offset = AdvTry_game_init_offset[i];
		AdvTry_game_context->sec_offset = AdvTry_game_sec_offset[i];
		printf("%s %x %x\n", AdvTry_game_key[i],  AdvTry_game_init_offset[i], AdvTry_game_sec_offset[i]);
	} else {
		const char *init_offset_str, *sec_offset_str, *key;
		DWORD init_offset, sec_offset;

		init_offset_str = get_options("init_offset");
		if (!init_offset_str) {
			free(AdvTry_game_context);
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}
		
		sec_offset_str = get_options("sec_offset");
		if (!sec_offset_str) {
			free(AdvTry_game_context);
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}

		key = get_options("key");
		if (!key) {
			free(AdvTry_game_context);
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}

		init_offset = strtoul(init_offset_str, NULL, 0);
		sec_offset = strtoul(sec_offset_str, NULL, 0);

		if (AdvTry_dat_check(pkg, init_offset, sec_offset, (BYTE *)key, 
				&AdvTry_game_context->dat_header, AdvTry_game_context->code)) {
			free(AdvTry_game_context);
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}
	
		AdvTry_game_context->key = key;
		AdvTry_game_context->init_offset = init_offset;
		AdvTry_game_context->sec_offset = sec_offset;
	}
	package_set_private(pkg, AdvTry_game_context);

	return 0;
}
#endif

static int AdvTry_dat_extract_directory(struct package *pkg,	
										struct package_directory *pkg_dir)
{
	dat_header_t *dat_header;
	dat_entry_t *index_buffer;
	DWORD index_length;
	AdvTry_game_context_t *AdvTry_game_context;
	
	AdvTry_game_context = (AdvTry_game_context_t *)package_get_private(pkg);
	dat_header = &AdvTry_game_context->dat_header;
	index_length = dat_header->index_entries * sizeof(dat_entry_t);
	index_buffer = (dat_entry_t *)malloc(index_length);
	if (!index_buffer)
		return -CUI_EMEM;
	
	if (pkg->pio->readvec(pkg, index_buffer, index_length, dat_header->index_offset + AdvTry_game_context->sec_offset, IO_SEEK_SET)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	xor_decode((BYTE *)index_buffer, index_length, AdvTry_game_context->code, dat_header->index_offset & 7);

	dat_entry_t *entry = &((dat_entry_t *)index_buffer)[dat_header->index_entries - 2];
	if (entry->offset + entry->length != (entry+1)->offset) {
		free(index_buffer);
		return -CUI_EMATCH;
	}

	pkg_dir->index_entries = dat_header->index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_length;
	pkg_dir->index_entry_length = sizeof(dat_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

static int AdvTry_dat_parse_resource_info(struct package *pkg,
										  struct package_resource *pkg_res)
{
	dat_entry_t *dat_entry;

	dat_entry = (dat_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, dat_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = dat_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = dat_entry->offset;

	return 0;
}

static int AdvTry_dat_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	BYTE *data;
	AdvTry_game_context_t *AdvTry_game_context;

	data = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!data)
		return -CUI_EMEM;
	
	AdvTry_game_context = (AdvTry_game_context_t *)package_get_private(pkg);
	if (pkg->pio->readvec(pkg, data, pkg_res->raw_data_length, pkg_res->offset + AdvTry_game_context->sec_offset, IO_SEEK_SET)) {
		free(data);
		return -CUI_EREADVEC;
	}
	
	xor_decode(data, pkg_res->raw_data_length, AdvTry_game_context->code, pkg_res->offset & 7);
	pkg_res->raw_data = data;
	return 0;
}

static int AdvTry_dat_save_resource(struct resource *res, 
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

static void AdvTry_dat_release_resource(struct package *pkg, 
									  struct package_resource *pkg_res)
{
	if (pkg_res && pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void AdvTry_dat_release(struct package *pkg, 
							 struct package_directory *pkg_dir)
{
	void *priv;

	if (pkg_dir->directory && pkg_dir->directory_length) {		
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg);
	priv = (void *)package_get_private(pkg);
	if (priv) {
		free(priv);
		package_set_private(pkg, NULL);
	}
}

static cui_ext_operation AdvTry_dat_operation = {
	AdvTry_dat_match,				/* match */
	AdvTry_dat_extract_directory,	/* extract_directory */
	AdvTry_dat_parse_resource_info,	/* parse_resource_info */
	AdvTry_dat_extract_resource,	/* extract_resource */
	AdvTry_dat_save_resource,		/* save_resource */
	AdvTry_dat_release_resource,	/* release_resource */
	AdvTry_dat_release				/* release */
};

int CALLBACK AdvTry_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &AdvTry_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	app_locale_id = locale_app_register(app_locale_configurations, 3);

	return 0;
}
