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

// pack_ver == 1 && new_pack_ver1: J:\(同人ソフト) [翠憐] 変貌の晩餐\(同人ソフト)[翠憐]変貌の晩餐\GameData

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information QLIE_cui_information = {
	_T("株式会社ワームスエンタテイメント"),						/* copyright */
	_T("QLIE AVD GAME CREATE SYSTEM"),			/* system */
	_T(".pack .hash"),			/* package */
	_T("0.8.3"),				/* revision */
	_T("痴漢公賊"),				/* author */
	_T("2008-12-13 15:43"),		/* date */
	NULL,						/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 封包结构
[0](...)资源数据（加密＋压缩）
[index_offset_lo](...)索引段
[...](...)hash索引段头、hash索引段（加密＋压缩，hash_index_entries项），结尾是index_entries项索引，每项2字节，作用不明
[...](0x440)用于计算key和校验和的数据
[...](0x1c)pack_tailer_t
*/

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[16];				// "FilePackVer1.0", "FilePackVer2.0" or "FilePackVer3.0", 
	u32 index_entries;			
	u32 index_offset_lo;
	u32 index_offset_hi;
} pack_tailer_t;

typedef struct {
	s8 magic[16];				// "HashVer1.2" or "HashVer1.3"
	u32 hash_index_entries;
	u32 index_entries;		
	u32 index_entries_length;	// index_entries * 2
	u32 data_length;
} hash_header_t;

typedef struct {
	u32 magic;					// 0xff435031
	u32 flag;
	u32 uncomprlen;
} pack_compr_t;

typedef struct {
	s8 magic[8];			// "ABMP7"
	u32 unknown;
	u32 data_length;
} abmp7_header_t;

typedef struct {
	s8 magic[16];			// "abmp10"
} abmp10_header_t;

typedef struct {
	s8 magic[16];			// "abdata10"
	u32 data_length;
} abdata10_header_t;

typedef struct {
	s8 magic[16];			// "abimage10"
	u8 type;				// 2
} abimage10_header_t;

typedef struct {
	s8 magic[16];			// "abimgdat10"
	u16 name_length;
	BYTE *name;
	u8 type;				// 3
	u32 data_length;
} abimgdat10_header_t;

typedef struct {
	s8 magic[16];			// "absound10"
	u8 entries;
} absound10_header_t;

typedef struct {
	s8 magic[16];			// "absnddat10"
	u16 name_length;
	BYTE *name;
	u8 type;				// 1
	u32 data_length;
} absnddat10_header_t;

typedef struct {
	s8 magic[17];			// "ARGBSaveData1"
	u32 length;
	u32 length_hi;
} argb_header_t;
#pragma pack ()

typedef struct {
	s8 name[256];
	u64 index_entries_offset;
	u32 hash;
} my_hash_entry_t;

typedef struct {
	s8 name[256];
	u64 offset;
	u32 comprlen;
	u32 uncomprlen;
	u32 is_compressed;
	u32 is_encoded;
	u32 hash;
	u32 key;
} my_entry_t;

static int new_pack_ver1;

static DWORD gen_hash(WORD *buf, DWORD len)
{
	WORD MM0[4], MM2[4];

	for (unsigned int i = 0; i < 4; i++)
		MM0[i] = MM2[i] = 0;

	for (i = 0; i < len / 8; i++) {
		for (unsigned int k = 0; k < 4; k++) {
			MM2[k] += 0x0307;
			MM0[k] += (buf[k] ^ MM2[k]);
		}
		buf += 4;
	}
	return (MM0[0] ^ MM0[2]) | ((MM0[1] ^ MM0[3]) << 16);
}

