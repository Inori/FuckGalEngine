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
struct acui_information GLib2_cui_information = {
	_T("ＲＵＮＥ"),			/* copyright */
	NULL,					/* system */
	_T(".g2 .tac"),				/* package */
	_T("0.0.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2007-3-24 10:12"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[20];		// "GlibArchiveData2.0" or "GlibArchiveData2.1"
	u8 key[4][16];		// [14]
	u32 index_offset;	// [54]
	u32 index_length;	// [58]
} g2_header_t;

typedef struct {
	s8 magic[4];		// "CDBD"
	u32 index_entries;
	u32 name_index_length;
	u32 length2;
} g2_index_header_t;
#pragma pack ()

// gen_key(init_key) = 0x741864eb
static BYTE init_key[16] = {
	0x9B, 0xB4, 0x65, 0x84, 0x7B, 0x9A, 0x61, 0x4D, 
	0xAD, 0xC6, 0x65, 0x73, 0xA7, 0x70, 0xFD, 0x7C
};

// gen_key(init_key) = 0x8C2087EE
static BYTE tac_init_key[16] = {
	0x08, 0x80, 0x5D, 0x9D, 0x0C, 0xFF, 0xDA, 0x43, 
	0x93, 0xCB, 0xE8, 0xC3, 0x5B, 0xEA, 0xDB, 0x49
};

static inline long good_rand(register long x)
{
	/*
	 * Compute x = (7^5 * x) mod (2^31 - 1)
	 * wihout overflowing 31 bits:
	 *      (2^31 - 1) = 127773 * (7^5) + 2836
	 * From "Random number generators: good ones are hard to find",
	 * Park and Miller, Communications of the ACM, vol. 31, no. 10,
	 * October 1988, p. 1195.
	 */
	register long hi, lo;

	hi = x / 127773;
	lo = x % 127773;
	x = 16807 * lo - 2836 * hi;
	if (x <= 0)
		x += 0x7fffffff;
	return (x);
}


static void decrypt(BYTE *dec_buf, BYTE *enc_buf, DWORD enc_len)
{
	DWORD step_tbl[4] = { 0, 2, 1, 3 };

	for (DWORD i = 0; i < enc_len; i++) {
		BYTE tmp = enc_buf[(i & ~3) - (i & 3) + 3] ^ (BYTE)i;
		dec_buf[step_tbl[i & 3] + (i & ~3)] = (tmp >> (i & 7)) | (tmp << (8 - (i & 7)));
	}
}

static void decrypt2(BYTE *dec_buf, BYTE *enc_buf, DWORD enc_len)
{
	DWORD act_enc_len;
	DWORD step_tbl[2][4] = { 
		{ 1, 0, 3, 2 },
		{ 3, 2, 1, 0 }
	};

	act_enc_len = enc_len;
	enc_len &= ~3;
	for (DWORD i = 0; i < enc_len; i++)
		dec_buf[step_tbl[1][i & 3] + (i & ~3)] = enc_buf[step_tbl[0][i & 3] + (i & ~3)] + 100;

	act_enc_len -= enc_len;
	for (i = 0; i < act_enc_len; i++)
		dec_buf[enc_len + i] = enc_buf[enc_len + i] + 100;
}

static void decrypt3(BYTE *dec_buf, BYTE *enc_buf, DWORD enc_len)
{
	DWORD act_enc_len;
	DWORD step_tbl[2][4] = { 
		{ 2, 1, 3, 0 },
		{ 3, 0, 2, 1 }
	};

	act_enc_len = enc_len;
	enc_len &= ~3;
	for (DWORD i = 0; i < enc_len; i++)
		dec_buf[step_tbl[1][i & 3] + (i & ~3)] = (enc_buf[step_tbl[0][i & 3] + (i & ~3)] + (BYTE)i) ^ (BYTE)i;

	for (; i < act_enc_len; i++)
		dec_buf[i] = (enc_buf[i] + (BYTE)i) ^ (BYTE)i;
}

static void decrypt4(BYTE *dec_buf, BYTE *enc_buf, DWORD enc_len)
{
	DWORD act_enc_len;
	DWORD step_tbl[2][4] = { 
		{ 0, 2, 1, 3 },
		{ 1, 0, 3, 2 }
	};
	BYTE tmp;

	act_enc_len = enc_len;
	enc_len &= ~3;
	for (DWORD i = 0; i < enc_len; i++) {
		tmp = ~enc_buf[step_tbl[0][i & 3] + (i & ~3)];
		dec_buf[step_tbl[1][i & 3] + (i & ~3)] = (tmp >> (i & 7)) | (tmp << (8 - (i & 7)));
	}

	for (i = enc_len; i < act_enc_len; i++) {
		tmp = ~enc_buf[i];
		dec_buf[i] = (tmp >> (i & 7)) | (tmp << (8 - (i & 7)));
	}
}

static void decrypt5(BYTE *dec_buf, BYTE *enc_buf, DWORD enc_len)
{
	DWORD act_enc_len;
	DWORD step_tbl[2][4] = { 
		{ 3, 2, 1, 0 },
		{ 2, 1, 3, 0 }
	};

	act_enc_len = enc_len;
	enc_len &= ~3;
	for (DWORD i = 0; i < enc_len; i++)
		dec_buf[step_tbl[1][i & 3] + (i & ~3)] = enc_buf[step_tbl[0][i & 3] + (i & ~3)] + 100;

	act_enc_len -= enc_len;
	for (i = 0; i < act_enc_len; i++)
		dec_buf[enc_len + i] = enc_buf[enc_len + i] + 100;
}

// 5e00a0
static void tac_decrypt(BYTE *dec_buf, BYTE *enc_buf, DWORD enc_len)
{
	DWORD act_enc_len;
	DWORD step_tbl[2][4] = { 
		{ 3, 2, 0, 1 },	// 864d04
		{ 3, 1, 2, 0 }	// 861184
	};

	act_enc_len = enc_len;
	enc_len &= ~3;
	for (DWORD i = 0; i < enc_len; ++i) {		
		BYTE tmp = enc_buf[step_tbl[0][i & 3] + (i & ~3)];
		tmp = i & 1 ? tmp + (BYTE)i : tmp - (BYTE)i;
		tmp = (BYTE)(i & 1) ^ tmp;
		dec_buf[step_tbl[1][i & 3] + (i & ~3)] = (((tmp >> 1) ^ (tmp << 1)) & 0x55) ^ (tmp << 1);
	}
		
	act_enc_len -= enc_len;
	for (i = 0; i < act_enc_len; i++) {
		BYTE tmp = enc_buf[enc_len + i];
		tmp = i & 1 ? tmp + (BYTE)i : tmp - (BYTE)i;
		tmp = (BYTE)(i & 1) ^ tmp;
		dec_buf[enc_len + i] = (((tmp >> 1) ^ (tmp << 1)) & 0x55) ^ (tmp << 1);
	}
}

static u32 gen_key(BYTE *key)
{
	return *(u32 *)key + *(u16 *)&key[6] + key[11] + key[15] + ((key[10] + key[14]
		+ ((*(u16 *)&key[4] + key[9] + key[13]
		+ ((key[8] + key[12]) << 8)) << 8)) << 8);
}

#if 0
typedef struct {
	u32 ?;
	u32 ?;
	u32 parent;		// 父节点索引号(-1表示根节点)
	u32 child;		// 左叶节点索引号
	u32 ?;
	u32 ?;
} g2_index_entry_node_t;
{
	
	for (DWORD i = 0; i < index_entries; i++) {
		if (entry->right == -1) {
			if (entry->left == -1)
				name_pointer_buffer[i] = edi;
			else {
				alloc_new_edi();
			}			
		}

	}

}
#endif

/********************* g2 *********************/

/* 封包匹配回调函数 */
static int GLib2_g2_match(struct package *pkg)
{
	BYTE enc_g2_header[0x5c];
	g2_header_t g2_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, enc_g2_header, sizeof(enc_g2_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	decrypt((BYTE *)&g2_header, enc_g2_header, sizeof(enc_g2_header));

	if (strncmp(g2_header.magic, "GLibArchiveData2.1", 20)
			&& strncmp(g2_header.magic, "GLibArchiveData2.0", 20)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int GLib2_g2_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	BYTE enc_g2_header[0x5c];
	g2_header_t g2_header;
//	unsigned int index_buffer_length;
	BYTE *index_buffer[2];
//	int key, a, b;

	if (pkg->pio->readvec(pkg, enc_g2_header, sizeof(enc_g2_header), 0, IO_SEEK_SET))
		return -CUI_EREAD;

	decrypt((BYTE *)&g2_header, enc_g2_header, sizeof(enc_g2_header));
MySaveFile(_T("g2_header"), &g2_header, sizeof(enc_g2_header));
	index_buffer[0] = (BYTE *)malloc(g2_header.index_length * 2);
	if (!index_buffer[0])
		return -CUI_EMEM;

	index_buffer[1] = index_buffer[0] + g2_header.index_length;

	if (pkg->pio->readvec(pkg, index_buffer[0], g2_header.index_length, 
			g2_header.index_offset, IO_SEEK_SET)) {
		free(index_buffer[0]);
		return -CUI_EREADVEC;
	}

	decrypt2(index_buffer[1], index_buffer[0], g2_header.index_length);
	decrypt3(index_buffer[0], index_buffer[1], g2_header.index_length);
	decrypt4(index_buffer[1], index_buffer[0], g2_header.index_length);
	decrypt5(index_buffer[0], index_buffer[1], g2_header.index_length);
MySaveFile(_T("index"), index_buffer[0], g2_header.index_length);
exit(0);


#if 0

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
		free(index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(dat_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;
#endif
	return 0;
}

/* 封包索引项解析函数 */
static int GLib2_g2_parse_resource_info(struct package *pkg,
									struct package_resource *pkg_res)
{
#if 0
	dat_entry_t *dat_entry;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	dat_entry = (dat_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, dat_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = dat_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = dat_entry->offset;
#endif
	return 0;
}

/* 封包资源提取函数 */
static int GLib2_g2_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
#if 0	
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	pkg_res->raw_data = malloc(pkg_res->raw_data_length);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EREADVEC;
	}
#endif
	return 0;
}

/* 资源保存函数 */
static int GLib2_g2_save_resource(struct resource *res, 
									struct package_resource *pkg_res)
{
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (pkg_res->raw_data && pkg_res->raw_data_length) {
		if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
			res->rio->close(res);
			return -CUI_EWRITE;
		}
	}

	res->rio->close(res);

	return 0;
}

/* 封包资源释放函数 */
static void GLib2_g2_release_resource(struct package *pkg, 
										struct package_resource *pkg_res)
{
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

/* 封包卸载函数 */
static void GLib2_g2_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation GLib2_g2_operation = {
	GLib2_g2_match,					/* match */
	GLib2_g2_extract_directory,		/* extract_directory */
	GLib2_g2_parse_resource_info,	/* parse_resource_info */
	GLib2_g2_extract_resource,		/* extract_resource */
	GLib2_g2_save_resource,			/* save_resource */
	GLib2_g2_release_resource,		/* release_resource */
	GLib2_g2_release				/* release */
};

/********************* tac *********************/

/* 封包匹配回调函数 */
static int GLib2_tac_match(struct package *pkg)
{
	BYTE enc_g2_header[0x5c];
	g2_header_t g2_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, enc_g2_header, sizeof(enc_g2_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	tac_decrypt((BYTE *)&g2_header, enc_g2_header, sizeof(enc_g2_header));
printf(g2_header.magic);
	if (strncmp(g2_header.magic, "GLibArchiveData2.1", 20)
			&& strncmp(g2_header.magic, "GLibArchiveData2.0", 20)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int GLib2_tac_extract_directory(struct package *pkg,
									   struct package_directory *pkg_dir)
{
	BYTE enc_g2_header[0x5c];
	g2_header_t g2_header;
//	unsigned int index_buffer_length;
	BYTE *index_buffer[2];
//	int key, a, b;

	if (pkg->pio->readvec(pkg, enc_g2_header, sizeof(enc_g2_header), 0, IO_SEEK_SET))
		return -CUI_EREAD;

	tac_decrypt((BYTE *)&g2_header, enc_g2_header, sizeof(enc_g2_header));
MySaveFile(_T("g2_header"), &g2_header, sizeof(enc_g2_header));
	index_buffer[0] = (BYTE *)malloc(g2_header.index_length * 2);
	if (!index_buffer[0])
		return -CUI_EMEM;

	index_buffer[1] = index_buffer[0] + g2_header.index_length;

	if (pkg->pio->readvec(pkg, index_buffer[0], g2_header.index_length, 
			g2_header.index_offset, IO_SEEK_SET)) {
		free(index_buffer[0]);
		return -CUI_EREADVEC;
	}

	printf("%x\n", gen_key((BYTE *)g2_header.key[3]));

	decrypt2(index_buffer[1], index_buffer[0], g2_header.index_length);
	decrypt3(index_buffer[0], index_buffer[1], g2_header.index_length);
	decrypt4(index_buffer[1], index_buffer[0], g2_header.index_length);
	decrypt5(index_buffer[0], index_buffer[1], g2_header.index_length);
MySaveFile(_T("index"), index_buffer[0], g2_header.index_length);
exit(0);

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation GLib2_tac_operation = {
	GLib2_tac_match,				/* match */
	GLib2_tac_extract_directory,	/* extract_directory */
	GLib2_g2_parse_resource_info,	/* parse_resource_info */
	GLib2_g2_extract_resource,		/* extract_resource */
	GLib2_g2_save_resource,			/* save_resource */
	GLib2_g2_release_resource,		/* release_resource */
	GLib2_g2_release				/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK GLib2_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".g2"), NULL, 
		NULL, &GLib2_g2_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".tac"), NULL, 
		NULL, &GLib2_tac_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	int key = 0x45a58e6b;//0x8C2087EE
	int seed = key ^ 0xdeadbeef;
	int seed = seed % 0x1f31d * 16807 - seed / 0x1f31d * 2836;
	if (seed < 0)
		seed += 0x7fffffff;
	return (double)seed / 2147483647D;

	printf("%x %x\n", seed % 0x1f31d,seed / 0x1f31d);
printf("%x\n",
	   

	   seed % 0x1f31d * 16807 - seed / 0x1f31d * 2836
	   );
	return 0;
}
