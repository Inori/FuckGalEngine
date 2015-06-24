#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <resource.h>
#include <package.h>
#include <cui_error.h>
#include <package_core.h>
#include <utility.h>
#include <stdio.h>

/*
解密串可能藏在start.mjo中
TS00 解密串
00443980  /$  55            PUSH    EBP
00443981  |.  56            PUSH    ESI
00443982  |.  8B7424 0C     MOV     ESI, DWORD PTR SS:[ESP+C]
00443986  |.  83C9 FF       OR      ECX, FFFFFFFF
00443989  |.  33C0          XOR     EAX, EAX
0044398B  |.  57            PUSH    EDI
0044398C  |.  8BFE          MOV     EDI, ESI  <--串
0044398E  |.  F2:AE         REPNE   SCAS BYTE PTR ES:[EDI]
00443990  |.  A1 182A5700   MOV     EAX, DWORD PTR DS:[572A18]
00443995  |.  F7D1          NOT     ECX
00443997  |.  49            DEC     ECX
00443998  |.  83CA FF       OR      EDX, FFFFFFFF
0044399B  |.  85C0          TEST    EAX, EAX
0044399D  |.  8BE9          MOV     EBP, ECX
0044399F  |.  75 3E         JNZ     SHORT 怚廱乣枀.004439DF
004439A1  |.  53            PUSH    EBX
004439A2  |.  33FF          XOR     EDI, EDI
004439A4  |.  BE 18265700   MOV     ESI, 怚廱乣枀.00572618
004439A9  |>  8BC7          /MOV     EAX, EDI
004439AB  |.  BB 08000000   |MOV     EBX, 8
004439B0  |>  A8 01         |/TEST    AL, 1
004439B2  |.  74 09         ||JE      SHORT 怚廱乣枀.004439BD
004439B4  |.  D1E8          ||SHR     EAX, 1
004439B6  |.  35 2083B8ED   ||XOR     EAX, EDB88320 <-- 定位关键字
*/

