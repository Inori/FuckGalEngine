#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <crass\locale.h>
#include <stdio.h>
#include <zlib.h>
#include <utility.h>
#include "SFMT-19937.h"
#include "SFMT-132049.h"

/* 寻找解密key的方法：
系统首先会读取system.arc，下ReadFile断点，然后回溯出fread(),
fread()返回后就是decode()函数（buf,buf_len），这里一上来会有个全局变量
执行一个保存key的指针，这里的值就是key。
然后搜索那个全局变量，找到给那个全局变量复制的语句。
它的下面有类似这样的代码：
00494CE0   .  A3 CCBD8000   MOV     DWORD PTR DS:[80BDCC], EAX 《－－key buf的地址
00494CE5   .  890D C4BD8000 MOV     DWORD PTR DS:[80BDC4], ECX
00494CEB   .  8915 C8BD8000 MOV     DWORD PTR DS:[80BDC8], EDX
00494CF1   .  74 12         JE      SHORT zw.00494D05
00494CF3   .  B9 BCBD8000   MOV     ECX, zw.0080BDBC 《－ini数组
00494CF8   .  E8 73D0F7FF   CALL    zw.00411D70 《－－gen_key
00494CFD   .  8B0D CCBD8000 MOV     ECX, DWORD PTR DS:[80BDCC]
00494D03   .  8901          MOV     DWORD PTR DS:[ECX], EAX 《－－key赋值
00494D05   >  68 B04D4900   PUSH    zw.00494DB0
00494D0A   .  E8 4D27FCFF   CALL    zw.0045745C
00494D0F   .  59            POP     ECX
00494D10   .  C3            RETN


 */

/* 寻找.asb的解密参数：
00431D80  /$  53            PUSH EBX
00431D81  |.  56            PUSH ESI
00431D82  |.  8BF1          MOV ESI,ECX
00431D84  |.  57            PUSH EDI
00431D85  |.  68 27100620   PUSH 20061027 <--- crc参数值
00431D8A  |.  33DB          XOR EBX,EBX
00431D8C  |.  33C0          XOR EAX,EAX
00431D8E  |.  B9 41000000   MOV ECX,41 <--- 关键字
00431D93  |.  8D7E 10       LEA EDI,DWORD PTR DS:[ESI+10]
00431D96  |.  F3:AB         REP STOS DWORD PTR ES:[EDI]
00431D98  |.  6A 0F         PUSH 0F <--- 关键字（string参数字符串的长度）
00431D9A  |.  68 18464800   PUSH sentitry.00484618（string参数字符串）
00431D9F  |.  899E 14010000 MOV DWORD PTR DS:[ESI+114],EBX
00431DA5  |.  899E 18010000 MOV DWORD PTR DS:[ESI+118],EBX
00431DAB  |.  899E 1C010000 MOV DWORD PTR DS:[ESI+11C],EBX
00431DB1  |.  899E 20010000 MOV DWORD PTR DS:[ESI+120],EBX
00431DB7  |.  E8 8472FDFF   CALL sentitry.00409040                                              ;  crc32_(20061027,体验版)
00431DBC  |.  83C4 0C       ADD ESP,0C
00431DBF  |.  8D7E 08       LEA EDI,DWORD PTR DS:[ESI+8]
00431DC2  |.  50            PUSH EAX
00431DC3  |.  8BCF          MOV ECX,EDI
00431DC5  |.  E8 4698FDFF   CALL sentitry.0040B610                                              ;  init_genrand
00431DCA  |.  8BCF          MOV ECX,EDI
00431DCC  |.  E8 EF98FDFF   CALL sentitry.0040B6C0                                              ;  genrand_int32
00431DD1  |.  8BCF          MOV ECX,EDI
00431DD3  |.  8986 24010000 MOV DWORD PTR DS:[ESI+124],EAX                                      ;  save decode_key
*/

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information AZSystem_cui_information = {
	_T("casper 氏"),		/* copyright */
	_T("AZSystem"),			/* system */
	_T(".arc"),				/* package */
	_T("0.7.2"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-5-6 22:28"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP | ACUI_ATTRIBUTE_PRELOAD
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];			/* "ARC" */
	u32 suffix_number;
	u32 index_entries;
	u32 compr_index_length;
	s8 suffix[32];			/* 扩展名列表 */
} arc_header_t;

typedef struct {
	s8 magic[4];				/* "ARC0x1a" */
	u32 suffix_number;
	u32 index_entries;
	u32 compr_index_length;
	s8 suffix[32];				/* eg: .tbl */
} arc1a_header_t;

typedef struct {
	u32 offset;
	u32 length;
	u32 name_crc;			/* 不包含NULL */
	u32 always_zero;
	s8 name[32];
} arc_entry_t;

typedef struct {
	u32 crc;				/* 数据部分的校验值 */
	u32 bitmaplen;			/* 压缩标志长度 */
	u32 comprlen1;			/* 标志字节为1时使用的压缩数据长度 */
	u32 comprlen0;			/* 标志字节为0时使用的压缩数据长度 */
	u32 uncomprlen;
} arc1a_compr_index_t;

typedef struct {
	s8 magic[4];			/* "MAP\x1a" */
	u32 width;	
	u32 height;	
	u32 bitmaplen;			/* 压缩标志长度 */
	u32 comprlen0;			/* 标志字节为0时使用的压缩数据长度 */
	u32 comprlen1;			/* 标志字节为1时使用的压缩数据长度 */
} map1a_header_t;

typedef struct {
	u32 offset;
	u32 length;
	u32 name_crc;
	u32 always_zero;
	s8 name[48];
} arc1a_entry_t;

typedef struct {
	s8 magic[4];			/* "ASB" */
	u32 comprlen;
	u32 uncomprlen;
	u32 unknown;
} asb_header_t;

typedef struct {
	s8 magic[4];			/* "ASB\x1a" */
	u32 comprlen;
	u32 uncomprlen;
} asb1a_header_t;

typedef struct {
	s8 magic[4];			/* "TYP1" */
	u8 color_depth;
	u8 flag;
	u16 width;
	u16 height;
	u32 max_comprlen;
	u32 comprlen[4];		/* RGBA是分块压缩的 */
} cpb_header_t;

typedef struct {
	s8 magic[4];			/* "CPB" */
	u8 unknown;				// 0	
	u8 version;				// 1有效
	u16 width;
	u16 color_depth;
	u16 height;
	u32 max_comprlen;
	u32 comprlen[4];		/* RGBA是分块压缩的 */
} cpb_old_header_t;

typedef struct {
	s8 magic[4];			/* "CPB" */
	u8 color_depth;	
	u8 version;				// 1有效
	u8 unknown;				// 0
	u16 width;
	u8 unknown1;			// 0
	u16 height;
	u32 max_comprlen;
	u32 comprlen[4];		/* RGBA是分块压缩的 */
} cpb_old2_header_t;

typedef struct {
	s8 magic[4];			/* "CPB\x1a" */
	u8 unknown;				// 0
	u8 color_depth;
	u8 unknown1;			// 0
	u8 version;				// 1有效
	u16 width;
	u16 height;
	u32 max_comprlen;
	u32 comprlen[4];		/* RGBA是分块压缩的 */
} cpb1a_header_t;

// 每块最长4096字节，每项64字节（名字前面的4字节是名字的crc32值）
typedef struct {
	s8 magic[4];		// "TBL"
	u32 comprlen;
	u32 uncomprlen;
	u32 blocks;			// 块数
	u32 entries;		// 总项数
} tbl_header_t;

typedef struct {
	s8 magic[4];			/* "TBL0x1a" */
	u32 comprlen;
	u32 uncomprlen;
} tbl1a_header_t;

typedef struct {
	s8 magic[4];			/* "MPP" */
	u32 index_entries;
	u32 width;
	u32 height;
	u32 unknown;			// 1
} mpp_header_t;
#pragma pack ()

typedef struct {
	char name[MAX_PATH];
	u32 length;
	u32 offset;
} mpp_entry_t;


static const TCHAR *simplified_chinese_strings[] = {
	_T("[警告] %s: 解密key保存在system.arc中，")
		_T("因此你必须使用\"system\"参数指定它的路径。\n")
		_T("例如：\n")
		_T("\tcrage -p xxx.arc -O system=X:\\system.arc\n"),
	_T("[错误] %s: 没有找到解密key ...\n")
};

