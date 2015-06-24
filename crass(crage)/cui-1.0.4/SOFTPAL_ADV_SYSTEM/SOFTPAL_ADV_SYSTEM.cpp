#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <resource.h>
#include <package.h>
#include <cui_error.h>
#include <stdio.h>
#include <utility.h>

/*
搜索decrypt密钥的方法：
密钥以常量的形式进行参数传递：
00429F5A  |.  FF15 50D14200 CALL DWORD PTR DS:[<&KERNEL32.ReadFile>] ; \ReadFile
00429F60  |.  8B55 F8       MOV EDX,DWORD PTR SS:[EBP-8]
00429F63  |.  52            PUSH EDX                                 ; /hObject
00429F64  |.  FF15 48D14200 CALL DWORD PTR DS:[<&KERNEL32.CloseHandl>; \CloseHandle
00429F6A  |.  8B45 08       MOV EAX,DWORD PTR SS:[EBP+8]
00429F6D  |.  8B88 28190700 MOV ECX,DWORD PTR DS:[EAX+71928]
00429F73  |.  0FB611        MOVZX EDX,BYTE PTR DS:[ECX]
00429F76  |.  83FA 24       CMP EDX,24
00429F79  |.  75 2E         JNZ SHORT yashin_t.00429FA9
00429F7B  |.  6A 04         PUSH 4                                   ; /Arg5 = 00000004
00429F7D  |.  68 73F84D08   PUSH 84DF873                             ; |Arg4 = 084DF873
00429F82  |.  68 EE7D98FF   PUSH FF987DEE                            ; |Arg3 = FF987DEE
00429F87  |.  8B45 08       MOV EAX,DWORD PTR SS:[EBP+8]             ; |
00429F8A  |.  8B88 30190700 MOV ECX,DWORD PTR DS:[EAX+71930]         ; |
00429F90  |.  83E9 10       SUB ECX,10                               ; |
00429F93  |.  51            PUSH ECX                                 ; |Arg2
00429F94  |.  8B55 08       MOV EDX,DWORD PTR SS:[EBP+8]             ; |
00429F97  |.  8B82 28190700 MOV EAX,DWORD PTR DS:[EDX+71928]         ; |
00429F9D  |.  83C0 10       ADD EAX,10                               ; |
00429FA0  |.  50            PUSH EAX                                 ; |Arg1
00429FA1  |.  E8 0A5BFEFF   CALL yashin_t.0040FAB0                   ; \yashin_t.0040FAB0
00429FA6  |.  83C4 14       ADD ESP,14
00429FA9  |>  B8 01000000   MOV EAX,1
00429FAE  |>  8BE5          MOV ESP,EBP
00429FB0  |.  5D            POP EBP
00429FB1  \.  C3            RETN
*/

struct acui_information SOFTPAL_ADV_SYSTEM_cui_information = {
	_T("SOFTPAL"),					/* copyright */
	_T("SOFTPAL ADV SYSTEM"),		/* system */
	_T(".pac .PGD"),				/* package */
	_T("1.3.2"),					/* revision */
	_T("痴漢公賊"),					/* author */
	_T("2009-6-23 22:02"),			/* date */
	NULL,							/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE | ACUI_ATTRIBUTE_PRELOAD
};

#pragma pack (1)
typedef struct {
	s8 magic[16];	// "VAFSH       "
} _058_header_t;

/* !from && !numbers (非可见字符); from == index_entries && !numbers (没有以该字母为首的资源项) */
typedef struct {
	u16 from;			// 从0开始
	u16 numbers;		// 具有相同首字母的项数
} pac_dir_t;

typedef struct {
	u32 from;			// 从0开始
	u32 numbers;		// 具有相同首字母的项数
} pac2_dir_t;

typedef struct {
	u16 index_entries;
	pac_dir_t dir[255];
} pac_header_t;

typedef struct {
	s8 magic[4];		// "PAC "
	u32 unknown;
	u32 index_entries;
	pac2_dir_t dir[255];
} pac2_header_t;

typedef struct {
	s8 magic[4];		// "EOF "
} pac2_tailer_t;

typedef struct {
	s8 name[16];
	u32 length;
	u32 offset;
} pac_entry_t;

typedef struct {
	s8 name[32];
	u32 length;
	u32 offset;
} pac_entry2_t;

typedef struct {
	u32 zero[2];
	u32 x_origin;	// ?图象左端坐标
	u32 y_origin;	// ?图象底端坐标
	u32 width;
	u32 height;
	s8 magic[4];	// "00_C"
	u32 uncomprlen;
	u32 comprlen;	// 包括0xc字节头
} pgd_header_t;

typedef struct {
	s8 maigc0[2];		// "GE"
	u16 sizeof_header;
	u32 orig_x;			// 图象顶点在整个显示区域的位置
	u32 orig_y;
	u32 width;	
	u32 height;		
	u32 orig_width;		// 图象整个显示区域中显示的宽度
	u32 orig_height;	// 图象整个显示区域中显示的高度
} pgd11_header_t;

typedef struct {
	s8 maigc0[2];		// "GE"
	u16 sizeof_header;	// 32
	u32 orig_x;			// 图象顶点在整个显示区域的位置
	u32 orig_y;
	u32 width;	
	u32 height;		
	u32 orig_width;		// 图象整个显示区域中显示的宽度
	u32 orig_height;	// 图象整个显示区域中显示的高度
	u16 compr_method;
	u16 unknown;
} pgd32_header_t;

