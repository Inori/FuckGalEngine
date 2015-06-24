#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <resource.h>
#include <package.h>
#include <cui_error.h>
#include <utility.h>
#include <stdio.h>
#include <crass\locale.h>
#include <package_core.h>
#include <vector>

using namespace std;
using std::vector;

/*
为game找key的方法：
用a5b9ac6b和9a639de5做关键字
往下找代码：
0048CA9C   . /75 15         JNZ SHORT 恀椔婏俀.0048CAB3
0048CA9E   . |8D55 A4       LEA EDX,DWORD PTR SS:[EBP-5C]
0048CAA1   . |68 C0AC4F00   PUSH 恀椔婏俀.004FACC0                       ; /Arg2 = 004FACC0
0048CAA6   . |52            PUSH EDX                                 ; |Arg1
0048CAA7   . |C745 A4 08000>MOV DWORD PTR SS:[EBP-5C],8              ; |
0048CAAE   . |E8 4F400500   CALL 恀椔婏俀.004E0B02                       ; \恀椔婏俀.004E0B02
0048CAB3   > \A1 C0935000   MOV EAX,DWORD PTR DS:[5093C0]   <---key0
0048CAB8   .  8B75 EC       MOV ESI,DWORD PTR SS:[EBP-14]
0048CABB   .  33C6          XOR EAX,ESI
0048CABD   .  8947 14       MOV DWORD PTR DS:[EDI+14],EAX
0048CAC0   .  8B45 D8       MOV EAX,DWORD PTR SS:[EBP-28]
0048CAC3   .  8B15 C8935000 MOV EDX,DWORD PTR DS:[5093C8]   <---key1
0048CAC9   .  8B75 CC       MOV ESI,DWORD PTR SS:[EBP-34]
0048CACC   .  33C2          XOR EAX,EDX
0048CACE   .  33F0          XOR ESI,EAX
0048CAD0   .  8945 D8       MOV DWORD PTR SS:[EBP-28],EAX
0048CAD3   .  8BC6          MOV EAX,ESI
0048CAD5   .  81E6 FFFFFF00 AND ESI,0FFFFFF



Lucifen Library & Tools Version:
俺たちに翼はない
1.2.2
ね～PON？×らいPON！
1.2.1
りこりす －lycoris radiata－ 体験版
1.2.0
DokiDokiる～みんぐ 体験版
1.2.2


E:\Program Files\NaponLipon
Q:\Program Files\Terios\りこりす体験版
Q:\sin_ryouki2_trial\恀愢 椔婏偺烞 戞俀復 懱尡斉   (依靠sob_parse("terios03.sob")产生key，很麻烦）
 */

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information Lucifen_cui_information = {
	_T("Lucifen"),			/* copyright */
	_T("Lucifen Library"),	/* system */
	_T(".LPK"),				/* package */
	_T("0.8.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-4-30 22:49"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];		// "LPK1"
	u32 index_length;
} LPK_header_t;

typedef struct {
	u32 index_entries;
	u8 unknown_length;
	u8 mode;
	u32 letter_table_length;
} LPK_index_header_t;

typedef struct {
	u8 letter;
	u16 next_letter;
} LPK_short_index_letter_t;

typedef struct {
	u8 letter;
	u32 next_letter;
} LPK_long_index_letter_t;

typedef struct {
	u8 is_crypted;
	u32 offset;
	u32 comprlen;
	u32 uncomprlen;
} LPK_entry_t;

// 对于这种类型，其实际uncomprlen被设置为comprlen
typedef struct {
	u8 is_crypted;
	u32 offset;
	u32 comprlen;
} LPK_short_entry_t;

typedef struct {
	s8 magic[3];	// "ELG"
	u8 bpp;
	u16 width;
	u16 height;
} elg_header_t;

typedef struct {
	s8 magic[3];	// "ELG"
	u8 type;		// 1
	u8 bpp;
	u16 orig_x;
	u16 orig_y;
	u16 width;
	u16 height;
} elg1_header_t;
#pragma pack ()

typedef struct {
	char name[MAX_PATH];
	DWORD is_crypted;
	DWORD uncomprlen;
	DWORD comprlen;
	DWORD offset;
} my_LPK_entry_t;

#if 1
static const TCHAR *simplified_chinese_strings[] = {
	_T("[注意] 必须指定\"script\"参数才能提取%s，详见document\\cn\\Lucifen.txt\n"),
	_T("[错误] 寻找key时发生错误\n"),
	_T("[错误] 没有找到与%s对应的key\n"),	
	_T("[错误] \"script\"参数指定的文件内不含有key\n"),
};

static const TCHAR *traditional_chinese_strings[] = {
	_T("[注意] 必須指定\"script\"參數才能提取%s，詳見document\\cn\\Lucifen.txt\n"),
	_T("[錯誤] 尋找key時發生錯誤\n"),
	_T("[錯誤] 沒有找到與%s對應的key\n"),	
	_T("[錯誤] \"script\"參數指定的文件內不含有key\n"),
};

static const TCHAR *japanese_strings[] = {
	_T("[注意] %sを抽出するためにはパラメーター\"script\"を指定しなければなりません、詳しくはdocument\\cn\\Lucifen.txtを見てください。\n"),
	_T("[エラー] キーを探している時にエラーが発生します\n"),
	_T("[エラー] %sに対応したキーが見つかりません\n"),	
	_T("[エラー] パラメーター\"script\"に指定されたファイルにキーが含まれていません\n"),
};

static const TCHAR *default_strings[] = {
	_T("[note] you must specify the \"script\" parameter to extract from %s, see document\\en\\Lucifen.txt for details\n"),
	_T("[error] failed to search the key\n"),
	_T("[error] not found the key corresponding to %s\n"),	
	_T("[error] there are not key in the file specified by \"script\" parameter\n"),
};

