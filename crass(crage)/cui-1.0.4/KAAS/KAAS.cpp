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

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information KAAS_cui_information = {
	_T("karry(TKCOB)"),		/* copyright */
	_T("KAAS17"),			/* system */
	_T(".pd .pb .id"),		/* package */
	_T("1.0.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-3-4 23:02"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	u32 index_entries;
	u32 reserved[3];
} pb_header_t;

typedef struct {
	u32 offset;
	u32 length;
} pb_entry_t;

typedef struct {
	u32 length;
	u16 unknown0;		// 0x800
	u16 channels;
	u32 SamplesPerSec;
	u32 unknown1;		// 0x84be2329
} voice_header_t;

typedef struct {
	u8 mode;
	u8 key;
	u16 width;
	u16 height;
	u16 reserved;
	u32 comp0_length;
	u32 comp1_length;
	u16 comp2_length;
} pic_header_t;
#pragma pack ()

typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 length;
} my_pb_entry_t;

static WORD decode_table[256] = {
		0x0000, 0x0001, 0x0007, 0x0011, 0x001f, 0x0031, 0x0047, 0x0061, 
		0x007e, 0x00a0, 0x00c6, 0x00ef, 0x011d, 0x014e, 0x0184, 0x01bd, 
		0x01fa, 0x023c, 0x0281, 0x02ca, 0x0318, 0x0369, 0x03be, 0x0417, 
		0x0474, 0x04d5, 0x053a, 0x05a3, 0x0610, 0x0681, 0x06f6, 0x076e, 
		0x07eb, 0x086c, 0x08f0, 0x0979, 0x0a06, 0x0a96, 0x0b2b, 0x0bc3, 
		0x0c60, 0x0d00, 0x0da4, 0x0e4d, 0x0ef9, 0x0fa9, 0x105d, 0x1115, 
		0x11d1, 0x1291, 0x1356, 0x141d, 0x14e9, 0x15b9, 0x168d, 0x1765, 
		0x1841, 0x1921, 0x1a04, 0x1aec, 0x1bd8, 0x1cc7, 0x1dbb, 0x1eb2, 
		0x1fae, 0x20ad, 0x21b0, 0x22b8, 0x23c3, 0x24d2, 0x25e6, 0x26fd, 
		0x2818, 0x2937, 0x2a5a, 0x2b81, 0x2cac, 0x2ddb, 0x2f0e, 0x3045, 
		0x3180, 0x32be, 0x3401, 0x3548, 0x3692, 0x37e1, 0x3934, 0x3a8a, 
		0x3be5, 0x3d43, 0x3ea6, 0x400c, 0x4176, 0x42e5, 0x4457, 0x45cd, 
		0x4747, 0x48c5, 0x4a47, 0x4bcd, 0x4d58, 0x4ee5, 0x5077, 0x520d, 
		0x53a7, 0x5545, 0x56e7, 0x588d, 0x5a36, 0x5be4, 0x5d96, 0x5f4b, 
		0x6105, 0x62c2, 0x6484, 0x6649, 0x6812, 0x69e0, 0x6bb1, 0x6d86, 
		0x6f60, 0x713d, 0x731e, 0x7503, 0x76ec, 0x78d9, 0x7aca, 0x7cbf, 
		0xffff, 0xfffe, 0xfff8, 0xffee, 0xffe0, 0xffce, 0xffb8, 0xff9e, 
		0xff81, 0xff5f, 0xff39, 0xff10, 0xfee2, 0xfeb1, 0xfe7b, 0xfe42, 
		0xfe05, 0xfdc3, 0xfd7e, 0xfd35, 0xfce7, 0xfc96, 0xfc41, 0xfbe8, 
		0xfb8b, 0xfb2a, 0xfac5, 0xfa5c, 0xf9ef, 0xf97e, 0xf909, 0xf891, 
		0xf814, 0xf793, 0xf70f, 0xf686, 0xf5f9, 0xf569, 0xf4d4, 0xf43c, 
		0xf39f, 0xf2ff, 0xf25b, 0xf1b2, 0xf106, 0xf056, 0xefa2, 0xeeea, 
		0xee2e, 0xed6e, 0xeca9, 0xebe2, 0xeb16, 0xea46, 0xe972, 0xe89a, 
		0xe7be, 0xe6de, 0xe5fb, 0xe513, 0xe427, 0xe338, 0xe244, 0xe14d, 
		0xe051, 0xdf52, 0xde4f, 0xdd47, 0xdc3c, 0xdb2d, 0xda19, 0xd902, 
		0xd7e7, 0xd6c8, 0xd5a5, 0xd47e, 0xd353, 0xd224, 0xd0f1, 0xcfba, 
		0xce7f, 0xcd41, 0xcbfe, 0xcab7, 0xc96d, 0xc81e, 0xc6cb, 0xc575, 
		0xc41a, 0xc2bc, 0xc159, 0xbff3, 0xbe89, 0xbd1a, 0xbba8, 0xba32, 
		0xb8b8, 0xb73a, 0xb5b8, 0xb432, 0xb2a7, 0xb11a, 0xaf88, 0xadf2, 
		0xac58, 0xaaba, 0xa918, 0xa772, 0xa5c9, 0xa41b, 0xa269, 0xa0b4, 
		0x9efa, 0x9d3d, 0x9b7b, 0x99b6, 0x97ed, 0x961f, 0x944e, 0x9279, 
		0x909f, 0x8ec2, 0x8ce1, 0x8afc, 0x8913, 0x8726, 0x8535, 0x8340, 
};