struct acui_information Majiro_Script_Engine_cui_information = {
	_T("越畑 声太/堀井 康弘"),		/* copyright */
	_T("ＭＡＪＩＲＯ　スクリプトエンジン"),		/* system */
	_T(".arc .rct"),				/* package */
	_T("1.2.1"),					/* revision */
	_T("痴漢公賊"),					/* author */
	_T("2009-7-5 20:28"),			/* date */
	NULL,							/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

#pragma pack (1)
typedef struct {
	s8 magic[16];			/* "MajiroArcV1.000" or "MajiroArcV2.000" */
	u32 index_entries;
	u32 name_table_offset;		
	u32 data_offset;
} arc_header_t;

typedef struct {
	u32 hash;
	u32 offset;
} arc_entry_t;

typedef struct {
	u32 hash;
	u32 offset;
	u32 length;
} arc2_entry_t;

typedef struct {				
	s8 magic[8];		/* "六丁TC00" and "六丁TC01" and "六丁TS00" and "六丁TS01" */
	u32 width;
	u32 height;
	u32 data_length;
} rct_header_t;

typedef struct {		/* 接下来0x300字节是调色版 */			
	s8 magic[8];		/* "六丁8_00" */
	u32 width;
	u32 height;
	u32 data_length;
} rc8_header_t;

typedef struct {		
	s8 magic[16];		/* "MajiroObjX1.000" */
	u32 unknown0;
	u32 unknown1;
	u32 entries;		/* 8 bytes per entry */
	u8 *entries_buffer;
	u32 data_length;
	u8 *data_buffer;	
} mjo_header_t;

typedef struct {		/* 接下来0x300字节是调色版 */			
	s8 magic[16];		/* "MajiroObjV1.000" */
	u32 unknown0;
	u32 unknown1;
	u32 entries;		/* 8 bytes per entry */
	u32 *entries_buffer;
	DWORD data_length;
	u8 *data_buffer;	
} mjoV_header_t;
#pragma pack ()		

typedef struct {
	BYTE name[256];
	DWORD offset;
	DWORD length;
} my_arc_entry_t;

struct game_config {
	const TCHAR *name;
	const char *ts_key;
} game_list[] = {
	{	// 催眠ペット ～メスになったワタシ～
		_T("saimin"),
		// "催眠ペット"
		"嵜柊儁僢僩"
	},
	{	// 姉ニモマケズ ～お姉ちゃんズは刺激的！～
		_T("anetai"),
		"巓僯儌儅働僘"
	},
	{	// 神出鬼没！異次元肉棒 ～こんなところからお邪魔します～
		_T("Nikubo"),
		"偄偪偛傒傞偔"
	},
	{	// ふわりコンプレックス
		_T("f_complex"),
		"googlechrome"
	},
	{	// 誰かのために出来ること ～Innocent Identity～
		_T("I2"),
		// そんな事を思わずにはいられなかった。
		"偦傫側帠傪巚傢偢偵偼偄傜傟側偐偭偨丅"
	},
	{	// アンバークォーツ
		_T("AmberQuartz"),
		// 夕焼け。学校の屋上。そこに伸びる二つの影。
		"梉從偗丅妛峑偺壆忋丅偦偙偵怢傃傞擇偮偺塭丅"
	},
	{	// 暁の護衛 ～プリンシパルたちの休日～
		_T("akatsuki_fd"),
		// もう足が針のようだよ
		"傕偆懌偑恓偺傛偆偩傛"
	},
	{	// 痴漢プレイ 電車の中で○○や××なこと
		_T("cplay"),
		// 恋人はチカン
		"楒恖偼僠僇儞"
	},
	{	// ゆ～パラ　～ただいま乳院中～
		_T("you"),
		// ガマン病院ＫＥＹ
		"僈儅儞昦堾俲俤倄"
	},
	{	// ちょこっと！ファンディスク
		_T("ochige"),
		// ミーンミンミンミーン…
		"儈乕儞儈儞儈儞儈乕儞乧"
	},
	{	// はっぴぃプリンセス Another Fairytale
		_T("hpaf"),
		"042314925971"
	},
	{	// 暁の護衛
		_T("akatsuki"),
		// おぬぐり食べる？
		"偍偸偖傝怘傋傞丠"
	},
	{	// 催淫術師 ～恥辱まみれの絶頂～
		_T("jutu"),
		// うはうはバレンタイン
		"偆偼偆偼僶儗儞僞僀儞"
	},
	{	// すいっチ！！ ～ボクがナツに想うこと～
		_T("swops"),
		"WinCVS"
	},
	{	// ナギサの
		_T("nagisano"),
		// 青い空に向かって、溜息を一つこぼす。
		"惵偄嬻偵岦偐偭偰丄棴懅傪堦偮偙傏偡丅"
	},
	{	// 蝕獣～妹のおしり～美羽編
		_T("SYOKUJYU"),
		// スリジャヤワルダナプラコッテ
		"僗儕僕儍儎儚儖僟僫僾儔僐僢僥"
	},
	{
		NULL, NULL
	}
};

static const char *ts_key;

static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static u32 hash_table[256];
static u32 hash_table2[256];

static void init_hash(void)
{
	unsigned int i, k;

	for (k = 0; k < 256; k++) {	
		DWORD flag = k;
		for (i = 0; i < 8; i++) {
			if (flag & 1)
				flag = (flag >> 1) ^ 0xEDB88320;
			else 
				flag >>= 1;
		}
		hash_table[k] = flag;
	}
}

/* seed determinated by 42f505 */
static DWORD do_hash(DWORD seed, BYTE *name, DWORD name_len)
{
	for (unsigned int i = 0; i < name_len; i++)
		seed = hash_table[name[i] ^ (seed & 0xff)] ^ (seed >> 8);
	return ~seed;
}

static void init_hash2(BYTE *name, DWORD name_len)
{
	u32 hash;

	hash = do_hash(-1, name, name_len);
	for (unsigned int i = 0; i < 256; i++)	
		hash_table2[i] = hash ^ hash_table[(i + (BYTE)hash) % 256];	
}

static DWORD rct_decompress(BYTE *uncompr, DWORD uncomprLen, 
		BYTE *compr, DWORD comprLen, DWORD width)
{
	DWORD act_uncomprLen = 0;
	DWORD curByte = 0;
	DWORD pos[32];
	
	pos[0] = -3;
	pos[1] = -6;
	pos[2] = -9;
	pos[3] = -12;	
	pos[4] = -15;
	pos[5] = -18;
	pos[6] = (3 - width) * 3;	
	pos[7] = (2 - width) * 3;		
	pos[8] = (1 - width) * 3;
	pos[9] = (0 - width) * 3;
	pos[10] = (-1 - width) * 3;		
	pos[11] = (-2 - width) * 3;
	pos[12] = (-3 - width) * 3;		
	pos[13] = 9 - ((width * 3) << 1);
	pos[14] = 6 - ((width * 3) << 1);
	pos[15] = 3 - ((width * 3) << 1);	
	pos[16] = 0 - ((width * 3) << 1);	
	pos[17] = -3 - ((width * 3) << 1);		
	pos[18] = -6 - ((width * 3) << 1);
	pos[19] = -9 - ((width * 3) << 1);
	pos[20] = 9 - width * 9;	
	pos[21] = 6 - width * 9;
	pos[22] = 3 - width * 9;	
	pos[23] = 0 - width * 9;	
	pos[24] = -3 - width * 9;
	pos[25] = -6 - width * 9;	
	pos[26] = -9 - width * 9;
	pos[27] = 6 - ((width * 3) << 2);
	pos[28] = 3 - ((width * 3) << 2);
	pos[29] = 0 - ((width * 3) << 2);
	pos[30] = -3 - ((width * 3) << 2);	
	pos[31] = -6 - ((width * 3) << 2);	
						
	uncompr[act_uncomprLen++] = compr[curByte++];
	uncompr[act_uncomprLen++] = compr[curByte++];	
	uncompr[act_uncomprLen++] = compr[curByte++];
		
	while (1) {
		BYTE flag;
		DWORD copy_bytes, copy_pos;
		
		if (curByte >= comprLen)
			break;	

		flag = compr[curByte++];
			
		if (!(flag & 0x80)) {
			if (flag != 0x7f)
				copy_bytes = flag * 3 + 3;
			else {
				if (curByte + 1 >= comprLen)
					break;
										
				copy_bytes = compr[curByte++];
				copy_bytes |= compr[curByte++] << 8;
				copy_bytes += 0x80;
				copy_bytes *= 3;
			}
			
			if (curByte + copy_bytes - 1 >= comprLen)
				break;	
			if (act_uncomprLen + copy_bytes - 1 >= uncomprLen)
				break;
								
			memcpy(&uncompr[act_uncomprLen], &compr[curByte], copy_bytes);
			act_uncomprLen += copy_bytes;
			curByte += copy_bytes;	
		} else {
			copy_bytes = flag & 3;
			copy_pos = (flag >> 2) & 0x1f;
			
			if (copy_bytes != 3) {
				copy_bytes = copy_bytes * 3 + 3;
			} else {
				if (curByte + 1 >= comprLen)
					break;	
					
				copy_bytes = compr[curByte++];
				copy_bytes |= compr[curByte++] << 8;
				copy_bytes += 4;
				copy_bytes *= 3;			
			}
			
			for (unsigned int i = 0; i < copy_bytes; i++) {
				if (act_uncomprLen >= uncomprLen)
					goto out;	
				uncompr[act_uncomprLen] = uncompr[act_uncomprLen + pos[copy_pos]];	
				act_uncomprLen++;			
			}
		}	
	}
out:
//	if (curByte != comprLen)
//		fprintf(stderr, "compr miss-match %d VS %d\n", curByte, comprLen);

	return act_uncomprLen;		
}

static DWORD rc8_decompress(BYTE *uncompr, DWORD uncomprLen, 
		BYTE *compr, DWORD comprLen, DWORD width)
{
	DWORD act_uncomprLen = 0;
	DWORD curByte = 0;
	DWORD pos[16];
	
	pos[0] = -1;
	pos[1] = -2;
	pos[2] = -3;
	pos[3] = -4;		
	pos[4] = 3 - width;
	pos[5] = 2 - width;		
	pos[6] = 1 - width;
	pos[7] = 0 - width;
	pos[8] = -1 - width;
	pos[9] = -2 - width;	
	pos[10] = -3 - width;	
	pos[11] = 2 - (width * 2);
	pos[12] = 1 - (width * 2);		
	pos[13] = (0 - width) << 1;
	pos[14] = -1 - (width * 2);
	pos[15] = (-1 - width) * 2;
					
	uncompr[act_uncomprLen++] = compr[curByte++];
	
	while (1) {
		BYTE flag;
		DWORD copy_bytes, copy_pos;
		
		if (curByte >= comprLen)
			break;	

		flag = compr[curByte++];
			
		if (!(flag & 0x80)) {
			if (flag != 0x7f)
				copy_bytes = flag + 1;
			else {
				if (curByte + 1 >= comprLen)
					break;
										
				copy_bytes = compr[curByte++];
				copy_bytes |= compr[curByte++] << 8;
				copy_bytes += 0x80;
			}
			
			if (curByte + copy_bytes - 1 >= comprLen)
				break;	
			if (act_uncomprLen + copy_bytes - 1 >= uncomprLen)
				break;
								
			memcpy(&uncompr[act_uncomprLen], &compr[curByte], copy_bytes);
			act_uncomprLen += copy_bytes;
			curByte += copy_bytes;	
		} else {
			copy_bytes = flag & 7;
			copy_pos = (flag >> 3) & 0xf;
			
			if (copy_bytes != 7)
				copy_bytes += 3;
			else {
				if (curByte + 1 >= comprLen)
					break;	
					
				copy_bytes = compr[curByte++];
				copy_bytes |= compr[curByte++] << 8;
				copy_bytes += 0xa;			
			}
			
			for (unsigned int i = 0; i < copy_bytes; i++) {
				if (act_uncomprLen >= uncomprLen)
					break;	
				uncompr[act_uncomprLen] = uncompr[act_uncomprLen + pos[copy_pos]];	
				act_uncomprLen++;			
			}
		}	
	}

//	if (curByte != comprLen)
//		fprintf(stderr, "compr miss-match %d VS %d\n", curByte, comprLen);

	return act_uncomprLen;		
}

static int dump_mjo(BYTE *buf, DWORD length, BYTE **ret_buf, DWORD *ret_len)
{
	mjo_header_t mjo;
	BYTE *tmp = buf;
	BYTE *htable = (BYTE *)hash_table;

	mjo.unknown0 = *(DWORD *)(buf + 16);
	buf += 20;

	mjo.unknown1 = *(DWORD *)buf;
	buf += 4;
	mjo.entries = *(DWORD *)buf;
	buf += 4;
	mjo.entries_buffer = buf;
	buf += mjo.entries * 8;
	mjo.data_length = *(DWORD *)buf;
	buf += 4;
	mjo.data_buffer = buf;

	if (buf + mjo.data_length != tmp + length)
		return -CUI_EMATCH;

	*ret_buf = (BYTE *)malloc(mjo.data_length);
	if (!*ret_buf)
		return -CUI_EMEM;

//	printf("entries %d, unknown0 %d, unknown1 %d, data length %d bytes\n",
//		mjo.entries, mjo.unknown0, mjo.unknown1, mjo.data_length);

	for (unsigned int i = 0; i < mjo.data_length; i++)
		(*ret_buf)[i] = mjo.data_buffer[i] ^ htable[i & (sizeof(hash_table) - 1)];

	*ret_len = mjo.data_length;

	return 0;
}

static int dump_ts00(rct_header_t *rct, DWORD length, BYTE **ret_buf, DWORD *ret_len)
{
	BYTE *compr, *uncompr;
	DWORD uncomprLen, comprLen, actLen;
	BYTE *htable = (BYTE *)hash_table2;

	uncomprLen = rct->width * rct->height * 3;
	uncompr = (BYTE *)malloc(uncomprLen);
	if (!uncomprLen)
		return -CUI_EMEM;	
	
	compr = (BYTE *)(rct + 1);	
	comprLen = rct->data_length;
	for (unsigned int i = 0; i < comprLen; i++)
		compr[i] ^= htable[i & (sizeof(hash_table2) - 1)];

	actLen = rct_decompress(uncompr, uncomprLen, compr, comprLen, rct->width);
	if (actLen != uncomprLen) {
		free(uncompr);	
		return -CUI_EUNCOMPR;
	}

	if (MyBuildBMPFile(uncompr, uncomprLen, NULL, 0, 
			rct->width, 0 - rct->height, 24, ret_buf, ret_len, my_malloc)) {
		free(uncompr);	
		return -CUI_EMEM;	
	}
	free(uncompr);
		
	return 0;		
}

static int dump_ts01(rct_header_t *rct, DWORD length, BYTE **ret_buf, DWORD *ret_len)
{
	BYTE *compr, *uncompr;
	DWORD uncomprLen, comprLen, actLen;
	BYTE *htable = (BYTE *)hash_table2;
	WORD fn_len;

	uncomprLen = rct->width * rct->height * 3;
	uncompr = (BYTE *)malloc(uncomprLen);
	if (!uncomprLen)
		return -CUI_EMEM;	
	
	compr = (BYTE *)(rct + 1);
	fn_len = *(WORD *)compr;
	compr += 2;
	compr += fn_len;
	comprLen = rct->data_length;
	for (unsigned int i = 0; i < comprLen; i++)
		compr[i] ^= htable[i & (sizeof(hash_table) - 1)];
	actLen = rct_decompress(uncompr, uncomprLen, compr, comprLen, rct->width);
	if (actLen != uncomprLen) {
		free(uncompr);	
		return -CUI_EUNCOMPR;
	}

	if (MyBuildBMPFile(uncompr, uncomprLen, NULL, 0, 
			rct->width, 0 - rct->height, 24, ret_buf, ret_len, my_malloc)) {
		free(uncompr);	
		return -CUI_EMEM;	
	}
	free(uncompr);
		
	return 0;		
}

static int dump_rct(rct_header_t *rct, DWORD length, BYTE **ret_buf, DWORD *ret_len)
{
	BYTE *compr, *uncompr;
	DWORD uncomprLen, comprLen, actLen;

	uncomprLen = rct->width * rct->height * 3;
	uncompr = (BYTE *)malloc(uncomprLen);
	if (!uncomprLen)
		return -1;	
	
	compr = (BYTE *)(rct + 1);	
	comprLen = rct->data_length;
	actLen = rct_decompress(uncompr, uncomprLen, compr, comprLen, rct->width);
	if (actLen != uncomprLen) {
//		fprintf(stderr, "rct decompress error occured (%d VS %d)\n",
//			actLen, uncomprLen);
		free(uncompr);	
		return -1;
	}

	if (MyBuildBMPFile(uncompr, uncomprLen, NULL, 0, 
			rct->width, 0 - rct->height, 24, ret_buf, ret_len, my_malloc)) {
		free(uncompr);	
		return -1;	
	}
	free(uncompr);
		
	return 0;		
}

static int dump_tc01(rct_header_t *rct, DWORD length, BYTE **ret_buf, DWORD *ret_len)
{
	BYTE *compr, *uncompr;
	DWORD uncomprLen, comprLen, actLen;

	uncomprLen = rct->width * rct->height * 3;
	uncompr = (BYTE *)malloc(uncomprLen);
	if (!uncomprLen)
		return -1;	
	
	compr = (BYTE *)(rct + 1);	
	comprLen = rct->data_length;
	actLen = rct_decompress(uncompr, uncomprLen, compr, comprLen, rct->width);
	if (actLen != uncomprLen) {
//		fprintf(stderr, "rct decompress error occured (%d VS %d)\n",
//			actLen, uncomprLen);
		free(uncompr);	
		return -1;
	}

	if (MyBuildBMPFile(uncompr, uncomprLen, NULL, 0, 
			rct->width, 0 - rct->height, 24, ret_buf, ret_len, my_malloc)) {
		free(uncompr);	
		return -1;	
	}
	free(uncompr);
		
	return 0;		
}

static int dump_rct1(rct_header_t *rct, DWORD length, BYTE **ret_buf, DWORD *ret_len)
{
	BYTE *compr, *uncompr;
	DWORD uncomprLen, comprLen, actLen;
	WORD fn_len;

	uncomprLen = rct->width * rct->height * 3;
	uncompr = (BYTE *)malloc(uncomprLen);
	if (!uncomprLen)
		return -1;	
	
	compr = (BYTE *)(rct + 1);
	fn_len = *(WORD *)compr;
	compr += 2;
	compr += fn_len;
	comprLen = rct->data_length;
	
	actLen = rct_decompress(uncompr, uncomprLen, compr, comprLen, rct->width);
	if (actLen != uncomprLen) {
//		fprintf(stderr, "rct decompress error occured (%d VS %d)\n",
//			actLen, uncomprLen);
		free(uncompr);	
		return -1;
	}

	if (MyBuildBMPFile(uncompr, uncomprLen, NULL, 0, 
			rct->width, 0 - rct->height, 24, ret_buf, ret_len, my_malloc)) {
		free(uncompr);	
		return -1;	
	}
	free(uncompr);
		
	return 0;		
}

static int dump_rc8( rc8_header_t *rc8, DWORD length, BYTE **ret_buf, DWORD *ret_len)
{
	BYTE *compr, *uncompr, *palette, *act_palette;
	DWORD uncomprLen, comprLen, actLen;		
	
	uncomprLen = rc8->width * rc8->height;
	uncompr = (BYTE *)malloc(uncomprLen);
	if (!uncomprLen)
		return -1;	
	
	palette = (BYTE *)(rc8 + 1);
	compr = palette + 0x300;	
	comprLen = rc8->data_length;
	actLen = rc8_decompress(uncompr, uncomprLen, compr, comprLen, rc8->width);
	if (actLen != uncomprLen) {
//		fprintf(stderr, "rc8 decompress error occured (%d VS %d)\n",
//			actLen, uncomprLen);
		free(uncompr);	
		return -1;
	}
	
	act_palette = (BYTE *)malloc(1024);
	if (!act_palette) {
		free(uncompr);	
		return -1;		
	}	

	for (unsigned int i = 0 ; i < 256; i++) {
		act_palette[i * 4 + 0] = palette[i * 3 + 0];
		act_palette[i * 4 + 1] = palette[i * 3 + 1];
		act_palette[i * 4 + 2] = palette[i * 3 + 2];
		act_palette[i * 4 + 3] = 0;
	}

	if (MyBuildBMPFile(uncompr, uncomprLen, act_palette, 1024, 
			rc8->width, 0 - rc8->height, 8, ret_buf, ret_len, my_malloc)) {
		free(act_palette);
		free(uncompr);	
		return -1;	
	}
	free(act_palette);
	free(uncompr);		
	
	return 0;	
}

/********************* arc *********************/

static int Majiro_Script_Engine_arc_match(struct package *pkg)
{	
	s8 magic[16];
	unsigned int ver;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->read(pkg, magic, 16)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (!memcmp(magic, "MajiroArcV1.000", 16))
		ver = 1;
	else if (!memcmp(magic, "MajiroArcV2.000", 16))
		ver = 2;
	else {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;		
	}
	package_set_private(pkg, (void *)ver);

	return 0;	
}

static int Majiro_Script_Engine_arc1_extract_directory(struct package *pkg,
													 struct package_directory *pkg_dir)
{		
	unsigned int index_buffer_len;
	arc_entry_t *index_offset_buffer;
	u32 index_entries, name_table_offset, data_offset;
	u32 arc_file_size;
	my_arc_entry_t *my_arc_entry;
	s8 *name_buffer;

