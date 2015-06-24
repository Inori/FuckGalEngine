#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>
#include <math.h>
#include <utility.h>
#include <zlib.h>
#include "port.h"
#include "qsmodel.h"
#include "rangecod.h"

/*
 脚本系统是2字节命令后接参数（fd是ReadREG)

  i是入口参数
((_Z000>>({i}*8))&0xff)*_Z001f+_Z002f

({b[0]}<<24)|(({b[0]}&0xff00)<<8)|(({b[0]}&0xff0000)>>8)|({b[0]}>>24)

 */
/* examples:
WARC 1.2:
Q:\Program Files\foster\壴偺婰壇 傢傫丒偮偆丒偡傝乣

WARC 1.3:
Q:\Program Files\BeF\PIANO
Q:\Program Files\foster\僔僇僄僔

WARC 1.4:
Q:\Program Files\foster\俽俿俙俧俤

WARC 1.5:
Q:\Program Files\LUCHA!\ONE仚BOKU懱尡斉

WARC 1.6:
K:\Program Files\lucha両\亀ONE仚BOKU亁

WARC1.7：
Q:\Program Files\LUCHA\戙乆栘恖嵢愱栧妛堾丂恖嵢忣帠懱尡斉
Q:\Program Files\Guilty\亀椫悌亁懱尡斉

 */
/*
E:\[trial]\[WAR}長靴をはいたデコ 体験版 (LOSTSCRIPT).zip
H:\trial\[WAR]ツイ☆てる C_drive
H:\trial\[WARC]CCホスピタル.exe
H:\trial\[WARC]Love☆Drops.zip
H:\trial\[WARC]ぷり_プリ ～PRINCE×PRINCE～.exe
H:\document\浮点数\maiden\亀壋彈骧鏦梀媃懱尡斉亁
J:\Program Files\Future-Digi\CLEAVAGE
K:\[WAR]どこでもすきしていつでもすきして[trial].zip
m:\[めろめろキュート][WAR][trial]ミンナノウタ.zip
M:\trial\[Gracious][WAR]恥辱皇宮 ～穢された王族～.zip
M:\[trial]\[WAR]ドラクリウス プレ体験版 (めろめろキュート).zip
M:\[trial]\[WAR]ドラクリウス 体験版 (めろめろキュート).zip
M:\[trial]\[WAR]ドラクリウス 体験版ver1.01 (めろめろキュート) .zip
M:\[trial]\[椎名里緒]つくとり 体験版 (ruf).exe
M:\[trial]\[椎名里緒]蠅声の王 [サバエノオウ] 体験版 (LOSTSCRIPT) .zip
q:\[Mischievous][war]つよいもよわいも 強妹×弱妹.zip
Q:\【WAR】
Q:\U\[WAR]つくして！？ Myシスターズ すもも .zip
*/

/*
ruf
検索結果（ブランド名） [消す]
?ruf
未定
奴隷市場II
2007/05/25
つくとり
2007/04/20
螺旋回廊 復刻版
2006/03/10
うつくしひめ 廉価版
奴隷市場 ルネッサンス 廉価版
2005/12/22
ユメミルクスリ
2003/08/08
セイレムの魔女たち
2003/07/31
背徳 DVDPG DVDPG
2003/04/25
生贄の教室
2002/10/25
羞中恥療室
2002/06/07
背徳
2001/12/14
螺旋回廊2
奴隷市場 Renaissance (DVD 完全版)
2001/08/03
傀儡の教室 ～HAPPYEND～	
2001/06/15
独占
2001/05/18
贖罪の教室 ～BADEND～
2000/12/22
奴隷市場
2000/11/10
傀儡の教室
2000/08/25
いなおり
2000/04/14
贖罪の教室	.PD FlyingShine
2000/01/14
螺旋回廊	.rio
1999/12/03
POW ～捕虜～ .PD FlyingShine
*/

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information RioShiina_cui_information = {
	_T("Y.Yamada/STUDIO よしくん"),	/* copyright */
	_T("椎名里緒"),					/* system */
	_T(".WAR .WAA .S25 .MI4 .OGV"),	/* package */
	_T("0.4.1"),					/* revision */
	_T("痴汉公贼"),					/* author */
	_T("2009-2-15 10:49"),			/* date */
	NULL,							/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[8];		// "WARC 1.X(1-7)"
	u32 index_offset;
} WAR_header_t;

typedef struct {
	s8 name[16];		// NULL terminated
	u32 offset;
	u32 comprlen;
	u32 uncomprlen;
	FILETIME time_stamp;
	u32 flags;			// bit 31 - need decrypt(), low bytes: 0 - plain; 1 - YLZ; 2 - YH1; 3 - YPK]
} WARC_entry_t;

typedef struct {
	s8 magic[4];		// "S25"
	u32 index_entries;
} S25_header_t;

typedef struct {
	s32 width;
	s32 height;
	s32 orig_x;
	s32 orig_y;
	s32 reserved;		// 0
} S25_info_t;

typedef struct {
	s8 magic[4];		// "MAI4"
	u32 reserved;		// 0
	s32 width;
	s32 height;
} MI4_header_t;
#pragma pack ()

typedef struct {
	char name[MAX_PATH];
	DWORD length;
	DWORD offset;
} my_s25_entry_t;


static void *my_malloc(DWORD len)
{
	return malloc(len);
}


static DWORD inline warc_max_index_length(unsigned long WARC_version)
{
	DWORD entry_size, max_index_entries;

	if (WARC_version < 150) {
		entry_size = sizeof(WARC_entry_t);
		max_index_entries = 8192;
	} else {
		entry_size = sizeof(WARC_entry_t);
		max_index_entries = 16384;
	}
	return entry_size * max_index_entries;
}

static char WARC_key[MAX_PATH];
static inline const char *WARC_key_string(DWORD WARC_version)
{
	const char *key = "Crypt Type %s - Copyright(C) 2000 Y.Yamada/STUDIO 傛偟偔傫";

	if (WARC_version <= 120)
		sprintf(WARC_key, key, "20000823");
	else
		sprintf(WARC_key, key, "20011002");

	return WARC_key;
}

/********************* compresion staffs *********************/

