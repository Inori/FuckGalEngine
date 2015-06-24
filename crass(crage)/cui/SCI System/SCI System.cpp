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
struct acui_information SCI_System_cui_information = {
	_T(""),					/* copyright */
	_T("SCI System"),		/* system */
	_T(".bin"),				/* package */
	_T("1.0.0"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-12-22 23:09"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	s8 magic[3];	// "TPW"
	u8 is_compressed;
	u32 uncomprlen;
	u16 offset;
} tpw_header_t;

typedef struct {
	u16 wFormatTag;
	u16 nChannels;
	u32 nSamplesPerSec;
	u32 nAvgBytesPerSec;
	u16 nBlockAlign;
	u16 wBitsPerSample;
	u16 cbSize;
	u32 data_size;
} wav_header_t;
#pragma pack ()

/* .bin封包的索引项结构 */
typedef struct {
	s8 name[256];
	u32 offset;
	u32 length;
} my_bin_entry_t;

static void *my_malloc(DWORD len)
{
	return new BYTE[len];
}

static DWORD tpw_uncompress(BYTE *uncompr, DWORD uncomprlen,
							BYTE *compr, DWORD comprlen, u16 offset)
{
	u16 offset_map[4];
	offset_map[0] = offset;
	offset_map[1] = offset * 2;
	offset_map[2] = offset * 3;
	offset_map[3] = offset * 4;
	DWORD act_uncomprlen = 0;
	BYTE len;
	while ((len = *compr++)) {
		if (len < 64) {
			for (DWORD i = 0; i < len; ++i)
				uncompr[act_uncomprlen++] = *compr++;
		} else if (len < 111) {
			len -= 61;
			u8 dat = *compr++;
			for (DWORD i = 0; i < len; ++i)
				uncompr[act_uncomprlen++] = dat;
		} else if (len == 111) {
			u16 len = *(u16 *)compr;
			compr += 2;
			u8 dat = *compr++;
			for (DWORD i = 0; i < len; ++i)
				uncompr[act_uncomprlen++] = dat;
		} else if (len < 159) {
			len -= 110;
			BYTE dat0 = *compr++;
			BYTE dat1 = *compr++;
			for (DWORD i = 0; i < len; ++i) {
				uncompr[act_uncomprlen++] = dat0;
				uncompr[act_uncomprlen++] = dat1;
			}
		} else if (len == 159) {
			u16 len = *(u16 *)compr;
			compr += 2;
			u8 dat0 = *compr++;
			u8 dat1 = *compr++;
			for (DWORD i = 0; i < len; ++i) {
				uncompr[act_uncomprlen++] = dat0;
				uncompr[act_uncomprlen++] = dat1;
			}
		} else if (len < 191) {
			len += 98;
			BYTE dat0 = *compr++;
			BYTE dat1 = *compr++;
			BYTE dat2 = *compr++;
			for (DWORD i = 0; i < len; ++i) {
				uncompr[act_uncomprlen++] = dat0;
				uncompr[act_uncomprlen++] = dat1;
				uncompr[act_uncomprlen++] = dat2;
			}
		} else if (len == 191) {
			u16 len = *(u16 *)compr;
			compr += 2;
			BYTE dat0 = *compr++;
			BYTE dat1 = *compr++;
			BYTE dat2 = *compr++;
			for (DWORD i = 0; i < len; ++i) {
				uncompr[act_uncomprlen++] = dat0;
				uncompr[act_uncomprlen++] = dat1;
				uncompr[act_uncomprlen++] = dat2;
			}
		} else {
			BYTE dat0 = *compr++;
			len = (len & 0x3f) + 3;
			BYTE *src = uncompr + act_uncomprlen + (dat0 & 0x3f) 
					- offset_map[dat0 >> 6];
			for (DWORD i = 0; i < len; ++i)
				uncompr[act_uncomprlen++] = *src++;
		}
	}

	return act_uncomprlen;
}

/********************* bin *********************/

/* 封包匹配回调函数 */
static int SCI_System_bin_match(struct package *pkg)
{
	u32 first_entry_offset;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &first_entry_offset, sizeof(first_entry_offset))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (first_entry_offset & 3) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (first_entry_offset <= 4) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int SCI_System_bin_extract_directory(struct package *pkg,
											struct package_directory *pkg_dir)
{
	u32 first_entry_offset;
	if (pkg->pio->readvec(pkg, &first_entry_offset, 4, 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	u32 index_entries = first_entry_offset / 4 - 1;
	u32 *index = new u32[index_entries + 1];
	if (!index)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, index, first_entry_offset, 0, IO_SEEK_SET)) {
		delete [] index;
		return -CUI_EREADVEC;
	}

	my_bin_entry_t *my_index = new my_bin_entry_t[index_entries];
	if (!my_index) {
		delete [] index;
		return -CUI_EMEM;
	}

