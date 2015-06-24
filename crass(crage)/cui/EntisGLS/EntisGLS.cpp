#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>
#include <crass/locale.h>
#include <utility.h>
#include <vector>
#include <string>
#include <xerisa.h>

/* 
【game参数游戏支持列表】
·ドS姉とボクの放尿関係
game=ane
·要！エプロン着用 体验版
game=ApronedOnly
·いろは ～秋の夕日に影ふみを～ 体験版
game=rural
·Alea －アレア－ 紅き月を遙かに望み 体験版
game=Alea
·donor［ドナー］ 体験版
game=donor
·絶対女子寮域！体験版
game=aftrial
·淫乳ファミレス ～深夜の母乳サービスはいかが？
game=fami
·隣り妻2 ～淫惑の閨房～ 体験版
game=NWives2
·アメサラサ ～雨と不思議な君に、恋をする 体験版
game=ame

Chanter（シャンテ） -キミの歌がとどいたら-.rar
[081205]ヨスガノソラ

搜索密码:
C源码
00405060  /$  83EC 10       SUB ESP,10                               ;  dec
00405063  |.  53            PUSH EBX
00405064  |.  55            PUSH EBP
00405065  |.  8BE9          MOV EBP,ECX                              ;  cxt0
00405067  |.  56            PUSH ESI
00405068  |.  57            PUSH EDI
00405069  |.  8B4D 14       MOV ECX,DWORD PTR SS:[EBP+14]            ;  (0)i?
0040506C  |.  8D41 01       LEA EAX,DWORD PTR DS:[ECX+1]             ;  ++i
0040506F  |.  8945 14       MOV DWORD PTR SS:[EBP+14],EAX
00405072  |.  8B5D 0C       MOV EBX,DWORD PTR SS:[EBP+C]             ;  (0x20)ERISADecodeContextLength
00405075  |.  8B55 08       MOV EDX,DWORD PTR SS:[EBP+8]             ;  113fa90? ERISADecodeContext
00405078  |.  3BC3          CMP EAX,EBX                              ;  orig_i VS ERISADecodeContextLength
0040507A  |.  895C24 18     MOV DWORD PTR SS:[ESP+18],EBX            ;  save ERISADecodeContextLength
0040507E  |.  895424 1C     MOV DWORD PTR SS:[ESP+1C],EDX            ;  ERISADecodeContext <-- 查看这个buffer
00405082  |.  7C 07         JL SHORT noa32c.0040508B
00405084  |.  C745 14 00000>MOV DWORD PTR SS:[EBP+14],0              ;  i = 0
0040508B  |>  8D7D 58       LEA EDI,DWORD PTR SS:[EBP+58]            ;  array32_1
0040508E  |.  BA A8FFFFFF   MOV EDX,-58                              ;  -58 <---关键字
00405093  |.  8BC7          MOV EAX,EDI                              ;  array32_1
00405095  |.  2BD5          SUB EDX,EBP                              ;  -58 - obj
00405097  |>  C640 C0 00    /MOV BYTE PTR DS:[EAX-40],0              ;  memset(array32_0, 0, 32)
0040509B  |.  C600 00       |MOV BYTE PTR DS:[EAX],0                 ;  memset(array32_1, 0, 32)
汇编：
ERIBshfBuffer@@DecodeBuffer	PROC	NEAR32 SYSCALL USES ebx esi edi

	mov	ebx, ecx
	ASSUME	ebx:PTR ERIBshfBuffer
	;
	; 復号の準備
	;
	mov	eax, [ebx].m_dwPassOffset
	mov	esi, eax			; esi = iPos
	inc	eax
	xor	edx, edx
	.IF	eax >= [ebx].m_strPassword.m_nLength
		xor	eax, eax
	.ENDIF
	mov	[ebx].m_dwPassOffset, eax
	;
	xor	ecx, ecx
	.REPEAT
		mov	DWORD PTR [ebx].m_bufBSHF[ecx], edx
		mov	DWORD PTR [ebx].m_maskBSHF[ecx], edx
		add	ecx, 4
	.UNTIL	ecx >= 32
	;
	; 暗号を復号
	;
	xor	edi, edi			; edi = iBit
	mov	edx, [ebx].m_strPassword.m_pszString 《---密码
	push	ebp
	mov	ebp, 256
	.REPEAT
		movzx	eax, BYTE PTR [edx + esi]
		inc	esi
		add	edi, eax
		cmp	esi, [ebx].m_strPassword.m_nLength
		sbb	edx, edx
		and	edi, 0FFH
			mov	eax, edi
			mov	ecx, edi
			shr	eax, 3
		and	esi, edx
			mov	dl, 80H
			and	ecx, 07H
			shr	dl, cl
		;
		.WHILE	[ebx].m_maskBSHF[eax] == 0FFH
			inc	eax
			add	edi, 8
			and	eax, 1FH  《-- 关键指令
			and	edi, 0FFH
		.ENDW
断点挺直后，搜索内存，关键字key=，然后dump这段内存，保存的就是xml数据
*/