static struct locale_configuration app_locale_configurations[4] = {
	{
		936, simplified_chinese_strings, 4
	},
	{
		950, traditional_chinese_strings, 4
	},
	{
		932, japanese_strings, 4
	},
	{
		0, default_strings, 4
	},
};

#else

static const TCHAR *simplified_chinese_strings[] = {
	_T("[注意] 必须指定\"game\"参数才能提取%s，详见document\\cn\\Lucifen.txt\n"),
	_T("[错误] 不支持参数\"game=%s\"，详见document\\cn\\Lucifen.txt\n"),
	_T("[错误] 无法提取提取%s\n"),
};

static const TCHAR *traditional_chinese_strings[] = {
	_T("[注意] 必須指定\"game\"參數才能提取%s，詳見document\\cn\\Lucifen.txt\n"),
	_T("[錯誤] 不支持參數\"game=%s\"，詳見document\\cn\\Lucifen.txt\n"),
	_T("[錯誤] 無法提取提取%s\n"),
};

static const TCHAR *default_strings[] = {
	_T("[note] you must specify the \"game\" parameter to extract from %s, see document\\en\\Lucifen.txt for details\n"),
	_T("[error] don\'t support the parameter \"game=%s\", see document\\en\\Lucifen.txt for details\n"),
	_T("[error] can\'t extract from the package %s\n"),
};

static struct locale_configuration app_locale_configurations[3] = {
	{
		936, simplified_chinese_strings, 2
	},
	{
		950, traditional_chinese_strings, 2
	},
	{
		0, default_strings, 2
	},
};
#endif

static int app_locale_id;


static void *my_malloc(DWORD len)
{
	return malloc(len);
}

struct lpk_crypt {
	DWORD flags_bit0;	// [c] 1 - offset <<= 11
	DWORD flags_bit1;	// [d]
	DWORD is_crypted;	// [e]
	DWORD entry_type;	// [f] LPK_entry size 0 -  9 bytes; 1 - 13 bytes
	DWORD total_crypted;// [10]
	DWORD key;			// [14]
	BYTE plain_data[255];
	DWORD plain_data_length;
};

struct lpk_info {
	char name[MAX_PATH];
	u32 lpk_key0, lpk_key1;
};

static struct lpk_info script_lpk_info = {
	"SCRIPT.LPK", 0x00000000, 0x00000000
};

struct game_list {
	TCHAR name[MAX_PATH];
	DWORD packages;
	struct lpk_info lpk_list[32];
} game_list[] = {
	{
		_T("sin_ryouki2"), 8, 
		{
			{ "SYS.LPK", 0x3986afc9, 0xfc8c32e7 },
			{ "CHR.LPK", 0x1b478946, 0x489e83f9 },
			{ "PIC.LPK", 0x3f9d8075, 0xa75cbda3 },
			{ "BGM.LPK", 0xeab5949c, 0x48abe813 },
			{ "SE.LPK", 0x5a95b36b, 0x249f4beb },
			{ "VOICE.LPK", 0x6b954634, 0xa1cf23ce },
			{ "DATA.LPK", 0x8395c7cb, 0xe4cf9e3b },
			{ "SCRIPT.LPK", 0x00000000, 0x00000000 },
		}
	},
	{
		_T("ORE_TUBA"), 8, 
		{
			{ "SYS.LPK", 0x524657ea, 0x782b5a5c },
			{ "CHR.LPK", 0x8c982bca, 0x46bc4cdd },
			{ "PIC.LPK", 0xb78c913c, 0x57ecb345 },
			{ "BGM.LPK", 0x45e32e7d, 0x4f538ce3 },
			{ "SE.LPK", 0x9a5af6de, 0xcb1866c7 },
			{ "VOICE.LPK", 0x3eb61d34, 0xb451ab16 },
			{ "DATA.LPK", 0x7ce5ac6a, 0x49a3a35b },
			{ "SCRIPT.LPK", 0x00000000, 0x00000000 },
		}
	},
	{
		_T("NaponLipon"), 9, 
		{
			{ "SYS.LPK", 0x67db35ed, 0xbe4d6a37 },
			{ "CHR.LPK", 0x9e3bad56, 0x95fe9dc3 },
			{ "PIC.LPK", 0xd9b5e6c3, 0xba53e5d5 },
			{ "BGM.LPK", 0xeb6de3b5, 0xcd571de9 },
			{ "SE.LPK", 0x9ab3ad5e, 0xad3d4d96 },
			{ "VOICE.LPK", 0x75cbd59d, 0x5ed59aca },
			{ "DATA.LPK", 0xde6ad53e, 0x9e3b5e3d },
			{ "DESKTOP.LPK", 0xacd5b36d, 0xd56adb5d },
			{ "SCRIPT.LPK", 0x00000000, 0x00000000 },
		}
	},
	{
		_T("ORE_TUBA_Trial"), 7, 
		{
			{ "SYS.LPK", 0x8030ab96, 0xb9f6638e },
			{ "CHR.LPK", 0xa7b9178b, 0xf79c547c },
			{ "PIC.LPK", 0x34b73229, 0xb690e07d },
			{ "BGM.LPK", 0x41e7505f, 0x1e10c3c5 },
			{ "SE.LPK", 0x34df875e, 0x435603e8 },
			{ "VOICE.LPK", 0x30701d73, 0xfb524ce5 },
			{ "SCRIPT.LPK", 0x00000000, 0x00000000 },
		}
	},
	{
		NULL, 0,
	}
};

struct lpk_info *lpk_info;
static struct lpk_crypt lpk_crypt;

