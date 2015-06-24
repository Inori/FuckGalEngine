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
struct acui_information NPS_cui_information = {
	_T("株式会社ニトロプラス"),	/* copyright */
	_T("NPS"),					/* system */
	_T(".npp"),					/* package */
	_T("1.0.0"),				/* revision */
	_T("痴漢公賊"),				/* author */
	_T("2008-4-23 1:08"),		/* date */
	NULL,						/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[4];		// "nitP"
	u32 index_entries;
} npp_header_t;

typedef struct {		// 0x90 bytes
	u32 offset;
	u32 comprlen;
	u32 uncomprlen;
	u16 is_compressed;
	u16 pad;
	s8 dir_name[64];
	s8 file_name[64];
} npp_entry_t;
#pragma pack ()

static void lzss_uncompress(BYTE *uncompr, DWORD uncomprlen, 
							BYTE *compr, DWORD comprlen)
{
	unsigned int act_uncomprlen = 0;
	/* compr中的当前字节中的下一个扫描位的位置 */
	unsigned int curbit = 0;
	/* compr中的当前扫描字节 */
	unsigned int curbyte = 0;
	unsigned int nCurWindowByte = 0xfee;
	const unsigned int win_size = 4096;
	BYTE win[4096];
	WORD flag = 0;

	memset(win, 0, win_size);
	while (1) {
		flag >>= 1;
		if (!(flag & 0x0100))
			flag = compr[curbyte++] | 0xff00;

		if (flag & 1) {
			unsigned char data;

			data = compr[curbyte++];
			uncompr[act_uncomprlen++] = data;
			if (act_uncomprlen >= uncomprlen)
				break;
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
				if (act_uncomprlen >= uncomprlen)
					return;
				/* 输出的1字节放入滑动窗口 */
				win[nCurWindowByte++] = data;
				nCurWindowByte &= win_size - 1;	
			}
		}
	}
}

/********************* npp *********************/

/* 封包匹配回调函数 */
static int NPS_npp_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "nitP", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int NPS_npp_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{
	npp_header_t npp_header;
	if (pkg->pio->readvec(pkg, &npp_header, sizeof(npp_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD index_buffer_length = npp_header.index_entries * sizeof(npp_entry_t);
	npp_entry_t *index_buffer = new npp_entry_t[npp_header.index_entries];
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		delete [] index_buffer;
		return -CUI_EREAD;
	}
#if 0	
	for (DWORD i = 0; i < npp_header.index_entries; i++) {
		npp_entry_t &entry = index_buffer[i];
		if (!entry.is_compressed)
			printf("%s: %x %x %x\n",entry.file_name, entry.unknown, entry.comprlen,entry.uncomprlen); 
	}exit(0);
#endif
	pkg_dir->index_entries = npp_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(npp_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int NPS_npp_parse_resource_info(struct package *pkg,
									   struct package_resource *pkg_res)
{
	npp_entry_t *npp_entry;

	npp_entry = (npp_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, npp_entry->file_name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = npp_entry->comprlen;
	pkg_res->actual_data_length = npp_entry->is_compressed ? npp_entry->uncomprlen : 0;
	pkg_res->offset = npp_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int NPS_npp_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	BYTE *compr = new BYTE[pkg_res->raw_data_length];
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET)) {
		delete [] pkg_res->raw_data;
		return -CUI_EREADVEC;
	}

	if (!(pkg_res->flags & PKG_RES_FLAG_RAW) && pkg_res->actual_data_length) {
		BYTE *uncompr = new BYTE[pkg_res->actual_data_length];
		if (!uncompr) {
			delete [] pkg_res->raw_data;
			return -CUI_EMEM;
		}

		lzss_uncompress(uncompr, pkg_res->actual_data_length, compr, pkg_res->raw_data_length);
		pkg_res->actual_data = uncompr;
	}
	pkg_res->raw_data = compr;

	return 0;
}

/* 资源保存函数 */
static int NPS_npp_save_resource(struct resource *res, 
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
static void NPS_npp_release_resource(struct package *pkg, 
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
static void NPS_npp_release(struct package *pkg, 
							struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation NPS_npp_operation = {
	NPS_npp_match,					/* match */
	NPS_npp_extract_directory,		/* extract_directory */
	NPS_npp_parse_resource_info,	/* parse_resource_info */
	NPS_npp_extract_resource,		/* extract_resource */
	NPS_npp_save_resource,			/* save_resource */
	NPS_npp_release_resource,		/* release_resource */
	NPS_npp_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK NPS_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".npp"), NULL, 
		NULL, &NPS_npp_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
