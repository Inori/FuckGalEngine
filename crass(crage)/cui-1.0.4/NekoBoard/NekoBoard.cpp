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
struct acui_information NekoBoard_cui_information = {
	NULL,					/* copyright */
	NULL,					/* system */
	_T(".dat"),				/* package */
	_T("1.1.3"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-7-30 19:29"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[8];		// "NEKOPACK"
	u32 package_id;		// 靠大小区分各个封包
	u32 dec_init_key;
	u32 dec_exec_key;
	u64 index_length;
} dat_header_t;

typedef struct {
	u32 dir_name_hash;
	u32 dir_entries;
	u32 dir_entries0;
} dat_dir_t;

typedef struct {
	u32 dir_name_hash;
	u32 dir_entries;
} new_dat_dir_t;

typedef struct {
	u32 name_hash;
	u32 length;
} dat_entry_t;

typedef struct {
	s8 magic[8];		/* "NEKOPACK" */
	u32 sync;			/* 0xcb or 0xcc */
	u32 unknown;		
	u32 index_entries;
} dat_old_header_t;

typedef struct {
	u8 sync;			/* 0x47 */
	u8 file_name_len;
	s8 *file_name;
	u32 offset;
	u32 length;
} dat_old_entry_t;

typedef struct {
	s8 magic[8];		// "NEKOPACK"
	u32 seed;
	u32 unknown;
	u32 parity;			// parity
	u32 index_length;
} dat_new_header_t;

typedef struct {
	s8 magic[8];		/* "NEKOPACK" */
	u32 unknown;
	u32 key;		
} dat_header2_t;
#pragma pack ()

typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 length;
	u32 sync;	// for old dat
} my_dat_entry_t;

static DWORD parity_check(DWORD key0, DWORD key1)
{
	DWORD tmp = (key1 ^ ((key0 ^ key1) + 0x5D588B65)) - 0x359D3E2A;
	DWORD v = (key1 ^ ((key1 ^ tmp) - 0x70E44324)) + 0x6C078965;
	return v << (tmp >> 27) | v >> (32 - (tmp >> 27));
}

static void gen_key(DWORD &ret_key0, DWORD &ret_key1, DWORD seed)
{
	DWORD tmp = seed ^ (seed + 0x5D588B65);
	DWORD tmp2 = tmp ^ (seed - 0x359D3E2A);
	ret_key0 = tmp2 ^ (tmp - 0x70E44324);
	ret_key1 = ret_key0 ^ (tmp2 + 0x6C078965);
}

static void decode(BYTE *buf, DWORD len, u16 *key)
{
	for (DWORD i = 0; i < len / 2; ++i) {
		*(u16 *)buf ^= key[i & 3];
		key[i & 3] += *(u16 *)buf;
		buf += 2;
	}
}


#define SWAP4(x)	(((x) & 0xff) << 24 | ((x) & 0xff00) << 8 | ((x) & 0xff0000) >> 8 | ((x) & 0xff000000) >> 24)

static char *dir_name_table[] = {
	"image/actor", "image/back", "sound/se", "voice", "sound/bgm", "script", "image/visual", "system", "count", NULL
};

static BYTE name_table[256] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0xc9, 0xca, 0x00, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0x00, 0xd2, 0xd3, 0x27, 0x25, 0xc8, 
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x00, 0xd4, 0x00, 0xd5, 0x00, 0x00, 
		0xd6, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 
		0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0xd7, 0xc8, 0xd8, 0xd9, 0x26, 
		0xda, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 
		0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0xdb, 0x00, 0xdc, 0xdd, 0x00, 
		0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 
		0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 
		0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 
		0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 
		0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 
		0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 
		0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 
		0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8 
};

