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
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/hmac.h>
#include <cui_common.h>

/* 查找特殊加密模式的key_string:
  1, 搜索常量0x12c。
  2. 找到push 12c的指令，它的调用者函数的前面有个push了6个参数的函数，
  这个函数就是HMAC, 观察第3个参数，该参数就是key_string。

方法2：
arc.nsa
%s\arc.nsa
字样的上方
 */

struct acui_information NScripter_cui_information = {
	_T("高橋直樹"),			/* copyright */
	_T("NScripter"),		/* system */
	_T(".nsa .dat .sar"),	/* package */
	_T("1.2.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-6-20 14:42"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};	

#pragma pack (1)
typedef struct {
	u16 index_entries;
	u32 data_offset;
} nsa_header_t;

typedef struct {
	s8 *name;
	u8 is_compressed;
	u32 offset;
	u32 uncomprlen;
	u32 comprlen;
} nsa_entry_t;
#pragma pack ()

typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 comprlen;
	u32 uncomprlen;
	u8 is_compressed;
} my_nsa_entry_t;

enum {
	sinsyoku2 = 1,
	kimiaru,
};

static const BYTE fuck_decrypt_table[256] = {
		0x52, 0x35, 0x36, 0x4d, 0xff, 0x03, 0x50, 0xc4, 0xc3, 0x91, 0xa8, 0x19, 0x20, 0xef, 0x45, 0x75, 
		0xd9, 0x99, 0x43, 0x09, 0x58, 0xb7, 0x89, 0xa4, 0x33, 0x5c, 0xa6, 0x8c, 0xc0, 0xb4, 0x0c, 0x12, 
		0xf0, 0x51, 0x62, 0x61, 0x7f, 0x42, 0x1c, 0x98, 0xf1, 0xd6, 0xd2, 0x84, 0x28, 0x90, 0x9d, 0x77, 
		0x3b, 0xb6, 0x71, 0xdb, 0xa7, 0xa3, 0x8e, 0xc7, 0x44, 0xe1, 0x7c, 0xc1, 0x24, 0x82, 0xe3, 0x6d, 
		0xe5, 0x40, 0x39, 0x41, 0x49, 0x6f, 0x2a, 0x22, 0xa9, 0xb2, 0xac, 0x92, 0x64, 0x1a, 0xea, 0x2c, 
		0x55, 0x17, 0xd4, 0xeb, 0x01, 0x2d, 0xb3, 0xd3, 0x5b, 0x05, 0xb9, 0x8f, 0xa2, 0x78, 0x10, 0x53, 
		0x9e, 0x97, 0x57, 0x73, 0x94, 0x59, 0xfe, 0x70, 0x4a, 0x26, 0xe0, 0xae, 0x8d, 0x1b, 0xe4, 0x48, 
		0xb8, 0x27, 0xd1, 0x32, 0x0f, 0xf5, 0xbe, 0xda, 0xf2, 0x60, 0x81, 0x5a, 0xab, 0xf8, 0x2b, 0x9c, 
		0xb5, 0x04, 0x2e, 0xe7, 0x7d, 0x4f, 0x95, 0xad, 0x00, 0xe6, 0x5d, 0xfd, 0x2f, 0x6a, 0x9f, 0x18, 
		0x1f, 0x83, 0xba, 0xd5, 0x67, 0x68, 0xfb, 0xc9, 0x8a, 0x93, 0x6b, 0xaf, 0x96, 0x0a, 0x31, 0x74, 
		0xce, 0x7b, 0xdf, 0x38, 0xbb, 0x15, 0xcc, 0x7e, 0xc2, 0x54, 0xa0, 0xbd, 0x07, 0x80, 0xcb, 0x86, 
		0xdc, 0x9b, 0x3c, 0x3a, 0xf4, 0x21, 0x06, 0x25, 0xe2, 0xe8, 0xdd, 0xfa, 0xf9, 0x76, 0xf3, 0x0d, 
		0x3f, 0x30, 0xa5, 0xbc, 0x66, 0x6c, 0xca, 0x63, 0x08, 0x4c, 0x5f, 0x3d, 0x02, 0x72, 0xec, 0x6e, 
		0xd0, 0x85, 0x1d, 0xd7, 0x65, 0xf7, 0x29, 0x11, 0xb1, 0x88, 0x14, 0x1e, 0x3e, 0x34, 0xfc, 0xc8, 
		0xf6, 0xc6, 0x4e, 0x0e, 0xcf, 0x16, 0xee, 0x13, 0xde, 0x87, 0xe9, 0x46, 0x7a, 0xcd, 0xbf, 0x5e, 
		0x47, 0xc5, 0xd8, 0x4b, 0x79, 0x0b, 0x37, 0xed, 0x69, 0x9a, 0xaa, 0xa1, 0x56, 0x23, 0x8b, 0xb0, 
};

