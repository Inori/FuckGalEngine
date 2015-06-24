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
#include <zlib.h>

/* 解密字符串在这个字符串附近找
0058ED30                    6D 67 5F 64 61 74 61 2E 6D 62        mg_data.mb
0058ED40  6C 00 72 62 00 6D 67 5F 64 61 74 61 32 2E 6D 62  l.rb.mg_data2.mb
0058ED50  6C 00 72 62 00                                   l.rb.
*/

/* 研究wady：
E:\oooooooooooooooooooooooooooooooooo\[[[[[[[[[[[[[[[[[[[[[[[[[
 */

struct acui_information MarbleEngine_cui_information = {
	_T("マーブルソフト"),		/* copyright */
	_T("Marble Engine"),		/* system */
	_T(".mbl"),					/* package */
	_T("0.8.1"),				/* revision */
	_T("痴汉公贼"),				/* author */
	_T("2008-6-11 23:34"),		/* date */
	NULL,						/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

#pragma pack (1)
typedef struct {
	u32 index_entries;		
	u32 name_length;
} mbl_header_t;

typedef struct {
	s8 name[56];
	u32 offset;
	u32 length;
} mg_data_entry_t;

typedef struct {
	s8 *name;
	u32 offset;
	u32 length;
} mbl_entry_t;

typedef struct {
	s8 magic[2];		/* "YB" */
	u8 mode;			
	u8 bpp;
	u32 comprLen;
	u32 uncomprLen;
	u16 width;
	u16 height;
} yb_header_t;

typedef struct {			// 0x30 bytes
	s8 magic[5];			/* "WADY" */
	u8 factor;
	u16 nChannels;		
	u32 nSamplesPerSec;		/* 采样率（每秒样本数） */
	u32 data_length0;		/* 语音数据的长度 */	
	u32 data_length1;		/* 语音数据的长度 */
	u32 unknown0;
	u32 data_offset;
	u16 left_channel_sample_data_base;
	u16 right_channel_sample_data_base;
	u16 wav_wFormatTag;
	u16 wav_nChannels;
	u32 wav_nSamplesPerSec;	/* 采样率（每秒样本数） */	
	u32 wav_nAvgBytesPerSec;/* 波形音频数据传送速率，其值为通道数×采样率×每样
		  　　　　　　　	* 本的数据位数／8 */
	u16 wav_nBlockAlign;
	u16 wav_wBitsPerSample;	/* 每样本的数据位数，表示每个声道中各个样本的数据位数。
							如果有多个声道，对每个声道而言，样本大小都一样。*/
} wady_header_t;

typedef struct {
	s8 ChunkID[4];
	u32 ChunkSize;
} riff_chunk_t;

typedef struct {
	riff_chunk_t riff_chunk;
	s8 Format[4];
} riff_header_t;

typedef struct {
	riff_chunk_t fmt_chunk;
	u16 wFormatTag;
	u16 nChannels;
	u32 nSamplesPerSec;
	u32 nAvgBytesPerSec;
	u16 nBlockAlign;
	u16 wBitsPerSample;
} fmt_header_t;

typedef struct {
	u32 frames;
	u32 unknown0;		// 0x21
	u32 unknown1;		// 0x190
	u32 unknown2;		// 0x12c
	u32 audio_data_size;
	u32 video_data_size;
} anim_header_t;
#pragma pack ()	

typedef struct {
	char name[256];
	DWORD offset;
	DWORD length;
} anim_entry_t;


static u16 wady_adpcm_step_table[128] = {
	0x0000, 0x0002, 0x0004, 0x0006, 0x0008, 0x000a, 0x000c, 0x000f,

	0x0012, 0x0015, 0x0018, 0x001c, 0x0020, 0x0024, 0x0028, 0x002c,

	0x0031, 0x0036, 0x003b, 0x0040, 0x0046, 0x004c, 0x0052, 0x0058,

	0x005f, 0x0066, 0x006d, 0x0074, 0x007c, 0x0084, 0x008c, 0x0094,

	0x00a0, 0x00aa, 0x00b4, 0x00be, 0x00c8, 0x00d2, 0x00dc, 0x00e6,

	0x00f0, 0x00ff, 0x010e, 0x011d, 0x012c, 0x0140, 0x0154, 0x0168,

	0x017c, 0x0190, 0x01a9, 0x01c2, 0x01db, 0x01f4, 0x020d, 0x0226,

	0x0244, 0x0262, 0x028a, 0x02bc, 0x02ee, 0x0320, 0x0384, 0x03e8,
};

static void wady_adpcm_step_table_init(void)
{
	for (unsigned int i = 0; i < 64; i++)
		wady_adpcm_step_table[i + 64] = -wady_adpcm_step_table[i];
}

static void *my_malloc(DWORD len)
{
	return malloc(len);
}	

static int MyBuildWAVFile(wady_header_t *wady_header, BYTE *data, DWORD data_length,
						  BYTE **ret_data, DWORD *ret_data_length)
{

	riff_header_t *riff_header;
	fmt_header_t *fmt_header;
	riff_chunk_t *dat_chunk;
	DWORD riff_chunk_len, fmt_chunk_len, data_chunk_len;
	DWORD wav_file_size;
	BYTE *wav_data, *dst, *phack;
	const char *hack_info = "Extracted By 痴汉公贼";

	fmt_chunk_len = 16;
	data_chunk_len = data_length;
	riff_chunk_len = 4 + (sizeof(riff_chunk_t) + fmt_chunk_len) + (sizeof(riff_chunk_t) + data_chunk_len);
	wav_file_size = sizeof(riff_chunk_t) + riff_chunk_len + strlen(hack_info);
	wav_data = (BYTE *)malloc(wav_file_size);
	if (!wav_data)
		return -CUI_EMEM;

	riff_header = (riff_header_t *)wav_data;
	fmt_header = (fmt_header_t *)(riff_header + 1);
	dat_chunk = (riff_chunk_t *)(fmt_header + 1);
	dst = (BYTE *)(dat_chunk + 1);
	phack = dst + data_length;

	memcpy(riff_header->riff_chunk.ChunkID, "RIFF", 4);
	riff_header->riff_chunk.ChunkSize = riff_chunk_len;
	memcpy(riff_header->Format, "WAVE", 4);

	memcpy(fmt_header->fmt_chunk.ChunkID, "fmt ", 4);
	fmt_header->fmt_chunk.ChunkSize = fmt_chunk_len;
	fmt_header->wFormatTag = 1;	// PCM
	fmt_header->nChannels = wady_header->nChannels;
	fmt_header->nSamplesPerSec = wady_header->nSamplesPerSec;
	fmt_header->nAvgBytesPerSec = wady_header->nChannels * 2 * wady_header->nSamplesPerSec;
	fmt_header->nBlockAlign = 2;
	fmt_header->wBitsPerSample = 16;

	memcpy(dat_chunk->ChunkID, "data", 4);
	dat_chunk->ChunkSize = data_chunk_len;	

	memcpy(dst, data, data_chunk_len);
	strcpy((char *)phack, hack_info);	

	*ret_data = wav_data;
	*ret_data_length = wav_file_size;

	return 0;
}

static BYTE inline getbyte_le(BYTE val)
{
	return val;
}
static inline BYTE getbit_be(unsigned char byte, unsigned int pos)
{
	return !!(byte & (1 << (7 - pos)));
}

static int yb_decompress(BYTE *uncompr, DWORD uncomprlen, BYTE *compr, DWORD comprlen)
{
	unsigned int act_uncomprlen = 0;
	unsigned int curbyte = 0;		/* compr中的当前扫描字节 */
	
	memset(uncompr, 0, uncomprlen);
	/* 因为flag的实际位数不明，因此由flag引起的compr下溢的问题都不算错误 */
	while (act_uncomprlen < uncomprlen) {
		unsigned char bitmap;

		if (curbyte >= comprlen)
			goto out;

		bitmap = getbyte_le(compr[curbyte++]);
		for (unsigned int curbit_bitmap = 0; curbit_bitmap < 8; curbit_bitmap++) {
			/* 如果为0, 表示接下来的1个字节原样输出 */
			if (!getbit_be(bitmap, curbit_bitmap)) {
				/* 输出1字节非压缩数据 */
				uncompr[act_uncomprlen++] = getbyte_le(compr[curbyte++]);	
			} else {
				BYTE flag;
				BYTE *src;
				DWORD copy_bytes;

				flag = getbyte_le(compr[curbyte++]);
				if (!(flag & 0x80)) {
					if ((flag & 3) == 3) {
						copy_bytes = (flag >> 2) + 9;
						src = &compr[curbyte];
						curbyte += copy_bytes;
					} else {
						src = &uncompr[act_uncomprlen - ((flag >> 2) + 1)];
						copy_bytes = (flag & 3) + 2;						
					}
				} else {
					BYTE offset_low;

					offset_low = getbyte_le(compr[curbyte++]);
					if (!(flag & 0x40)) {
						src = &uncompr[act_uncomprlen - ((((flag & 0x3f) << 4) | (offset_low >> 4)) + 1)];
						copy_bytes = (offset_low & 0xf) + 3;
					} else {
						src = &uncompr[act_uncomprlen - ((((flag & 0x3f) << 8) | offset_low) + 1)];	
						copy_bytes = getbyte_le(compr[curbyte++]);
						if (copy_bytes == 0xff)
							copy_bytes = 0x1000;
						else if (copy_bytes == 0xfe)
							copy_bytes = 0x400;
						else if (copy_bytes == 0xfd)
							copy_bytes = 0x100;
						else
							copy_bytes += 3;
					}
				}

				for (unsigned int i = 0; i < copy_bytes; i++)
					uncompr[act_uncomprlen + i] = src[i];
				act_uncomprlen += copy_bytes;
			}

			if (curbyte >= comprlen)
				break;
			if (act_uncomprlen >= uncomprlen)
				break;
		}
	}
out:	
	return act_uncomprlen;	
}

static int yb_decompress2(BYTE *uncompr, DWORD uncomprlen, BYTE *compr, DWORD comprlen)
{
	unsigned int act_uncomprlen = 0;
	unsigned int curbyte = 0;
	
	memset(uncompr, 0, uncomprlen);
	while (act_uncomprlen < uncomprlen) {
		unsigned char flag;

		if (curbyte >= comprlen)
			goto out;

		flag = compr[curbyte++];
		if (flag & 0x80) {
			unsigned int copy_counts;
			unsigned int copy_bytes;
			BYTE byte_data;
			WORD word_data;
			DWORD dword_data;
			unsigned int i;

			copy_counts = ((flag & 0x1f) << 8) | compr[curbyte++];
			copy_bytes = ((flag >> 5) & 3) + 1;
			if (copy_counts * copy_bytes + act_uncomprlen > uncomprlen)
				copy_counts = (uncomprlen - act_uncomprlen) / copy_bytes;

			switch (copy_bytes) {
			case 1:
				byte_data = compr[curbyte++];
				for (i = 0; i < copy_counts; i++)
					uncompr[act_uncomprlen++] = byte_data;
				break;
			case 2:
				word_data = *(WORD *)&compr[curbyte];
				curbyte += 2;
				for (i = 0; i < copy_counts; i++) {
					*(WORD *)&uncompr[act_uncomprlen] = word_data;
					act_uncomprlen += 2;
				}
				break;
			case 3:
				dword_data = compr[curbyte];
				curbyte += 3;
				for (i = 0; i < copy_counts; i++) {
					*(DWORD *)&uncompr[act_uncomprlen] = dword_data;
					act_uncomprlen += 3;
				}
				break;
			case 4:
				dword_data = compr[curbyte];
				curbyte += 4;
				for (i = 0; i < copy_counts; i++) {
					*(DWORD *)&uncompr[act_uncomprlen] = word_data;
					act_uncomprlen += 4;
				}
				break;
			}
		} else {
			unsigned int copy_bytes;

			copy_bytes = (flag & 0x7f) + 1;
			memcpy(&uncompr[act_uncomprlen], &compr[curbyte], copy_bytes);
			act_uncomprlen += copy_bytes;
			curbyte += copy_bytes;
		}
	}
out:	
	return act_uncomprlen;	
}

static void wady_adpcm_decode_mono(wady_header_t *wady_header, 
								   u16 *uncompr, u8 *compr, 
								   unsigned int comprlen)
{
	unsigned int curbyte = 0;
	unsigned int act_uncomprlen = 0;	// 以sample为单位
	u16 data = wady_header->left_channel_sample_data_base;

	while (1) {
		u8 code;

		if (curbyte >= comprlen)
			break;
			
		code = compr[curbyte++];
		if (code & 0x80)
			data = code << 9;
		else
			data += wady_header->factor * wady_adpcm_step_table[code];
		uncompr[act_uncomprlen++] = data;
	}
}

static void wady_adpcm_decode_stereo(wady_header_t *wady_header, 
									 u16 *uncompr, u8 *compr, 
									 unsigned int comprlen)
{
	unsigned int curbyte = 0;
	unsigned int act_uncomprlen = 0;	// 以sample为单位
	u16 ldata = wady_header->left_channel_sample_data_base;
	u16 rdata = wady_header->right_channel_sample_data_base;

	while (1) {
		u8 code;

		if (curbyte >= comprlen)
			break;
			
		code = compr[curbyte++];
		if (code & 0x80)
			ldata = code << 9;
		else
			ldata += wady_header->factor * wady_adpcm_step_table[code];
		uncompr[act_uncomprlen++] = ldata;

		if (curbyte >= comprlen)
			break;
			
		code = compr[curbyte++];
		if (code & 0x80)
			rdata = code << 9;
		else
			rdata += wady_header->factor * wady_adpcm_step_table[code];
		uncompr[act_uncomprlen++] = rdata;
	}
}

/********************* wady *********************/

static int MarbleEngine_wady_match(struct package *pkg)
{
	s8 magic[5];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, 5)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "WADY", 5)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (wcsstr(pkg->name, _T("mg_se"))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->pio->seek(pkg, 0, IO_SEEK_SET))  {
		pkg->pio->close(pkg);
		return -CUI_ESEEK;
	}

	return 0;	
}