typedef struct {
	s8 maigc[4];		// "PGD3"
	u16 orig_x;
	u16 orig_y;
	u16 width;
	u16 height;
	u16 bpp;
	s8 base_image[34];
} pgd3_header_t;

typedef struct {
	s8 magic[4];		// "11_C"
	u32 uncomprlen;
	u32 comprlen;		// 包括本Info信息在内的12字节
} pgd11_info_t;

typedef struct {
	u32 uncomprlen;
	u32 comprlen;		// 包括本Info信息在内的12字节
} pgd32_info_t;

typedef struct {
	u16 unknown;		// 7
	u16 bpp;			// 32 or 24
	u16 width;
	u16 height;
	//u8 *flags			// height个
} ge_header_t;

struct TargaHeader_s {
    u8  bID_Length;		// 附加信息长度 @ 0
    u8  bPalType;		// 调色板信息 @ 1
    u8  bImageType;		// 图象类型(0,1,2,3,9,10,11) @ 2 
    u16 wPalFirstNdx;	// 调色板第一个索引值  @ 3
    u16 wPalLength;		// 调色板索引数(以调色板单元为单位)  @ 5
    u8  bPalBits;		// 一个调色板单位位数(15,16,24,32)  @ 7
    u16 wLeft;			// 图象左端坐标(基本无用) @ 8
    u16 wBottom;		// 图象底端坐标(基本无用) @ a 
    u16 wWidth;			// 图象宽度 @ c 
    u16 wDepth;			// 图象长度  @ e
    u8  bBits;			// 一个象素位数 @ 10 
    u8  bDescriptor;	// 附加特性描述 @ 11 
};

typedef struct {
	s8 magic[4];		// "EPEG"
	u32 index_entries;	// 每项8字节
	u32 unknown;		// 2000
	u16 width;
	u16 height;
	u16 bpp;
	u32 unknown1;		// 0x39893582
	u8 unknown2[10];
	u32 flags;			// 1 - ？； 2 - PalSoundLoadVerEpeg
} epg_header_t;

typedef struct {
	u32 offset;
	u32 unknown;
} epg_entry_t;
#pragma pack ()		


static BYTE *base_image;
static DWORD base_image_length;
static DWORD base_width, base_height;


static int is_tga(struct TargaHeader_s *tga)
{
	if (!tga->bID_Length && !tga->bPalType && tga->bImageType == 2 && !tga->wPalFirstNdx 
			&& !tga->wPalLength && tga->wWidth && tga->wDepth 
			&& (tga->bBits == 24 || tga->bBits == 32))
		return 1;

	return 0;
}

static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static inline unsigned char getbit_le(unsigned char byte, unsigned int pos)
{
	return !!(byte & (1 << pos));
}

static inline unsigned char getbyte_le(unsigned char byte)
{
	return byte;
}

static void __pgd_uncompress(BYTE *compr, BYTE *uncompr, DWORD uncomprlen)
{
	DWORD act_uncomprlen = 0;
	DWORD flag = 0;

	while (act_uncomprlen != uncomprlen) {
		DWORD base_offset, copy_bytes;

		if (!(flag & 0x0100))
			flag = *compr++ | 0xff00;

		if (act_uncomprlen >= 0xffc)
			base_offset = act_uncomprlen - 0xffc;
		else
			base_offset = 0;

		if (flag & 1) {
			u16 offset = *(u16 *)compr;
			compr += 2;
			copy_bytes = *compr++;
			memcpy(&uncompr[act_uncomprlen], uncompr + base_offset + offset, copy_bytes);
		} else {
		    copy_bytes = *compr++;
			memcpy(&uncompr[act_uncomprlen], compr, copy_bytes);
			compr += copy_bytes;
		}
		flag >>= 1;
		act_uncomprlen += copy_bytes;
	}
}

static s16 SaturateToSignedByte(u8 para0, s16 para1)
{
	s16 sum = para0 + para1;
	if (para1 > 0 && sum < 0)
		return 32767;
	return sum;
}

static void pgd_ge_process1(BYTE *out, DWORD out_len, BYTE *__ge, DWORD __ge_length)
{
	u8 *src[4];

	DWORD len = __ge_length / 4;
	src[0] = __ge;
	src[1] = src[0] + len;
	src[2] = src[1] + len;
	src[3] = src[2] + len;

	for (DWORD k = 0; k < __ge_length / 4; ++k) {
		*out++ = *(src[3])++;
		*out++ = *(src[2])++;
		*out++ = *(src[1])++;
		*out++ = *(src[0])++;
	}
}