static void LPK_index_add_entry(BYTE *name, int name_len, 
								my_LPK_entry_t *p_entry, DWORD *count,
								void *LPK_entry,
								DWORD entry_num)
{
	my_LPK_entry_t *entry = p_entry + entry_num;
	strncpy(entry->name, (char *)name, name_len);
	entry->name[name_len] = 0;

	if (lpk_crypt.entry_type) {
		LPK_entry_t *LPK_entry_list = (LPK_entry_t *)LPK_entry;
		if (lpk_crypt.flags_bit0)
			entry->offset = LPK_entry_list[entry_num].offset << 11;
		else
			entry->offset = LPK_entry_list[entry_num].offset;
		entry->is_crypted = LPK_entry_list[entry_num].is_crypted;
		entry->uncomprlen = LPK_entry_list[entry_num].uncomprlen;
		entry->comprlen = LPK_entry_list[entry_num].comprlen;
	} else {
		LPK_short_entry_t *LPK_entry_list = (LPK_short_entry_t *)LPK_entry;
		if (lpk_crypt.flags_bit0)
			entry->offset = LPK_entry_list[entry_num].offset << 11;
		else
			entry->offset = LPK_entry_list[entry_num].offset;
		entry->is_crypted = LPK_entry_list[entry_num].is_crypted;
		entry->comprlen = LPK_entry_list[entry_num].comprlen;
		entry->uncomprlen = 0;
	}

	(*count)++;
}

static void LPK_long_index_searching(BYTE *p_index, BYTE *name, 
									 int name_len, my_LPK_entry_t *p_entry,
									 DWORD *count, void *LPK_entry_list)
{
	u8 letter_num = *p_index++;
	LPK_long_index_letter_t *lic = (LPK_long_index_letter_t *)p_index;
	for (DWORD i = 0; i < letter_num; i++) {
		name[name_len] = lic->letter;
		if (lic->letter)
			LPK_long_index_searching((BYTE *)(lic + 1) + lic->next_letter, 
				name, name_len + 1, p_entry, count, LPK_entry_list);
		else
			LPK_index_add_entry(name, name_len, p_entry, count, 
				LPK_entry_list, lic->next_letter);
		++lic;
	}	
}

static void LPK_short_index_searching(BYTE *p_index, BYTE *name, 
									  int name_len, my_LPK_entry_t *p_entry,
									  DWORD *count, void *LPK_entry_list)
{
	u8 letter_num = *p_index++;
	LPK_short_index_letter_t *sic = (LPK_short_index_letter_t *)p_index;
	for (DWORD i = 0; i < letter_num; ++i) {
		name[name_len] = sic->letter;
		if (sic->letter) {
			LPK_short_index_searching((BYTE *)(sic + 1) + sic->next_letter, 
				name, name_len + 1, p_entry, count, LPK_entry_list);
		} else
			LPK_index_add_entry(name, name_len, p_entry, count, 
				LPK_entry_list, sic->next_letter);
		sic++;
	}	
}

static void LPK_index_searching(u8 mode, BYTE *p_index, BYTE *name, 
								int name_len, my_LPK_entry_t *p_entry,
								DWORD *count, void *LPK_entry_list)
{
	if (mode)	// long mode
		LPK_long_index_searching(p_index, name, 0, p_entry, 
			count, LPK_entry_list);
	else		// short mode
		LPK_short_index_searching(p_index, name, 0, p_entry, 
			count, LPK_entry_list);
}

static void lzss_uncompress(BYTE *uncompr, DWORD uncomprlen, 
							BYTE *compr)
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
		if (!(flag & 0x0100))
			flag = compr[curbyte++] | 0xff00;

		if (flag & 1) {
			unsigned char data;

			data = compr[curbyte++];
			if (act_uncomprlen >= uncomprlen)
				break;
			win[nCurWindowByte++] = data;
			uncompr[act_uncomprlen++] = data;
			/* 输出的1字节放入滑动窗口 */
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
					return;
				data = win[(win_offset + i) & (win_size - 1)];				
				uncompr[act_uncomprlen++] = data;
				/* 输出的1字节放入滑动窗口 */
				win[nCurWindowByte++] = data;
				nCurWindowByte &= win_size - 1;	
			}
		}
	}
}