static DWORD rng_decompress(BYTE *out, BYTE *in, DWORD in_len)
{
	qsmodel qsm;
	rangecoder rc;
	int ch, syfreq, ltfreq;
	DWORD act_uncomprlen = 0;

	/* make an alphabet with 257 symbols, use 256 as end-of-file */
	initqsmodel(&qsm, 257, 12, 2000, NULL, 0);
	/* unknown crypt_index[0], seems to be always 0x00 */
	start_decoding(&rc, in + 1);

    while (1) {
        ltfreq = decode_culshift(&rc, 12);
        ch = qsgetsym(&qsm, ltfreq);
        if (ch == 256)  /* check for end-of-file */
            break;
        out[act_uncomprlen++] = ch;
        qsgetfreq(&qsm, ch, &syfreq, &ltfreq);
        decode_update(&rc, syfreq, ltfreq, 1 << 12);
        qsupdate(&qsm, ch);
	}
    qsgetfreq(&qsm, 256, &syfreq, &ltfreq);
    decode_update(&rc, syfreq, ltfreq, 1 << 12);
    done_decoding(&rc);
    deleteqsmodel(&qsm);

	return act_uncomprlen;
}

struct huffman_state {
	BYTE *in;
	DWORD curbits;
	DWORD cache;
	DWORD index;
	DWORD left[511];
	DWORD right[511];
};

static DWORD huffman_get_bits(struct huffman_state *hstat, DWORD req_bits)
{
	DWORD ret_val = 0;

	if (req_bits > hstat->curbits) {
		do {
			req_bits -= hstat->curbits;
			ret_val |= (hstat->cache & ((1 << hstat->curbits) - 1)) << req_bits;
			hstat->cache = *(u32 *)hstat->in;
			hstat->in += 4;
			hstat->curbits = 32;
		} while (req_bits > 32);
	}
	hstat->curbits -= req_bits;
	return ret_val | ((1 << req_bits) - 1) & (hstat->cache >> hstat->curbits);
}

static DWORD huffman_create_tree(struct huffman_state *hstat)
{
	u32 not_leaf;

	if (hstat->curbits-- < 1) {
		hstat->curbits = 31;
		hstat->cache = *(u32 *)hstat->in;
		hstat->in += 4;
		not_leaf = hstat->cache >> 31;
	} else
		not_leaf = (hstat->cache >> hstat->curbits) & 1;

	DWORD index;
	if (not_leaf) {
		index = hstat->index++;
		hstat->left[index] = huffman_create_tree(hstat);
		hstat->right[index] = huffman_create_tree(hstat);
	} else	// 返回字符
		index = huffman_get_bits(hstat, 8);

	return index;
}

static void huffman_decompress(BYTE *out, DWORD out_len, 
							   BYTE *in,  DWORD in_len, 
							   DWORD key)
{
	struct huffman_state hstat;

	if (key) {
		DWORD xor = key ^ 0x6393528E;
		u32 *p = (u32 *)in;

		for (DWORD i = 0; i < in_len / 4; ++i)
			p[i] ^= xor;
	}

	hstat.in = in;
	hstat.curbits = 0;
	hstat.index = 256;
	memset(hstat.left, 0, sizeof(hstat.left));
	memset(hstat.right, 0, sizeof(hstat.right));
	DWORD index = huffman_create_tree(&hstat);

	for (DWORD i = 0; i < out_len; ++i) {
		DWORD idx = index;
		while (idx >= 256) {
			u32 is_right;

			if ((int)--hstat.curbits < 0) {
				hstat.curbits = 31;
				hstat.cache = *(u32 *)hstat.in;
				hstat.in += 4;
				is_right = hstat.cache >> 31;
			} else
				is_right = (hstat.cache >> hstat.curbits) & 1;		

			if (is_right)
				idx = hstat.right[idx];
			else
				idx = hstat.left[idx];
		}
		*out++ = (BYTE)idx;
	}
}

static void YH1_decompress(BYTE *out, DWORD out_len, BYTE *in, DWORD in_len)
{
	if (in[3])
		huffman_decompress(out, out_len, in + 8, in_len - 8, 'KM');
	else
		huffman_decompress(out, out_len, in + 8, in_len - 8, 0);
}

static void lz_decompress(BYTE *out, BYTE *in, DWORD in_len, DWORD key)
{
	if (key) {
		u32 xor = (key << 16) | key;
		u32 *p = (u32 *)in;

		for (DWORD i = 0; i < in_len / 4; ++i)
			*p++ ^= xor;
		for (i = 0; i < (in_len & 3); ++i)
			((u8 *)p)[i] ^= ((u8 *)&xor)[i & 1];
	}

	u32 bit_count = 32;
	u32 flags = *(u32 *)in;
	in += 4;
	u32 bit;

	while (1) {
		u32 offset;

		#define get_bit()	\
			bit = flags & 0x80000000;	\
			flags <<= 1;	\
			if (!--bit_count) {	\
				bit_count = 32;	\
				flags = *(u32 *)in;	\
				in += 4;	\
			}

		get_bit();
		if (bit)
			*out++ = *in++;
		else {
			get_bit();
			offset = 0xffffff00 | *in++;
			if (bit) {
				get_bit();
				if (bit) {
					offset &= 0xfffffeff;
					get_bit();					
					if (bit)
						offset |= 1 << 8;
				} else {
					get_bit();
					if (bit) {
						offset &= 0xfffffcff;
						get_bit();
						if (bit)
							offset |= 1 << 8;
					} else {
						get_bit();
						if (bit) {
							offset &= 0xfffff8ff;
							get_bit();
							if (bit)
								offset |= 1 << 9;
							get_bit();
							if (bit)
								offset |= 1 << 8;
						} else {
							get_bit();
							if (bit) {
								offset &= 0xfffff0ff;
								get_bit();
								if (bit)
									offset |= 1 << 10;
								get_bit();
								if (bit)
									offset |= 1 << 9;
								get_bit();
								if (bit)
									offset |= 1 << 8;
							} else {
								offset &= 0xffffe0ff;
								get_bit();
								if (bit)
									offset |= 1 << 11;
								get_bit();
								if (bit)
									offset |= 1 << 10;
								get_bit();
								if (bit)
									offset |= 1 << 9;
								get_bit();
								if (bit)
									offset |= 1 << 8;
							}
						}
					}
				}

				get_bit();
				u32 cp;
				if (bit)
					cp = 3;
				else {
					get_bit();
					if (bit)
						cp = 4;
					else {
						get_bit();
						if (bit) {
							cp = 5;
							get_bit();
							if (bit)
								++cp;
						} else {
							get_bit();
							if (bit) {
								cp = 7;
								get_bit();
								if (bit)
									cp += 2;
								get_bit();
								if (bit)
									++cp;
							} else {
								get_bit();
								if (bit) {
									cp = 11;
									get_bit();
									if (bit)
										cp += 4;
									get_bit();
									if (bit)
										cp += 2;
									get_bit();
									if (bit)
										++cp;
								} else
									cp = *in++ + 19;
							}
						}
					}
				}

				BYTE *src = out + offset;
				for (DWORD i = 0; i < cp; ++i)
					*out++ = *src++;
			} else {
				bit = get_bit();
				if (!bit) {
					if (offset == 0xffffffff)
						return;
				} else {
					offset &= 0xfffff0ff;
					DWORD tmp = 8;
					bit = get_bit();
					if (bit)
						tmp += 4;
					bit = get_bit();
					if (bit)
						tmp += 2;
					bit = get_bit();
					if (bit)
						++tmp;
					--tmp;
					offset |= tmp << 8;
				}

				*out = *(out + offset);
				++out;
				*out = *(out + offset);
				++out;
			}
		}
		#undef get_bit
	}
}

