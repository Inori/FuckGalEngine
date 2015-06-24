#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <resource.h>
#include <package.h>
#include <cui_error.h>
#include <utility.h>
#include "port.h"
#include "qsmodel.h"
#include "rangecod.h"
#include <stdio.h>

struct acui_information Hypatia_cui_information = {
	_T("Hypatia(KGD)"),			/* copyright */
	_T(""),						/* system */
	_T(".pak .dat"),			/* package */
	_T("1.0.0"),				/* revision */
	_T("痴汉公贼"),				/* author */
	_T("2009-1-3 18:53"),		/* date */
	NULL,						/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

#pragma pack (1)
/*
	0x100:
		?

	0x200:
		?

	0x300:
		蒼い空のネオスフィア どきどきアドベンチャー Effective E

	0x301:
		?

 */
typedef struct {
	s8 magic[6];
	u16 version;
	u32 index_offset;
	u32 index_entries;
} pak_header_t;

typedef struct {	// 32 bytes
	s8 name[21];
	s8 suffix[3];
	u32 offset;
	u32 length;
} pak1_entry_t;

typedef struct {	// 40 bytes
	s8 name[21];
	s8 suffix[3];
	u32 offset;
	u32 uncomprLen;
	u32 comprLen;
	u8 mode;
	u8 pad[3];
} pak2_entry_t;

typedef struct {	// 48 bytes
	s8 name[21];
	s8 suffix[3];
	u32 offset;		// need plus sizeof(pak_header_t)
	u32 uncomprLen;
	u32 comprLen;
	u8 mode;		// 1 - lz压缩变体 2 - BWT+MTF+RangeCoder 3 - 数据取反 etc - 原始数据
	u8 do_crc_check;
	u16 crc;
	FILETIME time_stamp;
} pak3_entry_t;
#pragma pack ()

typedef	pak3_entry_t		pak_entry_t;


static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static int qsmodel_init_array[257] = {
		0x00000578, 0x00000280, 0x00000140, 0x000000f0, 
		0x000000a0, 0x00000078, 0x00000050, 0x00000040, 
		0x00000030, 0x00000028, 0x00000020, 0x00000018, 
		0x00000014, 0x00000014, 0x00000014, 0x00000014, 
		0x00000010, 0x00000010, 0x00000010, 0x00000010, 
		0x0000000c, 0x0000000c, 0x0000000c, 0x0000000c, 
		0x0000000c, 0x0000000c, 0x00000008, 0x00000008, 
		0x00000008, 0x00000008, 0x00000008, 0x00000008, 
		0x00000006, 0x00000006, 0x00000006, 0x00000006, 
		0x00000006, 0x00000006, 0x00000006, 0x00000006, 
		0x00000006, 0x00000006, 0x00000006, 0x00000006, 
		0x00000006, 0x00000006, 0x00000006, 0x00000006, 
		0x00000005, 0x00000005, 0x00000005, 0x00000005, 
		0x00000005, 0x00000005, 0x00000005, 0x00000005, 
		0x00000005, 0x00000005, 0x00000005, 0x00000005, 
		0x00000005, 0x00000005, 0x00000005, 0x00000005, 
		0x00000004, 0x00000004, 0x00000004, 0x00000004, 
		0x00000004, 0x00000004, 0x00000004, 0x00000004, 
		0x00000004, 0x00000004, 0x00000004, 0x00000004, 
		0x00000004, 0x00000004, 0x00000004, 0x00000004, 
		0x00000004, 0x00000004, 0x00000004, 0x00000004, 
		0x00000004, 0x00000004, 0x00000004, 0x00000004, 
		0x00000004, 0x00000004, 0x00000004, 0x00000004, 
		0x00000004, 0x00000004, 0x00000004, 0x00000004, 
		0x00000003, 0x00000003, 0x00000003, 0x00000003, 
		0x00000003, 0x00000003, 0x00000003, 0x00000003, 
		0x00000003, 0x00000003, 0x00000003, 0x00000003, 
		0x00000003, 0x00000003, 0x00000003, 0x00000003, 
		0x00000003, 0x00000003, 0x00000003, 0x00000003, 
		0x00000003, 0x00000003, 0x00000003, 0x00000003, 
		0x00000003, 0x00000003, 0x00000003, 0x00000003, 
		0x00000003, 0x00000003, 0x00000003, 0x00000003, 
		0x00000003, 0x00000003, 0x00000003, 0x00000003, 
		0x00000003, 0x00000003, 0x00000002, 0x00000002, 
		0x00000002, 0x00000002, 0x00000002, 0x00000002, 
		0x00000002, 0x00000002, 0x00000002, 0x00000002, 
		0x00000002, 0x00000002, 0x00000002, 0x00000002, 
		0x00000002, 0x00000002, 0x00000002, 0x00000002, 
		0x00000002, 0x00000002, 0x00000002, 0x00000002, 
		0x00000002, 0x00000002, 0x00000002, 0x00000002, 
		0x00000002, 0x00000002, 0x00000002, 0x00000002, 
		0x00000002, 0x00000002, 0x00000002, 0x00000002, 
		0x00000002, 0x00000002, 0x00000002, 0x00000002, 
		0x00000002, 0x00000002, 0x00000002, 0x00000002, 
		0x00000002, 0x00000002, 0x00000002, 0x00000002, 
		0x00000002, 0x00000002, 0x00000002, 0x00000002, 
		0x00000002, 0x00000002, 0x00000002, 0x00000002, 
		0x00000002, 0x00000002, 0x00000002, 0x00000002, 
		0x00000002, 0x00000002, 0x00000002, 0x00000002, 
		0x00000002, 0x00000002, 0x00000002, 0x00000002, 
		0x00000002, 0x00000002, 0x00000002, 0x00000002, 
		0x00000002, 0x00000002, 0x00000002, 0x00000002, 
		0x00000002, 0x00000002, 0x00000002, 0x00000002, 
		0x00000002, 0x00000002, 0x00000002, 0x00000002, 
		0x00000002, 0x00000002, 0x00000002, 0x00000002, 
		0x00000002, 0x00000002, 0x00000002, 0x00000002, 
		0x00000002, 0x00000002, 0x00000002, 0x00000002, 
		0x00000002, 0x00000002, 0x00000002, 0x00000002, 
		0x00000002, 0x00000002, 0x00000002, 0x00000002, 
		0x00000002, 0x00000002, 0x00000002, 0x00000002, 
		0x00000002, 0x00000002, 0x00000002, 0x00000002, 
		0x00000002, 0x00000002, 0x00000002, 0x00000002, 
		0x00000002, 0x00000002, 0x00000002, 0x00000002, 
		0x00000002, 0x00000002, 0x00000002, 0x00000002, 
		0x00000002
};

static inline unsigned char getbit_be(unsigned int dword, unsigned int pos)
{
	return !!(dword & (1 << (31 - pos)));
}

#if 1
static void MarielDecode(BYTE *uncompr, DWORD uncomprLen, 
						 BYTE *compr, DWORD comprLen)
{
	DWORD act_uncomprlen = 0;
	DWORD curbyte = 0;		/* compr中的当前扫描字节 */
	DWORD mask = 0;

	while (act_uncomprlen < uncomprLen || curbyte < comprLen) {
		u32 flag;

		if (!mask) {
			flag = *(u32 *)(compr + curbyte);
			curbyte += 4;
			mask = 0x80000000;
		}

		if (flag & mask) {
			DWORD copy_bytes, win_offset;

			copy_bytes = compr[curbyte++];
			win_offset = copy_bytes & 0x0f;
			copy_bytes >>= 4;
			if (copy_bytes == 0x0f) {
				copy_bytes = *(u16 *)(compr + curbyte);
				curbyte += 2;
			} else if (copy_bytes == 0x0e)
				copy_bytes = compr[curbyte++];					
			else
				++copy_bytes;

			if (win_offset >= 0x0a)
				win_offset = (win_offset << 8) + compr[curbyte++] - 0xa00;
			else
				++win_offset;

			for (DWORD i = 0; i < copy_bytes; ++i)
				uncompr[act_uncomprlen + i] = uncompr[act_uncomprlen - win_offset + i];
			act_uncomprlen += copy_bytes;
		} else
			uncompr[act_uncomprlen++] = compr[curbyte++];

		mask >>= 1;
	}
}
#else
static DWORD MarielDecode(BYTE *uncompr, DWORD uncomprLen, BYTE *compr, DWORD comprLen)
{
	DWORD act_uncomprlen = 0;
	DWORD curbyte = 0;		/* compr中的当前扫描字节 */
	
	while (1) {
		DWORD bits = 32;
		u32 flag;

		if (curbyte + 4 > comprLen)
			break;

		flag = *(u32 *)(compr + curbyte);
		curbyte += 4;

		for (unsigned int curbit = 0; curbit < bits; curbit++) {
			if (getbit_be(flag, curbit)) {
				unsigned int copy_bytes, win_offset;
				
				if (curbyte >= comprLen)
					goto out;	

				copy_bytes = compr[curbyte++];
				win_offset = copy_bytes & 0x0f;
				copy_bytes >>= 4;
				if (copy_bytes == 0x0f) {
					if (curbyte + 1 >= comprLen)
						goto out;
					copy_bytes = *(unsigned short *)(compr + curbyte);
					curbyte += 2;
				} else if (copy_bytes == 0x0e) {
					if (curbyte >= comprLen)
						goto out;
					copy_bytes = compr[curbyte++];					
				} else
					copy_bytes++;

				if (win_offset >= 0x0a) {
					win_offset = (win_offset << 8) - 0xa00;
					if (curbyte >= comprLen)
						goto out;	
					win_offset += compr[curbyte++];
				} else
					win_offset++;

				for (unsigned int i = 0; i < copy_bytes; i++) {
					if (act_uncomprlen + i >= uncomprLen)
						goto out;	

					uncompr[act_uncomprlen + i] = uncompr[act_uncomprlen - win_offset + i];
				}
				act_uncomprlen += copy_bytes;
			} else {
				if (act_uncomprlen >= uncomprLen)
					goto out;	
				if (curbyte >= comprLen)
					goto out;	
				uncompr[act_uncomprlen++] = compr[curbyte++];
			}
		}
	}
out:
	return act_uncomprlen;
}
#endif

static void NotCoding(BYTE *dat, DWORD len)
{
	for (DWORD i = 0; i < len; ++i)
		*dat++ = ~*dat++;
}

static BYTE *bwt_in_buffer;
static UINT bwt_in_buffer_length;

static int bwt_decode_compare(const void *index0, const void *index1)
{
	INT index[2] = { *(INT *)index0, *(INT *)index1 };
	int ret = bwt_in_buffer[index[0]] - bwt_in_buffer[index[1]];
	if (!ret)
		ret = index[0] - index[1];
	return ret;
}

static int bwt_decode(BYTE *out_buf, BYTE *L, UINT buf_len, UINT pi_index)
{
	UINT *T, *F;	

	F = (UINT *)malloc(buf_len * sizeof(UINT));
	if (!F)
		return -1;
	
	bwt_in_buffer = L;
	bwt_in_buffer_length = buf_len;
		
	/* F是L的排序结果 */
	for (UINT i = 0; i < buf_len; i++)
		F[i] = i;
	qsort(F, buf_len, sizeof(UINT), bwt_decode_compare);

	T = F;
	
	UINT index = T[pi_index];
	for (i = 0 ; i < buf_len; i++) {
		out_buf[i] = L[index];
		index = T[index];
	}
	free(F);

	return 0;
}

static int bwt_decode2(BYTE *out_buf, BYTE *L, UINT buf_len, UINT pi)
{
	UINT *count, *T;
	UINT **p;

	count = (UINT *)malloc(256 * sizeof(UINT));
	if (!count)
		return -1;

	p = (UINT **)malloc(256 * sizeof(UINT *));
	if (!p) {
		free(count);
		return -1;
	}

	T = (UINT *)malloc((buf_len + 1) * 2 * sizeof(UINT));
	if (!T) {
		free(p);
		free(count);
		return -1;
	}

	memset(count, 0, 256 * sizeof(UINT));
	for (UINT i = 0; i < buf_len; i++)
		count[L[i]]++;

	p[0] = &T[count[0]];
	for (i = 1; i < 256; i++) {
		count[i] += count[i - 1];
		p[i] = &T[count[i]];
	}

	for (INT k = buf_len - 1; k >= 0 ; k--) {
		p[L[k]]--;
		*p[L[k]] = k;
	}
	
	UINT index = T[pi];
	for (i = 0 ; i < buf_len; i++) {
		out_buf[i] = L[index];
		index = T[index];
	}
	free(T);
	free(p);
	free(count);

	return 0;
}

static int CocotteDecode(unsigned char *uncompr, unsigned int uncomprLen, 
						 unsigned char *compr, unsigned int comprLen)
{
	qsmodel qsm;
	unsigned int act_comprlen = 0;
	unsigned int act_uncomprlen = 0;	
	int err = 0;
	unsigned char mtf_table[256];
	unsigned int i;

	if (initqsmodel(&qsm, 257, 12, 2000, qsmodel_init_array))
		return -CUI_EUNCOMPR;

	for (i = 0; i < 256; i++)
  		mtf_table[i] = i;

	while (1) {
		rangecoder rc;
		int ch, syfreq, ltfreq;
		unsigned int compr_block_len, uncompr_block_len;
		unsigned int act_uncompr_block_len;
		unsigned char *block;

		compr_block_len = *(u16 *)compr - 4;
		compr += 2;
		uncompr_block_len = *(u16 *)compr;	/* 2字节pi索引 */
		compr += 2;

		if (act_uncomprlen + uncompr_block_len > uncomprLen) {
			return -CUI_EUNCOMPR;
		}

		uncompr_block_len += 2;

		if (act_comprlen + compr_block_len > comprLen) {
			return -CUI_EUNCOMPR;	
		}

		block = (unsigned char *)malloc(uncompr_block_len);
		if (!block)
			return -CUI_EMEM;
	
		/****** RangeCoder decode ******/
		act_uncompr_block_len = 0;

		if (compr_block_len == uncompr_block_len) {
			memcpy(block, compr, compr_block_len);
			deleteqsmodel(&qsm);
			initqsmodel(&qsm, 257, 12, 2000, qsmodel_init_array);
		} else {
			start_decoding(&rc, compr, compr_block_len, block, uncompr_block_len);

			while (1) {
				ltfreq = decode_culshift(&rc, 12);	
				ch = qsgetsym(&qsm, ltfreq);
				if (ch == 256)  /* check for end-of-file */
					break;
				block[act_uncompr_block_len++] = ch;		
				qsgetfreq(&qsm, ch, &syfreq, &ltfreq);    
				decode_update(&rc, syfreq, ltfreq, 4096);
				qsupdate(&qsm, ch);
			}
			qsgetfreq(&qsm, 256, &syfreq, &ltfreq);
			decode_update(&rc, syfreq, ltfreq, 4096);
			done_decoding(&rc);

			if (act_uncompr_block_len != uncompr_block_len) {
				crass_printf(_T("decompress miss-match (%d VS %d)\n"),
					act_uncompr_block_len, uncompr_block_len);
				free(block);
				return -CUI_EUNCOMPR;
			}
		}

		/****** MTF decode ******/
		for (i = 0; i < act_uncompr_block_len; i++) {
			unsigned char pos = block[i];
			unsigned char code;
			
			code = mtf_table[pos];
			for (unsigned int k = 0; k < pos; k++)
				mtf_table[pos - k] = mtf_table[pos - k - 1];
			block[i] = mtf_table[0] = code;
		}

		/****** BWT decode ******/
		unsigned int pi_idx = *(u16 *)block;
		act_uncompr_block_len -= 2;	/* 2字节pi索引 */

		BYTE *out_buf = (BYTE *)malloc(act_uncompr_block_len);
		if (!out_buf) {
			free(block);
			err = -CUI_EMEM;
			break;
		}

		if (bwt_decode(out_buf, block + 2, act_uncompr_block_len, pi_idx)) {
			free(out_buf);
			free(block);
			err = -CUI_EMEM;
			break;
		}

		compr += compr_block_len;
		memcpy(uncompr, out_buf, act_uncompr_block_len);
		free(out_buf);
		free(block);
		uncompr += act_uncompr_block_len;	/* need modify */
		/* pak->entry->comprLen字段比实际的act_comprlen多blocks * 4字节 */
		act_comprlen += compr_block_len + 4;
		act_uncomprlen += act_uncompr_block_len;

		if (act_comprlen == comprLen && act_uncomprlen == uncomprLen)
			break;
	}
	deleteqsmodel(&qsm);

	return err;
}

static u16 crc_table[256];

/*
 * Calculate CCITT (HDLC, X25) CRC
 */
static void CrcCheck_MakeCrcTable(int poly_sel)
{
	u16 poly;

	if (poly_sel)
		poly = 0xC001;
	else
		poly = 0x8408;

    for (u16 i = 0; i < 256; ++i) {
		u16 crc = i;
        for (DWORD k = 0; k < 8; ++k) {
            if (crc & 1)
                crc = (crc >> 1) ^ poly;
            else
                crc >>= 1;
        }
		crc_table[i] = crc;
    }
}

static u16 CrcCheck_UpDateCrc(u16 crc, BYTE *dat, DWORD dat_len)
{
	for (DWORD i = 0; i < dat_len; ++i)
		crc = (crc >> 8) ^ crc_table[dat[i] ^ (u8)crc];

	return crc;
}

/********************* pak *********************/

static int Hypatia_pak_match(struct package *pkg)
{
	pak_header_t *pak_header;

	pak_header = (pak_header_t *)malloc(sizeof(pak_header_t));
	if (!pak_header)
		return -CUI_EMEM;

	if (pkg->pio->open(pkg, IO_READONLY)) {
		free(pak_header);
		return -CUI_EOPEN;
	}

	if (pkg->pio->read(pkg, pak_header, sizeof(pak_header_t))) {
		pkg->pio->close(pkg);
		free(pak_header);
		return -CUI_EREAD;
	}

	if (strncmp((char *)pak_header->magic, "HyPack", 6)) {
		pkg->pio->close(pkg);
		free(pak_header);
		return -CUI_EMATCH;
	}

	if (pak_header->version == 0x0300 || pak_header->version == 0x0301) {
		if (pkg->pio->seek(pkg, 0, IO_SEEK_SET)) {
			pkg->pio->close(pkg);
			free(pak_header);
			return -CUI_ESEEK;
		}

		u32 crc_data_len = pak_header->index_offset + sizeof(pak_header_t)
			+ pak_header->index_entries * sizeof(pak3_entry_t);		
		BYTE *buf = (BYTE *)malloc(0x100000);
		if (!buf) {
			pkg->pio->close(pkg);
			free(pak_header);
			return -CUI_EMEM;			
		}

		u16 crc = 0xffff;
		CrcCheck_MakeCrcTable(0);
		for (u32 i = 0; i < (crc_data_len & ~(0x100000 - 1)); i += 0x100000) {
			if (pkg->pio->read(pkg, buf, 0x100000)) {
				free(buf);
				pkg->pio->close(pkg);
				free(pak_header);
				return -CUI_EREAD;
			}
			crc = CrcCheck_UpDateCrc(crc, buf, 0x100000);
		}

		if (i < crc_data_len) {
			if (pkg->pio->read(pkg, buf, crc_data_len - i)) {
				free(buf);
				pkg->pio->close(pkg);
				free(pak_header);
				return -CUI_EREAD;
			}
			crc = CrcCheck_UpDateCrc(crc, buf, crc_data_len - i);
		}
		free(buf);

		u16 real_crc;
		if (pkg->pio->read(pkg, &real_crc, sizeof(real_crc))) {
			pkg->pio->close(pkg);
			free(pak_header);
			return -CUI_EREADVEC;
		}
		if (real_crc != crc) {
			pkg->pio->close(pkg);
			free(pak_header);
			return -CUI_EMATCH;
		}
	} else if (pak_header->version != 0x0100 && pak_header->version != 0x0200) {
		pkg->pio->close(pkg);
		free(pak_header);
		return -CUI_EMATCH;
	}
	package_set_private(pkg, pak_header);

	return 0;	
}

static int Hypatia_pak_extract_directory(struct package *pkg,
										 struct package_directory *pkg_dir)
{
	pak_header_t *pak_header;
	BYTE *index_buffer;
	DWORD index_buffer_len;
	