static BYTE new_name_table[256] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x38, 0x2f, 0x33, 0x3c, 0x40, 0x3b, 0x2a, 0x2e, 0x31, 0x30, 0x26, 0x44, 0x35, 0x28, 0x3e, 0x12, 
		0x02, 0x22, 0x06, 0x20, 0x1a, 0x1c, 0x0f, 0x11, 0x18, 0x17, 0x42, 0x2b, 0x3a, 0x37, 0x34, 0x0c, 
		0x41, 0x08, 0x1d, 0x07, 0x15, 0x21, 0x05, 0x1e, 0x0a, 0x14, 0x0e, 0x10, 0x09, 0x27, 0x1f, 0x0b, 
		0x23, 0x16, 0x0d, 0x01, 0x25, 0x04, 0x1b, 0x03, 0x13, 0x24, 0x19, 0x2d, 0x12, 0x29, 0x32, 0x3f, 
		0x3d, 0x08, 0x1d, 0x07, 0x15, 0x21, 0x05, 0x1e, 0x0a, 0x14, 0x0e, 0x10, 0x09, 0x27, 0x1f, 0x0b, 
		0x23, 0x16, 0x0d, 0x01, 0x25, 0x04, 0x1b, 0x03, 0x13, 0x24, 0x19, 0x2c, 0x39, 0x43, 0x36, 0x00, 
		0x4b, 0xa9, 0xa7, 0xaf, 0x50, 0x52, 0x91, 0x9f, 0x47, 0x6b, 0x96, 0xab, 0x87, 0xb5, 0x9b, 0xbb, 
		0x99, 0xa4, 0xbf, 0x5c, 0xc6, 0x9c, 0xc2, 0xc4, 0xb6, 0x4f, 0xb8, 0xc1, 0x85, 0xa8, 0x51, 0x7e, 
		0x5f, 0x82, 0x73, 0xc7, 0x90, 0x4e, 0x45, 0xa5, 0x7a, 0x63, 0x70, 0xb3, 0x79, 0x83, 0x60, 0x55, 
		0x5b, 0x5e, 0x68, 0xba, 0x53, 0xa1, 0x67, 0x97, 0xac, 0x71, 0x81, 0x59, 0x64, 0x7c, 0x9d, 0xbd, 
		0x9d, 0xbd, 0x95, 0xa0, 0xb2, 0xc0, 0x6f, 0x6a, 0x54, 0xb9, 0x6d, 0x88, 0x77, 0x48, 0x5d, 0x72, 
		0x49, 0x93, 0x57, 0x65, 0xbe, 0x4a, 0x80, 0xa2, 0x5a, 0x98, 0xa6, 0x62, 0x7f, 0x84, 0x75, 0xbc, 
		0xad, 0xb1, 0x6e, 0x76, 0x8b, 0x9e, 0x8c, 0x61, 0x69, 0x8d, 0xb4, 0x78, 0xaa, 0xae, 0x8f, 0xc3, 
		0x58, 0xc5, 0x74, 0xb7, 0x8e, 0x7d, 0x89, 0x8a, 0x56, 0x4d, 0x86, 0x94, 0x9a, 0x4c, 0x92, 0xb0, 
};


static void xcode_create(u32 key, u32 *xcode, DWORD xcode_len)
{
	unsigned int eax = 0;
	unsigned int ebx = 0;

	do {
		eax <<= 1;
		ebx ^= 1;
		eax = ((eax | ebx) << (key & 1)) | ebx;
		key >>= 1;
	} while (!(eax & 0x80000000));
	key = eax << 1;
	eax = key + SWAP4(key);
	BYTE cl = (BYTE)key;
	do {
		ebx = key ^ eax;
		eax = (ebx << 4) ^ (ebx >> 4) ^ (ebx << 3) ^ (ebx >> 3) ^ ebx;
		--cl;
	} while (cl);

	for (DWORD i = 0; i < 616 / 4; i++) {
		ebx = key ^ eax;
		eax = (ebx << 4) ^ (ebx >> 4) ^ (ebx << 3) ^ (ebx >> 3) ^ ebx;
		xcode[i] = eax;
	}
}

