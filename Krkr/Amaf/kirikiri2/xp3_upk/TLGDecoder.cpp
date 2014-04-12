#include "TLGDecoder.h"
#include "my_headers.h"

#define TVP_TLG6_H_BLOCK_SIZE 8
#define TVP_TLG6_W_BLOCK_SIZE 8

#define TVP_TLG6_GOLOMB_N_COUNT  4
#define TVP_TLG6_LeadingZeroTable_BITS 12
#define TVP_TLG6_LeadingZeroTable_SIZE  (1 << TVP_TLG6_LeadingZeroTable_BITS)
static BYTE TVPTLG6LeadingZeroTable[TVP_TLG6_LeadingZeroTable_SIZE];
static short int TVPTLG6GolombCompressed[TVP_TLG6_GOLOMB_N_COUNT][9] =
{
    {3,7,15,27,63,108,223,448,130,},
    {3,5,13,24,51,95,192,384,257,},
    {2,5,12,21,39,86,155,320,384,},
    {2,3,9,18,33,61,129,258,511,},
    /* Tuned by W.Dee, 2004/03/25 */
};

static char TVPTLG6GolombBitLengthTable[TVP_TLG6_GOLOMB_N_COUNT*2*128][TVP_TLG6_GOLOMB_N_COUNT] =
{
    { 0 }
};

void TVPFillARGB(u32 *dest, int len, u32 value)
{
    int ___index = 0;

    len -= (8-1);

    while (___index < len) {
        dest[(___index+0)] = value;
        dest[(___index+1)] = value;
        dest[(___index+2)] = value;
        dest[(___index+3)] = value;
        dest[(___index+4)] = value;
        dest[(___index+5)] = value;
        dest[(___index+6)] = value;
        dest[(___index+7)] = value;
        ___index += 8;
    }

    len += (8-1);

    while (___index < len)
        dest[___index++] = value;
}

#define TVP_TLG6_FETCH_32BITS(addr) (*(u32 *)addr)

void
TVPTLG6DecodeGolombValuesForFirst(
    char *pixelbuf,
    int pixel_count,
    BYTE *bit_pool
)
{
	/*
		decode values packed in "bit_pool".
		values are coded using golomb code.

		"ForFirst" function do dword access to pixelbuf,
		clearing with zero except for blue (least siginificant byte).
	*/

	int n = TVP_TLG6_GOLOMB_N_COUNT - 1; /* output counter */
	int a = 0; /* summary of absolute values of errors */

	int bit_pos = 1;
	BYTE zero = (*bit_pool & 1) ? 0 : 1;

	char *limit = pixelbuf + pixel_count * 4;
	while (pixelbuf < limit) {
		/* get running count */
		int count;

		{
			u32 t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
			int b = TVPTLG6LeadingZeroTable[t & (TVP_TLG6_LeadingZeroTable_SIZE - 1)];
			int bit_count = b;

			while (!b) {
				bit_count += TVP_TLG6_LeadingZeroTable_BITS;
				bit_pos += TVP_TLG6_LeadingZeroTable_BITS;
				bit_pool += bit_pos >> 3;
				bit_pos &= 7;
				t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
				b = TVPTLG6LeadingZeroTable[t & (TVP_TLG6_LeadingZeroTable_SIZE - 1)];
				bit_count += b;
			}

			bit_pos += b;
			bit_pool += bit_pos >> 3;
			bit_pos &= 7;

			bit_count--;
			count = 1 << bit_count;
			count += ((TVP_TLG6_FETCH_32BITS(bit_pool) >> (bit_pos)) & (count - 1));

			bit_pos += bit_count;
			bit_pool += bit_pos >> 3;
			bit_pos &= 7;
		}

		if (zero) {
			/* zero values */

			/* fill distination with zero */
			do { *(u32 *)pixelbuf = 0; pixelbuf += 4; } while (--count);

			zero ^= 1;
		} else {
			/* non-zero values */

			/* fill distination with glomb code */

			do {
				int k = TVPTLG6GolombBitLengthTable[a][n], v, sign;

				u32 t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
				int bit_count;
				int b;
				if (t) {
					b = TVPTLG6LeadingZeroTable[t & (TVP_TLG6_LeadingZeroTable_SIZE-1)];
					bit_count = b;
					while (!b) {
						bit_count += TVP_TLG6_LeadingZeroTable_BITS;
						bit_pos += TVP_TLG6_LeadingZeroTable_BITS;
						bit_pool += bit_pos >> 3;
						bit_pos &= 7;
						t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
						b = TVPTLG6LeadingZeroTable[t & (TVP_TLG6_LeadingZeroTable_SIZE - 1)];
						bit_count += b;
					}
					bit_count--;
				} else {
					bit_pool += 5;
					bit_count = bit_pool[-1];
					bit_pos = 0;
					t = TVP_TLG6_FETCH_32BITS(bit_pool);
					b = 0;
				}

				v = (bit_count << k) + ((t >> b) & ((1 << k) - 1));
				sign = (v & 1) - 1;
				v >>= 1;
				a += v;
				*(u32 *)pixelbuf = (unsigned char)((v ^ sign) + sign + 1);
				pixelbuf += 4;

				bit_pos += b;
				bit_pos += k;
				bit_pool += bit_pos >> 3;
				bit_pos &= 7;

				if (--n < 0) {
					a >>= 1;
					n = TVP_TLG6_GOLOMB_N_COUNT - 1;
				}
			} while (--count);
			zero ^= 1;
		}
	}
}