static u8 pic2_conv_table[320] = {
		0x29, 0x23, 0xbe, 0x84, 0xe1, 0x6c, 0xd6, 0xae, 
		0x52, 0x90, 0x49, 0xf1, 0xf1, 0xbb, 0xe9, 0xeb, 
		0xb3, 0xa6, 0xdb, 0x3c, 0x87, 0x0c, 0x3e, 0x99, 
		0x24, 0x5e, 0x0d, 0x1c, 0x06, 0xb7, 0x47, 0xde, 
		0xb3, 0x12, 0x4d, 0xc8, 0x43, 0xbb, 0x8b, 0xa6, 
		0x1f, 0x03, 0x5a, 0x7d, 0x09, 0x38, 0x25, 0x1f, 
		0x5d, 0xd4, 0xcb, 0xfc, 0x96, 0xf5, 0x45, 0x3b, 
		0x13, 0x0d, 0x89, 0x0a, 0x1c, 0xdb, 0xae, 0x32, 
		0x20, 0x9a, 0x50, 0xee, 0x40, 0x78, 0x36, 0xfd, 
		0x12, 0x49, 0x32, 0xf6, 0x9e, 0x7d, 0x49, 0xdc, 
		0xad, 0x4f, 0x14, 0xf2, 0x44, 0x40, 0x66, 0xd0, 
		0x6b, 0xc4, 0x30, 0xb7, 0x32, 0x3b, 0xa1, 0x22, 
		0xf6, 0x22, 0x91, 0x9d, 0xe1, 0x8b, 0x1f, 0xda, 
		0xb0, 0xca, 0x99, 0x02, 0xb9, 0x72, 0x9d, 0x49, 
		0x2c, 0x80, 0x7e, 0xc5, 0x99, 0xd5, 0xe9, 0x80, 
		0xb2, 0xea, 0xc9, 0xcc, 0x53, 0xbf, 0x67, 0xd6, 
		0xbf, 0x14, 0xd6, 0x7e, 0x2d, 0xdc, 0x8e, 0x66, 
		0x83, 0xef, 0x57, 0x49, 0x61, 0xff, 0x69, 0x8f, 
		0x61, 0xcd, 0xd1, 0x1e, 0x9d, 0x9c, 0x16, 0x72, 
		0x72, 0xe6, 0x1d, 0xf0, 0x84, 0x4f, 0x4a, 0x77, 
		0x02, 0xd7, 0xe8, 0x39, 0x2c, 0x53, 0xcb, 0xc9, 
		0x12, 0x1e, 0x33, 0x74, 0x9e, 0x0c, 0xf4, 0xd5, 
		0xd4, 0x9f, 0xd4, 0xa4, 0x59, 0x7e, 0x35, 0xcf, 
		0x32, 0x22, 0xf4, 0xcc, 0xcf, 0xd3, 0x90, 0x2d, 
		0x48, 0xd3, 0x8f, 0x75, 0xe6, 0xd9, 0x1d, 0x2a, 
		0xe5, 0xc0, 0xf7, 0x2b, 0x78, 0x81, 0x87, 0x44, 
		0x0e, 0x5f, 0x50, 0x00, 0xd4, 0x61, 0x8d, 0xbe, 
		0x7b, 0x05, 0x15, 0x07, 0x3b, 0x33, 0x82, 0x1f, 
		0x18, 0x70, 0x92, 0xda, 0x64, 0x54, 0xce, 0xb1, 
		0x85, 0x3e, 0x69, 0x15, 0xf8, 0x46, 0x6a, 0x04, 
		0x96, 0x73, 0x0e, 0xd9, 0x16, 0x2f, 0x67, 0x68, 
		0xd4, 0xf7, 0x4a, 0x4a, 0xd0, 0x57, 0x68, 0x76, 
		0xfa, 0x16, 0xbb, 0x11, 0xad, 0xae, 0x24, 0x88, 
		0x79, 0xfe, 0x52, 0xdb, 0x25, 0x43, 0xe5, 0x3c, 
		0xf4, 0x45, 0xd3, 0xd8, 0x28, 0xce, 0x0b, 0xf5, 
		0xc5, 0x60, 0x59, 0x3d, 0x97, 0x27, 0x8a, 0x59, 
		0x76, 0x2d, 0xd0, 0xc2, 0xc9, 0xcd, 0x68, 0xd4, 
		0x49, 0x6a, 0x79, 0x25, 0x08, 0x61, 0x40, 0x14, 
		0xb1, 0x3b, 0x6a, 0xa5, 0x11, 0x28, 0xc1, 0x8c, 
		0xd6, 0xa9, 0x0b, 0x87, 0x97, 0x8c, 0x2f, 0xf1, 
};