	for (DWORD i = 0; i < index_entries; ++i) {
		sprintf(my_index[i].name, "%06d", i);
		my_index[i].offset = index[i];
		my_index[i].length = index[i + 1] - index[i];
	}
	delete [] index;

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = my_index;
	pkg_dir->directory_length = sizeof(my_bin_entry_t) * index_entries;
	pkg_dir->index_entry_length = sizeof(my_bin_entry_t);
	//pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

/* 封包索引项解析函数 */
static int SCI_System_bin_parse_resource_info(struct package *pkg,
											  struct package_resource *pkg_res)
{
	my_bin_entry_t *my_bin_entry;

	my_bin_entry = (my_bin_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_bin_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_bin_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = my_bin_entry->offset;

	return 0;
}

/* 封包资源提取函数 */
static int SCI_System_bin_extract_resource(struct package *pkg,
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

	tpw_header_t *tpw = (tpw_header_t *)raw;
	if (!strncmp(tpw->magic, "TPW", 3)) {
		if (!tpw->is_compressed) {
			DWORD tpw_data_len = pkg_res->raw_data_length - 4;
			BYTE *tpw_data = new BYTE[tpw_data_len];
			if (!tpw_data) {
				delete [] raw;
				return -CUI_EMEM;
			}
			memcpy(tpw_data, raw + 4, tpw_data_len);
			delete [] raw;
			pkg_res->actual_data = tpw_data;
			pkg_res->actual_data_length = tpw_data_len;
		} else {
			u32 uncomprlen = tpw->uncomprlen;
			BYTE *uncompr = new BYTE[uncomprlen + 1024];
			if (!uncompr) {
				delete [] raw;
				return -CUI_EMEM;
			}
			BYTE *compr = (BYTE *)(tpw + 1);
			DWORD comprlen = pkg_res->raw_data_length - sizeof(tpw_header_t);
			DWORD act_uncomprlen = tpw_uncompress(uncompr, uncomprlen, 
				compr, comprlen, tpw->offset);
			if (act_uncomprlen != uncomprlen) {
				printf("%x VS %x\n", act_uncomprlen, uncomprlen);
			//	delete [] uncompr;
			//	delete [] raw;
			//	return -CUI_EUNCOMPR;			
			}
			delete [] raw;
			pkg_res->actual_data = uncompr;
			pkg_res->actual_data_length = uncomprlen;
		}
	} else {
		pkg_res->actual_data = raw;
		pkg_res->actual_data_length = pkg_res->raw_data_length;
	}


	if (!strncmp((char *)pkg_res->actual_data, "BM", 2)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp");
	} else if (!strncmp((char *)pkg_res->actual_data + 4, "OggS", 4)) {
		// skip pcm size
		pkg_res->actual_data_length -= 4;
		BYTE *ogg = new BYTE[pkg_res->actual_data_length];
		if (!ogg) {
			delete [] pkg_res->actual_data;
			pkg_res->actual_data = NULL;
			return -CUI_EMEM;
		}
		memcpy(ogg, (BYTE *)pkg_res->actual_data + 4, pkg_res->actual_data_length);
		delete [] pkg_res->actual_data;
		pkg_res->actual_data = ogg;
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".ogg");
	} else if (
			pkg_res->actual_data_length > sizeof(wav_header_t) &&
			(((wav_header_t *)pkg_res->actual_data)->data_size + sizeof(wav_header_t)
				== pkg_res->actual_data_length ||
			/* for 00001 of se.bin */
			((wav_header_t *)pkg_res->actual_data)->data_size + sizeof(wav_header_t) + 0x33
				== pkg_res->actual_data_length) ||
			/* for 00000 and so on of voice.bin */
			((wav_header_t *)pkg_res->actual_data)->data_size + sizeof(wav_header_t) + 0x32
				== pkg_res->actual_data_length
			) {
		wav_header_t *wav_header = (wav_header_t *)pkg_res->actual_data;

		BYTE *ret;
		DWORD ret_len;
		if (MySaveAsWAVE(wav_header + 1, wav_header->data_size, 
						   wav_header->wFormatTag,wav_header->nChannels, 
						   wav_header->nSamplesPerSec, wav_header->wBitsPerSample, 
						   NULL, wav_header->cbSize, &ret, 
						   &ret_len, my_malloc)) {
			delete [] pkg_res->actual_data;
			pkg_res->actual_data = NULL;
			return -CUI_EMEM;			
		}
		delete [] pkg_res->actual_data;
		pkg_res->actual_data = ret;
		pkg_res->actual_data_length = ret_len;
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".wav");
	}

	return 0;
}

/* 资源保存函数 */
static int SCI_System_bin_save_resource(struct resource *res, 
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
static void SCI_System_bin_release_resource(struct package *pkg, 
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
static void SCI_System_bin_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation SCI_System_bin_operation = {
	SCI_System_bin_match,					/* match */
	SCI_System_bin_extract_directory,		/* extract_directory */
	SCI_System_bin_parse_resource_info,		/* parse_resource_info */
	SCI_System_bin_extract_resource,		/* extract_resource */
	SCI_System_bin_save_resource,			/* save_resource */
	SCI_System_bin_release_resource,		/* release_resource */
	SCI_System_bin_release					/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK SCI_System_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".bin"), NULL, 
		NULL, &SCI_System_bin_operation, CUI_EXT_FLAG_PKG 
		| CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	return 0;
}