static void TVPTLG6DecodeGolombValues(char *pixelbuf,
									  int pixel_count,
									  BYTE *bit_pool)
{
	/*
		decode values packed in "bit_pool".
		values are coded using golomb code.
	*/

	int n = TVP_TLG6_GOLOMB_N_COUNT - 1; /* output counter */
	int a = 0; /* summary of absolute values of errors */

	int bit_pos = 1;
	BYTE zero = (*bit_pool & 1) ? 0 : 1;

	char *limit = pixelbuf + pixel_count * 4;

	while (pixelbuf < limit) {
		/* get running count */
		int count;

		{
			u32 t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
			int b = TVPTLG6LeadingZeroTable[t & (TVP_TLG6_LeadingZeroTable_SIZE-1)];
			int bit_count = b;
			while (!b) {
				bit_count += TVP_TLG6_LeadingZeroTable_BITS;
				bit_pos += TVP_TLG6_LeadingZeroTable_BITS;
				bit_pool += bit_pos >> 3;
				bit_pos &= 7;
				t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
				b = TVPTLG6LeadingZeroTable[t & (TVP_TLG6_LeadingZeroTable_SIZE - 1)];
				bit_count += b;
			}

			bit_pos += b;
			bit_pool += bit_pos >> 3;
			bit_pos &= 7;

			bit_count--;
			count = 1 << bit_count;
			count += ((TVP_TLG6_FETCH_32BITS(bit_pool) >> (bit_pos)) & (count - 1));

			bit_pos += bit_count;
			bit_pool += bit_pos >> 3;
			bit_pos &= 7;
		}

		if (zero) {
			/* zero values */

			/* fill distination with zero */
			do { *pixelbuf = 0; pixelbuf+=4; } while(--count);

			zero ^= 1;
		} else {
			/* non-zero values */

			/* fill distination with glomb code */

			do {
				int k = TVPTLG6GolombBitLengthTable[a][n], v, sign;

				u32 t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
				int bit_count;
				int b;
				if (t) {
					b = TVPTLG6LeadingZeroTable[t & (TVP_TLG6_LeadingZeroTable_SIZE - 1)];
					bit_count = b;
					while (!b) {
						bit_count += TVP_TLG6_LeadingZeroTable_BITS;
						bit_pos += TVP_TLG6_LeadingZeroTable_BITS;
						bit_pool += bit_pos >> 3;
						bit_pos &= 7;
						t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
						b = TVPTLG6LeadingZeroTable[t & (TVP_TLG6_LeadingZeroTable_SIZE - 1)];
						bit_count += b;
					}
					bit_count--;
				} else {
					bit_pool += 5;
					bit_count = bit_pool[-1];
					bit_pos = 0;
					t = TVP_TLG6_FETCH_32BITS(bit_pool);
					b = 0;
				}

				v = (bit_count << k) + ((t >> b) & ((1 <<k ) - 1));
				sign = (v & 1) - 1;
				v >>= 1;
				a += v;
				*pixelbuf = (char)((v ^ sign) + sign + 1);
				pixelbuf += 4;

				bit_pos += b;
				bit_pos += k;
				bit_pool += bit_pos >> 3;
				bit_pos &= 7;

				if (--n < 0) {
					a >>= 1;
					n = TVP_TLG6_GOLOMB_N_COUNT - 1;
				}
			} while (--count);
			zero ^= 1;
		}
	}
}

u32 make_gt_mask(u32 a, u32 b)
{
    u32 tmp2 = ~b;
    u32 tmp = ((a & tmp2) + (((a ^ tmp2) >> 1) & 0x7f7f7f7f)) & 0x80808080;
    tmp = ((tmp >> 7) + 0x7f7f7f7f) ^ 0x7f7f7f7f;
    return tmp;
}

u32 packed_bytes_add(u32 a, u32 b)
{
    u32 tmp = (((a & b) << 1) + ((a ^ b) & 0xfefefefe) ) & 0x01010100;
    return a + b - tmp;
}

