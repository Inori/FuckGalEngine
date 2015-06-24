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
#include <vector>

using namespace std;
using std::vector;

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information AVGEngine_cui_information = {
	_T(""),			/* copyright */
	_T(""),					/* system */
	_T(".pck"),				/* package */
	_T("0.8.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-4-6 18:49"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	u32 seed;			// 0x00
	u32 crc;			// 0x04
	u32 dir0_size;		// 0x08
	u32 dir1_size;		// 0x0c
	u32 dir2_size;		// 0x10
	u32 dir3_size;		// 0x14
	u32 dir4_size;		// 0x18
	u32 verify;			// 0x1c	
	u32 index_offset;	// 0x20
	u32 dir6_size;		// 0x24
	u32 dir5_size;		// 0x28
	u16 unknown0;
	u16 unknown1;
	u16 unknown2;
	u16 unknown3;
	u32 dir7_size;		// 0x34
	u32 unknown4;		// 0x38
	u8 pad1[5];
	u8 delta_length;	// 0x41
	u8 pad2[2];
	u32 dir8_size;		// 0x44	
	s8 string[32];
} pck_tailer_t;

typedef struct {
	u32 unknown;
	u16 dir_number;
	u16 unknown1;
	u32 range_start_number;
	u32 entries;
} pck_index_header_t;

typedef struct {
	s8 name[40];
	u32 offset;
	u32 comprlen;
	u32 is_compressed;
	u32 uncomprlen;
} pck_index_entry_t;
#pragma pack ()

#define SWAP16(v)	((((v) & 0xff) << 8) | (((v) >> 8) & 0xff))
#define SWAP32(v)	((((v) & 0xff) << 24) | (((v) & 0xff00) << 8) | (((v) & 0xff0000) >> 8) | (((v) & 0xff000000) >> 24))

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
			if (curbyte >= comprlen)
				break;
			flag = compr[curbyte++] | 0xff00;
		}

		if (flag & 1) {
			unsigned char data;

			if (curbyte >= comprlen)
				break;

			data = compr[curbyte++];
			uncompr[act_uncomprlen++] = data;
			/* 输出的1字节放入滑动窗口 */
			win[nCurWindowByte++] = data;
			nCurWindowByte &= win_size - 1;
		} else {
			unsigned int copy_bytes, win_offset;
			unsigned int i;

			if (curbyte >= comprlen)
				break;
			win_offset = compr[curbyte++];

			if (curbyte >= comprlen)
				break;
			copy_bytes = compr[curbyte++];
			win_offset |= (copy_bytes & 0xf0) << 4;
			copy_bytes &= 0x0f;
			copy_bytes += 3;

			for (i = 0; i < copy_bytes; i++) {	
				unsigned char data;

				data = win[(win_offset + i) & (win_size - 1)];
				uncompr[act_uncomprlen++] = data;
				/* 输出的1字节放入滑动窗口 */
				win[nCurWindowByte++] = data;
				nCurWindowByte &= win_size - 1;	
			}
		}
	}

	return act_uncomprlen;
}

static long ia[56];
static long jrand = 0;

static void IRN55(void)
{
	long i, j;
	for (i = 1; i <= 24; i++) {
		j = ia[i] - ia[i+31];
		if (j < 0) 
			j += 1000000000;
		ia[i] = j;
	}

	for (i = 25; i <= 55; i++) {
		j = ia[i] - ia[i-24];
		if (j < 0) 
			j += 1000000000;
		ia[i] = j;
	}
}

static void IN55(long seed)
{
	ia[55] = seed;
	long k = 1;

	for (long ii, i = 1; i <= 54; i++) {
		ii = 21 * i % 55;
		ia[ii] = k;
		k = seed - k;
		if (k < 0)
			k += 1000000000;
		seed = ia[ii];
	}

	IRN55();
	IRN55();
	IRN55();
	jrand = 55;
}

static DWORD Rand(DWORD seed)
{
	if (!jrand)
		IN55(seed);

	++jrand;

	if (jrand > 55) {
		IRN55();
		jrand = 1;
	}

	return ia[jrand];
}

