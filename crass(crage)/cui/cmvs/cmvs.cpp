#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <zlib.h>
#include <stdio.h>
#include <utility.h>
#include "cmvs_md5.h"
#include "md5.h"
#include <vector>

using namespace std;
using std::vector;

/*TODO: GameData下的封包存在.nei文件需要破解
DefMap下面的.kmap需要破解
*/
struct acui_information cmvs_cui_information = {
	_T("（株）クリアブルーコミュニケーションズ"),		/* copyright */
	NULL,					/* system */
	_T(".cpz .ps2"),		/* package */
	_T("0.2.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-8-2 0:27"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

extern void cmvs_md5(unsigned int data[], cmvs_md5_ctx *ctx);

static void *my_malloc(DWORD len)
{
	return new BYTE[len];
}	

static int cmvs_save_resource(struct resource *res, 
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

static void cmvs_release_resource(struct package *pkg, 
								  struct package_resource *pkg_res)
{
	if (pkg_res && pkg_res->actual_data) {
		delete [] pkg_res->actual_data;
		pkg_res->actual_data = NULL;
	}
	if (pkg_res && pkg_res->raw_data) {
		delete [] pkg_res->raw_data;
		pkg_res->raw_data = NULL;
	}
}

static void cmvs_release(struct package *pkg, 
						 struct package_directory *pkg_dir)
{
	void *priv = (void *)package_get_private(pkg);
	if (priv) {
		delete [] priv;
		package_set_private(pkg, NULL);
	}
	if (pkg_dir->directory && pkg_dir->directory_length) {		
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg);
}

// "解析しちゃう悪い子はお仕置きなんだよ、メッ！□…誰に言ってるの？"
// "破解这个游戏的坏孩子必须要受到处罚哟，メッ！（应该是语气词）…这话该对谁说呢？"
static char cmvs_cpz3_decode_string[] = "夝愅偟偪傖偆埆偄巕偼偍巇抲偒側傫偩傛丄儊僢両侓乧扤偵尵偭偰傞偺丠";	
// "解析する悪い子はリコがお仕置きしちゃいます。呪われちゃいますよ～、というかもう呪っちゃいました□"
// "リコ正在惩罚破解的坏孩子.会被诅咒呦~不过已经诅咒过了"
static char cmvs_cpz5_decode_string[] = "夝愅偡傞埆偄巕偼儕僐偑偍巇抲偒偟偪傖偄傑偡丅庺傢傟偪傖偄傑偡傛乣丄偲偄偆偐傕偆庺偭偪傖偄傑偟偨侓";
static BYTE cpz5_decode_table[256];

#pragma pack (1)
typedef struct {
	s8 magic[4];				// "CPZ5"
	u32 dir_entries;			// 目录项个数
	u32 total_dir_entries_size;	// 所有目录项的总长度
	u32 total_file_entries_size;// 所有文件项的总长度
	u32 index_verify[4];
	u32 md5_data[4];
	u32 index_key;				// 解密index的key
	u32 is_encrypt;
	u32 is_compressed;			// ？当然现在仍是0
	u32 header_crc;
} cpz5_header_t;

typedef struct {
	s8 magic[4];			// "CPZ3"
	u32 index_entries;
	u32 index_length;
	u32 crc;
	u32 key;
} cpz3_header_t;

typedef struct {
	u32 entry_size;
	u32 length;
	u32 offset_lo;
	u32 offset_hi;			// always 0(猜测)
	u32 crc;
	u32 key;
	s8 *name;				// name may be 4 bytes aligned
} cpz3_entry_t;

typedef struct {
	s8 magic[4];			// "PS2A"
	u32 header_length;		// 0x30
	u32 unknown0;			// 0x205b8
	u32 key;
	u32 unknown1_count;		// 每项4字节
	u32 unknown2_length;
	u32 unknown3_length;
	u32 name_index_length;
	u32 unknown4;			// 0
	u32 comprlen;
	u32 uncomprlen;
	u32 unknown5;			// 0
} ps2_header_t;

typedef struct {
	s8 magic[4];			// "BP3B"
	u32 payload_length;		// 负载数据的长度
	u32 pictures;			// 负载的图片数
} pb3b_header_t;

typedef struct {
	s8 magic[4];			// "MSK0"
	u32 data_offset;
	u32 width;
	u32 height;
} msk0_header_t;
#pragma pack ()

typedef struct {
	s8 name[MAX_PATH];
	u32 offset_lo;
	u32 offset_hi;
	u32 length;
	u32 crc;
	u32 key;
	u32 dir_key;
} my_cpz_entry_t;

static u32 crc(void *__buf, DWORD len, u32 crc)
{
	u32 *buf = (u32 *)__buf;
	
	for (u32 k = 0; k < len / 4; k++)
		crc += buf[k];
	
	for (u32 i = 0; i < (len & 3); i++)
		crc += *((u8 *)&buf[k] + i);
		
	return crc;
}

static void decode(BYTE *__buf, DWORD len, u32 key)
{
	u32 shifts;
	DWORD decode_key[16];
	DWORD *buf = (DWORD *)__buf;
	int align;

	for (DWORD i = 0; i < 16; ++i)
		decode_key[i] = *(u32 *)&cmvs_cpz3_decode_string[i * 4] + key;
		
	shifts = key;
	for (i = 0; i < 7; i++)
		shifts = (shifts >> 4) ^ key;
	shifts ^= 0xFFFFFFFD;
	shifts &= 0x0f;
	shifts += 8;

	i = 3;
	for (DWORD k = 0; k < len / 4; k++) {
		buf[k] = _lrotl((decode_key[i++] ^ buf[k]) + 0x6E58A5C2, shifts);
		i &= 0x0f;
	}

	align = len & 3;
	for (int n = align; n > 0; n--) {
		*((BYTE *)&buf[k] + align - n) = (*((BYTE *)&buf[k] + align - n) ^ (BYTE)(decode_key[i++] >> (n * 4))) + 0x52;
		i &= 0x0f;
	}
}

static void decode_cpz5(BYTE *__buf, DWORD len, u32 key)
{
	DWORD decode_key[32];
	DWORD *buf = (DWORD *)__buf;
	int align;

	DWORD string_len = lstrlenA(cmvs_cpz5_decode_string) / 4;
	for (DWORD i = 0; i < string_len; ++i)
		decode_key[i] = *(u32 *)&cmvs_cpz5_decode_string[i * 4] - key;

	u32 shifts = key;
	shifts = (shifts >> 8) ^ key;
	shifts = (shifts >> 8) ^ key;
	shifts = (shifts >> 8) ^ key;
	shifts = (shifts >> 8) ^ key;
	shifts = ((shifts ^ 0xFFFFFFFB) & 0x0f) + 7;

	i = 5;
	for (DWORD k = 0; k < len / 4; ++k) {
		buf[k] = _lrotr((decode_key[i] ^ buf[k]) + 0x784C5962, shifts) + 0x01010101;
		i = (i + 1) % 24;
	}

	align = len & 3;
	for (int n = align; n > 0; --n) {
		*((BYTE *)&buf[k] + align - n) = (*((BYTE *)&buf[k] + align - n) ^ (BYTE)(decode_key[i] >> (n * 4))) - 0x79;
		i = (i + 1) % 24;
	}
}

static void decode_resource_cpz5(BYTE *__buf, DWORD len, u32 *key_array, u32 seed)
{
	DWORD decode_key[32];
	DWORD *buf = (DWORD *)__buf;

	DWORD string_len = lstrlenA(cmvs_cpz5_decode_string);
	u32 key = key_array[1] >> 2;
	BYTE *p = (BYTE *)decode_key;

	for (DWORD i = 0; i < string_len; ++i)
		p[i] = (BYTE)key ^ cpz5_decode_table[cmvs_cpz5_decode_string[i] & 0xff];

	for (i = 0; i < string_len / 4; ++i)		
		decode_key[i] ^= seed;

	key = 0x2547A39E;
	DWORD j = 9;
	for (i = 0; i < len / 4; ++i) {	
		buf[i] = key_array[key & 3] ^ ((buf[i] ^ decode_key[(key >> 6) & 0xf] ^ (decode_key[j] >> 1)) - seed);
		j = (j + 1) & 0xf;
		key += seed + buf[i];
	}
	p = (BYTE *)(buf + i);
	for (i = 0; i < (len & 3); ++i)
		p[i] = cpz5_decode_table[p[i] ^ 0xBC];
}

static DWORD cpz5_decode_init(DWORD key, DWORD seed)
{
	for (DWORD i = 0; i < 256; ++i)
		cpz5_decode_table[i] = (BYTE)i;

	for (i = 0; i < 256; ++i) {
		BYTE tmp = cpz5_decode_table[(key >> 16) & 0xff];
		cpz5_decode_table[(key >> 16) & 0xff] = cpz5_decode_table[key & 0xff];
		cpz5_decode_table[key & 0xff] = tmp;

		tmp = cpz5_decode_table[(key >> 8) & 0xff];
		cpz5_decode_table[(key >> 8) & 0xff] = cpz5_decode_table[key >> 24];
		cpz5_decode_table[key >> 24] = tmp;

		key = seed + 0x1A743125 * _lrotr(key, 2);
	}

	return key;
}

static void cpz5_decode(BYTE *data, DWORD len, DWORD key)
{
	for (DWORD i = 0; i < len; ++i)
		data[i] = cpz5_decode_table[key ^ data[i]];
}

static void ps2_uncompress(BYTE *uncompr, DWORD uncomprlen, BYTE *compr, DWORD comprlen)
{
	DWORD curbyte = 0;
	DWORD flag = 0;
	DWORD cur_winpos = 0x7df;
	BYTE window[2048];

	memset(window, 0, cur_winpos);
	while (1) {
		flag >>= 1;
		if (!(flag & 0x0100)) {
			if (curbyte >= comprlen)
				break;
			flag = compr[curbyte++] | 0xff00;
		}
		
		if (flag & 1) {
			BYTE data;
			
			if (curbyte >= comprlen)
				break;
			
			data = compr[curbyte++];
			window[cur_winpos++] = data;
			*uncompr++ = data;
			cur_winpos &= 0x7ff;
		} else {
			DWORD offset, count;
			
			if (curbyte >= comprlen)
				break;
			offset = compr[curbyte++];
			
			if (curbyte >= comprlen)
				break;
			count = compr[curbyte++];

			offset |= (count & 0xe0) << 3;
			count = (count & 0x1f) + 2;
			
			for (DWORD i = 0; i < count; ++i) {
				BYTE data;
				
				data = window[(offset + i) & 0x7ff];
				*uncompr++ = data;
				window[cur_winpos++] = data;
				cur_winpos &= 0x7ff;
			}
		}
	}
}

static int bp3_process(BYTE *bp3, DWORD pb3_length, BYTE **ret_data, DWORD *ret_data_len)
{
	int ret;

	if (!memcmp(bp3, "PB3B", 4)) {
		pb3b_header_t *pb3b = (pb3b_header_t *)bp3;
		u16 xor = *(u16 *)(bp3 + pb3_length - 3);

		for (DWORD i = 8; i < 52; i += 2)
			*(u16 *)&bp3[i] ^= xor;

		BYTE *p = bp3 + pb3_length - 47;
		for (i = 0; i < 44; ) {
			bp3[8 + i++] -= *p++;
			bp3[8 + i++] -= *p++;
			bp3[8 + i++] -= *p++;
			bp3[8 + i++] -= *p++;
		}


		ret = 1;
	} else if (!memcmp(bp3, "BM", 2))		// TODO: test	
		ret = 2;
	else if (!memcmp(bp3, "\x89PNG", 4))	// TODO: test
		ret = 3;
	else if (!memcmp(bp3, "MSK0", 4)) {	// TODO: test
		msk0_header_t *msk0 = (msk0_header_t *)bp3;
		BYTE *dib = bp3 + msk0->data_offset;
		if (MyBuildBMPFile(dib, msk0->width * msk0->height, NULL, 0,
				msk0->width, msk0->height, 8, ret_data, ret_data_len, 
					my_malloc))
			return -CUI_EMEM;
		ret = 4;	
	} else
		ret = 0;

	return ret;
}

/********************* cpz *********************/

static int cmvs_cpz3_match(struct package *pkg)
{
	s8 magic[4];
	
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "CPZ3", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;
}

static int cmvs_cpz3_extract_directory(struct package *pkg,
									   struct package_directory *pkg_dir)
{
	cpz3_header_t cpz3_header;
	my_cpz_entry_t *index_buffer;
	DWORD index_length;
	BYTE *index;

	if (pkg->pio->readvec(pkg, &cpz3_header, sizeof(cpz3_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	cpz3_header.index_entries ^= 0x5E9C4F37;
	cpz3_header.index_length ^= 0xF32AED17;
	cpz3_header.key ^= 0xDDDDDDDD;

	index = (BYTE *)malloc(cpz3_header.index_length);
	if (!index)
		return -CUI_EMEM;
		
	if (pkg->pio->read(pkg, index, cpz3_header.index_length)) {
		free(index);
		return -CUI_EREAD;
	}

	if (crc(index, cpz3_header.index_length, 0x78341256) != cpz3_header.crc) {
		free(index);
		return -CUI_EMATCH;
	}

	decode(index, cpz3_header.index_length, cpz3_header.key ^ 0x7BF4A539);

	index_length = cpz3_header.index_entries * sizeof(my_cpz_entry_t);
	index_buffer = (my_cpz_entry_t *)malloc(index_length);
	if (!index_buffer) {
		free(index);
		return -CUI_EMEM;
	}
	
	DWORD *p = (DWORD *)index;
	for (DWORD i = 0; i < cpz3_header.index_entries; i++) {
		my_cpz_entry_t *entry = &index_buffer[i];
		DWORD entry_size, name_len;

		entry_size = *p++;
		entry->length = *p++;
		entry->offset_lo = *p++ + sizeof(cpz3_header) + cpz3_header.index_length;
		entry->offset_hi = *p++;
		entry->crc = *p++;
		entry->key = *p++;
		name_len = entry_size - 24;
		strncpy(entry->name, (char *)p, name_len);
		p = (DWORD *)((BYTE *)p + name_len);
	}
	free(index);

	pkg_dir->index_entries = cpz3_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_length;
	pkg_dir->index_entry_length = sizeof(my_cpz_entry_t);

	return 0;
}

static int cmvs_cpz_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	my_cpz_entry_t *my_cpz_entry;

	my_cpz_entry = (my_cpz_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_cpz_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_cpz_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_cpz_entry->offset_lo;

	return 0;
}

static int cmvs_cpz3_extract_resource(struct package *pkg,
									  struct package_resource *pkg_res)
{
	my_cpz_entry_t *my_cpz_entry;

	pkg_res->raw_data = malloc(pkg_res->raw_data_length);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET)) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
		return -CUI_EREADVEC;
	}
	
	my_cpz_entry = (my_cpz_entry_t *)pkg_res->actual_index_entry;
	if (crc((BYTE *)pkg_res->raw_data, pkg_res->raw_data_length, 0x78341256) != my_cpz_entry->crc) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
		return -CUI_EMATCH;
	}
	decode((BYTE *)pkg_res->raw_data, pkg_res->raw_data_length, my_cpz_entry->key ^ 0xC7F5DA63);

	return 0;
}

static cui_ext_operation cmvs_cpz3_operation = {
	cmvs_cpz3_match,				/* match */
	cmvs_cpz3_extract_directory,	/* extract_directory */
	cmvs_cpz_parse_resource_info,	/* parse_resource_info */
	cmvs_cpz3_extract_resource,		/* extract_resource */
	cmvs_save_resource,				/* save_resource */
	cmvs_release_resource,			/* release_resource */
	cmvs_release					/* release */
};

/********************* cpz *********************/

static int cmvs_cpz5_match(struct package *pkg)
{
	cpz5_header_t cpz5_header;
	
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->read(pkg, &cpz5_header, sizeof(cpz5_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(cpz5_header.magic, "CPZ5", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (crc(&cpz5_header, sizeof(cpz5_header) - 4, 0x923A564C) != cpz5_header.header_crc) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;
}

static int cmvs_cpz5_extract_directory(struct package *pkg,
									   struct package_directory *pkg_dir)
{
	cpz5_header_t cpz5_header;

	if (pkg->pio->readvec(pkg, &cpz5_header, sizeof(cpz5_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	cpz5_header.dir_entries ^= 0xFE3A53D9;
	cpz5_header.total_dir_entries_size ^= 0x37F298E7;
	cpz5_header.total_file_entries_size ^= 0x7A6F3A2C;
	cpz5_header.md5_data[0] ^= 0x43DE7C19;
	cpz5_header.md5_data[1] ^= 0xCC65F415;
	cpz5_header.md5_data[2] ^= 0xD016A93C;
	cpz5_header.md5_data[3] ^= 0x97A3BA9A;
	cpz5_header.index_key ^= 0xAE7D39BF;
	cpz5_header.is_encrypt ^= 0xFB73A955;
	cpz5_header.is_compressed ^= 0x37ACF831;

	cmvs_md5_ctx ctx;
	cmvs_md5(cpz5_header.md5_data, &ctx);

	DWORD index_len = cpz5_header.total_dir_entries_size + cpz5_header.total_file_entries_size;
	BYTE *index = new BYTE[index_len];
	if (!index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index, index_len)) {
		delete [] index;
		return -CUI_EREAD;
	}

	unsigned char digest[16];
	struct MD5Context CTX;
	MD5Init(&CTX);
	MD5Update(&CTX, index, index_len);
	MD5Final(digest, &CTX);

	DWORD *verify = (DWORD *)digest;
	if (cpz5_header.index_verify[0] != verify[0]
			&& cpz5_header.index_verify[1] != verify[1]
			&& cpz5_header.index_verify[2] != verify[2]
			&& cpz5_header.index_verify[3] != verify[3]) {
		delete [] index;
		return -CUI_EMATCH;		
	}

	decode_cpz5(index, index_len, cpz5_header.index_key ^ 0x3795B39A);

	cpz5_decode_init(cpz5_header.index_key, cpz5_header.md5_data[1]);
	cpz5_decode(index, cpz5_header.total_dir_entries_size, 58);

	DWORD key[4];
	key[0] = cpz5_header.md5_data[0] ^ (cpz5_header.index_key + 0x76A3BF29);
	key[1] = cpz5_header.index_key ^ cpz5_header.md5_data[1];
    key[2] = cpz5_header.md5_data[2] ^ (cpz5_header.index_key + 0x10000000);
    key[3] = cpz5_header.index_key ^ cpz5_header.md5_data[3];

	// 解密目录项
	DWORD seed = 0x76548AEF;
	BYTE *p = index;
    for (DWORD i = 0; i < cpz5_header.total_dir_entries_size / 4; ++i) {
		*(u32 *)p = _lrotl((*(u32 *)p ^ key[i & 3]) - 0x4A91C262, 3) - seed;
		p += 4;
		seed += 0x10FB562A;
    }
    for (DWORD j = 0; j < (cpz5_header.total_dir_entries_size & 3); ++i, ++j) {
        *p = (*p ^ (BYTE)(key[i & 3] >> 6)) + 55;
		++p;
	}

	vector<my_cpz_entry_t> cpz5_entry;
	BYTE *cur_dir_entry = index;
	BYTE *next_dir_entry;
	for (i = 0; i < cpz5_header.dir_entries; ++i) {
		DWORD cur_file_entries_offset = *(u32 *)(cur_dir_entry + 8);
		next_dir_entry = cur_dir_entry + *(u32 *)cur_dir_entry;
		DWORD next_file_entries_offset;
		if (i + 1 >= cpz5_header.dir_entries)
			next_file_entries_offset = cpz5_header.total_file_entries_size;
		else
			next_file_entries_offset = *(u32 *)(next_dir_entry + 8);
		DWORD cur_file_entries_size = next_file_entries_offset - cur_file_entries_offset;
		cpz5_decode_init(cpz5_header.index_key, cpz5_header.md5_data[2]);
		BYTE *file_entries = index + cpz5_header.total_dir_entries_size + cur_file_entries_offset;
		for (j = 0; j < cur_file_entries_size; ++j)
            file_entries[j] = cpz5_decode_table[file_entries[j] ^ 0x7E];
		DWORD entries_key = *(u32 *)(cur_dir_entry + 12);
		key[0] = cpz5_header.md5_data[0] ^ entries_key;
		key[1] = cpz5_header.md5_data[1] ^ (entries_key + 0x112233);
		key[2] = cpz5_header.md5_data[2] ^ entries_key;
		key[3] = cpz5_header.md5_data[3] ^ (entries_key + 0x34258765);

		// 解密文件项
		seed = 0x2A65CB4E;
		p = file_entries;
		for (j = 0; j < cur_file_entries_size / 4; ++j) {
			*(u32 *)p = _lrotl((*(u32 *)p ^ key[j & 3]) - seed, 2) + 0x37A19E8B;
			p += 4;
			seed -= 0x139FA9B;
		}
		for (DWORD k = 0; k < (cur_file_entries_size & 3); ++j, ++k) {
			*p = (*p ^ (BYTE)(key[j & 3] >> 4)) + 5;
			++p;
		}

		BYTE *cur_entry = file_entries;
		for (j = 0; j < *(u32 *)(cur_dir_entry + 4); ++j) {
			my_cpz_entry_t entry;			
			u32 entry_size = *(u32 *)file_entries;
			file_entries += 4;
			entry.offset_lo = *(u32 *)file_entries + sizeof(cpz5_header_t) + index_len;
			file_entries += 4;
			entry.offset_hi = *(u32 *)file_entries;
			file_entries += 4;
			entry.length = *(u32 *)file_entries;
			file_entries += 4;
			entry.crc = *(u32 *)file_entries;
			file_entries += 4;
			entry.key = *(u32 *)file_entries;
			file_entries += 4;
			if (!strcmp((char *)cur_dir_entry + 16, "root"))
				sprintf(entry.name, "%s", file_entries);
			else
				sprintf(entry.name, "%s\\%s", cur_dir_entry + 16, file_entries);
			entry.dir_key = *(u32 *)(cur_dir_entry + 12);
			cpz5_entry.push_back(entry);
			cur_entry += entry_size;
			file_entries = cur_entry;
		}
		cur_dir_entry = next_dir_entry;
	}
	cpz5_decode_init(cpz5_header.md5_data[3], cpz5_header.index_key);

	delete [] index;

	my_cpz_entry_t *my_cpz_index = new my_cpz_entry_t[cpz5_entry.size()];
	if (!my_cpz_index)
		return -CUI_EMEM;

	for (i = 0; i < cpz5_entry.size(); ++i)
		my_cpz_index[i] = cpz5_entry[i];

	pkg_dir->index_entries = cpz5_entry.size();
	pkg_dir->directory = my_cpz_index;
	pkg_dir->directory_length = sizeof(my_cpz_entry_t) * cpz5_entry.size();
	pkg_dir->index_entry_length = sizeof(my_cpz_entry_t);

	cpz5_header_t *actual_cpz5_header = new cpz5_header_t;
	if (!actual_cpz5_header) {
		delete [] my_cpz_index;
		return -CUI_EMEM;
	}
	*actual_cpz5_header = cpz5_header;
	package_set_private(pkg, actual_cpz5_header);

	return 0;
}

static int cmvs_cpz5_extract_resource(struct package *pkg,
									  struct package_resource *pkg_res)
{
	BYTE *raw = new BYTE[pkg_res->raw_data_length];
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET)) {
		delete [] raw;
		return -CUI_EREADVEC;
	}

	cpz5_header_t *cpz5_header = (cpz5_header_t *)package_get_private(pkg);
	if (cpz5_header->is_encrypt) {
		my_cpz_entry_t *my_cpz_entry = (my_cpz_entry_t *)pkg_res->actual_index_entry;
		if (crc(raw, pkg_res->raw_data_length, 0x5A902B7C) != my_cpz_entry->crc) {
			delete [] raw;
			return -CUI_EMATCH;
		}
		decode_resource_cpz5(raw, pkg_res->raw_data_length, cpz5_header->md5_data, 
			(cpz5_header->index_key ^ (my_cpz_entry->dir_key + my_cpz_entry->key)) + cpz5_header->dir_entries + 0xA3D61785);
	}

	if (strstr(pkg_res->name, ".pb3")) {
		int ret = bp3_process(raw, pkg_res->raw_data_length, 
			(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length);
		//delete [] raw;
		if (ret < 0)
			return ret;
		
		if (ret == 3) {
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
			pkg_res->replace_extension = _T(".png");
		} else if (ret && ret != 1) {
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
			pkg_res->replace_extension = _T(".bmp");
		}

		pkg_res->raw_data = raw;
	} else
		pkg_res->raw_data = raw;

	return 0;
}

static cui_ext_operation cmvs_cpz5_operation = {
	cmvs_cpz5_match,				/* match */
	cmvs_cpz5_extract_directory,	/* extract_directory */
	cmvs_cpz_parse_resource_info,	/* parse_resource_info */
	cmvs_cpz5_extract_resource,		/* extract_resource */
	cmvs_save_resource,				/* save_resource */
	cmvs_release_resource,			/* release_resource */
	cmvs_release					/* release */
};

/********************* ps2 *********************/

static int cmvs_ps2_match(struct package *pkg)
{
	s8 magic[4];
	
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "PS2A", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;
}

static int cmvs_ps2_extract_resource(struct package *pkg,
											struct package_resource *pkg_res)
{
	u32 fsize;	
	if (pkg->pio->length_of(pkg, &fsize))
		return -CUI_ELEN;

	ps2_header_t ps2_header;
	if (pkg->pio->readvec(pkg, &ps2_header, sizeof(ps2_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;
	
	DWORD comprlen = ps2_header.comprlen;
	BYTE *compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, ps2_header.header_length, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}
	
	BYTE xor = (BYTE)((ps2_header.key >> 24) + (ps2_header.key >> 3));
	BYTE shifts = ((ps2_header.key >> 20) % 5) + 1;
	for (DWORD i = 0; i < comprlen; ++i) {
		BYTE tmp = (compr[i] - 0x7c) ^ xor;
		compr[i] = (tmp >> shifts) | (tmp << (8 - shifts));
	}

	DWORD uncomprlen = ps2_header.uncomprlen + sizeof(ps2_header);
	BYTE *uncompr = (BYTE *)malloc(uncomprlen);
	if (!uncompr) {
		free(compr);
		return -CUI_EMEM;
	}

	ps2_uncompress(uncompr + sizeof(ps2_header), 
		uncomprlen - sizeof(ps2_header), compr, comprlen);
	memcpy(uncompr, &ps2_header, sizeof(ps2_header));

	pkg_res->raw_data = compr;
	pkg_res->raw_data_length = comprlen;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;
	
	return 0;
}

static cui_ext_operation cmvs_ps2_operation = {
	cmvs_ps2_match,				/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	cmvs_ps2_extract_resource,	/* extract_resource */
	cmvs_save_resource,			/* save_resource */
	cmvs_release_resource,		/* release_resource */
	cmvs_release				/* release */
};
	
int CALLBACK cmvs_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".cpz"), NULL, 
		NULL, &cmvs_cpz5_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".cpz"), NULL, 
		NULL, &cmvs_cpz3_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".ps2"), NULL, 
		NULL, &cmvs_ps2_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;
	
	BYTE xor = 0xd7;
	for (DWORD i = 0; i < 64; ++i) {		
		cmvs_cpz3_decode_string[i] ^= xor;
		xor += 0xe3;
	}
	
	return 0;
}
