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

/*
寻找pic参数解密的顺序：
查找：
CML AL, 61
CMP AL, 62

  MOV DWORD PTR SS:[EBP+10],EAX
*/
// 分析垃圾数据的用途：J:\Program Files\TinkerBell\kasumi2

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information Csystem_cui_information = {
	_T("Cyberworks"),		/* copyright */
	_T("C,system"),			/* system */
	_T(".dat .00 .01 .02 .03. 04 .05 .06"),	/* package */
	_T("1.0.3"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-6-20 12:58"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	u8 uncomprlen[8];
	u8 comprlen[8];
} dat_header_t;
#pragma pack ()

typedef struct {
	char name[256];
	DWORD offset;
	DWORD comprlen;
	DWORD uncomprlen;
	DWORD type;
	DWORD unknown;
} dat_entry_t;

static int Debug;

#define WIDTH			0	// MOV DWORD PTR SS:[EBP+10],EAX
#define HEIGHT			1	// MOV DWORD PTR SS:[EBP+C],EAX
#define LENGTH			2	// MOV DWORD PTR SS:[EBP+14],EAX
#define TYPE			3	// MOV BYTE PTR SS:[EBP+A4],AL
#define BITMAP_LENGTH	4	// MOV DWORD PTR SS:[EBP+1C],EAX
#define COMPR_LENGTH	5	// MOV DWORD PTR SS:[EBP+18],EAX
#define UNKNOWN0		6	// MOV DWORD PTR SS:[EBP+4],EAX
#define UNKNOWN1		7	// MOV DWORD PTR SS:[EBP+8],EAX

static DWORD game_pic_parameter[7][3] = {
	{ 0xe9, 0xef, 0xfb },
	{ 0xe9, 0xc7, 0xfb },
	{ 0xf1, 0xfb, 0xfa },
	{ 0xf9, 0xfb, 0xfa },
	{ 0xc7, 0xfb, 0xfa },
	{ 0xef, 0xfb, 0xfa },
	{ 0xc8, 0xc9, 0xfa },
};

struct game_config {
	const char *name;
	DWORD parameter[8];
	DWORD *pic_parameter;
	DWORD parameter_number;
};

static struct game_config Ndoki_config = {
	"Ndoki",
	{
		5,	/* width */
		1,	/* height */
		3,	/* dib len */
		6,	/* type */
		2,	/* bitmap len */
		4,	/* compr len */
		0,	/* unknown0 */
		7,	/* unknown1 */
	},
	game_pic_parameter[0],
	8
};

static struct game_config tyouR_config = {
	"tyouR",
	{
		2,	/* width */
		1,	/* height */
		5,	/* dib len */
		3,	/* type */
		4,	/* bitmap len */
		6,	/* compr len */
		0,	/* unknown0 */
		7,	/* unknown1 */
	},
	game_pic_parameter[0],
	8
};

static struct game_config GYK_config = {
	"GYK",
	{
		5,	/* width */
		4,	/* height */
		3,	/* dib len */
		1,	/* type */
		2,	/* bitmap len */
		6,	/* compr len */
		0,	/* unknown0 */
		7,	/* unknown1 */
	},
	game_pic_parameter[0],
	8
};

static struct game_config koituma_config = {
	"koituma",
	{
		2,	/* width */
		1,	/* height */
		5,	/* dib len */
		3,	/* type */
		4,	/* bitmap len */
		6,	/* compr len */
		0,	/* unknown0 */
		7,	/* unknown1 */
	},
	game_pic_parameter[0],
	8
};

static struct game_config hobo_config = {
	"hobo",
	{
		1,	/* width */
		2,	/* height */
		5,	/* dib len */
		0,	/* type */
		3,	/* bitmap len */
		4,	/* compr len */
		6,	/* unknown0 */
		7,	/* unknown1 */
	},
	game_pic_parameter[6],
	6
};

static struct game_config inyou_shock_config = {
	"inyou_shock",
	{
		0,	/* width */
		2,	/* height */
		4,	/* dib len */
		7,	/* type */
		1,	/* bitmap len */
		5,	/* compr len */
		6,	/* unknown0 */
		3,	/* unknown1 */
	},
	game_pic_parameter[5],
	8
};

static struct game_config inyoutyu_config = {
	"inyoutyu",
	{
		6,	/* width */
		7,	/* height */
		2,	/* dib len */
		5,	/* type */
		2,	/* bitmap len */
		3,	/* compr len */
		0,	/* unknown0 */
		4,	/* unknown1 */
	},
	game_pic_parameter[5],
	8
};

static struct game_config kairaku_config = {
	"kairaku",
	{
		4,	/* width */
		0,	/* height */
		2,	/* dib len */
		7,	/* type */
		5,	/* bitmap len */
		3,	/* compr len */
		1,	/* unknown0 */
		6,	/* unknown1 */
	},
	game_pic_parameter[5],
	8
};

static struct game_config yumekoi_config = {
	"yumekoi",
	{
		2,	/* width */
		1,	/* height */
		6,	/* dib len */
		4,	/* type */
		5,	/* bitmap len */
		3,	/* compr len */
		0,	/* unknown0 */
		7,	/* unknown1 */
	},
	game_pic_parameter[0],
	8
};

// 義母の吐息～背徳心に漂う母の色香～
static struct game_config gibo_config = {
	"gibo",
	{
		6,	/* width */
		7,	/* height */
		3,	/* dib len */
		2,	/* type */
		4,	/* bitmap len */
		1,	/* compr len */
		0,	/* unknown0 */
		5,	/* unknown1 */
	},
	game_pic_parameter[5],
	8
};

// 蝶ノ夢
static struct game_config tyou_config = {
	"tyou",
	{
		6,	/* width */
		7,	/* height */
		4,	/* dib len */
		1,	/* type */
		0,	/* bitmap len */
		2,	/* compr len */
		5,	/* unknown0 */
		3,	/* unknown1 */
	},
	game_pic_parameter[4],
	8
};

// 人妻?かすみさん2 ～奥様?未亡人オーナーと共同性活～
static struct game_config kasumi2_config = {
	"kasumi2",
	{
		7,	/* width */
		6,	/* height */
		1,	/* dib len */
		4,	/* type */
		0,	/* bitmap len */
		2,	/* compr len */
		5,	/* unknown0 */
		3,	/* unknown1 */
	},
	game_pic_parameter[3],
	8
};

// はなマルッ！
static struct game_config hanamaru_config = {
	"hanamaru",
	{
		3,	/* width */
		5,	/* height */
		0,	/* dib len */
		4,	/* type */
		2,	/* bitmap len */
		1,	/* compr len */
			/* (none)unknown0 */
			/* (none)unknown1 */
	},
	game_pic_parameter[2],
	6
};

// 蟲惑の刻 web体験版
static struct game_config kowakunotoki_config = {
	"kowakunotoki",
	{
		4,	/* width */
		3,	/* height */
		1,	/* dib len */
		7,	/* type */
		5,	/* bitmap len */
		2,	/* compr len */
		0,	/* unknown0 */
		6,	/* unknown1 */
	},
	game_pic_parameter[0],
	8
};

static struct game_config suzune_config = {
	"suzune",
	{
		0,	/* width */
		1,	/* height */
		5,	/* dib len */
		7,	/* type */
		2,	/* bitmap len */
		6,	/* compr len */
		3,	/* unknown0 */
		4,	/* unknown1 */
	},
	game_pic_parameter[1],
	8
};

// [ぴんくはてな]姫∽神1/2 体験版
static struct game_config himekami_config = {
	"himekami",
	{
		2,	/* width */
		3,	/* height */
		5,	/* dib len */
		7,	/* type */
		4,	/* bitmap len */
		6,	/* compr len */
		0,	/* unknown0 */
		1,	/* unknown1 */
	},
	game_pic_parameter[0],
	8
};

// [WendyBell] 痴漢願望 体験版
static struct game_config tikangan_config = {
	"tikangan",
	{
		3,	/* width */
		6,	/* height */
		0,	/* dib len */
		2,	/* type */
		7,	/* bitmap len */
		1,	/* compr len */
		4,	/* unknown0 */
		5,	/* unknown1 */
	},
	game_pic_parameter[0],
	8
};

// [TinkerBell]孕女 ～俺はこの学園で復讐を成しとげる～ 体験版
static struct game_config harame_config = {
	"harame",
	{
		4,	/* width */
		1,	/* height */
		0,	/* dib len */
		3,	/* type */
		6,	/* bitmap len */
		2,	/* compr len */
		5,	/* unknown0 */
		7,	/* unknown1 */
	},
	game_pic_parameter[0],
	8
};

// [WendyBell]痴漢衝動 体験版
static struct game_config tikansyou_config = {
	"tikansyou",
	{
		3,	/* width */
		5,	/* height */
		0,	/* dib len */
		2,	/* type */
		7,	/* bitmap len */
		1,	/* compr len */
		4,	/* unknown0 */
		6,	/* unknown1 */
	},
	game_pic_parameter[0],
	8
};

// [WendyBell]ご奉仕★プリンセス 体験版
static struct game_config princess_config = {
	"princess",
	{
		0,	/* width */
		1,	/* height */
		5,	/* dib len */
		7,	/* type */
		2,	/* bitmap len */
		6,	/* compr len */
		3,	/* unknown0 */
		4,	/* unknown1 */
	},
	game_pic_parameter[0],
	8
};

// [WendyBell]続?快楽依存症 ～その後の涼子さん～
static struct game_config zoku_kairaku_config = {
	"zoku_kairaku",
	{
		5,	/* width */
		4,	/* height */
		2,	/* dib len */
		3,	/* type */
		7,	/* bitmap len */
		1,	/* compr len */
		0,	/* unknown0 */
		6,	/* unknown1 */
	},
	game_pic_parameter[0],
	8
};

// [TinkerBell]はなマルッ！2 体験版
static struct game_config hanamaru2_config = {
	"hanamaru2",
	{
		6,	/* width */
		5,	/* height */
		4,	/* dib len */
		2,	/* type */
		3,	/* bitmap len */
		1,	/* compr len */
		0,	/* unknown0 */
		7,	/* unknown1 */
	},
	game_pic_parameter[0],
	8
};

// [TinkerBell]イカレタ教室 ～こんなのが当り前の授業風景！？～
static struct game_config ikareta_config = {
	"ikareta",
	{
		4,	/* width */
		3,	/* height */
		2,	/* dib len */
		6,	/* type */
		1,	/* bitmap len */
		5,	/* compr len */
		0,	/* unknown0 */
		7,	/* unknown1 */
	},
	game_pic_parameter[0],
	8
};

// [TinkerBell]兄嫁はいじっぱり Web体験版
static struct game_config aniyomeiji_config = {
	"aniyomeiji",
	{
		0,	/* width */
		1,	/* height */
		5,	/* dib len */
		7,	/* type */
		2,	/* bitmap len */
		6,	/* compr len */
		3,	/* unknown0 */
		4,	/* unknown1 */
	},
	game_pic_parameter[0],
	8
};

static struct game_config *game_list[] = {
	&Ndoki_config,
	&tyouR_config,
	&GYK_config,
	&suzune_config,
	&hanamaru2_config,
	&tikansyou_config,
	&princess_config,
	&aniyomeiji_config,
	&zoku_kairaku_config,
	&ikareta_config,
	&harame_config,
	&tikangan_config,
	&himekami_config,
	&kowakunotoki_config,
	&hanamaru_config,
	&kasumi2_config,
	&tyou_config,
	&gibo_config,
	&yumekoi_config,
	&kairaku_config,
	&inyoutyu_config,
	&inyou_shock_config,
	&hobo_config,
	&koituma_config,
	NULL
};


#if 0
static DWORD current_pic_parameter[8];
static DWORD current_pic_parameter_state;

enum {
	STATE_BEGIN = 0,
	STATE_PROCESS,
	STATE_END
};

//[00000039] 320 258 0 29 23 75300 0 0

// 假定前几张图一定是mask图（800 x 600 x 256）
static void guess_parameter(DWORD para[8], DWORD curbyte, DWORD total_len)
{
	if (current_pic_parameter_state == STATE_END)
		return;

	if (current_pic_parameter_state == STATE_BEGIN) {
		for (DWORD i = 0; i < 8; ++i) {
			if (para[i] == 800)
				current_pic_parameter[WIDTH] = i;
			else if (para[i] == 600)
				current_pic_parameter[HEIGHT] = i;
			else if (para[i] == 800 * 600)
				current_pic_parameter[LENGTH] = i;
		}
		current_pic_parameter[TYPE] = 0;
		current_pic_parameter_state = STATE_PROCESS;
	} else if (current_pic_parameter_state == STATE_PROCESS) {
		for (DWORD i = 0; i < 8; ++i) {
			if (i != current_pic_parameter[WIDTH] && i != current_pic_parameter[HEIGHT]
					&& i != current_pic_parameter[LENGTH]) {
				if (para[i] - para[current_pic_parameter[WIDTH]] * para[current_pic_parameter[HEIGHT]] / 8 <= 1)
					current_pic_parameter[BITMAP_LENGTH] = i;
			}
		}
		for (DWORD i = 0; i < 8; ++i) {
			if (i != current_pic_parameter[WIDTH] && i != current_pic_parameter[HEIGHT]
					&& i != current_pic_parameter[LENGTH] && i != current_pic_parameter[BITMAP_LENGTH]) {
				if (para[i] == 7)
					current_pic_parameter[TYPE] = i;
			}
		}
		for (i = 0; i < 8; ++i) {
			if (i != current_pic_parameter[WIDTH] && i != current_pic_parameter[HEIGHT]
					&& i != current_pic_parameter[LENGTH] && i != current_pic_parameter[TYPE]
					&& i != current_pic_parameter[BITMAP_LENGTH]) {
				if ((curbyte + para[current_pic_parameter[BITMAP_LENGTH]] * 2 +
					para[i] + current_pic_parameter[LENGTH] == total_len) || 
					(!para[i] && (curbyte + para[current_pic_parameter[BITMAP_LENGTH]] * 2 +
						current_pic_parameter[LENGTH] == total_len))
				)
					current_pic_parameter[COMPR_LENGTH] = i;
				else 
					current_pic_parameter[UNKNOWN0] = i;
			}
		}
		for (i = 0; i < 8; ++i) {
			if (i != current_pic_parameter[WIDTH] && i != current_pic_parameter[HEIGHT]
					&& i != current_pic_parameter[LENGTH] && i != current_pic_parameter[TYPE]
					&& i != current_pic_parameter[BITMAP_LENGTH] 
					&& i != current_pic_parameter[COMPR_LENGTH] 
					&& i != current_pic_parameter[UNKNOWN0] 
			) {
				current_pic_parameter[UNKNOWN1] = i;
				break;
			}
		}
		current_pic_parameter_state = STATE_END;
	}
}
#endif

static void *my_malloc(DWORD len)
{
	return new BYTE[len];
}

static DWORD lzss_uncompress(BYTE *uncompr, DWORD uncomprlen, 
							 BYTE *compr, DWORD comprlen)
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
		if (!(flag & 0x0100)) {
			if (curbyte == comprlen)
				break;
			flag = compr[curbyte++] | 0xff00;
		}

		if (flag & 1) {
			unsigned char data;

			if (curbyte == comprlen)
				break;

			data = compr[curbyte++];
			if (act_uncomprlen != uncomprlen)
				uncompr[act_uncomprlen++] = data;
			/* 输出的1字节放入滑动窗口 */
			win[nCurWindowByte++] = data;
			nCurWindowByte &= win_size - 1;
		} else {
			unsigned int copy_bytes, win_offset;
			unsigned int i;

			if (curbyte == comprlen)
				break;
			win_offset = compr[curbyte++];

			if (curbyte == comprlen)
				break;
			copy_bytes = compr[curbyte++];
			win_offset |= (copy_bytes & 0xf0) << 4;
			copy_bytes &= 0x0f;
			copy_bytes += 3;

			for (i = 0; i < copy_bytes; i++) {	
				unsigned char data;

				data = win[(win_offset + i) & (win_size - 1)];
				if (act_uncomprlen != uncomprlen)
					uncompr[act_uncomprlen++] = data;
				/* 输出的1字节放入滑动窗口 */
				win[nCurWindowByte++] = data;
				nCurWindowByte &= win_size - 1;	
			}
		}
	}

	return act_uncomprlen;
}

