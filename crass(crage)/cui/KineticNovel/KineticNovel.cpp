#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information KineticNovel_cui_information = {
	_T("VisualArt"),		/* copyright */
	_T("VisualArt's ScriptEngine RealLive"),			/* system */
	_T(".pak"),		/* package */
	_T("1.0.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-3-24 8:52"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
							// 0
	u32 anime_paturn_width;	// 16
	u8//20
	u8;	// 21
	u32 xxggmhHher_width;// 24
	u32 anime_paturn_height;// 32
	u32 //36
	u32	// 40
	u32//44
	u32// 48
	u32		//56
	u8 // 60
	u32 cipher_length;// 64
	u32 //72
	u32 xxggmhHher_height;//76
	u8 ?(8);		// 80
} pak_header_t;
// 4 * xxggmhHher_width * xxggmhHher_height
#pragma pack ()

/* .dat封包的索引项结构 */
typedef struct {
	s8 name[256];
	u32 name_length;
	u32 offset;
	u32 length;
} dat_entry_t;

static BYTE decode_table[256] = {
		0x81, 0xf8, 0xdb, 0x9a, 0x00, 0x86, 0xdf, 0x0f, 
		0x2c, 0x71, 0xb2, 0x88, 0x83, 0xcf, 0x43, 0x19, 
		0x8a, 0x08, 0xdd, 0xb4, 0x8b, 0x32, 0xe5, 0x80, 
		0x29, 0xaa, 0xe8, 0x0b, 0x63, 0xf3, 0xa5, 0x19, 
		0x4f, 0xc0, 0x1b, 0xdc, 0xbb, 0xd3, 0x00, 0x40, 
		0x37, 0x26, 0xab, 0xea, 0x87, 0xea, 0x90, 0x8f, 
		0x50, 0x71, 0xe8, 0x17, 0xac, 0x93, 0x8e, 0xdc, 
		0x90, 0xad, 0x6f, 0xb4, 0x28, 0x3d, 0x13, 0x0a, 
		0xf5, 0x2b, 0xd6, 0x48, 0x2d, 0x0f, 0x33, 0x92, 
		0x27, 0xaa, 0xba, 0x32, 0x5f, 0x7a, 0xde, 0x00, 
		0xb7, 0x86, 0x1f, 0x30, 0xe9, 0x49, 0x3e, 0xe8, 
		0x7f, 0xb6, 0x87, 0xc4, 0x1c, 0xc3, 0x9c, 0x45, 
		0x04, 0x6f, 0x00, 0x00, 0xd0, 0x7f, 0x0b, 0xb6, 
		0xfe, 0x77, 0x09, 0x99, 0x61, 0x1a, 0x67, 0x91, 
		0x40, 0xeb, 0x63, 0x5d, 0xc7, 0x57, 0x06, 0x47, 
		0x3f, 0x60, 0x31, 0x3b, 0x47, 0xa9, 0x38, 0xc5, 
		0x8a, 0x2b, 0xa6, 0xe7, 0x1e, 0x47, 0xcb, 0xf0, 
		0x45, 0x22, 0xd6, 0xab, 0x46, 0x40, 0x61, 0x5e, 
		0x70, 0x69, 0xed, 0xcf, 0xe4, 0x92, 0x60, 0x1e, 
		0xec, 0x0a, 0xc0, 0xf2, 0x02, 0x6b, 0x84, 0x13, 
		0xe6, 0x0e, 0xae, 0x27, 0xf8, 0x25, 0xdb, 0xd6, 
		0x07, 0x74, 0x88, 0x9d, 0x27, 0xec, 0x54, 0x33, 
		0xd0, 0xa3, 0x6d, 0xeb, 0xa2, 0xf2, 0xdb, 0x6c, 
		0x81, 0x43, 0xa5, 0x77, 0x55, 0x01, 0x01, 0x95, 
		0xa3, 0x7e, 0x56, 0x22, 0x1a, 0x9e, 0x00, 0x00, 
		0x10, 0xb3, 0xfb, 0x62, 0x00, 0x36, 0x6e, 0x47, 
		0x68, 0xcc, 0x41, 0x6d, 0x63, 0x7a, 0xc7, 0x6d, 
		0x06, 0x97, 0xf7, 0x8e, 0x14, 0x37, 0x5d, 0xa5, 
		0x2f, 0xe2, 0x86, 0x13, 0xac, 0xfd, 0x20, 0x60, 
		0xde, 0x8b, 0x3b, 0x24, 0xbb, 0x63, 0xb1, 0x7a, 
		0x70, 0xde, 0x20, 0xd5, 0x2f, 0xe7, 0x58, 0x91, 
		0xe9, 0xfc, 0x81, 0x77, 0x82, 0x6a, 0x86, 0xf2 
};