static const char *game_key_string[] = {
	// dummy
	"",
	// 侵蝕2 ～淫魔の生贄～
	"sinsyoku2_sadasdaqwerkhlnlkfdsnlkadsfja;klsjraerioljadsfjkladsknfijhlg",
	// 君が主で執事が俺で
	"kopkl;fdsl;kl;mwekopj@pgfd[p;:kl:;,lwret;kl;kolsgfdio@pdsflkl:,rse;:l,;:lpksdfpo",
	// 魔法少女マナ Little Witch Mana
	"l:;sadl:;asd:;lasdl:;asdl:;wrew434324234dsiop@bvcnmtern,ter589sdftjskdr;w4k2034",
	// スレイブorラバーズ
	"asdsak;ll;;lkmhhwqhlhjlhkjfajlklkjlkjkjrekjlkj;asj;lksdalk;:edsa",
	// 触装天使セリカ
	"s.e.r.i.k.a.sajkreaajkldfasnmdfjklasdfm,.asdr,masdfkl;asfkl;asdflm;asdfkl;asdkl;asd",
	// キスより甘くて深いもの 体験版
	//"imakaratukuttokusadasgfdnklrsenklsdjklfdjilfdnkfsdnmfdsjklsdrnklgfnmnklgfnmgfnmdssdklnsdfsrtnmgfnmgfklnfkjlgfjklfdljk",
	// キスより甘くて深いもの
	"dfklmdsgkmlkmljklgfnlsdfnklsdfjkl;sdfmkldfskfsdmklsdfjklfdsjklsdfsdfl;",
	// 超外道勇者
	"tyou_dfsjklfgjklsdfgnmklasdfasdf_gedou_dfksdnlfgnklsrtsdf_yuusya",
	// めっちゃママ
	"sadjl;kas;klkd;lsakfsgclgfdsnmrfnzdsmflzxkmc,masd;lf;:lal;askdas;oe",
	// 新妻×義母 ～花嫁は女子校生～ パッケージ版
	"asdsak;ll;;lkmhhwqhlhjlhkjfajlklkjlkjkjrekjlkj;asj;lksdalk;:edsa",
	NULL
};

static DWORD game_id = 0;
static DWORD decrypt_block_size = 1024;

#define SWAP16(x)		((u16)(((x) << 8) | ((x) >> 8)))
#define SWAP32(x)		((u32)(((x) << 24) | (((x) << 8) & 0x00ff0000) | (((x) >> 8) & 0x0000ff00) | ((x) >> 24)))

static int test_if_encrypt(struct package *pkg)
{
	nsa_header_t nsa_header;
	int ret = pkg->pio->read(pkg, &nsa_header, sizeof(nsa_header));
	if (ret)
		return ret;

	nsa_header.index_entries = SWAP16(nsa_header.index_entries);
	nsa_header.data_offset = SWAP32(nsa_header.data_offset);

	DWORD index_length = nsa_header.data_offset - sizeof(nsa_header);
	BYTE *index_buffer = (BYTE *)malloc(index_length);
	if (!index_buffer)
		return -CUI_EMEM;

	ret = pkg->pio->read(pkg, index_buffer, index_length);
	if (ret) {
		free(index_buffer);
		return ret;
	}

	BYTE *p_index = index_buffer;
	BYTE *e_index = index_buffer + index_length;
	for (DWORD i = 0; i < nsa_header.index_entries && p_index < e_index; i++) {
		while (p_index < e_index) {
			if (!*p_index++)
				break;
		}
		p_index += 1 + 12;
	}
	free(index_buffer);
	if (i != nsa_header.index_entries || p_index != e_index)
		return 1;

	game_id = 0;
	return 0;
}

static int game_search(void)
{
	unsigned int i = 0;
	const char *game;

	game = get_options("game");
	if (!game)
		game_id = 0;
	else
		game_id = atoi(game);

	// 特殊解密
	if (game_id == 512)
		return 0;

	while (game_key_string[i++]);

	return game_id >= i ? -CUI_EMATCH : 0;
}