static const TCHAR *traditional_chinese_strings[] = {
	_T("[警告] %s: 解密key保存在system.arc中，")
		_T("因此你必須使用\"system\"參數指定它的路徑。\n")
		_T("例如：\n")
		_T("\tcrage -p xxx.arc -O system=X:\\system.arc\n"),
	_T("[錯誤] %s: 沒有找到解密key ...\n")
};

static const TCHAR *japanese_strings[] = {
	_T("[警告] %s: 解読キーはsystem.arcに保存されていますので、")
		_T("パラメーター\"system\"を使ってそのパスを指定しなければなりません。\n")
		_T("例えば：\n")
		_T("\tcrage -p xxx.arc -O system=X:\\system.arc\n"),
	_T("[エラー] %s: 解読キーが見つかりません ...\n")
};

static const TCHAR *default_strings[] = {
	_T("[WARNING] %s: the key is saved in system.arc, ")
		_T("so you must specify its path with \"system\" parameter.\n")
		_T("for expample:\n")
		_T("\tcrage -p xxx.arc -O system=X:\\system.arc\n"),
	_T("[ERROR] %s: not found key ...\n")
};

static struct locale_configuration AZSystem_locale_configurations[4] = {
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

static int AZSystem_locale_id;

enum {
	SFMT19937 = 1,
	SFMT132049 = 2,
};

#define MAX_INI		4
#define INI_ARRAY	\
	u32 ini[MAX_INI][4] = { 	\
		{ 0x2f4d7dfe, 0x47345292, 0x1ba5fe82, 0x7bc04525 },	\
		{ 0xff2c9171, 0x4a676214, 0xb8c62e81, 0x504ab64a },	/* Zwei Worter -ツヴァイ ウォルター (且不用ewf_crc) */ \
		{ 0x7f7f85c1, 0x49a60caa, 0x97daf182, 0xb86a1a05 },	/* Sentinel WEB体験版 ver1.5 */ \
		{ 0x76c9a719, 0x46e848d2, 0xb43713bf, 0x33770358 },	/* Zwei Worter 体験版 */ \
	}

#define INIT_ASB_ARRAY	\
	struct {	\
		char string[MAX_PATH];	\
		u32 crc;	\
	} asb_decode_table[] = {	\
		{	\
			"Sentinel 懱尡斉", 0x20061215	\
		},	\
		{	\
			"Sentinel 懱尡斉", 0x20061027	\
		},	\
		{	\
			"ZWEI WORTER -TRIAL VERSION-", 0x20070323	\
		},	\
		{	\
			"", 0	\
		}	\
	}

static int mt_mode;
static int old_arc;	// 介于arc1a和arc之间的一种格式

extern u32 ewf_crc(void *buffer, u32 buffer_size, u32 previous_key);
extern void init_genrand(unsigned long s);
extern unsigned long genrand_int32(void);

static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static int azsystem_decompress(BYTE *uncompr, DWORD uncomprlen, BYTE *compr, DWORD comprlen)
{
	DWORD act_uncomprlen;
	
	if (ewf_crc(compr + 4, comprlen - 4, 1) != *(u32 *)compr)
		return -CUI_EMATCH;

	act_uncomprlen = uncomprlen;
	if (uncompress(uncompr, &act_uncomprlen, compr + 4, comprlen - 4) != Z_OK)
		return -CUI_EUNCOMPR;

	if (act_uncomprlen != act_uncomprlen)
		return -CUI_EUNCOMPR;

	return 0;
}

static int azsystem_arc1a_decompress(BYTE *uncompr, DWORD uncomprlen, BYTE *compr, DWORD comprlen)
{
	DWORD act_uncomprlen;

	if (crc32(0, compr + 4, comprlen - 4) != *(u32 *)compr)
		return -CUI_EMATCH;

	act_uncomprlen = uncomprlen;
	if (uncompress(uncompr, &act_uncomprlen, compr + 4, comprlen - 4) != Z_OK)
		return -CUI_EUNCOMPR;

	if (act_uncomprlen != uncomprlen)
	return -CUI_EUNCOMPR;

	return 0;
}

static int asb_decompress(BYTE *uncompr, DWORD uncomprlen, BYTE *compr, DWORD comprlen)
{
	u32 key = uncomprlen ^ 0x9E370001;
	DWORD dword_count = comprlen / 4;
	u32 *enc_data = (u32 *)compr; 

	while (dword_count--)
		*enc_data++ -= key;

	return azsystem_decompress(uncompr, uncomprlen, compr, comprlen);
}

static int asb1a_decompress(BYTE *uncompr, DWORD uncomprlen, BYTE *compr, DWORD comprlen, DWORD key)
{
	DWORD dword_count = comprlen / 4;
	u32 *enc_data = (u32 *)compr; 
	key = (uncomprlen ^ key) ^ ((((uncomprlen ^ key) << 12) | (uncomprlen ^ key)) << 11);
	while (dword_count--)
		*enc_data++ -= key;
	return azsystem_arc1a_decompress(uncompr, uncomprlen, compr, comprlen);
}

static int __asb1a_process(asb1a_header_t *asb1a_header, 
						   const char *string, u32 seed,
						   BYTE *uncompr, DWORD uncomprlen)
{
	BYTE *asb1a_compr = (BYTE *)(asb1a_header + 1);
	seed = crc32(seed, (BYTE *)string, strlen(string));
	init_genrand(seed);
	u32 key = genrand_int32();
	return asb1a_decompress(uncompr, uncomprlen, asb1a_compr, 
		asb1a_header->comprlen, key);
}

static int asb1a_process(BYTE *compr, DWORD comprlen, BYTE *uncompr, DWORD uncomprlen)
{
	asb1a_header_t *asb1a_header = (asb1a_header_t *)compr;
	INIT_ASB_ARRAY;
	BYTE *compr_backup = (BYTE *)malloc(comprlen);
	if (!compr_backup)
		return -CUI_EMEM;
	memcpy(compr_backup, compr, comprlen);
	
	int ret = -CUI_EMATCH;
	for (DWORD i = 0; asb_decode_table[i].string[0]; ++i) {
		ret = __asb1a_process(asb1a_header, asb_decode_table[i].string,
				asb_decode_table[i].crc, uncompr, uncomprlen);
		if (!ret)
			break;
		memcpy(compr, compr_backup, comprlen);
	}
	if (!asb_decode_table[i].string[0]) {
		const char *string = get_options("string");
		const char *crc = get_options("crc");
		if (!string || !crc) {
			free(compr_backup);
			return -CUI_EMATCH;
		}
		memcpy(compr, compr_backup, comprlen);
		ret = __asb1a_process(asb1a_header, string, strtoul(crc, NULL, 0), 
			uncompr, uncomprlen);
	}
	free(compr_backup);
	return ret;
}

static u32 gen_key(u32 *key, unsigned int key_len)
{
	u32 _key;

	_key = crc32(key[1] & 0xffff, (BYTE *)key, key_len);
	_key ^= crc32(key[1] >> 16, (BYTE *)key, key_len);
	_key ^= crc32(key[0], (BYTE *)key, key_len);
	_key ^= crc32(key[2], (BYTE *)key, key_len);
	_key ^= crc32(key[3], (BYTE *)key, key_len);
	return _key ^ key[0];
}

#if 1
static void decode(BYTE *buf, DWORD len, DWORD shift, u32 key)
{
	u64 hash = (u64)key * 0x9E370001;
	u32 hi = (u32)(hash >> 32);
	u32 lo = (u32)hash;
	u32 tmp = lo;
	if (shift & 0x20) {
		lo = hi;
		hi = tmp;
	}

	tmp = lo;
	if (shift & 31) {
		lo = (lo << shift) | (hi >> (32 - shift));
		hi = (hi << shift) | (tmp >> (32 - shift));
	}

	for (DWORD i = 0; i < len; ++i) {
		buf[i] ^= lo;
		BYTE cf = !!(lo & 0x80000000);
		lo = (lo << 1) | (hi >> 31);
		hi = (hi << 1) | cf;
	}
}
#else
static void decode(BYTE *buf, DWORD len, DWORD shift, u32 key)
{
	u32 eax, edx, tmp;
	BYTE cf = 0;

	edx = (u32)(((u64)key * (u64)0x9E370001) >> 32);
	eax = key * 0x9E370001;
	tmp = eax;
	if (offset & 0x20) {	// 40eca3
		eax = edx;
		edx = tmp;
	}

	tmp = eax;
	offset &= 31;
	if (offset) {
		eax = (eax << offset) | (edx >> (32 - offset));
		edx = (edx << offset) | (tmp >> (32 - offset));
	}

	for (DWORD i = 0; i < len; i++) {
		buf[i] ^= eax;
		cf = !!(eax & 0x80000000);
		eax = (eax << 1) | (edx >> 31);
		edx = (edx << 1) | cf;
	}
}
#endif

static int decode_sysenv(struct package *pkg, arc_entry_t *arc_entry, u32 key, u32 *resource_key)
{
	BYTE *compr, *uncompr;
	DWORD comprlen;
	int ret;

	comprlen = arc_entry->length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, arc_entry->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}
	decode(compr, comprlen, arc_entry->offset, key);