	if (pkg->pio->length_of(pkg, &arc_file_size))
		return -CUI_ELEN;
	
	if (pkg->pio->read(pkg, &index_entries, 4))
		return -CUI_EREAD;
	
	if (pkg->pio->read(pkg, &name_table_offset, 4))
		return -CUI_EREAD;
	
	if (pkg->pio->read(pkg, &data_offset, 4))
		return -CUI_EREAD;

	index_buffer_len = (index_entries + 1) * sizeof(arc_entry_t);		
	index_offset_buffer = (arc_entry_t *)malloc(index_buffer_len);
	if (!index_offset_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_offset_buffer, index_buffer_len)) {
		free(index_offset_buffer);
		return -CUI_EREAD;
	}

	/* last index_entry: 0x0 and file_size */
	if (index_offset_buffer[index_entries].hash) {		
		free(index_offset_buffer);
		return -CUI_EMATCH;		
	}
	
	if (arc_file_size != index_offset_buffer[index_entries].offset) {
		free(index_offset_buffer);
		return -CUI_EMATCH;		
	}

	if (pkg->pio->seek(pkg, name_table_offset, IO_SEEK_SET)) {
		free(index_offset_buffer);
		return -CUI_EREAD;
	}

	name_buffer = (s8 *)malloc(data_offset - name_table_offset);
	if (!name_buffer) {
		free(index_offset_buffer);
		return -CUI_EMEM;	
	}