static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static u8 *base_pic;
static DWORD base_pic_len;
static DWORD base_width, base_height;

static void pic1_uncompress(u8 *in, u8 *out, DWORD out_len)
{
	for (DWORD k = 0; k < out_len; ++k) {
		DWORD f = *(u32 *)in;
		DWORD off = f & 0xffffff;
		in += 3;
		BYTE *dst = out + off * 3;
		*dst++ = *in++;
		*dst++ = *in++;
		*dst++ = *in++;
	}
}

static void pic2_uncompress(u8 *in, u8 *out, DWORD out_len)
{
	for (DWORD k = 0; k < out_len; ++k) {
		DWORD f = *(u32 *)in;
		DWORD off = f & 0x7ffff;
		DWORD pix = ((f >> 19) & 0x1f) + 1;
		in += 3;
		u8 *dst = out + 3 * off;
		for (DWORD i = 0; i < pix; ++i) {
			*dst++ = *in++;
			*dst++ = *in++;
			*dst++ = *in++;
		}
	}
}

static void pic5_uncompress(pic_header_t *in, BYTE *out)
{
	u32 *comp0 = (u32 *)(in + 1);
	u8 *comp1 = (u8 *)comp0 + in->comp0_length;
	u8 *comp2 = comp1 + in->comp1_length;
	int i = 0;
	int x = 0;
	BYTE *org_out = out;

	while (1) {
		DWORD cp, off;
		int type = (comp0[x / 32] >> (x & 0x1e)) & 3;

		if (type == 0)
			*out++ = *comp1++;
		else if (type == 1) {
			cp = ((comp2[i / 2] >> (4 * (i & 1))) & 0xf) + 2;
			off = *comp1++ + 2;
			memcpy(out, out - off, cp);
			out += cp;
			++i;
		} else if (type == 2) {
			cp = *(u16 *)comp1;
			if (!cp)
				break;
			off = (cp & 0xfff) + 2;
			cp = (cp >> 12) + 2;
			memcpy(out, out - off, cp);
			out += cp;
 			comp1 += 2;
		} else /* type == 3 */ {
			off = (((comp2[i / 2] << (4 * (2 - (i & 1)))) & 0xf00) | *(u8 *)comp1) + 2;
			cp = (*(u16 *)comp1 >> 8) + 18;
			memcpy(out, out - off, cp);
			out += cp;
			comp1 += 2;
			++i;
		}
		x += 2;
	}
}

