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
struct acui_information Echeross_cui_information = {
	_T(""),		/* copyright */
	_T(""),			/* system */
	_T(""),	/* package */
	_T("1.0.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-2-13 21:49"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	u32 index_entries;
} audio_header_t;

typedef struct {
	u32 Channels;
	u32 SamplesPerSec;
	u32 BitsPerSample;
	u32 AvgBytesPerSec;
	u32 BlockAlign;
	u32 data;		// "da"
	u32 pcm_data_len;
	u32 seconds;	// 播放时间
	u32 remain_len;	// 不足一秒的pcm数据长度
	u32 total_len;	// pcm_data_len
} pcm_header_t;

typedef struct {
	u32 unknown;	// 0x64
	u32 index_entries;
} ucg_header_t;

typedef struct {
	s8 name[20];
	u32 offset;
} ucg_entry_t;

typedef struct {
	u32 length;
	u32 unknown0;	// 0
	u32 width;
	u32 height;
	u32 unknown1;	// 0
	u32 unknown2;	// 0
	u32 unknown3;	// 0
} bmp_header_t;
#pragma pack ()

/* 封包的索引项结构 */
typedef struct {
	char name[MAX_PATH];
	u32 distance;
	u32 length;
} Echeross_entry_t;

static void *my_malloc(DWORD len)
{
	return new BYTE[len];
}

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
	while (curbyte < comprlen) {
		flag >>= 1;
		if (!(flag & 0x0100)) {
			flag = compr[curbyte++] | 0xff00;
			if (curbyte > comprlen)
				break;
		}

		if (flag & 1) {
			unsigned char data;

			data = compr[curbyte++];
			if (curbyte > comprlen)
				break;
			uncompr[act_uncomprlen++] = data;
			if (act_uncomprlen > uncomprlen)
				break;
			/* 输出的1字节放入滑动窗口 */
			win[nCurWindowByte++] = data;
			nCurWindowByte &= win_size - 1;
		} else {
			unsigned int copy_bytes, win_offset;
			unsigned int i;

			win_offset = compr[curbyte++];
			if (curbyte > comprlen)
				break;
			copy_bytes = compr[curbyte++];
			if (curbyte > comprlen)
				break;
			win_offset |= (copy_bytes & 0xf0) << 4;
			copy_bytes &= 0x0f;
			copy_bytes += 3;

			for (i = 0; i < copy_bytes; i++) {	
				unsigned char data;

				data = win[(win_offset + i) & (win_size - 1)];				
				uncompr[act_uncomprlen++] = data;
				if (act_uncomprlen > uncomprlen)
					goto out;
				/* 输出的1字节放入滑动窗口 */
				win[nCurWindowByte++] = data;
				nCurWindowByte &= win_size - 1;	
			}
		}
	}
out:
	return act_uncomprlen;
}

/********************* audio *********************/

