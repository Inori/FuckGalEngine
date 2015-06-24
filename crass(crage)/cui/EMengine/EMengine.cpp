#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <utility.h>
#include <stdio.h>

struct acui_information EMengine_cui_information = {
	NULL,					/* copyright */
	_T("ＥＭエンジン"),		/* system */
	_T(".eme .rre .ecf .rcf emesys.dat rresys.dat"),	/* package */
	_T("0.9.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-11-2 11:04"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};	

#pragma pack (1)
typedef struct {
	s8 magic[8];			/* "RREDATA " */
} eme_header_t;

typedef struct {
	s8 magic[16];			/* "EMESYS_VER1.X0\x0"，X可以是0（0x64==100),1(0x6e==110)或2(0x78==120) */
	u8 decode_data[40];		/* 前8字节是加密模式字段，后面32字节是解密数据 */
	u32 config_data_length;	/* 配置数据信息 */
} ecf_header_t;

/* SAVEDATA\emesys.dat or rresys.dat
 * 格式如下：
 * [0](4)total eme package count
 * [4](388)pkg0_info
 *		[](96)res0_entry
 *			[]()res0_private_data
 *		[](96)res1_entry
 *			[]()res1_private_data	
 *		...
 *		[](96)resN_entry
 *			[]()resN_private_data	
 * [](388)pkg1_info
 *		[](96)res0_entry
 *			[]()res0_private_data
 *		[](96)res1_entry
 *			[]()res1_private_data	
 *		...
 *		[](96)resN_entry
 *			[]()resN_private_data
 * ...
 * [](388)pkgN_info
 *		[](96)res0_entry
 *			[]()res0_private_data
 *		[](96)res1_entry
 *			[]()res1_private_data	
 *		...
 *		[](96)resN_entry
 *			[]()resN_private_data
 * 接下来的数据是一些字符串，暂不去理会它怎么来的
 */
typedef struct {
	s8 magic[16];			/* "EMESYS_VER1.20\x0" */
	u32 decode_data[10];	/* 前8字节是加密模式字段，后面32字节是解密数据 */
} dat_header_t;

/*
script: window_size 1000 forward_length 12 type 1 op 3
cg: window_size 1000 forward_length 12 type 10 op 4
music: window_size 0 forward_length 0 type 20400000 op 0
se & voice: window_size 0 forward_length 0 type 400000 op 0
*/
/*
01037C50  6D 61 73 6B 5F 30 30 2E 62 6D 70 00 00 00 00 00  mask_00.bmp.....
01037C60  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
01037C70  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
01037C80  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
01037C90  00 10 12 00 10 00 00 00 04 00 00 00 75 4B 02 00  ........uK.
01037CA0  00 F9 15 00 08 00 00 00 00 00 00 00 00 00 00 00  .?............
*/
typedef struct {
	s8 name[64];
	u16 window_size;	/* lzss解压时的窗口大小. 非压缩时为0 */
	u16 forward_length;	/* lzss解压时的前置缓冲区长度. 非压缩时为0 */
/*
bit01 - 是否需要cdrom检测（推测release版光盘上的封包都设置了该位或者init_package参数设置了该位，所以破除cdrom检测的方法一个是改exe，一个是免除所有封包的该位定义，体验版无需cdrom检测正是因为这点）
bit29 - 万用类型或强制类型匹配位
*/
	u32 type;			/* (从实际来看，该字段为0表示无效的资源）资源属性 cg: 0x00000010 se & voice: 0x00400000 music: 0x20400000 sc: 0x00000001 */
	u32 op;				/* 资源类型：3, 4, 5 and other */	
	u32 comprlen;	
	u32 uncomprlen;
	u32 offset;
	u32 ppackage_info;	/* 0，运行时参数，指向所属封包的pkg_info数据结构 */
	u32 pprivate_para;	/* 0，运行时参数，指向和自己的资源类型相关的数据结构 */
} eme_entry_t;

typedef struct {
	u16 bpp;
	u16 width;
	u16 height;
	u16 colors;			/* 如果非0且小于3，则强制等于3 */
	u32 pitch;			/* 4字节对齐 */
	u32 alpha_orig_x;	/* alpha区域描述 */
	u32 alpha_orig_y;
	u32 alpha_width;
	u32 alpha_height;
	u32 alpha_pattern;		/* 0x0000ff00 */
} cg_header_t;

typedef struct {	// 压缩的数据意义不明
	u32 comprlen;
	u32 uncomprlen;
	u32 unknown;	// 0
} sc_header_t;
#pragma pack ()

typedef struct {
	s8 name[64];
	u32 length;
	u32 offset;
} ecf_entry_t;

static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static int init_decode_table(u8 *decode_buffer, u32 decode_buffer_len, u32 code)
{
	u8 *decode_table;
	u32 offset = 0;

	decode_table = (BYTE *)malloc(decode_buffer_len);
	if (!decode_table)
		return -1;

	for (unsigned int i = 0; i < decode_buffer_len; i++) {
		offset += code;
		while (offset >= decode_buffer_len)
			offset -= decode_buffer_len;
		decode_table[offset] = decode_buffer[i];
	}
	memcpy(decode_buffer, decode_table, decode_buffer_len);
	free(decode_table);

	return 0;
}

static void xor_dword(u32 *decode_buffer, u32 dword_count, u32 code)
{
	for (unsigned int i = 0; i < dword_count; i++)
		decode_buffer[i] ^= code;
}

static void xor_mask_dword(u32 *decode_buffer, u32 dword_count, u32 code)
{
	u32 mask = code;
	
	for (unsigned int i = 0; i < dword_count; i++) {
		u32 tmp = decode_buffer[i];
		decode_buffer[i] ^= mask;
		mask = tmp;
	}
}

static void shift_decode(u32 *decode_buffer, u32 dword_count, u32 code)
{

	for (unsigned int i = 0; i < dword_count; i++) {
		unsigned int tmp = 0, val = 0;

		for (unsigned int shift = 0; shift < 32; shift++) {
			tmp += code;
			if (tmp >= 32)
				tmp -= 32;
			if (decode_buffer[i] & (1 << shift))
				val |= 1 << tmp;
		}
		decode_buffer[i] = val;
	}
}

static void decode(u8 *decode_header, u8 *decode_buffer, u32 decode_buffer_len)
{
	u32 *code_table = (u32 *)&decode_header[0x24];
	
	for (int i = 7; i >= 0; i--) {
		u8 flag = decode_header[i];
		u32 code = code_table[i - 7];

		switch (flag) {
		case 1:
			if (decode_buffer_len >= 4)
				xor_dword((u32 *)decode_buffer, decode_buffer_len >> 2, code);
			break;
		case 2:
			if (decode_buffer_len >= 4)
				xor_mask_dword((u32 *)decode_buffer, decode_buffer_len >> 2, code);
			break;
		case 4:
			if (decode_buffer_len >= 4)
				shift_decode((u32 *)decode_buffer, decode_buffer_len >> 2, code);	
			break;
		case 8:
			init_decode_table(decode_buffer, decode_buffer_len, code);	
			break;
		default:
			break;
		}
	}
}
DWORD act_len2;
DWORD act_len;
static int lzss_uncompress(unsigned char *uncompr, unsigned int uncomprlen, unsigned char *compr, unsigned int comprlen, unsigned short win_size, unsigned short forward)
{
	unsigned int act_uncomprlen = 0;
	unsigned int curbyte = 0;		/* compr中的当前扫描字节 */
	unsigned int nCurWindowByte = win_size - forward;
	unsigned char *win;
	int ret = 0;
	unsigned short flag = 0;
	BYTE data;

	win = (unsigned char *)malloc(win_size);
	if (!win)
		return -CUI_EMEM;	
	memset(win, 0, nCurWindowByte);

	while (1) {
		flag >>= 1;
		if (!(flag & 0x0100)) {
			if (curbyte >= comprlen)
				break;
			flag = compr[curbyte++] | 0xff00;
		}

		if (flag & 1) {
			if (curbyte >= comprlen)
              break;
            data = compr[curbyte++];
			/* 输出1字节非压缩数据 */
			uncompr[act_uncomprlen++] = data;
			/* 输出的1字节放入滑动窗口 */
			win[nCurWindowByte++] = data;
			nCurWindowByte &= win_size - 1;
		} else {
			unsigned short win_offset, copy_bytes;
			
			if (curbyte >= comprlen)
            	break;
			win_offset = compr[curbyte++];
			
          	if (curbyte >= comprlen)
            	break;
			copy_bytes = compr[curbyte++];
			
			win_offset |= (copy_bytes & 0xf0) << 4;
			copy_bytes &= 0x0f;
			copy_bytes += 3;	

			for (unsigned int i = 0; i < copy_bytes; i++) {
				data = win[(win_offset + i) & (win_size - 1)];
				/* 输出1字节非压缩数据 */
				uncompr[act_uncomprlen++] = data;		
				/* 输出的1字节放入滑动窗口 */
				win[nCurWindowByte++] = data;
				nCurWindowByte &= win_size - 1;		
			}
		}
	}
	free(win);
act_len2=curbyte;
act_len=act_uncomprlen;
	return 0;	
}

/********************* ecf *********************/

static int EMengine_ecf_match(struct package *pkg)
{
	ecf_header_t *ecf_header;
	
	ecf_header = (ecf_header_t *)malloc(sizeof(ecf_header_t));
	if (!ecf_header)
		return -CUI_EMEM;

	if (pkg->pio->open(pkg, IO_READONLY)) {
		free(ecf_header);
		return -CUI_EOPEN;
	}

	if (pkg->pio->read(pkg, ecf_header, sizeof(*ecf_header))) {
		pkg->pio->close(pkg);
		free(ecf_header);
		return -CUI_EREAD;
	}

	if (strncmp(ecf_header->magic, "EMESYS_VER", 10) && strncmp(ecf_header->magic, "RRESYS_VER", 10)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}
	
	if (strncmp(&ecf_header->magic[10], "1.00", 5) && strncmp(&ecf_header->magic[10], "1.10", 5) 
			&& strncmp(&ecf_header->magic[10], "1.20", 5)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}	
	package_set_private(pkg, ecf_header);
	
	return 0;	
}

static int EMengine_ecf_extract_directory(struct package *pkg,
										  struct package_directory *pkg_dir)
{
	ecf_header_t *ecf_header;
	BYTE *ecf_config_data;
	ecf_entry_t *index_buffer;
	DWORD index_length;
	