static BYTE *elg32_uncompress(BYTE *uncompr, BYTE *compr, int width)
{
	BYTE *org_c = compr;
	BYTE flags;

	while ((flags = *compr++) != 0xff) {
		DWORD cp, pos;

		if (!(flags & 0xc0)) {
			if (flags & 0x20)
				cp = ((flags & 0x1f) << 8) + *compr++ + 33;
			else
				cp = (flags & 0x1f) + 1;

			for (DWORD i = 0; i < cp; ++i) {
				*uncompr++ = *compr++;
				*uncompr++ = *compr++;
				*uncompr++ = *compr++;
				*uncompr++ = 0;
			}
		} else if ((flags & 0xc0) == 0x40) {
			if (flags & 0x20)
				cp = ((flags & 0x1f) << 8) + *compr++ + 34;
			else
				cp = (flags & 0x1f) + 2;			

			BYTE b = *compr++;
			BYTE g = *compr++;
			BYTE r = *compr++;
			for (DWORD i = 0; i < cp; ++i) {
				*uncompr++ = b;
				*uncompr++ = g;
				*uncompr++ = r;
				*uncompr++ = 0;
			}
		} else if ((flags & 0xc0) == 0x80) {
			if (!(flags & 0x30)) {
				cp = (flags & 0xf) + 1;
				pos = *compr++ + 2;
			} else if ((flags & 0x30) == 0x10) {
				pos = ((flags & 0xf) << 8) + *compr++ + 2;
				cp = *compr++ + 1;
			} else if ((flags & 0x30) == 0x20) {
				BYTE tmp = *compr++;
				pos = ((((flags & 0xf) << 8) + tmp) << 8) + *compr++ + 4098;
				cp = *compr++ + 1;
			} else {
				if (flags & 8)
					pos = ((flags & 0x7) << 8) + *compr++ + 10;
				else
					pos = (flags & 0x7) + 2;
				cp = 1;
			}

			BYTE *src = uncompr - 4 * pos;
			for (DWORD i = 0; i < cp; ++i) {
				*uncompr++ = *src++;
				*uncompr++ = *src++;
				*uncompr++ = *src++;
				*uncompr++ = *src++;
			}
		} else {
			int y, x;

			if (!(flags & 0x30)) {
				if (!(flags & 0xc)) {
					y = ((flags & 0x3) << 8) + *compr++ + 16;
					x = 0;
				} else if ((flags & 0xc) == 0x4) {
					y = ((flags & 0x3) << 8) + *compr++ + 16;
					x = -1;
				} else if ((flags & 0xc) == 0x8) {
					y = ((flags & 0x3) << 8) + *compr++ + 16;
					x = 1;
				} else {
					pos = ((flags & 0x3) << 8) + *compr++ + 2058;
					BYTE *src = uncompr - 4 * pos;
					*uncompr++ = *src++;
					*uncompr++ = *src++;
					*uncompr++ = *src++;
					*uncompr++ = *src;
					continue;
				}
			} else if ((flags & 0x30) == 0x10) {
				y = (flags & 0xf) + 1;
				x = 0;
			} else if ((flags & 0x30) == 0x20) {
				y = (flags & 0xf) + 1;
				x = -1;
			} else {
				y = (flags & 0xf) + 1;
				x = 1;
			}

			BYTE *src = uncompr + (x - width * y) * 4;
			*uncompr++ = *src++;
			*uncompr++ = *src++;
			*uncompr++ = *src++;
			*uncompr++ = *src;
		}
	}

	return compr;
}

static BYTE *elg24_uncompress(BYTE *uncompr, BYTE *compr, int width)
{
	BYTE *org_c = compr;
	BYTE flags;

	while ((flags = *compr++) != 0xff) {
		DWORD cp, pos;

		if (!(flags & 0xc0)) {
			if (flags & 0x20)
				cp = ((flags & 0x1f) << 8) + *compr++ + 33;
			else
				cp = (flags & 0x1f) + 1;

			for (DWORD i = 0; i < cp; ++i) {
				*uncompr++ = *compr++;
				*uncompr++ = *compr++;
				*uncompr++ = *compr++;
			}
		} else if ((flags & 0xc0) == 0x40) {
			if (flags & 0x20)
				cp = ((flags & 0x1f) << 8) + *compr++ + 34;
			else
				cp = (flags & 0x1f) + 2;			

			BYTE b = *compr++;
			BYTE g = *compr++;
			BYTE r = *compr++;
			for (DWORD i = 0; i < cp; ++i) {
				*uncompr++ = b;
				*uncompr++ = g;
				*uncompr++ = r;
			}
		} else if ((flags & 0xc0) == 0x80) {
			if (!(flags & 0x30)) {
				cp = (flags & 0xf) + 1;
				pos = *compr++ + 2;
			} else if ((flags & 0x30) == 0x10) {
				pos = ((flags & 0xf) << 8) + *compr++ + 2;
				cp = *compr++ + 1;
			} else if ((flags & 0x30) == 0x20) {
				BYTE tmp = *compr++;
				pos = ((((flags & 0xf) << 8) + tmp) << 8) + *compr++ + 4098;
				cp = *compr++ + 1;
			} else {
				if (flags & 8)
					pos = ((flags & 0x7) << 8) + *compr++ + 10;
				else
					pos = (flags & 0x7) + 2;
				cp = 1;
			}

			BYTE *src = uncompr - 3 * pos;
			for (DWORD i = 0; i < cp; ++i) {
				*uncompr++ = *src++;
				*uncompr++ = *src++;
				*uncompr++ = *src++;
			}
		} else {
			int y, x;

			if (!(flags & 0x30)) {
				if (!(flags & 0xc)) {
					y = ((flags & 0x3) << 8) + *compr++ + 16;
					x = 0;
				} else if ((flags & 0xc) == 0x4) {
					y = ((flags & 0x3) << 8) + *compr++ + 16;
					x = -1;
				} else if ((flags & 0xc) == 0x8) {
					y = ((flags & 0x3) << 8) + *compr++ + 16;
					x = 1;
				} else {
					pos = ((flags & 0x3) << 8) + *compr++ + 2058;
					BYTE *src = uncompr - 3 * pos;
					*uncompr++ = *src++;
					*uncompr++ = *src++;
					*uncompr++ = *src;
					continue;
				}
			} else if ((flags & 0x30) == 0x10) {
				y = (flags & 0xf) + 1;
				x = 0;
			} else if ((flags & 0x30) == 0x20) {
				y = (flags & 0xf) + 1;
				x = -1;
			} else {
				y = (flags & 0xf) + 1;
				x = 1;
			}

			BYTE *src = uncompr + (x - width * y) * 3;
			*uncompr++ = *src++;
			*uncompr++ = *src++;
			*uncompr++ = *src;
		}
	}

	return compr;
}