static void YLZ_decompress(BYTE *out, DWORD out_len, BYTE *in, DWORD in_len)
{
	if (in[3])
		lz_decompress(out, in + 8, in_len - 8, 'KM');
	else
		lz_decompress(out, in + 8, in_len - 8, 0);
}

static void ypk_decompress(BYTE *out, DWORD out_len, BYTE *in, DWORD in_len, DWORD key)
{
	if (key) {
		u32 xor = ~((key << 16) | key);
		u32 *p = (u32 *)in;

		for (DWORD i = 0; i < in_len / 4; ++i)
			*p++ ^= xor;
		for (i = 0; i < (in_len & 3); ++i)
			((u8 *)p)[i] ^= xor;
	}

	DWORD act_out_len = out_len;
	uncompress(out, &act_out_len, in, in_len);
}

static void YPK_decompress(BYTE *out, DWORD out_len, BYTE *in, DWORD in_len)
{
	if (in[3])
		ypk_decompress(out, out_len, in + 8, in_len - 8, 'KM');
	else
		ypk_decompress(out, out_len, in + 8, in_len - 8, 0);
}

static void MI4_decompress(BYTE *out, BYTE *in, DWORD width, DWORD height)
{
	u32 bit_count = 32;
	u32 flags = *(u32 *)in;
	in += 4;
	DWORD pixels = width * height;
	DWORD line_len = width * 3;

	for (DWORD p = 0; p < pixels; ++p) {
		u32 bit;
		u8 f;
		BYTE bgr[3];

		#define get_bit()	\
			bit = !!(flags & 0x80000000);	\
			flags <<= 1;	\
			if (!--bit_count) {	\
				bit_count = 32;	\
				flags = *(u32 *)in;	\
				in += 4;	\
			}

		get_bit();
		if (!bit) {
			get_bit();			
			if (bit) {
				bgr[0] = *in++;
				bgr[1] = *in++;
				bgr[2] = *in++;
			} else {
				get_bit();
				if (bit) {
					get_bit();
					f = bit;
					get_bit();
					f <<= 1;
					f |= bit;
					if (f == 3) {
						bgr[0] = out[0 - line_len];
						bgr[1] = out[1 - line_len];
						bgr[2] = out[2 - line_len];						
					} else {
						bgr[0] += f - 1;
						get_bit();
						f = bit;
						get_bit();
						f <<= 1;
						f |= bit;
						if (f == 3) {
							get_bit();
							if (bit) {
								bgr[0] = *(out - line_len - 3);
								bgr[1] = *(out - line_len - 2);
								bgr[2] = *(out - line_len - 1);										
							} else {
								bgr[0] = *(out - line_len + 3);
								bgr[1] = *(out - line_len + 4);
								bgr[2] = *(out - line_len + 5);
							}
						} else {
							bgr[1] += f - 1;
							get_bit();
							f = bit;
							get_bit();
							f <<= 1;
							f |= bit;
							bgr[2] += f - 1;
						}
					}
				} else {
					get_bit();
					if (bit) {
						get_bit();
						f = bit;
						get_bit();
						f <<= 1;
						f |= bit;
						get_bit();
						f <<= 1;
						f |= bit;
						if (f == 7) {
							bgr[0] = *(out - line_len + 0);
							bgr[1] = *(out - line_len + 1);
							bgr[2] = *(out - line_len + 2);
							get_bit();
							f = bit;
							get_bit();
							f <<= 1;
							f |= bit;
							get_bit();
							f <<= 1;
							f |= bit;
							bgr[0] += f - 3;
							get_bit();
							f = bit;
							get_bit();
							f <<= 1;
							f |= bit;
							get_bit();
							f <<= 1;
							f |= bit;
							bgr[1] += f - 3;
							get_bit();
							f = bit;
							get_bit();
							f <<= 1;
							f |= bit;
							get_bit();
							f <<= 1;
							f |= bit;
							bgr[2] += f - 3;
						} else {
							bgr[0] += f - 3;
							get_bit();
							f = bit;
							get_bit();
							f <<= 1;
							f |= bit;
							get_bit();
							f <<= 1;
							f |= bit;
							bgr[1] += f - 3;
							get_bit();
							f = bit;
							get_bit();
							f <<= 1;
							f |= bit;
							get_bit();
							f <<= 1;
							f |= bit;
							bgr[2] += f - 3;
						}
					} else {
						get_bit();
						if (bit) {
							get_bit();
							f = bit;
							get_bit();
							f <<= 1;
							f |= bit;
							get_bit();
							f <<= 1;
							f |= bit;
							get_bit();
							f <<= 1;
							f |= bit;
							bgr[0] += f - 7;
							get_bit();
							f = bit;
							get_bit();
							f <<= 1;
							f |= bit;
							get_bit();
							f <<= 1;
							f |= bit;
							get_bit();
							f <<= 1;
							f |= bit;
							bgr[1] += f - 7;
							get_bit();
							f = bit;
							get_bit();
							f <<= 1;
							f |= bit;
							get_bit();
							f <<= 1;
							f |= bit;
							get_bit();
							f <<= 1;
							f |= bit;
							bgr[2] += f - 7;
						} else {
							get_bit();
							f = bit;
							get_bit();
							f <<= 1;
							f |= bit;
							get_bit();
							f <<= 1;
							f |= bit;
							get_bit();
							f <<= 1;
							f |= bit;
							get_bit();
							f <<= 1;
							f |= bit;
							bgr[0] += f - 15;
							get_bit();
							f = bit;
							get_bit();
							f <<= 1;
							f |= bit;
							get_bit();
							f <<= 1;
							f |= bit;
							get_bit();
							f <<= 1;
							f |= bit;
							get_bit();
							f <<= 1;
							f |= bit;
							bgr[1] += f - 15;
							get_bit();
							f = bit;
							get_bit();
							f <<= 1;
							f |= bit;
							get_bit();
							f <<= 1;
							f |= bit;
							get_bit();
							f <<= 1;
							f |= bit;
							get_bit();
							f <<= 1;
							f |= bit;
							bgr[2] += f - 15;
						}
					}
				}
			}
		}
		#undef get_bit
		*out++ = bgr[0];
		*out++ = bgr[1];
		*out++ = bgr[2];
	}
}