	pak_header = (pak_header_t *)package_get_private(pkg);

	if (pkg->pio->seek(pkg, 
		pak_header->index_offset + sizeof(pak_header_t), IO_SEEK_SET))
			return -CUI_ESEEK;

	DWORD entry_size;
	if (pak_header->version == 0x0100)
		entry_size = sizeof(pak1_entry_t);
	else if (pak_header->version == 0x0200)
		entry_size = sizeof(pak2_entry_t);
	else if (pak_header->version == 0x0300 || pak_header->version == 0x0301)
		entry_size = sizeof(pak3_entry_t);

	index_buffer_len = pak_header->index_entries * entry_size;
	index_buffer = (BYTE *)malloc(index_buffer_len);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_len)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	index_buffer_len = pak_header->index_entries * sizeof(pak_entry_t);
	pak_entry_t *index = (pak_entry_t *)malloc(index_buffer_len);
	if (!index) {
		free(index_buffer);
		return -CUI_EMEM;
	}

	if (pak_header->version == 0x0100) {
		pak1_entry_t *pak_index = (pak1_entry_t *)index_buffer;
		for (DWORD i = 0; i < pak_header->index_entries; ++i) {
			memcmp(index[i].name, pak_index[i].name, sizeof(pak_index[i].name));
			memcmp(index[i].suffix, pak_index[i].suffix, sizeof(pak_index[i].suffix));
			index[i].offset = pak_index[i].offset;
			index[i].uncomprLen = 0;
			index[i].comprLen = pak_index[i].length;
			index[i].mode = 0;
			index[i].do_crc_check = 0;
			index[i].crc = 0;
			index[i].time_stamp.dwLowDateTime = 0xA8EE9800;
			index[i].time_stamp.dwHighDateTime = 0x1B41DDE;
		}
	} else if (pak_header->version == 0x0200) {
		pak2_entry_t *pak_index = (pak2_entry_t *)index_buffer;
		for (DWORD i = 0; i < pak_header->index_entries; ++i) {
			memcmp(index[i].name, pak_index[i].name, sizeof(pak_index[i].name));
			memcmp(index[i].suffix, pak_index[i].suffix, sizeof(pak_index[i].suffix));
			index[i].offset = pak_index[i].offset;
			index[i].uncomprLen = pak_index[i].uncomprLen;
			index[i].comprLen = pak_index[i].comprLen;
			index[i].mode = pak_index[i].mode;
			index[i].do_crc_check = 0;
			index[i].crc = 0;
			index[i].time_stamp.dwLowDateTime = 0xA8EE9800;
			index[i].time_stamp.dwHighDateTime = 0x1B41DDE;
		}
	} else if (pak_header->version == 0x0300 || pak_header->version == 0x0301)
		memcpy(index, index_buffer, index_buffer_len);
	free(index_buffer);