static BYTE *elga_uncompress(BYTE *uncompr, BYTE *compr)
{
	BYTE *org = uncompr;
	BYTE *org_c = compr;
	BYTE flags;

	uncompr += 3;
	while ((flags = *compr++) != 0xff) {
		DWORD cp, pos;

		if (!(flags & 0xc0)) {
			if (flags & 0x20)
				cp = ((flags & 0x1f) << 8) + *compr++ + 33;
			else
				cp = (flags & 0x1f) + 1;

			for (DWORD i = 0; i < cp; ++i) {
				*uncompr = *compr++;
				uncompr += 4;
			}
		} else if ((flags & 0xc0) == 0x40) {
			if (flags & 0x20)
				cp = ((flags & 0x1f) << 8) + *compr++ + 35;
			else
				cp = (flags & 0x1f) + 3;			

			BYTE a = *compr++;
			for (DWORD i = 0; i < cp; ++i) {
				*uncompr = a;
				uncompr += 4;
			}
		} else {
			if ((flags & 0xc0) == 0x80) {
				if (!(flags & 0x30)) {
					cp = (flags & 0xf) + 2;
					pos = *compr++ + 2;
				} else if ((flags & 0x30) == 0x10) {
					pos = ((flags & 0xf) << 8) + *compr++ + 3;
					cp = *compr++ + 4;
				} else if ((flags & 0x30) == 0x20) {
					pos = ((flags & 0xf) << 8) + *compr++ + 3;
					cp = 3;
				} else {
					pos = ((flags & 0xf) << 8) + *compr++ + 3;
					cp = 4;
				}
			} else if (flags & 0x20) {
				pos = (flags & 0x1f) + 2;
				cp = 2;
			} else {
				pos = (flags & 0x1f) + 1;
				cp = 1;
			}

			BYTE *src = uncompr - 4 * pos;
			for (DWORD i = 0; i < cp; ++i) {
				*uncompr = *src;
				src += 4;
				uncompr += 4;
			}
		}
	}

	return compr;
}

static BYTE *elg8_uncompress(BYTE *uncompr, BYTE *compr)
{
	BYTE *org = uncompr;
	BYTE *org_c = compr;
	BYTE flags;

	while ((flags = *compr++) != 0xff) {
		DWORD cp, pos;

		if (!(flags & 0xc0)) {
			if (flags & 0x20)
				cp = ((flags & 0x1f) << 8) + *compr++ + 33;
			else
				cp = (flags & 0x1f) + 1;

			for (DWORD i = 0; i < cp; ++i)
				*uncompr++ = *compr++;
		} else if ((flags & 0xc0) == 0x40) {
			if (flags & 0x20)
				cp = ((flags & 0x1f) << 8) + *compr++ + 35;
			else
				cp = (flags & 0x1f) + 3;			

			BYTE v = *compr++;
			for (DWORD i = 0; i < cp; ++i)
				*uncompr++ = v;
		} else {
			if ((flags & 0xc0) == 0x80) {
				if (!(flags & 0x30)) {
					cp = (flags & 0xf) + 2;
					pos = *compr++ + 2;
				} else if ((flags & 0x30) == 0x10) {
					pos = ((flags & 0xf) << 8) + *compr++ + 3;
					cp = *compr++ + 4;
				} else if ((flags & 0x30) == 0x20) {
					pos = ((flags & 0xf) << 8) + *compr++ + 3;
					cp = 3;
				} else {
					pos = ((flags & 0xf) << 8) + *compr++ + 3;
					cp = 4;
				}
			} else if (flags & 0x20) {
				pos = (flags & 0x1f) + 2;
				cp = 2;
			} else {
				pos = (flags & 0x1f) + 1;
				cp = 1;
			}

			BYTE *src = uncompr - pos;
			for (DWORD i = 0; i < cp; ++i)
				*uncompr++ = *src++;
		}
	}

	return compr;
}

static vector<struct lpk_info> probe_lpk_info;

static int Lucifen_gameinit_parser(BYTE *sob, DWORD len)
{
	if (strncmp((char *)sob, "SOB0", 4))
		return -CUI_EMATCH;

	u32 offset = *(u32 *)(sob + 4) + 8;
	u32 parser_len = *(u32 *)(sob + offset);
	u32 *p = (u32 *)(sob + offset + 4);
	u32 *end_p = p + parser_len / 4;

	probe_lpk_info.clear();
	while (p < end_p) {
		if (p[0] == 0x28 && p[1] == 0 && p[2] && p[3] == 8 && p[4] == 1 && p[5] == 8 && p[6] == 1 && p[7] == 8 && p[8] && p[9] == 8 && p[10] && p[11] == 5
				&& p[17] == 0x28 && p[18] == 0 && p[19] && p[20] == 8 && p[21] && p[22] == 8 && p[23] == -1 && p[24] == 8 && p[25] == 1 && p[26] == 8 && p[27] == 1 && p[28] == 5) {
			char *lpk = (char *)(sob + ((p[21] - p[2]) + ((BYTE *)p - sob + 0x44) - 0x10));
			struct lpk_info info;

			info.lpk_key0 = p[8];
			info.lpk_key1 = p[10];
			strcpy(info.name, lpk);
			probe_lpk_info.push_back(info);
			p += 0x44 * 2 / 4;
			//printf("key %s, %x %x\n", lpk, info.lpk_key0, info.lpk_key1);
		} else
			++p;
	}

	return 0;
}

/* gameinit.sob里藏有解密key，但解读方法不明 */
/* 封包索引目录提取函数 */
static int Lucifen_LPK_extract_directory(struct package *pkg,
										 struct package_directory *pkg_dir);
static int Lucifen_LPK_parse_resource_info(struct package *pkg,
										   struct package_resource *pkg_res);
static int Lucifen_LPK_extract_resource(struct package *pkg,
										struct package_resource *pkg_res);

