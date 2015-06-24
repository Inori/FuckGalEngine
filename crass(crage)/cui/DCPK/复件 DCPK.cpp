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
struct acui_information DCPK_cui_information = {
	NULL,					/* copyright */
	NULL,					/* system */
	_T(".TCD .TSD"),				/* package */
	_T("0.0.0"),			/* revision */
	_T("痴汉公贼"),			/* author */
	_T(""),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

/* 所有的封包特定的数据结构都要放在这个#pragma段里 */
#pragma pack (1)
typedef struct {
	u32 data_length;
	u32 index_offset;
	u32 dir_entries;
	u32 dir_name_length;
	u32 file_entries;
	u32 file_name_length;
	u32 unknown_entries;
	u32 unknown_length;
} TCD_info_t;

typedef struct {
	s8 magic[4];		// "TCD3"
	u32 unknown;		// 3
	TCD_info_t info[5];
} TCD_header_t;

typedef struct {
	u32 index_entries;
	u32 unknown[3];
} TCD_dir_info_t;
#pragma pack ()

/********************* TCD *********************/

/* 封包匹配回调函数 */
static int DCPK_TCD_match(struct package *pkg)
{
	s8 magic[4];

	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "TCD3", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

/* 封包索引目录提取函数 */
static int DCPK_TCD_extract_directory(struct package *pkg,
									  struct package_directory *pkg_dir)
{
	TCD_header_t TCD_header;	
	BYTE *dir_name_buffer = NULL;

	if (!pkg || !pkg_dir)
		return -CUI_EPARA;

	if (pkg->pio->readvec(pkg, &TCD_header, sizeof(TCD_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	for (DWORD lev = 0; lev < 5; lev++) {
		DWORD dir_name_buffer_length;
		TCD_dir_info_t *dir_info;
		DWORD dir_info_length;
		DWORD file_name_buffer_length;
		BYTE *file_name_buffer;
		u32 *file_offset_buffer;
		u32 *dir_entries_buffer;
		u32 *unknown_entries_buffer;
		u32 *file_length_buffer;
		BYTE *unknown_data;
		DWORD unknown_data_length;
		DWORD k;

		if (!lev && TCD_header.data_length) {
			if (TCD_header.file_entries >= 10240)
				return -CUI_EMATCH;
			if (TCD_header.dir_entries >= 256)
				return -CUI_EMATCH;
			if (TCD_header.dir_name_length >= 64)
				return -CUI_EMATCH;
			if (TCD_header.file_name_length >= 64)
				return -CUI_EMATCH;
            if (TCD_header.unknown_entries >= 2621440)
				return -CUI_EMATCH;
            if (TCD_header.unknown_entry_length > 32u)
				return -CUI_EMATCH;

			if (pkg->pio->seek(pkg, TCD_header.index_offset, IO_SEEK_SET))
				return -CUI_ESEEK;

			dir_name_buffer_length = TCD_header.dir_entries * TCD_header.dir_name_length;
			dir_name_buffer = (BYTE *)malloc(dir_name_buffer_length);
			if (!dir_name_buffer)
				return -CUI_EMEM;
			if (pkg->pio->read(pkg, dir_name_buffer, dir_name_buffer_length)) {
				free(dir_name_buffer);
				return -CUI_EREAD;
			}
			for (k = 0; k < dir_name_buffer_length; k++)
				dir_name_buffer[k] += 'I';
			printf("rootdir: %d subdir\n", TCD_header.dir_entries);
			dir_info_length = TCD_header.dir_entries * sizeof(TCD_dir_info_t);
			dir_info = (TCD_dir_info_t *)malloc(dir_info_length);
			if (!dir_info) {
				free(dir_name_buffer);
				return -CUI_EMEM;				
			}
			if (pkg->pio->read(pkg, dir_info, dir_info_length)) {
				free(dir_info);
				free(dir_name_buffer);
				return -CUI_EREAD;
			}
			file_name_buffer_length = TCD_header.file_entries * TCD_header.file_name_length;
			file_name_buffer = (BYTE *)malloc(file_name_buffer_length);
			if (!file_name_buffer) {
				free(dir_info);
				free(dir_name_buffer);
				return -CUI_EMEM;				
			}
			if (pkg->pio->read(pkg, file_name_buffer, file_name_buffer_length)) {
				free(file_name_buffer);
				free(dir_info);
				free(dir_name_buffer);
				return -CUI_EREAD;
			}
			for (k = 0; k < file_name_buffer_length; k++)
				file_name_buffer[k] += 'I';
			printf("rootdir: %d files\n", TCD_header.file_entries);

            file_offset_buffer = (u32 *)malloc(4 * (TCD_header.file_entries + 1));// 多一项
			if (pkg->pio->read(pkg, file_offset_buffer, 4 * (TCD_header.file_entries + 1))) {
				free(file_name_buffer);
				free(dir_info);
				free(dir_name_buffer);
				return -CUI_EREAD;
			}
			dir_entries_buffer = (u32 *)malloc(4 * TCD_header.dir_entries);
			if (!dir_entries_buffer) {
				free(file_offset_buffer);
				free(file_name_buffer);
				free(dir_info);
				free(dir_name_buffer);
				return -CUI_EREAD;
			}
			for (DWORD j = 0; j < TCD_header.dir_entries; j++)
				dir_entries_buffer[j] = dir_info[j].index_entries + 1;

            file_length_buffer = (u32 *)malloc(4 * (TCD_header.file_entries + 1));
			if (!file_length_buffer) {
				free(dir_entries_buffer);
				free(file_offset_buffer);
				free(file_name_buffer);
				free(dir_info);
				free(dir_name_buffer);
				return -CUI_EMEM;
			}
			if (pkg->pio->read(pkg, file_length_buffer, 4 * (TCD_header.file_entries + 1))) {
				free(file_length_buffer);
				free(dir_entries_buffer);
				free(file_offset_buffer);
				free(file_name_buffer);
				free(dir_info);
				free(dir_name_buffer);
				return -CUI_EREAD;
			}

            unknown_entries_buffer = (u32 *)malloc(4 * TCD_header.unknown_entries);
			if (!unknown_entries_buffer) {
				free(dir_entries_buffer);
				free(file_offset_buffer);
				free(file_name_buffer);
				free(dir_info);
				free(dir_name_buffer);
				return -CUI_EMEM;
			}
			if (pkg->pio->read(pkg, unknown_entries_buffer, 4 * TCD_header.unknown_entries)) {
				free(unknown_entries_buffer);
				free(file_length_buffer);
				free(dir_entries_buffer);
				free(file_offset_buffer);
				free(file_name_buffer);
				free(dir_info);
				free(dir_name_buffer);
				return -CUI_EREAD;
			}
	
            unknown_data_length = TCD_header.unknown_entries * TCD_header.unknown_entry_length;
            unknown_data = (BYTE *)malloc(unknown_data_length);
			if (!unknown_data) {
				free(unknown_entries_buffer);
				free(file_length_buffer);
				free(dir_entries_buffer);
				free(file_offset_buffer);
				free(file_name_buffer);
				free(dir_info);
				free(dir_name_buffer);
				return -CUI_EMEM;
			}
			if (pkg->pio->read(pkg, unknown_data, unknown_data_length)) {
				free(unknown_data);
				free(unknown_entries_buffer);
				free(file_length_buffer);
				free(dir_entries_buffer);
				free(file_offset_buffer);
				free(file_name_buffer);
				free(dir_info);
				free(dir_name_buffer);
				return -CUI_EREAD;
			}
            for (k = 0; k < unknown_data_length; k++)
				unknown_data[k] -= 0x1c;

            u32 *file_entries2_buffer = (u32 *)malloc(4 * (TCD_header.file_entries + 1));
			if (!file_entries2_buffer) {
				free(unknown_data);
				free(unknown_entries_buffer);
				free(file_length_buffer);
				free(dir_entries_buffer);
				free(file_offset_buffer);
				free(file_name_buffer);
				free(dir_info);
				free(dir_name_buffer);
				return -CUI_EMEM;
			}
			if (pkg->pio->read(pkg, file_entries2_buffer, 4 * (TCD_header.file_entries + 1))) {
				free(file_entries2_buffer);
				free(unknown_data);
				free(unknown_entries_buffer);
				free(file_length_buffer);
				free(dir_entries_buffer);
				free(file_offset_buffer);
				free(file_name_buffer);
				free(dir_info);
				free(dir_name_buffer);
				return -CUI_EREAD;
			}
            if (file_entries2_buffer[TCD_header.file_entries] >= 262144) {
				free(file_entries2_buffer);
				free(unknown_data);
				free(unknown_entries_buffer);
				free(file_length_buffer);
				free(dir_entries_buffer);
				free(file_offset_buffer);
				free(file_name_buffer);
				free(dir_info);
				free(dir_name_buffer);
				return -CUI_EMATCH;
			}

            u32 *dword_4D09D8 = (u32 *)malloc(4 * TCD_header.file_entries);
			if (!dword_4D09D8) {
				free(file_entries2_buffer);
				free(unknown_data);
				free(unknown_entries_buffer);
				free(file_length_buffer);
				free(dir_entries_buffer);
				free(file_offset_buffer);
				free(file_name_buffer);
				free(dir_info);
				free(dir_name_buffer);
				return -CUI_EMEM;
			}
			DWORD l;
			DWORD v22 = 0;
			for (k = 0, l = 0; k < TCD_header.unknown_entries; k++) {
				if (file_length_buffer[l] == k)
					dword_4D09D8[l++] = v22;
                v22 += TCD_header.unknown_entry_length;	
			}

			MySaveFile(_T("dir_name"), dir_name_buffer, dir_name_buffer_length);
MySaveFile(_T("dir_info"), dir_info, dir_info_length);
MySaveFile(_T("file_name"), file_name_buffer, file_name_buffer_length);
MySaveFile(_T("file_offset"), file_offset_buffer, 4 * (TCD_header.file_entries + 1));
MySaveFile(_T("file_length"), file_length_buffer, 4 * (TCD_header.file_entries + 1));
MySaveFile(_T("unknown_entries"), unknown_entries_buffer, 4 * TCD_header.unknown_entries);
MySaveFile(_T("unknown_data"), unknown_data, unknown_data_length);

		} else {
			if (TCD_header.info[i].data_length) {
				if (TCD_header.info[i].dir_entries >= 256)
					return -CUI_EMATCH;
				if (TCD_header.info[i]file_entries >= 10240)
					return -CUI_EMATCH;
				if (TCD_header.info[i]dir_name_length >= 64)
					return -CUI_EMATCH;
				if (TCD_header.info[i]file_name_length >= 64)
					return -CUI_EMATCH;


								TCD_header.info[i].dir_entries;
			}
		}
	}

//	pkg_dir->index_entries = TCD_header.index_entries;
//	pkg_dir->directory = index_buffer;
//	pkg_dir->directory_length = index_buffer_length;
//	pkg_dir->index_entry_length = sizeof(arc_entry_t);

	return 0;
}

/* 封包索引项解析函数 */
static int DCPK_TCD_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
#if 0
	arc_entry_t *arc_entry;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	arc_entry = (arc_entry_t *)pkg_res->actual_index_entry;
	strncpy(pkg_res->name, arc_entry->name, 64);
	pkg_res->name[64] = 0;
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = arc_entry->comprlen;
	if (arc_entry->is_compressed)
		pkg_res->actual_data_length = arc_entry->uncomprlen;
	else
		pkg_res->actual_data_length = 0;	
	pkg_res->offset = arc_entry->offset;
#endif
	return 0;
}

/* 封包资源提取函数 */
static int DCPK_TCD_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
#if 0
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)pkg->pio->readvec_only(pkg, comprlen, pkg_res->offset, IO_SEEK_SET);
	if (!compr)
		return -CUI_EREADVECONLY;

	uncompr = NULL;
	uncomprlen = pkg_res->actual_data_length;
	if (uncomprlen) {
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr)
			return -CUI_EMEM;

		memset(uncompr, 0xdd, uncomprlen);
		lzss_uncompress(uncompr, uncomprlen, compr, comprlen);
	}
	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;
#endif
	return 0;
}

/* 资源保存函数 */
static int DCPK_TCD_save_resource(struct resource *res, 
								  struct package_resource *pkg_res)
{
	if (!res || !pkg_res)
		return -CUI_EPARA;
	
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
static void DCPK_TCD_release_resource(struct package *pkg, 
									  struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}