/* 封包匹配回调函数 */
static int Echeross_audio_match(struct package *pkg)
{
	u32 index_entries;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &index_entries, 4)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (!index_entries) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	u32 offset = 4;
	u32 first_offset;
	for (DWORD i = 0; i < index_entries; ++i) {
		char fname[MAX_PATH];

		for (int nlen = 0; nlen < MAX_PATH; ++nlen) {			
			if (pkg->pio->read(pkg, fname + nlen, 1)) {
				pkg->pio->close(pkg);
				return -CUI_EREAD;
			}
			if (!fname[nlen])
				break;
		}
		if (!nlen || nlen == MAX_PATH)
			break;

		u32 entry_offset;
		if (pkg->pio->read(pkg, &entry_offset, 4)) {
			pkg->pio->close(pkg);
			return -CUI_EREAD;
		}
		if (!i)
			first_offset = entry_offset;
		offset += nlen + 1 + 4;
	}
	if (i != index_entries || offset != first_offset) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int Echeross_audio_extract_directory(struct package *pkg,
											struct package_directory *pkg_dir)
{
	u32 index_entries;
	if (pkg->pio->readvec(pkg, &index_entries, 4, 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD index_len = sizeof(Echeross_entry_t) * index_entries;
	Echeross_entry_t *index = new Echeross_entry_t[index_entries];
	if (!index)
		return -CUI_EMEM;

	Echeross_entry_t *entry = index;
	for (DWORD i = 0; i < index_entries; ++i) {
		for (int nlen = 0; ; ++nlen) {			
			if (pkg->pio->read(pkg, entry->name + nlen, 1)) {
				delete [] index;
				return -CUI_EREAD;
			}
			if (!entry->name[nlen])
				break;
		}
		if (!nlen)
			break;

		if (pkg->pio->read(pkg, &entry->distance, 4)) {
			delete [] index;
			return -CUI_EREAD;
		}

		++entry;
	}
	if (i != index_entries) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	for (i = 0; i < index_entries - 1; ++i)
		index[i].length = index[i + 1].distance - index[i].distance;

	u32 fsize;
	pkg->pio->length_of(pkg, &fsize);
	index[i].length = fsize - index[i].distance;

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index;
	pkg_dir->directory_length = index_len;
	pkg_dir->index_entry_length = sizeof(Echeross_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int Echeross_audio_parse_resource_info(struct package *pkg,
											  struct package_resource *pkg_res)
{
	Echeross_entry_t *Echeross_entry;

	Echeross_entry = (Echeross_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, Echeross_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = Echeross_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = Echeross_entry->distance;

	return 0;
}

/* 封包资源提取函数 */
static int Echeross_audio_extract_resource(struct package *pkg,
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

	pcm_header_t *header = (pcm_header_t *)raw;
	if (MySaveAsWAVE(header + 1, header->pcm_data_len, 
		   1, header->Channels, header->SamplesPerSec, 
		   header->BitsPerSample, NULL, 0, 
		   (BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length,
		   my_malloc)) {
		delete [] raw;
		return -CUI_EMEM;
	}
	delete [] raw;

	return 0;
}

/* 资源保存函数 */
static int Echeross_save_resource(struct resource *res, 
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
static void Echeross_release_resource(struct package *pkg,
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
static void Echeross_release(struct package *pkg, 
							 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation Echeross_audio_operation = {
	Echeross_audio_match,				/* match */
	Echeross_audio_extract_directory,	/* extract_directory */
	Echeross_audio_parse_resource_info,	/* parse_resource_info */
	Echeross_audio_extract_resource,	/* extract_resource */
	Echeross_save_resource,				/* save_resource */
	Echeross_release_resource,			/* release_resource */
	Echeross_release					/* release */
};

/********************* UCG *********************/

/* 封包匹配回调函数 */
static int Echeross_ucg_match(struct package *pkg)
{
	ucg_header_t header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &header, sizeof(header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (header.unknown != 0x64 || !header.index_entries) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	ucg_entry_t first_entry;

	if (pkg->pio->read(pkg, &first_entry, sizeof(first_entry))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (first_entry.offset != sizeof(header) + sizeof(first_entry) * header.index_entries) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int Echeross_ucg_extract_directory(struct package *pkg,
										  struct package_directory *pkg_dir)
{
	ucg_header_t header;

	if (pkg->pio->readvec(pkg, &header, sizeof(header), 0, IO_SEEK_SET))
		return -CUI_EREAD;

	ucg_entry_t *index = new ucg_entry_t[header.index_entries];
	if (!index)
		return -CUI_EMEM;
		
	if (pkg->pio->read(pkg, index, sizeof(ucg_entry_t) * header.index_entries)) {
		delete [] index;
		return -CUI_EREAD;
	}

	DWORD index_len = sizeof(Echeross_entry_t) * header.index_entries;
	Echeross_entry_t *my_index = new Echeross_entry_t[header.index_entries];
	if (!my_index) {
		delete [] index;
		return -CUI_EMEM;
	}

	for (DWORD i = 0; i < header.index_entries; ++i) {
		strcpy(my_index[i].name, index[i].name);
		my_index[i].distance = index[i].offset;
	}
	delete [] index;

	for (i = 0; i < header.index_entries - 1; ++i)
		my_index[i].length = my_index[i + 1].distance - my_index[i].distance;

	u32 fsize;
	pkg->pio->length_of(pkg, &fsize);
	my_index[i].length = fsize - my_index[i].distance;

	pkg_dir->index_entries = header.index_entries;
	pkg_dir->directory = my_index;
	pkg_dir->directory_length = index_len;
	pkg_dir->index_entry_length = sizeof(Echeross_entry_t);

	return 0;
}

/* 封包资源提取函数 */
static int Echeross_ucg_extract_resource(struct package *pkg,
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

	bmp_header_t *header = (bmp_header_t *)raw;
	DWORD uncomprlen = header->width * header->height * 4;
	DWORD comprlen = header->length;
	BYTE *compr = (BYTE *)(header + 1);
	BYTE *uncompr = new BYTE[uncomprlen];
	if (!uncompr) {
		delete [] raw;
		return -CUI_EMEM;
	}

	DWORD act_uncomprlen = lzss_uncompress(uncompr, uncomprlen, compr, comprlen);
	if (act_uncomprlen != uncomprlen) {
		delete [] uncompr;
		delete [] raw;
		return -CUI_EUNCOMPR;
	}

	if (MyBuildBMPFile(uncompr, uncomprlen, NULL, 0,
			header->width, header->height, 32,
			(BYTE **)&pkg_res->actual_data, &pkg_res->actual_data_length, 
			my_malloc)) {
		delete [] uncompr;
		delete [] raw;
		return -CUI_EMEM;
	}
	delete [] uncompr;
	delete [] raw;

	return 0;
}

/* 封包处理回调函数集合 */
static cui_ext_operation Echeross_ucg_operation = {
	Echeross_ucg_match,					/* match */
	Echeross_ucg_extract_directory,		/* extract_directory */
	Echeross_audio_parse_resource_info,	/* parse_resource_info */
	Echeross_ucg_extract_resource,		/* extract_resource */
	Echeross_save_resource,				/* save_resource */
	Echeross_release_resource,			/* release_resource */
	Echeross_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK Echeross_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, NULL, _T(".wav"), 
		NULL, &Echeross_audio_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR
			| CUI_EXT_FLAG_NOEXT))
			return -1;

	if (callback->add_extension(callback->cui, NULL, _T(".bmp"), 
		NULL, &Echeross_ucg_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR
			| CUI_EXT_FLAG_NOEXT))
			return -1;

	return 0;
}
