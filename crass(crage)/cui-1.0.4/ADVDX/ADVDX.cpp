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

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information ADVDX_cui_information = {
	NULL,					/* copyright */
	_T("ADVDX"),			/* system */
	_T(".dat .pak .ads"),	/* package */
	_T("1.1.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-1-24 16:34"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s32 type;			/* 1, 2 or other ... ogg - 4; 2 - bmd */
	u32 zero;			/* script.dat is not zero */
	u32 index_entries;		
	u32 data_offset;
} pak_header_t;

typedef struct {
	s8 name[32];
	u32 length;
} pak_entry_t;

typedef struct {
	s8 magic[4];	/* "_BMD" */
	u32 length;
	u32 width;		
	u32 height;
	u32 unknown;	/* 0x00000000 */
} bmd_header_t;
#pragma pack ()

typedef struct {
	s8 name[MAX_PATH];
	u32 length;
	u32 offset;
} my_pak_entry_t;

static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static void lzss_decompress(BYTE *uncompr, DWORD uncomprlen, 
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

			if (curbyte + 1 >= comprlen)
				break;

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
}

static int ADVDX_ads_encrypt;
static DWORD decrypt_block_size = 1024;

extern unsigned char ads_dec_key[256];

static void __ADVDX_decrypt(BYTE *enc_data, DWORD enc_date_len, DWORD block_number, 
							unsigned char *data, DWORD data_len)
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
	HMAC(EVP_sha512(), key, sizeof(key), data, data_len, hmac_md, &hmac_md_len);

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

static void ADVDX_decrypt(BYTE *enc_data, DWORD enc_date_len, DWORD block_number_base, 
						  unsigned char *key, DWORD key_len)
{
	DWORD align = enc_date_len % decrypt_block_size;

	for (DWORD b = 0; b < enc_date_len / decrypt_block_size; b++)
		__ADVDX_decrypt(enc_data + b * decrypt_block_size, decrypt_block_size, 
			block_number_base + b, key, key_len);
	if (align)
		__ADVDX_decrypt(enc_data + b * decrypt_block_size, align, 
		block_number_base + b, key, key_len);
}

static int ADVDX_readvec(struct package *pkg, void *data, DWORD data_len, 
						 DWORD offset, DWORD method)
{
	if (!ADVDX_ads_encrypt)
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

		ADVDX_decrypt(block, block_length, block_offset / decrypt_block_size, 
			ads_dec_key, sizeof(ads_dec_key));
		memcpy(data, block + abs_offset - block_offset, data_len);
		free(block);
	}

	return 0;
}

static int ADVDX_read(struct package *pkg, void *data, DWORD data_len)
{
	return ADVDX_readvec(pkg, data, data_len, 0, IO_SEEK_CUR);
}

/********************* pak *********************/