static void pic6_uncompress(pic_header_t *in, BYTE *out)
{
	DWORD dib_len = in->width * in->height * 3;
	if (dib_len > 0) {
		BYTE *p = (BYTE *)(in + 1);
		BYTE *pp = &pic2_conv_table[in->key & 0x3f];
		for (DWORD i = 0; i < dib_len; ++i) {
			out[i] = p[i] - pp[i & 0xff];
		}
	}
}

static void pic8_uncompress(pic_header_t *in, BYTE *out)
{
	u32 *comp0 = (u32 *)(in + 1);
	u8 *comp1 = (u8 *)comp0 + in->comp0_length;
	u8 *comp2 = comp1 + in->comp1_length;
	int i = 0;
	int x = 0;

	while (1) {
		DWORD cp, off;
		int type = (comp0[x / 32] >> (x & 0x1e)) & 3;

		if (type == 0) {
			*(u32 *)out = *(u32 *)comp1;
			comp1 += 3;
			out += 3;
		} else if (type == 1) {
			DWORD tmp;

			tmp = *comp1++ + ((comp2[i / 2] << (4 * (2 - (i & 1)))) & 0xf00);
			cp = tmp >> 10;
			off = ((tmp & 0x3ff) + 1) * 3;
			if (cp == 0) {
				*(u32 *)out = *(u32 *)(out - off);
				out += 3;
			} else if (cp == 1) {
				*(u32 *)out = *(u32 *)(out - off);
				*(u32 *)(out + 4) = *(u32 *)(out - off + 4);
				out += 6;
			} else if (cp == 2) {
				*(u32 *)out = *(u32 *)(out - off);
				*(u32 *)(out + 4) = *(u32 *)(out - off + 4);
				*(u32 *)(out + 8) = *(u32 *)(out - off + 8);
				*(u32 *)(out + 12)= *(u32 *)(out - off + 12);
				out += 9;
			} else /* cp == 3 */ {
				*(u32 *)out = *(u32 *)(out - off);
				*(u32 *)(out + 4) = *(u32 *)(out - off + 4);
				*(u32 *)(out + 8) = *(u32 *)(out - off + 8);
				*(u32 *)(out + 12)= *(u32 *)(out - off + 12);
				out += 12;
			}
			++i;
		} else if (type == 2) {
			cp = *(u16 *)comp1 >> 14;
			off = ((*(u16 *)comp1 & 0x3fff) + 1) * 3;
			comp1 += 2;
			if (cp == 0) {
				*(u32 *)out = *(u32 *)(out - off);
				out += 3;
			} else if (cp == 1) {
				*(u32 *)out = *(u32 *)(out - off);
				*(u32 *)(out + 4) = *(u32 *)(out - off + 4);
				out += 6;
			} else if (cp == 2) {
				*(u32 *)out = *(u32 *)(out - off);
				*(u32 *)(out + 4) = *(u32 *)(out - off + 4);
				*(u32 *)(out + 8) = *(u32 *)(out - off + 8);
				*(u32 *)(out + 12) = *(u32 *)(out - off + 12);
				out += 9;
			} else /* cp == 3 */ {
				*(u32 *)out = *(u32 *)(out - off);
				*(u32 *)(out + 4) = *(u32 *)(out - off + 4);
				*(u32 *)(out + 8) = *(u32 *)(out - off + 8);
				*(u32 *)(out + 12) = *(u32 *)(out - off + 12);
				out += 12;
			}
		} else /* type == 3 */ {
			DWORD tmp;				
			
			tmp = ((comp2[i / 2] << (4 * (4 - (i & 1)))) & 0xf0000) | *(u16 *)comp1;
			if (!tmp)
				break;

			cp = ((tmp >> 14) + 5) * 3;
			off = ((tmp & 0x3fff) + 1) * 3;
			BYTE *src = out - off;
			for (DWORD j = 0; j < cp; ++j)
				*out++ = *src++;
			comp1 += 2;
			++i;
		}
		x += 2;
	}	
}