u32 med2(u32 a, u32 b, u32 c)
{
    /* do Median Edge Detector   thx, Mr. sugi  at    kirikiri.info */
    u32 aa_gt_bb = make_gt_mask(a, b);
    u32 a_xor_b_and_aa_gt_bb = ((a ^ b) & aa_gt_bb);
    u32 aa = a_xor_b_and_aa_gt_bb ^ a;
    u32 bb = a_xor_b_and_aa_gt_bb ^ b;
    u32 n = make_gt_mask(c, bb);
    u32 nn = make_gt_mask(aa, c);
    u32 m = ~(n | nn);
    return (n & aa) | (nn & bb) | ((bb & m) - (c & m) + (aa & m));
}

u32 med(u32 a, u32 b, u32 c, u32 v)
{
    return packed_bytes_add(med2(a, b, c), v);
}

#define TLG6_AVG_PACKED(x, y) ((((x) & (y)) + ((((x) ^ (y)) & 0xfefefefe) >> 1)) +\
(((x)^(y))&0x01010101))

u32 avg(u32 a, u32 b, u32 c, u32 v)
{
    UNREFERENCED_PARAMETER(c);
    return packed_bytes_add(TLG6_AVG_PACKED(a, b), v);
}

#define TVP_TLG6_DO_CHROMA_DECODE_PROTO(B, G, R, A, POST_INCREMENT) do \
			{ \
				u32 u = *prevline; \
				p = med(p, u, up, \
					(0xff0000 & ((R) << 16)) + (0xff00 & ((G) << 8)) + (0xff & (B)) + ((A) << 24) ); \
				up = u; \
				*curline = p; \
				curline++; \
				prevline++; \
				POST_INCREMENT \
			} while (--w);
#define TVP_TLG6_DO_CHROMA_DECODE_PROTO2(B, G, R, A, POST_INCREMENT) do \
			{ \
				u32 u = *prevline; \
				p = avg(p, u, up, \
					(0xff0000 & ((R) << 16)) + (0xff00 & ((G) << 8)) + (0xff & (B)) + ((A) << 24) ); \
				up = u; \
				*curline = p; \
				curline ++; \
				prevline ++; \
				POST_INCREMENT \
			} while (--w);

#define TVP_TLG6_DO_CHROMA_DECODE(N, R, G, B) case (N << 1): \
	TVP_TLG6_DO_CHROMA_DECODE_PROTO(R, G, B, IA, {in += step;}) break; \
	case (N << 1) + 1: \
	TVP_TLG6_DO_CHROMA_DECODE_PROTO2(R, G, B, IA, {in += step;}) break;

void
TVPTLG6DecodeLineGeneric(
    u32 *prevline,
    u32 *curline,
    int width,
    int start_block,
    int block_limit,
    BYTE *filtertypes,
    int skipblockbytes,
    u32 *in,
    u32 initialp,
    int oddskip,
    int dir
)
{
	/*
		chroma/luminosity decoding
		(this does reordering, color correlation filter, MED/AVG  at a time)
	*/
	u32 p, up;
	int step, i;

	if (start_block) {
		prevline += start_block * TVP_TLG6_W_BLOCK_SIZE;
		curline  += start_block * TVP_TLG6_W_BLOCK_SIZE;
		p  = curline[-1];
		up = prevline[-1];
	} else {
		p = up = initialp;
	}

	in += skipblockbytes * start_block;
	step = (dir & 1) ? 1 : -1;

	for (i = start_block; i < block_limit; i++) {
		int w = width - i * TVP_TLG6_W_BLOCK_SIZE, ww;
		if (w > TVP_TLG6_W_BLOCK_SIZE) w = TVP_TLG6_W_BLOCK_SIZE;
		ww = w;
		if (step == -1) in += ww-1;
		if (i & 1) in += oddskip * ww;
		switch (filtertypes[i])	{
#define IA	(char)((*in >> 24) & 0xff)
#define IR	(char)((*in >> 16) & 0xff)
#define IG  (char)((*in >> 8 ) & 0xff)
#define IB  (char)((*in      ) & 0xff)
			TVP_TLG6_DO_CHROMA_DECODE( 0, IB, IG, IR);
			TVP_TLG6_DO_CHROMA_DECODE( 1, IB+IG, IG, IR+IG);
			TVP_TLG6_DO_CHROMA_DECODE( 2, IB, IG+IB, IR+IB+IG);
			TVP_TLG6_DO_CHROMA_DECODE( 3, IB+IR+IG, IG+IR, IR);
			TVP_TLG6_DO_CHROMA_DECODE( 4, IB+IR, IG+IB+IR, IR+IB+IR+IG);
			TVP_TLG6_DO_CHROMA_DECODE( 5, IB+IR, IG+IB+IR, IR);
			TVP_TLG6_DO_CHROMA_DECODE( 6, IB+IG, IG, IR);
			TVP_TLG6_DO_CHROMA_DECODE( 7, IB, IG+IB, IR);
			TVP_TLG6_DO_CHROMA_DECODE( 8, IB, IG, IR+IG);
			TVP_TLG6_DO_CHROMA_DECODE( 9, IB+IG+IR+IB, IG+IR+IB, IR+IB);
			TVP_TLG6_DO_CHROMA_DECODE(10, IB+IR, IG+IR, IR);
			TVP_TLG6_DO_CHROMA_DECODE(11, IB, IG+IB, IR+IB);
			TVP_TLG6_DO_CHROMA_DECODE(12, IB, IG+IR+IB, IR+IB);
			TVP_TLG6_DO_CHROMA_DECODE(13, IB+IG, IG+IR+IB+IG, IR+IB+IG);
			TVP_TLG6_DO_CHROMA_DECODE(14, IB+IG+IR, IG+IR, IR+IB+IG+IR);
			TVP_TLG6_DO_CHROMA_DECODE(15, IB, IG+(IB<<1), IR+(IB<<1));
		default:
			return;
		}
		if (step == 1)
			in += skipblockbytes - ww;
		else
			in += skipblockbytes + 1;
		if (i & 1) in -= oddskip * ww;
#undef IR
#undef IG
#undef IB
	}
}