static void S25_decompress(BYTE *out, BYTE **compr_line_table,
						   DWORD width, DWORD height)
{
	int line_len = width * 4;
	for (DWORD y = 0; y < height; ++y) {
		BYTE b, g, r, a;
		BYTE *compr_line = *compr_line_table++;
		DWORD compr_line_len = *(u16 *)compr_line;	// 不包含对齐用字节
		DWORD compr_line_pos = 2;
		compr_line += 2;
		DWORD cnt;

		for (DWORD x = width; (int)x > 0; ) {
			compr_line_pos += (u32)compr_line & 1;
			compr_line = (BYTE *)((u32)(compr_line + 1) & ~1);	// 2字节对齐
			cnt = *(u16 *)compr_line;
			compr_line += 2;
			compr_line_pos += 2;
			DWORD flag = cnt >> 13;
			compr_line += (cnt & 0x1800) >> 11;
			compr_line_pos += (cnt & 0x1800) >> 11;
			cnt &= 0x7ff;
			if (x < cnt) {
				cnt = x;
				x = 0;
			} else
				x -= cnt;
			if (flag == 3) {
				b = *compr_line++;
				g = *compr_line++;
				r = *compr_line++;
				compr_line_pos += 3;
				for (DWORD i = 0; i < cnt; ++i) {
					*out++ = b;
					*out++ = g;
					*out++ = r;
					*out++ = 0xff;
				}
			} else if (flag == 2) {
				for (DWORD i = 0; i < cnt; ++i) {
					*out++ = *compr_line++;
					*out++ = *compr_line++;
					*out++ = *compr_line++;
					*out++ = 0xff;
					compr_line_pos += 3;
				}
			} else if (flag == 4) {
				for (DWORD i = 0; i < cnt; ++i) {
					a = *compr_line++;
					*out += (*compr_line++ - *out) * a / 256;
					++out;
					*out += (*compr_line++ - *out) * a / 256;
					++out;
					*out += (*compr_line++ - *out) * a / 256;
					++out;
					*out++ = a;
					compr_line_pos += 4;
				}
			} else if (flag == 0 || flag == 1) {
				out += cnt * 4;
			} else {
				u32 rgba = *(u32 *)compr_line;
				compr_line += 4;
				compr_line_pos += 4;
				r = (rgba >> 24) & 0xff;
				g = (rgba >> 16) & 0xff;
				b = (rgba >> 8) & 0xff;
				a = rgba & 0xff;
				for (DWORD i = 0; i < cnt; ++i) {
					*out += (b - *out) * a / 256;
					++out;
					*out += (g - *out) * a / 256;
					++out;
					*out += (r - *out) * a / 256;
					++out;
					*out++ = a;
				}
			}
		}
	}
}

#if 0
static void PAD_decompress(BYTE *out, BYTE *in, DWORD channels)
{
	double table0[14][2];
	double table1[14][2];
	BYTE flag = *in++;
	int [+20] = 28;
	if (flag == 0x80) {

	} else if (flag == 0xff) {

	} else {
		if (flag)
			[+20] = flag;

		int d = *compr++;
		int d0_l = d & 0xf;
		int d0_h = d >> 4;
		if (channels == 2) {
			d = *compr++;
			int d1_l = d & 0xf;
			int d1_h = d >> 4;
		}

		for (DWORD i = 0; i < 14; ++i) {
			d = *compr++;
			int v = (d & 0xf) << 12;
			if (v & 0x8000)
				v |= 0xffff0000;
			v >>= d0_l;
			table[i][0] = (double)v;

			v = (d & 0xf0) << 8;
			if (v & 0x8000)
				v |= 0xffff0000;
			v >>= d0_l;
			table[i][1] = (double)v;
		}

		if (channels == 2) {
			for (DWORD i = 0; i < 14; ++i) {
				d = *compr++;
				int v = (d & 0xf) << 12;
				if (v & 0x8000)
					v |= 0xffff0000;
				v >>= d1_l;
				table[i][0] = (double)v;

				v = (d & 0xf0) << 8;
				if (v & 0x8000)
					v |= 0xffff0000;
				v >>= d1_l;
				table[i][1] = (double)v;
			}
		}

		if ([+20] > 0) {

		}

	}
}
#endif

/********************* decryption staffs *********************/

#define M_PI		3.141592653589793

static long rnd;

static inline unsigned long get_rand(void)
{
	rnd = 1566083941L * rnd + 1L;
	return rnd;
}

static inline void set_rand_seed(unsigned long seed)
{
	rnd = seed;
}

static double decrypt_helper1(double a)
{
	if (a < 0)
		return -decrypt_helper1(-a);

	if (a < 18.0) {
		double v0 = a;
		double v1 = a;
		double v2 = -(a * a);

		for (int i = 3; i < 1000; i += 2) {
			v1 *= v2 / (i * (i - 1));
			/* 可读性不强
			double tmp = v1 / i;
			if (!tmp)
				break;
			v0 += tmp;
			*/
			v0 += v1 / i;
			if (v0 == v2)
				break;
		}
		return v0;
	}

	int flags = 0;
	double v0_l = 0;
	double v1 = 0;
	double div = 1 / a;
	double v1_h = 2.0;
	double v0_h = 2.0;
	double v1_l = 0;
	double v0 = 0;
	int i = 0;
	
	do {
		v0 += div;
		div *= ++i / a;
		if (v0 < v0_h)
			v0_h = v0;
		else
			flags |= 1;

		v1 += div;
		div *= ++i / a;
		if (v1 < v1_h)
			v1_h = v1;
		else
			flags |= 2;

		v0 -= div;
		div *= ++i / a;
		if (v0 > v0_l)
			v0_l = v0;
		else
			flags |= 4;

		v1 -= div;
		div *= ++i / a;
		if (v1 > v1_l)
			v1_l = v1;
		else
			flags |= 8;
	} while (flags != 15);

	return ((M_PI - cos(a) * (v0_l + v0_h)) - (sin(a) * (v1_l + v1_h))) / 2.0;
}

static DWORD decrypt_helper2(DWORD WARC_version, double a)
{
	double v0, v1, v2, v3;

	if (a > 1.0) {
		v0 = sqrt(a * 2 - 1);	
		while (1) {
			v1 = 1 - (double)get_rand() / 4294967296.0;
			v2 = 2.0 * (double)get_rand() / 4294967296.0 - 1.0;
			if (v1 * v1 + v2 * v2 > 1.0)
				continue;

			v2 /= v1;
			v3 = v2 * v0 + a - 1.0;
			if (v3 <= 0)
				continue;

			v1 = (a - 1.0) * log(v3 / (a - 1.0)) - v2 * v0;
			if (v1 < -50.0)
				continue;

			if (((double)get_rand() / 4294967296.0) <= (exp(v1) * (v2 * v2 + 1.0)))
				break;
		}
	} else {
		v0 = exp(1.0) / (a + exp(1.0));
		do {
			v1 = (double)get_rand() / 4294967296.0;
			v2 = (double)get_rand() / 4294967296.0;
			if (v1 < v0) {
				v3 = pow(v2, 1.0 / a);
				v1 = exp(-v3);
			} else {
				v3 = 1.0 - log(v2);
				v1 = pow(v3, a - 1.0);
			}
		} while ((double)get_rand() / 4294967296.0 >= v1);
	}

	if (WARC_version > 120)
		return (DWORD)(v3 * 256.0);
	else
		return (BYTE)((double)get_rand() / 4294967296.0);	
}