static void pic9_uncompress(pic_header_t *in, BYTE *out)
{
	u32 *comp0 = (u32 *)(in + 1);
	u8 *comp1 = (u8 *)comp0 + in->comp0_length;
	u8 *comp2 = comp1 + in->comp1_length;
	int i = 0;
	int x = 0;

	while (1) {
		DWORD cp, off;
		int type = (comp0[x / 32] >> (x & 0x1e)) & 3;

		if (type == 0) {
			x += 2;
			cp = (comp0[x / 32] >> (x & 0x1e)) & 3;

			if (cp == 0) {
				*(u32 *)out = *(u32 *)comp1;
				out += 3;
				comp1 += 3;
			} else if (cp == 1) {
				*(u32 *)out = *(u32 *)comp1;
				*(u32 *)(out + 4) = *(u32 *)(comp1 + 4);
				out += 6;
				comp1 += 6;
			} else if (cp == 2) {
				*(u32 *)out = *(u32 *)comp1;
				*(u32 *)(out + 4) = *(u32 *)(comp1 + 4);
				*(u32 *)(out + 8) = *(u32 *)(comp1 + 8);
				*(u32 *)(out + 12) = *(u32 *)(comp1 + 12);
				out += 9;
				comp1 += 9;
			} else /* cp == 3 */ {
				*(u32 *)out = *(u32 *)comp1;
				*(u32 *)(out + 4) = *(u32 *)(comp1 + 4);
				*(u32 *)(out + 8) = *(u32 *)(comp1 + 8);
				*(u32 *)(out + 12) = *(u32 *)(comp1 + 12);
				out += 12;
				comp1 += 12;
			}
		} else if (type == 1) {
			DWORD tmp;

			tmp = *comp1++ + ((comp2[i / 2] << (4 * (2 - (i & 1)))) & 0xf00);
			cp = tmp >> 10;
			off = ((tmp & 0x3ff) + 1) * 3;
			if (cp == 0) {
				*(u32 *)out = *(u32 *)(out - off);
				out += 3;
			} else if (cp == 1) {
				*(u32 *)out = *(u32 *)(out - off);
				*(u32 *)(out + 4) = *(u32 *)(out - off + 4);
				out += 6;
			} else if (cp == 2) {
				*(u32 *)out = *(u32 *)(out - off);
				*(u32 *)(out + 4) = *(u32 *)(out - off + 4);
				*(u32 *)(out + 8) = *(u32 *)(out - off + 8);
				*(u32 *)(out + 12)= *(u32 *)(out - off + 12);
				out += 9;
			} else /* cp == 3 */ {
				*(u32 *)out = *(u32 *)(out - off);
				*(u32 *)(out + 4) = *(u32 *)(out - off + 4);
				*(u32 *)(out + 8) = *(u32 *)(out - off + 8);
				*(u32 *)(out + 12)= *(u32 *)(out - off + 12);
				out += 12;
			}
			++i;
		} else if (type == 2) {
			cp = *(u16 *)comp1 >> 14;
			off = ((*(u16 *)comp1 & 0x3fff) + 1) * 3;
			comp1 += 2;
			if (cp == 0) {
				*(u32 *)out = *(u32 *)(out - off);
				out += 3;
			} else if (cp == 1) {
				*(u32 *)out = *(u32 *)(out - off);
				*(u32 *)(out + 4) = *(u32 *)(out - off + 4);
				out += 6;
			} else if (cp == 2) {
				*(u32 *)out = *(u32 *)(out - off);
				*(u32 *)(out + 4) = *(u32 *)(out - off + 4);
				*(u32 *)(out + 8) = *(u32 *)(out - off + 8);
				*(u32 *)(out + 12) = *(u32 *)(out - off + 12);
				out += 9;
			} else /* cp == 3 */ {
				*(u32 *)out = *(u32 *)(out - off);
				*(u32 *)(out + 4) = *(u32 *)(out - off + 4);
				*(u32 *)(out + 8) = *(u32 *)(out - off + 8);
				*(u32 *)(out + 12) = *(u32 *)(out - off + 12);
				out += 12;
			}
		} else /* type == 3 */ {
			DWORD tmp;				
			
			tmp = ((comp2[i / 2] << (4 * (4 - (i & 1)))) & 0xf0000) | *(u16 *)comp1;
			if (!tmp)
				break;

			cp = ((tmp >> 14) + 5) * 3;
			off = ((tmp & 0x3fff) + 1) * 3;
			BYTE *src = out - off;
			for (DWORD j = 0; j < cp; ++j)
				*out++ = *src++;
			comp1 += 2;
			++i;
		}
		x += 2;
	}	
}