void
TVPTLG6DecodeLine(
    u32 *prevline,
    u32 *curline,
    int width,
    int block_count,
    BYTE *filtertypes,
    int skipblockbytes,
    u32 *in,
    u32 initialp,
    int oddskip,
    int dir
)
{
    TVPTLG6DecodeLineGeneric(prevline, curline, width, 0, block_count,
        filtertypes, skipblockbytes, in, initialp, oddskip, dir);
}

void* my_malloc(DWORD Size)
{
    CMem m;

    return m.Alloc(Size);
}

int
MyBuildBMPFile(
    BYTE *dib,
    DWORD dib_length,
    BYTE *palette,
    DWORD palette_length,
    DWORD width,
    DWORD height,
    DWORD bits_count,
    BYTE **ret,
    DWORD *ret_length,
    void *(*alloc)(DWORD)
)
{
	BITMAPFILEHEADER *bmfh;
	BITMAPINFOHEADER *bmiHeader;
	DWORD act_height, aligned_line_length, raw_line_length;
	DWORD act_dib_length, act_palette_length, output_length;
	BYTE *pdib, *pal, *output;
	unsigned int colors, pixel_bytes;
/*
	if (alloc == (void *(*)(DWORD))malloc)
		return -1;
*/
	raw_line_length = width * bits_count / 8;
	aligned_line_length = (width * bits_count / 8 + 3) & ~3;
	if (raw_line_length != aligned_line_length)
    {
        return -1;
	}

	if (bits_count <= 8) {
		if (palette_length) {
			if (!palette || palette_length == 1024)
				colors = 256;
			else
				colors = palette_length / 3;
		} else
			colors = 1 << bits_count;
	} else
		colors = 0;
	act_palette_length = colors * 4;

	pixel_bytes = bits_count / 8;

	if (!(height & 0x80000000))
		act_height = height;
	else
		act_height = 0 - height;
	act_dib_length = aligned_line_length * act_height;

	output_length = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + act_palette_length + act_dib_length;

	output = (BYTE *)alloc(output_length);
	if (!output)
		return -1;

	bmfh = (BITMAPFILEHEADER *)output;
	bmfh->bfType = 0x4D42;
	bmfh->bfSize = output_length;
	bmfh->bfReserved1 = 0;
	bmfh->bfReserved2 = 0;
	bmfh->bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + act_palette_length;
	bmiHeader = (BITMAPINFOHEADER *)(bmfh + 1);
	bmiHeader->biSize = sizeof(BITMAPINFOHEADER);
	bmiHeader->biWidth = width;
	bmiHeader->biHeight = act_height;
	bmiHeader->biPlanes = 1;
	bmiHeader->biBitCount = (WORD)bits_count;
	bmiHeader->biCompression = BI_RGB;
	bmiHeader->biSizeImage = act_dib_length;
	bmiHeader->biXPelsPerMeter = 0;
	bmiHeader->biYPelsPerMeter = 0;
	bmiHeader->biClrUsed = bits_count <= 8 ? colors : 0;
	bmiHeader->biClrImportant = 0;

	pal = (BYTE *)(bmiHeader + 1);
	if (bits_count <= 8) {
		unsigned int p;

		if (!palette || !palette_length) {
			for (p = 0; p < colors; p++) {
				pal[p * 4 + 0] = p;
				pal[p * 4 + 1] = p;
				pal[p * 4 + 2] = p;
				pal[p * 4 + 3] = 0;
			}
		} else if (palette_length != act_palette_length) {
			for (p = 0; p < colors; p++) {
				pal[p * 4 + 0] = palette[p * 3 + 0];
				pal[p * 4 + 1] = palette[p * 3 + 1];
				pal[p * 4 + 2] = palette[p * 3 + 2];
				pal[p * 4 + 3] = 0;
			}
		} else
			CopyMemory(pal, palette, palette_length);
	}

	pdib = pal + act_palette_length;
	/* 有些系统的bmp结尾会多出2字节 */
	if (dib_length > act_dib_length)
		dib_length = act_dib_length;

	if (dib_length == act_dib_length)
		raw_line_length = aligned_line_length;

	if (act_height == height) {
		for (unsigned int y = 0; y < act_height; ++y) {
			for (unsigned int x = 0; x < width; ++x) {
				for (unsigned int p = 0; p < pixel_bytes; ++p)
					pdib[y * aligned_line_length + x * pixel_bytes + p] = dib[y * raw_line_length + x * pixel_bytes + p];
			}
		}
	} else {
		for (unsigned int y = 0; y < act_height; ++y) {
			for (unsigned int x = 0; x < width; ++x) {
				for (unsigned int p = 0; p < pixel_bytes; ++p)
					pdib[y * aligned_line_length + x * pixel_bytes + p] = dib[(act_height - y - 1) * raw_line_length + x * pixel_bytes + p];
			}
		}
	}

#if 0
	if (pixel_bytes == 4) {
		BYTE *rgba = pdib;
		for (unsigned int y = 0; y < act_height; y++) {
			for (unsigned int x = 0; x < width; x++) {
				BYTE alpha = rgba[3];

				rgba[0] = (rgba[0] * alpha + 0xff * ~alpha) / 255;
				rgba[1] = (rgba[1] * alpha + 0xff * ~alpha) / 255;
				rgba[2] = (rgba[2] * alpha + 0xff * ~alpha) / 255;
				rgba += 4;
			}
		}
	}
#endif
	*ret = output;
	*ret_length = output_length;
	return 0;
}