	uncompr = (BYTE *)malloc(0x1d84);
	if (!uncompr) {
		free(compr);
		return -CUI_EMEM;
	}

	ret = azsystem_decompress(uncompr, 0x1d84, compr, comprlen);
	free(compr);
	if (ret) {		
		free(uncompr);
		return ret;
	}
	memcpy(resource_key, uncompr, 16);
	free(uncompr);

	return 0;
}

static int azsystem_find_key(const char *system_path, u32 *ret_key)
{
	arc_header_t arc_header;
	TCHAR path[MAX_PATH];
	HANDLE fd;
	INI_ARRAY;
	BYTE *compr_index;
	DWORD index_buffer_length, actlen;
	arc_entry_t *index_buffer;
	u32 key;

	if (acp2unicode(system_path, -1, path, SOT(path)))
		return -1;
	
	fd = MyOpenFile(path);
	if (fd == NULL)
		return -CUI_EOPEN;

	for (DWORD i = 0; i < MAX_INI; i++) {
		if (MySetFilePosition(fd, 0, 0, IO_SEEK_SET)) {
			MyCloseFile(fd);
			return -CUI_EREAD;
		}

		if (MyReadFile(fd, &arc_header, sizeof(arc_header))) {
			MyCloseFile(fd);
			return -CUI_EREAD;
		}

		key = gen_key(ini[i], sizeof(ini[i]));
		decode((BYTE *)&arc_header, sizeof(arc_header_t), 0, key);
		if (!strncmp(arc_header.magic, "ARC", 4))
			break;
	}
	if (i == MAX_INI) {
		MyCloseFile(fd);
		return -CUI_EMATCH;
	}

	compr_index = (BYTE *)malloc(arc_header.compr_index_length);
	if (!compr_index) {
		MyCloseFile(fd);
		return -CUI_EMEM;
	}
	
	if (MyReadFile(fd, compr_index, arc_header.compr_index_length)) {
		free(compr_index);
		MyCloseFile(fd);
		return -CUI_EREAD;
	}
	decode(compr_index, arc_header.compr_index_length, sizeof(arc_header_t), key);

	if (ewf_crc((BYTE *)(compr_index + 4), arc_header.compr_index_length - 4, 1) != *(u32 *)compr_index) {
		free(compr_index);
		MyCloseFile(fd);
		return -CUI_EMATCH;
	}

	index_buffer_length = arc_header.index_entries * sizeof(arc_entry_t);
	index_buffer = (arc_entry_t *)malloc(index_buffer_length);
	if (!index_buffer) {
		free(compr_index);
		MyCloseFile(fd);
		return -CUI_EMEM;
	}

	actlen = index_buffer_length;
	if (uncompress((BYTE *)index_buffer, &actlen, compr_index + 4, arc_header.compr_index_length - 4) != Z_OK) {
		free(index_buffer);
		free(compr_index);
		MyCloseFile(fd);
		return -CUI_EUNCOMPR;
	}
	free(compr_index);
	if (actlen != index_buffer_length) {
		free(index_buffer);
		MyCloseFile(fd);
		return -CUI_EUNCOMPR;
	}

	arc_entry_t *arc_entry = index_buffer;
	for (i = 0; i < arc_header.index_entries; i++) {
		if (!strcmp(arc_entry->name, "sysenv.tbl"))
			break;
		arc_entry++;
	}
	if (i == arc_header.index_entries) {
		free(index_buffer);
		MyCloseFile(fd);
		return -CUI_EMATCH;
	}

	u32 resource_key[4];
	u32 _ini[4];
	u32 seed;
	BYTE *compr, *uncompr;
	DWORD comprlen;
	int ret;
	int name_len = strlen(arc_entry->name);

	if (crc32(0, (BYTE *)arc_entry->name, name_len) != arc_entry->name_crc) {
		free(index_buffer);
		MyCloseFile(fd);
		return -CUI_EUNCOMPR;
	}

	arc_entry->offset += sizeof(arc_header_t) + arc_header.compr_index_length;
	if (MySetFilePosition(fd, arc_entry->offset, 0, IO_SEEK_SET)) {
		free(index_buffer);
		MyCloseFile(fd);
		return -CUI_ESEEK;
	}

	comprlen = arc_entry->length;
	compr = (BYTE *)malloc(arc_entry->length);
	if (!compr) {
		free(index_buffer);
		MyCloseFile(fd);
		return -CUI_ESEEK;
	}

	if (MyReadFile(fd, compr, comprlen)) {
		free(compr);
		free(index_buffer);
		MyCloseFile(fd);
		return -CUI_ESEEK;
	}
	decode(compr, comprlen, arc_entry->offset, key);

	uncompr = (BYTE *)malloc(0x1d84);
	if (!uncompr) {
		free(compr);
		free(index_buffer);
		MyCloseFile(fd);
		return -CUI_ESEEK;
	}

	ret = azsystem_decompress(uncompr, 0x1d84, compr, comprlen);
	free(compr);
	if (ret) {		
		free(uncompr);
		free(index_buffer);
		MyCloseFile(fd);
		return -CUI_ESEEK;
	}
	memcpy(resource_key, uncompr, 16);
	free(uncompr);

	seed = crc32(0, (BYTE *)resource_key, sizeof(resource_key));
	if (mt_mode == SFMT19937) {
		sfmt19937_init_gen_rand(seed);
		_ini[0] = sfmt19937_gen_rand32();
		_ini[1] = (sfmt19937_gen_rand32() & 0xffff);
		_ini[1] |= sfmt19937_gen_rand32() << 16;
		_ini[2] = sfmt19937_gen_rand32();
		_ini[3] = sfmt19937_gen_rand32();
		*ret_key = gen_key(_ini, sizeof(_ini));	// 覆盖magic
	} else if (mt_mode == SFMT132049) {
		sfmt132049_init_gen_rand(seed);
		_ini[0] = sfmt132049_gen_rand32();
		_ini[1] = (sfmt132049_gen_rand32() & 0xffff);
		_ini[1] |= sfmt132049_gen_rand32() << 16;
		_ini[2] = sfmt132049_gen_rand32();
		_ini[3] = sfmt132049_gen_rand32();
		*ret_key = gen_key(_ini, sizeof(_ini));	// 覆盖magic
	}
	MyCloseFile(fd);

	return 0;
}

static int arc1a_decompress_index(arc1a_compr_index_t *compr_index, BYTE *compr, DWORD comprlen, 
							 BYTE *uncompr, DWORD uncomprlen)
{
	BYTE mask;
	DWORD copy_bytes, act_uncomprlen;
	BYTE *compr1, *compr0;
	BYTE *bitmap;

	bitmap = compr;
	compr1 = bitmap + compr_index->bitmaplen;
	compr0 = compr1 + compr_index->comprlen1;
	act_uncomprlen = 0;
	mask = 0x80;

	while (act_uncomprlen < uncomprlen)  {
		if (*bitmap & mask) {
			DWORD offset = *(u16 *)compr1;
			compr1 += 2;
			copy_bytes = (offset >> 13) + 3;
			offset &= 0x1fff;
			offset++;
			for (DWORD i = 0; i < copy_bytes; i++) {
				uncompr[act_uncomprlen] = uncompr[act_uncomprlen - offset];
				act_uncomprlen++;
			}
		} else {
			copy_bytes = *compr0++;
			copy_bytes++;
			memcpy(&uncompr[act_uncomprlen], compr0, copy_bytes);
			compr0 += copy_bytes;
			act_uncomprlen += copy_bytes;
		}
		mask >>= 1;
		if (!mask) {
			bitmap++;
			mask = 0x80;
		}
	}

	return !(act_uncomprlen == uncomprlen);
}

