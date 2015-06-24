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
struct acui_information YELLOWPIG_cui_information = {
	_T("Kyoya Yuro"),		/* copyright */
	_T("ラムダ"),			/* system */
	_T(".dat"),		/* package */
	_T("1.0.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-3-24 8:52"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	u32 index_entries;
} dat_header_t;

typedef struct {
	u8 have_alpha;
	u16 orig_x;		// 图的原点在640 * 480分辨率屏幕中位置
	u16 orig_y;
	u16 width;
	u16 height;
	u32 unknown;
	u32 comprlen;
} gec_header_t;

typedef struct {
	u16 orig_x;
	u16 orig_y;
	u16 width;
	u16 height;
	u32 unknown;
} alpha_info_t;
#pragma pack ()

/* .dat封包的索引项结构 */
typedef struct {
	s8 name[256];
	u32 offset;
	u32 length;
} dat_entry_t;


static void *my_malloc(DWORD len)
{
	return new BYTE[len];
}

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
	while (curbyte != comprlen) {
		flag >>= 1;
		if (!(flag & 0x0100))
			flag = compr[curbyte++] | 0xff00;

		if (flag & 1) {
			unsigned char data;

			data = compr[curbyte++];
			uncompr[act_uncomprlen++] = data;
			/* 输出的1字节放入滑动窗口 */
			win[nCurWindowByte++] = data;
			nCurWindowByte &= win_size - 1;
		} else {
			unsigned int copy_bytes, win_offset;
			unsigned int i;

			win_offset = compr[curbyte++];
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

class lze {
public:
	void lze_rgba_uncompress(BYTE *compr, BYTE **p_dib_uncompr, BYTE **p_alpha_uncompr, 
		BYTE *dib_info, BYTE *alpha_info);

private:
	BYTE *compr;
	DWORD curbyte;
	BYTE mtf_table[256];
	BYTE uncompr[64 * 1024 + 1];
	BYTE L[64 * 1024 + 1];	// 前2字节是L_len
	WORD F[64 * 1024];

	DWORD get_bit()
	{
		DWORD ret = compr[4 * (curbyte >> 5)] & (1 << (curbyte & 0x1F));
		++curbyte;
		return ret;
	}

	DWORD lze_get_data();
	void lze_4292E0(BYTE **p_compr, DWORD uncomprlen, BYTE **p_uncompr);
	void lze_block_uncompess(DWORD comprlen, BYTE *uncompr, DWORD uncomprlen);
	void lze_mtf_decode(BYTE *src, BYTE *dst, DWORD len);
	void lze_bwt_decode(BYTE *L, DWORD L_len, DWORD pi, BYTE **p_uncompr);
	void lze_rgb_uncompress(BYTE *compr, BYTE **p_dib_uncompr, BYTE *dib_info);
	void lze_alpha_uncompress(BYTE *compr, BYTE **p_alpha_uncompr, BYTE *dib_info, BYTE *alpha_info);
};

void lze::lze_rgba_uncompress(BYTE *compr, BYTE **p_dib_uncompr, 
								   BYTE **p_alpha_uncompr, BYTE *dib_info,
								   BYTE *alpha_info)
{
	lze_rgb_uncompress(compr, p_dib_uncompr, dib_info);
	lze_alpha_uncompress(compr, p_alpha_uncompr, dib_info, alpha_info);
}

DWORD lze::lze_get_data()
{
	DWORD len = 0;
	DWORD data = 1;

	while (!get_bit())
		++len;

	while (len--)
		data = (data << 1) | get_bit();

	return data;
}

void lze::lze_block_uncompess(DWORD comprlen, BYTE *uncompr, DWORD uncomprlen)
{
	for (DWORD i = 0; i < uncomprlen; ) {
		if (get_bit())
			uncompr[i++] = (BYTE)lze_get_data();
		else {
			DWORD len = lze_get_data();
			memset(&uncompr[i], 0, len);
			i += len;
        }
	}
}

void lze::lze_mtf_decode(BYTE *src, BYTE *dst, DWORD len)
{
	// 利用pre_pos将连续的字符放在位置0,阻止抖动问题
	DWORD pre_pos = 1;
	for (DWORD i = 0; i < len; ++i) {
		DWORD pos = *src++;
		BYTE code = mtf_table[pos];
		if (pos == 1) {
			if (pre_pos) {
				mtf_table[1] = mtf_table[0];
				mtf_table[0] = code;
			}
		} else {
			if (pos > 1) {
				for (DWORD k = 0; k < pos - 1; ++k)
					mtf_table[pos - k] = mtf_table[pos - k - 1];
				mtf_table[1] = code;
			}
		}
		*dst++ = code;
		pre_pos = pos;
	}
}

void lze::lze_bwt_decode(BYTE *L, DWORD L_len, DWORD pi, BYTE **p_uncompr)
{
	u16 count[256];
	memset(count, 0, sizeof(count));
	for (DWORD i = 0; i < L_len; ++i)
		++count[L[i]];

	u16 range[256] = { 0 };
	for (i = 0; i < 255; ++i) {
		range[i + 1] = range[i] + count[i];
		count[i] = 0;
	}

	for (i = 0; i < L_len; ++i) {
		BYTE code = L[i];
		F[range[code] + count[code]++] = (WORD)i;
	}

	WORD index = F[pi];
	BYTE *uncompr = *p_uncompr;
	for (i = 0; i < L_len; ++i) {
		uncompr[i] = L[index];
		index = F[index];
	}
	*p_uncompr += L_len;
}

void lze::lze_4292E0(BYTE **p_compr, DWORD uncomprlen, BYTE **p_uncompr)
{
	for (DWORD act_uncomprlen = 0; act_uncomprlen < uncomprlen; ) {
		if (!get_bit()) {
			*(*p_uncompr)++ = *(*p_compr)++;
			*(*p_uncompr)++ = *(*p_compr)++;
			*(*p_uncompr)++ = *(*p_compr)++;
			act_uncomprlen += 3;
		} else {
			DWORD pixel = lze_get_data();
			BYTE b = *(*p_compr)++;
			BYTE g = *(*p_compr)++;
			BYTE r = *(*p_compr)++;
			for (DWORD i = 0; i < pixel; ++i) {
				*(*p_uncompr)++ = b;
				*(*p_uncompr)++ = g;
				*(*p_uncompr)++ = r;				
			}
			act_uncomprlen += pixel * 3;
		}
	}
}

void lze::lze_rgb_uncompress(BYTE *compr, BYTE **p_uncompr, 
							 BYTE *dib_info)
{
	gec_header_t *gec = (gec_header_t *)dib_info;
	DWORD comprlen = gec->comprlen;
	DWORD uncomprlen = gec->width * gec->height * 3;
	DWORD act_uncomprlen = 0;

	this->compr = compr;
	curbyte = 0;
	for (DWORD i = 0; i < 256; ++i)
		mtf_table[i] = (BYTE)i;

	BYTE *compr2 = &compr[gec->comprlen];
	while (act_uncomprlen != uncomprlen) {
		DWORD cur_uncomprlen = uncomprlen - act_uncomprlen;
		if (cur_uncomprlen > 0xffff)
			cur_uncomprlen = 0xffff;
		if (get_bit()) {
			lze_block_uncompess(comprlen, uncompr, cur_uncomprlen + 2);
			lze_mtf_decode(mtf_table, L, cur_uncomprlen + 2);
			lze_bwt_decode(L + 2, cur_uncomprlen, *(WORD *)L, p_uncompr);
		} else
			lze_4292E0(&compr2, cur_uncomprlen, p_uncompr);
		act_uncomprlen += cur_uncomprlen;
	}
}

void lze::lze_alpha_uncompress(BYTE *compr, BYTE **p_alpha_uncompr, 
							   BYTE *dib_info, BYTE *alpha_info)
{

}

/********************* dat *********************/

/* 封包匹配回调函数 */
static int YELLOWPIG_dat_match(struct package *pkg)
{
	if (!wcsncmp(pkg->name, L"SAVE", 4))
		return -CUI_EMATCH;

	if (!lstrcmpi(pkg->name, _T("skip.dat")))
		return -CUI_EMATCH;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包索引目录提取函数 */
static int YELLOWPIG_dat_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{
	dat_header_t dat_header;
	if (pkg->pio->read(pkg, &dat_header, sizeof(dat_header)))
		return -CUI_EREAD;

	dat_entry_t *index_buffer = new dat_entry_t[dat_header.index_entries];
	if (!index_buffer)
		return -CUI_EMEM;

	for (DWORD i = 0; i < dat_header.index_entries; ++i) {
		dat_entry_t &entry = index_buffer[i];
		u8 name_len;

		if (pkg->pio->read(pkg, &name_len, 1))
			break;
		if (pkg->pio->read(pkg, entry.name, name_len))
			break;		
		for (u8 n = 0; n < name_len; ++n)
			entry.name[n] -= name_len-- + 1;
		entry.name[n] = 0;

		if (pkg->pio->read(pkg, &entry.offset, 4))
			break;
		entry.offset = SWAP32(entry.offset);

		if (pkg->pio->read(pkg, &entry.length, 4))
			break;
		entry.length = SWAP32(entry.length);
	}
	if (i != dat_header.index_entries)
		return -CUI_EREAD;

	pkg_dir->index_entries = dat_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = dat_header.index_entries * sizeof(dat_entry_t);
	pkg_dir->index_entry_length = sizeof(dat_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int YELLOWPIG_dat_parse_resource_info(struct package *pkg,
											 struct package_resource *pkg_res)
{
	dat_entry_t *dat_entry;

	dat_entry = (dat_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, dat_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = dat_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = dat_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int YELLOWPIG_dat_extract_resource(struct package *pkg,
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

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = compr;
		return 0;
	}

	if (!strncmp((char *)compr, "RIFF", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".wav");
	} else {
		DWORD comprlen = pkg_res->raw_data_length;
		DWORD uncomprlen = 1024 * 1024 * 10;	// 10M
		BYTE *uncompr = new BYTE[uncomprlen];
		if (!uncompr) {
			delete [] compr;
			return -CUI_EMEM;
		}
		uncomprlen = lzss_uncompress(uncompr, uncomprlen, compr, comprlen);
		if (uncomprlen == 800 * 600) {
			if (MyBuildBMPFile(uncompr, uncomprlen, NULL, 0,
					800, 600, 8, (BYTE **)&pkg_res->actual_data, 
					&pkg_res->actual_data_length, my_malloc)) {
				delete [] uncompr;
				delete [] compr;
				return -CUI_EMEM;
			}
			delete [] uncompr;
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
			pkg_res->replace_extension = _T(".bmp");
		} else {
			pkg_res->actual_data = uncompr;
			pkg_res->actual_data_length = uncomprlen;
		}
	}

	pkg_res->raw_data = compr;

	return 0;
}

/* 资源保存函数 */
static int YELLOWPIG_dat_save_resource(struct resource *res, 
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
static void YELLOWPIG_dat_release_resource(struct package *pkg, 
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
static void YELLOWPIG_dat_release(struct package *pkg, 
								  struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation YELLOWPIG_dat_operation = {
	YELLOWPIG_dat_match,				/* match */
	YELLOWPIG_dat_extract_directory,	/* extract_directory */
	YELLOWPIG_dat_parse_resource_info,	/* parse_resource_info */
	YELLOWPIG_dat_extract_resource,		/* extract_resource */
	YELLOWPIG_dat_save_resource,		/* save_resource */
	YELLOWPIG_dat_release_resource,		/* release_resource */
	YELLOWPIG_dat_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK YELLOWPIG_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &YELLOWPIG_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	return 0;
}