static int pic_uncompress(BYTE *in, DWORD in_len, BYTE **ret_out, DWORD *ret_out_len)
{
	u8 mode = *in;

	DWORD out_len;
	BYTE *out;
	if (mode != 1 && mode != 2) {
		DWORD width = *(u16 *)(in + 2);
		DWORD height = *(u16 *)(in + 4);
		out_len = (width * height + 96) * 3;
		out = (BYTE *)malloc(out_len);
		if (!out)
			return -CUI_EMEM;

		if (base_pic) {
			free(base_pic);
			base_pic = NULL;
		}

		if (!base_pic) {
			base_pic_len = out_len;
			base_pic = (BYTE *)malloc(base_pic_len);
			if (!base_pic) {
				free(out);
				return -CUI_EMEM;
			}
			base_width = width;
			base_height = height;
		}
	} else {
		out_len = *(u32 *)(in + 4);
		out = (BYTE *)malloc(base_pic_len);
		if (!out) {
			free(out);
			return -CUI_EMEM;
		}
		memcpy(out, base_pic, base_pic_len);
	}

	switch (mode) {
	case 1:
		pic1_uncompress(in + 8, out, out_len);
		break;
	case 2:
		pic2_uncompress(in + 8, out, out_len);
		break;
	case 5:
		pic5_uncompress((pic_header_t *)in, out);
		break;
	case 6:
		pic6_uncompress((pic_header_t *)in, out);
		break;
	case 8:
		pic8_uncompress((pic_header_t *)in, out);
		break;
	case 9:
		pic9_uncompress((pic_header_t *)in, out);
		break;
	default:
		delete [] out;
		printf("unsupported %x\n", mode);
		return -CUI_EMATCH;
	}

	if (MyBuildBMPFile(out, base_pic_len, NULL, 0, base_width, 0 - base_height, 
			24, ret_out, ret_out_len, my_malloc)) {
		free(out);
		return -CUI_EMEM;
	}

	if (mode != 1 && mode != 2)
		memcpy(base_pic, out, base_pic_len);
	free(out);

	return 0;
}

/********************* pd *********************/