/********************* Arc00.dat *********************/

/* 封包匹配回调函数 */
static int Csystem_Arc00_match(struct package *pkg)
{
	if (lstrcmp(pkg->name, _T("Arc00.dat")) && lstrcmp(pkg->name, _T("data.00")))
		return -CUI_EMATCH;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包索引目录提取函数 */
static int Csystem_Arc00_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	dat_header_t dat_header;
	if (pkg->pio->read(pkg, &dat_header, sizeof(dat_header)))
		return -CUI_EREAD;

	DWORD uncomprlen = 0;
	if (dat_header.uncomprlen[0] != 0xff)
		uncomprlen = 10000000 * (dat_header.uncomprlen[0] ^ 0x7f);
	if (dat_header.uncomprlen[1] != 0xff)
		uncomprlen += 1000000 * (dat_header.uncomprlen[1] ^ 0x7f);
    if (dat_header.uncomprlen[2] != 0xff)
		uncomprlen += 100000 * (dat_header.uncomprlen[2] ^ 0x7f);
    if (dat_header.uncomprlen[3] != 0xff)
		uncomprlen += 10000 * (dat_header.uncomprlen[3] ^ 0x7f);
    if (dat_header.uncomprlen[4] != 0xff)
		uncomprlen += 1000 * (dat_header.uncomprlen[4] ^ 0x7f);
    if (dat_header.uncomprlen[5] != 0xff)
		uncomprlen += 100 * (dat_header.uncomprlen[5] ^ 0x7f);
    if (dat_header.uncomprlen[6] != 0xff)
		uncomprlen += 10 * (dat_header.uncomprlen[6] ^ 0x7f);
    if (dat_header.uncomprlen[7] != 0xff)
		uncomprlen += 1 * (dat_header.uncomprlen[7] ^ 0x7f);

	DWORD comprlen = 0;
	if (dat_header.comprlen[0] != 0xff)
		comprlen = 10000000 * (dat_header.comprlen[0] ^ 0x7f);
	if (dat_header.comprlen[1] != 0xff)
		comprlen += 1000000 * (dat_header.comprlen[1] ^ 0x7f);
    if (dat_header.comprlen[2] != 0xff)
		comprlen += 100000 * (dat_header.comprlen[2] ^ 0x7f);
    if (dat_header.comprlen[3] != 0xff)
		comprlen += 10000 * (dat_header.comprlen[3] ^ 0x7f);
    if (dat_header.comprlen[4] != 0xff)
		comprlen += 1000 * (dat_header.comprlen[4] ^ 0x7f);
    if (dat_header.comprlen[5] != 0xff)
		comprlen += 100 * (dat_header.comprlen[5] ^ 0x7f);
    if (dat_header.comprlen[6] != 0xff)
		comprlen += 10 * (dat_header.comprlen[6] ^ 0x7f);
    if (dat_header.comprlen[7] != 0xff)
		comprlen += 1 * (dat_header.comprlen[7] ^ 0x7f);

	if (pkg->pio->seek(pkg, comprlen, IO_SEEK_CUR))
		return -CUI_ESEEK;

	u8 length_str[8];
	if (pkg->pio->read(pkg, length_str, 8))
		return -CUI_EREAD;
	DWORD length = 0;
	if (length_str[0] != 0xff)
		length = 10000000 * (length_str[0] ^ 0x7f);
	if (length_str[1] != 0xff)
		length += 1000000 * (length_str[1] ^ 0x7f);
    if (length_str[2] != 0xff)
		length += 100000 * (length_str[2] ^ 0x7f);
    if (length_str[3] != 0xff)
		length += 10000 * (length_str[3] ^ 0x7f);
    if (length_str[4] != 0xff)
		length += 1000 * (length_str[4] ^ 0x7f);
    if (length_str[5] != 0xff)
		length += 100 * (length_str[5] ^ 0x7f);
    if (length_str[6] != 0xff)
		length += 10 * (length_str[6] ^ 0x7f);
    if (length_str[7] != 0xff)
		length += 1 * (length_str[7] ^ 0x7f);
	DWORD length0 = length;
	if (pkg->pio->seek(pkg, length0, IO_SEEK_CUR))
		return -CUI_ESEEK;	

	if (pkg->pio->read(pkg, length_str, 8))
		return -CUI_EREAD;
	length = 0;
	if (length_str[0] != 0xff)
		length = 10000000 * (length_str[0] ^ 0x7f);
	if (length_str[1] != 0xff)
		length += 1000000 * (length_str[1] ^ 0x7f);
    if (length_str[2] != 0xff)
		length += 100000 * (length_str[2] ^ 0x7f);
    if (length_str[3] != 0xff)
		length += 10000 * (length_str[3] ^ 0x7f);
    if (length_str[4] != 0xff)
		length += 1000 * (length_str[4] ^ 0x7f);
    if (length_str[5] != 0xff)
		length += 100 * (length_str[5] ^ 0x7f);
    if (length_str[6] != 0xff)
		length += 10 * (length_str[6] ^ 0x7f);
    if (length_str[7] != 0xff)
		length += 1 * (length_str[7] ^ 0x7f);
	DWORD length1 = length;

	if (pkg->pio->seek(pkg, 16, IO_SEEK_SET))
		return -CUI_ESEEK;

	BYTE *compr = new BYTE[comprlen];
	if (!compr)
		return -CUI_EMEM;
	if (pkg->pio->read(pkg, compr, comprlen)) {
		delete [] compr;
		return -CUI_EREAD;
	}

	BYTE *uncompr = new BYTE[uncomprlen + length0 + length1 + 12];
	if (!uncompr) {
		delete [] compr;
		return -CUI_EMEM;
	}

	DWORD act_uncomprlen = lzss_uncompress(uncompr + 4, uncomprlen, 
		compr, comprlen);
	delete [] compr;
	if (act_uncomprlen != uncomprlen) {
		delete [] uncompr;
		return -CUI_EUNCOMPR;
	}

	*(u32 *)uncompr = uncomprlen;

	if (pkg->pio->seek(pkg, 8, IO_SEEK_CUR)) {
		delete [] uncompr;
		return -CUI_ESEEK;
	}
	*(u32 *)(uncompr + 4 + uncomprlen) = length0;
	if (pkg->pio->read(pkg, uncompr + 4 + uncomprlen + 4, length0)) {
		delete [] uncompr;
		return -CUI_EREAD;
	}

	if (pkg->pio->seek(pkg, 8, IO_SEEK_CUR)) {
		delete [] uncompr;
		return -CUI_ESEEK;
	}
	*(u32 *)(uncompr + 4 + uncomprlen + 4 + length0) = length1;
	if (pkg->pio->read(pkg, uncompr + 4 + uncomprlen + 4 + length0 + 4, length1)) {
		delete [] uncompr;
		return -CUI_EREAD;
	}
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen + length0 + length1 + 12;

	return 0;
}