	ecf_header = (ecf_header_t *)package_get_private(pkg);
	
	xor_mask_dword((u32 *)ecf_header->decode_data, 11, 0xdc38eb76);
	
	ecf_config_data = (BYTE *)malloc(ecf_header->config_data_length);
	if (!ecf_config_data)
		return -CUI_EMEM;
	
	if (pkg->pio->read(pkg, ecf_config_data, ecf_header->config_data_length)) {
		free(ecf_config_data);
		return -CUI_EREAD;
	}

	decode((BYTE *)ecf_header->decode_data, ecf_config_data, ecf_header->config_data_length);
	free(ecf_config_data);

	if (pkg->pio->read(pkg, &pkg_dir->index_entries, 4))
		return -CUI_EREAD;
	
	index_length = pkg_dir->index_entries * sizeof(ecf_entry_t);
	index_buffer = (ecf_entry_t *)malloc(index_length);
	if (!index_buffer)
		return -CUI_EMEM;
	
	DWORD offset = sizeof(ecf_header_t) + ecf_header->config_data_length + 4;
	for (DWORD i = 0; i < pkg_dir->index_entries; i++) {
		if (pkg->pio->readvec(pkg, &index_buffer[i], 68, offset, IO_SEEK_SET))
			break;
		
		decode((BYTE *)ecf_header->decode_data, (BYTE *)&index_buffer[i], 68);
		offset += 68;
		index_buffer[i].offset = offset;
		offset += index_buffer[i].length;
	}
	if (i != pkg_dir->index_entries) {
		free(index_buffer);
		return -CUI_EREADVEC;
	}

	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_length;
	pkg_dir->index_entry_length = sizeof(ecf_entry_t);