	if (pkg->pio->read(pkg, name_buffer, data_offset - name_table_offset)) {
		free(name_buffer);
		free(index_offset_buffer);
		return -CUI_EREAD;
	}

	index_buffer_len = index_entries * sizeof(my_arc_entry_t);
	my_arc_entry = (my_arc_entry_t *)malloc(index_buffer_len);
	if (!my_arc_entry) {
		free(name_buffer);
		free(index_offset_buffer);
		return -CUI_EMEM;		
	}

	s8 *name_p = name_buffer;
	for (unsigned int i = 0; i < index_entries; i++) {
		int name_len;
		u32 hash;

		name_len = strlen(name_p);
		memcpy(my_arc_entry[i].name, name_p, name_len + 1);
		my_arc_entry[i].name[name_len] = 0;		
		hash = do_hash(-1, (BYTE *)name_p, name_len);
		if (hash != index_offset_buffer[i].hash)
			break;
		name_p += name_len + 1;
		my_arc_entry[i].offset = index_offset_buffer[i].offset;
		my_arc_entry[i].length = index_offset_buffer[i + 1].offset - index_offset_buffer[i].offset;	
	}
	free(name_buffer);
	free(index_offset_buffer);
	if (i != index_entries) {
		free(my_arc_entry);
		return -CUI_EMATCH;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->index_entry_length = sizeof(my_arc_entry_t);
	pkg_dir->directory = my_arc_entry;
	pkg_dir->directory_length = index_buffer_len;

	return 0;
}

static int Majiro_Script_Engine_arc2_extract_directory(struct package *pkg,
													 struct package_directory *pkg_dir)
{	
	arc2_entry_t *index_buffer;
	unsigned int index_buffer_len;
	u32 index_entries, name_table_offset, data_offset;
	my_arc_entry_t *my_arc_entry;
	s8 *name_buffer;
	