static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static void lzss_decompress(unsigned char *uncompr, DWORD uncomprlen, 
							unsigned char *compr, DWORD comprlen)
{
	unsigned int act_uncomprlen = 0;
	/* compr中的当前字节中的下一个扫描位的位置 */
	unsigned int curbit = 0;
	/* compr中的当前扫描字节 */
	unsigned int curbyte = 0;
	unsigned int nCurWindowByte = 0xef;
	BYTE win[256];
	struct bits bits;
	
	memset(win, 0x20, nCurWindowByte);
	bits_init(&bits, compr, comprlen);
	while (1) {
		/* 如果为0, 表示接下来的1个字节原样输出 */
		unsigned int flag;

		if (bits_get_high(&bits, 1, &flag))
			break;
		if (flag) {
			unsigned int data;

			if (bits_get_high(&bits, 8, &data))
				break;
			uncompr[act_uncomprlen++] = data;
			/* 输出的1字节放入滑动窗口 */
			win[nCurWindowByte++] = data;
			nCurWindowByte &= sizeof(win) - 1;
		} else {
			unsigned int copy_bytes, win_offset;
			unsigned int i;

			if (bits_get_high(&bits, 8, &win_offset))
				break;
			if (bits_get_high(&bits, 4, &copy_bytes))
				break;
			copy_bytes += 2;

			for (i = 0; i < copy_bytes; i++) {	
				unsigned char data;

				data = win[(win_offset + i) & (sizeof(win) - 1)];
				uncompr[act_uncomprlen++] = data;		
				/* 输出的1字节放入滑动窗口 */
				win[nCurWindowByte++] = data;
				nCurWindowByte &= sizeof(win) - 1;	
			}
		}
	}
}

static int nscripter_bmp_decompress(BYTE **__uncompr, DWORD *__uncomprlen, BYTE *compr, DWORD comprlen)
{
	BYTE *uncompr, *act_uncompr;
	DWORD uncomprlen;
	unsigned int width, height;
	struct bits bits;
	DWORD pitch;

	bits_init(&bits, compr, comprlen);
	bits_get_high(&bits, 16, &width);
	bits_get_high(&bits, 16, &height);

	pitch = (width * 3 + 3) & ~3;
	uncomprlen = pitch * height;
	uncompr = (BYTE *)malloc(uncomprlen);
	if (!uncompr)
		return -CUI_EMEM;

	memset(uncompr, 0, uncomprlen);

	/* BGR三个分量数据是分开单独存储的 */
	for (DWORD p = 0; p < 3; p++) {
		unsigned int ref_rect;
		int offset, rect_step;
		int w, w_step;

		w_step = 1;
		rect_step = 3;
		/* 参考分量 */
		bits_get_high(&bits, 8, &ref_rect);
		offset = pitch * (height - 1) + p;
		uncompr[offset] = ref_rect;
		offset += rect_step;
		w = 1;

		/* 这个循环处理一个完整的分量 */
		for (DWORD h = 0; h < height; ) {
			unsigned int N;

			bits_get_high(&bits, 3, &N);
	
			if (N == 7) {	/* 连续的具有相似数值的分量 */
				bits_get_high(&bits, 1, &N);
				N++;
			} else if (N)	/* 连续的具有不同数值的分量 */
				N += 2;

			/* 每次输出连续的4个分量 */
			for (DWORD i = 0; i < 4; i++) {
				if (N) {
					unsigned int delta;

					bits_get_high(&bits, N, &delta);
					if (N == 8)
						ref_rect = delta;
					else if (delta & 1)
						ref_rect += (delta + 1) >> 1;
					else
						ref_rect -= delta >> 1;
				}

				if (h < height)
					uncompr[offset] = ref_rect;
				w += w_step;
				offset += rect_step;

				if (w < 0) {
					w_step = 1;
					rect_step = 3;
					h++;
					w = 0;
					offset = pitch * (height - h - 1) + p;
				} else if (w >= (int)width) {
					w_step = -1;
					rect_step = -3;
					h++;
					w = width - 1;
					offset = pitch * (height - h - 1) + w * 3 + p;
				}	
			}
		}
	}

	act_uncompr = uncompr;
	if (MyBuildBMPFile(uncompr, uncomprlen, NULL, 0, width, height, 24, __uncompr, __uncomprlen, my_malloc)) {
		free(act_uncompr);
		return -CUI_EMEM;
	}
	free(act_uncompr);
	return 0;
}