/* 封包匹配回调函数 */
static int KAAS_pd_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包索引目录提取函数 */
static int KAAS_pd_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{
	pb_entry_t *index_buffer;
	my_pb_entry_t *my_index_buffer;
	DWORD index_buffer_length, index_entries;	
	u8 tmp[256];
	u32 tmp2[4];

	if (pkg->pio->read(pkg, tmp, 2))
		return -CUI_EREAD;

	if (pkg->pio->read(pkg, tmp + 2, tmp[0] - 2))
		return -CUI_EREAD;

	if (pkg->pio->read(pkg, tmp2, 16))
		return -CUI_EREAD;

	index_entries = tmp2[0] & 0xfff;
	index_buffer_length = index_entries * sizeof(pb_entry_t);
	index_buffer = (pb_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	for (DWORD k = 14; k < index_buffer_length + 14; k++)
		((BYTE *)index_buffer)[k-14] -= (BYTE)(((k * 0x6b) % (k / 2 + 1)) + tmp[1] * 0x3b * (0x0b + k) * (k % (k + 0x11)));

	index_buffer_length = index_entries * sizeof(my_pb_entry_t);
	my_index_buffer = (my_pb_entry_t *)malloc(index_buffer_length);
	if (!my_index_buffer) {
		free(index_buffer);
		return -CUI_EMEM;
	}

	for (k = 0; k < index_entries; k++) {
		sprintf(my_index_buffer[k].name, "%08d", k);
		my_index_buffer[k].offset = index_buffer[k].offset;
		my_index_buffer[k].length = index_buffer[k].length;
	}
	free(index_buffer);

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(my_pb_entry_t);
	pkg_dir->flags |= PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int KAAS_pd_parse_resource_info(struct package *pkg,
									   struct package_resource *pkg_res)
{
	my_pb_entry_t *my_pb_entry;

	my_pb_entry = (my_pb_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_pb_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_pb_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_pb_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int KAAS_pd_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	pkg_res->raw_data = malloc(pkg_res->raw_data_length);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EREADVEC;
	}

	int ret = pic_uncompress((BYTE *)pkg_res->raw_data, pkg_res->raw_data_length,
		(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length);

	if (!ret) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp");
	} else if (ret == -CUI_EMATCH)
		ret = 0;

	return ret;
}

/* 资源保存函数 */
static int KAAS_pd_save_resource(struct resource *res, 
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
static void KAAS_pd_release_resource(struct package *pkg, 
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
static void KAAS_pd_release(struct package *pkg,
							struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);

	if (base_pic) {
		free(base_pic);
		base_pic = NULL;
	}
}

/* 封包处理回调函数集合 */
static cui_ext_operation KAAS_pd_operation = {
	KAAS_pd_match,					/* match */
	KAAS_pd_extract_directory,		/* extract_directory */
	KAAS_pd_parse_resource_info,	/* parse_resource_info */
	KAAS_pd_extract_resource,		/* extract_resource */
	KAAS_pd_save_resource,			/* save_resource */
	KAAS_pd_release_resource,		/* release_resource */
	KAAS_pd_release					/* release */
};

/********************* pb *********************/

/* 封包匹配回调函数 */
static int KAAS_pb_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包索引目录提取函数 */
static int KAAS_pb_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{
	pb_header_t pb_header;
	pb_entry_t *index_buffer;
	my_pb_entry_t *my_index_buffer;
	unsigned int index_buffer_length;	
	unsigned int i;

	if (pkg->pio->read(pkg, &pb_header, sizeof(pb_header)))
		return -CUI_EREAD;

	index_buffer_length = pb_header.index_entries * sizeof(pb_entry_t);
	index_buffer = (pb_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	index_buffer_length = pb_header.index_entries * sizeof(my_pb_entry_t);
	my_index_buffer = (my_pb_entry_t *)malloc(index_buffer_length);
	if (!my_index_buffer) {
		free(index_buffer);
		return -CUI_EMEM;
	}

	for (i = 0; i < pb_header.index_entries; i++) {
		sprintf(my_index_buffer[i].name, "%08d", i);
		my_index_buffer[i].offset = index_buffer[i].offset;
		my_index_buffer[i].length = index_buffer[i].length;
	}
	free(index_buffer);

	pkg_dir->index_entries = pb_header.index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(my_pb_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int KAAS_pb_parse_resource_info(struct package *pkg,
									   struct package_resource *pkg_res)
{
	my_pb_entry_t *my_pb_entry;

	my_pb_entry = (my_pb_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_pb_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_pb_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_pb_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int KAAS_pb_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	pkg_res->raw_data = malloc(pkg_res->raw_data_length);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EREADVEC;
	}

	voice_header_t *vh = (voice_header_t *)pkg_res->raw_data;
	if (!strncmp((char *)pkg_res->raw_data, "RIFF", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".wav");
	} else if (vh->length + sizeof(voice_header_t) == pkg_res->raw_data_length
			&& vh->unknown0 == 0x800) {
		BYTE *v_data = (BYTE *)(vh + 1);

		WORD *pcm = (WORD *)malloc(vh->length * 2);
		if (!pcm) {
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EMEM;
		}

		for (DWORD i = 0; i < vh->length; ++i)
			pcm[i] = decode_table[v_data[i]];

		if (MySaveAsWAVE(pcm, vh->length * 2, 1, 
				vh->channels + 1, vh->SamplesPerSec, 
				16, NULL, 0, (BYTE **)&pkg_res->actual_data, 
				(DWORD *)&pkg_res->actual_data_length, my_malloc)) {
			free(pcm);
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EMEM;
		}
		free(pcm);
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".wav");
	} else if (!lstrcmp(pkg->name, L"voice.pb")) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT | PKG_RES_FLAG_FIO;
		pkg_res->replace_extension = _T(".pb");
	}

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation KAAS_pb_operation = {
	KAAS_pb_match,					/* match */
	KAAS_pb_extract_directory,		/* extract_directory */
	KAAS_pb_parse_resource_info,	/* parse_resource_info */
	KAAS_pb_extract_resource,		/* extract_resource */
	KAAS_pd_save_resource,			/* save_resource */
	KAAS_pd_release_resource,		/* release_resource */
	KAAS_pd_release					/* release */
};

/********************* id *********************/

/* 封包匹配回调函数 */
static int KAAS_id_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包资源提取函数 */
static int KAAS_id_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	u32 v0, v1;
	u32 fsize;
	BYTE *id;

	if (pkg->pio->length_of(pkg, &fsize))
		return -CUI_ELEN;

	id = (BYTE *)malloc(fsize);
	if (!id)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, id, fsize, 0, IO_SEEK_SET)) {
		free(id);
		return -CUI_EREADVEC;
	}

	v0 = *(u32 *)id;
	v1 = *(u32 *)&id[4];
	if (v0 * 2 < v1 * 2) {
		u16 *tmp = (u16 *)&id[v0 * 4];
		DWORD count = v1 * 2 - v0 * 2;

		for (DWORD i = 0; i < count; i++) {
			if (tmp[i] & 1)
				tmp[i] &= 0xFFFE;
			else
				tmp[i] |= 1;
		}
	}
	pkg_res->raw_data = id;
	pkg_res->raw_data_length = fsize;

	return 0;
}

/* 资源保存函数 */
static int KAAS_id_save_resource(struct resource *res, 
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
static void KAAS_id_release_resource(struct package *pkg, 
									 struct package_resource *pkg_res)
{
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

/* 封包卸载函数 */
static void KAAS_id_release(struct package *pkg, 
							struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation KAAS_id_operation = {
	KAAS_id_match,					/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	KAAS_id_extract_resource,		/* extract_resource */
	KAAS_id_save_resource,			/* save_resource */
	KAAS_id_release_resource,		/* release_resource */
	KAAS_id_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK KAAS_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".pd"), NULL, 
		NULL, &KAAS_pd_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR 
		| CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".pb"), NULL, 
		NULL, &KAAS_pb_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES |
		CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_RECURSION | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".id"), NULL, 
		NULL, &KAAS_id_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	return 0;
}