/*
exe内藏加密资源型密码分析用：
M:\Program Files\highsox\愨懳彈巕椌堟両 懱尡斉
exe内藏加密资源型密码测试用：
Q:\Program Files\highsox\SpycyTrial
exe加壳，密码未知：
Q:\[unpack]\[noa](18禁ゲーム)すぺ～す☆とらぶる
*/

using namespace std;
using std::vector;

static int debug;
static int EntisGLS_locale_id;

static const TCHAR *simplified_chinese_strings[] = {
	_T("%s: crc校验失败(0x%08x), 应该是0x%08x.\n"),
	_T("%s: 指定的exe文件不存在, 请在exe=的后面用\"\"把路径信息括起来.\n"),
};

static const TCHAR *traditional_chinese_strings[] = {
	_T("%s: crc校驗失敗(0x%08x), 應該是0x%08x.\n"),
	_T("%s: 指定的exe文件不存在, 請在exe=的後面用\"\"把路徑信息括起來.\n"),
};

static const TCHAR *japanese_strings[] = {
	_T("%s: crcチェック失敗(0x%08x)、もとい0x%08x。\n"),
	_T("%s: 指定されたexeファイルは存在しません、exe=の後に\"\"でパスメッセージを括ってください。\n"),
};

static const TCHAR *default_strings[] = {
	_T("%s: crc error(0x%08x), shoule be 0x%08x.\n"),
	_T("%s: the exe file specified doesn't exist, please add \"\" pair to surond the path information behind exe=.\n"),
};