static void xcode_init(u32 key, BYTE *xcode, BYTE flag)
{
	int v4;
	BYTE byte_532310[14] = { 0xEF, 0xFC, 0xFD, 0xFE, 0xF8, 0xF9, 0xFA, 0xEF, 0xF8, 0xF9, 0xFA, 0xFC, 0xFD, 0xFE };
	BYTE byte_532324[6] = { 0xCA, 0xD3, 0xDC, 0xE5, 0xEE, 0xF1 };

	xcode[0] = 0x01;
	xcode[1] = 0xF1;
	xcode[2] = 0x0F;
	xcode[3] = 0x6F;
	xcode[4] = 0x06;
	xcode += 5;
	int v15 = key & 0xffff;
	int v16 = (key >> 16) & 0xfff;
	if (flag)
		v4 = 0;
	else
		v4 = 7;
	unsigned int v17 = v4 + (key >> 28);
	for (DWORD i = 0; i < 4; i++) {
		BYTE v6;
		int v11, v12;

		if (flag)
			v6 = (BYTE)i;
		else
			v6 = (BYTE)(3 - i);
		v11 = v15 >> (4 * v6);
		v12 = v16 >> (3 * v6);
		xcode[0] = 15;
		xcode[1] = byte_532310[(v11 + v17) % 0xE];
		xcode[2] = v12 % 6 - 63;
		xcode += 3;
	}
	xcode[0] = 0x0F;
	xcode[1] = 0x7F;
	xcode[2] = 0x07;
	xcode[3] = 0x83;
	xcode[4] = 0xC6;
	xcode[5] = 0x08;
	xcode[6] = 0x83;
	xcode[7] = 0xC7;
	xcode[8] = 0x08;
	xcode += 9;
	for (i = 0; i < 6; i++) {
		xcode[0] = 15;
		xcode[1] = -44;
		xcode[2] = byte_532324[(i + key) % 6];
		xcode += 3;
	}
	xcode[0] = 0x39;
	xcode[1] = 0xCE;
	xcode[2] = 0x72;
	xcode[3] = 0xD2;
	xcode[4] = 0xC3;
}

static void xcode_execute(u32 key, BYTE *xcode_base, BYTE *dst, BYTE *src, SIZE_T len)
{
	u64 mm_reg[6];
	void (*xcode_start)(void) = (void (*)(void))(xcode_base + xcode_base[0] + 320);

	for (DWORD i = 0; i < 6; i++) {
		mm_reg[i] = *(u64 *)&xcode_base[(key % 0x28) * 8];
		key /= 0x28; 
	}

	__asm {
		movq    mm1, qword ptr mm_reg[0]
		movq    mm2, qword ptr mm_reg[8]
		movq    mm3, qword ptr mm_reg[16]
		movq    mm4, qword ptr mm_reg[24]
		movq    mm5, qword ptr mm_reg[32]
		movq    mm6, qword ptr mm_reg[40]
		mov		esi, src
		mov		edi, dst
		mov		ecx, len
	}

	/*
		debug045:01AD01B5 add     ecx, esi
		debug045:01AD01B7 loc_1AD01B7:
		debug045:01AD01B7 movq    mm0, qword ptr [esi]
		debug045:01AD01BA psubd   mm0, mm1
		debug045:01AD01BD pxor    mm0, mm4
		debug045:01AD01C0 paddw   mm0, mm1
		debug045:01AD01C3 paddd   mm0, mm2
		debug045:01AD01C6 movq    qword ptr [edi], mm0
		debug045:01AD01C9 add     esi, 8
		debug045:01AD01CC add     edi, 8
		debug045:01AD01CF paddq   mm6, mm1
		debug045:01AD01D2 paddq   mm1, mm2
		debug045:01AD01D5 paddq   mm2, mm3
		debug045:01AD01D8 paddq   mm3, mm4
		debug045:01AD01DB paddq   mm4, mm5
		debug045:01AD01DE paddq   mm5, mm6
		debug045:01AD01E1 cmp     esi, ecx
		debug045:01AD01E3 jb      short loc_1AD01B7
		debug045:01AD01E5 retn
	*/
    (*xcode_start)();

	__asm { emms }
}