static void pgd_ge_process2(BYTE *out, DWORD out_len, BYTE *__ge, DWORD __ge_length,
						   DWORD width, DWORD height)
{
	u8 *src[4];

	src[0] = __ge;
	src[1] = src[0] + width * height / 4;
	src[2] = src[1] + width * height / 4;

	BYTE *dst = out;
	for (DWORD k = 0; k < height / 2; ++k) {
		for (DWORD j = 0; j < width / 2; ++j) {
			int tmp[2]; 
			int val, para[3];

			tmp[0] = (s8)*src[0]++;
			tmp[1] = (s8)*src[1]++;

			para[0] = tmp[0] * 226;
			para[1] = -43 * tmp[0] - 89 * tmp[1];
			para[2] = tmp[1] * 179;

			for (DWORD i = 0; i < 3; ++i) {
				val = (para[i] + (src[2][0] << 7)) >> 7;
				if (val <= 255) {
					if (val < 0)
						val = 0;
				} else
					val = 255;
				dst[i] = val;
			}
			dst[i++] = 0xff;

			for (; i < 7; ++i) {
				val = (para[i - 4] + (src[2][1] << 7)) >> 7;
				if (val <= 255) {
					if (val < 0)
						val = 0;
				} else
					val = 255;
				dst[i] = val;
			}
			dst[i] = 0xff;

			dst += width * 4;
			src[3] = src[2] + width;

			for (i = 0; i < 3; ++i) {
				val = (para[i] + (src[3][0] << 7)) >> 7;
				if (val <= 255) {
					if (val < 0)
						val = 0;
				} else
					val = 255;
				dst[i] = val;
			}
			dst[i++] = 0xff;

			for (; i < 7; ++i) {
				val = (para[i - 4] + (src[3][1] << 7)) >> 7;
				if (val <= 255) {
					if (val < 0)
						val = 0;
				} else
					val = 255;
				dst[i] = val;
			}
			dst[i] = 0xff;

			dst -= width * 4;			
			dst += 8;
			src[2] += 2;
		}
		src[2] += width;
		dst += width * 4;
	}
}

static void __pgd_uncompress32(BYTE *compr, BYTE *uncompr, DWORD uncomprlen)
{
	DWORD act_uncomprlen = 0;
	DWORD flag = *compr++ | 0xff00;

	while (act_uncomprlen != uncomprlen) {
		DWORD base_offset, copy_bytes;
		BYTE *src, *dst;

		if (flag & 1) {
			DWORD tmp = *(u16 *)compr;
			compr += 2;
			if (tmp & 8) {
				base_offset = tmp >> 4;
				copy_bytes = (tmp & 0x7) + 4;
			} else {
				base_offset = (tmp << 8) | *compr++;
				copy_bytes = (base_offset & 0xfff) + 4;
				base_offset >>= 12;
			}
			src = uncompr + act_uncomprlen - base_offset;
		} else {
		    copy_bytes = *compr++;
			src = compr;
			compr += copy_bytes;
		}
		dst = uncompr + act_uncomprlen;
		for (DWORD i = 0; i < copy_bytes; ++i)
			*dst++ = *src++;
		flag >>= 1;
		act_uncomprlen += copy_bytes;
		if (!(flag & 0x0100))
			flag = *compr++ | 0xff00;
	}
}

static void __pgd3_ge_process_24(BYTE *out, DWORD out_len, BYTE *__ge, DWORD __ge_length,
								 int width, int height)
{
	BYTE *flag = __ge;
	BYTE *src = flag + height;
	BYTE *data = out;
	int org_width = width;

	for (int h = 0; h < height; ++h) {
		width = org_width;

		if (flag[h] & 1) {
			*data++ = *src++;
			*data++ = *src++;
			*data++ = *src++;
			--width;
			u8 *pre = data - 3;
			while (width-- > 0) {
				*data++ = *pre++ - *src++;
				*data++ = *pre++ - *src++;
				*data++ = *pre++ - *src++;
			}
		} else if (flag[h] & 2) {
			u8 *pre = data - width * 3;
			while (width-- > 0) {
				*data++ = *pre++ - *src++;
				*data++ = *pre++ - *src++;
				*data++ = *pre++ - *src++;
			}
		} else if (flag[h] & 4) {
			*data++ = *src++;
			*data++ = *src++;
			*data++ = *src++;
			u8 *pre = data - width * 3;
			--width;
			while (width-- > 0) {
				data[0] = (*pre++ + data[0 - 3]) / 2 - *src++;
				data[1] = (*pre++ + data[1 - 3]) / 2 - *src++;
				data[2] = (*pre++ + data[2 - 3]) / 2 - *src++;
				data += 3;
			}
		}
	}
}

static void __pgd3_ge_process_32(BYTE *out, DWORD out_len, BYTE *__ge, DWORD __ge_length,
								 int width, int height)
{
	BYTE *flag = __ge;
	BYTE *data = out;
	BYTE *src = flag + height;
	int org_width = width;

	for (int h = 0; h < height; ++h) {
		width = org_width;

		if (flag[h] & 1) {
			*(u32 *)data = *(u32 *)src;
			data += 4;
			src += 4;
			--width;
			u8 *pre = data - 4;
			while (width-- > 0) {
				*data++ = *pre++ - *src++;
				*data++ = *pre++ - *src++;
				*data++ = *pre++ - *src++;
				*data++ = *pre++ - *src++;
			}
		} else if (flag[h] & 2) {
			u8 *pre = data - width * 4;
			while (width-- > 0) {
				*data++ = *pre++ - *src++;
				*data++ = *pre++ - *src++;
				*data++ = *pre++ - *src++;
				*data++ = *pre++ - *src++;
			}
		} else {
			*(u32 *)data = *(u32 *)src;			
			data += 4;
			u8 *pre = data - width * 4;
			src += 4;
			--width;
			while (width-- > 0) {
				for (DWORD i = 0; i < 4; ++i)
					data[i] = (pre[i] + data[i - 4]) / 2 - src[i];
				data += 4;
				src += 4;
				pre += 4;
			}
		}
	}
}