	return 0;
}

static int EMengine_ecf_parse_resource_info(struct package *pkg,
											struct package_resource *pkg_res)
{
	ecf_entry_t *ecf_entry;

	ecf_entry = (ecf_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, ecf_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = ecf_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = ecf_entry->offset;

	return 0;
}

static int EMengine_ecf_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	ecf_header_t *ecf_header;
	BYTE *data;

	data = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, data, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET)) {
		free(data);
		return -CUI_EREADVEC;
	}
	
	ecf_header = (ecf_header_t *)package_get_private(pkg);	
	decode((BYTE *)ecf_header->decode_data, data, pkg_res->raw_data_length);
	pkg_res->raw_data = data;
	
	return 0;
}

static int EMengine_ecf_save_resource(struct resource *res, 
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

static void EMengine_ecf_release_resource(struct package *pkg, 
										  struct package_resource *pkg_res)
{
	if (pkg_res && pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res && pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void EMengine_ecf_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	ecf_header_t *ecf_header;
	
	if (pkg_dir->directory && pkg_dir->directory_length) {
		free(pkg_dir->directory);
		pkg_dir->directory_length = 0;
	}
	pkg->pio->close(pkg);
	ecf_header = (ecf_header_t *)package_get_private(pkg);
	if (ecf_header) {
		free(ecf_header);
		package_set_private(pkg, NULL);
	}
}

static cui_ext_operation EMengine_ecf_operation = {
	EMengine_ecf_match,					/* match */
	EMengine_ecf_extract_directory,		/* extract_directory */
	EMengine_ecf_parse_resource_info,	/* parse_resource_info */
	EMengine_ecf_extract_resource,		/* extract_resource */
	EMengine_ecf_save_resource,			/* save_resource */
	EMengine_ecf_release_resource,		/* release_resource */
	EMengine_ecf_release				/* release */
};

/********************* emesys.dat / rresys.dat *********************/

static int EMengine_dat_match(struct package *pkg)
{
	s8 magic[16];

	if (lstrcmp(pkg->name, _T("emesys.dat")) && lstrcmp(pkg->name, _T("rresys.dat")))
		return -CUI_EMATCH;
	
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "EMESYS_VER1.20", 16) && strncmp(magic, "RRESYS_VER1.20", 16)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}
	
	return 0;	
}