int
tlg6_process(
    BYTE *tlg_raw_data,
    DWORD tlg_size,
    BYTE **ret_actual_data,
    DWORD *ret_actual_data_length
)
{
	tlg6_header_t *tlg6_header = (tlg6_header_t *)tlg_raw_data;
	int outsize;
	BYTE *out;
    CMem mem;

    UNREFERENCED_PARAMETER(tlg_size);

	outsize = tlg6_header->width * 4 * tlg6_header->height;
	out = (BYTE *)mem.Alloc(outsize);
	if (!out)
		return -1;

	int x_block_count = (int)((tlg6_header->width - 1) / TVP_TLG6_W_BLOCK_SIZE) + 1;
	int y_block_count = (int)((tlg6_header->height - 1) / TVP_TLG6_H_BLOCK_SIZE) + 1;
	int main_count = tlg6_header->width / TVP_TLG6_W_BLOCK_SIZE;
	int fraction = tlg6_header->width -  main_count * TVP_TLG6_W_BLOCK_SIZE;

	BYTE *bit_pool = (BYTE *)mem.Alloc((tlg6_header->max_bit_length / 8 + 5 + 3) & ~3);
	u32 *pixelbuf = (u32 *)mem.Alloc((4 * tlg6_header->width * TVP_TLG6_H_BLOCK_SIZE + 1 + 3) & ~3);
	BYTE *filter_types = (BYTE *)mem.Alloc((x_block_count * y_block_count + 3) & ~3);
	u32 *zeroline = (u32 *)mem.Alloc(tlg6_header->width * 4);
	if (!bit_pool || !pixelbuf || !filter_types || !zeroline) {
		mem.Free(out);
		return -1;
	}

	BYTE LZSS_text[4096];

	// initialize zero line (virtual y=-1 line)
	TVPFillARGB(zeroline, tlg6_header->width,
		tlg6_header->colors == 3 ? 0xff000000 : 0x00000000);
	// 0xff000000 for colors=3 makes alpha value opaque

	// initialize LZSS text (used by chroma filter type codes)
	u32 *p = (u32 *)LZSS_text;
	for (u32 i = 0; i < 32 * 0x01010101; i += 0x01010101) {
		for (u32 j = 0; j < 16 * 0x01010101; j += 0x01010101) {
			p[0] = i, p[1] = j, p += 2;
		}
	}

	// read chroma filter types.
	// chroma filter types are compressed via LZSS as used by TLG5.
	int inbuf_size = tlg6_header->filter_length;
	BYTE *inbuf = (BYTE *)(tlg6_header + 1);
	TVPTLG5DecompressSlide(filter_types, inbuf, inbuf_size,	LZSS_text, 0);

	BYTE *in_p = inbuf + inbuf_size;
	// for each horizontal block group ...
	u32 *prevline = zeroline;
	for (int y = 0; y < (int)tlg6_header->height; y += TVP_TLG6_H_BLOCK_SIZE) {
		int ylim = y + TVP_TLG6_H_BLOCK_SIZE;
		if (ylim >= (int)tlg6_header->height)
			ylim = tlg6_header->height;

		int pixel_count = (ylim - y) * tlg6_header->width;

		// decode values
		for (int c = 0; c < tlg6_header->colors; c++) {
			// read bit length
			int bit_length = *(u32 *)in_p;
			in_p += 4;

			// get compress method
			int method = (bit_length >> 30) & 3;
			bit_length &= 0x3fffffff;

			// compute byte length
			int byte_length = bit_length / 8;
			if (bit_length % 8)
				byte_length++;

			// read source from input
			CopyMemory(bit_pool, in_p, byte_length);
			in_p += byte_length;

			// decode values
			// two most significant bits of bitlength are
			// entropy coding method;
			// 00 means Golomb method,
			// 01 means Gamma method (not yet suppoted),
			// 10 means modified LZSS method (not yet supported),
			// 11 means raw (uncompressed) data (not yet supported).

			switch (method) {
			case 0:
				if (c == 0 && tlg6_header->colors != 1)
					TVPTLG6DecodeGolombValuesForFirst((char *)pixelbuf,
						pixel_count, bit_pool);
				else
					TVPTLG6DecodeGolombValues((char *)pixelbuf + c,
						pixel_count, bit_pool);
				break;
			default:
				if (byte_length & 3)
					mem.Free(bit_pool);
				mem.Free(zeroline);
				mem.Free(filter_types);
				mem.Free(pixelbuf);
				mem.Free(bit_pool);
				mem.Free(out);
				return -2;
			}
		}

		// for each line
		unsigned char *ft =
			filter_types + (y / TVP_TLG6_H_BLOCK_SIZE) * x_block_count;
		int skipbytes = (ylim - y) * TVP_TLG6_W_BLOCK_SIZE;

		for (int yy = y; yy < ylim; yy++) {
			u32 *curline = (u32 *)&out[yy * tlg6_header->width * 4];

			int dir = (yy & 1) ^ 1;
			int oddskip = ((ylim - yy - 1) - (yy - y));
			if (main_count) {
				int start =
					((tlg6_header->width < TVP_TLG6_W_BLOCK_SIZE) ? tlg6_header->width :
						TVP_TLG6_W_BLOCK_SIZE) * (yy - y);
				TVPTLG6DecodeLine(
					prevline,
					curline,
					tlg6_header->width,
					main_count,
					ft,
					skipbytes,
					pixelbuf + start, tlg6_header->colors == 3 ? 0xff000000 : 0,
					oddskip, dir);
			}

			if (main_count != x_block_count) {
				int ww = fraction;
				if (ww > TVP_TLG6_W_BLOCK_SIZE)
					ww = TVP_TLG6_W_BLOCK_SIZE;
				int start = ww * (yy - y);
				TVPTLG6DecodeLineGeneric(
					prevline,
					curline,
					tlg6_header->width,
					main_count,
					x_block_count,
					ft,
					skipbytes,
					pixelbuf + start,
					tlg6_header->colors == 3 ? 0xff000000 : 0,
					oddskip, dir);
			}
			prevline = curline;
		}
	}
	mem.Free(zeroline);
	mem.Free(filter_types);
	mem.Free(pixelbuf);
	mem.Free(bit_pool);

	BYTE *b = out;
	DWORD pixels = tlg6_header->width * tlg6_header->height;
	for (UInt i = 0; i < pixels; ++i) {
		b[0] = b[0] * b[3] / 255 + (255 - b[3]);
		b[1] = b[1] * b[3] / 255 + (255 - b[3]);
		b[2] = b[2] * b[3] / 255 + (255 - b[3]);
		b += 4;
	}

	if (MyBuildBMPFile(out, outsize, NULL, 0, tlg6_header->width,
			0 - tlg6_header->height, 32, ret_actual_data,
				ret_actual_data_length, my_malloc)) {
		mem.Free(out);
		return -1;
	}

    mem.Free(out);

	return 0;
}