static int dword_12B8D44 = 0;
static int dword_12B8D48 = 0;
static int dword_12B8D4C = 0;
static int dword_12B8D50 = 0;
static int dword_12B8D54 = 0;
static int dword_12B8D58 = 0;
static int dword_12B8D5C = 0;
static int dword_12B8D60 = 0;
static int dword_12B8D64 = 0;
static int dword_12B8D68 = 0;
static int dword_12B8D6C = 0;
static int dword_69D500 = 0;

static void get_time(int *a1, int *a2, int *a3, int *a4, 
					 int *a5, int *a6, int *a7, int *a8)
{
	struct _SYSTEMTIME SystemTime;

	GetLocalTime(&SystemTime);
	if (a1)
		*a1 = (WORD)SystemTime.wYear;
	if (a2)
		*a2 = SystemTime.wMonth;
	if (a3)
		*a3 = SystemTime.wDay;
	if (a4)
		*a4 = SystemTime.wDayOfWeek;
	if (a5)
		*a5 = SystemTime.wHour;
	if (a6)
		*a6 = SystemTime.wMinute;
	if (a7)
		*a7 = SystemTime.wSecond;
	if (a8)
		*a8 = SystemTime.wMilliseconds;
}


static int gen_key(int seed)
{
	int result; // eax@3
	signed int v2; // eax@4
	int v6; // esi@4
	int v7; // eax@5
	int v10; // [sp+18h] [bp-4h]@18
	int v11; // [sp+14h] [bp-8h]@18
	int v12; // [sp+10h] [bp-Ch]@18

	if (seed == 2) {
		if (dword_12B8D58 == dword_69D500) {
			dword_12B8D44 += 10000;
			dword_12B8D48 += 1000;
			dword_12B8D4C += 100;
			++dword_12B8D54;
			++dword_12B8D50;
      		v6 = dword_12B8D54 + dword_12B8D50 + dword_12B8D4C + dword_12B8D48 + dword_12B8D44;
			v2 = v6 + dword_12B8D6C;

      		if (dword_12B8D54 & 1)
        		v7 = 3 * (v2 >> 2);
      		else
        		v7 = 11 * (v2 >> 1);
      		dword_12B8D6C = result = v6 + v7;
      		
			if (dword_12B8D54 > 100)
        		dword_12B8D54 = dword_69D500 & 0xF;
      		if (dword_12B8D50 > 100)
        		dword_12B8D50 = dword_69D500 & 0xF;
      		if (dword_12B8D4C > 1000)
        		dword_12B8D4C = (BYTE)dword_69D500;
      		if (dword_12B8D48 > 10000)
        		dword_12B8D48 = dword_69D500 & 0xFFF;
      		if (dword_12B8D44 > 100000)
        		dword_12B8D44 = (WORD)dword_69D500;
    	} else {
			dword_12B8D58 = dword_69D500;
			dword_12B8D48 = dword_69D500 & 0xFFF;
			dword_12B8D44 = (WORD)dword_69D500;
			result = (dword_69D500 & 0xFFF) + 2 * (((WORD)dword_69D500 + dword_12B8D6C) >> 1)
				+ (((WORD)dword_69D500 + dword_12B8D6C) >> 1);
			dword_12B8D54 = dword_69D500 & 0xF;
			dword_12B8D50 = dword_69D500 & 0xF;
			dword_12B8D6C = (dword_69D500 & 0xFFF) + 2 * (((WORD)dword_69D500 + dword_12B8D6C) >> 1)
				+ (((WORD)dword_69D500 + dword_12B8D6C) >> 1);
			dword_12B8D4C = (BYTE)dword_69D500;
		}
	} else {
		if (seed) {
  			if (seed == 1) {
				get_time(&v10, &v11, &v12, &seed, &dword_12B8D68, &dword_12B8D64, &dword_12B8D60, &dword_12B8D5C);
				result = (dword_12B8D6C + dword_12B8D5C + dword_12B8D60 + dword_12B8D64 + dword_12B8D68 + dword_12B8D5C % 33) << 8;
				dword_12B8D6C = result;
			} else {
				result = dword_12B8D6C;
			}
		} else {
      		get_time(&v10, &v11, &v12, &seed, &dword_12B8D68, &dword_12B8D64, &dword_12B8D60, &dword_12B8D5C);
dword_12B8D5C=0x2ee;
dword_12B8D60=0x33;
dword_12B8D64=0x16;
dword_12B8D68=0x17;
			result = dword_12B8D5C + dword_12B8D60 + dword_12B8D64 + dword_12B8D68 + dword_12B8D5C % 33 
      			+ ((dword_12B8D5C + dword_12B8D60 + dword_12B8D64 + dword_12B8D68 + dword_12B8D5C % 33
				+ ((dword_12B8D5C + dword_12B8D60 + dword_12B8D64 + dword_12B8D68 + dword_12B8D5C % 33
				+ ((dword_12B8D6C + dword_12B8D5C + dword_12B8D60 + dword_12B8D64 + dword_12B8D5C % 33 + dword_12B8D68) << 8)) << 8)) << 8);
			dword_12B8D6C = result;
		}
	}
	return result;
}