static int EMengine_dat_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	BYTE decode_data[40];
	BYTE *dat;
	u32 dat_size;

	if (pkg->pio->read(pkg, decode_data, 40))
		return -CUI_EREAD;

	if (pkg->pio->length_of(pkg, &dat_size))
		return -CUI_ELEN;

	dat_size -= sizeof(dat_header_t);
	dat = (BYTE *)malloc(dat_size);
	if (!dat)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, dat, dat_size)) {
		free(dat);
		return -CUI_EREAD;
	}

	decode(decode_data, dat, dat_size);

	pkg_res->raw_data = dat;
	pkg_res->raw_data_length = dat_size;
	pkg_res->actual_data = NULL;
	pkg_res->actual_data_length = 0;

	return 0;
}

static void EMengine_dat_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

static cui_ext_operation EMengine_dat_operation = {
	EMengine_dat_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	EMengine_dat_extract_resource,	/* extract_resource */
	EMengine_ecf_save_resource,		/* save_resource */
	EMengine_ecf_release_resource,	/* release_resource */
	EMengine_dat_release			/* release */
};

/********************* eme *********************/

static int EMengine_eme_match(struct package *pkg)
{
	s8 magic[8];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "RREDATA ", 8)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int EMengine_eme_extract_directory(struct package *pkg,
										  struct package_directory *pkg_dir)
{
	eme_entry_t *index_buffer;
	DWORD i;
	BYTE *decode_data;
	DWORD index_entries, index_length;
	sc_header_t sc_header;
	cg_header_t cg_header;

	if (pkg->pio->readvec(pkg, &index_entries, 4, -4, IO_SEEK_END))
		return -CUI_EREADVEC;
	
	index_length = index_entries * sizeof(eme_entry_t);
	decode_data = (BYTE *)malloc(40);
	if (!decode_data)
		return -CUI_EMEM;
	
	if (pkg->pio->readvec(pkg, decode_data, 40, -4 - index_length - 40, IO_SEEK_END)) {
		free(decode_data);
		return -CUI_EREADVEC;
	}
	
	index_buffer = (eme_entry_t *)malloc(index_length);
	if (!index_buffer) {
		free(decode_data);
		return -CUI_EMEM;
	}
	
	if (pkg->pio->read(pkg, index_buffer, index_length)) {
		free(index_buffer);
		free(decode_data);
		return -CUI_EREAD;
	}

