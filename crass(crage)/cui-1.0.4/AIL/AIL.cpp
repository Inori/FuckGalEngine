#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>

//以下game的Sall.snl文件比较奇怪，mode后面直接是lzss压缩数据。而没有uncomprLen
//デュアルソウル
//魔女狩りの夜に

struct acui_information AIL_cui_information = {
	_T("アイル"),			/* copyright */
	_T(""),					/* system */
	_T(".dat .snl .anm"),	/* package */
	_T("1.0.2"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-5-4 18:54"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};	

#pragma pack (1)
typedef struct {
	u32 index_entries;
} dat_header_t;

typedef struct {
	u32 length;
} dat_entry_t;

typedef struct {
	u16 type;
	u32 actual_length;	// 这个length是ogg解码为wav、png解码为bmp后的长度
} dat_info_t;
#pragma pack ()

typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 length;
} my_dat_entry_t;

static void lzss_decompress(BYTE *uncompr, DWORD uncomprlen, 
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

	memset(win, ' ', nCurWindowByte);
	while (1) {
		if (curbyte >= comprlen)
			break;
		flag >>= 1;
		if (!(flag & 0x0100))
			flag = compr[curbyte++] | 0xff00;
		if (!(flag & 1)) {
			unsigned char data;

			if (curbyte >= comprlen)
				break;
			data = compr[curbyte++];
			uncompr[act_uncomprlen++] = data;
			/* 输出的1字节放入滑动窗口 */
			win[nCurWindowByte++] = data;
			nCurWindowByte &= win_size - 1;
		} else {
			unsigned int copy_bytes, win_offset;
			unsigned int i;

			if (curbyte >= comprlen)
				break;
			win_offset = compr[curbyte++];
			if (curbyte >= comprlen)
				break;
			copy_bytes = compr[curbyte++];
			win_offset |= (copy_bytes >> 4) << 8;
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
}

/********************* dat *********************/

static int AIL_dat_match(struct package *pkg)
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

	return 0;	
}

static int AIL_dat_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{
	dat_header_t dat_header;
	my_dat_entry_t *my_index_buffer;
	DWORD my_index_length;
	
	if (pkg->pio->readvec(pkg, &dat_header, sizeof(dat_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	my_index_length = dat_header.index_entries * sizeof(my_dat_entry_t);
	my_index_buffer = (my_dat_entry_t *)malloc(my_index_length);
	if (!my_index_buffer)
		return -CUI_EMEM;

	DWORD offset = 4 + dat_header.index_entries * 4;
	for (DWORD i = 0; i < dat_header.index_entries; i++) {
		if (pkg->pio->read(pkg, &my_index_buffer[i].length, 4))
			break;

		sprintf(my_index_buffer[i].name, "%08d", i);
		
		my_index_buffer[i].offset = offset;
		offset += my_index_buffer[i].length;
	}
	if (i != dat_header.index_entries) {
		free(my_index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = dat_header.index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = my_index_length;
	pkg_dir->index_entry_length = sizeof(my_dat_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

static int AIL_dat_parse_resource_info(struct package *pkg,
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

static int AIL_dat_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	BYTE tmp[8];
	dat_info_t *dat_info;
	BYTE *compr, *uncompr, *data;
	DWORD comprlen, uncomprlen;

	if (pkg->pio->readvec(pkg, tmp, sizeof(tmp), pkg_res->offset, IO_SEEK_SET))
		return -CUI_EREADVEC;

	dat_info = (dat_info_t *)tmp;
	if (!strncmp((char *)tmp, "RIFF", 4)) {
		comprlen = pkg_res->raw_data_length;
		compr = (BYTE *)pkg->pio->readvec_only(pkg, comprlen, 
			pkg_res->offset, IO_SEEK_SET);
		if (!compr)
			return -CUI_EREADVECONLY;

		uncompr = NULL;
		uncomprlen = 0;
		data = compr;
	} else if (!strncmp((char *)tmp + 4, "OggS", 4)) {
		comprlen = pkg_res->raw_data_length - 4;
		compr = (BYTE *)pkg->pio->readvec_only(pkg, comprlen, 
			pkg_res->offset + 4, IO_SEEK_SET);
		if (!compr)
			return -CUI_EREADVECONLY;

		uncompr = NULL;
		uncomprlen = 0;
		data = compr;
	} else {
		comprlen = pkg_res->raw_data_length - sizeof(dat_info_t);
		compr = (BYTE *)pkg->pio->readvec_only(pkg, comprlen, 
			pkg_res->offset + sizeof(dat_info_t), IO_SEEK_SET);
		if (!compr)
			return -CUI_EREADVECONLY;

		if ((dat_info->type & 8) || (dat_info->type & 0x10)) {// 24位PNG和32位PNG，actual_length是解压为bmp以后的长度
			uncomprlen = comprlen;
			uncompr = (BYTE *)malloc(comprlen);
			if (!uncompr)
				return -CUI_EMEM;
			memcpy(uncompr, compr, uncomprlen);
			pkg_res->flags |= PKG_RES_FLAG_REEXT;
			pkg_res->replace_extension = _T(".png");
			data = uncompr;
		} else if (dat_info->type & 6) {
			return -CUI_EMATCH;
		} else if (dat_info->type & 1) {
			uncomprlen = dat_info->actual_length;
			uncompr = (BYTE *)malloc(uncomprlen);
			if (!uncompr)
				return -CUI_EMEM;

			lzss_decompress(uncompr, uncomprlen, compr, comprlen);
			data = uncompr;
		} else {	// !(type & 1)
			uncompr = NULL;
			uncomprlen = 0;
			data = compr;
		}
	}

	if (!strncmp((char *)data, "RIFF", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".wav");
	} else if (!strncmp((char *)data, "BM", 2)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".bmp");
	} else if (!strncmp((char *)data, "OggS", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".ogg");
	} else if (!strncmp((char *)data, "\x89PNG", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".png");
	}

	pkg_res->raw_data = compr;
	pkg_res->raw_data_length = comprlen;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

static int AIL_dat_save_resource(struct resource *res, 
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

static void AIL_dat_release_resource(struct package *pkg, 
									 struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res->raw_data)
		pkg_res->raw_data = NULL;
}

static void AIL_dat_release(struct package *pkg, 
							struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {		
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg);
}

static cui_ext_operation AIL_dat_operation = {
	AIL_dat_match,				/* match */
	AIL_dat_extract_directory,	/* extract_directory */
	AIL_dat_parse_resource_info,/* parse_resource_info */
	AIL_dat_extract_resource,	/* extract_resource */
	AIL_dat_save_resource,		/* save_resource */
	AIL_dat_release_resource,	/* release_resource */
	AIL_dat_release				/* release */
};

int CALLBACK AIL_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &AIL_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".snl"), NULL, 
		NULL, &AIL_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".anm"), NULL, 
		NULL, &AIL_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	return 0;
}