static void __NScripter_decrypt(BYTE *enc_data, DWORD enc_date_len, DWORD block_number, const char *data)
{
	unsigned int d[2] = { block_number, 0x00000000 };
	unsigned char md5_hash[MD5_DIGEST_LENGTH]; 
	unsigned char sha1_hash[SHA_DIGEST_LENGTH]; 
	unsigned char key[16];
	unsigned char hmac_md[SHA512_DIGEST_LENGTH];
	unsigned int hmac_md_len;
	DWORD i0, i1;
	DWORD ptext[256];

	MD5((unsigned char *)d, sizeof(d), md5_hash);
	SHA1((unsigned char *)d, sizeof(d), sha1_hash);
	for (DWORD i = 0; i < 16; i++)
		key[i] = md5_hash[i] ^ sha1_hash[i];
	HMAC(EVP_sha512(), key, sizeof(key), (unsigned char *)data, strlen((char *)data), hmac_md, &hmac_md_len);

	for (i = 0; i < 256; i++)
		ptext[i] = i;

	BYTE index = 0;
	DWORD k = 0;
	for (i = 0; i < 256; i++) {
		DWORD tmp;

		tmp = ptext[i];
		index = (BYTE)(tmp + hmac_md[k++] + index);
		if (k == hmac_md_len)
			k = 0;
		ptext[i] = ptext[index];
		ptext[index] = tmp;
	}

	i0 = i1 = 0;
	for (i = 0; i < 0x12c; i++) {
		DWORD tmp;

		i0 = (i0 + 1) & 0xff;
		tmp = ptext[i0];
		i1 = (i1 + tmp) & 0xff;
		ptext[i0] = ptext[i1];
		ptext[i1] = tmp;
	}

	for (i = 0; i < enc_date_len; i++) {
		DWORD tmp;

		i0 = (i0 + 1) & 0xff;
		tmp = ptext[i0];
		i1 = (i1 + tmp) & 0xff;
		ptext[i0] = ptext[i1];
		ptext[i1] = tmp;
		enc_data[i] ^= (BYTE)ptext[(ptext[i0] + tmp) & 0xff];
	}
}

static void NScripter_decrypt(BYTE *enc_data, DWORD enc_date_len, DWORD block_number_base, const char *key_string)
{
	DWORD align = enc_date_len % decrypt_block_size;

	for (DWORD b = 0; b < enc_date_len / decrypt_block_size; b++)
		__NScripter_decrypt(enc_data + b * decrypt_block_size, decrypt_block_size, block_number_base + b, key_string);
	if (align)
		__NScripter_decrypt(enc_data + b * decrypt_block_size, align, block_number_base + b, key_string);
}

static int NScripter_readvec(struct package *pkg, void *data, DWORD data_len, 
							 DWORD offset, DWORD method)
{
	if (!game_id)
		return pkg->pio->readvec(pkg, data, data_len, offset, method);

	u32 abs_offset;
	switch (method) {
	case IO_SEEK_SET:
		abs_offset = offset;
		break;
	case IO_SEEK_CUR:
		if (pkg->pio->locate(pkg, &abs_offset))
			return -CUI_ELOC;
		abs_offset += offset;
		break;
	case IO_SEEK_END:
		if (pkg->pio->length_of(pkg, &abs_offset))
			return -CUI_ELEN;
		abs_offset += offset;
		break;
	default:
		return -CUI_EPARA;
	}

	// 特殊解密
	if (game_id == 512) {
		if (pkg->pio->readvec(pkg, data, data_len, offset, method))
			return -CUI_EREADVEC;

		BYTE *dec = (BYTE *)data - abs_offset;
		for (DWORD i = abs_offset; i < data_len + abs_offset; ++i)
			dec[i] = (BYTE)i ^ fuck_decrypt_table[dec[i]];
		
		return 0;
	}

	{
		DWORD block_offset, block_length;		
		BYTE *block;

		block_offset = abs_offset & ~(decrypt_block_size - 1);
		block_length = abs_offset - block_offset + data_len;

		block = (BYTE *)malloc(block_length);
		if (!block)
			return -CUI_EMEM;

		if (pkg->pio->readvec(pkg, block, block_length, block_offset, IO_SEEK_SET)) {
			free(block);
			return -CUI_EREADVEC;
		}

		if (pkg->pio->seek(pkg, abs_offset + data_len, IO_SEEK_SET)) {
			free(block);
			return -CUI_ESEEK;
		}

		NScripter_decrypt(block, block_length, block_offset / decrypt_block_size, game_key_string[game_id]);
		memcpy(data, block + abs_offset - block_offset, data_len);
		free(block);
	}

	return 0;
}