static void __decode1(DWORD *buf, DWORD len, DWORD key)
{
	u32 tmp[2][2];
	u32 xor[2];

	for (DWORD i = 0; i < 2; ++i) {
		tmp[0][i] = 0xA73C5F9D;
		tmp[1][i] = 0xCE24F523;
		xor[i] = key ^ 0xFEC9753E;
	}

	for (i = 0; i < len / 8; ++i) {
		for (DWORD k = 0; k < 2; ++k) {
			tmp[0][k] = (tmp[0][k] + tmp[1][k]) ^ xor[k];
			*buf ^= tmp[0][k];
			xor[k] = *buf++;
		}
	}	
}

static void __decode(DWORD *buf, DWORD len, DWORD key)
{
	u32 tmp[2][2];
	u32 xor[2];

	for (DWORD i = 0; i < 2; ++i) {
		tmp[0][i] = 0xA73C5F9D;
		tmp[1][i] = 0xCE24F523;
		xor[i] = (len + key) ^ 0xFEC9753E;
	}

	for (i = 0; i < len / 8; ++i) {
		for (DWORD k = 0; k < 2; ++k) {
			tmp[0][k] = (tmp[0][k] + tmp[1][k]) ^ xor[k];
			*buf ^= tmp[0][k];
			xor[k] = *buf++;
		}
	}	
}

static void decode(DWORD *buf, DWORD len, DWORD key)
{
	if (!new_pack_ver1)
		__decode(buf, len, key);
	else
		__decode1(buf, len, key);
}

#define LEFT_CHILD	0
#define RIGHT_CHILD	1

static int pack_decompress(BYTE **ret_uncompr, DWORD *ret_uncomprlen,
						   pack_compr_t *pack_compr_header, DWORD pack_compr_len)
{
	BYTE *compr, *uncompr;
	DWORD curbyte;
	DWORD comprlen, act_uncomprlen;
	BYTE node[2][256];
	BYTE child_node[256];

	if (pack_compr_header->magic != 0xFF435031)
		return -CUI_EMATCH;

	uncompr = (BYTE *)malloc(pack_compr_header->uncomprlen);
	if (!uncompr)
		return -CUI_EMEM;

	compr = (BYTE *)(pack_compr_header + 1);
	comprlen = pack_compr_len - sizeof(pack_compr_t);
	curbyte = 0;
	act_uncomprlen = 0;
	while (curbyte < comprlen) {
		unsigned int i, count, index;

		for (i = 0; i < 256; i++)
			node[LEFT_CHILD][i] = i;

		/* 构造huffman树（因为有重复的节点index，所以应该是一种串的压缩算法），采用位置编码. 对于不连续的位置，位置标识符+0x80表示；
		 * 对于连续的位置，只有第一个+0x80，后面跟一个连续的位置计数，
		 * 后面的位置直接跟数据，而无需位置标识符.
		 */
		for (i = 0; i < 256; ) {
			count = compr[curbyte++];

			if (count > 127) {
				unsigned int step = count - 127;
				i += step;
				count = 0;
			}

			if (i > 255)
				break;

			count++;
			for (unsigned int k = 0; k < count; k++) {
				node[LEFT_CHILD][i] = compr[curbyte++];
				if (node[LEFT_CHILD][i] != i)
					node[RIGHT_CHILD][i] = compr[curbyte++];
				i++;
			}
		}

		if (pack_compr_header->flag & 1) {
			count = *(u16 *)&compr[curbyte];
			curbyte += 2;
		} else {
			count = *(u32 *)&compr[curbyte];
			curbyte += 4;
		}

		unsigned int k = 0;
		while (1) {
			/* 处理每一个子树分支 */
			if (k > 0)
				index = child_node[--k];
			else {
				if (!count)
					break;
				count--;
				index = compr[curbyte++];
			}

			/* 达到叶节点 */
			if (node[LEFT_CHILD][index] == index)
				uncompr[act_uncomprlen++] = index;
			else {
				/* 先子左树后右子树 */
				child_node[k++] = node[RIGHT_CHILD][index];
				child_node[k++] = node[LEFT_CHILD][index];
			}
		}
	}
	if (act_uncomprlen != pack_compr_header->uncomprlen) {
		free(uncompr);
		return -CUI_EUNCOMPR;
	}
	*ret_uncompr = uncompr;
	*ret_uncomprlen = act_uncomprlen;

	return 0;
}