static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static vector<pck_index_entry_t> index_entry;

/* 封包匹配回调函数 */
static int AVGEngine_pck_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	u32 offset[4];
	u32 tmp;

	if (pkg->pio->seek(pkg, 4, IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_ESEEK;
	}

	if (pkg->pio->read(pkg, &tmp, sizeof(tmp))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	offset[0] = SWAP32(tmp & ~0x80) + 4;

	if (pkg->pio->seek(pkg, SWAP32(tmp & ~0x80) + 16, IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_ESEEK;
	}

	if (pkg->pio->read(pkg, &tmp, sizeof(tmp))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	offset[1] = offset[0] + SWAP32(tmp & ~0x80) + 4;

	if (pkg->pio->seek(pkg, SWAP32(tmp & ~0x80) + offset[0] + 12, IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_ESEEK;
	}
  
	if (pkg->pio->read(pkg, &tmp, sizeof(tmp))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	offset[2] = offset[1] + SWAP32(tmp & ~0x80) + 4;

	if (pkg->pio->seek(pkg, SWAP32(tmp & ~0x80) + offset[1] + 4, IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_ESEEK;
	}

	if (pkg->pio->read(pkg, &tmp, sizeof(tmp))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	offset[3] = SWAP32(tmp & ~0x80);

	u32 enc[26];
	if (pkg->pio->readvec(pkg, enc, sizeof(enc), offset[3], IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_EREADVEC;
	}

	jrand = 0;
	enc[0] = SWAP32(enc[0]);

	for (DWORD j = 1; j < 26; ++j) {
		u32 tmp = Rand(enc[0]);
		enc[j] ^= SWAP32(tmp);
	}

	enc[2] = SWAP32(enc[2]);
	enc[3] = SWAP32(enc[3]);
	enc[4] = SWAP32(enc[4]);
	enc[5] = SWAP32(enc[5]);
	enc[6] = SWAP32(enc[6]);
	enc[7] = SWAP32(enc[7]);
	enc[8] = SWAP32(enc[8]);
	enc[9] = SWAP32(enc[9]);
	enc[10] = SWAP32(enc[10]);
	u16 *p = (u16 *)&enc[11];
	*p = SWAP16(*p);
	++p;
	*p = SWAP16(*p);
	++p;
	*p = SWAP16(*p);
	++p;
	*p = SWAP16(*p);
	enc[13] = SWAP32(enc[13]);
	enc[14] = SWAP32(enc[14]);
	p = (u16 *)enc + 0x21;
	*p = SWAP16(*p);
	enc[17] = SWAP32(enc[17]);
	DWORD crc = enc[1];
	u8 *c = (u8 *)&crc;
	crc = c[1] | c[2] << 8 | c[0] << 16 | c[3] << 24;
    if ((crc ^ 0xDEFD32D3) != enc[7]) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	pck_tailer_t *t = new pck_tailer_t;
	if (!t) {
		pkg->pio->close(pkg);
		return -CUI_EMEM;
	}
	memcpy(t, enc, sizeof(enc));
	package_set_private(pkg, t);

	pck_index_entry_t entry;

	if (pkg->pio->readvec(pkg, &entry.uncomprlen, sizeof(entry.uncomprlen), 
			0, IO_SEEK_SET)) {
		delete [] t;
		pkg->pio->close(pkg);
		return -CUI_EREADVEC;
	}

	if (pkg->pio->read(pkg, &entry.comprlen, sizeof(entry.comprlen))) {
		delete [] t;
		pkg->pio->close(pkg);
		return -CUI_EREADVEC;
	}

	strcpy(entry.name, "top\\Splash.bmp");
	entry.comprlen = SWAP32(entry.comprlen);
	entry.uncomprlen = SWAP32(entry.uncomprlen) - 4;
	entry.is_compressed = !!(entry.comprlen & 0x80000000);
	entry.comprlen &= ~0x80000000;
	entry.comprlen -= 4;
	entry.offset = 8;
	index_entry.push_back(entry);

	if (pkg->pio->readvec(pkg, &entry.comprlen, sizeof(entry.comprlen), 
			offset[0] + 12, IO_SEEK_SET)) {
		delete [] t;
		pkg->pio->close(pkg);
		return -CUI_EREADVEC;
	}

	strcpy(entry.name, "000001");
	entry.comprlen = SWAP32(entry.comprlen);
	entry.uncomprlen = 4096;
	entry.is_compressed = !!(entry.comprlen & 0x80000000);
	entry.comprlen &= ~0x80000000;
	entry.offset = offset[0];
	index_entry.push_back(entry);

	if (pkg->pio->readvec(pkg, &entry.uncomprlen, sizeof(entry.uncomprlen), 
			offset[1], IO_SEEK_SET)) {
		delete [] t;
		pkg->pio->close(pkg);
		return -CUI_EREADVEC;
	}

	if (pkg->pio->readvec(pkg, &entry.comprlen, sizeof(entry.comprlen), 
			offset[1] + 8, IO_SEEK_SET)) {
		delete [] t;
		pkg->pio->close(pkg);
		return -CUI_EREADVEC;
	}

	strcpy(entry.name, "000002");
	entry.comprlen = SWAP32(entry.comprlen);
	entry.uncomprlen = SWAP32(entry.uncomprlen);
	entry.is_compressed = !!(entry.comprlen & 0x80000000);
	entry.comprlen &= ~0x80000000;
	entry.comprlen -= 4;
	entry.offset = offset[1] + 4;
	index_entry.push_back(entry);

	return 0;	
}

/* 封包索引目录提取函数 */
static int AVGEngine_pck_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{
	pck_tailer_t *t = (pck_tailer_t *)package_get_private(pkg);

	u32 uncomprlen;
	if (pkg->pio->readvec(pkg, &uncomprlen, sizeof(uncomprlen), t->index_offset, IO_SEEK_SET))
		return -CUI_EREADVEC;

	uncomprlen = SWAP32(uncomprlen);

	u32 comprlen;
	if (pkg->pio->readvec(pkg, &comprlen, sizeof(comprlen), t->index_offset + 36, IO_SEEK_SET))
		return -CUI_EREADVEC;

	comprlen = SWAP32(comprlen);
	int is_compressed = comprlen & 0x80000000;
	comprlen &= ~0x80000000;
	BYTE *compr = new BYTE[comprlen];
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, 32, t->index_offset + 4, IO_SEEK_SET)) {
		delete [] compr;
		return -CUI_EREAD;
	}

	if (pkg->pio->readvec(pkg, compr + 32, comprlen - 32, t->index_offset + 40, IO_SEEK_SET)) {
		delete [] compr;
		return -CUI_EREAD;
	}

	BYTE *index;
	DWORD index_len;
	if (is_compressed) {
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
		index = uncompr;
		index_len = uncomprlen;
	} else {
		index = compr;
		index_len = comprlen;
	}
	
	BYTE *cur_index = index;
	if (t->dir0_size) {
		pck_index_header_t *h = (pck_index_header_t *)cur_index;
		h->unknown1 = SWAP16(h->unknown1);
		h->range_start_number = SWAP32(h->range_start_number);
		h->entries = SWAP32(h->entries);
		pck_index_entry_t *e = (pck_index_entry_t *)(h + 1);
		for (DWORD i = 0; i < h->entries; ++i) {
			s8 name[MAX_PATH];

			sprintf(name, "PartAnims\\%s", e->name);
			e->comprlen = SWAP32(e->comprlen);
			e->offset = SWAP32(e->offset);
			strcpy(e->name, name);
			if (e->is_compressed)
				e->is_compressed |= (8 - (((h->range_start_number & 0xff) + (BYTE)i) & 7)) << 24;
			index_entry.push_back(*e);
			++e;
		}
		cur_index += t->dir0_size;
	}

	if (t->dir1_size) {
		pck_index_header_t *h = (pck_index_header_t *)cur_index;
		h->unknown1 = SWAP16(h->unknown1);
		h->range_start_number = SWAP32(h->range_start_number);
		h->entries = SWAP32(h->entries);
		pck_index_entry_t *e = (pck_index_entry_t *)(h + 1);
		for (DWORD i = 0; i < h->entries; ++i) {
			s8 name[MAX_PATH];

			sprintf(name, "scripts\\%s", e->name);
			e->comprlen = SWAP32(e->comprlen);
			e->offset = SWAP32(e->offset);
			strcpy(e->name, name);
			if (e->is_compressed)
				e->is_compressed |= (8 - (((h->range_start_number & 0xff) + (BYTE)i) & 7)) << 24;
			index_entry.push_back(*e);
			++e;
		}
		cur_index += t->dir1_size;
	}

	if (t->dir2_size) {
		pck_index_header_t *h = (pck_index_header_t *)cur_index;
		h->unknown1 = SWAP16(h->unknown1);
		h->range_start_number = SWAP32(h->range_start_number);
		h->entries = SWAP32(h->entries);
		pck_index_entry_t *e = (pck_index_entry_t *)(h + 1);
		for (DWORD i = 0; i < h->entries; ++i) {
			s8 name[MAX_PATH];

			sprintf(name, "graphs\\%s", e->name);
			e->comprlen = SWAP32(e->comprlen);
			e->offset = SWAP32(e->offset);
			strcpy(e->name, name);
			if (e->is_compressed)
				e->is_compressed |= (8 - (((h->range_start_number & 0xff) + (BYTE)i) & 7)) << 24;
			index_entry.push_back(*e);
			++e;
		}
		cur_index += t->dir2_size;
	}

	if (t->dir3_size) {
		pck_index_header_t *h = (pck_index_header_t *)cur_index;
		h->unknown1 = SWAP16(h->unknown1);
		h->range_start_number = SWAP32(h->range_start_number);
		h->entries = SWAP32(h->entries);
		pck_index_entry_t *e = (pck_index_entry_t *)(h + 1);
		for (DWORD i = 0; i < h->entries; ++i) {
			s8 name[MAX_PATH];

			sprintf(name, "sounds\\%s", e->name);
			e->comprlen = SWAP32(e->comprlen);
			e->offset = SWAP32(e->offset);
			strcpy(e->name, name);
			if (e->is_compressed)
				e->is_compressed |= (8 - (((h->range_start_number & 0xff) + (BYTE)i) & 7)) << 24;
			index_entry.push_back(*e);
			++e;
		}
		cur_index += t->dir3_size;
	}

	if (t->dir4_size) {
		pck_index_header_t *h = (pck_index_header_t *)cur_index;
		h->unknown1 = SWAP16(h->unknown1);
		h->range_start_number = SWAP32(h->range_start_number);
		h->entries = SWAP32(h->entries);
		pck_index_entry_t *e = (pck_index_entry_t *)(h + 1);
		for (DWORD i = 0; i < h->entries; ++i) {
			e->comprlen = SWAP32(e->comprlen);
			e->offset = SWAP32(e->offset);
			if (e->is_compressed)
				e->is_compressed |= (8 - (((h->range_start_number & 0xff) + (BYTE)i) & 7)) << 24;
			index_entry.push_back(*e);
			++e;
		}
		cur_index += t->dir4_size;
	}

	if (t->dir5_size) {
		pck_index_header_t *h = (pck_index_header_t *)cur_index;
		h->unknown1 = SWAP16(h->unknown1);
		h->range_start_number = SWAP32(h->range_start_number);
		h->entries = SWAP32(h->entries);
		pck_index_entry_t *e = (pck_index_entry_t *)(h + 1);
		for (DWORD i = 0; i < h->entries; ++i) {
			e->comprlen = SWAP32(e->comprlen);
			e->offset = SWAP32(e->offset);
			if (e->is_compressed)
				e->is_compressed |= (8 - (((h->range_start_number & 0xff) + (BYTE)i) & 7)) << 24;
			index_entry.push_back(*e);
			++e;
		}
		cur_index += t->dir5_size;
	}

	if (t->dir6_size) {
		pck_index_header_t *h = (pck_index_header_t *)cur_index;
		h->unknown1 = SWAP16(h->unknown1);
		h->range_start_number = SWAP32(h->range_start_number);
		h->entries = SWAP32(h->entries);
		pck_index_entry_t *e = (pck_index_entry_t *)(h + 1);
		for (DWORD i = 0; i < h->entries; ++i) {
			e->comprlen = SWAP32(e->comprlen);
			e->offset = SWAP32(e->offset);
			if (e->is_compressed)
				e->is_compressed |= (8 - (((h->range_start_number & 0xff) + (BYTE)i) & 7)) << 24;
			index_entry.push_back(*e);
			++e;
		}
		cur_index += t->dir6_size;
	}

	if (t->dir7_size) {
		pck_index_header_t *h = (pck_index_header_t *)cur_index;
		h->unknown1 = SWAP16(h->unknown1);
		h->range_start_number = SWAP32(h->range_start_number);
		h->entries = SWAP32(h->entries);
		pck_index_entry_t *e = (pck_index_entry_t *)(h + 1);
		for (DWORD i = 0; i < h->entries; ++i) {
			s8 name[MAX_PATH];

			sprintf(name, "Effects\\%s", e->name);
			e->comprlen = SWAP32(e->comprlen);
			e->offset = SWAP32(e->offset);
			strcpy(e->name, name);
			if (e->is_compressed)
				e->is_compressed |= (8 - (((h->range_start_number & 0xff) + (BYTE)i) & 7)) << 24;
			index_entry.push_back(*e);
			++e;
		}
		cur_index += t->dir7_size;
	}

	if (t->dir8_size) {
		pck_index_header_t *h = (pck_index_header_t *)cur_index;
		h->unknown1 = SWAP16(h->unknown1);
		h->range_start_number = SWAP32(h->range_start_number);
		h->entries = SWAP32(h->entries);
		pck_index_entry_t *e = (pck_index_entry_t *)(h + 1);
		for (DWORD i = 0; i < h->entries; ++i) {
			e->comprlen = SWAP32(e->comprlen);
			e->offset = SWAP32(e->offset);
			if (e->is_compressed)
				e->is_compressed |= (8 - (((h->range_start_number & 0xff) + (BYTE)i) & 7)) << 24;
			index_entry.push_back(*e);
			++e;
		}
		cur_index += t->dir8_size;
	}

	delete [] index;

	pck_index_entry_t *index_buffer = new pck_index_entry_t[index_entry.size()];
	if (!index_buffer)
		return -CUI_EMEM;

	for (DWORD j = 0; j < index_entry.size(); ++j)
		index_buffer[j] = index_entry[j];

	pkg_dir->index_entries = index_entry.size();
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = pkg_dir->index_entries * sizeof(pck_index_entry_t);
	pkg_dir->index_entry_length = sizeof(pck_index_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int AVGEngine_pck_parse_resource_info(struct package *pkg,
											 struct package_resource *pkg_res)
{
	pck_index_entry_t *entry;

	entry = (pck_index_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = entry->comprlen;
	pkg_res->actual_data_length = entry->uncomprlen;	/* 数据都是明文 */
	pkg_res->offset = entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int AVGEngine_pck_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	u32 pos;

	if (pkg_res->index_number == 1)
		pos = 12;
	else if (pkg_res->index_number == 2)
		pos = 4;
	else
		pos = 0;

	DWORD comprlen = pkg_res->raw_data_length;
	BYTE *compr = new BYTE[comprlen];
	if (!compr)
		return -CUI_EMEM;

	if (pos) {
		if (pkg->pio->readvec(pkg, compr, pos, pkg_res->offset, IO_SEEK_SET)) {
			delete [] compr;
			return -CUI_EREADVEC;
		}
	}

	if (pkg->pio->readvec(pkg, compr + pos, comprlen - pos, 
		pkg_res->offset + (pos ? pos + 4 : 0), IO_SEEK_SET)) {
		delete [] compr;
		return -CUI_EREADVEC;
	}

	pck_index_entry_t *entry = (pck_index_entry_t *)pkg_res->actual_index_entry;
	pos = entry->is_compressed >> 24;
	entry->is_compressed &= ~0xff000000;

	if (entry->is_compressed) {
		BYTE *src;

		if (pkg_res->index_number > 2) {
			u32 tmp = *(u32 *)&compr[4 * pos];	// should be comprlen-4
			memmove(compr + 4, compr, 4 * pos);
			comprlen -= 4;
			src = compr + 4;
			if (comprlen >= 16) {
				*(u32 *)&src[0] ^= 0xC53A9A6C;
				*(u32 *)&src[4] ^= 0xC53A9A6C;
				*(u32 *)&src[8] ^= 0xC53A9A6C;
				*(u32 *)&src[12] ^= 0xC53A9A6C;
			} else {
				BYTE code[4] = { 0xc5, 0x3a, 0x9a, 0x6c }; 
				for (DWORD i = 0; i < comprlen; ++i)
					src[i] ^= code[i & 3];
			}
		} else
			src = compr;

		DWORD uncomprlen = pkg_res->actual_data_length;
		BYTE *uncompr = new BYTE[uncomprlen];
		if (!uncompr) {
			delete [] compr;
			return -CUI_EMEM;
		}

		BYTE *dst;
		if (pkg_res->index_number > 2 && strstr(pkg_res->name, ".bsd")) {
			memcpy(uncompr, src, 48);
			src += 48;
			comprlen -= 48;
			uncomprlen -= 48;
			dst = uncompr + 48;
		} else
			dst = uncompr;

		DWORD act_uncomprlen = lzss_uncompress(dst, uncomprlen, 
			src, comprlen);
		delete [] compr;
		if (act_uncomprlen != uncomprlen) {
			delete [] uncompr;
			return -CUI_EUNCOMPR;
		}

		if (!pkg_res->index_number) {
			BYTE *p = uncompr;
			DWORD plen = uncomprlen;
		
			uncomprlen -= 4;
			u16 width = SWAP16(*(u16 *)p);
			u16 height = SWAP16(*(u16 *)(p + 2));
			DWORD rgba_len = width * height * 4;
			BYTE *rgba = new BYTE[rgba_len];
			if (!rgba) {
				delete [] uncompr;
				delete [] compr;
				return -CUI_EMEM;
			}

			BYTE *src = p + 4;
			BYTE *a = src + width * height * 3;
			p = rgba;
			for (DWORD i = 0; i < width * height; ++i) {
				BYTE ap = *a++;
				*p++ = *src++;
				*p++ = *src++;
				*p++ = *src++;
				*p++ = ap;
			}
			
			if (MyBuildBMPFile(rgba, rgba_len, NULL, 0, width, -height, 32,
					(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, my_malloc)) {
				delete [] uncompr;
				delete [] compr;
				return -CUI_EMEM;
			}
			delete [] uncompr;
			delete [] compr;
		} else
			pkg_res->actual_data = uncompr;
	} else
		pkg_res->raw_data = compr;

	return 0;
}

/* 资源保存函数 */
static int AVGEngine_pck_save_resource(struct resource *res, 
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
static void AVGEngine_pck_release_resource(struct package *pkg, 
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
static void AVGEngine_pck_release(struct package *pkg, 
								  struct package_directory *pkg_dir)
{
	void *priv = (void *)package_get_private(pkg);
	if (priv) {
		delete [] priv;
		package_set_private(pkg, 0);
	}

	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation AVGEngine_pck_operation = {
	AVGEngine_pck_match,				/* match */
	AVGEngine_pck_extract_directory,	/* extract_directory */
	AVGEngine_pck_parse_resource_info,	/* parse_resource_info */
	AVGEngine_pck_extract_resource,		/* extract_resource */
	AVGEngine_pck_save_resource,		/* save_resource */
	AVGEngine_pck_release_resource,		/* release_resource */
	AVGEngine_pck_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK AVGEngine_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".pck"), NULL, 
		NULL, &AVGEngine_pck_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