static BYTE *xcode = NULL;

static void decrypt_release(void)
{
	if (xcode) {
		VirtualFree(xcode, 0, MEM_RELEASE);
		xcode = NULL;
	}
}

static int decrypt_init(u32 init_key)
{
	DWORD old_protection;

	xcode = (BYTE *)VirtualAlloc(NULL, 616, MEM_COMMIT, PAGE_READWRITE);
	if (!xcode)
		return -CUI_EMEM;

	xcode_create(init_key, (u32 *)xcode, 616);
	xcode_init(init_key, xcode + xcode[0] + 320, 0);
	if (!VirtualProtect(xcode, 616, PAGE_EXECUTE_READ, &old_protection)) {
		decrypt_release();
		return -CUI_EMATCH;
	}

	return 0;
}

static void decrypt(u32 exec_key, BYTE *dec, BYTE *enc, DWORD len)
{
	if (xcode)
		xcode_execute(exec_key, xcode, dec, enc, len);
	else
		memcpy(dec, enc, len);
}

// for exp: get_hash(0x99bdf6f6@<"system.dat">, "script") -> 0xf8288e7c
static u32 get_hash(u32 hash, char *name)
{
	while (1) {
		BYTE chr = *name++;
		if (!chr)
			break;
		hash = 16777258 * (name_table[chr] ^ hash);
	}
	return hash;
}

static u32 get_new_hash(u32 hash, char *name)
{
	while (1) {
		BYTE chr = *name++;
		if (!chr)
			break;
		hash = 81 * (new_name_table[chr] ^ hash);
	}
	return hash;
}


static DWORD index_decode_table[0x270];
static DWORD index_decode_table_count;
static DWORD data_decode_table[0x270];
static DWORD data_decode_table_count;

static void init_decode_table(DWORD *decode_table, unsigned int decode_table_len, DWORD key, DWORD *code_table_count)
{
	unsigned int dtlen = decode_table_len / 4;

	for (unsigned int i = 0; i < dtlen; i++) {
		key = (key * 0x10dcd) & 0xffffffff;
		decode_table[i] = key;
		//printf("%08x ", key);
	}
	*code_table_count = 0;
}

static BYTE	decode(BYTE src, DWORD *decode_table, DWORD decode_table_len, DWORD *code_table_count)
{
	DWORD val;

	if (*code_table_count == decode_table_len / 4)
		init_decode_table(decode_table, decode_table_len, decode_table[*code_table_count - 1], code_table_count);

	val = decode_table[(*code_table_count)++];
	val ^= (val >> 11);
	val ^= (val << 7) & 0x31518a63;
	val ^= 	(val << 15) & 0x17f1ca43;
	val ^= (val >> 18);
	return (BYTE)((val & 0xff) ^ src);
}

/********************* old dat *********************/