	for (i = 0; i < index_entries; i++) {
		decode(decode_data, (BYTE *)&index_buffer[i], sizeof(eme_entry_t));
		switch (index_buffer[i].op) {
		case 3:
			if (pkg->pio->readvec(pkg, &sc_header, sizeof(sc_header_t), index_buffer[i].offset, IO_SEEK_SET)) {
				free(index_buffer);
				free(decode_data);
				return -CUI_EREADVEC;
			}
			decode(decode_data, (BYTE *)&sc_header, sizeof(sc_header_t));
			//printf("%s: %x / %x, sc header %x / %x\n", index_buffer[i].name, index_buffer[i].comprlen, index_buffer[i].uncomprlen, 
			//	sc_header.comprlen, sc_header.uncomprlen);
			index_buffer[i].comprlen += sizeof(sc_header_t) + sc_header.comprlen;
			break;
		case 4:
			if (pkg->pio->readvec(pkg, &cg_header, sizeof(cg_header_t), index_buffer[i].offset, IO_SEEK_SET)) {
				free(index_buffer);
				free(decode_data);
				return -CUI_EREADVEC;
			}
			decode(decode_data, (BYTE *)&cg_header, sizeof(cg_header_t));

			cg_header.bpp &= 0xff;	/* fix for .rre */
#if 0
			printf("%s: %x / %x, width %d height %d bpp %d colors %d pitch %d [width0 %d height0 %d pad %x %x mask %x]\n",
				index_buffer[i].name, index_buffer[i].comprlen, index_buffer[i].uncomprlen,  
				cg_header.width, cg_header.height, cg_header.bpp, cg_header.colors, cg_header.pitch, cg_header.alpha_width, 
				cg_header.alpha_height, cg_header.alpha_orig_x, cg_header.alpha_orig_y, cg_header.alpha_mask);
#endif
			DWORD palette_size;
			if (cg_header.colors) {
				if (cg_header.colors < 3)
					cg_header.colors = 3;
				palette_size = cg_header.colors * 4;
			} else
				palette_size = 0;

			if (cg_header.bpp != 7)
				index_buffer[i].comprlen += sizeof(cg_header_t) + palette_size;
			break;
		}
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_length;
	pkg_dir->index_entry_length = sizeof(eme_entry_t);
	package_set_private(pkg, decode_data);

	return 0;
}
/*
RE0189.ogg: window_size 0 forward_length 0 type 400000 op 0 comprlen 396fc uncom
prlen 396fc reserved0 0 reserved1 0
RE0190.ogg: window_size 0 forward_length 0 type 400000 op 0 comprlen 3778e uncom
prlen 3778e reserved0 0 reserved1 0
RE0191.ogg: window_size 0 forward_length 0 type 400000 op 0 comprlen 339a uncomp
rlen 339a reserved0 0 reserved1 0
RE0192.ogg: window_size 0 forward_length 0 type 400000 op 0 comprlen 1fbc5 uncom
prlen 1fbc5 reserved0 0 reserved1 0
RE0193.ogg: window_size 0 forward_length 0 type 400000 op 0 comprlen 29dd0 uncom
prlen 29dd0 reserved0 0 reserved1 0
RE0194.ogg: window_size 0 forward_length 0 type 400000 op 0 comprlen b1e7 uncomp
rlen b1e7 reserved0 0 reserved1 0
RE0195.ogg: window_size 0 forward_length 0 type 400000 op 0 comprlen aac5 uncomp
rlen aac5 reserved0 0 reserved1 0
*/
static int EMengine_eme_parse_resource_info(struct package *pkg,
											struct package_resource *pkg_res)
{
	eme_entry_t *eme_entry;