	pkg_dir->index_entries = pak_header->index_entries;
	pkg_dir->index_entry_length = sizeof(pak_entry_t);
	pkg_dir->directory = index;
	pkg_dir->directory_length = index_buffer_len;
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

static int Hypatia_pak_parse_resource_info(struct package *pkg,
										   struct package_resource *pkg_res)
{
	pak_entry_t *pak_entry;
	
	pak_entry = (pak_entry_t *)pkg_res->actual_index_entry;
	strncpy((char *)pkg_res->name, pak_entry->name, sizeof(pak_entry->name));
	strcat((char *)pkg_res->name, ".");
	strncat((char *)pkg_res->name, pak_entry->suffix, 3);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = pak_entry->comprLen;
	//pkg_res->actual_data_length = !pak_entry->mode ? 0 : (!pak_entry->uncomprLen ? pak_entry->comprLen : pak_entry->uncomprLen);
	pkg_res->actual_data_length = pak_entry->uncomprLen;
	pkg_res->offset = pak_entry->offset + sizeof(pak_header_t);

	return 0;
}

static int Hypatia_pak_extract_resource(struct package *pkg,
										struct package_resource *pkg_res)
{
	pak_entry_t *pak_entry = (pak_entry_t *)pkg_res->actual_index_entry;

	BYTE *raw = (BYTE *)malloc(pak_entry->comprLen);
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pak_entry->comprLen,
			pkg_res->offset, IO_SEEK_SET)) {
		free(raw);
		return -CUI_EREADVEC;
	}

	if (pak_entry->do_crc_check) {
		if (pak_entry->crc != 
				CrcCheck_UpDateCrc(0xffff, raw, pak_entry->comprLen)) {
			free(raw);
			return -CUI_EMATCH;			
		}
	}

	BYTE *act_data;
	switch (pak_entry->mode) {
	case 1:
		act_data = (BYTE *)malloc(pak_entry->uncomprLen);
		if (!act_data) {
			free(raw);
			return -CUI_EMEM;
		}
		MarielDecode(act_data, pak_entry->uncomprLen, 
			raw, pak_entry->comprLen);
		break;
	case 2:
		act_data = (BYTE *)malloc(pak_entry->uncomprLen);
		if (!act_data) {
			free(raw);
			return -CUI_EMEM;
		}
		CocotteDecode(act_data, pak_entry->uncomprLen, raw, pak_entry->comprLen);
		break;
	case 3:
		NotCoding(raw, pak_entry->comprLen);
		act_data = NULL;
		break;
	default:
		act_data = NULL;
		break;
	}

	pkg_res->raw_data = raw;
	pkg_res->actual_data = act_data;

	return 0;
}