#if 0
// 433B70
static DWORD decrypt_helper3(BYTE *buf)
{
	// とんがり帽子のメモノレ
	char decrypt_string[] = "偲傫偑傝朮巕偺儊儌哨";
	char UserName[MAX_PATH];
	DWORD UserNameLen = sizeof(UserName);
	u32 code[80], v39[64], v42[64];

	GetUserNameA(UserName, &UserNameLen);

	u32 *p = (u32 *)(buf + 40);
	for (DWORD i = 0; i < 16; ++i) {
		code[i] = ((p[i] & 0xFF00 | (p[i] << 16)) << 8) | (((p[i] >> 16) | p[i] & 0xFF0000) >> 8);
		
		if (i >= 12)
			v5 = 1;
		else if (i >= 8)
			v5 = -4;
		else (i >= 4)
			v5 = 2;
		else
			v5 = 3;

		v39[i] = v5;
		v42[i] = UserName[i] | (UserName[i+1] << 8) | (UserName[i+3] << 16);
	}

	for (DWORD k = 0; k < 80 - i; ++k) {
		u32 tmp = code[0 + k] ^ code[2 + k] ^ code[8 + k] ^ code[13 + k];
		code[16 + k] = (tmp >> 31) | (tmp << 1);
	}

	v39[0] = GetSystemMetrics(v42[0] & 0x3F);
	v39[1] = GetKeyboardType(v42[1] & 1);
	v39[2] = GetSysColor(v42[2] & 0xF);
	v39[3] = p[0] | 0x80000000;
	v39[4] = p[1] & 0xFFFFFF;
	
	u32 a = ((u32 *)decrypt_string)[0];
	u32 b = ((u32 *)decrypt_string)[1];
	u32 c = ((u32 *)decrypt_string)[2];
	u32 d = ((u32 *)decrypt_string)[3];
	u32 e = ((u32 *)decrypt_string)[4];
	for (i = acos(v39[12]); i < 80; ++i) {
		u32 v0, v1, v2, v3;
		
		/*
			EDI: d
			EBX: c
			ESI: b			
			+18: e
			+14: a
		 */
		if (i < 16) {
			v0 = b ^ c ^ d;	// EDX
			v1 = 0;	//+10
		} else if (i < 32) {
			v0 = (~b & d) | (b & c);
			v1 = 0x5A827999;
		} else if (i < 48) {
			v0 = (b | ~c) ^ d;
			v1 = 0x6ED9EBA1;
		} else if (i < 64) {
			v0 = (c & ~d) | (b & d);
			v1 = 0x8F1BBCDC;
		} else {
			v0 = (c | ~d) ^ b;
			v1 = 0xA953FD4E;
		}

		v2 = ((a >> 27) | (a << 5)) + v1 + v0 + e + code[i];
		v3 = (b >> 2) | (b << 30);
		b = a;
		e = d;
		d = c;
		c = v3;
		a = v2;
	}

	EAX = v39[2];
	if (EAX & 0x78000000)
		EAX |= 0x98000000;

	((u32 *)decrypt_string)[0] += a;
	((u32 *)decrypt_string)[1] += b;
	((u32 *)decrypt_string)[2] += c;


}
#endif