void TVPTLG6InitLeadingZeroTable()
{
    /* table which indicates first set bit position + 1. */
    /* this may be replaced by BSF (IA32 instrcution). */
    
    int i;
    for(i = 0; i < TVP_TLG6_LeadingZeroTable_SIZE; i++)
    {
        int cnt = 0;
        int j;
        for(j = 1; j != TVP_TLG6_LeadingZeroTable_SIZE && !(i & j);
        j <<= 1, cnt++);
        cnt ++;
        if(j == TVP_TLG6_LeadingZeroTable_SIZE) cnt = 0;
        TVPTLG6LeadingZeroTable[i] = cnt;
    }
}

void TVPTLG6InitGolombTable()
{
    int n, i, j;
    for (n = 0; n < TVP_TLG6_GOLOMB_N_COUNT; n++)
    {
        int a = 0;
        for (i = 0; i < 9; i++)
        {
            for (j = 0; j < TVPTLG6GolombCompressed[n][i]; j++)
                TVPTLG6GolombBitLengthTable[a++][n] = (char)i;
        }
/*
        if (a != TVP_TLG6_GOLOMB_N_COUNT * 2 * 128)
            *(char *)0 = 0;
*/
    }
}

// ↑ copy from crass

BOOL DecodeTLG6(PVOID lpInBuffer, ULONG uInSize, PVOID *ppOutBuffer, PULONG pOutSize)
{
    static BOOL bInit = FALSE;

    if (!bInit)
    {
        bInit = TRUE;
        TVPTLG6InitLeadingZeroTable();
	    TVPTLG6InitGolombTable();
    }

    return !tlg6_process((PBYTE)lpInBuffer, uInSize, (PBYTE *)ppOutBuffer, (LPDWORD)pOutSize);
}