	if (pkg_res->raw_data)
		pkg_res->raw_data = NULL;
}

/* 封包卸载函数 */
static void DCPK_TCD_release(struct package *pkg, 
							 struct package_directory *pkg_dir)
{
	if (!pkg || !pkg_dir)
		return;

	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation DCPK_TCD_operation = {
	DCPK_TCD_match,					/* match */
	DCPK_TCD_extract_directory,		/* extract_directory */
	DCPK_TCD_parse_resource_info,	/* parse_resource_info */
	DCPK_TCD_extract_resource,		/* extract_resource */
	DCPK_TCD_save_resource,			/* save_resource */
	DCPK_TCD_release_resource,		/* release_resource */
	DCPK_TCD_release				/* release */
};

/********************* TSD *********************/

/* 封包匹配回调函数 */
static int DCPK_TSD_match(struct package *pkg)
{
	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包资源提取函数 */
static int DCPK_TSD_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{
	BYTE *tsd;
	DWORD tsd_size;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	if (pkg->pio->length_of(pkg, &tsd_size))
		return -CUI_ELEN;

	tsd = (BYTE *)malloc(tsd_size);
	if (!tsd)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, tsd, tsd_size, 0, IO_SEEK_SET)) {
		free(tsd);
		return -CUI_EREADVEC;
	}