static struct locale_configuration EntisGLS_locale_configurations[4] = {
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

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information EntisGLS_cui_information = {
	_T("Leshade Entis, Entis-soft."),	/* copyright */
	_T("Entis Generalized Library Set version 3"),	/* system */
	_T(".noa .dat .arc"),	/* package */
	_T("0.5.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-5-2 17:08"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 header_code[8];		// "Entis\x1a\x00\x00"
	/*	erisafile.h:
		fidArchive			= 0x02000400,
		fidRasterizedImage	= 0x03000100,
		fidEGL3DSurface		= 0x03001100,
		fidEGL3DModel		= 0x03001200,
		fidUndefinedEMC		= -1
	*/
	u32 file_id;
	u32 reserved;	// 0
	s8 format_desc[40];
	u32 record_len_lo;
	u32 record_len_hi;
} noa_header_t;

typedef struct {
	s8 dir_record_code[8];	// "DirEntry"
	u32 dir_record_len_lo;
	u32 dir_record_len_hi;
	//u32 index_entries;	// 包含在dir_record_len
} noa_dir_record_t;

typedef struct {
	u32 data_len_lo;		// 原始长度
	u32 data_len_hi;	
	/*	erisafile.h:
		attrNormal			= 0x00000000,
		attrReadOnly		= 0x00000001,
		attrHidden			= 0x00000002,
		attrSystem			= 0x00000004,
		attrDirectory		= 0x00000010,
		attrEndOfDirectory	= 0x00000020,
		attrNextDirectory	= 0x00000040
	*/
	u32 attribute;
	/*	erisafile.h:
		etRaw				= 0x00000000,	// /raw
		etERISACode			= 0x80000010,	// default or /r
		etBSHFCrypt			= 0x40000000,	// /bshf
		etERISACrypt		= 0xC0000010
	*/
	u32 encode_type;
	u32 data_offset_lo;		// 相对于目录项
	u32 data_offset_hi;
	u8 second;
	u8 minute;
	u8 hour;
	u8 week;
	u8 day;
	u8 month;
	u16 year;
	u32 extra_info_len;
	u8 *extra_info;
	u32 name_len;
	u8 *name;
} noa_dir_record_info_t;

typedef struct {
	s8 file_record_code[8];	// "filedata"
	u32 file_record_len_lo;
	u32 file_record_len_hi;
} noa_file_record_t;
#pragma pack ()

typedef struct {
	char path[MAX_PATH];
	u64 offset;
	u64 length;
	u32 extra_info_len;
	u8 *extra_info;
	u32 encode_type;
	u32 attr;
} my_noa_entry_t;

struct game_key_sub_table {
	char path[32];	// 封包名
	char password[256];
};

struct game_key_table {
	char caption[128];	// 游戏名
	unsigned int number;
	struct game_key_sub_table sub_table[32];
};

static struct game_key_table default_key_table = {
	"default", 1, { { "", " " }, }
};

// 对于exe不能提取的游戏(比如exe加壳)，使用game参数
static struct game_key_table game_key_table_list[] = {
#if 0
	/* 要！エプロン着用 体验版(临时用) */
	{
		"ApronedOnly", 1,
		{
			{
				"ContainerB.noa", 
				"7DQ1Xm7ZahIv1ZwlFgyMTMryKC6OP9V6cAgL64WD5JLyvmeEyqTSA5rUbRigOtebnnK4MuOptwsbOf4K8UBDH4kpAUOQgB71Qr1qxtHGxQl8KZKj6WIYWpPh0G3JOJat"
			}
		}
	},
	/* Alea －アレア－ 紅き月を遙かに望み 体験版(临时用) */
	{
		"Alea", 1,
		{
			{
				"Data2.noa", 
				"faTSgh75PfbZHctOvW3hHphnA7LEaa8gm5qMcI5Bn4eZa3bzbRwA8E4CnNocr2pG16KMdhSyZSiRJ1b27ECw2RSsIyfxsgFyworNlq6q7qYnqZ9SO2cnQws4hFGjb9Dl"
			}
		}
	},
	/* donor［ドナー］ 体験版(临时用) */
	{
		"donor", 2,
		{
			{
				"data1.noa", "vfhvbqwydyqgbpv6teq"
			},
			{
				"data3.noa", "vdsygo3423byfwo"
			}
		}
	},
	/* 絶対女子寮域！体験版(临时用) */
	{
		"aftrial", 1,
		{
			"ContainerB.noa",
			"XFwcnU1Qn3mUICUv"
			"pw1KgEuc9eVRZWQz"
			"DtKgf9dSQSNcnB6O"
			"YvntbzwULCU81Lvx"
			"WLJFX64NvGPKMUPx"
			"lKrUVJaNHCzfdkCQ"
			"wXYp7rI6fsi4T1nV"
			"4rg75JoiXnTbkQFM"
		}
	},
	/* ドS姉とボクの放尿関係(临时用) */
	{
		"ane", 2,
		{
			{
				"d02.dat", 
				"vwerc7s65r21bnfu"
			},
			{
				"d03.dat", 
				"ctfvgbhnj67y8u"
			}
		}
	},
	{
		"SpaceTrouble",	1,
#if 0
		{
		"PtOJsyBhKoZHH4Lm"
		"9trW0EAhVWc5ufMR"
		"Xl7MoYsnOHfhRyRN"
		"n4ka4q9SddgGCVL2"
		"tPbIrLLTgejeSbnA"
		
		"UYVYYUY2YV12YVU9"
		"IF09IV32IV31NV12"
		"RAW8DXT1DXT2DXT3"
		"DXT4DXT5NVHSNVHU"
		"NVCSNVBF"

		"Z062D9322446GM"
		}
#endif
	},
	/* 淫乳ファミレス～深夜の母乳サービスはいかが？(临时用) */
	{
		"fami", 2, 
		{
			{
				"d01.dat",
				"vdiu$43AfUCfh9aksf"
			},
			{
				"d03.dat",
				"gaivnwq7365e021gf"
			},
		}
	},
	/* 隣り妻2 体験版(临时用) */
	{
		"NWives2", 2, 
		{
			{
				"Data6.noa",
				"ahyohsimohfaish1ao6ohh6edoh1yeezooDooj4Eabie0Aik4neeJohl0doo6fie2Iedao4poh9ahMoothohm3ba5LieC2beeba3hiyu1wit2fachae2Eecheed5kahc"
			},
			{
				"Data7.noa",
				"fee4Thaixuol2siema2azeiha7Quek2Egh5soov1soich1haeShu2Vuashai6ba1Gejei0eonooz0cooliciexoh1WohmaBaemaezieraibeevohjeceiT5eecogh1eo"
			},
		}
	},
	/* アメサラサ ～雨と不思議な君に、恋をする 体験版(临时用) */
	{
		"ame", 1,
		{
			{
				"system.noa", "lipsync"
			}
		}
	},
	/* いろは ～秋の夕日に影ふみを～ 体験版(临时用) */
	{
		"rural", 2,
		{
			{
				"scenario.noa",
				"0XT9ORIT0WlIl7U16yjKxTCyUOYZoN1cL7FxrBTiuvi0EY50b7Pu8uvLVlX0opOU5ash97Bkkqq"
			},
			{
				"graphics02.noa",
				"2o3bZEwqb9JMfkU4CYv2BaAD2sSdyUxtiAR4IQWG8UP6EmWkN0JcA9nmV6MAHH3AspaGBUqib3i"
			}
		}
	},
#endif
	/* 终止项 */
	{
		"", 0, 
	}
};

/* 包含运行时需要的所有可能的tbl */
static vector<struct game_key_table> runtime_game_key_table;
static struct game_key_sub_table *cur_sub_table;

//static BYTE BshfCode[512];
//static DWORD BshfCodePos;
//static DWORD BshfCodeLen;

static u32 ERISACrc(BYTE *data, DWORD data_len)
{
	BYTE crc[4] = { 0, 0, 0, 0 };

	for (DWORD i = 0; i < data_len; ++i)
		crc[i & 3] ^= data[i];

	return crc[0] | (crc[1] << 8) | (crc[2] << 16) | (crc[3] << 24);
}

#if 0
static void ERIBshfDecodeInit(const char *password)
{
	int len;

	if (!password)
		password = " ";
	len = strlen(password);
	strcpy((char *)BshfCode, password);
	if (len < 32) {
		BshfCode[len++] = 0x1b;
		for (DWORD i = len; i < 32; ++i)
			BshfCode[i] = BshfCode[i - 1] + BshfCode[i % len];
		len = 32;
	}
	BshfCodeLen = len;
	BshfCodePos = 0;

	if (debug) {
		printf("Dump BshfCode (%d bytes)\n", len);
		printf("Use password: \"%s\"\n", password);
		for (DWORD i = 0; i < len; ++i)
			printf("0x%02x ", BshfCode[i]);
		printf("\n");
	}
}

static int __ERIBshfDecode(BYTE *BSHF_dst, DWORD BSHF_dst_len, 
						   BYTE *BSHF_src, DWORD BSHF_src_len)
{
	DWORD act_BSHF_dst_len = 0;
	u32 crc = *(u32 *)&BSHF_src[BSHF_src_len - 4];

	for (DWORD i = 0; i < BSHF_src_len / 32; ++i) {
		BYTE BSHF_mask[32];
		BYTE BSHF_buf[32];
		BYTE bit = 0;
		BYTE cur = i;

		memset(BSHF_mask, 0, sizeof(BSHF_mask));
		memset(BSHF_buf, 0, sizeof(BSHF_buf));
		for (DWORD k = 0; k < 256; ++k) {
			bit += BshfCode[cur++ % BshfCodeLen];
			BYTE mask = 0x80 >> (bit & 7);
			while (BSHF_mask[bit / 8] == 0xff)
				bit += 8;
			while (BSHF_mask[bit / 8] & mask) {
				++bit;
				mask >>= 1;
				if (!mask) {
					bit += 8;
					mask = 0x80;
				}
			}
			BSHF_mask[bit / 8] |= mask;
			if (BSHF_src[k / 8] & (0x80 >> (k & 7)))
				BSHF_buf[bit / 8] |= mask;
		}
		BSHF_src += 32;
		for (k = 0; k < 32; ++k) {
			BSHF_dst[act_BSHF_dst_len++] = BSHF_buf[k];
			if (act_BSHF_dst_len >= BSHF_dst_len)
				goto out;
		}
	}
out:
	return ERISACrc(BSHF_dst, act_BSHF_dst_len, crc);
}
#endif

#if 1
static int ERIBSHFDecode(BYTE *dec, DWORD dec_len, 
						 BYTE *enc, DWORD enc_len, char *pwd)
{
	EMemoryFile DEC;
	DEC.Open(enc, enc_len);

	ESLFileObject *file = DEC.Duplicate();
	ERISADecodeContext BSHF(dec_len);
	BSHF.AttachInputFile(file);
	BSHF.PrepareToDecodeBSHFCode(pwd);

	EStreamBuffer buf;
	BYTE *ptrBuf = (BYTE *)buf.PutBuffer(dec_len);
	int ret = BSHF.DecodeBSHFCodeBytes((SBYTE *)ptrBuf, dec_len);
	if (ret > 0)
		memcpy(dec, ptrBuf, dec_len);
	delete file;

	return ret;
}
#else
static int ERIBshfDecode(BYTE *BSHF_dst, DWORD BSHF_dst_len, 
						 BYTE *BSHF_src, DWORD BSHF_src_len)
{
	int ret = __ERIBshfDecode(BSHF_dst, BSHF_dst_len, BSHF_src, BSHF_src_len);
	if (!ret) {
		for (DWORD i = 0; i < runtime_game_key_table.size(); ++i) {
			struct game_key_table &tbl = runtime_game_key_table[i];
			for (DWORD k = 0; k < tbl.number; ++k) {
				if (cur_sub_table && cur_sub_table == &tbl.sub_table[k])
					continue;
				ERIBshfDecodeInit(tbl.sub_table[k].password);
				ret = __ERIBshfDecode(BSHF_dst, BSHF_dst_len, BSHF_src, BSHF_src_len);
				if (ret) {
					cur_sub_table = &tbl.sub_table[k];
					return 1;
				}
			}
		}
	}
	cur_sub_table = default_key_table.sub_table;
	return ret;
}
#endif

static void parse_xml_data(char *xml_cfg)
{
	string xml = xml_cfg;
	string::size_type begin, end;
	char game[256] = "no_name";
	int game_name_len = strlen(game);
	struct game_key_table entry;
	unsigned int &i = entry.number;

	begin = xml.find("<display caption=\"", 0);
	if (begin != string::npos) {
		begin += strlen("<display caption=\"");
		end = xml.find("\"", begin);
		if (end == string::npos)
			return;
		memcpy(game, &xml[begin], end - begin);
		game_name_len = end - begin;
	}

	begin = 0;
	i = 0;
	while ((begin = xml.find("<archive ", begin)) != string::npos) {
		end = xml.find("/>", begin);
		if (end == string::npos)
			break;

		string::size_type path = xml.find("path=", begin);
		if (path == string::npos || path >= end)
			break;

		string::size_type key = xml.find("key=", begin);
		if (key == string::npos || key >= end) {
			begin = end + 2;
			continue;
		}

		memset(entry.sub_table[i].password, 0, sizeof(entry.sub_table[i].password));
		memcpy(entry.sub_table[i].password, &xml[key + 5] /* "key=\"" */, 
			end - key - 5 /* "key=\"" */ - 1 /* "\"" */);

		memset(entry.sub_table[i].path, 0, sizeof(entry.sub_table[i].path));
		string::size_type dim;

		dim = xml.rfind("\\", key - 1);
		if (dim == string::npos || dim <= path)
			dim = path + 6 /* "path=\"" */;
		else
			++dim;

		// 通常情况，xml里会对加密的noa所在光盘和硬盘上不同的位置有
		// 2组key，内容都一样，所以这里进行检查，不加入重复的key。
		for (DWORD k = 0; k < i; ++k) {
			if (!strcmp(entry.sub_table[k].path, &xml[dim]))
				goto not_insert;
		}
		memcpy(entry.sub_table[i].path, &xml[dim], key - 1 /* " " */ - dim - 1 /* "\"" */);
not_insert:
		begin = end + 2;
		++i;
	}
	if (i) {
		game[game_name_len] = 0;
		strcpy(entry.caption, game);
		runtime_game_key_table.push_back(entry);
	}
}

static int load_exe_xml(const char *exe_file)
{
	if (!exe_file)
		return 0;

	HMODULE exe = LoadLibraryA(exe_file);
	if ((DWORD)exe > 31) {
		HRSRC code = FindResourceA(exe, "IDR_COTOMI", (const char *)RT_RCDATA);
		if (code) {
			DWORD sz = SizeofResource(exe, code); 
			HGLOBAL hsrc = LoadResource(exe, code); 
			if (hsrc) {
				LPVOID cipher = LockResource(hsrc); 
				if (cipher) {
					EMemoryFile xml;
					xml.Open(cipher, sz);
					ERISADecodeContext ERISA(0x10000);
					ESLFileObject *dup = xml.Duplicate();
					ERISA.AttachInputFile(dup);
					ERISA.PrepareToDecodeERISANCode();
					EStreamBuffer buf;
					BYTE *ptrBuf = (BYTE *)buf.PutBuffer(0x10000);
					unsigned int Result = ERISA.DecodeERISANCodeBytes((SBYTE *)ptrBuf, 0x10000);
					if (debug)
						MySaveFile(_T("exe.xml"), ptrBuf, Result);
					parse_xml_data((char *)ptrBuf);
					delete dup;
					return runtime_game_key_table.size();
				}
				FreeResource(hsrc);
			}
		}
		FreeLibrary(exe);
	} else {
		TCHAR exe_path[MAX_PATH];
		acp2unicode(exe_file, -1, exe_path, MAX_PATH);
		locale_app_printf(EntisGLS_locale_id, 1, exe_path);
	}

	return 0;
}

/********************* noa *********************/

/* 封包匹配回调函数 */
static int EntisGLS_noa_match(struct package *pkg)
{
	noa_header_t noa_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &noa_header, sizeof(noa_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(noa_header.header_code, "Entis\x1a\x00\x00", 8)
			|| noa_header.file_id != 0x02000400
			|| noa_header.reserved) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}
	//BshfCodePos = 0;

	return 0;	
}

static int noa_dir_record_process(struct package *pkg, 
								  vector<my_noa_entry_t> &my_noa_index,
								  char *path_name)
{
	u64 base_offset;
	pkg->pio->locate64(pkg, (u32 *)&base_offset, ((u32 *)&base_offset) + 1);

	noa_dir_record_t dir_rec;
	if (pkg->pio->read(pkg, &dir_rec, sizeof(dir_rec)))
		return -CUI_EREAD;

	if (strncmp(dir_rec.dir_record_code, "DirEntry", sizeof(dir_rec.dir_record_code)))
		return -CUI_EMATCH;

	BYTE *dir_rec_info = new BYTE[dir_rec.dir_record_len_lo];
	if (!dir_rec_info)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, dir_rec_info, dir_rec.dir_record_len_lo)) {
		delete [] dir_rec_info;
		return -CUI_EREAD;
	}

	BYTE *p_dir = dir_rec_info;
	u32 index_entries = *(u32 *)p_dir;
	p_dir += 4;

	int ret = 0;
	for (DWORD i = 0; i < index_entries; ++i) {		
		my_noa_entry_t entry;
		
		entry.length = *(u64 *)p_dir;
		p_dir += 8;		
		u32 attr = *(u32 *)p_dir;
		p_dir += 4;		
		entry.attr = attr;
		entry.encode_type = *(u32 *)p_dir;
		p_dir += 4;
		entry.offset = *(u64 *)p_dir + base_offset;
		p_dir += 16;	// bypass filetime
		entry.extra_info_len = *(u32 *)p_dir;
		p_dir += 4;
		if (entry.extra_info_len) {
			entry.extra_info = new BYTE[entry.extra_info_len];
			if (!entry.extra_info)
				break;
		} else
			entry.extra_info = NULL;
		p_dir += entry.extra_info_len;
		sprintf(entry.path, "%s\\%s", path_name, (char *)p_dir + 4);		
		p_dir += 4 + *(u32 *)p_dir;
		my_noa_index.push_back(entry);
		if (attr == 0x00000010)	{	// attrDirectory
			noa_file_record_t file_rec;	// 目录项也是个filedata

			if (pkg->pio->readvec64(pkg, &file_rec, sizeof(file_rec), 0, 
					(u32)entry.offset, (u32)(entry.offset >> 32), IO_SEEK_SET)) {
				ret = -CUI_EREADVEC;
				break;
			}
			ret = noa_dir_record_process(pkg, my_noa_index, entry.path);
			if (ret)
				break;
		} else if (attr == 0x00000020) {	// attrEndOfDirectory
			printf("attrEndOfDirectory!!\n");exit(0);
		} else if (attr == 0x00000040) {	// attrNextDirectory
			printf("attrNextDirectory!!\n");exit(0);
		}
	}
	delete [] dir_rec_info;

	return ret;
}