static int NScripter_read(struct package *pkg, void *data, DWORD data_len)
{
	return NScripter_readvec(pkg, data, data_len, 0, IO_SEEK_CUR);
}

/********************* nsa *********************/

static int NScripter_nsa_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (NScripter_read(pkg, magic, 4)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (!memcmp(magic, "\x00\x00\x01\xba", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->pio->seek(pkg, 0, IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_ESEEK;
	}

	return !test_if_encrypt(pkg) ? 0 : game_search();	
}

static int NScripter_nsa_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{
	nsa_header_t nsa_header;
	my_nsa_entry_t *my_index_buffer;
	BYTE *index_buffer;
	DWORD my_index_length, index_length;
	int ret;
	
	ret = NScripter_readvec(pkg, &nsa_header, sizeof(nsa_header), 0, IO_SEEK_SET);
	if (ret)
		return ret;

	nsa_header.index_entries = SWAP16(nsa_header.index_entries);
	nsa_header.data_offset = SWAP32(nsa_header.data_offset);

	index_length = nsa_header.data_offset - sizeof(nsa_header);
	index_buffer = (BYTE *)malloc(index_length);
	if (!index_buffer)
		return -CUI_EMEM;

	ret = NScripter_read(pkg, index_buffer, index_length);
	if (ret) {
		free(index_buffer);
		return ret;
	}

	my_index_length = nsa_header.index_entries * sizeof(my_nsa_entry_t);
	my_index_buffer = (my_nsa_entry_t *)malloc(my_index_length);
	if (!my_index_buffer) {
		free(index_buffer);
		return -CUI_EMEM;
	}

	BYTE *p_index = index_buffer;
	for (DWORD i = 0; i < nsa_header.index_entries; i++) {
		int name_length;
		
		for (name_length = 0; ; name_length++) {
			my_index_buffer[i].name[name_length] = *p_index++;
			if (!my_index_buffer[i].name[name_length])
				break;
		}

		my_index_buffer[i].is_compressed = *p_index++;
		my_index_buffer[i].offset = *(u32 *)p_index;
		p_index += 4;
		my_index_buffer[i].comprlen = *(u32 *)p_index;
		p_index += 4;
		my_index_buffer[i].uncomprlen = *(u32 *)p_index;
		p_index += 4;

		my_index_buffer[i].offset = SWAP32(my_index_buffer[i].offset);
		my_index_buffer[i].uncomprlen = SWAP32(my_index_buffer[i].uncomprlen);
		my_index_buffer[i].comprlen = SWAP32(my_index_buffer[i].comprlen);
		my_index_buffer[i].offset += nsa_header.data_offset;
	}
	if (i != nsa_header.index_entries) {
		free(my_index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = nsa_header.index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = my_index_length;
	pkg_dir->index_entry_length = sizeof(my_nsa_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;
	return 0;
}

static int NScripter_nsa_parse_resource_info(struct package *pkg,
											 struct package_resource *pkg_res)
{
	my_nsa_entry_t *my_nsa_entry;

	my_nsa_entry = (my_nsa_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_nsa_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_nsa_entry->comprlen;
	pkg_res->actual_data_length = my_nsa_entry->uncomprlen;	/* 数据都是明文 */
	pkg_res->offset = my_nsa_entry->offset;

	return 0;
}

static int NScripter_nsa_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;
	my_nsa_entry_t *my_nsa_entry;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (NScripter_readvec(pkg, compr, comprlen,	pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREAD;
	}

	my_nsa_entry = (my_nsa_entry_t *)pkg_res->actual_index_entry;
	if (my_nsa_entry->is_compressed == 1) {
		int ret;

		// my_nsa_entry->uncomprlen的长度是4字节不对齐的
		ret = nscripter_bmp_decompress(&uncompr, &uncomprlen, compr, comprlen);
		if (ret) {
			free(compr);
			return ret;
		}
	} else if (my_nsa_entry->is_compressed == 2) {
		uncomprlen = my_nsa_entry->uncomprlen;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}

		lzss_decompress(uncompr, uncomprlen, compr, comprlen);
	} else {
		uncompr = NULL;
		uncomprlen = 0;
	}

	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

static cui_ext_operation NScripter_nsa_operation = {
	NScripter_nsa_match,				/* match */
	NScripter_nsa_extract_directory,	/* extract_directory */
	NScripter_nsa_parse_resource_info,	/* parse_resource_info */
	NScripter_nsa_extract_resource,		/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* sar *********************/

static int NScripter_sar_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

static int NScripter_sar_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{
	nsa_header_t nsa_header;
	my_nsa_entry_t *my_index_buffer;
	BYTE *index_buffer;
	DWORD my_index_length, index_length;
	int ret;
	
	ret = NScripter_read(pkg, &nsa_header, sizeof(nsa_header));
	if (ret)
		return ret;

	nsa_header.index_entries = SWAP16(nsa_header.index_entries);
	nsa_header.data_offset = SWAP32(nsa_header.data_offset);

	index_length = nsa_header.data_offset - sizeof(nsa_header);
	index_buffer = (BYTE *)malloc(index_length);
	if (!index_buffer)
		return -CUI_EMEM;

	ret = NScripter_read(pkg, index_buffer, index_length);
	if (ret) {
		free(index_buffer);
		return ret;
	}

	my_index_length = nsa_header.index_entries * sizeof(my_nsa_entry_t);
	my_index_buffer = (my_nsa_entry_t *)malloc(my_index_length);
	if (!my_index_buffer) {
		free(index_buffer);
		return -CUI_EMEM;
	}

	BYTE *p_index = index_buffer;
	for (DWORD i = 0; i < nsa_header.index_entries; i++) {
		int name_length;
		
		for (name_length = 0; ; name_length++) {
			my_index_buffer[i].name[name_length] = *p_index++;
			if (!my_index_buffer[i].name[name_length])
				break;
		}

		my_index_buffer[i].offset = *(u32 *)p_index;
		p_index += 4;
		my_index_buffer[i].comprlen = *(u32 *)p_index;
		p_index += 4;
		my_index_buffer[i].is_compressed = 0;
		my_index_buffer[i].uncomprlen = 0;

		my_index_buffer[i].offset = SWAP32(my_index_buffer[i].offset);
		my_index_buffer[i].comprlen = SWAP32(my_index_buffer[i].comprlen);
		my_index_buffer[i].offset += nsa_header.data_offset;
	}
	if (i != nsa_header.index_entries) {
		free(my_index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = nsa_header.index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = my_index_length;
	pkg_dir->index_entry_length = sizeof(my_nsa_entry_t);
//	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

static cui_ext_operation NScripter_sar_operation = {
	NScripter_sar_match,				/* match */
	NScripter_sar_extract_directory,	/* extract_directory */
	NScripter_nsa_parse_resource_info,	/* parse_resource_info */
	NScripter_nsa_extract_resource,		/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* dat *********************/

static int NScripter_dat_match(struct package *pkg)
{
	DWORD test_data;
	
	if (lstrcmpi(pkg->name, _T("nscript.dat")))
		return -CUI_EMATCH;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (NScripter_read(pkg, &test_data, 4)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (game_search()) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	// 妹逆レイプ～淫らなつぼみの誘い的文本直接是明文
	if (!(test_data & 0x80808080)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;
}

static int NScripter_dat_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	u32 fsize;

	if (pkg->pio->length_of(pkg, &fsize))
		return -CUI_ELEN;

	pkg_res->raw_data = (BYTE *)malloc(fsize);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (NScripter_readvec(pkg, pkg_res->raw_data, fsize, 0, IO_SEEK_SET)) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
		return -CUI_EREADVEC;
	}

	for (DWORD i = 0; i < fsize; i++)
		((BYTE *)pkg_res->raw_data)[i] ^= 0x84;

	pkg_res->raw_data_length = fsize;

	return 0;
}

static cui_ext_operation NScripter_dat_operation = {
	NScripter_dat_match,			/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	NScripter_dat_extract_resource,	/* extract_resource */
	CUI_COMMON_OPERATION
};

int CALLBACK NScripter_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".nsa"), NULL, 
		NULL, &NScripter_nsa_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR 
		| CUI_EXT_FLAG_RECURSION | CUI_EXT_FLAG_RES | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;
	
	if (callback->add_extension(callback->cui, _T(".dat"), _T(".txt"), 
		NULL, &NScripter_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;
	
	if (callback->add_extension(callback->cui, _T(".sar"), NULL, 
		NULL, &NScripter_sar_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;
	
	return 0;
}