#if 0
static int abmp7_decompress(BYTE **ret_uncompr, DWORD *ret_uncomprlen, BYTE *data, DWORD data_len)
{
	abmp7_header_t *abmp7_header = (abmp7_header_t *)data;
	BYTE *compr, *uncompr, *act_data;
	DWORD comprlen, uncomprlen, act_data_len;
	int ret;

	compr = (BYTE *)(abmp7_header + 1);
	comprlen = abmp7_header->data_length;
	ret = pack_decompress(&uncompr, &uncomprlen, (pack_compr_t *)compr, comprlen);
	if (ret)
		return ret;

	/* 这段信息意义不明 */
	free(uncompr);

	act_data_len = *(u32 *)(compr + comprlen);
	act_data = (BYTE *)malloc(act_data_len);
	if (!act_data)
		return -CUI_EMEM;

	memcpy(act_data, compr + comprlen + 4, act_data_len);
	*ret_uncompr = act_data;
	*ret_uncomprlen = act_data_len;

	return 0;
}

static int abmp10_decompress(BYTE **ret_uncompr, DWORD *ret_uncomprlen, BYTE *data, DWORD data_len)
{
	abmp10_header_t *abmp10_header = (abmp10_header_t *)data;
	BYTE *compr, *uncompr, *act_data;
	DWORD comprlen, uncomprlen, act_data_len;
	int ret;

	compr = (BYTE *)(abmp10_header + 1);
	comprlen = abmp10_header->data_length;
	ret = pack_decompress(&uncompr, &uncomprlen, (pack_compr_t *)compr, comprlen);
	if (ret)
		return ret;

	/* 这段信息意义不明 */
	free(uncompr);

	act_data_len = *(u32 *)(compr + comprlen);

	act_data = (BYTE *)malloc(act_data_len);
	if (!act_data)
		return -CUI_EMEM;

	memcpy(act_data, compr + comprlen + 4, act_data_len);
	*ret_uncompr = act_data;
	*ret_uncomprlen = act_data_len;

	return 0;
}
#endif

/********************* pack *********************/

/* 封包匹配回调函数 */
static int QLIE_pack_match(struct package *pkg)
{
	pack_tailer_t pack_tailer;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->readvec(pkg, &pack_tailer, sizeof(pack_tailer), 0 - sizeof(pack_tailer), IO_SEEK_END)) {
		pkg->pio->close(pkg);
		return -CUI_EREADVEC;
	}

	if (strncmp(pack_tailer.magic, "FilePackVer3.0", 16) 
			&& strncmp(pack_tailer.magic, "FilePackVer2.0", 16)
			&& strncmp(pack_tailer.magic, "FilePackVer1.0", 16)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}
	new_pack_ver1 = 0;

	return 0;	
}

/* 封包索引目录提取函数 */
static int QLIE_pack_extract_directory(struct package *pkg,
											struct package_directory *pkg_dir)
{
	pack_tailer_t pack_tailer;
	hash_header_t *hash_header;
	my_hash_entry_t *my_index_buffer;
	BYTE buf[0x440];
	u32 key;
	BYTE *hash_index;
	unsigned int i;
	DWORD hash_index_length, index_buffer_length;
	int ret;
	int pack_ver, hash_ver;
	BYTE *p;

	if (pkg->pio->readvec(pkg, &pack_tailer, sizeof(pack_tailer), 0 - sizeof(pack_tailer), IO_SEEK_END))
		return -CUI_EREADVEC;

	if (!strncmp(pack_tailer.magic, "FilePackVer3.0", 16))
		pack_ver = 3;
	else if (!strncmp(pack_tailer.magic, "FilePackVer2.0", 16))
		pack_ver = 2;
	else if (!strncmp(pack_tailer.magic, "FilePackVer1.0", 16))
		pack_ver = 1;