	if (pkg->pio->read(pkg, &index_entries, 4))
		return -CUI_EREAD;
	
	if (pkg->pio->read(pkg, &name_table_offset, 4))
		return -CUI_EREAD;
	
	if (pkg->pio->read(pkg, &data_offset, 4))
		return -CUI_EREAD;

	index_buffer_len = (index_entries) * sizeof(arc2_entry_t);		
	index_buffer = (arc2_entry_t *)malloc(index_buffer_len);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_len)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	if (pkg->pio->seek(pkg, name_table_offset, IO_SEEK_SET)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	name_buffer = (s8 *)malloc(data_offset - name_table_offset);
	if (!name_buffer) {
		free(index_buffer);
		return -CUI_EMEM;	
	}

	if (pkg->pio->read(pkg, name_buffer, data_offset - name_table_offset)) {
		free(name_buffer);
		free(index_buffer);
		return -CUI_EREAD;
	}

	index_buffer_len = index_entries * sizeof(my_arc_entry_t);
	my_arc_entry = (my_arc_entry_t *)malloc(index_buffer_len);
	if (!my_arc_entry) {
		free(name_buffer);
		free(index_buffer);
		return -CUI_EMEM;		
	}

	s8 *name_p = name_buffer;
	for (unsigned int i = 0; i < index_entries; i++) {
		int name_len;
		u32 hash;

		name_len = strlen(name_p);
		memcpy(my_arc_entry[i].name, name_p, name_len + 1);
		my_arc_entry[i].name[name_len] = 0;		
		hash = do_hash(-1, (BYTE *)name_p, name_len);
		if (hash != index_buffer[i].hash)
			break;
		name_p += name_len + 1;
		my_arc_entry[i].offset = index_buffer[i].offset;
		my_arc_entry[i].length = index_buffer[i].length;	
	}
	free(name_buffer);
	free(index_buffer);
	if (i != index_entries) {
		free(my_arc_entry);
		return -CUI_EMATCH;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->index_entry_length = sizeof(my_arc_entry_t);
	pkg_dir->directory = my_arc_entry;
	pkg_dir->directory_length = index_buffer_len;

	return 0;
}

static int Majiro_Script_Engine_arc_extract_directory(struct package *pkg,
													  struct package_directory *pkg_dir)
{		
	unsigned int ver;

	ver = (unsigned int)package_get_private(pkg);
	return ver == 1 ? Majiro_Script_Engine_arc1_extract_directory(pkg, pkg_dir) 
		: Majiro_Script_Engine_arc2_extract_directory(pkg, pkg_dir);
}

static int Majiro_Script_Engine_arc_parse_resource_info(struct package *pkg,
														struct package_resource *pkg_res)
{
	my_arc_entry_t *arc_entry;

	arc_entry = (my_arc_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, (char *)arc_entry->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = arc_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = arc_entry->offset;

	return 0;
}

static int Majiro_Script_Engine_arc_extract_resource(struct package *pkg,
													 struct package_resource *pkg_res)
{	
	BYTE *data;

	data = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, data, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET)) {
		free(data);
		return -CUI_EREADVEC;
	}

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = data;
		return 0;
	}