static int Lucifen_LPK_set_key(const char *script_path)
{
	TCHAR path[MAX_PATH];

	if (acp2unicode(script_path, -1, path, SOT(path)))
		return -1;

	struct package *pkg = package_fio_create(_T(""), path);
	if (!pkg)
		return -1;

	if (pkg->pio->open(pkg, IO_READONLY)) {
		package_release(pkg);
		return -CUI_EOPEN;
	}

	s8 magic[4];	
	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		package_release(pkg);
		return -CUI_EREAD;
	}

	lpk_info = &script_lpk_info;

	int ret = 0;
	struct package_resource pkg_res;
	if (!strncmp(magic, "LPK1", 4)) {
		struct package_directory pkg_dir;
		ret = Lucifen_LPK_extract_directory(pkg, &pkg_dir);
		if (!ret) {
			my_LPK_entry_t *LPK_entry = (my_LPK_entry_t *)pkg_dir.directory;
			for (DWORD i = 0; i < pkg_dir.index_entries; ++i) {
				if (!stricmp(LPK_entry->name, "gameinit.sob"))
					break;
				++LPK_entry;
			}
			if (i != pkg_dir.index_entries) {
				pkg_res.actual_index_entry = LPK_entry;
				ret = Lucifen_LPK_parse_resource_info(pkg, &pkg_res);
				if (!ret) {
					ret = Lucifen_LPK_extract_resource(pkg, &pkg_res);
					if (!ret) {
						char *magic;
						if (pkg_res.raw_data)
							magic = (char *)pkg_res.raw_data;
						else
							magic = (char *)pkg_res.actual_data;
					}
				}
			} else
				ret = -CUI_EMATCH;
			free(pkg_dir.directory);
		}
	} else if (!strncmp(magic, "SOB0", 4)) {
		pkg->pio->length_of(pkg, (u32 *)&pkg_res.raw_data_length);
		pkg_res.raw_data = malloc(pkg_res.raw_data_length);
		if (!pkg_res.raw_data)
			ret = -CUI_EMEM;
		else if (pkg->pio->readvec(pkg, pkg_res.raw_data, pkg_res.raw_data_length, 0, IO_SEEK_SET)) 
			ret = -CUI_EREADVEC;						
	} else
		ret = -CUI_EMATCH;

	if (!ret)
		ret = Lucifen_gameinit_parser(pkg_res.raw_data ? (BYTE *)pkg_res.raw_data : (BYTE *)pkg_res.actual_data, 
			pkg_res.raw_data_length);
	
	pkg->pio->close(pkg);
	package_release(pkg);

	return ret;	
}

static int sob0_parse(BYTE *sob)
{
	u32 index_len = *(u32 *)sob;
	sob += 4;
	BYTE *sob_data = sob + index_len;

	u32 len0 = *(u32 *)sob_data;
	sob_data += 4;
	u32 len1 = *(u32 *)sob_data;
	sob_data += 4;

	u32 entries = *(u32 *)sob;
	sob += 4;
	for (DWORD i = 0; i < entries; ++i) {
		u32 offset = *(u32 *)sob;
		sob += 4;
		u32 src_offset = *(u32 *)sob;
		sob += 4;
		printf("[0][%02x]: 12 bytes per entry, src %x, offset %x %08x\n",
			i, src_offset, offset, *(u32 *)(sob_data + offset));
	}

	entries = *(u32 *)sob;
	sob += 4;
	// 辅助func_addr, src_offset指向的是exe中的数据区
	for (i = 0; i < entries; ++i) {
		u32 offset = *(u32 *)sob;
		sob += 4;
		u32 src_offset = *(u32 *)sob;
		sob += 4;
		printf("[1][%02x]: 4 bytes per entry, src %x, offset %x %08x\n",
			i, src_offset, offset, *(u32 *)(sob_data + offset));
	}

	entries = *(u32 *)sob;
	sob += 4;
	for (i = 0; i < entries; ++i) {
		u32 offset = *(u32 *)sob;
		sob += 4;
		u32 src_offset = *(u32 *)sob;
		sob += 4;
		printf("[2][%02x]: 12 bytes per entry, src %x, offset %x %08x\n",
			i, src_offset, offset, *(u32 *)(sob_data + offset));
	}

	entries = *(u32 *)sob;
	sob += 4;
	// 其src_offset是相对于sob_data开始的
	for (i = 0; i < entries; ++i) {
		u32 offset = *(u32 *)sob;
		sob += 4;
		u32 src_offset = *(u32 *)sob;
		sob += 4;
		printf("[3][%02x]: pointer, src %x, offset %x %08x\n",
			i, src_offset, offset, *(u32 *)(sob_data + offset));
	}

	entries = *(u32 *)sob;
	sob += 4;
	// 其src_offset是相对于sob_data开始的
	for (i = 0; i < entries; ++i) {
		u32 offset = *(u32 *)sob;
		sob += 4;
		u32 src_offset = *(u32 *)sob;
		sob += 4;
		printf("[4][%02x]: pointer, src %x, offset %x %08x\n",
			i, src_offset, offset, *(u32 *)(sob_data + offset));
	}

	if (*(u32 *)sob)
		printf("sdf\n");

	return 0;
}

/********************* LPK *********************/