	if (pack_ver == 3 || pack_ver == 2) {
		if (pkg->pio->readvec(pkg, buf, sizeof(buf), 0 - sizeof(buf), IO_SEEK_END))
			return -CUI_EREADVEC;

		if (pack_ver == 3) {
			key = gen_hash((WORD *)&buf[0x24], 256) & 0x0fffffff;
			decode((DWORD *)buf, 32, key);// 计算出校验和
		} else if (pack_ver == 2)
			decode((DWORD *)buf, 32, 0);// 计算出校验和
	}

	u32 hash_size = 0;
	if (pkg->lst) {
		if (pkg->pio->open(pkg->lst, IO_READONLY))
			return -CUI_EOPEN;

		if (pkg->pio->length_of(pkg->lst, &hash_size)) {
			pkg->pio->close(pkg->lst);
			return -CUI_ELEN;
		}

		hash_header = (hash_header_t *)malloc(hash_size * 2);
		if (!hash_header) {
			pkg->pio->close(pkg->lst);
			return -CUI_EMEM;
		}

		if (pkg->pio->read(pkg->lst, hash_header, hash_size)) {
			free(hash_header);
			pkg->pio->close(pkg->lst);
			return -CUI_EREAD;
		}
		memcpy((BYTE *)hash_header + hash_size, hash_header, hash_size);
		pkg->pio->close(pkg->lst);
	} else if (pack_ver != 1) {
		hash_size = *(u32 *)&buf[32];
		hash_header = (hash_header_t *)malloc(hash_size);
		if (!hash_header)
			return -CUI_EMEM;

		if (pkg->pio->seek(pkg, 0 - (sizeof(buf) + hash_size), IO_SEEK_END)) {
			free(hash_header);
			return -CUI_ESEEK;
		}

		if (pkg->pio->read(pkg, hash_header, hash_size)) {
			free(hash_header);
			return -CUI_EREAD;
		}
	}

	if (!hash_size)
		goto no_hash;

	if (!strncmp(hash_header->magic, "HashVer1.3", sizeof(hash_header->magic)))
		hash_ver = 13;
	else if (!strncmp(hash_header->magic, "HashVer1.2", sizeof(hash_header->magic)))
		hash_ver = 12;
	else {
		free(hash_header);
		return -CUI_EMATCH;
	}		

retry:
	decode((DWORD *)(hash_header + 1), hash_header->data_length, 0x428);
	ret = pack_decompress(&hash_index, &hash_index_length, (pack_compr_t *)(hash_header + 1), hash_header->data_length);
	if (ret) {
		if (pack_ver == 1 && !new_pack_ver1) {
			new_pack_ver1 = 1;
			memcpy(hash_header, (BYTE *)hash_header + hash_size, hash_size);
			goto retry;
		}
		free(hash_header);
		return ret;
	}

	index_buffer_length = sizeof(my_hash_entry_t) * hash_header->hash_index_entries;
	my_index_buffer = (my_hash_entry_t *)malloc(index_buffer_length);
	if (!my_index_buffer) {
		free(hash_index);
		free(hash_header);
		return -CUI_EMEM;
	}

	/* 这个表的具体作用不明 */
	p = hash_index;
	for (i = 0; i < hash_header->hash_index_entries; i++) {		
		u16 n = *(u16 *)p;

		p += 2;
		for (u16 j = 0; j < n; ++j) {
			u16 name_length;
			u32 offset_lo, offset_hi;
			
			name_length = *(u16 *)p;
			p += 2;

			strncpy(my_index_buffer[i].name, (char *)p, name_length);
			my_index_buffer[i].name[name_length] = 0;
			p += name_length;
			offset_lo = *(u32 *)p;
			p += 4;
			offset_hi = *(u32 *)p;
			p += 4;
			if (hash_ver == 13) {
				my_index_buffer[i].hash = *(u32 *)p;
				p += 4;
				my_index_buffer[i].index_entries_offset = (u64)(offset_lo) | (u64)(offset_hi) << 32;			
			}
			//printf("%s: hash %x @ %08x%08x\n", my_index_buffer[i].name, my_index_buffer[i].hash, offset_hi, offset_lo);
		}
	}
	free(hash_index);
	free(hash_header);

no_hash:
	if (pkg->pio->seek(pkg, pack_tailer.index_offset_lo, IO_SEEK_SET)) {
		free(my_index_buffer);
		return -CUI_ESEEK;
	}