	if (!memcmp(data, "\x98\x5a\x92\x9aTC00", 8)) {
		if (dump_rct((rct_header_t *)data, pkg_res->raw_data_length, (BYTE **)&pkg_res->actual_data, (DWORD *)&pkg_res->actual_data_length)) {
			free(data);
			return -CUI_EUNCOMPR;		
		}
		pkg_res->replace_extension = _T(".bmp");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
	} else if (!memcmp(data, "\x98\x5a\x92\x9aTC01", 8)) {
		if (dump_rct1((rct_header_t *)data, pkg_res->raw_data_length, (BYTE **)&pkg_res->actual_data, (DWORD *)&pkg_res->actual_data_length)) {
			free(data);
			return -CUI_EUNCOMPR;		
		}
		pkg_res->replace_extension = _T(".bmp");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
	} else if (!memcmp(data, "\x98\x5a\x92\x9a\x38_00", 8)) {
		if (dump_rc8((rc8_header_t *)data, pkg_res->raw_data_length, (BYTE **)&pkg_res->actual_data, (DWORD *)&pkg_res->actual_data_length)) {
			free(data);
			return -CUI_EUNCOMPR;
		}
		pkg_res->replace_extension = _T(".bmp");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;		
	} else if (ts_key && !memcmp(data, "\x98\x5a\x92\x9aTS00", 8)) {
		if (dump_ts00((rct_header_t *)data, pkg_res->raw_data_length, (BYTE **)&pkg_res->actual_data, (DWORD *)&pkg_res->actual_data_length)) {
			free(data);
			return -CUI_EUNCOMPR;		
		}
		pkg_res->replace_extension = _T(".bmp");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
	} else if (ts_key && !memcmp(data, "\x98\x5a\x92\x9aTS01", 8)) {
		if (dump_ts01((rct_header_t *)data, pkg_res->raw_data_length, (BYTE **)&pkg_res->actual_data, (DWORD *)&pkg_res->actual_data_length)) {
			free(data);
			return -CUI_EUNCOMPR;		
		}
		pkg_res->replace_extension = _T(".bmp");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
	} else if (!memcmp(data, "RIFF", 4)) {
		pkg_res->replace_extension = _T(".wav");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
	} else if (!strcmp((char *)data, "MajiroObjX1.000")) {
		if (dump_mjo(data, pkg_res->raw_data_length, (BYTE **)&pkg_res->actual_data, (DWORD *)&pkg_res->actual_data_length)) {
			free(data);
			return -CUI_EUNCOMPR;
		}
	} else if (!strcmp((char *)data, "MajiroObjV1.000")) {
		if (dump_mjo(data, pkg_res->raw_data_length, (BYTE **)&pkg_res->actual_data, (DWORD *)&pkg_res->actual_data_length)) {
			free(data);
			return -CUI_EUNCOMPR;
		}
	}
	pkg_res->raw_data = data;