/********************* pak *********************/

/* 封包匹配回调函数 */
static int KineticNovel_pak_match(struct package *pkg)
{
	if (lstrcmpi(pkg->name, L"kineticdata.pak"))
		return -CUI_EMATCH;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

#if 0
/* 封包索引目录提取函数 */
static int KineticNovel_pak_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	u32 index_length;
	if (pkg->pio->read(pkg, &index_length, 4))
		return -CUI_EREAD;

	int key;	
	gen_key(0);

	printf("%x %x %x %x %x\n", 
		dword_12B8D5C, dword_12B8D60, dword_12B8D64, 
		dword_12B8D68, dword_12B8D6C);

	key = gen_key(2);
	int dword_09241B4 = key % 0x1FFFFFF - 0x1000000;
	int dword_11D93D0 = key % 0x1FFFFFF - 0x1000000;
	printf("%x %x %x %x %x\n", 
		dword_12B8D5C, dword_12B8D60, dword_12B8D64, 
		dword_12B8D68, dword_12B8D6C);

	key = gen_key(2);
	int dword_69D500 = key % 0x1FFFFFF - 0x1000000;
	int dword_633AF4 = key % 0x1FFFFFF - 0x1000000;
	printf("%x %x %x %x %x\n", 
		dword_12B8D5C, dword_12B8D60, dword_12B8D64, 
		dword_12B8D68, dword_12B8D6C);


	key = gen_key(2);
	BYTE *buffer0 = new BYTE[key % 0x34 + 8];
	if (!buffer0)
		return -CUI_EMEM;

	printf("%x %x %x %x %x\n", 
		dword_12B8D5C, dword_12B8D60, dword_12B8D64, 
		dword_12B8D68, dword_12B8D6C);
	exit(0);

	key = gen_key(2);
	BYTE *buffer1 = new BYTE[key % 0x31 + 32];
	if (!buffer1) {
		delete [] buffer0;
		return -CUI_EMEM;
	}

	key = gen_key(2);
	BYTE *buffer2 = new BYTE[(key & 0xF) + 4];
	if (!buffer2) {
		delete [] buffer1;
		delete [] buffer0;
		return -CUI_EMEM;
	}

	key = gen_key(2);
	BYTE *buffer3 = new BYTE[key % 0x39 + 4];
	if (!buffer3) {
		delete [] buffer2;
		delete [] buffer1;
		delete [] buffer0;
		return -CUI_EMEM;
	}

	key = gen_key(2);
	BYTE *buffer4 = new BYTE[key % 0x2B + 12];
	if (!buffer4) {
		delete [] buffer3;
		delete [] buffer2;
		delete [] buffer1;
		delete [] buffer0;
		return -CUI_EMEM;
	}

	DWORD buffer5;

	// "P_a_C__K"
	BYTE *buffer6 = new BYTE[index_length];
	if (!buffer6) {
		delete [] buffer4;
		delete [] buffer3;
		delete [] buffer2;
		delete [] buffer1;
		delete [] buffer0;
		return -CUI_EMEM;
	}
	if (pkg->pio->read(pkg, buffer6, index_length)) {
		delete [] buffer6;
		delete [] buffer4;
		delete [] buffer3;
		delete [] buffer2;
		delete [] buffer1;
		delete [] buffer0;
		return -CUI_EREAD;
	}

	key = gen_key(2);
	BYTE *buffer7 = new BYTE[(key & 0xF) + 32];
	if (!buffer7) {
		delete [] buffer6;
		delete [] buffer4;
		delete [] buffer3;
		delete [] buffer2;
		delete [] buffer1;
		delete [] buffer0;
		return -CUI_EMEM;
	}

	key = gen_key(2);
	BYTE *buffer8 = new BYTE[(key & 7) + 4];
	if (!buffer8) {
		delete [] buffer6;
		delete [] buffer4;
		delete [] buffer3;
		delete [] buffer2;
		delete [] buffer1;
		delete [] buffer0;
		return -CUI_EMEM;
	}

	key = gen_key(2);
	BYTE *buffer9 = new BYTE[(key & 0xF) + 4];
	if (!buffer9) {
		delete [] buffer6;
		delete [] buffer4;
		delete [] buffer3;
		delete [] buffer2;
		delete [] buffer1;
		delete [] buffer0;
		return -CUI_EMEM;
	}

	BYTE *buffer10 = new BYTE[0x1c];

	BYTE byte_11D6DF3 = buffer6[80];
#if 0
	BYTE *enc = buffer6 + 84;
	v28 = *(int *)&buffer6[64];
	v70 = a3 + 4;	// a3 = 8, 0x27
	for (int i = 0; i < *(int *)&buffer6[64]; ++i) {
		int v9 = (i + 9) % v70;
		if (v9 < a3)
			*enc = *(_BYTE *)(v9 + a2) ^ *enc;
		++enc;
	}
#endif



	exit(0);

	dat_entry_t *index_buffer;
	unsigned int index_buffer_length;	
	unsigned int i;

	if (pkg->pio->read(pkg, &pkg_dir->index_entries, 4))
		return -CUI_EREAD;

	index_buffer_length = pkg_dir->index_entries * sizeof(dat_entry_t);
	index_buffer = new dat_entry_t[pkg_dir->index_entries];
	if (!index_buffer)
		return -CUI_EMEM;

	dat_entry_t *index = index_buffer;
	for (i = 0; i < pkg_dir->index_entries; i++) {
		if (pkg->pio->read(pkg, &index->name_length, 4))
			break;

		if (pkg->pio->read(pkg, index->name, index->name_length))
			break;
		index->name[index->name_length] = 0;

		if (pkg->pio->read(pkg, &index->offset, 4))
			break;

		if (pkg->pio->read(pkg, &index->length, 4))
			break;

		index++;
	}
	if (i != pkg_dir->index_entries) {
		delete [] index_buffer;
		return -CUI_EREAD;
	}

	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(dat_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}
#endif

/* 封包索引目录提取函数 */
static int KineticNovel_pak_extract_directory(struct package *pkg,
											  struct package_directory *pkg_dir)
{
	u32 P_a_C__K_length;
	if (pkg->pio->read(pkg, &P_a_C__K_length, 4))
		return -CUI_EREAD;

	// "P_a_C__K"
	BYTE *P_a_C__K = new BYTE[P_a_C__K_length];
	if (!P_a_C__K)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, P_a_C__K, P_a_C__K_length)) {
		delete [] P_a_C__K;
		return -CUI_EREAD;
	}

	BYTE *enc = P_a_C__K + 84;
	DWORD enc_len = *(DWORD *)&P_a_C__K[64];
	DWORD lim = 0x27 + 4;
	for (DWORD i = 0; i < enc_len; ++i) {
		DWORD tmp = (i + 9) % lim;
		if (tmp < 0x27)
			*enc = decode_table[tmp] ^ *enc;
		++enc;
	}

	return 0;
}