/************************************************************************/
/* TLG5                                                                 */
/************************************************************************/

Void TVPTLG5ComposeColors3To4(UInt8 *outp, const UInt8 *upper, UInt8 * const * buf, Int width)
{
    Int x;
    UInt8 pc[3];
    UInt8 c[3];
    pc[0] = pc[1] = pc[2] = 0;
    for(x = 0; x < width; x++)
    {
        c[0] = buf[0][x];
        c[1] = buf[1][x];
        c[2] = buf[2][x];
        c[0] += c[1]; c[2] += c[1];
        *(ULONG *)outp =
            ((((pc[0] += c[0]) + upper[0]) & 0xff)      ) +
            ((((pc[1] += c[1]) + upper[1]) & 0xff) <<  8) +
            ((((pc[2] += c[2]) + upper[2]) & 0xff) << 16) +
            0xff000000;
        outp += 4;
        upper += 4;
    }
}

Void TVPTLG5ComposeColors4To4(UInt8 *outp, const UInt8 *upper, UInt8 * const* buf, Int width)
{
    Int x;
    UInt8 pc[4];
    UInt8 c[4];
    pc[0] = pc[1] = pc[2] = pc[3] = 0;
    for(x = 0; x < width; x++)
    {
        c[0] = buf[0][x];
        c[1] = buf[1][x];
        c[2] = buf[2][x];
        c[3] = buf[3][x];
        c[0] += c[1]; c[2] += c[1];
        *(ULONG *)outp =
            ((((pc[0] += c[0]) + upper[0]) & 0xff)      ) +
            ((((pc[1] += c[1]) + upper[1]) & 0xff) <<  8) +
            ((((pc[2] += c[2]) + upper[2]) & 0xff) << 16) +
            ((((pc[3] += c[3]) + upper[3]) & 0xff) << 24);
        outp += 4;
        upper += 4;
    }
}

LONG TVPTLG5DecompressSlide(UInt8 *out, const UInt8 *in, LONG insize, UInt8 *text, LONG initialr)
{
    LONG r = initialr;
    UInt flags = 0;
    const UInt8 *inlim = in + insize;
    while(in < inlim)
    {
        if(((flags >>= 1) & 256) == 0)
        {
            flags = 0[in++] | 0xff00;
        }
        if(flags & 1)
        {
            LONG mpos = in[0] | ((in[1] & 0xf) << 8);
            LONG mlen = (in[1] & 0xf0) >> 4;
            in += 2;
            mlen += 3;
            if(mlen == 18) mlen += 0[in++];

            while(mlen--)
            {
                0[out++] = text[r++] = text[mpos++];
                mpos &= (4096 - 1);
                r &= (4096 - 1);
            }
        }
        else
        {
            unsigned char c = 0[in++];
            0[out++] = c;
            text[r++] = c;
            /*			0[out++] = text[r++] = 0[in++];*/
            r &= (4096 - 1);
        }
    }
    return r;
}