	eme_entry = (eme_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, eme_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = eme_entry->comprlen;
	pkg_res->actual_data_length = eme_entry->uncomprlen;	/* 数据都是明文 */
	pkg_res->offset = eme_entry->offset;

	return 0;
}

static int EMengine_eme_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;
	eme_entry_t *eme_entry;
	sc_header_t *sc_header;
	BYTE *decode_data;
	cg_header_t *cg_header;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen,	pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREAD;
	}

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = compr;
		return 0;
	}
	
	eme_entry = (eme_entry_t *)pkg_res->actual_index_entry;
	decode_data = (BYTE *)package_get_private(pkg);

	switch (eme_entry->op) {
	case 3:	/* script */
		sc_header = (sc_header_t *)compr;

		decode(decode_data, (BYTE *)sc_header, sizeof(sc_header_t));
		comprlen -= sizeof(sc_header_t) + sc_header->comprlen;

		if (sc_header->uncomprlen) {	/* 丑陋的把戏 */
			BYTE *extra_uncompr;

			extra_uncompr = (BYTE *)malloc(sc_header->uncomprlen);
			if (!extra_uncompr) {
				free(compr);
				return -CUI_EMEM;
			}

			if (lzss_uncompress(extra_uncompr, sc_header->uncomprlen, (BYTE *)(sc_header + 1), sc_header->comprlen, 
					eme_entry->window_size, eme_entry->forward_length)) {
				free(extra_uncompr);
				free(compr);
				return -CUI_EUNCOMPR;
			}

			pkg_res->actual_data_length += sc_header->uncomprlen;
			uncomprlen = pkg_res->actual_data_length;
			uncompr = (BYTE *)malloc(uncomprlen);
			if (!uncompr) {
				free(extra_uncompr);
				free(compr);
				return -CUI_EMEM;
			}		
			//memset(uncompr, 0xdd, uncomprlen);	/* used for debug */
			if (lzss_uncompress(uncompr, uncomprlen, ((BYTE *)(sc_header + 1)) + sc_header->comprlen, comprlen, 
					eme_entry->window_size, eme_entry->forward_length)) {
				free(uncompr);
				free(extra_uncompr);
				free(compr);
				return -CUI_EUNCOMPR;
			}
			memcpy(uncompr + pkg_res->actual_data_length - sc_header->uncomprlen, extra_uncompr, sc_header->uncomprlen);
			free(extra_uncompr);
		} else {
			uncomprlen = pkg_res->actual_data_length;
			uncompr = (BYTE *)malloc(uncomprlen);
			if (!uncompr) {
				free(compr);
				return -CUI_EMEM;
			}		
			
			if (lzss_uncompress(uncompr, uncomprlen, ((BYTE *)(sc_header + 1)) + sc_header->comprlen, 
					comprlen, eme_entry->window_size, eme_entry->forward_length)) {
				free(uncompr);
				free(compr);
				return -CUI_EUNCOMPR;
			}
		}
		pkg_res->actual_data = uncompr;
		pkg_res->actual_data_length = uncomprlen;
		break;
	case 5:
		decode(decode_data, compr, 4);
	
		uncomprlen = pkg_res->actual_data_length;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}
		
		if (lzss_uncompress(uncompr, uncomprlen, compr + 12, comprlen - 12, eme_entry->window_size, eme_entry->forward_length)) {
			free(uncompr);
			free(compr);
			return -CUI_EUNCOMPR;
		}
		
		/* 接下来是预处理的riff_process部分，但是为啥和后处理部分不大一样？ */
		
		/* 这是后处理部分 */
		if (eme_entry->window_size) {
			if (lzss_uncompress(uncompr, uncomprlen, compr, comprlen, eme_entry->window_size, eme_entry->forward_length)) {
				free(uncompr);
				free(compr);
				return -CUI_EUNCOMPR;
			}
		} else
		    memcpy(uncompr, compr, uncomprlen);
		break;
	case 4:
		cg_header = (cg_header_t *)compr;
		decode(decode_data, (BYTE *)cg_header, sizeof(cg_header_t));

		cg_header->bpp &= 0xff;	/* fix for .rre */

		BYTE *palette;
		DWORD palette_size;
		if (cg_header->colors) {
			if (cg_header->colors < 3)
				cg_header->colors = 3;
			palette = compr + sizeof(cg_header_t);
			palette_size = cg_header->colors * 4;
		} else {
			palette = NULL;
			palette_size = 0;
		}

		uncomprlen = pkg_res->actual_data_length;
		uncompr = (BYTE *)malloc(uncomprlen + 1024 * 1024);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}

		if (eme_entry->window_size) {
			if (lzss_uncompress(uncompr, uncomprlen + 1024 * 1024, compr + sizeof(cg_header_t) + palette_size, 
					comprlen - sizeof(cg_header_t) - palette_size, eme_entry->window_size, eme_entry->forward_length)) {
				free(uncompr);
				free(compr);
				return -CUI_EUNCOMPR;
			}
		} else
		    memcpy(uncompr, compr + sizeof(cg_header_t) + palette_size, uncomprlen);