static int resource_decrypt_flag;
static void decrypt(unsigned long WARC_version, BYTE *cipher, unsigned int cipher_length)
{
	DWORD _cipher_length = cipher_length;
	const char *key = WARC_key_string(WARC_version);

	if (cipher_length < 3)
		return;

	if (cipher_length > 1024)
		cipher_length = 1024;

	extern BYTE RioShiina_png[49790];

	int a, b;
	DWORD fac = 0;
	if (WARC_version > 120) {
		set_rand_seed(_cipher_length);
		a = (char)cipher[0] ^ (char)_cipher_length;
		b = (char)cipher[1] ^ (char)(_cipher_length / 2);
		if (resource_decrypt_flag && (_cipher_length != warc_max_index_length(WARC_version))) {
			if (WARC_version >= 160) {
				fac = RioShiina_png[(DWORD)((double)get_rand() * (sizeof(RioShiina_png) / 4294967296.0))];
				fac += rnd;

				if (WARC_version == 16) {
					float fv;
					BYTE *p;
					p = (BYTE *)&fac;
					fv = (float)(1.5 * (double)p[0] + 0.1);
					DWORD v0 = *(DWORD *)&fv;
					fv = (float)(1.5 * (double)p[1] + 0.1);
					DWORD v1 = *(DWORD *)&fv;
					fv = (float)(1.5 * (double)p[2] + 0.1);
					DWORD v2 = *(DWORD *)&fv;
					fv = (float)(1.5 * (double)p[3] + 0.1);
					DWORD v3 = *(DWORD *)&fv;
					p = (BYTE *)&v0;

					BYTE tmp = p[0];
					p[0] = p[3];
					p[3] = tmp;
					tmp = p[1];
					p[1] = p[2];
					p[2] = tmp;
					v1 = (DWORD)(*(float *)&v1);
					v2 = 0 - v2;
					v3 = ~v3;

					fac = ((v0 + v1) | (v2 - v3)) & 0xfffffff;
				} else {
					// fac = key & 0xfffffff;
					//decrypt_helper3(cipher + 4);
				}
			} else if (WARC_version == 150) {
				fac = RioShiina_png[(DWORD)((double)get_rand() * (sizeof(RioShiina_png) / 4294967296.0))];
				fac += rnd;
				fac ^= (fac & 0xfff) * (fac & 0xfff);
				DWORD v = 0;
				for (DWORD i = 0; i <= 32; ++i) {
					DWORD tmp = fac & 1;
					fac >>= 1;
					if (tmp)
						v += fac;
				}
				fac = v;
			} else if (WARC_version == 140)
				fac = RioShiina_png[(DWORD)((double)get_rand() * (sizeof(RioShiina_png) / 4294967296.0))];
			else if (WARC_version == 130)
				fac = RioShiina_png[((DWORD)((double)get_rand() * (sizeof(RioShiina_png) / 4294967296.0))) & 0xff];
		}
	} else {
		a = cipher[0];
		b = cipher[1];
	}

	set_rand_seed(rnd ^ (DWORD)(decrypt_helper1(a) * 100000000.0));

	double tmp;
	if (!a && !b)	// 1.7 staff
		tmp = 0;
	else
		tmp = atan2(b, a);
	tmp = tmp / M_PI * 180.0;
	// FIXME: tmp is wrong for 1.7
	if (tmp < 0)
		tmp += 360.0;

	const char *key_string = WARC_key_string(WARC_version);
	int key_string_len = strlen(key_string);

	DWORD i = ((BYTE)decrypt_helper2(WARC_version, tmp) + fac) % key_string_len;
	int n = 0;
	for (DWORD k = 2; k < cipher_length; ++k) {
		if (WARC_version > 120)
			cipher[k] ^= (BYTE)((double)get_rand() / 16777216.0);
		else
			cipher[k] ^= (BYTE)((double)get_rand() / 4294967296.0);
		cipher[k] = ((cipher[k] & 1) << 7) | (cipher[k] >> 1);
		cipher[k] ^= key_string[n++] ^ key_string[i];
		i = cipher[k] % key_string_len;
		if (n >= key_string_len)
			n = 0;
	}

	return;

#if 0
	v5 = 0;
	if (_cipher_length != warc_max_index_length(WARC_version)) {
		if (WARC_version == 170) {

			//double qword0 = v10;
			//rnd = v10;
			dword_4A45D4 = 1;

			dword_4A45D8 = (1566083941 * _cipher_length + 1) + RioShiina_png[(unsigned long)(rnd * -1.159263774752617e-5)];
			//func_4237C0(256);	// 该函数修改了dword_4A45D4的值
			v5 = 0xcd0da955 & 0xFFFFFFF;
			if (cipher_length > 128) {
				func_440940(cipher + 4, &unk_478108);
            	cipher += 128;
            	cipher_length -= 128;
			}
		}
#endif
}

extern BYTE raw_init_sc[];
static BYTE init_sc[0x21a];

static int init_sc_uncompress(BYTE *uncompr, BYTE *compr)
{
	DWORD flag = *(u32 *)compr;
	int bits = 32;
	BYTE *orig_uncompr = uncompr;
	BYTE *orig_compr = compr;
	
	compr += 4;
	while (1) {
		int bit_is_set;

		bit_is_set = flag & 0x80000000;
		flag <<= 1;
		bits--;
		if (!bits) {
			flag = *(u32 *)compr;
			compr += 4;
			bits = 32;
		}				

		if (bit_is_set)
			*uncompr++ = ~*compr++;
		else {
			DWORD copy_bytes;
			DWORD offset;

			bit_is_set = flag & 0x80000000;
			flag <<= 1;
			bits--;
			if (!bits) {
				flag = *(u32 *)compr;
				compr += 4;
				bits = 32;
			}

			if (bit_is_set) {
				copy_bytes = *(u16 *)compr;
				compr += 2;
				offset = (copy_bytes >> 3) | 0xffffe000;
				copy_bytes &= 7;
				if (copy_bytes)
					copy_bytes += 2;
				else {
					copy_bytes = *compr++;
					if (!copy_bytes)
						break;
					copy_bytes += 9;
				}
			} else {
				copy_bytes = 0;

				bit_is_set = flag & 0x80000000;
				flag <<= 1;
				bits--;
				if (!bits) {
					flag = *(u32 *)compr;
					compr += 4;
					bits = 32;
				}	
				if (bit_is_set)
					copy_bytes++;

				bit_is_set = flag & 0x80000000;
				flag <<= 1;
				bits--;
				if (!bits) {
					flag = *(u32 *)compr;
					compr += 4;
					bits = 32;
				}	
				copy_bytes <<= 1;
				if (bit_is_set)
					copy_bytes++;

				offset = *compr++ | 0xffffff00;
				copy_bytes += 2;
			}
			
			for (DWORD i = 0; i < copy_bytes; i++) {
				*uncompr = uncompr[offset];
				uncompr++;
			}
		}
	}
//	printf("actual comprlen %x\n", compr - orig_compr);
	return uncompr - orig_uncompr;
}

#if 0
static int decode_init_sc(void)
{
	int raw_length = sizeof(raw_init_sc);

	if (raw_length > 1024)
		raw_length = 1024;

	for (int i = 0; i < raw_length; i++) {
		float result = (float)sin((i % 180) - 90);
		raw_init_sc[i] ^= *(BYTE *)&result;
	}
	memset(init_sc, 0, sizeof(init_sc));
	init_sc_uncompress(init_sc, raw_init_sc);

	return 0;
}
#endif

/********************* WAR *********************/

/* 封包匹配回调函数 */
static int RioShiina_WAR_match(struct package *pkg)
{
	WAR_header_t WAR_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &WAR_header, sizeof(WAR_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(WAR_header.magic, "WARC 1", 6)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	//char magic[9];
	//strncpy(magic, WAR_header.magic, 8);
	//magic[8] = 0;
	//printf("WARC version: %s\n", magic);

	if (strncmp(WAR_header.magic, "WARC 1.2", 8) 
			&& strncmp(WAR_header.magic, "WARC 1.3", 8)
			&& strncmp(WAR_header.magic, "WARC 1.4", 8)
			&& strncmp(WAR_header.magic, "WARC 1.5", 8)
			&& strncmp(WAR_header.magic, "WARC 1.6", 8)
			&& strncmp(WAR_header.magic, "WARC 1.7", 8)
			) {
		printf("不支持该WARC文件。请将本游戏名字、RIO.ini（或与游戏exe同名的.ini）"
			"文件中第一行中内容为[捙柤棦弿 vX.XX]中的X.XX版本号和前面显示的WARC version这3个信息一起"
			"报告到https://www.yukict.com/bbs/thread-20582-1-1.html（可匿名发帖）.谢谢");
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int RioShiina_WAR_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{
	WAR_header_t WAR_header;
	DWORD crypt_index_length;	
	BYTE *crypt_index;
	u32 WAR_size;
	DWORD WARC_version;

	if (pkg->pio->readvec(pkg, &WAR_header, sizeof(WAR_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

    if (WAR_header.magic[6] == '.')
		WARC_version = (WAR_header.magic[5] - '0') * 100 + (WAR_header.magic[7] - '0') * 10;
    else
		WARC_version = (WAR_header.magic[5] - '0') * 100 + (WAR_header.magic[6] - '0') * 10 + (WAR_header.magic[7] - '0');

	WAR_header.index_offset ^= 0xF182AD82;	// "くん"

	pkg->pio->length_of(pkg, &WAR_size);

	crypt_index_length = WAR_size - WAR_header.index_offset;
	DWORD max_index_len = warc_max_index_length(WARC_version);
	if (crypt_index_length > max_index_len)
		crypt_index_length = max_index_len;
	crypt_index = (BYTE *)malloc(max_index_len);
	if (!crypt_index)
		return -CUI_EMEM;

	memset(crypt_index, 0, max_index_len);
	if (pkg->pio->readvec(pkg, crypt_index, crypt_index_length, WAR_header.index_offset, IO_SEEK_SET)) {
		free(crypt_index);
		return -CUI_EREADVEC;
	}
	decrypt(WARC_version, crypt_index, max_index_len);

	for (DWORD i = 0; i < max_index_len / 4; ++i)
		((u32 *)crypt_index)[i] ^= WAR_header.index_offset;

	DWORD index_buffer_len = max_index_len;
	BYTE *index_buffer = (BYTE *)malloc(index_buffer_len);
	if (!index_buffer) {
		free(crypt_index);
		return -CUI_EMEM;
	}

	if (WARC_version >= 170) {
#if 1
		for (i = 0; i < max_index_len; i++)
			crypt_index[i] ^= (BYTE)WARC_version;
#else
		DWORD key = WARC_version << 24 | WARC_version << 16 | WARC_version << 8 | WARC_version;
		if (key) {
			BYTE xor = (BYTE)~(key << 16 | key);
			for (i = 0; i < max_index_len - 8; i++)
				(crypt_index + 8)[i] ^= xor;
		}
#endif
		// crypt_index[0-7]?
		uncompress(index_buffer, &index_buffer_len, crypt_index + 8, crypt_index_length - 8);
	} else
		index_buffer_len = rng_decompress(index_buffer, crypt_index, crypt_index_length);

	free(crypt_index);
	pkg_dir->index_entries = index_buffer_len / sizeof(WARC_entry_t);
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_len;
	pkg_dir->index_entry_length = sizeof(WARC_entry_t);
	package_set_private(pkg, WARC_version);

	return 0;
}

/* 封包索引项解析函数 */
static int RioShiina_WAR_parse_resource_info(struct package *pkg,
											 struct package_resource *pkg_res)
{
	WARC_entry_t *WAR_entry;

	WAR_entry = (WARC_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, WAR_entry->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = WAR_entry->comprlen;
	pkg_res->actual_data_length = WAR_entry->uncomprlen;
	pkg_res->offset = WAR_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int RioShiina_WAR_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	BYTE *compr;
	DWORD comprlen;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	WARC_entry_t *WAR_entry = (WARC_entry_t *)pkg_res->actual_index_entry;
	DWORD WARC_version = (DWORD)package_get_private(pkg);
	if (WAR_entry->flags & 0x80000000) {
		*(u32 *)compr ^= (*((u32 *)compr + 1) ^ 0xFF82AD82) & 0xffffff;
		if (comprlen > 8)			
			decrypt(WARC_version, compr + 8, comprlen - 8);
	}

//	printf("%s: flags %x, %x / %x\n", pkg_res->name, WAR_entry->flags, 
	//	WAR_entry->comprlen, WAR_entry->uncomprlen);

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = compr;
		return 0;
	}

	if (!strncmp((char *)compr, "YH1", 3)) {
		u32 uncomprlen = *(u32 *)(compr + 4);
		BYTE *uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}
		YH1_decompress(uncompr, uncomprlen, compr, comprlen);
		pkg_res->actual_data = uncompr;
		pkg_res->actual_data_length = uncomprlen;
	} else if (!strncmp((char *)compr, "YLZ", 3)) {
		u32 uncomprlen = *(u32 *)(compr + 4);
		BYTE *uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}
		YLZ_decompress(uncompr, uncomprlen, compr, comprlen);
		pkg_res->actual_data = uncompr;
		pkg_res->actual_data_length = uncomprlen;
	} else if (!strncmp((char *)compr, "YPK", 3)) {
		u32 uncomprlen = *(u32 *)(compr + 4);
		BYTE *uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}
		YPK_decompress(uncompr, uncomprlen, compr, comprlen);
		pkg_res->actual_data = uncompr;
		pkg_res->actual_data_length = uncomprlen;
	} else
		*(u32 *)compr ^= (*((u32 *)compr + 1) ^ 0xFF82AD82) & 0xffffff;

	pkg_res->raw_data = compr;

	return 0;
}

/* 资源保存函数 */
static int RioShiina_WAR_save_resource(struct resource *res, 
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
static void RioShiina_WAR_release_resource(struct package *pkg, 
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
static void RioShiina_WAR_release(struct package *pkg, 
								  struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation RioShiina_WAR_operation = {
	RioShiina_WAR_match,				/* match */
	RioShiina_WAR_extract_directory,	/* extract_directory */
	RioShiina_WAR_parse_resource_info,	/* parse_resource_info */
	RioShiina_WAR_extract_resource,		/* extract_resource */
	RioShiina_WAR_save_resource,		/* save_resource */
	RioShiina_WAR_release_resource,		/* release_resource */
	RioShiina_WAR_release				/* release */
};

/********************* S25 *********************/

static int RioShiina_S25_match(struct package *pkg)
{
	S25_header_t S25_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &S25_header, sizeof(S25_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(S25_header.magic, "S25", 4) || !S25_header.index_entries) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int RioShiina_S25_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{
	S25_header_t header;

	if (pkg->pio->readvec(pkg, &header, sizeof(header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	u32 *offset_table = new u32[header.index_entries + 1];
	if (!offset_table)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, offset_table, sizeof(u32) * header.index_entries)) {
		delete [] offset_table;
		return -CUI_EREAD;
	}
	pkg->pio->length_of(pkg, &offset_table[header.index_entries]);

	my_s25_entry_t *index = new my_s25_entry_t[header.index_entries];
	if (!index) {
		delete [] offset_table;
		return -CUI_EMEM;
	}

	char pkg_name[MAX_PATH];
	unicode2acp(pkg_name, MAX_PATH, pkg->name, -1);
	*strstr(pkg_name, ".") = 0;
	int p = -1;
	for (DWORD i = 0; i < header.index_entries; ++i) {
		S25_info_t info;

		sprintf(index[i].name, "%s@%04d", pkg_name, i);
		index[i].offset = offset_table[i];
		if (!index[i].offset)
			index[i].length = 0;
		else if (!pkg->pio->readvec(pkg, &info, sizeof(info), offset_table[i], IO_SEEK_SET)) {
			if (!info.width || !info.height)
				index[i].length = 0;
			else {
				if (p != -1)
					index[p].length = offset_table[i] - offset_table[p];
				p = i;
			}
		} else
			break;
	}
	if (i != header.index_entries) {
		delete [] offset_table;
		delete [] index;
		return -CUI_EREADVEC;
	}
	if (p != -1)
		index[p].length = offset_table[i] - offset_table[p];
	delete [] offset_table;

	/* 因为后面的图像可能引用前面图像的compr_line数据 */
	u32 s25_size;
	pkg->pio->length_of(pkg, &s25_size);

	BYTE *S25 = new BYTE[s25_size];
	if (!S25) {
		delete [] index;
		return -CUI_EMEM;
	}

	if (pkg->pio->readvec(pkg, S25, s25_size, 0, IO_SEEK_SET)) {
		delete [] S25;
		delete [] index;
		return -CUI_EREADVEC;
	}

	pkg_dir->index_entries = header.index_entries;
	pkg_dir->index_entry_length = sizeof(my_s25_entry_t);
	pkg_dir->directory = index;
	pkg_dir->directory_length = sizeof(my_s25_entry_t) * header.index_entries;
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;
	package_set_private(pkg, S25);

	return 0;
}

static int RioShiina_S25_parse_resource_info(struct package *pkg,
											 struct package_resource *pkg_res)
{
	my_s25_entry_t *entry;

	entry = (my_s25_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = entry->offset;

	return 0;
}

static int RioShiina_S25_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	BYTE *S25 = (BYTE *)package_get_private(pkg);
	S25_info_t *info = (S25_info_t *)(S25 + pkg_res->offset);
	if (info->reserved)
		printf("%s: orig_x %x, orig_y %x, reserved %x\n", 
			pkg_res->name, info->orig_x, info->orig_y, info->reserved);
	BYTE **line_offset_table = (BYTE **)(info + 1);
	for (int i = 0; i < info->height; ++i)
		line_offset_table[i] = S25 + (u32)line_offset_table[i];

	DWORD uncomprlen = info->width * info->height * 4;
	BYTE *uncompr = new BYTE[uncomprlen];
	if (!uncompr)
		return -CUI_EMEM;
	memset(uncompr, 0, uncomprlen);

	S25_decompress(uncompr, line_offset_table, info->width, info->height);

	BYTE *p  = uncompr;
	for (i = 0; i < info->width * info->height; ++i) {
		BYTE a = p[3];
		p[0] = p[0] * a / 255 + (255 - a);
		p[1] = p[1] * a / 255 + (255 - a);
		p[2] = p[2] * a / 255 + (255 - a);
		p += 4;
	}

	if (MyBuildBMPFile(uncompr, uncomprlen, NULL, 0, info->width,
			-info->height, 32, (BYTE **)&pkg_res->actual_data,
			&pkg_res->actual_data_length, my_malloc)) {
		delete [] uncompr;
		return -CUI_EMEM;		
	}
	delete [] uncompr;

	return 0;
}

/* 封包卸载函数 */
static void RioShiina_S25_release(struct package *pkg, 
								  struct package_directory *pkg_dir)
{
	void *priv = (void *)package_get_private(pkg);
	if (priv) {
		free(priv);
		package_set_private(pkg, NULL);
	}

	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

static cui_ext_operation RioShiina_S25_operation = {
	RioShiina_S25_match,				/* match */
	RioShiina_S25_extract_directory,	/* extract_directory */
	RioShiina_S25_parse_resource_info,	/* parse_resource_info */
	RioShiina_S25_extract_resource,		/* extract_resource */
	RioShiina_WAR_save_resource,		/* save_resource */
	RioShiina_WAR_release_resource,		/* release_resource */
	RioShiina_S25_release				/* release */
};

/********************* MI4 *********************/

static int RioShiina_MI4_match(struct package *pkg)
{
	MI4_header_t MI4_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &MI4_header, sizeof(MI4_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(MI4_header.magic, "MAI4", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int RioShiina_MI4_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	BYTE *MI4 = new BYTE[pkg_res->raw_data_length];
	if (!MI4)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, MI4, pkg_res->raw_data_length, 0, IO_SEEK_SET)) {
		delete [] MI4;
		return -CUI_EREADVEC;
	}

	MI4_header_t *MI4_header = (MI4_header_t *)MI4;

	DWORD uncomprlen = MI4_header->width * MI4_header->height * 3;
	BYTE *uncompr = new BYTE[uncomprlen];
	if (!uncompr) {
		delete [] MI4;
		return -CUI_EMEM;
	}

	MI4_decompress(uncompr, (BYTE *)(MI4_header + 1), MI4_header->width, MI4_header->height);

	if (MyBuildBMPFile(uncompr, uncomprlen, NULL, 0, MI4_header->width,
			-MI4_header->height, 24, (BYTE **)&pkg_res->actual_data,
			&pkg_res->actual_data_length, my_malloc)) {
		delete [] uncompr;
		delete [] MI4;
		return -CUI_EMEM;		
	}
	delete [] uncompr;
	delete [] MI4;

	return 0;
}

static cui_ext_operation RioShiina_MI4_operation = {
	RioShiina_MI4_match,			/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	RioShiina_MI4_extract_resource,	/* extract_resource */
	RioShiina_WAR_save_resource,	/* save_resource */
	RioShiina_WAR_release_resource,	/* release_resource */
	RioShiina_WAR_release			/* release */
};

/********************* OGV *********************/

static int RioShiina_OGV_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "OGV", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int RioShiina_OGV_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	BYTE *OGV = new BYTE[pkg_res->raw_data_length];
	if (!OGV)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, OGV, pkg_res->raw_data_length, 0, IO_SEEK_SET)) {
		delete [] OGV;
		return -CUI_EREADVEC;
	}

	OGV[1] = 'g';
	OGV[2] = 'g';
	OGV[3] = 'S';
	pkg_res->raw_data = OGV;

	return 0;
}

static cui_ext_operation RioShiina_OGV_operation = {
	RioShiina_OGV_match,			/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	RioShiina_OGV_extract_resource,	/* extract_resource */
	RioShiina_WAR_save_resource,	/* save_resource */
	RioShiina_WAR_release_resource,	/* release_resource */
	RioShiina_WAR_release			/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK RioShiina_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".WAR"), NULL, 
		NULL, &RioShiina_WAR_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".WAA"), NULL, 
		NULL, &RioShiina_WAR_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".MI4"), _T(".bmp"), 
		NULL, &RioShiina_MI4_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;
	
	if (callback->add_extension(callback->cui, _T(".S25"), _T(".bmp"), 
		NULL, &RioShiina_S25_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".OGV"), _T(".ogg"), 
		NULL, &RioShiina_OGV_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	//return decode_init_sc();

	if (get_options2(_T("nodec")))
		resource_decrypt_flag = 0;
	else
		resource_decrypt_flag = 1;

	return 0;
}