	my_entry_t *index_buffer = (my_entry_t *)malloc(sizeof(my_entry_t) * pack_tailer.index_entries);
	if (!index_buffer) {
		free(my_index_buffer);
		return -CUI_EMEM;
	}

	for (i = 0; i < pack_tailer.index_entries; ++i) {	
		u16 name_length;
		BYTE dec;
		u32 offset_lo, offset_hi;

		if (pkg->pio->read(pkg, &name_length, sizeof(name_length)))
			break;

		if (pkg->pio->read(pkg, index_buffer[i].name, name_length))
			break;

		if (pack_ver == 3)
			dec = name_length + (key ^ 0x3e);
		else if (pack_ver == 2)
			dec = name_length + (0xc4 ^ 0x3e);
		else if (!new_pack_ver1)
			dec = name_length + (0xc4 ^ 0x3e);
		else
			dec = 0xc4 ^ 0x3e;

		for (unsigned int k = 0; k < name_length; ++k)
			index_buffer[i].name[k] ^= (((k + 1) ^ dec) + (k + 1));
		index_buffer[i].name[k] = 0;

		if (pkg->pio->read(pkg, &offset_lo, sizeof(offset_lo)))
			break;

		if (pkg->pio->read(pkg, &offset_hi, sizeof(offset_hi)))
			break;

		if (pkg->pio->read(pkg, &index_buffer[i].comprlen, sizeof(index_buffer[i].comprlen)))
			break;

		if (pkg->pio->read(pkg, &index_buffer[i].uncomprlen, sizeof(index_buffer[i].uncomprlen)))
			break;

		if (pkg->pio->read(pkg, &index_buffer[i].is_compressed, sizeof(index_buffer[i].is_compressed)))
			break;

		if (pkg->pio->read(pkg, &index_buffer[i].is_encoded, sizeof(index_buffer[i].is_encoded)))
			break;

		if (new_pack_ver1)
			index_buffer[i].hash = 0;
		else {
			if (pkg->pio->read(pkg, &index_buffer[i].hash, sizeof(index_buffer[i].hash)))
				break;
		}

		index_buffer[i].offset = (u64)offset_lo | ((u64)offset_hi << 32);
		if (pack_ver == 3)
			index_buffer[i].key = key;
		else if (pack_ver == 2 || pack_ver == 1)
			index_buffer[i].key = 0;
	//	printf("%s: hash %x %d / %d @ %08x%08x\n", index_buffer[i].name, index_buffer[i].hash, index_buffer[i].comprlen, 
	//		index_buffer[i].uncomprlen, offset_hi, offset_lo);
	}
	if (i != pack_tailer.index_entries) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = pack_tailer.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = sizeof(my_entry_t) * pack_tailer.index_entries;
	pkg_dir->index_entry_length = sizeof(my_entry_t);
	package_set_private(pkg, pack_ver);

	return 0;
}

/* 封包索引项解析函数 */
static int QLIE_pack_parse_resource_info(struct package *pkg,
											  struct package_resource *pkg_res)
{
	my_entry_t *my_entry;