/* 封包索引目录提取函数 */
static int EntisGLS_noa_extract_directory(struct package *pkg,
										  struct package_directory *pkg_dir)
{
	vector<my_noa_entry_t> my_noa_index;
	int ret;

	my_noa_entry_t root_dir;
	memset(&root_dir, 0, sizeof(root_dir));
	root_dir.attr = 0x00000010;
	my_noa_index.push_back(root_dir);
	ret = noa_dir_record_process(pkg, my_noa_index, ".");
	if (ret)
		return ret;

	DWORD index_entries = 0;
	for (DWORD i = 0; i < my_noa_index.size(); ++i) {
//		printf("%d %s %x %x %x\n", i, my_noa_index[i].path, 
//			my_noa_index[i].attr, my_noa_index[i].offset,
//			(u32)my_noa_index[i].length);
		if (my_noa_index[i].attr != 0x00000010 && 
			my_noa_index[i].attr != 0x00000020 && 
			my_noa_index[i].attr != 0x00000040)
				++index_entries;
	}

	my_noa_entry_t *index = new my_noa_entry_t[index_entries];
	if (!index)
		return -CUI_EMEM;

	DWORD k = 0;
	for (i = 0; i < my_noa_index.size(); ++i) {
		if (my_noa_index[i].attr == 0x00000010)	{	// attrDirectory
			if (my_noa_index[i].extra_info)
				delete [] my_noa_index[i].extra_info;
		} else if (my_noa_index[i].attr == 0x00000020) {	// attrEndOfDirectory
			if (my_noa_index[i].extra_info)
				delete [] my_noa_index[i].extra_info;
			printf("attrEndOfDirectory!!\n");exit(0);
		} else if (my_noa_index[i].attr == 0x00000040) {	// attrNextDirectory
			if (my_noa_index[i].extra_info)
				delete [] my_noa_index[i].extra_info;
			printf("attrNextDirectory!!\n");exit(0);
		} else
			index[k++] = my_noa_index[i];		
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index;
	pkg_dir->directory_length = index_entries * sizeof(my_noa_entry_t);
	pkg_dir->index_entry_length = sizeof(my_noa_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int EntisGLS_noa_parse_resource_info(struct package *pkg,
											struct package_resource *pkg_res)
{
	my_noa_entry_t *my_noa_entry;

	my_noa_entry = (my_noa_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_noa_entry->path);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = (u32)my_noa_entry->length;
	pkg_res->actual_data_length = (u32)my_noa_entry->length;
	pkg_res->offset = (u32)my_noa_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int EntisGLS_noa_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	my_noa_entry_t *my_noa_entry = (my_noa_entry_t *)pkg_res->actual_index_entry;
	noa_file_record_t file_rec;
	u64 offset = my_noa_entry->offset;

	if (pkg->pio->readvec64(pkg, &file_rec, sizeof(file_rec), 0,
		(u32)offset, (u32)(offset >> 32), IO_SEEK_SET))
			return -CUI_EREADVEC;

	u64 raw_len = (u64)file_rec.file_record_len_lo | ((u64)file_rec.file_record_len_hi << 32);
	BYTE *raw = new BYTE[(u32)raw_len];
	if (!raw)
		return -CUI_EMEM;

	offset += sizeof(file_rec);
	if (pkg->pio->readvec64(pkg, raw, (u32)raw_len, (u32)(raw_len >> 32), 
		(u32)offset, (u32)(offset >> 32), IO_SEEK_SET)) {
			delete [] raw;
			return -CUI_EREADVEC;
	}

	pkg_res->raw_data_length = raw_len;
	pkg_res->actual_data_length = my_noa_entry->length;
	if (my_noa_entry->encode_type == 0x00000000)	// etRaw
		pkg_res->raw_data = raw;
	else if (my_noa_entry->encode_type == 0x80000010) {	// etERISACode
		EMemoryFile dec;
		dec.Open(raw, pkg_res->raw_data_length);
		ERISADecodeContext ERISA(pkg_res->actual_data_length);

		ESLFileObject *dup = dec.Duplicate();
		ERISA.AttachInputFile(dup);
		ERISA.PrepareToDecodeERISANCode();
		EStreamBuffer buf;
		BYTE *ptrBuf = (BYTE *)buf.PutBuffer(pkg_res->actual_data_length);
		pkg_res->actual_data_length = ERISA.DecodeERISANCodeBytes((SBYTE *)ptrBuf, 
			pkg_res->actual_data_length);
		BYTE *act = new BYTE[pkg_res->actual_data_length];
		if (!act) {
			delete [] raw;
			return -CUI_EMEM;
		}
		memcpy(act, ptrBuf, pkg_res->actual_data_length);
		delete dup;
		pkg_res->raw_data = raw;
		pkg_res->actual_data = act;
	} else if (my_noa_entry->encode_type == 0x40000000) {	// etBSHFCrypt
#if 1
		BYTE *act = new BYTE[pkg_res->actual_data_length];
		if (!act) {
			delete [] raw;
			return -CUI_EMEM;
		}

		char pkg_name[MAX_PATH];
		unicode2acp(pkg_name, MAX_PATH, pkg->name, -1);

		for (DWORD i = 0; i < runtime_game_key_table.size(); ++i) {
			struct game_key_table &tbl = runtime_game_key_table[i];

			for (DWORD k = 0; k < tbl.number; ++k) {
				if (tbl.sub_table[k].path[0] && strcmpi(pkg_name, tbl.sub_table[k].path))
					continue;

				if (!pkg_res->index_number && debug) {					
					printf("%s: using password \"%s\"", pkg_name,
						tbl.sub_table[k].password);
					if (tbl.sub_table[k].path[0])
						printf(" for %s\n\n", tbl.sub_table[k].path);
				}

				int act_len = ERIBSHFDecode(act, pkg_res->actual_data_length, 
					raw, pkg_res->raw_data_length-4, 
					tbl.sub_table[k].password);

				if (act_len <= 0) {
					delete [] act;
					delete [] raw;
					return -CUI_EUNCOMPR;
				}

				u32 crc = *(u32 *)&raw[pkg_res->raw_data_length-4];
				u32 act_crc = ERISACrc(act, act_len);
				if (crc != act_crc) {
					WCHAR res_name[MAX_PATH];
					acp2unicode(pkg_res->name, -1, res_name, MAX_PATH);
					locale_app_printf(EntisGLS_locale_id, 0, res_name,
						crc, act_crc);
					delete [] act;
					delete [] raw;
					return -CUI_EMATCH;
				} else {
					pkg_res->actual_data_length = act_len;
					goto out;
				}
			}
		}
	out:
		pkg_res->raw_data = raw;
		pkg_res->actual_data = act;
#else
		BYTE *dec = new BYTE[pkg_res->actual_data_length];
		if (!dec) {
			delete [] raw;
			return -CUI_EMEM;
		}

		if (!ERIBshfDecode(dec, pkg_res->actual_data_length, raw, raw_len)) {
			printf("%s: crc不正确, 可能是文件损坏或者需要指定正确的exe/game/pwd参数提供密码.\n", pkg_res->name);
			printf("%s: crc is incorrect, maybe the data is corrupt or need correct parameter exe/game/pwd to provide the password.\n", pkg_res->name);
			delete [] dec;
			delete [] raw;
			return -CUI_EMATCH;
		}
		delete [] raw;
		pkg_res->actual_data = dec;
#endif
	} else if (my_noa_entry->encode_type == 0xC0000010) {	// etERISACrypt
		printf("unsupport type etERISACrypt\n");
		pkg_res->raw_data = raw;
	} else {
		printf("unsupport type unknown\n");
		pkg_res->raw_data = raw;
	}

	return 0;
}

/* 资源保存函数 */
static int EntisGLS_noa_save_resource(struct resource *res, 
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
static void EntisGLS_noa_release_resource(struct package *pkg, 
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
static void EntisGLS_noa_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		my_noa_entry_t *index = (my_noa_entry_t *)pkg_dir->directory;

		for (DWORD i = 0; i < pkg_dir->index_entries; ++i)
			delete [] index[i].extra_info;
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation EntisGLS_noa_operation = {
	EntisGLS_noa_match,					/* match */
	EntisGLS_noa_extract_directory,		/* extract_directory */
	EntisGLS_noa_parse_resource_info,	/* parse_resource_info */
	EntisGLS_noa_extract_resource,		/* extract_resource */
	EntisGLS_noa_save_resource,			/* save_resource */
	EntisGLS_noa_release_resource,		/* release_resource */
	EntisGLS_noa_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK EntisGLS_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".noa"), NULL, 
		_T("ERISA-Archive file（乃亞アーカイバ）"), &EntisGLS_noa_operation, 
		CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_RES | CUI_EXT_FLAG_RECURSION))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		_T("ERISA-Archive file（乃亞アーカイバ）"), &EntisGLS_noa_operation, 
		CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_RES | CUI_EXT_FLAG_RECURSION))
			return -1;