/* 封包匹配回调函数 */
static int Lucifen_LPK_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "LPK1", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (!_tcsicmp(pkg->name, _T("SCRIPT.LPK"))) {
		lpk_info = &script_lpk_info;
		return 0;
	}

	const char *script = get_options("script");	
	if (!script) {
		const TCHAR *game = get_options2(_T("game"));
		if (!game) {
			lpk_info = &script_lpk_info;
			return 0;
		}

		struct game_list *game_config = NULL;	
		for (DWORD i = 0; game_list[i].packages; ++i) {
			if (!lstrcmpi(game, game_list[i].name)) {
				game_config = &game_list[i];
				break;
			}
		}
		if (!game_list[i].packages) {
			//locale_app_printf(app_locale_id, 1, game);
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}

		char pkg_name[MAX_PATH];
		unicode2sj(pkg_name, sizeof(pkg_name), pkg->name, -1);
		for (i = 0; i < game_config->packages; ++i) {
			if (!strcmpi(pkg_name, game_config->lpk_list[i].name)) {
				lpk_info = &game_config->lpk_list[i];
				break;
			}
		}
		if (i == game_config->packages) {
			//locale_app_printf(app_locale_id, 2, pkg->name);
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}

		return 0;
	}

	if (Lucifen_LPK_set_key(script) < 0) {
		locale_app_printf(app_locale_id, 1, pkg->name);
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (probe_lpk_info.size()) {
		char pkg_name[MAX_PATH];

		unicode2sj(pkg_name, sizeof(pkg_name), pkg->name, -1);
		for (DWORD i = 0; probe_lpk_info.size(); ++i) {
			if (!stricmp(pkg_name, probe_lpk_info[i].name)) {
				lpk_info = &probe_lpk_info[i];
				break;
			}
		}
		if (i == probe_lpk_info.size()) {
			locale_app_printf(app_locale_id, 2, pkg->name);
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}
	} else {
		locale_app_printf(app_locale_id, 3, pkg->name);
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int Lucifen_LPK_extract_directory(struct package *pkg,
										 struct package_directory *pkg_dir)
{
	u32 index_length;	
	TCHAR pkg_name[MAX_PATH];
	char pkg_name_ansi[MAX_PATH];

	if (pkg->pio->read(pkg, &index_length, 4))
		return -CUI_EREAD;

	_tcsncpy(pkg_name, pkg->name, pkg->extension - pkg->name);
	pkg_name[pkg->extension - pkg->name] = 0;
	if (!CharUpper(pkg_name))
		return -CUI_EMATCH;
	if (unicode2sj(pkg_name_ansi, sizeof(pkg_name_ansi), pkg_name, -1))
		return -CUI_EMATCH;

	u32 key0, key1;
	int name_len = lstrlenA((char *)pkg_name_ansi);
	key0 = 0xa5b9ac6b;
	key1 = 0x9a639de5;
	for (int n = 0; n < name_len; ++n) {
		key0 ^= pkg_name_ansi[name_len - 1 - n];
		key1 ^= pkg_name_ansi[n];
		key0 = (key0 >> 7) | (key0 << 25);
		key1 = (key1 << 7) | (key1 >> 25);
	}

	key0 ^= lpk_info->lpk_key0;
	key1 ^= lpk_info->lpk_key1;
	index_length ^= key1;
	u32 flags = index_length >> 24;
	index_length &= 0xffffff;

	if (flags & 1)
		index_length = (index_length << 11) - 8;

	DWORD *index_buffer = (DWORD *)malloc(index_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	lpk_crypt.flags_bit0 = (flags >> 0) & 1;
	lpk_crypt.flags_bit1 = (flags >> 1) & 1;
	lpk_crypt.is_crypted = (flags >> 2) & 1;
	lpk_crypt.entry_type = (flags >> 3) & 1;
	lpk_crypt.total_crypted = (flags >> 4) & 1;
	lpk_crypt.key = key0;

	u32 d = 0x31746285;
	for (DWORD i = 0; i < index_length / 4; ++i) {
		u8 shift;

		shift = d = (d << 4) | (d >> 28);
		shift &= 31;
		index_buffer[i] ^= key1;
		key1 = (key1 >> shift) | (key1 << (32 - shift));
	}

	BYTE *p_index = (BYTE *)index_buffer;
	u32 index_entries = *(u32 *)p_index;
	p_index += 4;
	u8 plain_data_length = *p_index++;		// 附加在每个资源前面的公共数据长度
	lpk_crypt.plain_data_length = plain_data_length;
	memcpy(lpk_crypt.plain_data, p_index, plain_data_length);
	p_index += plain_data_length;
	if (lpk_crypt.flags_bit1) {
		u8 mode = !!*p_index++;
		u32 index_letter_table_len = *(u32 *)p_index;
		p_index += 4;

		void *LPK_entry_buffer = (void *)(p_index + index_letter_table_len);	// 3c_obj->[2c]
		my_LPK_entry_t *p_entry_list = (my_LPK_entry_t *)malloc(sizeof(my_LPK_entry_t) * index_entries);
		if (!p_entry_list) {
			free(index_buffer);
			return -CUI_EMEM;
		}

		// 对每个资源名称的每一个字母做一级编码
		DWORD count = 0;
		BYTE name[MAX_PATH];
		LPK_index_searching(mode, p_index, name, 0, p_entry_list, 
				&count, LPK_entry_buffer);

		if (count != index_entries) {
			free(p_entry_list);
			free(index_buffer);
			return -CUI_EMATCH;
		}
		free(index_buffer);

		pkg_dir->index_entries = index_entries;
		pkg_dir->directory = p_entry_list;
		pkg_dir->directory_length = sizeof(my_LPK_entry_t) * index_entries;
		pkg_dir->index_entry_length = sizeof(my_LPK_entry_t);
	} else {
		printf("unsupported index mode, dump index ...\n");
		MySaveFile(_T("index"), index_buffer, index_length);
		return -CUI_EMATCH;
	}

	return 0;
}

/* 封包索引项解析函数 */
static int Lucifen_LPK_parse_resource_info(struct package *pkg,
										   struct package_resource *pkg_res)
{
	my_LPK_entry_t *LPK_entry;

	LPK_entry = (my_LPK_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, LPK_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = LPK_entry->comprlen;
	pkg_res->actual_data_length = LPK_entry->uncomprlen;
	pkg_res->offset = LPK_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int Lucifen_LPK_extract_resource(struct package *pkg,
										struct package_resource *pkg_res)
{
	BYTE *raw = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw + lpk_crypt.plain_data_length, 
		pkg_res->raw_data_length - lpk_crypt.plain_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(raw);
			return -CUI_EREADVEC;
	}

#if 0
	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		memcpy(raw, lpk_crypt.plain_data, lpk_crypt.plain_data_length);
		pkg_res->raw_data = raw;
		return 0;
	}
#endif

	DWORD dec_len;
	BYTE *dec;
	if (pkg_res->actual_data_length) {
		pkg_res->actual_data = malloc(pkg_res->actual_data_length + lpk_crypt.plain_data_length);
		if (!pkg_res->actual_data) {
			free(raw);
			return -CUI_EMEM;
		}
		BYTE *uncompr = (BYTE *)pkg_res->actual_data + lpk_crypt.plain_data_length;
		lzss_uncompress(uncompr, pkg_res->actual_data_length, raw);
		free(raw);
		dec = uncompr;
		dec_len = pkg_res->actual_data_length;
		pkg_res->actual_data_length += lpk_crypt.plain_data_length;
		memcpy(pkg_res->actual_data, lpk_crypt.plain_data, lpk_crypt.plain_data_length);
		pkg_res->raw_data = NULL;
	} else {
		dec = raw + lpk_crypt.plain_data_length;
		dec_len = pkg_res->raw_data_length - lpk_crypt.plain_data_length;
		pkg_res->raw_data = raw;
		memcpy(raw, lpk_crypt.plain_data, lpk_crypt.plain_data_length);
	}

	if (lpk_crypt.is_crypted) {
		if (lpk_crypt.total_crypted) {
			for (DWORD i = 0; i < dec_len; i++)
				dec[i] = ((dec[i] ^ 0x50) >> 4) + (((dec[i] ^ 0xFD) & 0xF) << 4);
		}

		DWORD enc_len = dec_len / 4;
		if (enc_len) {
			if (enc_len > 64)
				enc_len = 64;
			DWORD key = lpk_crypt.key;
			u32 d = 0x31746285;
			DWORD *enc = (DWORD *)dec;
			for (DWORD i = 0; i < enc_len; i++) {
				u8 shift = d = (d >> 4) | (d << 28);
				enc[i] ^= key;
				key = _rotl(key, shift);
		//		shift &= 31;
		//		key = (key << shift) | (key >> (32 - shift));
			}
		}		
	}

	//if (!strcmp(pkg_res->name, "terios03.sob")) {
	//	sob0_parse(dec + 4);
	//}

	return 0;
}

static int Lucifen_LPK_save_resource(struct resource *res, 
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

static void Lucifen_LPK_release_resource(struct package *pkg,
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

static void Lucifen_LPK_release(struct package *pkg, 
								struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation Lucifen_LPK_operation = {
	Lucifen_LPK_match,					/* match */
	Lucifen_LPK_extract_directory,		/* extract_directory */
	Lucifen_LPK_parse_resource_info,	/* parse_resource_info */
	Lucifen_LPK_extract_resource,		/* extract_resource */
	Lucifen_LPK_save_resource,			/* save_resource */
	Lucifen_LPK_release_resource,		/* release_resource */
	Lucifen_LPK_release					/* release */
};

/********************* elg *********************/

/* 封包匹配回调函数 */
static int Lucifen_elg_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	s8 magic[3];
	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "ELG", 3)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	u8 bpp;
	if (pkg->pio->read(pkg, &bpp, sizeof(bpp))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (bpp == 1) {
		if (pkg->pio->read(pkg, &bpp, sizeof(bpp))) {
			pkg->pio->close(pkg);
			return -CUI_EREAD;
		}
	}

	if (bpp != 8 && bpp != 24 && bpp != 32) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包资源提取函数 */
static int Lucifen_elg_extract_resource(struct package *pkg,
										struct package_resource *pkg_res)
{
	DWORD elg_len = pkg_res->raw_data_length;
	BYTE *elg = (BYTE *)malloc(elg_len);
	if (!elg)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, elg, elg_len, 0, IO_SEEK_SET)) {
		free(elg);
		return -CUI_EREADVEC;
	}

	elg_header_t *header = (elg_header_t *)elg;
	DWORD width, height, bpp;
	BYTE *compr;
	if (header->bpp == 1) {
		elg1_header_t *header = (elg1_header_t *)elg;
		width = header->width;
		height = header->height;
		bpp = header->bpp;
		compr = (BYTE *)(header + 1);
	} else {
		width = header->width;
		height = header->height;
		bpp = header->bpp;
		compr = (BYTE *)(header + 1);
	}

	DWORD uncomprlen = width * height * bpp / 8;
	BYTE *uncompr = (BYTE *)malloc(uncomprlen);
	if (!uncompr) {
		free(elg);
		return -CUI_EMEM;
	}

	u32 palette[256];

	if (bpp == 24)
		elg24_uncompress(uncompr, compr, width);
	else if (bpp == 32)
		elga_uncompress(uncompr, elg32_uncompress(uncompr, compr, width));
	else if (bpp == 8)
		elg8_uncompress(uncompr, elg8_uncompress((BYTE *)palette, compr));
	free(elg);

	if (bpp == 8) {
		if (MyBuildBMPFile(uncompr, uncomprlen, (BYTE *)palette, sizeof(palette), width, 0 - height, bpp,
				(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, my_malloc)) {
			free(uncompr);
			return -CUI_EMEM;
		}
	} else {	
		if (MyBuildBMPFile(uncompr, uncomprlen, NULL, 0, width, 0 - height, bpp,
				(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, my_malloc)) {
			free(uncompr);
			return -CUI_EMEM;
		}
	}
	free(uncompr);

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation Lucifen_elg_operation = {
	Lucifen_elg_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	Lucifen_elg_extract_resource,	/* extract_resource */
	Lucifen_LPK_save_resource,		/* save_resource */
	Lucifen_LPK_release_resource,	/* release_resource */
	Lucifen_LPK_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK Lucifen_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".LPK"), NULL, 
		NULL, &Lucifen_LPK_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".elg"), _T(".bmp"), 
		NULL, &Lucifen_elg_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	app_locale_id = locale_app_register(app_locale_configurations, 3);

	return 0;
}