static int MarbleEngine_wady_extract_resource(struct package *pkg,
											  struct package_resource *pkg_res)
{
	wady_header_t wady_header;
	u32 wady_size;

	wady_adpcm_step_table_init();
	
	if (pkg->pio->length_of(pkg, &wady_size)) 
		return -CUI_ELEN;

	if (pkg->pio->readvec(pkg, &wady_header, sizeof(wady_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	unsigned int comprlen = wady_header.data_length0;
	BYTE *compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;
printf("%d %d\n", wady_header.data_offset, wady_header.nChannels);
printf("%d VS %d VS %d\n", wady_header.data_length0, wady_header.data_length1,
wady_size - sizeof(wady_header));
	if (pkg->pio->readvec(pkg, compr, comprlen, sizeof(wady_header) + wady_header.data_offset, 
			IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	unsigned int uncomprlen = comprlen * 2;	// 以sample为单位（1:2压缩）
	u16 *uncompr = (u16 *)malloc(uncomprlen);
	if (!uncompr) {
		free(compr);
		return -CUI_EMEM;
	}
	
	if (wady_header.nChannels == 1)
		wady_adpcm_decode_mono(&wady_header, uncompr, compr, comprlen);
	else
		wady_adpcm_decode_stereo(&wady_header, uncompr, compr, comprlen);

	free(compr);

	if (MyBuildWAVFile(&wady_header, (BYTE *)uncompr, uncomprlen, (BYTE **)&pkg_res->actual_data,
			(DWORD *)&pkg_res->actual_data_length)) {
		free(uncompr);
		return -CUI_EMEM;
	}
	free(uncompr);

	pkg_res->raw_data = NULL;
	pkg_res->raw_data_length = wady_size;

	return 0;
}

static int MarbleEngine_wady_save_resource(struct resource *res, 
										   struct package_resource *pkg_res)
{
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (res->rio->write(res, pkg_res->actual_data, pkg_res->actual_data_length)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}
	res->rio->close(res);
	
	return 0;
}

static void MarbleEngine_wady_release_resource(struct package *pkg, 
											   struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
}

static void MarbleEngine_wady_release(struct package *pkg, 
									  struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

static cui_ext_operation MarbleEngine_wady_operation = {
	MarbleEngine_wady_match,			/* match */
	NULL,								/* extract_directory */
	NULL,								/* parse_resource_info */
	MarbleEngine_wady_extract_resource,	/* extract_resource */
	MarbleEngine_wady_save_resource,	/* save_resource */
	MarbleEngine_wady_release_resource,	/* release_resource */
	MarbleEngine_wady_release			/* release */
};

/********************* yb *********************/

static int MarbleEngine_yb_match(struct package *pkg)
{
	s8 magic[2];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, 2)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "YB", 2)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->pio->seek(pkg, 0, IO_SEEK_SET))  {
		pkg->pio->close(pkg);
		return -CUI_ESEEK;
	}

	return 0;	
}

static int MarbleEngine_yb_extract_resource(struct package *pkg,
											struct package_resource *pkg_res)
{
	yb_header_t yb_header;
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprLen, act_uncomprlen;
	u32 yb_size;

	if (pkg->pio->length_of(pkg, &yb_size)) 
		return -CUI_ELEN;

	if (pkg->pio->readvec(pkg, &yb_header, sizeof(yb_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	if (yb_header.mode & 0x40)
		uncomprLen = yb_header.uncomprLen;
	else 
		uncomprLen = yb_header.width * yb_header.height * yb_header.bpp;

	compr = (BYTE *)malloc(yb_header.comprLen);
	if (!compr)
		return -CUI_EMEM;
	
	if (pkg->pio->readvec(pkg, compr, yb_header.comprLen, sizeof(yb_header), IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}	

	uncompr = (BYTE *)malloc(uncomprLen);
	if (!uncompr) {
		free(compr);
		return -CUI_EMEM;
	}	
	
	act_uncomprlen = yb_decompress(uncompr, uncomprLen, compr, yb_header.comprLen);
	free(compr);
	if (act_uncomprlen != uncomprLen) {
		free(uncompr);
		return -CUI_EUNCOMPR;
	}

	if (yb_header.mode & 0x40) {
		comprlen = act_uncomprlen;
		uncomprLen = yb_header.width * yb_header.height * yb_header.bpp;
		compr = uncompr;
		uncompr = (BYTE *)malloc(uncomprLen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;			
		}
		
		act_uncomprlen = yb_decompress2(uncompr, uncomprLen, compr, comprlen);
		free(compr);
		if (act_uncomprlen != uncomprLen) {
			free(uncompr);
			return -CUI_EUNCOMPR;
		}
	}

	if (yb_header.mode & 0x80) {
		for (unsigned int j = 0; j < 3; j++) {
			BYTE tmp = 0;
			for (unsigned int k = 0; k < act_uncomprlen; k += 3) {
				uncompr[k + j] += tmp;
				tmp = uncompr[k + j];
			}
		}
	}

	if (MyBuildBMPFile(uncompr, uncomprLen, NULL, 0, yb_header.width, 
			0 - yb_header.height, yb_header.bpp * 8, (BYTE **)&pkg_res->actual_data,
				&pkg_res->actual_data_length, my_malloc)) {
		free(uncompr);
		return -CUI_EMEM;
	}
	free(uncompr);
			
	pkg_res->raw_data = NULL;
	pkg_res->raw_data_length = yb_size;

	return 0;
}

static int MarbleEngine_yb_save_resource(struct resource *res, 
										 struct package_resource *pkg_res)
{
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (res->rio->write(res, pkg_res->actual_data, pkg_res->actual_data_length)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}
	res->rio->close(res);
	
	return 0;
}

static void MarbleEngine_yb_release_resource(struct package *pkg, 
											 struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
}

static void MarbleEngine_yb_release(struct package *pkg, 
									struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

static cui_ext_operation MarbleEngine_yb_operation = {
	MarbleEngine_yb_match,				/* match */
	NULL,								/* extract_directory */
	NULL,								/* parse_resource_info */
	MarbleEngine_yb_extract_resource,	/* extract_resource */
	MarbleEngine_yb_save_resource,		/* save_resource */
	MarbleEngine_yb_release_resource,	/* release_resource */
	MarbleEngine_yb_release				/* release */
};

/********************* mbl *********************/

static int MarbleEngine_mbl_match(struct package *pkg)
{	
	u32 index_entries;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &index_entries, 4)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (!index_entries) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->pio->seek(pkg, 0, IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_ESEEK;
	}

	return 0;	
}

static int mbl_probe_name_length(struct package *pkg, unsigned int files, unsigned int *ret_name_length)
{
	unsigned int name_length = 4;
#if 0
	unsigned long file_size;

	if (pkg->pio->length_of(pkg, &file_size))
		return -CUI_ELEN;

	while (1) {
		u32 offset, length;
		unsigned int entry_size;

		if (pkg->pio->readvec(pkg, &offset, 4, 4 + name_length, IO_SEEK_SET))
			return -CUI_EREADVEC;

		entry_size = name_length + 8;
		if (offset >= (4 + (entry_size) * files)) {
			if (pkg->pio->readvec(pkg, &offset, 4, 4 + entry_size * (files - 1) + name_length, IO_SEEK_SET))
				return -CUI_EREADVEC;

			if (pkg->pio->read(pkg, &length, 4))
				return -CUI_EREAD;

			if (offset + length == file_size)
				break;
		}
		name_length++;
	}
#else
	while (1) {
		u32 offset;

		if (pkg->pio->readvec(pkg, &offset, 4, 4 + name_length, IO_SEEK_SET))
			return -CUI_EREADVEC;
		if (offset == files * (name_length + 8) + 4 + 4 || offset == files * (name_length + 8) + 4)
			break;
		name_length++;
	}
#endif
	*ret_name_length = name_length;
	return 0;
}

static int MarbleEngine_mbl_extract_directory(struct package *pkg,
											  struct package_directory *pkg_dir)
{		
	unsigned int index_buffer_len;
	void *index_buffer;
	mbl_header_t mbl_header;

	if (!_tcsicmp(pkg->name, _T("mg_data.mbl"))) {
		if (pkg->pio->read(pkg, &mbl_header.index_entries, 4))
			return -CUI_EREAD;
		if (mbl_probe_name_length(pkg, mbl_header.index_entries, &mbl_header.name_length))
			return -CUI_EMATCH;
		if (pkg->pio->seek(pkg, 4, IO_SEEK_SET))
			return -CUI_ESEEK;
	} else {
		if (pkg->pio->read(pkg, &mbl_header, sizeof(mbl_header)))
			return -CUI_EREAD;

		/* 罪悪感 「お願い…こんな姿見ないで」 @ mg_gra.mbl */
		if (mbl_header.name_length > 32) {
			u32 tmp;

			tmp = mbl_header.name_length;
			mbl_header.name_length = mbl_header.index_entries;
			mbl_header.index_entries = tmp;
		}
	}

	index_buffer_len = mbl_header.index_entries * (mbl_header.name_length + 8);		
	index_buffer = malloc(index_buffer_len);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_len)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = mbl_header.index_entries;
	pkg_dir->index_entry_length = mbl_header.name_length + 8;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_len;

	return 0;
}

static int MarbleEngine_mbl_parse_resource_info(struct package *pkg,
												struct package_resource *pkg_res)
{
	void *mbl_entry;

	mbl_entry = pkg_res->actual_index_entry;
	pkg_res->name_length = pkg_res->actual_index_entry_length - 8;
	strncpy(pkg_res->name, (char *)mbl_entry, pkg_res->name_length);	
	pkg_res->offset = *(u32 *)((s8 *)mbl_entry + pkg_res->name_length);
	pkg_res->actual_data_length = 0;
	pkg_res->raw_data_length = *((u32 *)((s8 *)mbl_entry + pkg_res->name_length + 4));

	return 0;
}

static int MarbleEngine_mbl_extract_resource(struct package *pkg,
											 struct package_resource *pkg_res)
{
	BYTE *data, *act_data = NULL;
	DWORD act_len = 0;
	TCHAR name[8];

	data = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, data, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET)) {
		free(data);
		return -CUI_EREADVEC;
	}

	_tcsncpy(name, pkg->name, 7);
	name[7] = 0;
	if (!lstrcmpi(name, _T("mg_data"))) {
		const char *dec_key;
		unsigned int i;

		dec_key = get_options("dec_key");
		if (dec_key) {
			int str_len = strlen(dec_key);;

			for (i = 0; i < pkg_res->raw_data_length; i++)
				data[i] ^= dec_key[i % str_len];
		} else {
			for (i = 0; i < pkg_res->raw_data_length; i++)
				data[i] = 0 - data[i];
		}

		pkg_res->replace_extension = _T(".S");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
	} else if (!lstrcmpi(name, _T("mg_anim"))) {
		pkg_res->replace_extension = _T(".anim");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;		
	} else {
		BYTE *comp_data;

		if (!memcmp(data, "\x78\x9c", 2)) {
			act_len = pkg_res->raw_data_length;
			while (1) {
				act_len <<= 1;
				act_data = (BYTE *)malloc(act_len);
				if (!act_data) {
					free(data);
					return -CUI_EMEM;
				}
				if (uncompress(act_data, &act_len, data, pkg_res->raw_data_length) == Z_OK)
					break;
				free(act_data);
			}
			comp_data = act_data;
		} else
			comp_data = data;

		if (!memcmp(comp_data, "BM", 2)) {
			pkg_res->replace_extension = _T(".bmp");
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
		} else if (!memcmp(comp_data, "YB", 2)) {
			pkg_res->replace_extension = _T(".yb");
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
		} else if (!memcmp(comp_data, "WADY", 5)) {
			pkg_res->replace_extension = _T(".wady");
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
		} else if (!memcmp(comp_data, "\xff\xd8\xff\xe0\x00\x10JFIF", 10)) {
			pkg_res->replace_extension = _T(".jpg");
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
		} else if (!memcmp(comp_data, "OggS", 4)) {
			pkg_res->replace_extension = _T(".ogg");
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
		} else if (!memcmp(comp_data, "RIFF", 4)) {
			pkg_res->replace_extension = _T(".wav");
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
		}
	}
	
	pkg_res->raw_data = data;
	pkg_res->actual_data = act_data;
	pkg_res->actual_data_length = act_len;	

	return 0;
}

static int MarbleEngine_mbl_save_resource(struct resource *res, 
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

static void MarbleEngine_mbl_release_resource(struct package *pkg, 
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

static void MarbleEngine_mbl_release(struct package *pkg, 
									 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg);
}

static cui_ext_operation MarbleEngine_mbl_operation = {
	MarbleEngine_mbl_match,					/* match */
	MarbleEngine_mbl_extract_directory,		/* extract_directory */
	MarbleEngine_mbl_parse_resource_info,	/* parse_resource_info */
	MarbleEngine_mbl_extract_resource,		/* extract_resource */
	MarbleEngine_mbl_save_resource,			/* save_resource */
	MarbleEngine_mbl_release_resource,		/* release_resource */
	MarbleEngine_mbl_release				/* release */
};

/********************* mg_anim.mbl *********************/

static int MarbleEngine_anim_match(struct package *pkg)
{	
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

static int MarbleEngine_anim_extract_directory(struct package *pkg,
											   struct package_directory *pkg_dir)
{	
	anim_header_t anim_header;
	if (pkg->pio->read(pkg, &anim_header, sizeof(anim_header)))
		return -CUI_EREAD;

#if 0	
	if (anim_header.unknown0 == 0x21)
		unknown0 = 33333;
	else if (anim_header.unknown0 == 0x0f)
		unknown0 = 66666;
	else
		unknown0 = anim_header.unknown0 * 1000;
#endif

	DWORD skip_offset = anim_header.frames * 2 /* flag */ + anim_header.frames * 2
		+ anim_header.frames * 2;

	if (pkg->pio->seek(pkg, skip_offset, IO_SEEK_CUR))
		return -CUI_ESEEK;

	u32 *offset_index = (u32 *)malloc(anim_header.frames * 4);
	if (!offset_index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, offset_index, anim_header.frames * 4)) {
		free(offset_index);
		return -CUI_EREAD;
	}

	u32 *length_index = (u32 *)malloc(anim_header.frames * 4);
	if (!length_index) {
		free(offset_index);
		return -CUI_EMEM;
	}

	if (pkg->pio->read(pkg, length_index, anim_header.frames * 4)) {
		free(length_index);
		free(offset_index);
		return -CUI_EREAD;
	}

	DWORD index_entries = anim_header.frames;
	if (anim_header.audio_data_size)
		++index_entries;

	DWORD index_buffer_len = index_entries * sizeof(anim_entry_t);		
	anim_entry_t *index_buffer = (anim_entry_t *)malloc(index_buffer_len);
	if (!index_buffer) {
		free(length_index);
		free(offset_index);
		return -CUI_EMEM;
	}

	for (DWORD i = 0; i < anim_header.frames; ++i) {
		sprintf(index_buffer[i].name, "%08d.jpg", i);
		index_buffer[i].offset = offset_index[i];
		index_buffer[i].length = length_index[i];
	}
	free(length_index);
	free(offset_index);

	if (anim_header.audio_data_size) {
		strcpy(index_buffer[i].name, "00000000.wady");
		index_buffer[i].offset = anim_header.video_data_size;
		index_buffer[i].length = anim_header.audio_data_size;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->index_entry_length = sizeof(anim_entry_t);
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_len;

	return 0;
}

static int MarbleEngine_anim_parse_resource_info(struct package *pkg,
												 struct package_resource *pkg_res)
{
	anim_entry_t *anim_entry;

	anim_entry = (anim_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, anim_entry->name);	
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->offset = anim_entry->offset;
	pkg_res->actual_data_length = 0;
	pkg_res->raw_data_length = anim_entry->length;
	return 0;
}

static int MarbleEngine_anim_extract_resource(struct package *pkg,
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
	pkg_res->raw_data = data;

	return 0;
}

static cui_ext_operation MarbleEngine_anim_operation = {
	MarbleEngine_anim_match,				/* match */
	MarbleEngine_anim_extract_directory,	/* extract_directory */
	MarbleEngine_anim_parse_resource_info,	/* parse_resource_info */
	MarbleEngine_anim_extract_resource,		/* extract_resource */
	MarbleEngine_mbl_save_resource,			/* save_resource */
	MarbleEngine_mbl_release_resource,		/* release_resource */
	MarbleEngine_mbl_release				/* release */
};
	
int CALLBACK MarbleEngine_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".mbl"), NULL, 
		NULL, &MarbleEngine_mbl_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".yb"), _T(".bmp"), 
		NULL, &MarbleEngine_yb_operation, CUI_EXT_FLAG_RES))
			return -1;

//	if (callback->add_extension(callback->cui, _T(".wady"), _T(".wav"), 
//		NULL, &MarbleEngine_wady_operation, CUI_EXT_FLAG_RES))
//			return -1;

	if (callback->add_extension(callback->cui, _T(".anim"), NULL, 
		NULL, &MarbleEngine_anim_operation, CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	return 0;
}
/*
0040D7A8  /$  55            PUSH EBP
0040D7A9  |.  8BEC          MOV EBP,ESP
0040D7AB  |.  83C4 A0       ADD ESP,-60
0040D7AE  |.  56            PUSH ESI
0040D7AF  |.  57            PUSH EDI
0040D7B0  |.  894D F4       MOV DWORD PTR SS:[EBP-C],ECX                                          ;  uncompr
0040D7B3  |.  8955 F8       MOV DWORD PTR SS:[EBP-8],EDX                                          ;  compr
0040D7B6  |.  8945 FC       MOV DWORD PTR SS:[EBP-4],EAX
0040D7B9  |.  8D7D D4       LEA EDI,DWORD PTR SS:[EBP-2C]                                         ;  (u32)step_table
0040D7BC  |.  B9 08000000   MOV ECX,8
0040D7C1  |.  BE A0955000   MOV ESI,aqua_try.005095A0
0040D7C6  |.  F3:A5         REP MOVS DWORD PTR ES:[EDI],DWORD PTR DS:[ESI]                        ;  copy step_table(32 bytes)
0040D7C8  |.  8B45 F8       MOV EAX,DWORD PTR SS:[EBP-8]                                          ;  compr
0040D7CB  |.  8B50 04       MOV EDX,DWORD PTR DS:[EAX+4]                                          ;  compr.[4]??_len
0040D7CE  |.  8955 B0       MOV DWORD PTR SS:[EBP-50],EDX
0040D7D1  |.  8B4D F8       MOV ECX,DWORD PTR SS:[EBP-8]
0040D7D4  |.  66:8B41 08    MOV AX,WORD PTR DS:[ECX+8]                                            ;  compr.[8](u16)
0040D7D8  |.  66:8945 D2    MOV WORD PTR SS:[EBP-2E],AX
0040D7DC  |.  C745 C0 0A000>MOV DWORD PTR SS:[EBP-40],0A
0040D7E3  |.  66:8B55 D2    MOV DX,WORD PTR SS:[EBP-2E]                                           ;  [-2e] = compr.[8](u16)
0040D7E7  |.  8B4D F4       MOV ECX,DWORD PTR SS:[EBP-C]
0040D7EA  |.  66:8911       MOV WORD PTR DS:[ECX],DX                                              ;  *(u16 *)uncompr = compr.[8](u16)
0040D7ED  |.  0FBF45 08     MOVSX EAX,WORD PTR SS:[EBP+8]                                         ;  nChannels
0040D7F1  |.  03C0          ADD EAX,EAX                                                           ;  nChannels * 2
0040D7F3  |.  33D2          XOR EDX,EDX
0040D7F5  |.  0145 F4       ADD DWORD PTR SS:[EBP-C],EAX
0040D7F8  |.  8955 C8       MOV DWORD PTR SS:[EBP-38],EDX                                         ;  i = 0
0040D7FB  |.  8B4D F8       MOV ECX,DWORD PTR SS:[EBP-8]                                          ;  compr
0040D7FE  |.  8B45 C0       MOV EAX,DWORD PTR SS:[EBP-40]
0040D801  |.  8D1401        LEA EDX,DWORD PTR DS:[ECX+EAX]                                        ;  cur_compr = &compr[10]
0040D804  |.  8955 A4       MOV DWORD PTR SS:[EBP-5C],EDX
0040D807  |.  8B4D C8       MOV ECX,DWORD PTR SS:[EBP-38]
0040D80A  |.  3B4D B0       CMP ECX,DWORD PTR SS:[EBP-50]                                         ;  i VS compr.[4]??_len
0040D80D  |.  0F8D F1000000 JGE aqua_try.0040D904
0040D813  |>  8B45 B0       /MOV EAX,DWORD PTR SS:[EBP-50]                                        ;  compr.[4]??_len
0040D816  |.  05 D4FEFFFF   |ADD EAX,-12C                                                         ;  compr.[4]??_len - 0x12c
0040D81B  |.  3B45 C8       |CMP EAX,DWORD PTR SS:[EBP-38]                                        ;  compr.[4]??_len - 0x12c Vs i
0040D81E  |.  75 06         |JNZ SHORT aqua_try.0040D826
0040D820  |.  66:C745 D2 00>|MOV WORD PTR SS:[EBP-2E],0                                           ;  compr.[4]??_len - 0x12c == i: [-2e] = 0
0040D826  |>  8B55 A4       |MOV EDX,DWORD PTR SS:[EBP-5C]
0040D829  |.  66:8B0A       |MOV CX,WORD PTR DS:[EDX]                                             ;  flag = *(u16 *)cur_compr
0040D82C  |.  66:894D D0    |MOV WORD PTR SS:[EBP-30],CX
0040D830  |.  FF45 A4       |INC DWORD PTR SS:[EBP-5C]                                            ;  (u8 *)cur_compr++
0040D833  |.  F645 D0 01    |TEST BYTE PTR SS:[EBP-30],1
0040D837  |.  74 46         |JE SHORT aqua_try.0040D87F
0040D839  |.  0FBF45 D0     |MOVSX EAX,WORD PTR SS:[EBP-30]                                       ;  flag is set:
0040D83D  |.  D1F8          |SAR EAX,1
0040D83F  |.  66:83E0 7F    |AND AX,7F
0040D843  |.  66:8945 D0    |MOV WORD PTR SS:[EBP-30],AX
0040D847  |.  F645 D0 40    |TEST BYTE PTR SS:[EBP-30],40
0040D84B  |.  74 0D         |JE SHORT aqua_try.0040D85A
0040D84D  |.  66:8B55 D0    |MOV DX,WORD PTR SS:[EBP-30]
0040D851  |.  C1E2 0A       |SHL EDX,0A
0040D854  |.  66:8955 D2    |MOV WORD PTR SS:[EBP-2E],DX
0040D858  |.  EB 10         |JMP SHORT aqua_try.0040D86A
0040D85A  |>  0FBF4D D0     |MOVSX ECX,WORD PTR SS:[EBP-30]
0040D85E  |.  66:8B044D FC2>|MOV AX,WORD PTR DS:[ECX*2+522CFC]
0040D866  |.  66:0145 D2    |ADD WORD PTR SS:[EBP-2E],AX
0040D86A  |>  66:8B55 D2    |MOV DX,WORD PTR SS:[EBP-2E]
0040D86E  |.  8B4D F4       |MOV ECX,DWORD PTR SS:[EBP-C]
0040D871  |.  66:8911       |MOV WORD PTR DS:[ECX],DX
0040D874  |.  0FBF45 08     |MOVSX EAX,WORD PTR SS:[EBP+8]
0040D878  |.  03C0          |ADD EAX,EAX
0040D87A  |.  0145 F4       |ADD DWORD PTR SS:[EBP-C],EAX
0040D87D  |.  EB 76         |JMP SHORT aqua_try.0040D8F5
0040D87F  |>  FF45 A4       |INC DWORD PTR SS:[EBP-5C]                                            ;  flag is not set: (u8 *)cur_compr++
0040D882  |.  0FBF55 D0     |MOVSX EDX,WORD PTR SS:[EBP-30]                                       ;  (u16)flag
0040D886  |.  D1FA          |SAR EDX,1                                                            ;  step_idx = ((u16)flag >> 1) & 7
0040D888  |.  83E2 07       |AND EDX,7
0040D88B  |.  8B4C95 D4     |MOV ECX,DWORD PTR SS:[EBP+EDX*4-2C]                                  ;  step_table[step_idx]
0040D88F  |.  894D BC       |MOV DWORD PTR SS:[EBP-44],ECX
0040D892  |.  33C9          |XOR ECX,ECX
0040D894  |.  66:8365 D0 F0 |AND WORD PTR SS:[EBP-30],0FFF0                                       ;  flag &= 0xfff0
0040D899  |.  0FBF45 D0     |MOVSX EAX,WORD PTR SS:[EBP-30]
0040D89D  |.  0FBF55 D2     |MOVSX EDX,WORD PTR SS:[EBP-2E]
0040D8A1  |.  2BC2          |SUB EAX,EDX                                                          ;  flag - [-2e]
0040D8A3  |.  8945 A0       |MOV DWORD PTR SS:[EBP-60],EAX
0040D8A6  |.  DB45 A0       |FILD DWORD PTR SS:[EBP-60]
0040D8A9  |.  DB45 BC       |FILD DWORD PTR SS:[EBP-44]
0040D8AC  |.  DEF9          |FDIVP ST(1),ST
0040D8AE  |.  D95D A8       |FSTP DWORD PTR SS:[EBP-58]
0040D8B1  |.  DF45 D2       |FILD WORD PTR SS:[EBP-2E]
0040D8B4  |.  D95D AC       |FSTP DWORD PTR SS:[EBP-54]
0040D8B7  |.  894D C4       |MOV DWORD PTR SS:[EBP-3C],ECX
0040D8BA  |.  8B45 C4       |MOV EAX,DWORD PTR SS:[EBP-3C]
0040D8BD  |.  3B45 BC       |CMP EAX,DWORD PTR SS:[EBP-44]
0040D8C0  |.  7D 2B         |JGE SHORT aqua_try.0040D8ED
0040D8C2  |>  D945 A8       |/FLD DWORD PTR SS:[EBP-58]
0040D8C5  |.  D845 AC       ||FADD DWORD PTR SS:[EBP-54]
0040D8C8  |.  D95D AC       ||FSTP DWORD PTR SS:[EBP-54]
0040D8CB  |.  D945 AC       ||FLD DWORD PTR SS:[EBP-54]
0040D8CE  |.  E8 45D50D00   ||CALL aqua_try.004EAE18
0040D8D3  |.  8B55 F4       ||MOV EDX,DWORD PTR SS:[EBP-C]
0040D8D6  |.  66:8902       ||MOV WORD PTR DS:[EDX],AX
0040D8D9  |.  0FBF4D 08     ||MOVSX ECX,WORD PTR SS:[EBP+8]
0040D8DD  |.  03C9          ||ADD ECX,ECX
0040D8DF  |.  014D F4       ||ADD DWORD PTR SS:[EBP-C],ECX
0040D8E2  |.  FF45 C4       ||INC DWORD PTR SS:[EBP-3C]
0040D8E5  |.  8B45 C4       ||MOV EAX,DWORD PTR SS:[EBP-3C]
0040D8E8  |.  3B45 BC       ||CMP EAX,DWORD PTR SS:[EBP-44]
0040D8EB  |.^ 7C D5         |\JL SHORT aqua_try.0040D8C2
0040D8ED  |>  66:8B55 D0    |MOV DX,WORD PTR SS:[EBP-30]
0040D8F1  |.  66:8955 D2    |MOV WORD PTR SS:[EBP-2E],DX
0040D8F5  |>  FF45 C8       |INC DWORD PTR SS:[EBP-38]
0040D8F8  |.  8B4D C8       |MOV ECX,DWORD PTR SS:[EBP-38]
0040D8FB  |.  3B4D B0       |CMP ECX,DWORD PTR SS:[EBP-50]
0040D8FE  |.^ 0F8C 0FFFFFFF \JL aqua_try.0040D813
0040D904  |>  5F            POP EDI
0040D905  |.  5E            POP ESI
0040D906  |.  8BE5          MOV ESP,EBP
0040D908  |.  5D            POP EBP
0040D909  \.  C2 0400       RETN 4
*/
#if 0
005095A0  03 00 00 00 04 00 00 00 05 00 00 00 06 00 00 00  ................
005095B0  08 00 00 00 10 00 00 00 20 00 00 00 00 01 00 00  ........ .......
#endif