/* 封包索引项解析函数 */
static int KineticNovel_pak_parse_resource_info(struct package *pkg,
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
static int KineticNovel_pak_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	BYTE *raw = new BYTE[pkg_res->raw_data_length];
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			delete [] raw;
			return -CUI_EREADVEC;
	}

	// .dst
	if (!strncmp((char *)raw, "OggS", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".ogg");
	} else if (!strncmp((char *)raw, "RIFF", 4)) {// .dse
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".wav");
	}

	pkg_res->raw_data = raw;

	return 0;
}

/* 资源保存函数 */
static int KineticNovel_pak_save_resource(struct resource *res, 
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
static void KineticNovel_pak_release_resource(struct package *pkg, 
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
static void KineticNovel_pak_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation KineticNovel_pak_operation = {
	KineticNovel_pak_match,					/* match */
	KineticNovel_pak_extract_directory,		/* extract_directory */
	KineticNovel_pak_parse_resource_info,	/* parse_resource_info */
	KineticNovel_pak_extract_resource,		/* extract_resource */
	KineticNovel_pak_save_resource,			/* save_resource */
	KineticNovel_pak_release_resource,		/* release_resource */
	KineticNovel_pak_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK KineticNovel_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".pak"), NULL, 
		NULL, &KineticNovel_pak_operation, CUI_EXT_FLAG_PKG 
		| CUI_EXT_FLAG_WEAK_MAGIC | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