static void __pgd_ge_process3_24(BYTE *out, DWORD out_len, BYTE *__ge, DWORD __ge_length)
{
	ge_header_t *ge_header = (ge_header_t *)__ge;
	__pgd3_ge_process_24(out, out_len, (BYTE *)(ge_header + 1), __ge_length - sizeof(ge_header_t), 
		ge_header->width, ge_header->height);
}

static void __pgd_ge_process3_32(BYTE *out, DWORD out_len, BYTE *__ge, DWORD __ge_length)
{
	ge_header_t *ge_header = (ge_header_t *)__ge;
	__pgd3_ge_process_32(out, out_len, (BYTE *)(ge_header + 1), __ge_length - sizeof(ge_header_t), 
		ge_header->width, ge_header->height);
}

static void pgd_ge_process3(BYTE *out, DWORD out_len, BYTE *__ge, DWORD __ge_length, DWORD bpp)
{
	if (bpp == 32)
		__pgd_ge_process3_32(out, out_len, __ge, __ge_length);
	else
		__pgd_ge_process3_24(out, out_len, __ge, __ge_length);
}

static int pgd_uncompress(BYTE **ret_uncompr, DWORD *ret_uncomprlen, BYTE *__pgd, DWORD __pgd_length)
{
	pgd_header_t *pgd_header = (pgd_header_t *)__pgd;
	DWORD width, height;

	if (!memcmp(__pgd, "GE", 2)) {	// Sprite
		pgd11_header_t *pgd11_header = (pgd11_header_t *)__pgd;
		__pgd += pgd11_header->sizeof_header;
		__pgd_length -= pgd11_header->sizeof_header;

		BYTE *dib_data;
		DWORD dib_data_len;
		if (!memcmp(__pgd, "11_C", 4)) {
printf("GE 11_C!\n\n");
			pgd11_info_t *pgd11_info = (pgd11_info_t *)__pgd;
			BYTE *compr = (BYTE *)(pgd11_info + 1);

			BYTE *uncompr = (BYTE *)malloc(pgd11_info->uncomprlen);
			if (!uncompr)
				return -CUI_EMEM;
			__pgd_uncompress(compr, uncompr, pgd11_info->uncomprlen);
			dib_data = uncompr;
			dib_data_len = pgd11_info->uncomprlen;
			width = pgd11_header->width;
			height = pgd11_header->height;
		} else if (pgd11_header->sizeof_header == 32) {
			pgd32_header_t *pgd32_header = (pgd32_header_t *)pgd11_header;
			pgd32_info_t *info = (pgd32_info_t *)__pgd;
			BYTE *compr = (BYTE *)(info + 1);

			BYTE *uncompr = (BYTE *)malloc(info->uncomprlen);
			if (!uncompr)
				return -CUI_EMEM;

			DWORD out_len;
			BYTE *out = NULL;
			DWORD bpp;
			if (pgd32_header->compr_method == 3) {
				__pgd_uncompress32(compr, uncompr, info->uncomprlen);
				ge_header_t *ge_header = (ge_header_t *)uncompr;
				bpp = ge_header->bpp;

				out_len = pgd32_header->width * pgd32_header->height * bpp / 8;
				out = (BYTE *)malloc(out_len);
				if (!out) {
					free(uncompr);
					return -CUI_EMEM;
				}
				pgd_ge_process3(out, out_len, uncompr, info->uncomprlen, bpp);
			} else if (pgd32_header->compr_method == 2) {	// 虽然是32位图(背景), 但似乎是24位色+纯白alp
				bpp = 32;
				out_len = pgd32_header->width * pgd32_header->height * bpp / 8;
				out = (BYTE *)malloc(out_len);
				if (!out) {
					free(uncompr);
					return -CUI_EMEM;
				}
				__pgd_uncompress32(compr, uncompr, info->uncomprlen);
				pgd_ge_process2(out, out_len, uncompr, info->uncomprlen,
						pgd32_header->width, pgd32_header->height);
			} else if (pgd32_header->compr_method == 1) {
				bpp = 32;
				out_len = pgd32_header->width * pgd32_header->height * bpp / 8;
				out = (BYTE *)malloc(out_len);
				if (!out) {
					free(uncompr);
					return -CUI_EMEM;
				}
				__pgd_uncompress32(compr, uncompr, info->uncomprlen);
				pgd_ge_process1(out, out_len, uncompr, info->uncomprlen);
			}

			free(uncompr);

			if (base_image)
				free(base_image);
			base_image_length = out_len;
			base_image = (BYTE *)malloc(base_image_length);
			if (!base_image) {
				free(out);
				return -CUI_EMEM;;
			}
			base_width = pgd32_header->width;
			base_height = pgd32_header->height;
			memcpy(base_image, out, base_image_length);

			alpha_blending(out, pgd32_header->width, pgd32_header->height, bpp);

			if (MyBuildBMPFile(out, out_len, NULL, 0, pgd32_header->width,
					0 - pgd32_header->height, bpp, ret_uncompr, ret_uncomprlen, my_malloc)) {
				free(out);
				return -CUI_EMEM;
			}
			free(out);

			return 0;
		} else if (!memcmp(__pgd, " EZL", 4)) {
			printf("not support EZL!\n");
			exit(0);
		} else {	// 这个分支需要测试！
			BYTE *ret_bgd = (BYTE *)malloc(__pgd_length);
			if (!ret_bgd)
				return -CUI_EMEM;
			memcpy(ret_bgd, __pgd, __pgd_length);
			dib_data = ret_bgd;
			dib_data_len = __pgd_length;
			width = pgd11_header->width;
			height = pgd11_header->height;
		}

		BYTE *act_dib = (BYTE *)malloc(dib_data_len);
		if (!act_dib) {
			free(dib_data);
			return -CUI_EMEM;
		}

		DWORD pixel = width * height;
		BYTE *b = dib_data;
		BYTE *g = b + pixel;
		BYTE *r = g + pixel;
		BYTE *a = r + pixel;
		for (DWORD p = 0; p < dib_data_len; p += 4) {
			act_dib[p + 0] = *b++;
			act_dib[p + 1] = *g++;
			act_dib[p + 2] = *r++;
			act_dib[p + 3] = *a++;
		}
		free(dib_data);

		alpha_blending(act_dib, width, height, 32);

		if (MyBuildBMPFile(act_dib, dib_data_len, NULL, 0, width,
				0 - height, 32, ret_uncompr, ret_uncomprlen, my_malloc)) {
			free(act_dib);
			return -CUI_EMEM;
		}
		free(act_dib);
	} else if (!memcmp(pgd_header->magic, "00_C", 4)) {
printf("00_C!\n\n");
		DWORD uncomprlen = pgd_header->uncomprlen;
		BYTE *uncompr = (BYTE *)malloc(uncomprlen + 0x1000);
		if (!uncompr)
			return -CUI_EMEM;
		
		BYTE *compr = (BYTE *)(pgd_header + 1);
		DWORD comprlen = pgd_header->comprlen - 12;
		DWORD act_uncomprlen = 0;
		DWORD total_uncomprlen = 0;
		DWORD curbyte = 0;
		DWORD cur_window = 0;	
		
		while (curbyte != comprlen) {
			unsigned char bitmap;
			unsigned int copy_bytes;

			bitmap = getbyte_le(compr[curbyte++]);
			for (unsigned int curbit_bitmap = 0; curbit_bitmap < 8; curbit_bitmap++) {
				if (curbyte == comprlen)
					goto out;

				if (!getbit_le(bitmap, curbit_bitmap)) {
					copy_bytes = getbyte_le(compr[curbyte++]);
					for (unsigned int i = 0; i < copy_bytes; i++)					
						uncompr[act_uncomprlen + i] = compr[curbyte++];				
				} else {
					unsigned int win_offset = 0;

					win_offset = getbyte_le(compr[curbyte++]);
					win_offset |= getbyte_le(compr[curbyte++]) << 8;
					copy_bytes = getbyte_le(compr[curbyte++]);
					win_offset += cur_window;

					for (unsigned int i = 0; i < copy_bytes; i++)					
						uncompr[act_uncomprlen + i] = uncompr[win_offset + i];
				}
				act_uncomprlen += copy_bytes;
				total_uncomprlen += copy_bytes;
				if (total_uncomprlen > uncomprlen)
					total_uncomprlen = uncomprlen;
				if (total_uncomprlen <= 3000)
					cur_window = 0;
				else
					cur_window = total_uncomprlen - 3000;
			}
		}
	out:
		if (act_uncomprlen != uncomprlen) {
			free(uncompr);
			return -CUI_EUNCOMPR;
		}

		*ret_uncompr = uncompr;
		*ret_uncomprlen = uncomprlen;
	} else if (!memcmp(__pgd, "PGD3", 4)) {
		pgd3_header_t *pgd3_header = (pgd3_header_t *)__pgd;
		pgd32_info_t *info = (pgd32_info_t *)(pgd3_header + 1);
		__pgd += sizeof(pgd3_header_t) + sizeof(pgd32_info_t);
		__pgd_length -= sizeof(pgd3_header_t) + sizeof(pgd32_info_t);

		BYTE *compr = (BYTE *)__pgd;
		BYTE *uncompr = (BYTE *)malloc(info->uncomprlen);
		if (!uncompr)
			return -CUI_EMEM;

		DWORD out_len = pgd3_header->width * pgd3_header->height * pgd3_header->bpp / 8;
		BYTE *out = (BYTE *)malloc(out_len);
		if (!out) {
			free(uncompr);
			return -CUI_EMEM;
		}

		__pgd_uncompress32(compr, uncompr, info->uncomprlen);
		if (pgd3_header->bpp == 24)
			__pgd3_ge_process_24(out, out_len, uncompr, info->uncomprlen, pgd3_header->width, pgd3_header->height);
		else
			__pgd3_ge_process_32(out, out_len, uncompr, info->uncomprlen, pgd3_header->width, pgd3_header->height);

		free(uncompr);

		BYTE *base = (BYTE *)malloc(base_image_length);
		if (!base) {
			free(out);
			return -CUI_EMEM;
		}
		memcpy(base, base_image, base_image_length);

		BYTE *src = out;
		DWORD line_len = base_width * pgd3_header->bpp / 8;
		BYTE *dst = &base[pgd3_header->orig_y * line_len + pgd3_header->orig_x * pgd3_header->bpp / 8];
		for (DWORD y = 0; y < pgd3_header->height; ++y) {
			DWORD len = pgd3_header->width * pgd3_header->bpp / 8;
			for (DWORD i = 0; i < len; ++i)
				*dst++ ^= *src++;
			dst += line_len - len;
		}
		free(out);

		alpha_blending(base, base_width, base_height, pgd3_header->bpp);

		if (MyBuildBMPFile(base, base_image_length, NULL, 0, base_width,
				0 - base_height, pgd3_header->bpp, ret_uncompr, ret_uncomprlen, 
				my_malloc)) {
			return -CUI_EMEM;
		}
		free(base);
	} else
		return -CUI_EMATCH;

	return 0;
}