	if (callback->add_extension(callback->cui, _T(".arc"), NULL, 
		_T("ERISA-Archive file（乃亞アーカイバ）"), &EntisGLS_noa_operation, 
		CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_RES | CUI_EXT_FLAG_RECURSION))
			return -1;

#if 0
	if (callback->add_extension(callback->cui, _T(".eri"), NULL, 
		_T("Entis Rasterized Image - 1（エリちゃん）"), &EntisGLS_eri_operation, 
		CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".mio"), NULL, 
		_T("Music Interleaved and Orthogonal transformaed（ミーオ）"), 
		&EntisGLS_mio_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".csx"), NULL, 
		_T("Cotopha Image file"), &EntisGLS_csx_operation, 
		CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;
#endif

	EntisGLS_locale_id = locale_app_register(EntisGLS_locale_configurations, 3);

	debug = !!(unsigned long)get_options("debug");

	const char *exe = get_options("exe");
	const char *game = get_options("game");	
	if (!load_exe_xml(exe) && game) {
		for (DWORD i = 0; game_key_table_list[i].caption[0]; ++i) {
			if (!strcmpi(game_key_table_list[i].caption, game))
				break;
		}
		if (game_key_table_list[i].caption[0])
			runtime_game_key_table.push_back(game_key_table_list[i]);
	}
	
	const char *password = get_options("pwd");
	if (!runtime_game_key_table.size() && password) {
		struct game_key_table pwd_game;

		strcpy(pwd_game.caption, "password");
		pwd_game.number = 1;
		strcpy(pwd_game.sub_table[0].password, password);
		strcpy(pwd_game.sub_table[0].path, "");
		runtime_game_key_table.push_back(pwd_game);
	} else
		runtime_game_key_table.push_back(default_key_table);

	//ERIBshfDecodeInit(password);

	if (debug) {
		printf("Dump key ring (%d):\n", runtime_game_key_table.size());
		for (DWORD i = 0; i < runtime_game_key_table.size(); ++i) {
			printf("%2d) game \"%s\" (%d)\n", i, runtime_game_key_table[i].caption, 
				runtime_game_key_table[i].number);
			for (DWORD k = 0; k < runtime_game_key_table[i].number; ++k) {
				printf("\t%2d> path \"%s\", password \"%s\"\n", k,
					runtime_game_key_table[i].sub_table[k].path, 
					runtime_game_key_table[i].sub_table[k].password);
			}
		}
	}

	return 0;
}
