#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <utility.h>
#include <stdio.h>
#include <openssl/md5.h>

struct acui_information FFDSystem_cui_information = {
	_T("Littlewitch / MONOCHROMA Inc."),	/* copyright */
	_T("FLOATING FRAME DIRECTOR"),			/* system */
	_T(".dat"),								/* package */
	_T("1.0.3"),							/* revision */
	_T("痴漢公賊"),							/* author */
	_T("2008-6-24 14:04"),					/* date */
	NULL,									/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

#define MIN_NAME_LEN		4
#define MAX_NAME_LEN		64				/* 来自ver2的索引项推断 */

#pragma pack (1)
typedef struct {
	s8 magic[8];	/* "RepiPack" */
	u32 version;	/* 2, 3, 4, 5 */
	u32 dat_name_length;	
} dat_header_t;

typedef struct {
	u8 name_md5[16];
	u32 offset;
	u32 comprlen;
	u32 uncomprlen;
	u32 reserved;	// always 0
} dat_entry_t;

typedef struct {	// ver2版的索引项
	s8 name[64];
	u32 offset;
	u32 uncomprlen;
	u32 comprlen;
	u32 mode;
} dat2_entry_t;
#pragma pack ()

struct resource_type {
	const char *type_magic;
	int type_magic_len;
	const TCHAR *type_suffix;
	const char *suffix;
	int (*probe)(BYTE *, DWORD);
};

/* 前2个用于解密dat name,后2个用于解密index */
static u32 dat_decode_list[][4] = {
	// 少女魔法学リトルウィッチロマネスク editio perfecta ver5
	{ 0xF358ACE6, 0x77DF8518, 0xBD42D5EF, 0x77DF8518 },
	// ピリオド ver5
	{ 0xDED5A4D1, 0xFA871AB2, 0x8128CFA5, 0xFA871AB2 },
	// ピリオド 体验版 ver5
	{ 0xEB561409, 0x13C55B05, 0x7DFF2A18, 0x13C55B05 },
	// ロンド·リーフレット ver5
	{ 0x8AF4622E, 0xCE4985A0, 0x1AF56CDB, 0xCE4985A0 },
	// 少女魔法学 リトルウィッチロマネスク特別企画番外編 ver5
	{ 0x7D59CAB4, 0x37C1ADE2, 0x805F2CAD, 0x37C1ADE2 },
	// 少女魔法学リトルウィッチロマネスク ver5
	{ 0xBD42D5EF, 0x77DF8518, 0xF358ACE6, 0x77DF8518 },
	// Quartett! Standard Edition ver4
	{ 0xB76522AB, 0x38F5CB86, 0xF5FACF01, 0x38F5CB86 },
	{ 0x361F8AA5, 0x8B4E16D9, 0x5C920F8A, 0x8B4E16D9 },
	// Quartett! 体验版 ver3
	{ 0x84F6BD90, 0xF497265C, 0xCAB563AB, 0xF497265C },
	//白詰草話番外編 ver2
	{ 0xD107AF35, 0x389ABB57, 0x21F5ACCD, 0x389ABB57 },
	// 白詰草話 -Episode of the Clovers- Standard Edition ver2
	// 白詰草話 -Episode of the Clovers- ver2
	{ 0x837FC07A, 0x98FCDBA2, 0xF517AA26, 0x98FCDBA2 },
	// terminated
	{ 0, 0, 0, 0 }
};

static int brute_crack;
static u32 *dat_decode;

static int check_if_plain_data(BYTE *data, DWORD data_length)
{
	return !MultiByteToWideChar(932, MB_ERR_INVALID_CHARS, (LPCSTR)data, data_length, NULL, 0) ? -1 : 0;
}

/* 按照匹配难度从简单到复杂排序 */
struct resource_type resource_type[] = {
	{ "OggS", 4, _T(".ogg"), ".ogg", NULL },
	{ "RIFF", 4, _T(".wav"), ".wav", NULL },
	{ "\x89PNG", 4, _T(".png"), ".png", NULL },
	{ "RASTFONT", 8, _T(".rsf"), ".rsf", NULL },
	{ "TOKENSET", 8, _T(".tkn"), ".tkn", NULL },
	{ "BM", 2, _T(".bmp"), ".bmp", NULL },
	{ "#", 1, _T(".def.txt"), ".txt", check_if_plain_data },
	{ "//", 2, _T(".def.txt"), "txt", check_if_plain_data },
	{ "", 0, _T(".def.txt"), ".txt", check_if_plain_data },
	{ NULL, 0, NULL, NULL, NULL }
};

static char dict[] = {
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k',
	'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6',
	'7', '8', '9', 0
};

static const char *suffix_list[] = {
	".def", ".ogg", ".bmp",	".wav",	".txt",	".png",	".rsf", NULL
};

static void decode(u32 *dec, DWORD dec_len, u32 hash0, u32 hash1)
{
	unsigned int dec_len_in_dword = dec_len / 4;
	
	for (unsigned int i = 0; i < dec_len_in_dword; i++) {		
		u32 tmp;
		
		dec[i] ^= hash0;
		tmp = dec[i];
		hash0 += ((tmp << 16) | (tmp >> 16)) ^ hash1;
	}
}

static void decode2(BYTE *enc_data, DWORD enc_len, BYTE key)
{
	DWORD dword_count;
	DWORD seed;
	DWORD i;

	dword_count = enc_len / 4;
	seed = key | ((key | ((key | (key << 8)) << 8)) << 8);
	for (i = 0; i < dword_count; i++) {
		int tmp = *(u32 *)enc_data;
		*(u32 *)enc_data = seed ^ (tmp << 6) ^ (((tmp >> 2) ^ (tmp << 6)) & 0x3F3F3F3F);
		enc_data += 4;
	}

	for (i = 0; i < (enc_len & 3); i++) {
		BYTE tmp = *enc_data;
		*enc_data++ = key ^ ((tmp >> 2) | (tmp << 6));
    }
}

static void decode_resource(BYTE *enc_data, DWORD enc_data_len, DWORD offset, 
							BYTE *dec_buf, DWORD dec_buf_len)
{
	if (offset < dec_buf_len) {
		if (enc_data_len > dec_buf_len - offset)
			enc_data_len = dec_buf_len - offset;

		for (DWORD i = 0; i < enc_data_len; i++)
			enc_data[i] ^= dec_buf[offset + i];
	}
}

static int lzss_decompress(BYTE *uncompr, DWORD uncomprlen, 
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

	memset(win, 0, sizeof(win));
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

			if (act_uncomprlen >= uncomprlen)
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
out:
	return (act_uncomprlen != uncomprlen) || (curbyte != comprlen);
}

#if 0
static int FFDSystem_extract_ver5(struct package *pkg, const char *name)
{
	dat_entry_t *index_buffer;
	unsigned int index_entries;
	unsigned int index_buffer_length;
	char key[MAX_PATH];
	BYTE md5[16];
	dat_entry_t dat_entry;
	BYTE *compr;
	unsigned char dec[64][16];
	MD5_CTX ctx;
	int key_len;

	if (pkg->pio->read(pkg, &index_entries, 4))
		return -CUI_EREAD;

	index_buffer_length = index_entries * sizeof(dat_entry_t);
	index_buffer = (dat_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	key_len = strlen(name);
	for (int n = 0; n < key_len; n++)
		key[n] = tolower(name[n]);
	MD5((const unsigned char *)key, key_len, md5);
	for (unsigned int i = 0; i < index_entries; i++) {
		decode((u32 *)&index_buffer[i], sizeof(dat_entry_t), dat_decode[2], dat_decode[3]);		
		if (!memcmp(md5, index_buffer[i].name_md5, 16))
			break;
	}
	if (i == index_entries) {
		printf("%s: not found in package\n", name);
		free(index_buffer);
		return -CUI_EMATCH;
	}
	memcpy(&dat_entry, &index_buffer[i], sizeof(dat_entry));
	free(index_buffer);

	if (pkg->pio->seek(pkg, dat_entry.offset, IO_SEEK_SET))
		return -CUI_ESEEK;

	compr = (BYTE *)malloc(dat_entry.comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, compr, dat_entry.comprlen)) {
		free(compr);
		return -CUI_EREAD;
	}

	char r_name[64];
	for (n = 0; n < key_len; n++)
		r_name[n] = name[key_len - 1 - n];
	r_name[key_len] = 0;

	MD5_Init(&ctx);
	MD5_Update(&ctx, r_name, key_len);
	MD5_Final(dec[0], &ctx);
	for (i = 1; i < 64; i++) {
		MD5_Update(&ctx, &r_name[i % key_len], strlen(&r_name[i % key_len]));
		MD5_Final(dec[i], &ctx);
	}

	BYTE *act_data;
	DWORD act_data_len;
	decode_resource(compr, dat_entry.comprlen, 0, &dec[0][0], 1024);
	if (dat_entry.uncomprlen == dat_entry.comprlen) {		
		act_data = compr;
		act_data_len = dat_entry.comprlen;
	} else {
		BYTE *uncompr = (BYTE *)malloc(dat_entry.uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}
		lzss_decompress(uncompr, dat_entry.uncomprlen, compr, dat_entry.comprlen);
		free(compr);
		act_data = uncompr;
		act_data_len = dat_entry.uncomprlen;
	}

	TCHAR save_name[MAX_PATH];
	if (acp2unicode(name, -1, save_name, SOT(save_name))) {
		free(act_data);
		return -1;
	}
	MySaveFile(save_name, act_data, act_data_len);
	free(act_data);

	return 0;
}
#endif

static int brute_crack_name(char *ret_name, int *ret_name_len, BYTE *md5)
{
	char name[MAX_NAME_LEN + 1];
	BYTE name_count[MAX_NAME_LEN - 4];
	int name_len;

	for (int k = 0; k < MIN_NAME_LEN; k++) {
		name_count[k] = 0;
		name[k] = dict[0];
	}
	for (name_len = MIN_NAME_LEN; name_len < MAX_NAME_LEN - 4; name_len++) {
		while (1) {
			for (int s = 0; suffix_list[s]; s++) {
				BYTE _md5[16];

				name[name_len] = 0;
				strcat(name, suffix_list[s]);	
				MD5((BYTE *)name, name_len, _md5);
				printf("cracking %s ...\r", name);
				if (!memcmp(_md5, md5, 16))
					goto out;
			}

			for (int n = 0; n < name_len; n++) {
				name_count[n]++;
				if (dict[name_count[n]]) {
					name[n] = dict[name_count[n]];
					break;
				}
				name_count[n] = 0;
				name[n] = dict[0];
			}
			if (n == name_len)
				break;
		}
	}
	if (name_len == MAX_NAME_LEN)
		return -1;
out:
	strncpy(ret_name, name, name_len);
	*ret_name_len = name_len;
	return 0;
}

static int brute_guess_name(char *ret_name, int name_len, const char *suffix, BYTE *md5)
{
	char name[MAX_NAME_LEN + 1];
	BYTE name_count[MAX_NAME_LEN - 4];

	for (int k = 0; k < name_len; k++) {
		name_count[k] = 0;
		name[k] = dict[0];
	}
	while (1) {
		BYTE _md5[16];

		name[name_len] = 0;
		strcat(name, suffix);	
		MD5((BYTE *)name, name_len, _md5);
		printf("guessing %s ...\r", name);
		if (!memcmp(_md5, md5, 16))
			break;

		if (!strcmp(suffix, ".def")) {
			name[name_len] = 0;
			strcat(name, ".txt");	
			MD5((BYTE *)name, name_len, _md5);
			printf("guessing %s ...\r", name);
			if (!memcmp(_md5, md5, 16))
				break;
		}

		for (int n = 0; n < name_len; n++) {
			name_count[n]++;
			if (dict[name_count[n]]) {
				name[n] = dict[name_count[n]];
				break;
			}
			name_count[n] = 0;
			name[n] = dict[0];
		}
		if (n == name_len)
			return -1;
	}
	strncpy(ret_name, name, name_len);
	return 0;
}

/********************* dat *********************/

static int FFDSystem_dat_match(struct package *pkg)
{
	dat_header_t dat_header;
	char *dat_name;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &dat_header, sizeof(dat_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(dat_header.magic, "RepiPack", 8)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (dat_header.version != 2 && dat_header.version != 3 && dat_header.version != 4 && dat_header.version != 5) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	dat_name = (char *)malloc(dat_header.dat_name_length + 1);
	if (!dat_name) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	char pkg_name[MAX_PATH];
	unicode2sj(pkg_name, MAX_PATH, pkg->name, -1);
	for (int i = 0; dat_decode_list[i][0]; i++) {
		if (pkg->pio->readvec(pkg, dat_name, dat_header.dat_name_length, 
				sizeof(dat_header), IO_SEEK_SET))
			break;
		dat_name[dat_header.dat_name_length] = 0;

		decode((u32 *)dat_name, dat_header.dat_name_length, 
			dat_decode_list[i][0], dat_decode_list[i][1]);

		if (!lstrcmpiA(dat_name, pkg_name))
			break;
	}

	if (!dat_decode_list[i][0]) {
		const char *code[4];

		code[0] = get_options("code0");
		code[1] = get_options("code1");
		code[2] = get_options("code2");
		code[3] = get_options("code3");
		if (!code[0] || !code[1] || !code[2] || !code[3]) {
			free(dat_name);
			pkg->pio->close(pkg);
			return -CUI_EMATCH;	
		}
		dat_decode_list[i][0] = strtoul(code[0], NULL, 0);
		dat_decode_list[i][1] = strtoul(code[1], NULL, 0);
		dat_decode_list[i][2] = strtoul(code[2], NULL, 0);
		dat_decode_list[i][3] = strtoul(code[3], NULL, 0);
	}
	free(dat_name);
	dat_decode = dat_decode_list[i];

	if (dat_header.version == 5 && !brute_crack) {
		const char *name;

		name = get_options("name");
		if (!name) {
			wcprintf(_T("you must specify the resource name for extraction, ")
				_T("otherwise use crack parameter to brute cracking\n"));
			wcprintf(_T("你必须指定要提取的资源的名字，或者使用crack参数暴力破解提取。\n"));
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		} 
#if 0	
		else {
			int ret = FFDSystem_extract_ver5(pkg, name);
			pkg->pio->close(pkg);
			if (!ret)
				printf("%s: extract OK\n", name);
			else
				printf("%s: extract failed\n", name);
			return 2;
		}
#endif
	}
	package_set_private(pkg, dat_header.version);

	return 0;	
}

static int FFDSystem_dat_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{
	BYTE *index_buffer;
	unsigned int index_entries;
	unsigned int index_buffer_length;	
	int version;

	if (pkg->pio->read(pkg, &index_entries, 4))
		return -CUI_EREAD;

	version = package_get_private(pkg);
	if (version == 2 || version == 3)
		index_buffer_length = index_entries * sizeof(dat2_entry_t);
	else
		index_buffer_length = index_entries * sizeof(dat_entry_t);

	index_buffer = (BYTE *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	for (unsigned int i = 0; i < index_entries; i++) {
		if (version == 2 || version == 3)
			// fix by 2008.3.9
			decode((u32 *)&(((dat2_entry_t *)index_buffer)[i]), sizeof(dat2_entry_t), dat_decode[2], dat_decode[3]);
		else if (version == 4 || version == 5)
			decode((u32 *)&(((dat_entry_t *)index_buffer)[i]), sizeof(dat_entry_t), dat_decode[2], dat_decode[3]);
	}

	if (version == 5 && !brute_crack) {
		const char *name = get_options("name");
		int key_len = strlen(name);
		char key[MAX_PATH];
		BYTE md5[16];
		dat_entry_t dat_entry;

		for (int n = 0; n < key_len; n++)
			key[n] = tolower(name[n]);
		MD5((const unsigned char *)key, key_len, md5);
		for (unsigned int i = 0; i < index_entries; i++) {
		//	decode((u32 *)&index_buffer[i], sizeof(dat_entry_t), dat_decode[2], dat_decode[3]);		
			if (!memcmp(md5, (((dat_entry_t *)index_buffer)[i]).name_md5, 16))
				break;
		}
		if (i == index_entries) {
			printf("%s: not found in package\n", name);
			free(index_buffer);
			return -CUI_EMATCH;
		}
		memcpy(&dat_entry, &((dat_entry_t *)index_buffer)[i], sizeof(dat_entry));
		memcpy(index_buffer, &dat_entry, sizeof(dat_entry));
		index_entries = 1;
		index_buffer_length = sizeof(dat_entry_t);
	}

	pkg_dir->index_entry_length = (version == 2) || (version == 3) ? sizeof(dat2_entry_t) : sizeof(dat_entry_t);
	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;

	return 0;
}

static int FFDSystem_dat_parse_resource_info(struct package *pkg,
											 struct package_resource *pkg_res)
{
	int version;

	version = package_get_private(pkg);
	if (version == 2 || version == 3) {
		dat2_entry_t *dat2_entry = (dat2_entry_t *)pkg_res->actual_index_entry;

		strncpy(pkg_res->name, dat2_entry->name, sizeof(dat2_entry->name));
		pkg_res->name_length = sizeof(dat2_entry->name);
		pkg_res->raw_data_length = dat2_entry->comprlen;
		pkg_res->actual_data_length = dat2_entry->uncomprlen;
		pkg_res->offset = dat2_entry->offset;
	} else {
		dat_entry_t *dat_entry = (dat_entry_t *)pkg_res->actual_index_entry;
		DWORD name_len = 0;

		if (version != 5 || brute_crack)
			for (DWORD i = 0; i < 16; i++)
				name_len += sprintf(pkg_res->name + name_len, "%02x", dat_entry->name_md5[i]);
		else {
			const char *name = get_options("name");
			name_len = strlen(name);
			strncpy(pkg_res->name, name, name_len);
		}
		pkg_res->name_length = name_len;
		pkg_res->raw_data_length = dat_entry->comprlen;
		pkg_res->actual_data_length = dat_entry->uncomprlen;
		pkg_res->offset = dat_entry->offset;
	}

	return 0;
}

static int FFDSystem_dat_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	unsigned char dec[64][16];
	MD5_CTX ctx;
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;
	BYTE md5[16];
	struct resource_type *type;
	int version;

	uncomprlen = pkg_res->actual_data_length;
	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (uncomprlen != comprlen) {
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}
	} else
		uncompr = NULL;

	version = package_get_private(pkg);
	if (version == 2 || version == 3) {
		dat2_entry_t *dat2_entry;

		if (pkg->pio->readvec(pkg, compr, comprlen, pkg_res->offset, IO_SEEK_SET)) {
			free(uncompr);
			free(compr);
			return -CUI_EREADVEC;
		}

		dat2_entry = (dat2_entry_t *)pkg_res->actual_index_entry;
		if (dat2_entry->mode == 1)
			// fix by 2008.3.9
			decode((u32 *)compr, comprlen, dat_decode[2], dat_decode[3]);
		else if (dat2_entry->mode == 2)
			// fix by 2008.3.9
			decode2(compr, comprlen, (BYTE)dat_decode[2]);

		if (uncompr)
			lzss_decompress(uncompr, uncomprlen, compr, comprlen);
	} else if (version == 5) {
		char name[64];
		int name_len;
		char r_name[64];
		BYTE *act_data;
		DWORD act_data_len;

		for (DWORD i = 0; i < 16; i++) {
			char hex[3];
			hex[0] = pkg_res->name[i * 2];
			hex[1] = pkg_res->name[i * 2 + 1];
			hex[2] = 0;
			md5[i] = (BYTE)strtoul(hex, NULL, 16);
		}

		if (brute_crack) {
			if (brute_crack_name(name, &name_len, md5)) {
				free(uncompr);
				free(compr);
				return -1;
			}
			strcpy(pkg_res->name, name);
		} else {
			name_len = pkg_res->name_length;
			strncpy(name, pkg_res->name, name_len);
			name[name_len] = 0;
		}

		for (int n = 0; n < name_len; n++)
			r_name[n] = name[name_len - 1 - n];
		r_name[name_len] = 0;

		MD5_Init(&ctx);
		MD5_Update(&ctx, r_name, name_len);
		MD5_Final(dec[0], &ctx);
		for (i = 1; i < 64; i++) {
			MD5_Update(&ctx, &r_name[i % name_len], strlen(&r_name[i % name_len]));
			MD5_Final(dec[i], &ctx);
		}

		if (pkg->pio->readvec(pkg, compr, comprlen, pkg_res->offset, IO_SEEK_SET)) {
			free(uncompr);
			free(compr);
			return -CUI_EREADVEC;
		}

		decode_resource(compr, comprlen, 0, dec[0], sizeof(dec));
		if (uncomprlen != comprlen)	{
			lzss_decompress(uncompr, uncomprlen, compr, comprlen);
			act_data = uncompr;
			act_data_len = uncomprlen;
		} else {
			act_data = compr;
			act_data_len = comprlen;
		}

		if (brute_crack) {
			for (type = resource_type; type->type_magic; type++) {
				if (!strncmp((char *)act_data, type->type_magic, type->type_magic_len)) {
					if (!type->probe || !type->probe(act_data, act_data_len)) {
						pkg_res->flags |= PKG_RES_FLAG_APEXT;
						pkg_res->replace_extension = type->type_suffix;
						break;
					}
				}				
			}
		}
	} else {
		char seed = ' ';

		for (DWORD i = 0; i < 16; i++) {
			char hex[3];
			hex[0] = pkg_res->name[i * 2];
			hex[1] = pkg_res->name[i * 2 + 1];
			hex[2] = 0;
			md5[i] = (BYTE)strtoul(hex, NULL, 16);
		}

		if (uncomprlen != comprlen) {
			for (int name_len = 5; name_len < MAX_NAME_LEN; name_len++) {
				if (pkg->pio->readvec(pkg, compr, comprlen, pkg_res->offset, IO_SEEK_SET)) {
					free(uncompr);
					free(compr);
					return -CUI_EREADVEC;
				}
				MD5_Init(&ctx);
				memcpy(&ctx, md5, 16);	// A,B,C,D
				ctx.Nl = name_len * 8;	/* name len * 8 */
				ctx.data[14] = ctx.Nl;

				for (i = 0; i < 64; i++) {
					MD5_Update(&ctx, &seed, 1);
					MD5_Final(dec[i], &ctx);
				}

				decode_resource(compr, comprlen, 0, dec[0], sizeof(dec));
				if (!lzss_decompress(uncompr, uncomprlen, compr, comprlen))
					break;
			}
			if (name_len != MAX_NAME_LEN) {
				for (type = resource_type; type->type_magic; type++) {
					if (!strncmp((char *)uncompr, type->type_magic, type->type_magic_len)) {
						if (!type->probe || !type->probe(uncompr, uncomprlen)) {
							if (brute_crack && brute_guess_name(pkg_res->name, name_len, type->suffix, md5)) {
								free(uncompr);
								free(compr);
								return -1;
							}
							sprintf(&pkg_res->name[32], "_%02d", name_len);
							pkg_res->name_length = -1;
							pkg_res->flags |= PKG_RES_FLAG_APEXT;
							pkg_res->replace_extension = type->type_suffix;
							break;
						}
					}				
				}
			}
		} else {
			for (type = resource_type; type->type_magic; type++) {
				for (int name_len = 5; name_len < MAX_NAME_LEN; name_len++) {
					if (pkg->pio->readvec(pkg, compr, comprlen, pkg_res->offset, IO_SEEK_SET)) {
						free(compr);
						return -CUI_EREADVEC;
					}
					MD5_Init(&ctx);
					memcpy(&ctx, md5, 16);	// A,B,C,D
					ctx.Nl = name_len * 8;	/* name len * 8 */
					ctx.data[14] = ctx.Nl;

					for (i = 0; i < 64; i++) {
						MD5_Update(&ctx, &seed, 1);
						MD5_Final(dec[i], &ctx);
					}

					decode_resource(compr, comprlen, 0, dec[0], sizeof(dec));
					if (!strncmp((char *)compr, type->type_magic, type->type_magic_len)) {
						if (!type->probe || !type->probe(compr, comprlen)) {
							if (brute_crack && brute_guess_name(pkg_res->name, name_len, type->suffix, md5)) {
								free(uncompr);
								free(compr);
								return -1;
							}
							sprintf(&pkg_res->name[32], "_%02d", name_len);
							pkg_res->name_length = -1;
							pkg_res->flags |= PKG_RES_FLAG_APEXT;
							pkg_res->replace_extension = type->type_suffix;
							break;
						}
					}
				}
				if (name_len != MAX_NAME_LEN)
					break;
			}
		}
	}
	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;

	return 0;
}

static int FFDSystem_dat_save_resource(struct resource *res, 
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

static void FFDSystem_dat_release_resource(struct package *pkg, 
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

static void FFDSystem_dat_release(struct package *pkg, 
								  struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

static cui_ext_operation FFDSystem_dat_operation = {
	FFDSystem_dat_match,				/* match */
	FFDSystem_dat_extract_directory,	/* extract_directory */
	FFDSystem_dat_parse_resource_info,	/* parse_resource_info */
	FFDSystem_dat_extract_resource,		/* extract_resource */
	FFDSystem_dat_save_resource,		/* save_resource */
	FFDSystem_dat_release_resource,		/* release_resource */
	FFDSystem_dat_release				/* release */
};

int CALLBACK FFDSystem_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &FFDSystem_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;
;
	if (get_options("crack"))
		brute_crack = 1;
	else
		brute_crack = 0;

	return 0;
}