static void decrypt(BYTE *__enc, DWORD enc_len, DWORD key0, DWORD key1, BYTE shift)
{
	DWORD *enc = (DWORD *)__enc;
	DWORD i;

	enc_len /= 4;	
	key0 ^= key1;
	for (i = 0; i < enc_len; i++) {
		BYTE tmp;

		shift &= 7;
		tmp = (BYTE)enc[i];
		enc[i] &= ~0xff;
		tmp = tmp << shift | tmp >> (8 - shift);
		enc[i] |= tmp;
		shift++;
		enc[i] ^= key0;
	}
}

/********************* pac *********************/

static int SOFTPAL_ADV_SYSTEM_pac_match(struct package *pkg)
{	
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	pac_header_t pac_header;
	if (pkg->pio->read(pkg, &pac_header, sizeof(pac_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (!pac_header.index_entries) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	pac_entry_t entry;
	if (pkg->pio->read(pkg, &entry, sizeof(entry))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (entry.offset == sizeof(pac_header_t) + pac_header.index_entries * sizeof(pac_entry_t)) {
		package_set_private(pkg, sizeof(pac_entry_t));
		return 0;
	}

	pac_entry2_t entry2;
	if (pkg->pio->readvec(pkg, &entry2, sizeof(entry2), sizeof(pac_header_t), IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (entry2.offset == sizeof(pac_header_t) + pac_header.index_entries * sizeof(pac_entry2_t)) {
		package_set_private(pkg, sizeof(pac_entry2_t));
		return 0;
	}

	return -CUI_EMATCH;	
}

static int SOFTPAL_ADV_SYSTEM_pac_extract_directory(struct package *pkg,
										 struct package_directory *pkg_dir)
{	
	pac_header_t pac_header;
	unsigned int index_buffer_len;
	pac_entry_t *index_buffer;
	
	if (pkg->pio->readvec(pkg, &pac_header, sizeof(pac_header), 0, IO_SEEK_SET))
		return -CUI_EREAD;

	DWORD entry_size = package_get_private(pkg);
	index_buffer_len = pac_header.index_entries * entry_size;		
	index_buffer = (pac_entry_t *)malloc(index_buffer_len);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_len)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = pac_header.index_entries;
	pkg_dir->index_entry_length = entry_size;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_len;
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

static int SOFTPAL_ADV_SYSTEM_pac_parse_resource_info(struct package *pkg,
										   struct package_resource *pkg_res)
{
	DWORD entry_size = package_get_private(pkg);
	
	if (entry_size == sizeof(pac_entry_t)) {
		pac_entry_t *pac_entry;		

		pac_entry = (pac_entry_t *)pkg_res->actual_index_entry;
		strncpy(pkg_res->name, pac_entry->name, 16);
		pkg_res->name_length = 16;			/* -1表示名称以NULL结尾 */
		pkg_res->raw_data_length = pac_entry->length;
		pkg_res->actual_data_length = 0;	/* 数据都是明文 */
		pkg_res->offset = pac_entry->offset;
	} else if (entry_size == sizeof(pac_entry2_t)) {
		pac_entry2_t *pac_entry;		

		pac_entry = (pac_entry2_t *)pkg_res->actual_index_entry;
		strncpy(pkg_res->name, pac_entry->name, 32);
		pkg_res->name_length = 32;			/* -1表示名称以NULL结尾 */
		pkg_res->raw_data_length = pac_entry->length;
		pkg_res->actual_data_length = 0;	/* 数据都是明文 */
		pkg_res->offset = pac_entry->offset;
	}

	return 0;
}

static int SOFTPAL_ADV_SYSTEM_pac_extract_resource(struct package *pkg,
												   struct package_resource *pkg_res)
{	
	BYTE *data, *uncompr;
	DWORD uncomprlen;
	int ret;

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

	uncompr = NULL;
	uncomprlen = 0;
	ret = pgd_uncompress(&uncompr, &uncomprlen, data, pkg_res->raw_data_length);
	if (!ret) {
		if (!memcmp(uncompr, "BM", 2)) {
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
			pkg_res->replace_extension = _T(".bmp");
		} else {
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
			pkg_res->replace_extension = _T(".tga");
		}
	} else if (ret != -CUI_EMATCH) {
		free(data);
		return ret;
	} else if (!memcmp(data, "BM", 2)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp");
	} else {
		if (data[0] == '$')
			decrypt(data + 16, pkg_res->raw_data_length - 16, 0xFF987DEE, 0x84DF873, 4);
	}

	pkg_res->raw_data = data;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;	

	return 0;
}

static int SOFTPAL_ADV_SYSTEM_pac_save_resource(struct resource *res, 
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

static void SOFTPAL_ADV_SYSTEM_pac_release_resource(struct package *pkg, 
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

static void SOFTPAL_ADV_SYSTEM_pac_release(struct package *pkg, 
								struct package_directory *pkg_dir)
{
	if (base_image) {
		free(base_image);
		base_image = NULL; 
	}

	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg);
}

static cui_ext_operation SOFTPAL_ADV_SYSTEM_pac_operation = {
	SOFTPAL_ADV_SYSTEM_pac_match,				/* match */
	SOFTPAL_ADV_SYSTEM_pac_extract_directory,	/* extract_directory */
	SOFTPAL_ADV_SYSTEM_pac_parse_resource_info,	/* parse_resource_info */
	SOFTPAL_ADV_SYSTEM_pac_extract_resource,	/* extract_resource */
	SOFTPAL_ADV_SYSTEM_pac_save_resource,		/* save_resource */
	SOFTPAL_ADV_SYSTEM_pac_release_resource,	/* release_resource */
	SOFTPAL_ADV_SYSTEM_pac_release				/* release */
};

/********************* pac *********************/

static int SOFTPAL_ADV_SYSTEM_pac2_match(struct package *pkg)
{	
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	pac2_header_t header;
	if (pkg->pio->read(pkg, &header, sizeof(header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(header.magic, "PAC ", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int SOFTPAL_ADV_SYSTEM_pac2_extract_directory(struct package *pkg,
													 struct package_directory *pkg_dir)
{	
	pac2_header_t header;
	
	if (pkg->pio->readvec(pkg, &header, sizeof(header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD index_buffer_len = header.index_entries * sizeof(pac_entry2_t);		
	pac_entry2_t *index_buffer = (pac_entry2_t *)malloc(index_buffer_len);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_len)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = header.index_entries;
	pkg_dir->index_entry_length = sizeof(pac_entry2_t);
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_len;
	//pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	package_set_private(pkg, sizeof(pac_entry2_t));

	return 0;
}

static cui_ext_operation SOFTPAL_ADV_SYSTEM_pac2_operation = {
	SOFTPAL_ADV_SYSTEM_pac2_match,				/* match */
	SOFTPAL_ADV_SYSTEM_pac2_extract_directory,	/* extract_directory */
	SOFTPAL_ADV_SYSTEM_pac_parse_resource_info,	/* parse_resource_info */
	SOFTPAL_ADV_SYSTEM_pac_extract_resource,	/* extract_resource */
	SOFTPAL_ADV_SYSTEM_pac_save_resource,		/* save_resource */
	SOFTPAL_ADV_SYSTEM_pac_release_resource,	/* release_resource */
	SOFTPAL_ADV_SYSTEM_pac_release				/* release */
};

/********************* PGD *********************/

static int SOFTPAL_ADV_SYSTEM_PGD_match(struct package *pkg)
{	
	s8 magic[2];
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	struct TargaHeader_s tga_header;
	if (pkg->pio->readvec(pkg, &tga_header, sizeof(tga_header), 0, IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "GE", 2) && strncmp(magic, "BM", 2) && !is_tga(&tga_header)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int SOFTPAL_ADV_SYSTEM_PGD_extract_resource(struct package *pkg,
										struct package_resource *pkg_res)
{	
	BYTE *uncompr;
	DWORD uncomprlen;
	int ret;
	u32 PGD_size;

	if (pkg->pio->length_of(pkg, &PGD_size))
		return -CUI_ELEN;

	BYTE *PGD = (BYTE *)malloc(PGD_size);
	if (!PGD)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, PGD, PGD_size, 0, IO_SEEK_SET)) {
		free(PGD);
		return -CUI_EREADVEC;
	}

	uncompr = NULL;
	uncomprlen = 0;
	ret = pgd_uncompress(&uncompr, &uncomprlen, PGD, PGD_size);
	if (!ret) {
		if (!memcmp(uncompr, "BM", 2)) {
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
			pkg_res->replace_extension = _T(".bmp");
		} else if (!memcmp(uncompr + uncomprlen - 18, "TRUEVISION-XFILE.", 18)) {
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
			pkg_res->replace_extension = _T(".tga");
		}
	} else if (ret == -CUI_EMATCH) {
		if (!memcmp(PGD, "BM", 2)) {
			pkg_res->flags |= PKG_RES_FLAG_APEXT;
			pkg_res->replace_extension = _T(".bmp");
		} else {
			pkg_res->flags |= PKG_RES_FLAG_APEXT;
			pkg_res->replace_extension = _T(".tga");
		}		
	} else {
		free(PGD);
		return ret;
	} 
	pkg_res->raw_data = PGD;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;	

	return 0;
}

static cui_ext_operation SOFTPAL_ADV_SYSTEM_PGD_operation = {
	SOFTPAL_ADV_SYSTEM_PGD_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	SOFTPAL_ADV_SYSTEM_PGD_extract_resource,	/* extract_resource */
	SOFTPAL_ADV_SYSTEM_pac_save_resource,		/* save_resource */
	SOFTPAL_ADV_SYSTEM_pac_release_resource,	/* release_resource */
	SOFTPAL_ADV_SYSTEM_pac_release				/* release */
};

/********************* EPG *********************/

static int SOFTPAL_ADV_SYSTEM_EPG_match(struct package *pkg)
{	
	epg_header_t header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &header, sizeof(header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(header.magic, "EPEG", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int SOFTPAL_ADV_SYSTEM_EPG_extract_directory(struct package *pkg,
													struct package_directory *pkg_dir)
{	
	epg_header_t header;
	if (pkg->pio->readvec(pkg, &header, sizeof(header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	epg_entry_t *index = new epg_entry_t[header.index_entries + 1];
	if (!index)
		return -CUI_EMEM;

	DWORD index_len = sizeof(epg_entry_t) * (header.index_entries + 1);
	if (pkg->pio->read(pkg, index, index_len)) {
		delete [] index;
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = header.index_entries;
	pkg_dir->index_entry_length = sizeof(epg_entry_t);
	pkg_dir->directory = index;
	pkg_dir->directory_length = sizeof(epg_entry_t) * header.index_entries;

	return 0;
}

static int SOFTPAL_ADV_SYSTEM_EPG_parse_resource_info(struct package *pkg,
													  struct package_resource *pkg_res)
{
	epg_entry_t *entry = (epg_entry_t *)pkg_res->actual_index_entry;

	sprintf(pkg_res->name, "%06d", pkg_res->index_number);		
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = (entry+1)->offset - entry->offset;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = entry->offset;

	return 0;
}

static int SOFTPAL_ADV_SYSTEM_EPG_extract_resource(struct package *pkg,
												   struct package_resource *pkg_res)
{	
	BYTE *raw = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length, 
			pkg_res->offset, IO_SEEK_SET)) {
		delete [] raw;
		return -CUI_EREADVEC;
	}

	pkg_res->raw_data = raw;

	return 0;
}

static cui_ext_operation SOFTPAL_ADV_SYSTEM_EPG_operation = {
	SOFTPAL_ADV_SYSTEM_EPG_match,				/* match */
	SOFTPAL_ADV_SYSTEM_EPG_extract_directory,	/* extract_directory */
	SOFTPAL_ADV_SYSTEM_EPG_parse_resource_info,	/* parse_resource_info */
	SOFTPAL_ADV_SYSTEM_EPG_extract_resource,	/* extract_resource */
	SOFTPAL_ADV_SYSTEM_pac_save_resource,		/* save_resource */
	SOFTPAL_ADV_SYSTEM_pac_release_resource,	/* release_resource */
	SOFTPAL_ADV_SYSTEM_pac_release				/* release */
};

/********************* 058 *********************/

static int SOFTPAL_ADV_SYSTEM_058_match(struct package *pkg)
{	
	_058_header_t header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &header, sizeof(header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(header.magic, "VAFSH       ", 12)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int SOFTPAL_ADV_SYSTEM_058_extract_directory(struct package *pkg,
										 struct package_directory *pkg_dir)
{	
	u32 index_entries;
	if (pkg->pio->read(pkg, &index_entries, sizeof(index_entries)))
		return -CUI_EREAD;

	index_entries = (index_entries - sizeof(_058_header_t)) / 4;
	u32 *offset_table = new u32[index_entries];
	if (!offset_table)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, offset_table, index_entries * 4, 
			sizeof(_058_header_t), IO_SEEK_SET)) {
		delete [] offset_table;
		return -CUI_EREADVEC;
	}

	for (DWORD i = 0; i < index_entries; ++i)
		if (!offset_table[i] || offset_table[i] == -1)
			break;
	--i;

	pkg_dir->index_entries = i;
	pkg_dir->index_entry_length = 4;
	pkg_dir->directory = offset_table;
	pkg_dir->directory_length = i * 4;
	//pkg_dir->flags = PKG_DIR_FLAG_VARLEN;

	return 0;
}

static int SOFTPAL_ADV_SYSTEM_058_parse_resource_info(struct package *pkg,
										   struct package_resource *pkg_res)
{
	u32 *offset = (u32 *)pkg_res->actual_index_entry;

	sprintf(pkg_res->name, "%06d", pkg_res->index_number);		
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = offset[1] - offset[0];
	pkg_res->actual_data_length = 0;
	pkg_res->offset = *offset;

	return 0;
}

static int SOFTPAL_ADV_SYSTEM_058_extract_resource(struct package *pkg,
										struct package_resource *pkg_res)
{	
	BYTE *raw = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length, 
			pkg_res->offset, IO_SEEK_SET)) {
		delete [] raw;
		return -CUI_EREADVEC;
	}

	if (_tcsstr(pkg->name, _T("BGM"))) {
		if (MySaveAsWAVE(raw, pkg_res->raw_data_length, 
				1, 2, 0x5622, 16, NULL, 0, (BYTE **)&pkg_res->actual_data,
				&pkg_res->actual_data_length, my_malloc)) {
			delete [] raw;
			return -CUI_EMEM;
		}
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".wav");
	}

	pkg_res->raw_data = raw;

	return 0;
}

static cui_ext_operation SOFTPAL_ADV_SYSTEM_058_operation = {
	SOFTPAL_ADV_SYSTEM_058_match,				/* match */
	SOFTPAL_ADV_SYSTEM_058_extract_directory,	/* extract_directory */
	SOFTPAL_ADV_SYSTEM_058_parse_resource_info,/* parse_resource_info */
	SOFTPAL_ADV_SYSTEM_058_extract_resource,	/* extract_resource */
	SOFTPAL_ADV_SYSTEM_pac_save_resource,		/* save_resource */
	SOFTPAL_ADV_SYSTEM_pac_release_resource,	/* release_resource */
	SOFTPAL_ADV_SYSTEM_pac_release				/* release */
};

int CALLBACK SOFTPAL_ADV_SYSTEM_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".pac"), NULL, 
		NULL, &SOFTPAL_ADV_SYSTEM_pac2_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".pac"), NULL, 
		NULL, &SOFTPAL_ADV_SYSTEM_pac_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".PGD"), NULL, 
		NULL, &SOFTPAL_ADV_SYSTEM_PGD_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".EPG"), NULL, 
		NULL, &SOFTPAL_ADV_SYSTEM_EPG_operation, CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".058"), NULL, 
		NULL, &SOFTPAL_ADV_SYSTEM_058_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