	return 0;
}

static int Majiro_Script_Engine_arc_save_resource(struct resource *res, 
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

static void Majiro_Script_Engine_arc_release_resource(struct package *pkg, 
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

static void Majiro_Script_Engine_arc_release(struct package *pkg, 
										   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg);
}

static cui_ext_operation Majiro_Script_Engine_arc_operation = {
	Majiro_Script_Engine_arc_match,					/* match */
	Majiro_Script_Engine_arc_extract_directory,		/* extract_directory */
	Majiro_Script_Engine_arc_parse_resource_info,	/* parse_resource_info */
	Majiro_Script_Engine_arc_extract_resource,		/* extract_resource */
	Majiro_Script_Engine_arc_save_resource,			/* save_resource */
	Majiro_Script_Engine_arc_release_resource,		/* release_resource */
	Majiro_Script_Engine_arc_release				/* release */
};

/********************* rct *********************/

static int Majiro_Script_Engine_rct_match(struct package *pkg)
{	
	s8 magic[8];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->read(pkg, magic, 8)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "\x98\x5a\x92\x9aTC00", 8) && memcmp(magic, "\x98\x5a\x92\x9aTC01", 8)
			&& memcmp(magic, "\x98\x5a\x92\x9a\x38_00", 8)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;		
	}

	if (pkg->pio->seek(pkg, 0, IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	return 0;	
}

static int Majiro_Script_Engine_rct_extract_resource(struct package *pkg,
												   struct package_resource *pkg_res)
{	
	BYTE *data;

	data = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, data, pkg_res->raw_data_length,	pkg_res->offset, IO_SEEK_SET)) {
		free(data);
		return -CUI_EREADVEC;
	}

	if (!memcmp(data, "\x98\x5a\x92\x9aTC00", 8)) {
		if (dump_rct((rct_header_t *)data, pkg_res->raw_data_length, (BYTE **)&pkg_res->actual_data, (DWORD *)&pkg_res->actual_data_length)) {
			free(data);
			return -CUI_EUNCOMPR;		
		}
		pkg_res->replace_extension = _T(".bmp");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
	} else if (!memcmp(data, "\x98\x5a\x92\x9aTC01", 8)) {
		if (dump_rct1((rct_header_t *)data, pkg_res->raw_data_length, (BYTE **)&pkg_res->actual_data, (DWORD *)&pkg_res->actual_data_length)) {
			free(data);
			return -CUI_EUNCOMPR;		
		}
		pkg_res->replace_extension = _T(".bmp");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
	} else	if (!memcmp(data, "\x98\x5a\x92\x9a\x38_00", 8)) {
		if (dump_rc8((rc8_header_t *)data, pkg_res->raw_data_length, (BYTE **)&pkg_res->actual_data, (DWORD *)&pkg_res->actual_data_length)) {
			free(data);
			return -CUI_EUNCOMPR;
		}
		pkg_res->replace_extension = _T(".bmp");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;		
	}
	pkg_res->raw_data = data;

	return 0;
}