	my_entry = (my_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_entry->comprlen;
	pkg_res->actual_data_length = my_entry->uncomprlen;	/* 数据都是明文 */
	pkg_res->offset = (u32)my_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int QLIE_pack_extract_resource(struct package *pkg,
										   struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr, **act_data;
	DWORD comprlen, uncomprlen, act_data_len;
	my_entry_t *my_entry;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = compr;
		return 0;
	}

	my_entry = (my_entry_t *)pkg_res->actual_index_entry;
	if (my_entry->hash) {
		if (gen_hash((WORD *)compr, comprlen) != my_entry->hash) {
			free(compr);
			return -CUI_EMATCH;
		}
	}

	int pack_ver = (int)package_get_private(pkg);
	if (my_entry->is_encoded)
		decode((DWORD *)compr, comprlen, my_entry->key);

	if (my_entry->is_compressed) {
		int ret = pack_decompress(&uncompr, &uncomprlen, (pack_compr_t *)compr, comprlen);
		free(compr);
		compr = NULL;
		if (ret)
			return ret;
		act_data = &uncompr;
		act_data_len = uncomprlen;
	} else {
		uncompr = NULL;
		uncomprlen = 0;
		act_data = &compr;
		act_data_len = comprlen;
	}

	if (!strncmp("ARGBSaveData1", (char *)*act_data, 17)) {
		argb_header_t *argb_header = (argb_header_t *)*act_data;
		BYTE *argb_data;

		uncomprlen = argb_header->length;
		argb_data = (BYTE *)malloc(uncomprlen);
		if (!argb_data) {
			free(act_data);
			return -CUI_EMEM;
		}
		memcpy(argb_data, argb_header + 1, uncomprlen);
		free(*act_data);
		*act_data = NULL;
		uncompr = argb_data;

		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".bmp");
	}
#if 0
	if (!strncmp("ABMP7", (char *)act_data, 8)) {
		int ret;
		BYTE *ret_data;
		DWORD ret_data_len;

		ret = abmp7_decompress(&ret_data, &ret_data_len, act_data, act_data_len);
		if (ret) {
			free(act_data);
			return ret;
		}
		free(act_data);
		uncompr = ret_data;
		uncomprlen = ret_data_len;

		if (!memcmp(uncompr, "\x89PNG", 4)) {
			pkg_res->flags |= PKG_RES_FLAG_APEXT;
			pkg_res->replace_extension = _T(".png");
		} else if (!memcmp(uncompr, "BM", 2)) {
			pkg_res->flags |= PKG_RES_FLAG_APEXT;
			pkg_res->replace_extension = _T(".bmp");
		}
	} else if (!strncmp("abmp10", (char *)act_data, 8)) {
		int ret;
		BYTE *ret_data;
		DWORD ret_data_len;

		ret = abmp10_decompress(&ret_data, &ret_data_len, act_data, act_data_len);
		if (ret) {
			free(act_data);
			return ret;
		}
		free(act_data);
		uncompr = ret_data;
		uncomprlen = ret_data_len;

		if (!memcmp(uncompr, "\x89PNG", 4)) {
			pkg_res->flags |= PKG_RES_FLAG_APEXT;
			pkg_res->replace_extension = _T(".png");
		} else if (!memcmp(uncompr, "BM", 2)) {
			pkg_res->flags |= PKG_RES_FLAG_APEXT;
			pkg_res->replace_extension = _T(".bmp");
		}
	}
#endif

	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

/* 资源保存函数 */
static int QLIE_pack_save_resource(struct resource *res, 
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
static void QLIE_pack_release_resource(struct package *pkg, 
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
static void QLIE_pack_release(struct package *pkg, 
								   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation QLIE_pack_operation = {
	QLIE_pack_match,					/* match */
	QLIE_pack_extract_directory,		/* extract_directory */
	QLIE_pack_parse_resource_info,		/* parse_resource_info */
	QLIE_pack_extract_resource,		/* extract_resource */
	QLIE_pack_save_resource,			/* save_resource */
	QLIE_pack_release_resource,		/* release_resource */
	QLIE_pack_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK QLIE_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".pack"), NULL, 
		NULL, &QLIE_pack_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;
#if 0
	if (callback->add_extension(callback->cui, _T(".b"), NULL, 
		NULL, &QLIE_pack_operation, CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
			return -1;
#endif
	return 0;
}