static int map1a_decompress(map1a_header_t *compr_index, BYTE *compr, DWORD comprlen, 
							 BYTE *uncompr, DWORD uncomprlen)
{
	BYTE mask;
	DWORD copy_bytes, act_uncomprlen;
	BYTE *compr1, *compr0;
	BYTE *bitmap;

	bitmap = compr;
	compr0 = bitmap + compr_index->bitmaplen;
	compr1 = compr0 + compr_index->comprlen0;
	act_uncomprlen = 0;
	mask = 0x80;

	while (act_uncomprlen < uncomprlen)  {
		if (*bitmap & mask) {
			DWORD offset = *(u16 *)compr1;
			compr1 += 2;
			copy_bytes = (offset >> 13) + 3;
			offset &= 0x1fff;
			offset++;
			for (DWORD i = 0; i < copy_bytes; i++) {
				uncompr[act_uncomprlen] = uncompr[act_uncomprlen - offset];
				act_uncomprlen++;
			}
		} else
			uncompr[act_uncomprlen++] = *compr0++;

		mask >>= 1;
		if (!mask) {
			bitmap++;
			mask = 0x80;
		}
	}

	return !(act_uncomprlen == uncomprlen);
}

/********************* arc *********************/

/* 封包匹配回调函数 */
static int AZSystem_arc_match(struct package *pkg)
{
	arc_header_t *arc_header;
	INI_ARRAY;
	u32 *key;
	const char *mt_mode_string;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	key = (u32 *)malloc(sizeof(arc_header_t) + 4);
	if (!key) {
		pkg->pio->close(pkg);
		return -CUI_EMEM;
	}
	arc_header = (arc_header_t *)(key + 1);

	for (DWORD i = 0; i < MAX_INI; i++) {
		if (pkg->pio->readvec(pkg, arc_header, sizeof(arc_header_t), 0, IO_SEEK_SET)) {
			free(key);
			pkg->pio->close(pkg);
			return -CUI_EREADVEC;
		}

		*key = gen_key(ini[i], sizeof(ini[i]));
		decode((BYTE *)arc_header, sizeof(arc_header_t), 0, *key);
		if (!strncmp(arc_header->magic, "ARC", 4))
			break;
	}
	if (i == MAX_INI) {
		free(key);
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}
	package_set_private(pkg, key);

	mt_mode_string = get_options("mt");
	if (!lstrcmpiA(mt_mode_string, "SFMT19937"))
		mt_mode = SFMT19937;	
	else if (!lstrcmpiA(mt_mode_string, "SFMT132049"))
		mt_mode = SFMT132049;
	else
		mt_mode = SFMT19937;	

	return 0;	
}

/* 封包索引目录提取函数 */
static int AZSystem_arc_extract_directory(struct package *pkg,
										  struct package_directory *pkg_dir)
{
	arc_header_t *arc_header;
	BYTE *compr_index;
	DWORD index_buffer_length, actlen;
	arc_entry_t *index_buffer;
	u32 *priv, key;
	int found_key = 0;

	priv = (u32 *)package_get_private(pkg);
	key = priv[0];
	arc_header = (arc_header_t *)&priv[1];

	compr_index = (BYTE *)malloc(arc_header->compr_index_length);
	if (!compr_index)
		return -CUI_EMEM;
	
	/* 读操作和decode是一体的 */
	if (pkg->pio->read(pkg, compr_index, arc_header->compr_index_length)) {
		free(compr_index);
		return -CUI_EREAD;
	}
	decode(compr_index, arc_header->compr_index_length, sizeof(arc_header_t), key);

	/* ewf crc和zlib解压缩操作是一体的 */
	u32 crc = *(u32 *)compr_index;
	if (crc == ewf_crc((BYTE *)(compr_index + 4), arc_header->compr_index_length - 4, 1))
		old_arc = 0;
	else if (crc == crc32(0, (BYTE *)(compr_index + 4), arc_header->compr_index_length - 4))
		old_arc = 1;
	else {
		free(compr_index);
		return -CUI_EMATCH;
	}

	index_buffer_length = arc_header->index_entries * sizeof(arc_entry_t);
	index_buffer = (arc_entry_t *)malloc(index_buffer_length);
	if (!index_buffer) {
		free(compr_index);
		return -CUI_EMEM;
	}

	actlen = index_buffer_length;
	if (uncompress((BYTE *)index_buffer, &actlen, compr_index + 4, arc_header->compr_index_length - 4) != Z_OK) {
		free(index_buffer);
		free(compr_index);
		return -CUI_EUNCOMPR;
	}
	free(compr_index);
	if (actlen != index_buffer_length) {
		free(index_buffer);
		return -CUI_EUNCOMPR;
	}

	for (unsigned int i = 0; i < arc_header->index_entries; i++) {
		arc_entry_t *arc_entry = &index_buffer[i];
		int name_len;

		if (!arc_entry->name[31])
			name_len = strlen(arc_entry->name);
		else
			name_len = 32;
		if (crc32(0, (BYTE *)arc_entry->name, name_len) != arc_entry->name_crc)
			break;
		arc_entry->offset += sizeof(arc_header_t) + arc_header->compr_index_length;

		if (!strcmp(arc_entry->name, "sysenv.tbl")) {
			u32 resource_key[4];
			u32 ini[4];
			u32 seed;

			if (decode_sysenv(pkg, arc_entry, key, resource_key))
				break;

			seed = crc32(0, (BYTE *)resource_key, sizeof(resource_key));
			if (mt_mode == SFMT19937) {
				sfmt19937_init_gen_rand(seed);
				ini[0] = sfmt19937_gen_rand32();
				ini[1] = (sfmt19937_gen_rand32() & 0xffff);
				ini[1] |= sfmt19937_gen_rand32() << 16;
				ini[2] = sfmt19937_gen_rand32();
				ini[3] = sfmt19937_gen_rand32();
				priv[1] = gen_key(ini, sizeof(ini));	// 覆盖magic
			} else if (mt_mode == SFMT132049) {
				sfmt132049_init_gen_rand(seed);
				ini[0] = sfmt132049_gen_rand32();
				ini[1] = (sfmt132049_gen_rand32() & 0xffff);
				ini[1] |= sfmt132049_gen_rand32() << 16;
				ini[2] = sfmt132049_gen_rand32();
				ini[3] = sfmt132049_gen_rand32();
				priv[1] = gen_key(ini, sizeof(ini));	// 覆盖magic
			}
			found_key = 1;
		}
	}
	if (i != arc_header->index_entries) {
		free(index_buffer);
		return -CUI_EMATCH;
	}

	if (!found_key) {
		const char *system = get_options("system");		
#if 1
		if (!old_arc) {
			if (!system) {
				locale_app_printf(AZSystem_locale_id, 0, pkg->name);
				free(index_buffer);
				return -CUI_EMATCH;
			}
			if (azsystem_find_key(system, &key)) {
				locale_app_printf(AZSystem_locale_id, 1, pkg->name);
				free(index_buffer);
				return -CUI_EMATCH;
			}
		}
		// 旧版本的key使用的就是前面的key
		priv[1] = key;
#else
		if (!system) {
			crass_printf(_T("[WARNING] %s: the key is saved in system.arc, ")
				_T("so you must specify its path with \"system\" parameter. \nfor expample: ")
				_T("crage -p xxx.arc -O system=d:\\system.arc\n"), pkg->name);
			free(index_buffer);
			return -CUI_EMATCH;
		}
		if (azsystem_find_key(system, &key)) {
			crass_printf(_T("[ERROR] not found any key ...\n"));
			free(index_buffer);
			return -CUI_EMATCH;
		}
		priv[1] = key;
#endif
	}