static int Majiro_Script_Engine_rct_save_resource(struct resource *res, 
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

static void Majiro_Script_Engine_rct_release_resource(struct package *pkg, 
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

static void Majiro_Script_Engine_rct_release(struct package *pkg, 
											 struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

static cui_ext_operation Majiro_Script_Engine_rct_operation = {
	Majiro_Script_Engine_rct_match,				/* match */
	NULL,										/* extract_directory */
	NULL,										/* parse_resource_info */
	Majiro_Script_Engine_rct_extract_resource,	/* extract_resource */
	Majiro_Script_Engine_rct_save_resource,		/* save_resource */
	Majiro_Script_Engine_rct_release_resource,	/* release_resource */
	Majiro_Script_Engine_rct_release			/* release */
};

static void find_ts_key(BYTE *in, DWORD size, 
						BYTE *ret_key, DWORD *ret_key_len)
{
	BYTE *end = in + size;

	while (in < end) {
		if (*(u16 *)in != 0x0801) {
			++in;
			continue;
		}

		in += 2;
		short key_len = *(u16 *)in;
		if (key_len > 4 && key_len <= 64 && (in + key_len < end)) {
			*ret_key_len = key_len - 1;
			strcpy((char *)ret_key, (char *)in + 2);
			break;
		}
	}
}
	
int CALLBACK Majiro_Script_Engine_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".arc"), NULL, 
		NULL, &Majiro_Script_Engine_arc_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".rct"), _T(".bmp"), 
		NULL, &Majiro_Script_Engine_rct_operation, CUI_EXT_FLAG_PKG))
			return -1;

	init_hash();
	ts_key = get_options("ts_key");
	if (!ts_key) {
		const TCHAR *game = get_options2(_T("game"));
		if (game) {
			for (DWORD i = 0; game_list[i].name; ++i) {
				if (!_tcsicmp(game, game_list[i].name)) {
					ts_key = game_list[i].ts_key;
					break;
				}
			}
		} else {
			const char *start_mjo = get_options("scenario");
			if (start_mjo) {
				TCHAR path[MAX_PATH];

				acp2unicode(start_mjo, -1, path, MAX_PATH);
				struct package *pkg = package_fio_create(_T(""), path);
				if (pkg) {
					struct package_directory pkg_dir;
					int ts_key_ok = 0;

					memset(&pkg_dir, 0, sizeof(pkg_dir));
					if (!Majiro_Script_Engine_arc_match(pkg)) {
						if (!Majiro_Script_Engine_arc_extract_directory(pkg, &pkg_dir)) {
							struct package_resource pkg_res;
	
							memset(&pkg_res, 0, sizeof(pkg_res));
							pkg_res.actual_index_entry = pkg_dir.directory;

							for (DWORD i = 0; i < pkg_dir.index_entries; ++i) {
								if (Majiro_Script_Engine_arc_parse_resource_info(pkg, &pkg_res))
									break;

								if (!strcmp(pkg_res.name, "start.mjo")) {
									if (!Majiro_Script_Engine_arc_extract_resource(pkg, &pkg_res)) {
										BYTE _ts_key[MAX_PATH];
										DWORD _ts_key_len = 0;
										find_ts_key((BYTE *)pkg_res.actual_data, 
											pkg_res.actual_data_length > 128 ? 128 : pkg_res.actual_data_length,
											_ts_key, &_ts_key_len);
										if (!_ts_key_len)
											break;
										init_hash2(_ts_key, _ts_key_len);	
										ts_key_ok = 1;
										Majiro_Script_Engine_arc_release_resource(pkg, &pkg_res);
										break;
									}
								}

								pkg_res.actual_index_entry = (BYTE *)pkg_res.actual_index_entry 
									+ pkg_dir.index_entry_length;
							}	
						}
						Majiro_Script_Engine_arc_release(pkg, &pkg_dir);
					}
					package_release(pkg);
					if (ts_key_ok) {
						ts_key = (const char *)1;
						return 0;
					}
				}
			}
		}
	}

	if (ts_key)
		init_hash2((BYTE *)ts_key, strlen(ts_key));

	return 0;
}