#if 0
		/* 之所以这么做，主要是描绘alpha区域的信息对每个cg都同等
		 * 存在，只能靠文件名做区分。
		 * 举例：
		 * cg301是完整的基本图 24位色
		 * cg301s??是差分图 用于和完整图做差分 24位色（比如汁液）
		 * mcg301s??是256色的cg301s??的mask图
		 * 结论证明ws的这种做法是错误的
		 */
		if (cg_header->bpp == 24) {
			char pn[MAX_PATH];
			int num;

			if (sscanf(pkg_res->name, "cg%3cs%d.bmp", pn, &num) == 2) {
				BYTE *alpha = (BYTE *)malloc(cg_header->alpha_width * cg_header->alpha_height);
				if (!alpha) {
					free(uncompr);
					free(compr);
					return -CUI_EMEM;
				}

				BYTE *p = alpha;
				// uncomprlen是行对齐长度的，且结尾多2字节
				for (int y = 0; y < cg_header->height; ++y) {
					BYTE *line = uncompr + y * cg_header->pitch;
					for (int x = 0; x < cg_header->width; ++x) {
						BYTE B = line[x * 3 + 0];
						BYTE G = line[x * 3 + 1];
						BYTE R = line[x * 3 + 2];
						DWORD pixel = B | (G << 8) | (R << 16);
						if (pixel == cg_header->alpha_pattern)
							*p++ = 0x00;
						else
							*p++ = 0xff;
					}
				}

				uncomprlen = cg_header->width * cg_header->height * 4;
				BYTE *act_dib = (BYTE *)malloc(uncomprlen);
				if (!act_dib) {
					free(alpha);
					free(uncompr);
					free(compr);
					return -CUI_EMEM;
				}

				BYTE *src = uncompr;
				BYTE *a = alpha;
				for (y = 0; y < cg_header->height; ++y) {
					BYTE *dst_line = act_dib + y * cg_header->width * 4;
					BYTE *src_line = uncompr + y * cg_header->pitch;
					for (DWORD x = 0; x < cg_header->width; ++x) {
						dst_line[x * 4 + 0] = src_line[x * 3 + 0];
						dst_line[x * 4 + 1] = src_line[x * 3 + 1];
						dst_line[x * 4 + 2] = src_line[x * 3 + 2];
						dst_line[x * 4 + 3] = *a++;	
					}
				}
				free(alpha);
				free(uncompr);
				uncompr = act_dib;
				cg_header->bpp = 32;
			}
		}
#endif
		if (cg_header->bpp != 7) {	// （有1位mask？）
			if (MySaveAsBMP(uncompr, uncomprlen, palette, palette_size, cg_header->width, cg_header->height, 
					cg_header->bpp, cg_header->colors, (BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, my_malloc)) {
				free(uncompr);
				free(compr);
				return -CUI_EMEM;
			}
			free(uncompr);
		} else {
			// fix: 用cg_header->width*cg_header->height代替uncomprlen
			if (MySaveAsBMP(uncompr, cg_header->width * cg_header->height, palette, palette_size, cg_header->width, -cg_header->height, 
					8, cg_header->colors, (BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, my_malloc)) {
				free(uncompr);
				free(compr);
				return -CUI_EMEM;
			}
			free(uncompr);
		}
		break;
	default:	/* 0 - ogg */
		pkg_res->actual_data = NULL;
		pkg_res->actual_data_length = 0;
	}

	pkg_res->raw_data = compr;

	return 0;
}

static int EMengine_eme_save_resource(struct resource *res, 
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

static void EMengine_eme_release_resource(struct package *pkg, 
										  struct package_resource *pkg_res)
{
	if (pkg_res->actual_data && pkg_res->actual_data_length) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res->raw_data && pkg_res->raw_data_length) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void EMengine_eme_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	void *priv;

	if (pkg_dir->directory && pkg_dir->directory_length) {		
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	priv = (void *)package_get_private(pkg);
	if (priv) {
		free(priv);
		package_set_private(pkg, NULL);
	}
	pkg->pio->close(pkg);
}

static cui_ext_operation EMengine_eme_operation = {
	EMengine_eme_match,					/* match */
	EMengine_eme_extract_directory,		/* extract_directory */
	EMengine_eme_parse_resource_info,	/* parse_resource_info */
	EMengine_eme_extract_resource,		/* extract_resource */
	EMengine_eme_save_resource,			/* save_resource */
	EMengine_eme_release_resource,		/* release_resource */
	EMengine_eme_release				/* release */
};

int CALLBACK EMengine_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".eme"), NULL, 
		NULL, &EMengine_eme_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;
	
	if (callback->add_extension(callback->cui, _T(".rre"), NULL, 
		NULL, &EMengine_eme_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".ecf"), _T(".txt"), 
		NULL, &EMengine_ecf_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".rcf"), _T(".txt"), 
		NULL, &EMengine_ecf_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), _T(".dat.dump"), 
		NULL, &EMengine_dat_operation, CUI_EXT_FLAG_PKG))
			return -1;

	return 0;
}