static int Hypatia_pak_save_resource(struct resource *res, 
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

static void Hypatia_pak_release_resource(struct package *pkg, 
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

static void Hypatia_pak_release(struct package *pkg, 
								struct package_directory *pkg_dir)
{
	pak_header_t *pak_header;

	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pak_header = (pak_header_t *)package_get_private(pkg);
	free(pak_header);
	package_set_private(pkg, NULL);

	pkg->pio->close(pkg);
}

static cui_ext_operation Hypatia_pak_operation = {
	Hypatia_pak_match,				/* match */
	Hypatia_pak_extract_directory,	/* extract_directory */
	Hypatia_pak_parse_resource_info,/* parse_resource_info */
	Hypatia_pak_extract_resource,	/* extract_resource */
	Hypatia_pak_save_resource,		/* save_resource */
	Hypatia_pak_release_resource,	/* release_resource */
	Hypatia_pak_release				/* release */
};

/********************* dat *********************/

static int Hypatia_dat_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp((char *)magic, "OggS", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int Hypatia_dat_extract_resource(struct package *pkg,
										struct package_resource *pkg_res)
{
	u32 dat_len;

	if (pkg->pio->length_of(pkg, &dat_len))
		return -CUI_ELEN;

	pkg_res->raw_data = malloc(dat_len);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, dat_len, 0, IO_SEEK_SET)) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
		return -CUI_EREADVEC;
	}
	pkg_res->raw_data_length = dat_len;

	return 0;
}