	pkg_dir->index_entries = arc_header->index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(arc_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int AZSystem_arc_parse_resource_info(struct package *pkg,
											struct package_resource *pkg_res)
{
	arc_entry_t *arc_entry;

	arc_entry = (arc_entry_t *)pkg_res->actual_index_entry;
	strncpy(pkg_res->name, arc_entry->name, 32);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = arc_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = arc_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int AZSystem_arc_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;
	u32 *priv, key;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen,	pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	priv = (u32 *)package_get_private(pkg);

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		if (strcmp(pkg_res->name, "sysenv.tbl"))
			key = priv[1];
		else
			key = priv[0];
		decode(compr, comprlen, pkg_res->offset, key);
		uncompr = NULL;
		uncomprlen = 0;
		goto out;
	}

	if (!strcmp(pkg_res->name, "sysenv.tbl")) {
		int ret;

		uncomprlen = 0x1d84;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}

		key = priv[0];
		decode(compr, comprlen, pkg_res->offset, key);
		ret = azsystem_decompress(uncompr, uncomprlen, compr, comprlen);
		if (ret) {
			free(uncompr);
			free(compr);
			return ret;
		}
	} else {
		key = priv[1];
		decode(compr, comprlen, pkg_res->offset, key);

		if (!strncmp((char *)compr, "TBL", 4)) {
			tbl_header_t *tbl_header;
			BYTE *tbl_compr;
			DWORD tbl_comprlen;
			u32 crc;

			if (!old_arc) {
				tbl_header = (tbl_header_t *)compr;
				tbl_compr = (BYTE *)(tbl_header + 1);
				crc = *(u32 *)tbl_compr;
				tbl_compr += 4;
				tbl_comprlen = tbl_header->comprlen - 4;
				if (ewf_crc(tbl_compr, tbl_comprlen, 1) != crc) {
					free(compr);
					return -CUI_EMATCH;
				}
			} else {
				tbl1a_header_t *old_tbl_header = (tbl1a_header_t *)compr;
				tbl_compr = (BYTE *)(old_tbl_header + 1);
				crc = *(u32 *)tbl_compr;
				tbl_compr += 4;
				tbl_comprlen = old_tbl_header->comprlen - 4;				
				if (crc32(0, tbl_compr, tbl_comprlen) != crc) {
					free(compr);
					return -CUI_EMATCH;
				}
				tbl_header = (tbl_header_t *)old_tbl_header;
			}

			uncomprlen = tbl_header->uncomprlen;
			uncompr = (BYTE *)malloc(uncomprlen);
			if (!uncompr) {
				free(compr);
				return -CUI_EMEM;
			}

			if (uncompress(uncompr, &uncomprlen, tbl_compr, tbl_comprlen) != Z_OK) {
				free(uncompr);
				free(compr);
				return -CUI_EUNCOMPR;
			}
			if (uncomprlen != tbl_header->uncomprlen) {
				free(uncompr);
				free(compr);
				return -CUI_EUNCOMPR;
			}
		} else if (!strncmp((char *)compr, "CPB", 4)) {
			cpb_old2_header_t *cpb_old2_header = (cpb_old2_header_t *)compr;
			if (!cpb_old2_header->unknown && !cpb_old2_header->unknown1
					&& cpb_old2_header->version == 1)
				old_arc = 2;

			BYTE *cpb_compr;
			BYTE *blk_uncompr;
			DWORD blk_uncomprlen;
			BYTE *act_uncompr;
			BYTE *palette;
			u32 *comprlen_block;
			int pixel_bytes;
			int ret = 0;
			int width, height;

			if (old_arc == 1) {
				cpb_old_header_t *cpb_old_header = (cpb_old_header_t *)compr;
				cpb_compr = (BYTE *)(cpb_old_header + 1);
				pixel_bytes = cpb_old_header->color_depth / 8;
				blk_uncomprlen = cpb_old_header->width * cpb_old_header->height;
				comprlen_block = cpb_old_header->comprlen;
				width = cpb_old_header->width;
				height = cpb_old_header->height;
			} else if (old_arc == 2) {
				cpb_old2_header_t *cpb_old2_header = (cpb_old2_header_t *)compr;
				cpb_compr = (BYTE *)(cpb_old2_header + 1);
				pixel_bytes = cpb_old2_header->color_depth / 8;
				blk_uncomprlen = cpb_old2_header->width * cpb_old2_header->height;
				comprlen_block = cpb_old2_header->comprlen;
				width = cpb_old2_header->width;
				height = cpb_old2_header->height;
			}

			uncomprlen = blk_uncomprlen * pixel_bytes;
			uncompr = (BYTE *)malloc(uncomprlen);
			if (!uncompr) {
				free(compr);
				return -CUI_EMEM;
			}
			blk_uncompr = uncompr;
#if 0
			if (cpb_old_header->unknown || cpb_old_header->version != 1) {
				printf("%s %x %x\n", 
					pkg_res->name, cpb_old_header->unknown,cpb_old_header->version);
				palette = cpb_compr;
				cpb_compr += 0x400;
				ret = azsystem_arc1a_decompress(blk_uncompr, blk_uncomprlen, cpb_compr, cpb_old_header->max_comprlen);
			}
#endif
			palette = NULL;
			int tbl[4] = { 2, 0, 3, 1 };
			if (old_arc == 2) {
				tbl[0] = 2;
				tbl[1] = 0;
				tbl[2] = 3;
				tbl[3] = 1;
			} else if (old_arc == 1) {
				tbl[0] = 2;
				tbl[1] = 1;
				tbl[2] = 0;
				tbl[3] = 3;
			}

			int act_pixel_bytes = pixel_bytes;
			/* cpb的压缩数据按A,G,B,R的顺序排放；而cpb_header中的压缩长度字段按B,G,A,R顺序存储 */
			for (int i = 0; i < pixel_bytes; i++) {
				if (!comprlen_block[tbl[i]]) {
					pixel_bytes++;
					continue;
				}
				ret = azsystem_arc1a_decompress(blk_uncompr, blk_uncomprlen, cpb_compr, comprlen_block[tbl[i]]);
				if (ret)
					break;				
				cpb_compr += comprlen_block[tbl[i]];
				blk_uncompr += blk_uncomprlen;
			}			
			if (ret) {
				free(uncompr);
				free(compr);
				return ret;
			}
			
			pixel_bytes = act_pixel_bytes;
			act_uncompr = (BYTE *)malloc(uncomprlen);
			if (!act_uncompr) {
				free(uncompr);
				free(compr);
				return -CUI_EMEM;
			}

			BYTE *r = uncompr;
			if (pixel_bytes == 4) {
				BYTE tbl[4] = { 3, 0, 1, 2 };
				for (int p = 0; p < pixel_bytes; p++) {
					BYTE *pixel = act_uncompr + (height - 1) * width * pixel_bytes + tbl[p];
					for (int h = 0; h < height; h++) {
						for (int w = 0; w < width; w++) {
							*pixel = *r++;
							pixel += pixel_bytes;
						}
						pixel -= width * pixel_bytes * 2;
					}				
				}
			} else {
				for (int p = 0; p < pixel_bytes; p++) {
					BYTE *pixel = act_uncompr + (height - 1) * width * pixel_bytes + p;
					for (int h = 0; h < height; h++) {
						for (int w = 0; w < width; w++) {
							*pixel = *r++;
							pixel += pixel_bytes;
						}
						pixel -= width * pixel_bytes * 2;
					}				
				}
			}
			free(uncompr);

			alpha_blending(act_uncompr, width, height, pixel_bytes * 8);

			if (MyBuildBMPFile(act_uncompr, uncomprlen, palette, 0x400, width, height,
					pixel_bytes * 8, (BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, my_malloc)) {
				free(act_uncompr);
				free(compr);
				return -CUI_EMEM;
			}
			free(act_uncompr);
			free(compr);
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
			pkg_res->replace_extension = _T(".bmp");

			return 0;
		} else if (!strncmp((char *)compr, "TYP1", 4)) {
			cpb_header_t *cpb_header = (cpb_header_t *)compr;
			BYTE *cpb_compr = (BYTE *)(cpb_header + 1);
			BYTE *blk_uncompr;
			DWORD blk_uncomprlen;
			BYTE *act_uncompr;
			BYTE *palette;
			int pixel_bytes;
			int ret = 0;

			pixel_bytes = cpb_header->color_depth / 8;
			blk_uncomprlen = cpb_header->width * cpb_header->height;
			uncomprlen = blk_uncomprlen * pixel_bytes;
			uncompr = (BYTE *)malloc(uncomprlen);
			if (!uncompr) {
				free(compr);
				return -CUI_EMEM;
			}
			blk_uncompr = uncompr;

			if (cpb_header->flag) {
				palette = cpb_compr;
				cpb_compr += 0x400;
				ret = azsystem_decompress(blk_uncompr, blk_uncomprlen, cpb_compr, cpb_header->max_comprlen);
			} else {
				palette = NULL;
				/* cpb的压缩数据按A,B,G,R的顺序排放；而cpb_header中的压缩长度字段按B,R,G,A顺序存储 */
				for (int i = pixel_bytes - 1; i >= 0; i--) {
					ret = azsystem_decompress(blk_uncompr, blk_uncomprlen, cpb_compr, cpb_header->comprlen[i]);
					if (ret)
						break;
					cpb_compr += cpb_header->comprlen[i];
					blk_uncompr += blk_uncomprlen;
				}
			}
			if (ret) {
				free(uncompr);
				free(compr);
				return ret;
			}

			act_uncompr = (BYTE *)malloc(uncomprlen);
			if (!act_uncompr) {
				free(uncompr);
				free(compr);
				return -CUI_EMEM;
			}

			BYTE *r = uncompr;
			if (cpb_header->color_depth == 32) {
				BYTE tbl[4] = { 3, 0, 1, 2 };
				for (int p = 0; p < pixel_bytes; p++) {
					BYTE *pixel = act_uncompr + (cpb_header->height - 1) * cpb_header->width * pixel_bytes + tbl[p];
					for (int h = 0; h < cpb_header->height; h++) {
						for (int w = 0; w < cpb_header->width; w++) {
							*pixel = *r++;
							pixel += pixel_bytes;
						}
						pixel -= cpb_header->width * pixel_bytes * 2;
					}				
				}
			} else {
				for (int p = 0; p < pixel_bytes; p++) {
					BYTE *pixel = act_uncompr + (cpb_header->height - 1) * cpb_header->width * pixel_bytes + p;
					for (int h = 0; h < cpb_header->height; h++) {
						for (int w = 0; w < cpb_header->width; w++) {
							*pixel = *r++;
							pixel += pixel_bytes;
						}
						pixel -= cpb_header->width * pixel_bytes * 2;
					}				
				}
			}
			free(uncompr);

			alpha_blending(act_uncompr, cpb_header->width, cpb_header->height, 
				cpb_header->color_depth);

			if (MyBuildBMPFile(act_uncompr, uncomprlen, palette, 0x400, cpb_header->width, cpb_header->height,
					cpb_header->color_depth, (BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, my_malloc)) {
				free(act_uncompr);
				free(compr);
				return -CUI_EMEM;
			}
			free(act_uncompr);
			free(compr);
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
			pkg_res->replace_extension = _T(".bmp");

			return 0;
		} else if (!strncmp((char *)compr, "ASB", 4)) {
			asb_header_t *asb_header = (asb_header_t *)compr;
			int ret;

			uncomprlen = asb_header->uncomprlen;
			uncompr = (BYTE *)malloc(uncomprlen);
			if (!uncompr) {
				free(compr);
				return -CUI_EMEM;
			}

			if (!old_arc)
				ret = asb_decompress(uncompr, uncomprlen, (BYTE *)(asb_header + 1), asb_header->comprlen);
			else
				ret = asb1a_process(compr, comprlen, uncompr, uncomprlen);
			if (ret) {
				free(uncompr);
				free(compr);
				return ret;
			}
		} else {
			uncompr = NULL;
			uncomprlen = 0;
		}
	}
out:
	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;
	
	return 0;
}

/* 资源保存函数 */
static int AZSystem_arc_save_resource(struct resource *res, 
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
static void AZSystem_arc_release_resource(struct package *pkg, 
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
static void AZSystem_arc_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	void *priv;

	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	priv = (void *)package_get_private(pkg);
	if (priv) {
		free(priv);
		package_set_private(pkg, NULL);
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation AZSystem_arc_operation = {
	AZSystem_arc_match,					/* match */
	AZSystem_arc_extract_directory,		/* extract_directory */
	AZSystem_arc_parse_resource_info,	/* parse_resource_info */
	AZSystem_arc_extract_resource,		/* extract_resource */
	AZSystem_arc_save_resource,			/* save_resource */
	AZSystem_arc_release_resource,		/* release_resource */
	AZSystem_arc_release				/* release */
};

/********************* new arc *********************/

static void gen_code_buffer(int seed, DWORD *code_buffer)
{
	sfmt19937_init_gen_rand(seed);
	for (DWORD i = 0; i < 256; ++i)
		code_buffer[i] = sfmt19937_gen_rand32();
}

static void decode2(BYTE *buf, DWORD len, DWORD total_size, DWORD offset)
{
	static DWORD code_buffer[256];

	gen_code_buffer(total_size, code_buffer);

	for (DWORD i = 0; i < len; ++i) {
		buf[i] ^= _rotl(code_buffer[offset & 0xff], (offset >> 8) & 0xff);
		++offset;
	}
}

/* 封包匹配回调函数 */
static int AZSystem_new_arc_match(struct package *pkg)
{
	arc_header_t arc_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &arc_header, sizeof(arc_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}
	
	u32 fsize;
	pkg->pio->length_of(pkg, &fsize);

	decode2((BYTE *)&arc_header, sizeof(arc_header), fsize, 0);

	if (strncmp(arc_header.magic, "ARC", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}


	return 0;	
}

/* 封包索引目录提取函数 */
static int AZSystem_new_arc_extract_directory(struct package *pkg,
											  struct package_directory *pkg_dir)
{
	arc_header_t arc_header;

	if (pkg->pio->readvec(pkg, &arc_header, sizeof(arc_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;
	
	u32 fsize;
	pkg->pio->length_of(pkg, &fsize);

	decode2((BYTE *)&arc_header, sizeof(arc_header), fsize, 0);

	BYTE *compr_index = (BYTE *)malloc(arc_header.compr_index_length);
	if (!compr_index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, compr_index, arc_header.compr_index_length)) {
		free(compr_index);
		return -CUI_EREAD;
	}
	decode2(compr_index, arc_header.compr_index_length, fsize, sizeof(arc_header));

	/* ewf crc和zlib解压缩操作是一体的 */
	u32 crc = *(u32 *)compr_index;
	if (crc == ewf_crc((BYTE *)(compr_index + 4), arc_header.compr_index_length - 4, 1))
		old_arc = 0;
	else {
		free(compr_index);
		return -CUI_EMATCH;
	}

	DWORD index_buffer_length = arc_header.index_entries * sizeof(arc_entry_t);
	arc_entry_t *index_buffer = (arc_entry_t *)malloc(index_buffer_length);
	if (!index_buffer) {
		free(compr_index);
		return -CUI_EMEM;
	}

	DWORD actlen = index_buffer_length;
	if (uncompress((BYTE *)index_buffer, &actlen, compr_index + 4, arc_header.compr_index_length - 4) != Z_OK) {
		free(index_buffer);
		free(compr_index);
		return -CUI_EUNCOMPR;
	}
	free(compr_index);
	if (actlen != index_buffer_length) {
		free(index_buffer);
		return -CUI_EUNCOMPR;
	}

	for (unsigned int i = 0; i < arc_header.index_entries; ++i) {
		arc_entry_t *arc_entry = &index_buffer[i];
		int name_len;

		if (!arc_entry->name[31])
			name_len = strlen(arc_entry->name);
		else
			name_len = 32;
		if (crc32(0, (BYTE *)arc_entry->name, name_len) != arc_entry->name_crc)
			break;
		arc_entry->offset += sizeof(arc_header_t) + arc_header.compr_index_length;
	}
	if (i != arc_header.index_entries) {
		free(index_buffer);
		return -CUI_EMATCH;
	}

	pkg_dir->index_entries = arc_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(arc_entry_t);

	return 0;
}

/* 封包资源提取函数 */
static int AZSystem_new_arc_extract_resource(struct package *pkg,
											 struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;
	u32 *priv, key;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen,	pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		decode2(compr, comprlen, comprlen, 0);
		uncompr = NULL;
		uncomprlen = 0;
		goto out;
	}

	if (!strcmp(pkg_res->name, "sysenv.tbl")) {
		int ret;

		uncomprlen = 0x1d84;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}
		decode2(compr, comprlen, comprlen, 0);
		if (uncompress(uncompr, &uncomprlen, compr + 4, comprlen - 4) != Z_OK) {
			free(uncompr);
			free(compr);
			return ret;
		}
	} else {
		decode2(compr, comprlen, comprlen, 0);

		if (!strncmp((char *)compr, "TBL", 4)) {
			tbl_header_t *tbl_header;
			BYTE *tbl_compr;
			DWORD tbl_comprlen;
			u32 crc;

			if (!old_arc) {
				tbl_header = (tbl_header_t *)compr;
				tbl_compr = (BYTE *)(tbl_header + 1);
				crc = *(u32 *)tbl_compr;
				tbl_compr += 4;
				tbl_comprlen = tbl_header->comprlen - 4;
				if (ewf_crc(tbl_compr, tbl_comprlen, 1) != crc) {
					free(compr);
					return -CUI_EMATCH;
				}
			}

			uncomprlen = tbl_header->uncomprlen;
			uncompr = (BYTE *)malloc(uncomprlen);
			if (!uncompr) {
				free(compr);
				return -CUI_EMEM;
			}

			if (uncompress(uncompr, &uncomprlen, tbl_compr, tbl_comprlen) != Z_OK) {
				free(uncompr);
				free(compr);
				return -CUI_EUNCOMPR;
			}
			if (uncomprlen != tbl_header->uncomprlen) {
				free(uncompr);
				free(compr);
				return -CUI_EUNCOMPR;
			}
		} else if (!strncmp((char *)compr, "TYP1", 4)) {
			cpb_header_t *cpb_header = (cpb_header_t *)compr;
			BYTE *cpb_compr = (BYTE *)(cpb_header + 1);
			BYTE *blk_uncompr;
			DWORD blk_uncomprlen;
			BYTE *act_uncompr;
			BYTE *palette;
			int pixel_bytes;
			int ret = 0;

			pixel_bytes = cpb_header->color_depth / 8;
			blk_uncomprlen = cpb_header->width * cpb_header->height;
			uncomprlen = blk_uncomprlen * pixel_bytes;
			uncompr = (BYTE *)malloc(uncomprlen);
			if (!uncompr) {
				free(compr);
				return -CUI_EMEM;
			}
			blk_uncompr = uncompr;

			if (cpb_header->flag) {
				palette = cpb_compr;
				cpb_compr += 0x400;
				ret = azsystem_decompress(blk_uncompr, blk_uncomprlen, cpb_compr, cpb_header->max_comprlen);
			} else {
				palette = NULL;
				/* cpb的压缩数据按A,B,G,R的顺序排放；而cpb_header中的压缩长度字段按B,R,G,A顺序存储 */
				for (int i = pixel_bytes - 1; i >= 0; i--) {
					ret = azsystem_decompress(blk_uncompr, blk_uncomprlen, cpb_compr, cpb_header->comprlen[i]);
					if (ret)
						break;
					cpb_compr += cpb_header->comprlen[i];
					blk_uncompr += blk_uncomprlen;
				}
			}
			if (ret) {
				free(uncompr);
				free(compr);
				return ret;
			}

			act_uncompr = (BYTE *)malloc(uncomprlen);
			if (!act_uncompr) {
				free(uncompr);
				free(compr);
				return -CUI_EMEM;
			}

			BYTE *r = uncompr;
			if (cpb_header->color_depth == 32) {
				BYTE tbl[4] = { 3, 0, 1, 2 };
				for (int p = 0; p < pixel_bytes; p++) {
					BYTE *pixel = act_uncompr + (cpb_header->height - 1) * cpb_header->width * pixel_bytes + tbl[p];
					for (int h = 0; h < cpb_header->height; h++) {
						for (int w = 0; w < cpb_header->width; w++) {
							*pixel = *r++;
							pixel += pixel_bytes;
						}
						pixel -= cpb_header->width * pixel_bytes * 2;
					}				
				}
			} else {
				for (int p = 0; p < pixel_bytes; p++) {
					BYTE *pixel = act_uncompr + (cpb_header->height - 1) * cpb_header->width * pixel_bytes + p;
					for (int h = 0; h < cpb_header->height; h++) {
						for (int w = 0; w < cpb_header->width; w++) {
							*pixel = *r++;
							pixel += pixel_bytes;
						}
						pixel -= cpb_header->width * pixel_bytes * 2;
					}				
				}
			}
			free(uncompr);

			alpha_blending(act_uncompr, cpb_header->width, cpb_header->height, 
				cpb_header->color_depth);

			if (MyBuildBMPFile(act_uncompr, uncomprlen, palette, 0x400, cpb_header->width, cpb_header->height,
					cpb_header->color_depth, (BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, my_malloc)) {
				free(act_uncompr);
				free(compr);
				return -CUI_EMEM;
			}
			free(act_uncompr);
			free(compr);
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
			pkg_res->replace_extension = _T(".bmp");

			return 0;
		} else {
			uncompr = NULL;
			uncomprlen = 0;
		}
	}
out:
	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;
	
	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation AZSystem_new_arc_operation = {
	AZSystem_new_arc_match,				/* match */
	AZSystem_new_arc_extract_directory,	/* extract_directory */
	AZSystem_arc_parse_resource_info,	/* parse_resource_info */
	AZSystem_new_arc_extract_resource,	/* extract_resource */
	AZSystem_arc_save_resource,			/* save_resource */
	AZSystem_arc_release_resource,		/* release_resource */
	AZSystem_arc_release				/* release */
};

/********************* arc *********************/

/* 封包匹配回调函数 */
static int AZSystem_arc1a_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "ARC\x1a", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int AZSystem_arc1a_extract_directory(struct package *pkg,
											struct package_directory *pkg_dir)
{
	arc1a_header_t arc1a_header;
	arc1a_compr_index_t arc1a_compr_index;
	arc1a_entry_t *uncompr_index;
	BYTE *compr_index;
	DWORD compr_index_len, uncompr_index_len;

	if (pkg->pio->readvec(pkg, &arc1a_header, sizeof(arc1a_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	if (pkg->pio->read(pkg, &arc1a_compr_index, sizeof(arc1a_compr_index_t)))
		return -CUI_EREAD;

	compr_index_len = arc1a_header.compr_index_length - sizeof(arc1a_compr_index);
	compr_index = (BYTE *)malloc(compr_index_len);
	if (!compr_index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, compr_index, compr_index_len)) {
		free(compr_index);
		return -CUI_EREAD;
	}

	if (crc32(0, compr_index, compr_index_len) != arc1a_compr_index.crc) {
		free(compr_index);
		return -CUI_EMATCH;
	}

	uncompr_index_len = arc1a_header.index_entries * sizeof(arc1a_entry_t);
	uncompr_index = (arc1a_entry_t *)malloc(uncompr_index_len);
	if (!uncompr_index) {
		free(compr_index);
		return -CUI_EMEM;
	}

	if (arc1a_decompress_index(&arc1a_compr_index, compr_index, 
			compr_index_len, (BYTE *)uncompr_index, uncompr_index_len)) {
		free(uncompr_index);
		free(compr_index);
		return -CUI_EUNCOMPR;
	}
	free(compr_index);

	for (DWORD i = 0; i < arc1a_header.index_entries; i++)
		uncompr_index[i].offset += sizeof(arc1a_header) + arc1a_header.compr_index_length;

	pkg_dir->index_entries = arc1a_header.index_entries;
	pkg_dir->directory = uncompr_index;
	pkg_dir->directory_length = uncompr_index_len;
	pkg_dir->index_entry_length = sizeof(arc1a_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int AZSystem_arc1a_parse_resource_info(struct package *pkg,
											  struct package_resource *pkg_res)
{
	arc1a_entry_t *arc1a_entry;

	arc1a_entry = (arc1a_entry_t *)pkg_res->actual_index_entry;
	strncpy(pkg_res->name, arc1a_entry->name, 48);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = arc1a_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = arc1a_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int AZSystem_arc1a_extract_resource(struct package *pkg,
										   struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen,	pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		uncompr = NULL;
		uncomprlen = 0;
		goto out;
	}

	if (!strncmp((char *)compr, "TBL\x1a", 4)) {
		tbl1a_header_t *tbl1a_header = (tbl1a_header_t *)compr;
		BYTE *tbl_compr;
		int ret;

		tbl_compr = (BYTE *)(tbl1a_header + 1);
		uncomprlen = tbl1a_header->uncomprlen;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}

		ret = azsystem_arc1a_decompress(uncompr, uncomprlen, tbl_compr, tbl1a_header->comprlen);
		if (ret) {
			free(uncompr);
			free(compr);
			return ret;
		}
	} else if (!strncmp((char *)compr, "CPB\x1a", 4)) {
		cpb1a_header_t *cpb1a_header = (cpb1a_header_t *)compr;
		BYTE *cpb_compr = (BYTE *)(cpb1a_header + 1);
		BYTE *blk_uncompr;
		DWORD blk_uncomprlen;
		BYTE *act_uncompr;
		BYTE *palette;
		int pixel_bytes;
		int ret = 0;

		pixel_bytes = cpb1a_header->color_depth / 8;
		blk_uncomprlen = cpb1a_header->width * cpb1a_header->height;
		uncomprlen = blk_uncomprlen * pixel_bytes;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}
		blk_uncompr = uncompr;

		if (cpb1a_header->unknown || cpb1a_header->unknown1 || cpb1a_header->version != 1) {
			printf("%s %x %x %x\n", 
				pkg_res->name, cpb1a_header->unknown,cpb1a_header->unknown1,cpb1a_header->version);
			palette = cpb_compr;
			cpb_compr += 0x400;
			ret = azsystem_arc1a_decompress(blk_uncompr, blk_uncomprlen, cpb_compr, cpb1a_header->max_comprlen);
		} else {
			palette = NULL;
			int tbl[4] = { 2, 0, 1, 3 };
			int act_pixel_bytes = pixel_bytes;
			/* cpb的压缩数据按A,B,G,R的顺序排放；而cpb_header中的压缩长度字段按B,G,A,R顺序存储 */
			for (int i = 0; i < pixel_bytes; i++) {
				if (!cpb1a_header->comprlen[tbl[i]]) {
					pixel_bytes++;
					continue;
				}
				ret = azsystem_arc1a_decompress(blk_uncompr, blk_uncomprlen, cpb_compr, cpb1a_header->comprlen[tbl[i]]);
				if (ret)
					break;				
				cpb_compr += cpb1a_header->comprlen[tbl[i]];
				blk_uncompr += blk_uncomprlen;
			}
			pixel_bytes = act_pixel_bytes;
		}
		if (ret) {
			free(uncompr);
			free(compr);
			return ret;
		}

		act_uncompr = (BYTE *)malloc(uncomprlen);
		if (!act_uncompr) {
			free(uncompr);
			free(compr);
			return -CUI_EMEM;
		}

		BYTE *r = uncompr;
		if (cpb1a_header->color_depth == 32) {
			BYTE tbl[4] = { 3, 0, 1, 2 };
			for (int p = 0; p < pixel_bytes; p++) {
				BYTE *pixel = act_uncompr + (cpb1a_header->height - 1) * cpb1a_header->width * pixel_bytes + tbl[p];
				for (int h = 0; h < cpb1a_header->height; h++) {
					for (int w = 0; w < cpb1a_header->width; w++) {
						*pixel = *r++;
						pixel += pixel_bytes;
					}
					pixel -= cpb1a_header->width * pixel_bytes * 2;
				}				
			}
		} else {
			for (int p = 0; p < pixel_bytes; p++) {
				BYTE *pixel = act_uncompr + (cpb1a_header->height - 1) * cpb1a_header->width * pixel_bytes + p;
				for (int h = 0; h < cpb1a_header->height; h++) {
					for (int w = 0; w < cpb1a_header->width; w++) {
						*pixel = *r++;
						pixel += pixel_bytes;
					}
					pixel -= cpb1a_header->width * pixel_bytes * 2;
				}				
			}
		}
		free(uncompr);

		alpha_blending(act_uncompr, cpb1a_header->width, cpb1a_header->height, 
			cpb1a_header->color_depth);

		if (MyBuildBMPFile(act_uncompr, uncomprlen, palette, 0x400, cpb1a_header->width, cpb1a_header->height,
				cpb1a_header->color_depth, (BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, my_malloc)) {
			free(act_uncompr);
			free(compr);
			return -CUI_EMEM;
		}
		free(act_uncompr);
		free(compr);
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp");

		return 0;
	} else if (!strncmp((char *)compr, "ASB\x1a", 4)) {
		asb1a_header_t *asb1a_header = (asb1a_header_t *)compr;

		uncomprlen = asb1a_header->uncomprlen;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}
		int ret = asb1a_process(compr, comprlen, uncompr, uncomprlen);
		if (ret) {
			free(uncompr);
			if (ret == -CUI_EMATCH) {
				uncompr = NULL;
				uncomprlen = 0;
				goto out;
			}
			free(compr);
			return ret;
		}
	} 
#if 0
	else if (!strncmp((char *)compr, "MAP\x1a", 4)) {
		map1a_header_t *map1a_header = (map1a_header_t *)compr;
		BYTE *map_compr;
		DWORD map_comprlen;
		int ret;

		map_compr = (BYTE *)(map1a_header + 1);
		uncomprlen = map1a_header->width * map1a_header->height * 4;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}

		map_comprlen = pkg_res->raw_data_length - sizeof(map1a_header_t);
		ret = map1a_decompress(map1a_header, map_compr, map_comprlen, uncompr, uncomprlen);
		if (ret) {
			free(uncompr);
			free(compr);
			return ret;
		}
	} 
#endif
	else {
		uncompr = NULL;
		uncomprlen = 0;
	}
out:
	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;
	
	return 0;
}

/* 资源保存函数 */
static int AZSystem_arc1a_save_resource(struct resource *res, 
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
static void AZSystem_arc1a_release_resource(struct package *pkg, 
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
static void AZSystem_arc1a_release(struct package *pkg, 
								   struct package_directory *pkg_dir)
{
	void *priv;

	if (!pkg || !pkg_dir)
		return;

	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	priv = (void *)package_get_private(pkg);
	if (priv) {
		free(priv);
		package_set_private(pkg, NULL);
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation AZSystem_arc1a_operation = {
	AZSystem_arc1a_match,				/* match */
	AZSystem_arc1a_extract_directory,	/* extract_directory */
	AZSystem_arc1a_parse_resource_info,	/* parse_resource_info */
	AZSystem_arc1a_extract_resource,	/* extract_resource */
	AZSystem_arc1a_save_resource,		/* save_resource */
	AZSystem_arc1a_release_resource,	/* release_resource */
	AZSystem_arc1a_release				/* release */
};

/********************* mpp *********************/

/* 封包匹配回调函数 */
static int AZSystem_mpp_match(struct package *pkg)
{
	mpp_header_t mpp_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &mpp_header, sizeof(mpp_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (mpp_header.unknown != 1) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	package_set_private(pkg, 2/* old_arc */);

	return 0;
}

/* 封包索引目录提取函数 */
static int AZSystem_mpp_extract_directory(struct package *pkg,
										  struct package_directory *pkg_dir)
{
	mpp_header_t mpp_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->readvec(pkg, &mpp_header, sizeof(mpp_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	mpp_entry_t *mpp_index = new mpp_entry_t[mpp_header.index_entries];
	if (!mpp_index)
		return -CUI_EMEM;

	DWORD offset = sizeof(mpp_header);
	int ret;
	for (DWORD i = 0; i < mpp_header.index_entries; ++i) {	
		cpb_old2_header_t cpb_header;

		if (pkg->pio->readvec(pkg, &cpb_header, sizeof(cpb_header), 
			offset, IO_SEEK_SET)) {
			ret = -CUI_EREADVEC;
			break;
		}

		if (strncmp(cpb_header.magic, "CPB", 4)) {
			ret = -CUI_EMATCH;
			break;
		}

		sprintf(mpp_index[i].name, "%04d.cpb", i);
		mpp_index[i].offset = offset;
		mpp_index[i].length = sizeof(cpb_header) + cpb_header.max_comprlen;
		offset += mpp_index[i].length;
	}
	if (i != mpp_header.index_entries) {
		delete [] mpp_index;
		return ret;
	}

	pkg_dir->index_entries = mpp_header.index_entries;
	pkg_dir->directory = mpp_index;
	pkg_dir->directory_length = sizeof(mpp_entry_t) * mpp_header.index_entries;
	pkg_dir->index_entry_length = sizeof(mpp_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int AZSystem_mpp_parse_resource_info(struct package *pkg,
											struct package_resource *pkg_res)
{
	mpp_entry_t *mpp_entry;

	mpp_entry = (mpp_entry_t *)pkg_res->actual_index_entry;
	strncpy(pkg_res->name, mpp_entry->name, 32);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = mpp_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = mpp_entry->offset;

	return 0;
}

static int AZSystem_mpp_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	DWORD comprlen = pkg_res->raw_data_length;
	BYTE *compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen,	pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	pkg_res->raw_data = compr;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation AZSystem_mpp_operation = {
	AZSystem_mpp_match,					/* match */
	AZSystem_mpp_extract_directory,		/* extract_directory */
	AZSystem_mpp_parse_resource_info,	/* parse_resource_info */
	AZSystem_mpp_extract_resource,		/* extract_resource */
	AZSystem_arc1a_save_resource,		/* save_resource */
	AZSystem_arc1a_release_resource,	/* release_resource */
	AZSystem_arc1a_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK AZSystem_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".arc"), NULL, 
		NULL, &AZSystem_new_arc_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".arc"), NULL, 
		NULL, &AZSystem_arc_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".arc"), NULL, 
		NULL, &AZSystem_arc1a_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	// .cpb(old2)文件合集
	if (callback->add_extension(callback->cui, _T(".mpp"), NULL, 
		NULL, &AZSystem_mpp_operation, CUI_EXT_FLAG_PKG// | CUI_EXT_FLAG_RES 
		| CUI_EXT_FLAG_DIR))
			return -1;

	AZSystem_locale_id = locale_app_register(AZSystem_locale_configurations, 3);

	return 0;
}