/* 资源保存函数 */
static int Csystem_Arc00_save_resource(struct resource *res, 
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
static void Csystem_Arc00_release_resource(struct package *pkg, 
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
static void Csystem_Arc00_release(struct package *pkg, 
								  struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg->lst);
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation Csystem_Arc00_operation = {
	Csystem_Arc00_match,			/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	Csystem_Arc00_extract_resource,	/* extract_resource */
	Csystem_Arc00_save_resource,	/* save_resource */
	Csystem_Arc00_release_resource,	/* release_resource */
	Csystem_Arc00_release			/* release */
};

/********************* dat *********************/

/* 封包匹配回调函数 */
static int Csystem_dat_match(struct package *pkg)
{
	if (!lstrcmpi(pkg->name, _T("Arc00.dat")) || !lstrcmpi(pkg->name, _T("data.00")))
		return -CUI_EMATCH;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->open(pkg->lst, IO_READONLY)) {
		pkg->pio->close(pkg);
		return -CUI_EOPEN;
	}

	const char *game = get_options("game");
	if (game) {
		struct game_config **cfg = game_list;
		for (; *cfg; ++cfg) {
			if (!strcmpi((*cfg)->name, game))
				break;
		}
		if (!*cfg) {
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}
		package_set_private(pkg, *cfg);
	} else
		package_set_private(pkg, NULL);

	return 0;	
}

/* 封包索引目录提取函数 */
static int Csystem_dat_extract_directory(struct package *pkg,
										 struct package_directory *pkg_dir)
{
	dat_header_t dat_header;
	if (pkg->pio->read(pkg->lst, &dat_header, sizeof(dat_header)))
		return -CUI_EREAD;

	DWORD uncomprlen = 0;
	if (dat_header.uncomprlen[0] != 0xff)
		uncomprlen = 10000000 * (dat_header.uncomprlen[0] ^ 0x7f);
	if (dat_header.uncomprlen[1] != 0xff)
		uncomprlen += 1000000 * (dat_header.uncomprlen[1] ^ 0x7f);
    if (dat_header.uncomprlen[2] != 0xff)
		uncomprlen += 100000 * (dat_header.uncomprlen[2] ^ 0x7f);
    if (dat_header.uncomprlen[3] != 0xff)
		uncomprlen += 10000 * (dat_header.uncomprlen[3] ^ 0x7f);
    if (dat_header.uncomprlen[4] != 0xff)
		uncomprlen += 1000 * (dat_header.uncomprlen[4] ^ 0x7f);
    if (dat_header.uncomprlen[5] != 0xff)
		uncomprlen += 100 * (dat_header.uncomprlen[5] ^ 0x7f);
    if (dat_header.uncomprlen[6] != 0xff)
		uncomprlen += 10 * (dat_header.uncomprlen[6] ^ 0x7f);
    if (dat_header.uncomprlen[7] != 0xff)
		uncomprlen += 1 * (dat_header.uncomprlen[7] ^ 0x7f);

	DWORD comprlen = 0;
	if (dat_header.comprlen[0] != 0xff)
		comprlen = 10000000 * (dat_header.comprlen[0] ^ 0x7f);
	if (dat_header.comprlen[1] != 0xff)
		comprlen += 1000000 * (dat_header.comprlen[1] ^ 0x7f);
    if (dat_header.comprlen[2] != 0xff)
		comprlen += 100000 * (dat_header.comprlen[2] ^ 0x7f);
    if (dat_header.comprlen[3] != 0xff)
		comprlen += 10000 * (dat_header.comprlen[3] ^ 0x7f);
    if (dat_header.comprlen[4] != 0xff)
		comprlen += 1000 * (dat_header.comprlen[4] ^ 0x7f);
    if (dat_header.comprlen[5] != 0xff)
		comprlen += 100 * (dat_header.comprlen[5] ^ 0x7f);
    if (dat_header.comprlen[6] != 0xff)
		comprlen += 10 * (dat_header.comprlen[6] ^ 0x7f);
    if (dat_header.comprlen[7] != 0xff)
		comprlen += 1 * (dat_header.comprlen[7] ^ 0x7f);

	BYTE *compr = new BYTE[comprlen];
	if (!compr)
		return -CUI_EMEM;
	if (pkg->pio->read(pkg->lst, compr, comprlen)) {
		delete [] compr;
		return -CUI_EREAD;
	}

	BYTE *uncompr = new BYTE[uncomprlen];
	if (!uncompr) {
		delete [] compr;
		return -CUI_EMEM;
	}

	DWORD act_uncomprlen = lzss_uncompress(uncompr, uncomprlen, 
		compr, comprlen);
	delete [] compr;
	if (act_uncomprlen != uncomprlen) {
		delete [] uncompr;
		return -CUI_EUNCOMPR;
	}

	BYTE *p = uncompr;
	for (DWORD index_entries = 0; p < uncompr + uncomprlen; ++index_entries)
		p += *(u32 *)p + 4;

	dat_entry_t *index_buffer = new dat_entry_t[index_entries];
	if (!index_buffer) {
		delete [] uncompr;
		return -CUI_EMEM;
	}
		
	p = uncompr;
	for (DWORD i = 0; i < index_entries; ++i) {
		dat_entry_t &entry = index_buffer[i];
		u32 entry_size = *(u32 *)p;
		p += 4;
		BYTE *p_entry = p;
		p += entry_size;
		u32 res_id = *(u32 *)p_entry;
		p_entry += 4;
		sprintf(entry.name, "%08d", res_id);
		entry.uncomprlen = *(u32 *)p_entry;
		p_entry += 4;
		entry.comprlen = *(u32 *)p_entry;
		p_entry += 4;
		entry.offset = *(u32 *)p_entry;
		p_entry += 4;

		// unknown 6 bytes
		if (Debug) {
			printf("%s: %x %x %x %x %x %x\n", entry.name, 
				p_entry[0],p_entry[1],p_entry[2],p_entry[3],
				p_entry[4],p_entry[5]);
		}

		entry.type = *p_entry++;
		entry.unknown = *p_entry++;
		/* Arc05.dat(picture)
		 * 一般24bits图 -> 62 30 ff ff ff ff
		 * 位置索引数据(后来的系统版本TinkerBell版) -> 63 30 ff ff ff ff
		 * mask picture(256色) -> 6e 30 ff ff ff ff
		 * mask取景图+位置索引数据(早一点的WendyBell版)(256色) -> 6f 30 ff ff ff ff
		 * ？？ -> 62 0 ff ff ff ff
		 */
		/* Arc04.dat(script)
			61 30 ff ff ff ff
			65 30 ff ff ff ff
			66 30 ff ff ff ff
			6c 30 ff ff ff ff
			6d 30 ff ff ff ff
			70 30 ff ff ff ff
			72 30 ff ff ff ff
			74 30 ff ff ff ff
			76 30 ff ff ff ff
		 */
		/* Arc06.dat(music)
			external bgm file name -> 68 30 ff ff ff ff
			external mpg file name -> 69 30 ff ff ff ff
			voice -> 6a 30 ff ff ff ff
			se -> 6b 30 ff ff ff ff
			bgm -> 75 30 ff ff ff ff
		 */
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = sizeof(dat_entry_t) * index_entries;
	pkg_dir->index_entry_length = sizeof(dat_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int Csystem_dat_parse_resource_info(struct package *pkg,
										   struct package_resource *pkg_res)
{
	dat_entry_t *dat_entry;

	dat_entry = (dat_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, dat_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = dat_entry->comprlen;
	pkg_res->actual_data_length = dat_entry->uncomprlen == dat_entry->comprlen ? 0 : dat_entry->uncomprlen;
	pkg_res->offset = dat_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int Csystem_dat_extract_resource(struct package *pkg,
										struct package_resource *pkg_res)
{
	BYTE *compr = new BYTE[pkg_res->raw_data_length];
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			delete [] compr;
			return -CUI_EREADVEC;
	}

	BYTE *act_data;
	DWORD act_data_len;
	if (pkg_res->actual_data_length) {
		BYTE *uncompr = new BYTE[pkg_res->actual_data_length];
		if (!uncompr) {
			delete [] compr;
			return -CUI_EMEM;
		}
		lzss_uncompress(uncompr, pkg_res->actual_data_length,
			compr, pkg_res->raw_data_length);
		delete [] compr;
		pkg_res->actual_data = uncompr;
		act_data = uncompr;
		act_data_len = pkg_res->actual_data_length;
	} else {
		pkg_res->raw_data = compr;
		act_data = compr;
		act_data_len = pkg_res->raw_data_length;
	}

	if (pkg_res->flags & PKG_RES_FLAG_RAW)
		return 0;

	dat_entry_t *dat_entry = (dat_entry_t *)pkg_res->actual_index_entry;
	if ((dat_entry->type == 0x62) || (dat_entry->type == 0x6e) || (dat_entry->type == 0x6f)) {	// "Arc05.dat"
		if (dat_entry->unknown == 0x00) {
			pkg_res->actual_data = act_data;
			pkg_res->actual_data_length = act_data_len;
			return 0;
		}

		if (act_data[0] == 'a') {
			struct game_config *cfg = (struct game_config *)package_get_private(pkg);
			DWORD dib_len, compr_dib_len, type, width, height, 
				unknown2, unknown3, bitmap_len;
			BYTE step[3] = { 0, 0, 0 };
			DWORD curbyte = 2;
			DWORD para[8];

			if (!cfg)
				return 0;

			for (DWORD k = 0; k < cfg->parameter_number; ++k) {
				step[0] = act_data[curbyte++];
				if (step[0] == cfg->pic_parameter[2])
					step[0] = 0;
				BYTE tmp = act_data[curbyte++];
				while (tmp != cfg->pic_parameter[1]) {
					if (tmp == cfg->pic_parameter[0])
						++step[2];
					else if (tmp == cfg->pic_parameter[2])
						step[1] = 0;
					else
						step[1] = tmp;
					tmp = act_data[curbyte++];
				}
				DWORD val = step[2] * cfg->pic_parameter[0] * cfg->pic_parameter[0] 
					+ step[1] * cfg->pic_parameter[0] + step[0];
				step[1] = 0;
				step[2] = 0;
				para[k] = val;
			}
			
			dib_len = para[cfg->parameter[LENGTH]];
			compr_dib_len = para[cfg->parameter[COMPR_LENGTH]];
			type = para[cfg->parameter[TYPE]];
			width = para[cfg->parameter[WIDTH]];
			height = para[cfg->parameter[HEIGHT]];
			unknown2 = para[cfg->parameter[UNKNOWN0]];
			unknown3 = para[cfg->parameter[UNKNOWN1]];
			bitmap_len = para[cfg->parameter[BITMAP_LENGTH]];

			if (Debug) {
				printf("[%s] %x / %x: len %x, width %x, height %x, clen %x type %x, blen %x, "
					"%x, %x\n",
				   pkg_res->name, curbyte, act_data_len, 
				   dib_len, width, height, compr_dib_len,type, bitmap_len, 
				   unknown2, unknown3);
			}

			BYTE mask[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };
			BYTE *bitmap0 = act_data + curbyte;
			BYTE *bitmap1 = bitmap0 + bitmap_len;
			BYTE *src0 = bitmap1 + bitmap_len;
			BYTE *rgb;
			BYTE *mask_dib = NULL;
			if ((type & 4) && (type & 2)) {
				rgb = new BYTE[width * height * 3];
				if (!rgb) {
					delete [] act_data;
					return -CUI_EMEM;
				}
				mask_dib = new BYTE[width * height * 3];
				if (!mask_dib) {
					delete [] rgb;
					delete [] act_data;
					return -CUI_EMEM;
				}

				BYTE *src1 = src0 + compr_dib_len;
				BYTE *__rgb = rgb;
				BYTE *__mask_dib = mask_dib;
				BYTE *org_src0 = src0;
				BYTE *org_src1 = src1;
				// 每像素用1位编码
				for (DWORD i = 0; i < width * height; ++i) {
					DWORD mi = i & 7;
					DWORD bi = i / 8;
					BYTE flag0 = mask[mi] & bitmap1[bi];
					BYTE flag1;
					if (flag0)
						flag1 = 0;
					else
						flag1 = !!(mask[mi] & bitmap0[bi]);

					BYTE tmp;
					for (DWORD p = 0; p < 3; ++p) {
						if (flag0 || flag1)
							tmp = *src1++;
						else
							tmp = 0;
						*__rgb++ = tmp;
					}
					for (p = 0; p < 3; ++p) {
						if (flag0)
							tmp = *src0++;
						else if (flag1)
							tmp = 0xff;
						else
							tmp = 0;
						*__mask_dib++ = tmp;
					}
				}

#if 0
				if (MyBuildBMPFile(mask_dib, width * height * 3,
						NULL, 0, width, height, 24,
						(BYTE **)&pkg_res->actual_data,
						&pkg_res->actual_data_length, my_malloc)) {
					delete [] act_data;
					return -CUI_EMEM;
				}
				delete [] rgb;				
#else
				BYTE *rgba_dib = new BYTE[width * height * 4];
				if (!rgba_dib) {
					delete [] mask_dib;
					delete [] rgb;
					delete [] act_data;
					return -CUI_EMEM;
				}

				__rgb = rgb;
				__mask_dib = mask_dib;
				BYTE *__pixel = rgba_dib;
				for (DWORD k = 0; k < width * height; ++k) {
					BYTE b = *__rgb++;
					BYTE g = *__rgb++;
					BYTE r = *__rgb++;
					BYTE a = *__mask_dib++;
					//*__pixel++ = b * (255 - a) / 255;
					*__pixel++ = b;
					a = *__mask_dib++;
					//*__pixel++ = g * (255 - a) / 255;
					*__pixel++ = g;
					a = *__mask_dib++;
					//*__pixel++ = r * (255 - a) / 255;
					*__pixel++ = r;
					*__pixel++ = a;
				}

				delete [] mask_dib;
				delete [] rgb;

				if (MyBuildBMPFile(rgba_dib, width * height * 4,
						NULL, 0, width, height, 32,
						(BYTE **)&pkg_res->actual_data,
						&pkg_res->actual_data_length, my_malloc)) {
					delete [] act_data;
					return -CUI_EMEM;
				}
				delete [] rgba_dib;
#endif
			} else if (!(type & 4) && (type & 2)) {
				rgb = new BYTE[width * height * 3];
				if (!rgb) {
					delete [] act_data;
					return -CUI_EMEM;
				}
				BYTE *__rgb = rgb;
				// 每像素用1位编码
				for (DWORD i = 0; i < width * height; ++i) {
					DWORD mi = i & 7;
					DWORD bi = i / 8;
					BYTE flag0 = mask[mi] & bitmap1[bi];
					BYTE flag1;
					if (flag0)
						flag1 = 0;
					else
						flag1 = !!(mask[mi] & bitmap0[bi]);

					BYTE tmp;
					for (DWORD p = 0; p < 3; ++p) {
						if (flag1)
							tmp = *src0++;
						else
							tmp = 0;
						*__rgb++ = tmp;
					}
				}
				if (MyBuildBMPFile(rgb, width * height * 3,
						NULL, 0, width, height, 24,
						(BYTE **)&pkg_res->actual_data,
						&pkg_res->actual_data_length, my_malloc)) {
					delete [] rgb;
					delete [] act_data;
					return -CUI_EMEM;
				}
				delete [] rgb;
			} else if (type == 0x00) {
				// fix: [00000475] 1db / 1758adb: len a17800, width 320, height 27d8, clen 0 type 0, blen 0, 166, d0
				// the len field is invalid
				//if (MyBuildBMPFile(act_data + curbyte, dib_len,
				dib_len = act_data_len - curbyte;
				if (MyBuildBMPFile(act_data + curbyte, dib_len, NULL, 0, 
					width, height, dib_len / width / height * 8,
						(BYTE **)&pkg_res->actual_data,
						&pkg_res->actual_data_length, my_malloc)) {
					delete [] act_data;
					return -CUI_EMEM;
				}			
			} else {	// [00001223] 19 / 5cef: len 0, width 12a, height 13f, clen 0 type 1, blen 2e6b, 1a1, fe
				pkg_res->actual_data = act_data;
				pkg_res->actual_data_length = act_data_len;
				return 0;
			}
			delete [] act_data;
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
			pkg_res->replace_extension = _T(".bmp");
		} else if (act_data[0] == 'b') {
			printf("[%s] pic type b\n",   pkg_res->name);exit(0);
		}
	} else if (!memcmp(act_data, "Tink", 4)) {	// "Arc06.dat"
		const char *key = "DBB3206F-F171-4885-A131-EC7FBA6FF491 Copyright 2004 Cyberworks \"TinkerBell\"., all rights reserved.";
		DWORD key_len = strlen(key) + 1;
		BYTE *p = act_data + 4;
		for (DWORD i = 0, ki = 0; i < 0xE1B; ++i) {
			p[i] ^= key[ki];
			++ki;
			if (ki == key_len)
				ki = 1;
		}
		act_data[0] = 'O';
		act_data[1] = 'g';
		act_data[2] = 'g';
		act_data[3] = 'S';
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".ogg");
	} else if (!memcmp(act_data, "Song", 4)) {	// "data.06"
		const char *key = "49233ED4911E48c68EBF1DDACE3A7752A8B52D3D13C34e509FBE-E3EFDE3F2D61";
		DWORD key_len = strlen(key) + 1;
		BYTE *p = act_data + 4;
		for (DWORD i = 0, ki = 0; i < 0xE1B; ++i) {
			p[i] ^= key[ki];
			++ki;
			if (ki == key_len)
				ki = 1;
		}
		act_data[0] = 'O';
		act_data[1] = 'g';
		act_data[2] = 'g';
		act_data[3] = 'S';
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".ogg");
	} else if (!memcmp(act_data, "RIFF", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".wav");
	} else if (!memcmp(act_data, "OggS", 4)) {	// "Arc06.dat"
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".ogg");
	}

	return 0;
}

static cui_ext_operation Csystem_dat_operation = {
	Csystem_dat_match,				/* match */
	Csystem_dat_extract_directory,	/* extract_directory */
	Csystem_dat_parse_resource_info,/* parse_resource_info */
	Csystem_dat_extract_resource,	/* extract_resource */
	Csystem_Arc00_save_resource,	/* save_resource */
	Csystem_Arc00_release_resource,	/* release_resource */
	Csystem_Arc00_release			/* release */
};

/********************* dat *********************/

/* 封包匹配回调函数 */
static int Csystem_old_dat_match(struct package *pkg)
{
	if (lstrcmpi(pkg->lst->name, _T("head.dat")))
		return -CUI_EMATCH;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->open(pkg->lst, IO_READONLY)) {
		pkg->pio->close(pkg);
		return -CUI_EOPEN;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int Csystem_old_dat_extract_directory(struct package *pkg,
											 struct package_directory *pkg_dir)
{
	u32 index_entries;

	if (pkg->pio->read(pkg->lst, &index_entries, sizeof(index_entries)))
		return -CUI_EREAD;

	/*
      pkg_id == 0: arc.dat or arc2.dat
      pkg_id != 0: arcX2.dat
      2由is_sub_pkg标志字段决定; X由pkg_id字段决定
     */
	for (DWORD i = 0; i < index_entries; ++i) {
		u32 res_id, offset, length;
		u8 pkg_id, is_sub_pkg;

		if (pkg->pio->read(pkg->lst, &res_id, sizeof(res_id)))
			break;

		if (pkg->pio->read(pkg->lst, &offset, sizeof(offset)))
			break;

		if (pkg->pio->read(pkg->lst, &length, sizeof(length)))
			break;

		if (pkg->pio->read(pkg->lst, &pkg_id, sizeof(pkg_id)))
			break;

		if (pkg->pio->read(pkg->lst, &is_sub_pkg, sizeof(is_sub_pkg)))
			break;

		printf("[%x]: %x %x %x pkg_id: %x %x\n", i, res_id, offset, length,
			pkg_id, is_sub_pkg);
	}
	exit(0);
#if 0
	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = sizeof(dat_entry_t) * index_entries;
	pkg_dir->index_entry_length = sizeof(dat_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;
#endif
	return 0;
}

/* 封包索引项解析函数 */
static int Csystem_old_dat_parse_resource_info(struct package *pkg,
										   struct package_resource *pkg_res)
{
	dat_entry_t *dat_entry;

	dat_entry = (dat_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, dat_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = dat_entry->comprlen;
	pkg_res->actual_data_length = dat_entry->uncomprlen == dat_entry->comprlen ? 0 : dat_entry->uncomprlen;
	pkg_res->offset = dat_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int Csystem_old_dat_extract_resource(struct package *pkg,
										struct package_resource *pkg_res)
{
	BYTE *compr = new BYTE[pkg_res->raw_data_length];
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			delete [] compr;
			return -CUI_EREADVEC;
	}

	return 0;
}

static cui_ext_operation Csystem_old_dat_operation = {
	Csystem_old_dat_match,				/* match */
	Csystem_old_dat_extract_directory,	/* extract_directory */
	Csystem_old_dat_parse_resource_info,/* parse_resource_info */
	Csystem_old_dat_extract_resource,	/* extract_resource */
	Csystem_Arc00_save_resource,	/* save_resource */
	Csystem_Arc00_release_resource,	/* release_resource */
	Csystem_Arc00_release			/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK Csystem_register_cui(struct cui_register_callback *callback)
{	
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &Csystem_old_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_LST | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), _T(".dump"), 
		NULL, &Csystem_Arc00_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".00"), NULL, 
		NULL, &Csystem_Arc00_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &Csystem_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_LST | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".04"), NULL, 
		NULL, &Csystem_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_LST | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".05"), NULL, 
		NULL, &Csystem_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_LST | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".06"), NULL, 
		NULL, &Csystem_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_LST | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	if (get_options("debug"))
		Debug = 1;
	else
		Debug = 0;

	return 0;
}