/* 封包匹配回调函数 */
static int NekoBoard_old_dat_match(struct package *pkg)
{
	dat_old_header_t header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &header, sizeof(header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(header.magic, "NEKOPACK", sizeof(header.magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}
	
	if (header.sync != 0xcb && header.sync != 0xcc) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}
	
	return 0;
}

/* 封包索引目录提取函数 */
static int NekoBoard_old_dat_extract_directory(struct package *pkg,
											  struct package_directory *pkg_dir)
{
	dat_old_header_t header;

	if (pkg->pio->readvec(pkg, &header, sizeof(header), 0, IO_SEEK_SET)) 
		return -CUI_EREADVEC;
	
	DWORD index_buffer_len = sizeof(my_dat_entry_t) * header.index_entries;
	my_dat_entry_t *index_buffer = (my_dat_entry_t *)malloc(index_buffer_len);
	if (!index_buffer)
		return -CUI_EMEM;
	
	init_decode_table(index_decode_table, sizeof(index_decode_table), 0x9999, &index_decode_table_count);
	
	for (DWORD i = 0; i < header.index_entries; ++i) {
		my_dat_entry_t &entry = index_buffer[i];
		u8 sync;
		if (pkg->pio->read(pkg, &sync, 1)) {
			free(index_buffer);
			return -CUI_EREAD;
		}
		entry.sync = sync;

		u8 name_len;
		if (pkg->pio->read(pkg, &name_len, 1)) {
			free(index_buffer);
			return -CUI_EREAD;
		}
		
		if (pkg->pio->read(pkg, entry.name, name_len)) {
			free(index_buffer);
			return -CUI_EREAD;
		}
		entry.name[name_len] = 0;
		for (BYTE n = 0; n < name_len; ++n)
			entry.name[n] = decode(entry.name[n], index_decode_table, sizeof(index_decode_table), &index_decode_table_count);
		
		if (pkg->pio->read(pkg, &entry.offset, 4)) {
			free(index_buffer);
			return -CUI_EREAD;
		}
		
		if (pkg->pio->read(pkg, &entry.length, 4)) {
			free(index_buffer);
			return -CUI_EREAD;
		}
	}

	pkg_dir->index_entries = header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_len;
	pkg_dir->index_entry_length = sizeof(my_dat_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int NekoBoard_old_dat_parse_resource_info(struct package *pkg,
												struct package_resource *pkg_res)
{
	my_dat_entry_t *my_dat_entry;

	my_dat_entry = (my_dat_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_dat_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_dat_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_dat_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int NekoBoard_old_dat_extract_resource(struct package *pkg,
											 struct package_resource *pkg_res)
{
	BYTE *raw = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length,
			pkg_res->offset, IO_SEEK_SET)) {
		free(raw);
		return -CUI_EREADVEC;
	}
	
	my_dat_entry_t *my_dat_entry = (my_dat_entry_t *)pkg_res->actual_index_entry;
	if (my_dat_entry->sync) {
		init_decode_table(data_decode_table, sizeof(data_decode_table), my_dat_entry->sync + 0x9999, &data_decode_table_count);
		for (DWORD i = 0; i < pkg_res->raw_data_length; ++i)
			raw[i] = decode(raw[i], data_decode_table, sizeof(data_decode_table), &data_decode_table_count);
	}

	pkg_res->raw_data = raw;

	return 0;
}

/* 资源保存函数 */
static int NekoBoard_old_dat_save_resource(struct resource *res, 
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
static void NekoBoard_old_dat_release_resource(struct package *pkg, 
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
static void NekoBoard_old_dat_release(struct package *pkg, 
									 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation NekoBoard_old_dat_operation = {
	NekoBoard_old_dat_match,					/* match */
	NekoBoard_old_dat_extract_directory,		/* extract_directory */
	NekoBoard_old_dat_parse_resource_info,	/* parse_resource_info */
	NekoBoard_old_dat_extract_resource,		/* extract_resource */
	NekoBoard_old_dat_save_resource,			/* save_resource */
	NekoBoard_old_dat_release_resource,		/* release_resource */
	NekoBoard_old_dat_release				/* release */
};
	
/********************* dat *********************/

static int NekoBoard_dat_extract_directory(struct package *pkg,
										  struct package_directory *pkg_dir);

/* 封包匹配回调函数 */
static int NekoBoard_dat_match(struct package *pkg)
{
	s8 magic[8];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "NEKOPACK", sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	struct package_directory pkg_dir;
	memset(&pkg_dir, 0, sizeof(pkg_dir));
	int ret = NekoBoard_dat_extract_directory(pkg, &pkg_dir);
	if (ret) {
		pkg->pio->close(pkg);
		return ret;
	}
	free(pkg_dir.directory);
	
	return 0;	
}

/* 封包索引目录提取函数 */
static int NekoBoard_dat_extract_directory(struct package *pkg,
										  struct package_directory *pkg_dir)
{
	dat_header_t dat_header;
	u32 index_length[2];

	if (pkg->pio->readvec(pkg, &dat_header, sizeof(dat_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	int ret = decrypt_init(dat_header.dec_init_key);
	if (ret)
		return ret;	

	decrypt(dat_header.dec_exec_key, (BYTE *)&index_length, (BYTE *)&dat_header.index_length, 8);
	if (index_length[0] != index_length[1]) {		
		decrypt_release();
		return -CUI_EMATCH;
	}

	DWORD index_len = (index_length[0] + 7) & ~7;
	BYTE *enc_index = (BYTE *)malloc(index_len);
	if (!enc_index) {
		decrypt_release();
		return -CUI_EMEM;
	}

	if (pkg->pio->read(pkg, enc_index, index_len)) {
		free(enc_index);
		decrypt_release();
		return -CUI_EREAD;
	}

	BYTE *dec_index = (BYTE *)VirtualAlloc(NULL, (index_length[0] + 64) & 0xFFFFFFBF, MEM_COMMIT, PAGE_READWRITE);
	if (!dec_index) {
		free(enc_index);
		decrypt_release();
		return -CUI_EMEM;
	}

	decrypt(dat_header.dec_exec_key, dec_index, enc_index, index_len);
	free(enc_index);	

	DWORD dirs = 0;
	DWORD index_entries = 0;
	for (BYTE *p = dec_index; p < dec_index + index_len; ) {
		dat_dir_t *dir = (dat_dir_t *)p;

		if (dir->dir_entries != dir->dir_entries0)
			break;

		index_entries += dir->dir_entries;
		dirs++;
		p += sizeof(dat_dir_t) + dir->dir_entries * sizeof(dat_entry_t);
	}
	if (p < dec_index + index_len) {
		VirtualFree(dec_index, 0, MEM_RELEASE);
		decrypt_release();
		return -CUI_EMATCH;
	}

	DWORD index_buffer_length = index_entries * sizeof(my_dat_entry_t);
	my_dat_entry_t *index_buffer = (my_dat_entry_t *)malloc(index_buffer_length);
	if (!index_buffer) {
		VirtualFree(dec_index, 0, MEM_RELEASE);
		decrypt_release();
		return -CUI_EMEM;
	}

	p = dec_index;
	DWORD k = 0;
	DWORD offset = sizeof(dat_header_t) + index_len;
	for (DWORD d = 0; d < dirs; d++) {
		dat_dir_t *dir = (dat_dir_t *)p;
		const char *dir_name = NULL;

		p += sizeof(dat_dir_t);

		for (DWORD n = 0; dir_name_table[n]; n++) {
			if (get_hash(dat_header.dec_init_key, dir_name_table[n]) == dir->dir_name_hash) {
				dir_name = dir_name_table[n];
				break;
			}
		}

		for (DWORD i = 0; i < dir->dir_entries; i++) {
			dat_entry_t *entry = (dat_entry_t *)p;

			if (dir_name)
				sprintf(index_buffer[k].name, "%s/%08x", dir_name, entry->name_hash);
			else
				sprintf(index_buffer[k].name, "%08x/%08x", dir->dir_name_hash, entry->name_hash);
			index_buffer[k].length = entry->length;
			index_buffer[k++].offset = offset;
			offset += entry->length;
			p += sizeof(dat_entry_t);
		}
	}
	VirtualFree(dec_index, 0, MEM_RELEASE);

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(my_dat_entry_t);

	return 0;
}

/* 封包资源提取函数 */
static int NekoBoard_dat_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	u32 dec_exec_key;
	u64 enc_data_length;
	u32 dec_data_length[2];
	BYTE *raw;	
	my_dat_entry_t *my_dat_entry;

	raw = (BYTE *)pkg->pio->readvec_only(pkg, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET);
	if (!raw)
		return -CUI_EREADVECONLY;

	dec_exec_key = *(u32 *)raw;
	enc_data_length = *(u64 *)(raw + 4);
	decrypt(dec_exec_key, (BYTE *)&dec_data_length, (BYTE *)&enc_data_length, 8);
	if (dec_data_length[0] != dec_data_length[1])
		return -CUI_EMATCH;

	BYTE *act = (BYTE *)malloc((dec_data_length[0] + 7) & ~7);
	if (!act)
		return -CUI_EMEM;

	decrypt(dec_exec_key, act, raw + 12, (dec_data_length[0] + 7) & ~7);

	my_dat_entry = (my_dat_entry_t *)pkg_res->actual_index_entry;
	if (!strncmp((char *)act, "OggS", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".ogg");
	} else if (!strncmp((char *)act, "\x89PNG", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".png");
	} else if (!strncmp((char *)act, "\x8aMNG", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".mng");
	} else if (!strncmp(pkg_res->name, "script", 6)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".txt");
	}

	pkg_res->raw_data = raw;
	pkg_res->actual_data = act;
	pkg_res->actual_data_length = dec_data_length[0];

	return 0;
}

/* 封包卸载函数 */
static void NekoBoard_dat_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	decrypt_release();
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation NekoBoard_dat_operation = {
	NekoBoard_dat_match,						/* match */
	NekoBoard_dat_extract_directory,			/* extract_directory */
	NekoBoard_old_dat_parse_resource_info,	/* parse_resource_info */
	NekoBoard_dat_extract_resource,			/* extract_resource */
	NekoBoard_old_dat_save_resource,			/* save_resource */
	NekoBoard_old_dat_release_resource,		/* release_resource */
	NekoBoard_dat_release					/* release */
};

/********************* new dat *********************/

/* 封包匹配回调函数 */
static int NekoBoard_new_dat_match(struct package *pkg)
{
	dat_new_header_t header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &header, sizeof(header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(header.magic, "NEKOPACK", sizeof(header.magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int NekoBoard_new_dat_extract_directory(struct package *pkg,
											  struct package_directory *pkg_dir)
{
	dat_new_header_t header;
	if (pkg->pio->readvec(pkg, &header, sizeof(header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	BYTE *index = (BYTE *)malloc((header.index_length + 63) & ~63);
	if (!index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index, header.index_length)) {
		free(index);
		return -CUI_EREAD;
	}

	DWORD key[2];
	gen_key(key[0], key[1], header.parity);
	decode(index, header.index_length, (u16 *)key);

	DWORD dirs = 0;
	DWORD index_entries = 0;
	for (BYTE *p = index; p < index + header.index_length; ) {
		new_dat_dir_t *dir = (new_dat_dir_t *)p;

		index_entries += dir->dir_entries;
		dirs++;
		p += sizeof(new_dat_dir_t) + dir->dir_entries * sizeof(dat_entry_t);
	}

	DWORD index_buffer_length = index_entries * sizeof(my_dat_entry_t);
	my_dat_entry_t *index_buffer = (my_dat_entry_t *)malloc(index_buffer_length);
	if (!index_buffer) {
		free(index);
		return -CUI_EMEM;
	}

	p = index;
	DWORD k = 0;
	DWORD offset = sizeof(dat_new_header_t) + header.index_length;
	for (DWORD d = 0; d < dirs; d++) {
		new_dat_dir_t *dir = (new_dat_dir_t *)p;
		const char *dir_name = NULL;

		p += sizeof(new_dat_dir_t);

		for (DWORD n = 0; dir_name_table[n]; n++) {
			if (get_new_hash(header.seed, dir_name_table[n]) == dir->dir_name_hash) {
				dir_name = dir_name_table[n];
				break;
			}
		}

		for (DWORD i = 0; i < dir->dir_entries; i++) {
			dat_entry_t *entry = (dat_entry_t *)p;

			if (dir_name)
				sprintf(index_buffer[k].name, "%s/%08x", dir_name, entry->name_hash);
			else
				sprintf(index_buffer[k].name, "%08x/%08x", dir->dir_name_hash, entry->name_hash);
			index_buffer[k].length = entry->length;
			index_buffer[k++].offset = offset;
			offset += entry->length + 8;
			p += sizeof(dat_entry_t);
		}
	}

	free(index);

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(my_dat_entry_t);
	package_set_private(pkg, header.seed);

	return 0;
}

/* 封包资源提取函数 */
static int NekoBoard_new_dat_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	u32 parity;
	if (pkg->pio->readvec(pkg, &parity, 4, pkg_res->offset, IO_SEEK_SET))
		return -CUI_EREADVEC;

	u32 enc_len;
	if (pkg->pio->readvec(pkg, &enc_len, 4, pkg_res->offset + 4, IO_SEEK_SET))
		return -CUI_EREADVEC;

	// 针对单字节对齐的文件，避免末字节没有被正确解密
	BYTE *raw = (BYTE *)malloc(pkg_res->raw_data_length + 1);
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length, pkg_res->offset + 8, IO_SEEK_SET)) {
		free(raw);
		return -CUI_EREADVEC;
	}

	u32 seed = (u32)package_get_private(pkg);
	if (parity_check(seed, enc_len) != parity) {
		free(raw);
		return -CUI_EMATCH;	
	}

	DWORD key[2];
	gen_key(key[0], key[1], parity);
	decode(raw, enc_len + 1, (u16 *)key);

	if (!strncmp((char *)raw, "OggS", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".ogg");
	} else if (!strncmp((char *)raw, "\x89PNG", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".png");
	} else if (!strncmp((char *)raw, "\x8aMNG", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".mng");
	} else if (!strncmp(pkg_res->name, "script", 6)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".txt");
	}

	pkg_res->raw_data = raw;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation NekoBoard_new_dat_operation = {
	NekoBoard_new_dat_match,				/* match */
	NekoBoard_new_dat_extract_directory,	/* extract_directory */
	NekoBoard_old_dat_parse_resource_info,	/* parse_resource_info */
	NekoBoard_new_dat_extract_resource,		/* extract_resource */
	NekoBoard_old_dat_save_resource,		/* save_resource */
	NekoBoard_old_dat_release_resource,		/* release_resource */
	NekoBoard_dat_release					/* release */
};

/********************* dat2 *********************/

/* 封包匹配回调函数 */
static int NekoBoard_dat2_match(struct package *pkg)
{
	dat_new_header_t header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &header, sizeof(header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(header.magic, "NEKOPACK", sizeof(header.magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int NekoBoard_dat2_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{
	dat_header2_t header;
	if (pkg->pio->readvec(pkg, &header, sizeof(header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	u8 enc[1024];

	if (pkg->pio->read(pkg, enc, sizeof(enc)))
		return -CUI_EREAD;

	DWORD cnt = (header.key % 7) + 3;
	for (DWORD j = 0; j < cnt; ++j) {
		u32 key = header.key;
		for (DWORD i = 0; i < 256; ++i) {
			key += 0xeb0974c3;
			key <<= 23;
			key >>= 23;
			u32 idx = key;
			key += ((u32 *)enc)[i];
			((u32 *)enc)[i] ^= *(u32 *)&enc[idx];
		}
	}

	MySaveFile(_T("sdf"), enc, 1024);exit(0);

#if 0
	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(my_dat_entry_t);
	package_set_private(pkg, header.seed);
#endif
	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation NekoBoard_dat2_operation = {
	NekoBoard_dat2_match,					/* match */
	NekoBoard_dat2_extract_directory,		/* extract_directory */
	NekoBoard_old_dat_parse_resource_info,	/* parse_resource_info */
	NekoBoard_new_dat_extract_resource,		/* extract_resource */
	NekoBoard_old_dat_save_resource,			/* save_resource */
	NekoBoard_old_dat_release_resource,		/* release_resource */
	NekoBoard_dat_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK NekoBoard_register_cui(struct cui_register_callback *callback)
{

	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &NekoBoard_old_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &NekoBoard_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &NekoBoard_new_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;
#if 0
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &NekoBoard_dat2_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;
#endif
	return 0;
}
