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
struct acui_information SystemC_cui_information = {
	_T("SystemC"),			/* copyright */
	_T("INTERHEART"),		/* system */
	_T(".fpk"),				/* package */
	_T("1.1.3"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-5-30 10:37"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	u32 index_entries;
} fpk_header_t;

typedef struct {		
	u32 offset;
	u32 length;
	s8 name[24];
} fpk_entry_t;

/*
新的entry格式：
typedef struct {		
	u32 offset;
	u32 length;
	s8 name[24];
	u32 unknown;
} fpk_entry2_t;

typedef struct {
	u32 offset;
	u32 length;
	s8 name[128];
	u32 unknown;
} fpk_entry3_t;
*/

typedef struct {
	s8 magic[4];		// "ZLC2"
	u32 length;			// data actual length
} zlc2_header_t;

typedef struct {
	s8 magic[4];		// "GCGK"
	u16 width;
	u16 height;
	u32 comprlen;		// 12 + heigth * 4 + comprlen = kg file length
} kg_header_t;

typedef struct {
	s8 magic[4];		// "RLE0"
	u32 header_size;	// 0x18
	u32 comprlen;
	u32 uncomprlen;
	u32 mode;
	u32 unknown1;		// 0
} rle0_header_t;
#pragma pack ()

static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static inline BYTE getbit_be(BYTE byte, unsigned int pos)
{
	return !!(byte & (1 << (7 - pos)));
}

static void lzxx_decompress(BYTE *uncompr, DWORD *uncomprlen, 
							BYTE *compr, DWORD comprlen)
{
	unsigned int act_uncomprlen = 0;
	/* compr中的当前字节中的下一个扫描位的位置 */
	unsigned int curbit = 0;
	/* compr中的当前扫描字节 */
	unsigned int curbyte = 0;
	
	memset(uncompr, 0, *uncomprlen);
	while (1) {
		/* 如果为0, 表示接下来的1个字节原样输出 */
		BYTE flag;

		if (curbyte >= comprlen)
			break;

		flag = compr[curbyte++];
		for (curbit = 0; curbit < 8; curbit++) {
			if (!getbit_be(flag, curbit)) {
				if (curbyte >= comprlen)
					goto out;

				if (act_uncomprlen >= *uncomprlen)
					goto out;

				uncompr[act_uncomprlen++] = compr[curbyte++];
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

				if (!win_offset)
					win_offset = act_uncomprlen - 0x1000;
				else
					win_offset = act_uncomprlen - win_offset;

				for (i = 0; i < copy_bytes; i++) {	
					if (act_uncomprlen >= *uncomprlen)
						goto out;

					uncompr[act_uncomprlen++] = uncompr[win_offset + i];		
				}
			}
		}
	}
out:
	*uncomprlen = act_uncomprlen;
}

static DWORD rle0_uncompress(BYTE *uncompr, DWORD uncomprlen, 
							 BYTE *compr, DWORD comprlen)
{
	DWORD curbyte = 0;
	DWORD act_uncomprlen = 0;
	while (curbyte < comprlen && act_uncomprlen < uncomprlen) {
		BYTE flag = compr[curbyte++];
		DWORD count = flag & 0x3f;
		if (!count)
			count = 64;
		flag >>= 6;
		DWORD i;
		switch (flag) {
		case 0:
			for (i = 0; i < count; ++i)
				uncompr[act_uncomprlen++] = compr[curbyte++];
			break;
		case 1:
			for (i = 0; i < count + 1; ++i)
				uncompr[act_uncomprlen++] = compr[curbyte];
			++curbyte;
			break;
		case 2:
			for (i = 0; i < count + 1; ++i) {
				uncompr[act_uncomprlen++] = compr[curbyte + 0];
				uncompr[act_uncomprlen++] = compr[curbyte + 1];
			}
			curbyte += 2;			
			break;
		case 3:
			for (i = 0; i < count + 1; ++i) {
				uncompr[act_uncomprlen++] = compr[curbyte + 0];
				uncompr[act_uncomprlen++] = compr[curbyte + 1];
				uncompr[act_uncomprlen++] = compr[curbyte + 2];
			}
			curbyte += 3;
			break;
		}
	}
	return act_uncomprlen;
}

/* 32 bit图像解压缩 */
static DWORD kg_decompress(BYTE *__uncompr, DWORD uncomprlen, BYTE *compr, DWORD comprlen)
{
	kg_header_t *kg = (kg_header_t *)compr;
	u32 *offset = (u32 *)(kg + 1);	/* 每一个偏移值指向一组用于解压width个象素数据的编码 */
	DWORD *uncompr = (DWORD *)__uncompr;
	DWORD act_uncomprlen = 0;	/* 实际解压的象素数 */

	compr = (BYTE *)(offset + kg->height);
	for (unsigned int h = 0; h < kg->height; h++) {
		BYTE *cp;

		cp = compr + offset[h];
//		if (act_uncomprlen != kg->width * h)
//			printf("miss-match!\n");
		for (unsigned int w = 0; w < kg->width; ) {
			BYTE flag = *cp++;
			DWORD pixel = *cp++;

			if (!pixel)
				pixel = 256;
			
			if (!flag) {
				memset(&uncompr[act_uncomprlen], 0, pixel * 4);
				act_uncomprlen += pixel;
			} else {
				for (unsigned int j = 0; j < pixel; j++) {
					BYTE b, g, r, a;

					a = flag;
					r = *cp++;
					g = *cp++;
					b = *cp++;
					uncompr[act_uncomprlen++] = (a << 24) | (r << 16) | (g << 8) | b;
				}
			}
			w += pixel;
		}
	}
	return act_uncomprlen * 4;
}

/********************* fpk *********************/

static int SystemC_fpk_extract_directory(struct package *pkg,
										 struct package_directory *pkg_dir);

/* 封包匹配回调函数 */
static int SystemC_fpk_match(struct package *pkg)
{
	if (!lstrcmpi(pkg->name, L"DM1.FPK"))
		return -CUI_EMATCH;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	fpk_header_t header;

	if (pkg->pio->read(pkg, &header, sizeof(header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (!memcmp(&header.index_entries, "\x00\x00\x01\0xba", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	struct package_directory pkg_dir;
	int ret = SystemC_fpk_extract_directory(pkg, &pkg_dir);
	if (!ret)
		free(pkg_dir.directory);

	return ret;	
}

/* 封包索引目录提取函数 */
static int SystemC_fpk_extract_directory(struct package *pkg,
										 struct package_directory *pkg_dir)
{
	fpk_header_t header;
	DWORD index_buffer_length;	
	BYTE *index_buffer;

	if (pkg->pio->readvec(pkg, &header, sizeof(header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	if (!(header.index_entries & 0x80000000)) {
		index_buffer_length = header.index_entries * sizeof(fpk_entry_t);
		index_buffer = (BYTE *)new fpk_entry_t[header.index_entries];
		if (!index_buffer)
			return -CUI_EMEM;

		if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
			delete [] index_buffer;
			return -CUI_EREAD;
		}
	} else {
		header.index_entries &= ~0x80000000;

		if (pkg->pio->seek(pkg, -8, IO_SEEK_END))
			return -CUI_ESEEK;

		u32 dec_key;
		if (pkg->pio->read(pkg, &dec_key, 4))
			return -CUI_EREAD;

		u32 index_offset;
		if (pkg->pio->read(pkg, &index_offset, 4))
			return -CUI_EREAD;

		if (pkg->pio->seek(pkg, index_offset, IO_SEEK_SET))
			return -CUI_ESEEK;

		u8 test_data[256];
		memset(test_data, 0, sizeof(test_data));
		pkg->pio->read(pkg, test_data, sizeof(test_data));
		DWORD *p = (DWORD *)(test_data + 8);
		// guess name length
		int first = 0;
		for (DWORD i = 0; i < (256 - 8) / 4; i++) {
			if (!first && (p[i] == dec_key))
				first = 1;
			else if (first && (p[i] != dec_key))
				break;
		}

		DWORD entry_size = 8 + i * 4 + 4;
		index_buffer_length = header.index_entries * entry_size;
		index_buffer = new BYTE[index_buffer_length];
		if (!index_buffer)
			return -CUI_EMEM;

		if (pkg->pio->seek(pkg, index_offset, IO_SEEK_SET)) {
			free(index_buffer);
			return -CUI_ESEEK;
		}

		if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
			free(index_buffer);
			return -CUI_EREAD;
		}

		DWORD *enc = (DWORD *)index_buffer;
		for (i = 0; i < index_buffer_length / 4; i++)
			enc[i] ^= dec_key;
	}

	pkg_dir->index_entries = header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = index_buffer_length / header.index_entries;
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;
	package_set_private(pkg, pkg_dir->index_entry_length);

	return 0;
}

/* 封包索引项解析函数 */
static int SystemC_fpk_parse_resource_info(struct package *pkg,
										   struct package_resource *pkg_res)
{
	fpk_entry_t *fpk_entry;
	DWORD entry_size = (DWORD)package_get_private(pkg);

	fpk_entry = (fpk_entry_t *)pkg_res->actual_index_entry;
	if (entry_size == sizeof(fpk_entry_t))
		strncpy(pkg_res->name, fpk_entry->name, 24);
	else
		strcpy(pkg_res->name, fpk_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = fpk_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = fpk_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int SystemC_fpk_extract_resource(struct package *pkg,
										struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)pkg->pio->readvec_only(pkg, comprlen, pkg_res->offset, IO_SEEK_SET);
	if (!compr)
		return -CUI_EREADVECONLY;

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = compr;
		return 0;
	}

	if (!memcmp((char *)compr, "ZLC2", 4)) {
		BYTE *act_uncompr;
		zlc2_header_t *zlc2_header = (zlc2_header_t *)compr;
		DWORD actlen;

		uncompr = (BYTE *)malloc(zlc2_header->length);
		if (!uncompr)
			return -CUI_EMEM;

		uncomprlen = actlen = zlc2_header->length;
		lzxx_decompress(uncompr, &actlen, compr + sizeof(zlc2_header_t), comprlen - sizeof(zlc2_header_t));
		if (actlen != uncomprlen) {
			free(uncompr);
			return -CUI_EUNCOMPR;
		}
		compr = NULL;
		if (strstr(pkg_res->name, ".hmp")) {
			if (MyBuildBMPFile(uncompr + 8, uncomprlen - 8, NULL, 0, *(u32 *)uncompr, *(u32 *)(uncompr + 4), 
					8, &act_uncompr, &actlen, my_malloc)) {
				free(uncompr);
				return -CUI_EUNCOMPR;	
			}
			free(uncompr);
			uncompr = act_uncompr;
			uncomprlen = actlen;
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
			pkg_res->replace_extension = _T(".hmp.bmp");
		} else if (!memcmp(uncompr, "GCGK", 4)) {
			kg_header_t *kg = (kg_header_t *)uncompr;
			DWORD kg_actlen;
			BYTE *kg_act;
			DWORD width, height;

			width = kg->width;
			height = kg->height;
			kg_actlen = width * height * 4;
			kg_act = (BYTE *)malloc(kg_actlen);
			if (!kg_act) {
				free(uncompr);
				return -CUI_EMEM;
			}
			actlen = kg_decompress(kg_act, kg_actlen, uncompr, uncomprlen);
			free(uncompr);
			if (actlen != kg_actlen) {
				free(kg_act);
				return -CUI_EUNCOMPR;	
			}

			if (MyBuildBMPFile(kg_act, kg_actlen, NULL, 0, width, 0 - height, 32, 
					&uncompr, &uncomprlen, my_malloc)) {
				free(kg_act);
				return -CUI_EMEM;	
			}
			free(kg_act);
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
			pkg_res->replace_extension = _T(".kg.bmp");
		}
	} else if (!memcmp(compr, "RLE0", 4)) {
		rle0_header_t *rle = (rle0_header_t *)compr;
		uncomprlen = rle->uncomprlen;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr)
			return -CUI_EMEM;

		if (rle->mode == 0) {
			memcpy(uncompr, compr + rle->header_size, uncomprlen);
		} else if (rle->mode == 1) {
			rle0_uncompress(uncompr, rle->uncomprlen, 
				compr + rle->header_size, rle->comprlen);
		} else {
			free(uncompr);
			return -CUI_EMATCH;
		}
	} else {
		uncompr = NULL;
		uncomprlen = 0;
	}

	pkg_res->raw_data = compr;
	pkg_res->raw_data_length = comprlen;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

/* 资源保存函数 */
static int SystemC_fpk_save_resource(struct resource *res, 
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
static void SystemC_fpk_release_resource(struct package *pkg, 
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
static void SystemC_fpk_release(struct package *pkg, 
								struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation SystemC_fpk_operation = {
	SystemC_fpk_match,				/* match */
	SystemC_fpk_extract_directory,	/* extract_directory */
	SystemC_fpk_parse_resource_info,/* parse_resource_info */
	SystemC_fpk_extract_resource,	/* extract_resource */
	SystemC_fpk_save_resource,		/* save_resource */
	SystemC_fpk_release_resource,	/* release_resource */
	SystemC_fpk_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK SystemC_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".fpk"), NULL, 
		NULL, &SystemC_fpk_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