/* 封包匹配回调函数 */
static int ADVDX_pak_match(struct package *pkg)
{
	pak_header_t pak_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (ADVDX_read(pkg, &pak_header, sizeof(pak_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (pak_header.type <= 0) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int ADVDX_pak_extract_directory(struct package *pkg,
									   struct package_directory *pkg_dir)
{
	pak_header_t pak_header;
	my_pak_entry_t *index_buffer;
	u32 *index_offset_buffer;
	DWORD index_offset_length;
	DWORD index_buffer_length;	
	u32 pak_size;

	if (pkg->pio->length_of(pkg, &pak_size))
		return -CUI_ELEN;

	if (ADVDX_readvec(pkg, &pak_header, sizeof(pak_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	index_offset_length = pak_header.index_entries * 4;
	index_offset_buffer = (u32 *)malloc(index_offset_length);
	if (!index_offset_buffer)
		return -CUI_EMEM;

	if (ADVDX_read(pkg, index_offset_buffer, index_offset_length)) {
		free(index_offset_buffer);
		return -CUI_EREAD;
	}

	index_buffer_length = pak_header.index_entries * sizeof(my_pak_entry_t);
	index_buffer = (my_pak_entry_t *)malloc(index_buffer_length);
	if (!index_buffer) {
		free(index_offset_buffer);
		return -CUI_EMEM;
	}

	for (DWORD i = 0; i < pak_header.index_entries; i++) {
		pak_entry_t entry;

		if (index_offset_buffer[i] == -1) {
			index_buffer[i].length = 0;
			index_buffer[i].offset = -1;
			strcpy(index_buffer[i].name, "dummy");
		} else {
			index_buffer[i].offset = index_offset_buffer[i] + pak_header.data_offset;
			if (ADVDX_readvec(pkg, &entry, sizeof(entry), index_buffer[i].offset, IO_SEEK_SET))
				break;
			index_buffer[i].offset += sizeof(entry);
			
			// 如果entry中的name字段过长，会覆盖length字段
			// 这里就是针对这种情况进行恢复
			char *lost_suffix = NULL;
			if (entry.name[31] && entry.name[31] != 'd' && entry.name[30] != 'm' && entry.name[29] != 'b' && entry.name[28] != '.') {
				s8 magic[4];

				if (ADVDX_read(pkg, magic, sizeof(magic)))
					break;
				
				// TODO: 可能要加入更多针对magic的恢复手段以废除前后项恢复方式
				if (!strncmp(magic, "_BMD", 4)) {
					u32 length;
					if (ADVDX_read(pkg, &length, 4))
						break;

					index_buffer[i].length = length + sizeof(bmd_header_t);
					lost_suffix = ".bmd";
				} else {
					//exit(0);
					// 靠前后项恢复
					if (i == pak_header.index_entries - 1)
						index_buffer[i].length = pak_size - index_offset_buffer[i];
					else
						index_buffer[i].length = index_offset_buffer[i + 1] - index_offset_buffer[i];
				}
			} else
				index_buffer[i].length = entry.length;

			memset(index_buffer[i].name, 0, sizeof(index_buffer[i].name));
			strncpy(index_buffer[i].name, entry.name, 32);
			if (entry.name[0])
				strncpy(index_buffer[i].name, entry.name, 32);
			else
				sprintf(index_buffer[i].name, "noname_%04d", i);

			if (lost_suffix) {
				char *suf = strstr(index_buffer[i].name, ".");
				if (!suf)
					strcat(index_buffer[i].name, lost_suffix);
				else
					strcpy(suf, lost_suffix);
			}
		}
	}
	if (i != pak_header.index_entries) {
		free(index_buffer);
		free(index_offset_buffer);
		return -CUI_EREADVEC;
	}

	pkg_dir->index_entries = pak_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(my_pak_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int ADVDX_pak_parse_resource_info(struct package *pkg,
										 struct package_resource *pkg_res)
{
	my_pak_entry_t *my_pak_entry;

	my_pak_entry = (my_pak_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_pak_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_pak_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = my_pak_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int ADVDX_pak_extract_resource(struct package *pkg,
									  struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (ADVDX_readvec(pkg, compr, comprlen, pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVECONLY;
	}

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = compr;
		return 0;		
	}

	if (!strncmp((char *)compr, "_BMD", 4)) {	/* type2 */
		bmd_header_t *bmd = (bmd_header_t *)compr;
					
		uncomprlen = bmd->width * bmd->height * 4;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;	
		}

		lzss_decompress(uncompr, uncomprlen, compr + sizeof(*bmd), bmd->length);	
		
		if (MySaveAsBMP(uncompr, uncomprlen, NULL, 0, bmd->width, 0 - bmd->height, 32, 0,
				(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, my_malloc)) {
			free(uncompr);
			free(compr);
			return -CUI_EMEM;						
		}
		free(uncompr);
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".bmp");
	}

	pkg_res->raw_data = compr;

	return 0;
}

/* 资源保存函数 */
static int ADVDX_pak_save_resource(struct resource *res, 
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
static void ADVDX_pak_release_resource(struct package *pkg, 
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

/* 封包卸载函数 */
static void ADVDX_pak_release(struct package *pkg, 
							  struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation ADVDX_pak_operation = {
	ADVDX_pak_match,				/* match */
	ADVDX_pak_extract_directory,	/* extract_directory */
	ADVDX_pak_parse_resource_info,	/* parse_resource_info */
	ADVDX_pak_extract_resource,		/* extract_resource */
	ADVDX_pak_save_resource,		/* save_resource */
	ADVDX_pak_release_resource,		/* release_resource */
	ADVDX_pak_release				/* release */
};

/********************* ads *********************/

/* 封包匹配回调函数 */
static int ADVDX_ads_match(struct package *pkg)
{
	ADVDX_ads_encrypt = 1;

	return ADVDX_pak_match(pkg);	
}

/* 封包处理回调函数集合 */
static cui_ext_operation ADVDX_ads_operation = {
	ADVDX_ads_match,				/* match */
	ADVDX_pak_extract_directory,	/* extract_directory */
	ADVDX_pak_parse_resource_info,	/* parse_resource_info */
	ADVDX_pak_extract_resource,		/* extract_resource */
	ADVDX_pak_save_resource,		/* save_resource */
	ADVDX_pak_release_resource,		/* release_resource */
	ADVDX_pak_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK ADVDX_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &ADVDX_pak_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".pak"), NULL, 
		NULL, &ADVDX_pak_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".ads"), NULL, 
		NULL, &ADVDX_ads_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	ADVDX_ads_encrypt = 0;

	return 0;
}
