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

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information EAGLS_cui_information = {
	_T("Tech-Arts"),		/* copyright */
	_T("Enhanced Adventure Game Language System)"),/* system */
	_T(".pak .idx"),		/* package */
	_T("1.1.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-7-21 16:59"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 name[20];
	u32 offset;
	u32 length;
} idx_entry_t;
#pragma pack ()


static int old;

static inline unsigned char getbit_le(unsigned char byte, unsigned int pos)
{
	return !!(byte & (1 << pos));
}

static DWORD alis_lzss_decompress(unsigned char *uncompr, DWORD uncomprlen, 
							unsigned char *compr, DWORD comprlen)
{
	unsigned int act_uncomprlen = 0;
	/* compr中的当前字节中的下一个扫描位的位置 */
	unsigned int curbit = 0;
	/* compr中的当前扫描字节 */
	unsigned int curbyte = 0;
	unsigned int nCurWindowByte = 0xfee;
	unsigned int win_size = 4096;
	BYTE win[4096];
	
	memset(win, 0, sizeof(win));
	while (1) {
		/* 如果为0, 表示接下来的1个字节原样输出 */
		BYTE flag;

		if (curbyte >= comprlen)
			break;

		flag = compr[curbyte++];
		for (curbit = 0; curbit < 8; curbit++) {
			if (getbit_le(flag, curbit)) {
				unsigned char data;

				if (curbyte >= comprlen)
					goto out;

				if (act_uncomprlen >= uncomprlen)
					goto out;

				data = compr[curbyte++];
				uncompr[act_uncomprlen++] = data;
				/* 输出的1字节放入滑动窗口 */
				win[nCurWindowByte++] = data;
				nCurWindowByte &= win_size - 1;
			} else {
				unsigned int copy_bytes, win_offset;
				unsigned int i;

				if (curbyte >= comprlen)
					goto out;
				win_offset = compr[curbyte++];

				if (curbyte >= comprlen)
					goto out;
				copy_bytes = compr[curbyte++];
				win_offset |= (copy_bytes >> 4) << 8;
				copy_bytes &= 0x0f;
				copy_bytes += 3;

				for (i = 0; i < copy_bytes; i++) {	
					unsigned char data;

					if (act_uncomprlen >= uncomprlen)
						goto out;

					data = win[(win_offset + i) & (win_size - 1)];
					uncompr[act_uncomprlen++] = data;		
					/* 输出的1字节放入滑动窗口 */
					win[nCurWindowByte++] = data;
					nCurWindowByte &= win_size - 1;	
				}
			}
		}
	}
out:
	return act_uncomprlen;
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
			flag = compr[curbyte++] | 0xff00;
			if (curbyte == comprlen)
				break;
		}

		if (flag & 1) {
			unsigned char data;

			data = compr[curbyte++];
			if (curbyte == comprlen)
				break;
			uncompr[act_uncomprlen++] = data;
			/* 输出的1字节放入滑动窗口 */
			win[nCurWindowByte++] = data;
			nCurWindowByte &= win_size - 1;
		} else {
			unsigned int copy_bytes, win_offset;
			unsigned int i;

			win_offset = compr[curbyte++];
			if (curbyte == comprlen)
				break;
			copy_bytes = compr[curbyte++];
			if (curbyte == comprlen)
				break;
			win_offset |= (copy_bytes & 0xf0) << 4;
			copy_bytes &= 0x0f;
			copy_bytes += 3;

			for (i = 0; i < copy_bytes; ++i) {	
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

#if 1
/*
 *	algorithm by
 *	D. P. Mitchell & J. A. Reeds
 */
static long lrand2(long *seed)
{
	#undef	A
	#undef	Q
	#undef	R
	#undef	M
	#define	A	16807
	#define Q	127773
	#define	R	2836
	#define	M	2147483647
	long x = *seed;
	/*
	 *	Initialize by x[n+1] = 48271 * x[n] mod (2**31 - 1)
	 */
	long hi = x / Q;
	long lo = x % Q;
	x = A * lo - R * hi;
	if (x < 0)
		x += M;
	*seed = x;

	return (long)(x * 4.656612875245797e-10 * 256);
}
#else
/**
 * generates 32 bit pseudo-random numbers.
 * Adapted from http://www.snippets.org
 */
static int randomParkAndMiller(long *seed0)
{
	const long a =      16807;
	const long m = 2147483647;
	const long q =     127773;  /* m div a */
	const long r =       2836;  /* m mod a */

	long seed = *seed0;
	long hi   = seed / q;
	long lo   = seed % q;
	long test = a * lo - r * hi;
	if (test > 0)
		seed = test;
	else
		seed = test + m;
	*seed0 = seed;

	return (long)(seed * 4.656612875245797e-10 * 256);
}
#endif

/*
 *	algorithm by
 *	D. P. Mitchell & J. A. Reeds
 */
static long lrand(long *seed)
{
	#undef	A
	#undef	Q
	#undef	R
	#undef	M
	#define	A	48271
	#define Q	44488
	#define	R	3399
	#define	M	2147483647
	long x = *seed;
	/*
	 *	Initialize by x[n+1] = 48271 * x[n] mod (2**31 - 1)
	 */
	long hi = x / Q;
	long lo = x % Q;
	x = A * lo - R * hi;
	if (x < 0)
		x += M;
	*seed = x;

	return (long)(x * 4.656612875245797e-10 * 256);
}

static long inline update_seed(long *seed)
{
	*seed = *seed * 0x343FD + 0x269EC3;
	return (*seed >> 16) & 0x7fff;
}

static void alis_decode_script(char *encypt_key, BYTE *buf, DWORD len)
{
	DWORD encypt_key_length = strlen(encypt_key);
	
	for (unsigned int i = 0; i < len; i++)
		buf[i] ^= encypt_key[i % encypt_key_length];
}

static void decode_script(char *encypt_key, long seed, BYTE *dec, DWORD len)
{
	DWORD encypt_key_length = strlen(encypt_key);
	BYTE decode_table[128];

	strcpy((char *)decode_table, encypt_key);
	
	for (unsigned int i = 0; i < len; i += 2)
		dec[i] ^= decode_table[update_seed(&seed) % encypt_key_length];
}

// 姉とボイン
static void ANEBOIN_decode_script(char *encypt_key, long seed, BYTE *dec, DWORD len)
{
	BYTE decode_table[5] = { 'E', 'A', 'G', 'L', 'S' };
	for (unsigned int i = 0; i < len; ++i)
		dec[i] ^= decode_table[i % 5];
}

static void decode_cg(char *encypt_key, long seed, BYTE *buf, DWORD len)
{
	seed ^= 0x75BD924;
	DWORD encypt_key_length = strlen(encypt_key);
	BYTE decode_table[128];

	strcpy((char *)decode_table, encypt_key);
		
	for (unsigned int i = 0; i < 0x174b; ++i)
		buf[i] ^= decode_table[lrand(&seed) % encypt_key_length];
}

// お姉さん中出し痴漢列車
static void TIKAN_decode_cg(char *encypt_key, long seed, BYTE *dec, DWORD len)
{
	DWORD encypt_key_length = strlen(encypt_key);
	BYTE decode_table[128];

	strcpy((char *)decode_table, encypt_key);
	
	for (unsigned int i = 0; i < 0x174b; ++i)
		dec[i] ^= decode_table[update_seed(&seed) % encypt_key_length];
}

// 姉とボイン
static void ANEBOIN_decode_cg(char *encypt_key, long seed, BYTE *dec, DWORD len)
{
}

static void hensin2_decode_cg(char *encypt_key, long seed, BYTE *dec, DWORD len)
{
	seed ^= 0x75BD924;
	DWORD encypt_key_length = strlen(encypt_key);
	BYTE decode_table[128];

	strcpy((char *)decode_table, encypt_key);
		
	for (unsigned int i = 0; i < 0x174b; ++i)
		dec[i] ^= decode_table[lrand2(&seed) % encypt_key_length];
}

static void decode_idx(char *encypt_key, long seed, BYTE *buf, DWORD len)
{
	DWORD encypt_key_length = strlen(encypt_key);
	BYTE decode_table[128];

	memset(decode_table, 0, sizeof(decode_table));
	strcpy((char *)decode_table, encypt_key);
	
	for (unsigned int i = 0; i < len; ++i)
		buf[i] ^= decode_table[update_seed(&seed) % encypt_key_length];
}

static void (*find_decode_cg(const char *name))(char *, long, BYTE *, DWORD)
{
	void (*ret)(char *, long, BYTE *, DWORD) = decode_cg;

	if (name) {
		if (!strcmpi(name, "TIKAN"))
			ret = TIKAN_decode_cg;
		else if (!strcmpi(name, "ANEBOIN"))
			ret = ANEBOIN_decode_cg;
		else if (!strcmpi(name, "hensin2"))
			ret = hensin2_decode_cg;
	}

	return ret;
}

static void (*find_decode_script(const char *name))(char *, long, BYTE *, DWORD)
{
	void (*ret)(char *, long, BYTE *, DWORD) = decode_script;

	if (name) {
		if (!strcmpi(name, "ANEBOIN"))
			ret = ANEBOIN_decode_script;
	}

	return ret;
}

/********************* pak *********************/

static int EAGLS_pak_extract_directory(struct package *pkg,
									   struct package_directory *pkg_dir);

/* 封包匹配回调函数 */
static int EAGLS_pak_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->open(pkg->lst, IO_READONLY)) {
		pkg->pio->close(pkg);
		return -CUI_EOPEN;
	}

	struct package_directory pkg_dir;
	int ret = EAGLS_pak_extract_directory(pkg, &pkg_dir);
	if (!ret)
		free(pkg_dir.directory);

	return ret;	
}

/* 封包索引目录提取函数 */
static int EAGLS_pak_extract_directory(struct package *pkg,
									   struct package_directory *pkg_dir)
{
	u32 idx_size;

	if (pkg->pio->length_of(pkg->lst, &idx_size))
		return -CUI_ELEN;

	DWORD idx_length = idx_size - 4;
	idx_entry_t *idx_buffer = (idx_entry_t *)malloc(idx_length);
	if (!idx_buffer)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg->lst, idx_buffer, idx_length, 0, IO_SEEK_SET)) {
		free(idx_buffer);
		return -CUI_EREADVEC;
	}

	long init_seed;
	if (pkg->pio->read(pkg->lst, &init_seed, 4)) {
		free(idx_buffer);
		return -CUI_EREAD;
	}

	if (init_seed)	// for old idx format
		decode_idx("1qaz2wsx3edc4rfv5tgb6yhn7ujm8ik,9ol.0p;/-@:^[]", 
			init_seed, (BYTE *)idx_buffer, idx_length);

	idx_entry_t *entry = idx_buffer;
	const char *name = get_options("game");
	DWORD i;
	if (init_seed) {
		if (name && !strcmpi(name, "ALIS")) {	// ALIS系统
			for (i = 0; entry->name[0]; i++) {
				entry->offset -= 0x800;
				++entry;
			}		
		} else {
			for (i = 0; entry->name[0]; i++) {
				entry->offset -= 0x174b;
				++entry;
			}
		}
	} else {
		for (i = 0; entry->name[0]; i++)
			++entry;
	}

	pkg_dir->index_entries = i;
	pkg_dir->directory = idx_buffer;
	pkg_dir->directory_length = i * sizeof(idx_entry_t);
	pkg_dir->index_entry_length = sizeof(idx_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int EAGLS_pak_parse_resource_info(struct package *pkg,
										 struct package_resource *pkg_res)
{
	idx_entry_t *idx_entry;

	idx_entry = (idx_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, idx_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = idx_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = idx_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int EAGLS_pak_extract_resource(struct package *pkg,
									  struct package_resource *pkg_res)
{
	BYTE *raw = (BYTE *)pkg->pio->readvec_only(pkg, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET);
	if (!raw)
		return -CUI_EREADVECONLY;

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = raw;
		return 0;
	}

	const char *name = get_options("game");
	if (strstr((char *)pkg_res->name, ".dat")) {	/* script */
		if (name && !strcmpi(name, "ALIS")) {
			pkg_res->actual_data_length = pkg_res->raw_data_length - 0x21340;
			pkg_res->actual_data = new BYTE[pkg_res->actual_data_length];
			if (!pkg_res->actual_data) {
				delete [] raw;
				return -CUI_EMEM;
			}
			memcpy(pkg_res->actual_data, raw + 0x21340, pkg_res->actual_data_length);
			alis_decode_script("ADVSYS", (BYTE *)pkg_res->actual_data, pkg_res->actual_data_length);
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
			pkg_res->replace_extension = _T(".txt");		
			goto out;
		} 

		long seed = (char)raw[pkg_res->raw_data_length - 1];
		BYTE *enc = raw + 3600;
		DWORD enc_len = pkg_res->raw_data_length - 3600 - 2;
		BYTE *dec = (BYTE *)malloc(enc_len);
		if (!dec)
			return -CUI_EMEM;

		void (*decode_script)(char *, long, BYTE *, DWORD) = find_decode_script(name);
		memcpy(dec, enc, enc_len);
		(*decode_script)("EAGLS_SYSTEM", seed, dec, enc_len);
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".txt");
		pkg_res->actual_data = dec;
		pkg_res->actual_data_length = enc_len;
	} else if (strstr((char *)pkg_res->name, ".gr")) {	/* cg */
		if (name && !strcmpi(name, "ALIS")) {
			DWORD uncomprlen = 100000000;
			BYTE *uncompr = new BYTE[uncomprlen];
			if (!uncompr) {
				delete [] raw;
				return -CUI_EMEM;
			}
			uncomprlen = alis_lzss_decompress(uncompr, uncomprlen, raw, pkg_res->raw_data_length);
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
			pkg_res->replace_extension = _T(".bmp");
			pkg_res->actual_data = uncompr;
			pkg_res->actual_data_length = uncomprlen;
			goto out;
		}

		DWORD seed = raw[pkg_res->raw_data_length - 1];
		BYTE *dec = (BYTE *)malloc(pkg_res->raw_data_length + 1);
		if (!dec)
			return -CUI_EMEM;
		// 为了弥补解压缩数据长度不足的问题
		dec[pkg_res->raw_data_length] = 0xff;

		DWORD uncomprlen = 100000000;
		BYTE *uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(dec);
			return -CUI_EMEM;
		}

		const char *name = get_options("game");
		void (*decode_cg)(char *, long, BYTE *, DWORD) = find_decode_cg(name);
		memcpy(dec, raw, pkg_res->raw_data_length);
		(*decode_cg)("EAGLS_SYSTEM", seed, dec, pkg_res->raw_data_length);
		DWORD act_uncomprlen = lzss_uncompress(uncompr, uncomprlen, dec, pkg_res->raw_data_length + 1);
		free(dec);
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp");
		pkg_res->actual_data = uncompr;
		pkg_res->actual_data_length = act_uncomprlen;	
	}
out:
	pkg_res->raw_data = raw;

	return 0;
}

/* 资源保存函数 */
static int EAGLS_pak_save_resource(struct resource *res, 
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
static void EAGLS_pak_release_resource(struct package *pkg, 
									   struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res->raw_data)
		pkg_res->raw_data = NULL;
}

/* 封包卸载函数 */
static void EAGLS_pak_release(struct package *pkg, 
							  struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg->lst);
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation EAGLS_pak_operation = {
	EAGLS_pak_match,				/* match */
	EAGLS_pak_extract_directory,	/* extract_directory */
	EAGLS_pak_parse_resource_info,	/* parse_resource_info */
	EAGLS_pak_extract_resource,		/* extract_resource */
	EAGLS_pak_save_resource,		/* save_resource */
	EAGLS_pak_release_resource,		/* release_resource */
	EAGLS_pak_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK EAGLS_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".pak"), NULL, 
		NULL, &EAGLS_pak_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR 
			| CUI_EXT_FLAG_LST))
				return -1;

	return 0;
}