BOOL DecodeTLG5(PVOID lpInBuffer, ULONG uInSize, PVOID *ppOutBuffer, PULONG pOutSize)
{
    CMem mem;
    PBYTE pbInBuffer, pbOutBuffer;

    UNREFERENCED_PARAMETER(uInSize);

    pbInBuffer = (PBYTE)lpInBuffer;

	// load TLG v5.0 lossless compressed graphic

	UChar mark[12];
    KRKR2_TLG5_HEADER *pTLGHeader;
    SBitMapHeader BitmpHeader;
	Int width, height, colors, blockheight, stride;

    pTLGHeader = (KRKR2_TLG5_HEADER *)pbInBuffer;
    colors = pTLGHeader->Colors;
    if (colors != 3 && colors != 4)
        return FALSE;

	width  = pTLGHeader->Width;
	height = pTLGHeader->Height;
    blockheight = pTLGHeader->BlockHeight;

    InitBitmapHeader(&BitmpHeader, width, height, 32, &stride);
    pbOutBuffer = (PBYTE)mem.Alloc(stride * height + sizeof(BitmpHeader));
    if (pbOutBuffer == NULL)
        return FALSE;

    *ppOutBuffer = pbOutBuffer;
    *pOutSize = stride * height + sizeof(BitmpHeader);
    *(SBitMapHeader *)pbOutBuffer = BitmpHeader;
    pbOutBuffer += sizeof(BitmpHeader) + (height - 1) * stride;

	Int blockcount = (int)((height - 1) / blockheight) + 1;

	// skip block size section
    pbInBuffer = (PBYTE)&pTLGHeader->BlockOffset[blockcount];

	// decomperss
	UInt8 *inbuf = NULL;
	UInt8 *outbuf[4];
	UInt8 *text = NULL;
	Int r = 0;

	for(int i = 0; i < colors; i++)
        outbuf[i] = NULL;

    text = (UInt8*)mem.Alloc(4096);
    memset(text, 0, 4096);

    inbuf = (UInt8*)mem.Alloc(blockheight * width + 10);
    for(Int i = 0; i < colors; i++)
        outbuf[i] = (UInt8*)mem.Alloc(blockheight * width + 10);

    UInt8 *prevline = NULL;
    for(Int y_blk = 0; y_blk < height; y_blk += blockheight)
    {
        // read file and decompress
        for(Int c = 0; c < colors; c++)
        {
            Int size;

            mark[0] = *pbInBuffer++;
            size = *(PULONG)pbInBuffer;
            pbInBuffer += 4;
            if(mark[0] == 0)
            {
                // modified LZSS compressed data
                CopyMemory(inbuf, pbInBuffer, size);
                pbInBuffer += size;
                r = TVPTLG5DecompressSlide(outbuf[c], inbuf, size, text, r);
            }
            else
            {
                // raw data
                CopyMemory(outbuf[c], pbInBuffer, size);
                pbInBuffer += size;
            }
        }

        // compose colors and store
        Int y_lim = y_blk + blockheight;
        if(y_lim > height) y_lim = height;
        UInt8 * outbufp[4];
        for(Int c = 0; c < colors; c++) outbufp[c] = outbuf[c];
        for(Int y = y_blk; y < y_lim; y++)
        {
            UInt8 *current = pbOutBuffer;
            UInt8 *current_org = current;

            pbOutBuffer -= stride;
            if(prevline)
            {
                // not first line
                switch(colors)
                {
                case 3:
                    TVPTLG5ComposeColors3To4(current, prevline, outbufp, width);
                    outbufp[0] += width; outbufp[1] += width;
                    outbufp[2] += width;
                    break;
                case 4:
                    TVPTLG5ComposeColors4To4(current, prevline, outbufp, width);
                    outbufp[0] += width; outbufp[1] += width;
                    outbufp[2] += width; outbufp[3] += width;
                    break;
                }
            }
            else
            {
                // first line
                switch(colors)
                {
                case 3:
                    for(Int pr = 0, pg = 0, pb = 0, x = 0;
                    x < width; x++)
                    {
                        Int b = outbufp[0][x];
                        Int g = outbufp[1][x];
                        Int r = outbufp[2][x];
                        b += g; r += g;
                        0[current++] = pb += b;
                        0[current++] = pg += g;
                        0[current++] = pr += r;
                        0[current++] = 0xff;
                    }
                    outbufp[0] += width;
                    outbufp[1] += width;
                    outbufp[2] += width;
                    break;
                case 4:
                    for(Int pr = 0, pg = 0, pb = 0, pa = 0, x = 0;
                    x < width; x++)
                    {
                        Int b = outbufp[0][x];
                        Int g = outbufp[1][x];
                        Int r = outbufp[2][x];
                        Int a = outbufp[3][x];
                        b += g; r += g;
                        0[current++] = pb += b;
                        0[current++] = pg += g;
                        0[current++] = pr += r;
                        0[current++] = pa += a;
                    }
                    outbufp[0] += width;
                    outbufp[1] += width;
                    outbufp[2] += width;
                    outbufp[3] += width;
                    break;
                }
            }

            prevline = current_org;
        }
    }

	if(inbuf) mem.Free(inbuf);
	if(text) mem.Free(text);
	for(Int i = 0; i < colors; i++)
		if(outbuf[i]) mem.Free(outbuf[i]);

    return TRUE;
}