	for (DWORD i = 0; i < tsd_size; i++)
		tsd[i] = (tsd[i] << 7) | (tsd[i] >> 1);

	pkg_res->raw_data = tsd;
	pkg_res->raw_data_length = tsd_size;

	return 0;
}

/* 资源保存函数 */
static int DCPK_TSD_save_resource(struct resource *res, 
								  struct package_resource *pkg_res)
{
	if (!res || !pkg_res)
		return -CUI_EPARA;
	
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
static void DCPK_TSD_release_resource(struct package *pkg, 
									  struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}

	if (pkg_res->raw_data)
		pkg_res->raw_data = NULL;
}

/* 封包卸载函数 */
static void DCPK_TSD_release(struct package *pkg, 
							 struct package_directory *pkg_dir)
{
	if (!pkg)
		return;
	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation DCPK_TSD_operation = {
	DCPK_TSD_match,				/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	DCPK_TSD_extract_resource,	/* extract_resource */
	DCPK_TSD_save_resource,		/* save_resource */
	DCPK_TSD_release_resource,	/* release_resource */
	DCPK_TSD_release			/* release */
};

/* 接口函数: 向cui_core注册支持的封包类型 */
int CALLBACK DCPK_register_cui(struct cui_register_callback *callback)
{
	/* 注册cui插件支持的扩展名、资源放入扩展名、处理回调函数和封包属性 */
	if (callback->add_extension(callback->cui, _T(".TCD"), NULL, 
		NULL, &DCPK_TCD_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".TSD"), NULL, 
		NULL, &DCPK_TSD_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	return 0;
}
