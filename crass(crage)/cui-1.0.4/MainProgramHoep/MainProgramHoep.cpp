#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>
#include <zlib.h>
#include <utility.h>
#include <cui_common.h>

/* 挫败install.CWL的方法:
这个文件只在安装游戏时候使用。读取它的是install.exe，
这个家伙比较讨厌，它把自身复制到temp下，然后CreateProcess
产生子进程继续，然后自己就退出了，用oly这种ring3调试器抓不住。
所以改用一种方法：启动oly加载install.exe，执行过了第二条指令后，
把第二条指令mov esp,ebp改为int 3，然后另存为install1.exe。
双击Install1.exe，弹出oly，这样就进入了子进程的领空，把第二条恢复
为mov esp,ebp，同时修正eip（减1），然后就可以慢慢调试它了。
*/

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information MainProgramHoep_cui_information = {
	_T("CROWD"),			/* copyright */
	_T("Main Program Hoep"),/* system */
	_T(".sce .amw .amb .amp .pck .eog .cwp .dat .cwd .cw_ .cwl"),	/* package */
	_T("1.1.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-3-6 22:36"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	u32 index_entries;
} amw_header_t;

typedef struct {
	u32 zero;
	u32 offset;
	u32 length;
} amw_entry_t;

typedef struct {
	s8 magic[4];	// "CRM"
	u32 samples;	// samples x 4倍的大小
} eog_header_t;

typedef struct {
	s8 magic[4];	// "CRM"
	u32 samples;
} amb_header_t;

typedef struct {
	s8 magic[4];	// "AMNP"
} amp_header_t;

typedef struct {
	s8 magic[4];	// "CWDP"
} cwp_header_t;

typedef struct {
	u32 magic;	// 0x01000000
	u8 decrypt_buffer[16];
} dat_header_t;

typedef struct {
	s8 magic[36];	// "cwd format  - version 1.00 -"
	u32 unknown0;	// 0x91
	u32 unknown1;	// 0x64
	u32 width;
	u32 height;
	u32 decrypt_key;
} cwd_header_t;
#pragma pack ()

typedef struct {
	s8 name[64];
	u32 offset;
	u32 length;
} my_amw_entry_t;


static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static void sub_defiltering(BYTE *buf, DWORD width, DWORD depth)
{
	for (DWORD w = 1; w < width; w++) {
		buf += depth;
		for (DWORD p = 0; p < depth; p++)
			buf[p] += buf[p - depth];
	}
}

static void up_defiltering(BYTE *above, BYTE *buf, DWORD width, DWORD depth)
{
	if (!above)
		return;

	for (DWORD w = 0; w < width; w++) {
		for (DWORD p = 0; p < depth; p++)
			buf[p] += above[p];
		buf += depth;
		above += depth;
	}
}

static void average_defiltering(BYTE *above, BYTE *buf, DWORD width, DWORD depth)
{
	if (!above) {
		for (DWORD w = 1; w < width; w++) {
			buf += depth;
			for (DWORD p = 0; p < depth; p++)
				buf[p] += buf[p - depth] / 2;
		}
		return;
	}

	for (DWORD p = 0; p < depth; p++)
		buf[p] += above[p] / 2;

	for (DWORD w = 1; w < width; w++) {
		buf += depth;
		above += depth;
		for (DWORD p = 0; p < depth; p++)
			buf[p] += (buf[p - depth] + above[p]) / 2;
	}
}

static void paeth_defiltering(BYTE *above, BYTE *buf, DWORD width, DWORD depth)
{
	if (!above) {
		for (DWORD w = 1; w < width; w++) {
			buf += depth;
			for (DWORD d = 0; d < depth; d++)
				buf[d] += buf[d - depth];
		}
		return;
	}
	
	for (DWORD p = 0; p < depth; p++)
		buf[p] += above[p];
		
	for (DWORD w = 1; w < width; w++) {
		BYTE a, b, c;	// a = left, b = above, c = upper left
		int p;
		int pa, pb, pc;
		
		buf += depth;
		above += depth;
		for (DWORD d = 0; d < depth; d++) {
			a = buf[d - depth];
			b = above[d];
			c = above[d - depth];
			p = a + b - c;

			pa = abs(p - a);
			pb = abs(p - b);
			pc = abs(p - c);

			if (pa <= pb && pa <= pc) 
				buf[d] += a;
			else if (pb <= pc)
				buf[d] += b;
			else 
				buf[d] += c;
		}
	}
}

static BYTE decode_string[] = "crowd script yeah !";

static BYTE __decrypt(BYTE a, BYTE b)
{
	return ~(a & b) & (a | b);
}

static void decrypt(BYTE *enc_buf, DWORD enc_buf_len)
{
	DWORD ebx, esp_1c;

	ebx = esp_1c = 0;
	for (DWORD i = 0; i < enc_buf_len; i++) {
		DWORD edi;
			
		edi = (ebx + i) % 18;	
		enc_buf[i] = __decrypt(enc_buf[i], (BYTE)(decode_string[edi] | (ebx & esp_1c)));
		if (!edi)
			ebx = (DWORD)decode_string[(ebx + esp_1c++) % 18];
	}
}

static void decrypt2(BYTE *enc_buf, DWORD enc_buf_len, BYTE *dec_buf)
{
	for (DWORD i = 0, k = 0; i < enc_buf_len; i++) {
		enc_buf[i] = __decrypt(enc_buf[i], dec_buf[k++]);
		if (k == 16) {
			BYTE key = enc_buf[i - 1];
			switch (key & 7) {
			case 0:
				dec_buf[0] += key;
				dec_buf[4] = dec_buf[2] + key + 11;
				dec_buf[3] += key + 2;
				dec_buf[8] = dec_buf[6] + 7;
				break;
			case 1:
				dec_buf[2] = dec_buf[9] + dec_buf[10];
				dec_buf[6] = dec_buf[7] + dec_buf[15];
				dec_buf[8] += dec_buf[1];
				dec_buf[15] = dec_buf[3] + dec_buf[5];
				break;
			case 2:
				dec_buf[1] += dec_buf[2];
				dec_buf[5] += dec_buf[6];
				dec_buf[7] += dec_buf[8];
				dec_buf[10] += dec_buf[11];
				break;
			case 3:
				dec_buf[9] = dec_buf[1] + dec_buf[2];
				dec_buf[11] = dec_buf[5] + dec_buf[6];
				dec_buf[12] = dec_buf[7] + dec_buf[8];
				dec_buf[13] = dec_buf[10] + dec_buf[11];
				break;
			case 4:
				dec_buf[0] = dec_buf[1] + 0x6f;
				dec_buf[3] = dec_buf[4] + 0x47;
				dec_buf[4] = dec_buf[5] + 0x11;
				dec_buf[14] = dec_buf[15] + 0x40;
				break;
			case 5:
				dec_buf[2] += dec_buf[10];
				dec_buf[4] = dec_buf[5] + dec_buf[12];
				dec_buf[6] = dec_buf[8] + dec_buf[14];
				dec_buf[8] = dec_buf[0] + dec_buf[11];
				break;
			case 6:
				dec_buf[9] = dec_buf[1] + dec_buf[11];
				dec_buf[11] = dec_buf[3] + dec_buf[13];
				dec_buf[13] = dec_buf[5] + dec_buf[15];
				dec_buf[15] = dec_buf[7] + dec_buf[9];
			default:
				dec_buf[1] = dec_buf[5] + dec_buf[9];
				dec_buf[2] = dec_buf[6] + dec_buf[10];
				dec_buf[3] = dec_buf[7] + dec_buf[11];
				dec_buf[4] = dec_buf[8] + dec_buf[12];
				break;
			}
			k = 0;
		}
	}
}

/********************* dat *********************/

/* 封包匹配回调函数 */
static int MainProgramHoep_dat_match(struct package *pkg)
{
	u32 magic;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (magic != 0x01000000) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包资源提取函数 */
static int MainProgramHoep_dat_extract_resource(struct package *pkg,
												struct package_resource *pkg_res)
{
	u8 decrypt_buffer[16];
	u32 fsize;
	BYTE *data;
	DWORD data_len;

	if (pkg->pio->length_of(pkg, &fsize))
		return -CUI_ELEN;

	if (pkg->pio->readvec(pkg, decrypt_buffer, sizeof(decrypt_buffer), 4, IO_SEEK_SET))
		return -CUI_EREADVEC;

	data_len = fsize - sizeof(dat_header_t);
	data = (BYTE *)malloc(data_len);
	if (!data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, data, data_len, sizeof(dat_header_t), IO_SEEK_SET)) {
		free(data);
		return -CUI_EREADVEC;
	}

	decrypt2(data, data_len, decrypt_buffer);

	pkg_res->actual_data = data;
	pkg_res->actual_data_length = data_len;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation MainProgramHoep_dat_operation = {
	MainProgramHoep_dat_match,				/* match */
	NULL,									/* extract_directory */
	NULL,									/* parse_resource_info */
	MainProgramHoep_dat_extract_resource,	/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* sce *********************/

/* 封包匹配回调函数 */
static int MainProgramHoep_sce_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}
static int MainProgramHoep_sce_extract_resource(struct package *pkg,
									  struct package_resource *pkg_res)
{
	pkg_res->actual_data = malloc(pkg_res->raw_data_length);
	if (!pkg_res->actual_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->actual_data, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(pkg_res->actual_data);
			pkg_res->actual_data = NULL;
			return -CUI_EREADVEC;
	}

	decrypt((BYTE *)pkg_res->actual_data, pkg_res->raw_data_length);
	pkg_res->actual_data_length = pkg_res->raw_data_length;
	
	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation MainProgramHoep_sce_operation = {
	MainProgramHoep_sce_match,				/* match */
	NULL,									/* extract_directory */
	NULL,									/* parse_resource_info */
	MainProgramHoep_sce_extract_resource,	/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* amw *********************/
static int MainProgramHoep_amw_extract_directory(struct package *pkg,
												 struct package_directory *pkg_dir);
/* 封包匹配回调函数 */
static int MainProgramHoep_amw_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	struct package_directory pkg_dir;
	int ret = MainProgramHoep_amw_extract_directory(pkg, &pkg_dir);
	if (!ret)
		free(pkg_dir.directory);

	return ret;	
}

/* 封包索引目录提取函数 */
static int MainProgramHoep_amw_extract_directory(struct package *pkg,
												 struct package_directory *pkg_dir)
{
	amw_header_t amw_header;
	amw_entry_t *index_buffer;
	my_amw_entry_t *my_index_buffer;
	DWORD index_buffer_length;	
	DWORD i;

	if (pkg->pio->readvec(pkg, &amw_header, sizeof(amw_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	index_buffer_length = amw_header.index_entries * sizeof(amw_entry_t);
	index_buffer = (amw_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EMEM;
	}

	index_buffer_length = amw_header.index_entries * sizeof(my_amw_entry_t);
	my_index_buffer = (my_amw_entry_t *)malloc(index_buffer_length);
	if (!my_index_buffer) {
		free(index_buffer);
		return -CUI_EMEM;
	}

	for (i = 0; i < amw_header.index_entries; i++) {
		int err = 0;
		int name_len = 0;

		while (1) {
			if (pkg->pio->read(pkg, &my_index_buffer[i].name[name_len], 1)) {
				err = 1;
				break;
			}
			if (!my_index_buffer[i].name[name_len])
				break;
			name_len++;
		}
		if (err)
			break;
		
		my_index_buffer[i].length = index_buffer[i].length;
		my_index_buffer[i].offset = index_buffer[i].offset;
	}
	free(index_buffer);
	if (i != amw_header.index_entries) {
		free(my_index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = amw_header.index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(my_amw_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int MainProgramHoep_amw_parse_resource_info(struct package *pkg,
												   struct package_resource *pkg_res)
{
	my_amw_entry_t *my_amw_entry;

	my_amw_entry = (my_amw_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_amw_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_amw_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_amw_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int MainProgramHoep_amw_extract_resource(struct package *pkg,
												struct package_resource *pkg_res)
{
	BYTE *raw_data, *act_data;
	DWORD raw_data_len, act_data_len;

	raw_data_len = pkg_res->raw_data_length;
	raw_data = (BYTE *)malloc(raw_data_len);
	if (!raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw_data, raw_data_len, pkg_res->offset, IO_SEEK_SET)) {
		free(raw_data);
		return -CUI_EREADVEC;
	}

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = raw_data;
		return 0;
	}

	if (!strncmp((char *)raw_data, "CRM", 4)) {
		eog_header_t *eog_header = (eog_header_t *)raw_data;

		act_data_len = raw_data_len - sizeof(eog_header_t);
		act_data = (BYTE *)malloc(act_data_len);
		if (!act_data) {
			free(raw_data);
			return -CUI_EMEM;
		}
		memcpy(act_data, eog_header + 1, act_data_len);
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".ogg");
	} else {
		act_data = NULL;
		act_data_len = 0;
	}
	pkg_res->raw_data = raw_data;
	pkg_res->raw_data_length = raw_data_len;
	pkg_res->actual_data = act_data;
	pkg_res->actual_data_length = act_data_len;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation MainProgramHoep_amw_operation = {
	MainProgramHoep_amw_match,				/* match */
	MainProgramHoep_amw_extract_directory,	/* extract_directory */
	MainProgramHoep_amw_parse_resource_info,/* parse_resource_info */
	MainProgramHoep_amw_extract_resource,	/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* amb *********************/

/* 封包匹配回调函数 */
static int MainProgramHoep_amb_match(struct package *pkg)
{
	amb_header_t amb_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &amb_header, sizeof(amb_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;	
	}

	if (strncmp(amb_header.magic, "CRM", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包资源提取函数 */
static int MainProgramHoep_amb_extract_resource(struct package *pkg,
									  struct package_resource *pkg_res)
{
	u32 fsize;
	BYTE *dat;

	if (pkg->pio->length_of(pkg, &fsize))
		return -CUI_ELEN;

	dat = (BYTE *)pkg->pio->readvec_only(pkg, fsize - sizeof(amb_header_t), sizeof(amb_header_t), IO_SEEK_SET);
	if (!dat)
		return -CUI_EREADVECONLY;

	pkg_res->raw_data = dat;
	pkg_res->raw_data_length = fsize - sizeof(amb_header_t);

	return 0;
}

/* 封包资源释放函数 */
static void MainProgramHoep_amb_release_resource(struct package *pkg,
												 struct package_resource *pkg_res)
{
	if (pkg_res->raw_data)
		pkg_res->raw_data = NULL;
}

/* 封包处理回调函数集合 */
static cui_ext_operation MainProgramHoep_amb_operation = {
	MainProgramHoep_amb_match,				/* match */
	NULL,									/* extract_directory */
	NULL,									/* parse_resource_info */
	MainProgramHoep_amb_extract_resource,	/* extract_resource */
	cui_common_save_resource,				/* save_resource */
	MainProgramHoep_amb_release_resource,	/* release_resource */
	cui_common_release						/* release */
};

/********************* amp *********************/

/* 封包匹配回调函数 */
static int MainProgramHoep_amp_match(struct package *pkg)
{
	amp_header_t amp_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &amp_header, sizeof(amp_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;	
	}

	if (strncmp(amp_header.magic, "AMNP", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包资源提取函数 */
static int MainProgramHoep_amp_extract_resource(struct package *pkg,
									  struct package_resource *pkg_res)
{
	u32 fsize;
	DWORD idat_comprlen, zlib_comprlen, zlib_uncomprlen, curbyte, actual_data_len;
	BYTE *idat_compr, *zlib_compr, *zlib_uncompr, *actual_data;
	DWORD i, crc, width, height;
	int SAVE_AS_PNG;

	if (get_options("SaveAsPNG"))
		SAVE_AS_PNG = 1;
	else
		SAVE_AS_PNG = 0;

	/* 读取图像宽度和高度 */
	if (pkg->pio->readvec(pkg, &width, 4, 4, IO_SEEK_SET))
		return -CUI_EREADVEC;

	if (pkg->pio->readvec(pkg, &height, 4, 8, IO_SEEK_SET))
		return -CUI_EREADVEC;

	#define SWAP32(x)		((u32)(((x) << 24) | (((x) << 8) & 0x00ff0000) | (((x) >> 8) & 0x0000ff00) | ((x) >> 24)))

	width = SWAP32(width);
	height = SWAP32(height);

	if (pkg->pio->length_of(pkg, &fsize))
		return -CUI_ELEN;
	
	idat_comprlen = fsize - 0x1a + 8;	/* 补上第一个IDAT长度和IDAT标签（注意这里多减了一字节是因为amp最后1自己是IEND长度的最高字节） */
	idat_compr = (BYTE *)malloc(idat_comprlen);
	if (!idat_compr)
		return -CUI_EMEM;

	/* 读取第一块IDAT的长度 */
	if (pkg->pio->readvec(pkg, idat_compr, 4, 0x15, IO_SEEK_SET)) {
		free(idat_compr);
		return -CUI_EREADVEC;
	}
	idat_compr[4] = 'I';
	idat_compr[5] = 'D';
	idat_compr[6] = 'A';
	idat_compr[7] = 'T';

	/* 读取整个IDAT块数据 */
	if (pkg->pio->readvec(pkg, idat_compr + 8, idat_comprlen - 8, 0x19, IO_SEEK_SET)) {
		free(idat_compr);
		return -CUI_EREADVEC;
	}

	/* 第一块IDAT恢复完毕，收集所有IDAT组成完整的zlib stream */
	curbyte = 0;
	zlib_compr = NULL;
	zlib_comprlen = 0;
	do {
		BYTE *sub_idat_compr;
		DWORD sub_idat_comprlen;

		/* 获得IDAT长度 */
		sub_idat_comprlen = SWAP32(*(u32 *)(&idat_compr[curbyte]));
		curbyte += 4;

		/* 非IDAT */
		if (memcmp("IDAT", &idat_compr[curbyte], 4))
			break;

		sub_idat_compr = &idat_compr[curbyte];	/* 指向IDAT标识 */

		/* 检查crc */
		if (crc32(0L, sub_idat_compr, sub_idat_comprlen + 4) != SWAP32(*(u32 *)(&sub_idat_compr[4 + sub_idat_comprlen])))
			break;

		curbyte += 4;	
		sub_idat_compr += 4;
		if (!zlib_compr)
			zlib_compr = (BYTE *)malloc(sub_idat_comprlen);
		else
			zlib_compr = (BYTE *)realloc(zlib_compr, zlib_comprlen + sub_idat_comprlen);
		if (!zlib_compr)
			break;
		memcpy(zlib_compr + zlib_comprlen, sub_idat_compr, sub_idat_comprlen);
		curbyte += sub_idat_comprlen + 4;
		zlib_comprlen += sub_idat_comprlen;
	} while (curbyte < idat_comprlen);
	free(idat_compr);
	if (curbyte != idat_comprlen) {
		if (zlib_compr)
			free(zlib_compr);
		return -CUI_EUNCOMPR;
	}

	/* 解压zlib stream */
	for (i = 1; ; i++) {	
		zlib_uncomprlen = zlib_comprlen << i;
		zlib_uncompr = (BYTE *)malloc(zlib_uncomprlen);
		if (!zlib_uncompr)
			break;
			
		if (uncompress(zlib_uncompr, &zlib_uncomprlen, zlib_compr, zlib_comprlen) == Z_OK)
			break;
		free(zlib_uncompr);
	}
	free(zlib_compr);
	if (!zlib_uncompr)
		return -CUI_EUNCOMPR;

	for (DWORD h = 0; h < height; h++) {
		BYTE *line, *pre_line, *rgba;

		line = zlib_uncompr + h * (width * 4 + 1);
		pre_line = h ? line - (width * 4 + 1) : (BYTE*)-1;
		rgba = line + 1;

		if (!SAVE_AS_PNG) {
			/* defiltering处理 */
			switch (line[0]) {
			case 0:			
				break;
			case 1:
				sub_defiltering(&line[1], width, 4);
				break;
			case 2:
				up_defiltering(pre_line + 1, &line[1], width, 4);	
				break;
			case 3:
				average_defiltering(pre_line + 1, &line[1], width, 4);	
				break;
			case 4:
				paeth_defiltering(pre_line + 1, &line[1], width, 4);
				break;
			}
			/* 最终保存的PNG不做filtering处理 */
			line[0] = 0;
		} else {
			/* 交换R和B分量 */
			for (DWORD w = 0; w < width; w++) {
				BYTE tmp;

				tmp = rgba[0];			
				rgba[0] = rgba[2];
				rgba[2] = tmp;
				rgba += 4;
			}
		}
	}

	if (!SAVE_AS_PNG) {
		BYTE *dst, *src;
		DWORD line_len;
#if 1
		line_len = (width * 3 + 3) & ~3;
		DWORD align = line_len - width * 3;
		actual_data_len = line_len * height;
		actual_data = (BYTE *)malloc(actual_data_len);
		if (!actual_data) {
			free(actual_data);
			return -CUI_EMEM;
		}

		dst = actual_data;
		src = zlib_uncompr;
		for (h = 0; h < height; ++h) {
			++src;
			for (DWORD x = 0; x < width; ++x) {
				*dst++ = *src++;
				*dst++ = *src++;
				*dst++ = *src++;
				++src;
			}
			dst += align;
		}
		free(zlib_uncompr);

		if (MyBuildBMPFile(actual_data, actual_data_len, NULL, 0, 
				width, 0 - height, 24, &dst, &actual_data_len, my_malloc)) {
			free(actual_data);
			return -CUI_EMEM;
		}
		free(actual_data);
		actual_data = dst;
#else
		line_len = width * 4;
		actual_data_len = line_len * height;
		actual_data = (BYTE *)malloc(actual_data_len);
		if (!actual_data) {
			free(actual_data);
			return -CUI_EMEM;
		}

		dst = actual_data;
		src = zlib_uncompr;
		for (h = 0; h < height; h++) {
			memcpy(dst, src + 1, line_len);
			dst += line_len;
			src += line_len + 1;
		}
		free(zlib_uncompr);

		if (MyBuildBMPFile(actual_data, actual_data_len, NULL, 0, 
				width, 0 - height, 32, &dst, &actual_data_len, my_malloc)) {
			free(actual_data);
			return -CUI_EMEM;
		}
		free(actual_data);
		actual_data = dst;
#endif
		pkg_res->replace_extension = _T(".bmp");
	} else {
		/* 现在得到原始的RGB(A)数据, 然后重新压缩 */
		for (i = 1; ; i++) {
			zlib_comprlen = zlib_uncomprlen << i;			
			zlib_compr = (BYTE *)malloc(zlib_comprlen);
			if (!zlib_compr)
				break;
				
			if (compress(zlib_compr, &zlib_comprlen, zlib_uncompr, zlib_uncomprlen) == Z_OK)
				break;
			free(zlib_compr);
		}
		free(zlib_uncompr);
		if (!zlib_compr)
			return -CUI_EUNCOMPR;

		/* 创建最终经过“修正的”PNG图像缓冲区(非括号部分来自amp文件) */
		actual_data_len = (8/* png header */ + 8 /* IHDR length and "IHDR" */) + 13 /* IHDR data */ + 4 /* IHDR crc */ 
			+ (4 /* IDAT length */ + 4/* "IDAT" */ + zlib_comprlen /* IDAT数据 */ + 4 /* IDAT crc */
			+ 4 /* IEND length */ + 4 /* "IEND" */ + 4 /* IEND crc */);
		actual_data = (BYTE *)malloc(actual_data_len);
		if (!actual_data) {
			free(zlib_compr);
			return -CUI_EMEM;
		}

		/* 写入PNG头 */
		actual_data[0] = '\x89';
		actual_data[1] = 'P';
		actual_data[2] = 'N';
		actual_data[3] = 'G';
		actual_data[4] = '\xd';
		actual_data[5] = '\xa';
		actual_data[6] = '\x1a';
		actual_data[7] = '\xa';
		/* 写入IHDR长度 */
		actual_data[8] = 0;
		actual_data[9] = 0;
		actual_data[10] = 0;
		actual_data[11] = 13;
		/* 写入IHDR标识 */
		actual_data[12] = 'I';
		actual_data[13] = 'H';
		actual_data[14] = 'D';
		actual_data[15] = 'R';

		/* 读取IHDR数据和crc */
		if (pkg->pio->readvec(pkg, &actual_data[16], 13 + 4, 4, IO_SEEK_SET)) {
			free(actual_data);
			free(zlib_compr);
			return -CUI_EREADVEC;
		}

		/* 写入IDAT长度 */
		*(u32 *)(&actual_data[33]) = SWAP32(zlib_comprlen);
		/* 写入IDAT标识 */
		actual_data[37] = 'I';
		actual_data[38] = 'D';
		actual_data[39] = 'A';
		actual_data[40] = 'T';
		/* 写入IDAT数据 */
		memcpy(actual_data + 41, zlib_compr, zlib_comprlen);
		/* 计算IDAT crc */
		crc = crc32(0L, &actual_data[37], zlib_comprlen + 4);
		free(zlib_compr);
		/* 写入crc */
		*(u32 *)(&actual_data[actual_data_len - 16]) = SWAP32(crc);
		/* 写入IEND信息。实际上IEND长度的第一个字节来自amp文件的最后1字节，但总是为0，所以就不再从amp读取了 */
		actual_data[actual_data_len - 12] = 0;
		actual_data[actual_data_len - 11] = 0;
		actual_data[actual_data_len - 10] = 0;
		actual_data[actual_data_len - 9] = 0;
		actual_data[actual_data_len - 8] = 'I';
		actual_data[actual_data_len - 7] = 'E';
		actual_data[actual_data_len - 6] = 'N';
		actual_data[actual_data_len - 5] = 'D';
		actual_data[actual_data_len - 4] = '\xae';
		actual_data[actual_data_len - 3] = '\x42';
		actual_data[actual_data_len - 2] = '\x60';
		actual_data[actual_data_len - 1] = '\x82';

		pkg_res->replace_extension = _T(".png");
	}
	pkg_res->actual_data = actual_data;
	pkg_res->actual_data_length = actual_data_len;
	pkg_res->raw_data_length = fsize;
	pkg_res->flags |= PKG_RES_FLAG_REEXT;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation MainProgramHoep_amp_operation = {
	MainProgramHoep_amp_match,				/* match */
	NULL,									/* extract_directory */
	NULL,									/* parse_resource_info */
	MainProgramHoep_amp_extract_resource,	/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* cwp *********************/

/* 封包匹配回调函数 */
static int MainProgramHoep_cwp_match(struct package *pkg)
{
	cwp_header_t cwp_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &cwp_header, sizeof(cwp_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;	
	}

	if (strncmp(cwp_header.magic, "CWDP", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包处理回调函数集合 */
static cui_ext_operation MainProgramHoep_cwp_operation = {
	MainProgramHoep_cwp_match,				/* match */
	NULL,									/* extract_directory */
	NULL,									/* parse_resource_info */
	MainProgramHoep_amp_extract_resource,	/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* cwd *********************/

/* 封包匹配回调函数 */
static int MainProgramHoep_cwd_match(struct package *pkg)
{
	s8 magic[32];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;	
	}

	if (strncmp(magic, "cwd format  - version 1.00 -", 32)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包资源提取函数 */
static int MainProgramHoep_cwd_extract_resource(struct package *pkg,
												struct package_resource *pkg_res)
{
	u32 width, height, decrypt_key;
	BYTE *dib;
	DWORD dib_length;
	u32 fsize;

	if (pkg->pio->length_of(pkg, &fsize))
		return -CUI_ELEN;

	/* 读取图像宽度和高度 */
	if (pkg->pio->readvec(pkg, &width, 4, 0x2c, IO_SEEK_SET))
		return -CUI_EREADVEC;

	if (pkg->pio->readvec(pkg, &height, 4, 0x30, IO_SEEK_SET))
		return -CUI_EREADVEC;

	if (pkg->pio->readvec(pkg, &decrypt_key, 4, 0x34, IO_SEEK_SET))
		return -CUI_EREADVEC;

	decrypt_key = (decrypt_key & 0xff) + 0x259a;
	width += decrypt_key;
	height += decrypt_key;

	dib_length = width * height * 2;
	dib = (BYTE *)malloc(dib_length);
	if (!dib)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, dib, dib_length, 0x38, IO_SEEK_SET)) {
		free(dib);
		return -CUI_EREADVEC;
	}

	if (MyBuildBMP16File(dib, dib_length, width, 0 - height, (BYTE **)&pkg_res->actual_data, 
			&pkg_res->actual_data_length, RGB555, NULL, my_malloc)) {
		free(dib);
		return -CUI_EMEM;
	}
	free(dib);

	pkg_res->raw_data_length = fsize;
	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation MainProgramHoep_cwd_operation = {
	MainProgramHoep_cwd_match,				/* match */
	NULL,									/* extract_directory */
	NULL,									/* parse_resource_info */
	MainProgramHoep_cwd_extract_resource,	/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* CWL *********************/

/* 封包匹配回调函数 */
static int MainProgramHoep_cwl_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;	
	}

	if (strncmp(magic, "SZDD", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包资源提取函数 */
static int MainProgramHoep_cwl_extract_resource(struct package *pkg,
									  struct package_resource *pkg_res)
{
	OFSTRUCT of;
	INT CWL_SIZE;

	int file = LZOpenFile((TCHAR *)pkg->full_path, &of, OF_READ);
	if (file < 0)
		return -CUI_EOPEN;

	CWL_SIZE = LZSeek(file, 0, 2);
	if (CWL_SIZE < 0) {
		LZClose(file);
		return -CUI_ESEEK;
	}

	if (LZSeek(file, 0, 0) < 0) {
		LZClose(file);
		return -CUI_ESEEK;
	}

	char *CWL = (char *)malloc(CWL_SIZE);
	if (!CWL) {
		LZClose(file);
		return -CUI_EMEM;
	}

	if (LZRead(file, CWL, CWL_SIZE) != CWL_SIZE) {
		free(CWL);
		LZClose(file);
		return -CUI_EREAD;
	}
	LZClose(file);

	pkg_res->actual_data = CWL;
	pkg_res->actual_data_length = CWL_SIZE;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation MainProgramHoep_cwl_operation = {
	MainProgramHoep_cwl_match,				/* match */
	NULL,									/* extract_directory */
	NULL,									/* parse_resource_info */
	MainProgramHoep_cwl_extract_resource,	/* extract_resource */
	CUI_COMMON_OPERATION
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK MainProgramHoep_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".cwl"), _T(".cwd"), 
		NULL, &MainProgramHoep_cwl_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".CWZ"), _T(".cwd"), 
		NULL, &MainProgramHoep_cwl_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".sce"), NULL, 
		NULL, &MainProgramHoep_sce_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &MainProgramHoep_dat_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".amw"), NULL, 
		NULL, &MainProgramHoep_amw_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".pck"), NULL, 
		NULL, &MainProgramHoep_amw_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".amb"), _T(".ogg"), 
		NULL, &MainProgramHoep_amb_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".eog"), _T(".ogg"), 
		NULL, &MainProgramHoep_amb_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".amp"), NULL, 
		NULL, &MainProgramHoep_amp_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".cwp"), NULL, 
		NULL, &MainProgramHoep_cwp_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".cwd"), _T(".bmp"), 
		NULL, &MainProgramHoep_cwd_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".cw_"), _T(".bmp"), 
		NULL, &MainProgramHoep_cwd_operation, CUI_EXT_FLAG_PKG))
			return -1;

	return 0;
}