static int Hypatia_dat_save_resource(struct resource *res, 
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

static void Hypatia_dat_release_resource(struct package *pkg, 
										 struct package_resource *pkg_res)
{
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void Hypatia_dat_release(struct package *pkg, 
								struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

static cui_ext_operation Hypatia_dat_operation = {
	Hypatia_dat_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	Hypatia_dat_extract_resource,	/* extract_resource */
	Hypatia_dat_save_resource,		/* save_resource */
	Hypatia_dat_release_resource,	/* release_resource */
	Hypatia_dat_release				/* release */
};

/********************* dat *********************/

static int Hypatia_wav_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp((char *)magic, "RIFF", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static cui_ext_operation Hypatia_wav_operation = {
	Hypatia_wav_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	Hypatia_dat_extract_resource,	/* extract_resource */
	Hypatia_dat_save_resource,		/* save_resource */
	Hypatia_dat_release_resource,	/* release_resource */
	Hypatia_dat_release				/* release */
};

/********************* ogg *********************/

static int Hypatia_ogg_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp((char *)magic, "OggS", 3)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static cui_ext_operation Hypatia_ogg_operation = {
	Hypatia_ogg_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	Hypatia_dat_extract_resource,	/* extract_resource */
	Hypatia_dat_save_resource,		/* save_resource */
	Hypatia_dat_release_resource,	/* release_resource */
	Hypatia_dat_release				/* release */
};

int CALLBACK Hypatia_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".pak"), NULL, 
		NULL, &Hypatia_pak_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &Hypatia_pak_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), _T(".ogg"), 
		NULL, &Hypatia_dat_operation, CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".PAK"), _T(".wav"), 
		NULL, &Hypatia_wav_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".snd"), _T(".ogg"), 
		NULL, &Hypatia_ogg_operation, CUI_EXT_FLAG_PKG))
			return -1;

	return 0;
}
