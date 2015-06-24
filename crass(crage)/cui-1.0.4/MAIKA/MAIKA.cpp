#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>
#include <cui_common.h>

/* 接口数据结构: 表示cui插件的一般信息 */
struct acui_information MAIKA_cui_information = {
	_T("MAIKA"),			/* copyright */
	_T(""),					/* system */
	_T(".DAT"),				/* package */
	_T("1.0.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-4-4 19:52"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
// 关于加密部分，它的代码有问题
typedef struct {
	s8 magic[8];		// "MK2.0"
	u16 date_offset;
	u32 index_length;
	u32 index_offset;
	u32 index_entries;
	u16 reserved;		// 0
	u16 flags;			// bit0 - 索引段是否加密
	u16 crypt_data_length;
} DAT_header_t;

/* 索引段分2个部分：前部分（具体长度不定）是512项每项6字节的
 * 快速索引，指向索引段后部分的真正的索引项信息.counts字段表示
 * 有多少连续的有效项(counts为0表示当前fast index无效)。
 */
typedef struct {
	u32 offset;
	u16 counts;
} DAT_fast_index_t;

typedef struct {
	s8 magic[2];	// "F1" or "C1" or "D1"
	u32 comprlen;
	u32 uncomprlen;
} B1_header_t;
#pragma pack ()

/* .dat封包的索引项结构 */
typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 length;
} my_DAT_entry_t;


static DWORD lzss_uncompress(BYTE *uncompr, DWORD uncomprlen, 
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
	while (act_uncomprlen < uncomprlen) {
		flag >>= 1;
		if (!(flag & 0x0100))
			flag = compr[curbyte++] | 0xff00;

		if (flag & 1) {
			unsigned char data;

			data = compr[curbyte++];
			uncompr[act_uncomprlen++] = data;
			/* 输出的1字节放入滑动窗口 */
			win[nCurWindowByte++] = data;
			nCurWindowByte &= win_size - 1;
		} else {
			unsigned int copy_bytes, win_offset;
			unsigned int i;

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

	return act_uncomprlen;
}

static DWORD rle_uncompress(BYTE *uncompr, BYTE *compr)
{
	DWORD act_uncomprlen = 0;
	DWORD curbyte = 0;

	while (1) {
		BYTE flag = compr[curbyte++];
		if (flag == 0xff)
			break;

		DWORD cp = *(u32 *)&compr[curbyte];
		curbyte += 4;
		if (flag == 3) {
			BYTE dat = compr[curbyte++];
			for (DWORD i = 0; i < cp; i++)
				uncompr[act_uncomprlen++] = dat;
		} else {
			for (DWORD i = 0; i < cp; i++)
				uncompr[act_uncomprlen++] = compr[curbyte++];
		}
	}

	return act_uncomprlen;
}

/********************* DAT *********************/

/* 封包匹配回调函数 */
static int MAIKA_DAT_match(struct package *pkg)
{
	s8 magic[8];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "MK2.0", sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int MAIKA_DAT_extract_directory(struct package *pkg,
									   struct package_directory *pkg_dir)
{
	DAT_header_t DAT_header;
	my_DAT_entry_t *my_index_buffer;
	DWORD index_buffer_length;	

	if (pkg->pio->readvec(pkg, &DAT_header, sizeof(DAT_header), 
			0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	index_buffer_length = DAT_header.index_entries * sizeof(my_DAT_entry_t);
	my_index_buffer = (my_DAT_entry_t *)malloc(index_buffer_length);
	if (!my_index_buffer)
		return -CUI_EMEM;

	BYTE *index_buffer = (BYTE *)malloc(DAT_header.index_length);
	if (!index_buffer) {
		free(my_index_buffer);
		return -CUI_EMEM;
	}

	if (pkg->pio->readvec(pkg, index_buffer, DAT_header.index_length, 
			DAT_header.index_offset, IO_SEEK_SET)) {
		free(index_buffer);
		free(my_index_buffer);
		return -CUI_EREADVEC;
	}

	DAT_fast_index_t *fentry = (DAT_fast_index_t *)index_buffer;
	DWORD k = 0;
	for (DWORD i = 0; i < 512; i++) {
		BYTE *p;
		int name_len;

		p = index_buffer + fentry->offset;
		if (fentry->counts) {
			for (DWORD j = 0; j < fentry->counts; j++) {
				my_DAT_entry_t *DAT_entry = &my_index_buffer[k++];

				DAT_entry->offset = *(u32 *)p + DAT_header.date_offset;
				p += 4;
				DAT_entry->length = *(u32 *)p;
				p += 4;
				name_len = *p++;
				strncpy(DAT_entry->name, (char *)p, name_len);
				p += name_len;
				DAT_entry->name[name_len] = 0;
			}
		} else if (*(u32 *)p == -1)	// last entry
			break;
		fentry++;
	}
	free(index_buffer);
	if (k != DAT_header.index_entries) {
		free(my_index_buffer);
		return -CUI_EMATCH;
	}

	pkg_dir->index_entries = DAT_header.index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(my_DAT_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int MAIKA_DAT_parse_resource_info(struct package *pkg,
										 struct package_resource *pkg_res)
{
	my_DAT_entry_t *my_DAT_entry;

	my_DAT_entry = (my_DAT_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_DAT_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_DAT_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_DAT_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int MAIKA_DAT_extract_resource(struct package *pkg,
									  struct package_resource *pkg_res)
{
	void *raw = malloc(pkg_res->raw_data_length);
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length,
			pkg_res->offset, IO_SEEK_SET)) {
		free(raw);
		return -CUI_EREADVECONLY;
	}

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = raw;
		return 0;
	}

	if (!strncmp((char *)raw, "F1", 2) || !strncmp((char *)raw, "D1", 2)
			|| !strncmp((char *)raw, "C1", 2)) {
		B1_header_t *B1_header = (B1_header_t *)raw;

		BYTE *uncompr = (BYTE *)malloc(B1_header->uncomprlen);
		if (!uncompr)
			return -CUI_EMEM;

		BYTE *compr = (BYTE *)(B1_header + 1);

		BYTE tmp;
		tmp = compr[7];
		compr[7] = compr[11];
		compr[11] = tmp;
		tmp = compr[9];
		compr[9] = compr[12];
		compr[12] = tmp;

		DWORD act_uncomprlen = lzss_uncompress(uncompr, B1_header->uncomprlen,
			compr, B1_header->comprlen);
		if (act_uncomprlen != B1_header->uncomprlen) {
			free(uncompr);
			free(compr);
			return -CUI_EUNCOMPR;
		}

		if (strncmp((char *)uncompr, "BPR02", 5)) {
			free(uncompr);
			free(compr);
			return -CUI_EUNCOMPR;			
		}
		BYTE *act_uncompr = (BYTE *)malloc(0x400000);
		if (!act_uncompr) {
			free(uncompr);
			free(compr);
			return -CUI_EMEM;	
		}
		act_uncomprlen = rle_uncompress(act_uncompr, uncompr + 5);
		free(uncompr);
		pkg_res->actual_data = act_uncompr;
		pkg_res->actual_data_length = act_uncomprlen;
	}
	pkg_res->raw_data = raw;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation MAIKA_DAT_operation = {
	MAIKA_DAT_match,				/* match */
	MAIKA_DAT_extract_directory,	/* extract_directory */
	MAIKA_DAT_parse_resource_info,	/* parse_resource_info */
	MAIKA_DAT_extract_resource,		/* extract_resource */
	CUI_COMMON_OPERATION
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK MAIKA_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".DAT"), NULL, 
		NULL, &MAIKA_DAT_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
