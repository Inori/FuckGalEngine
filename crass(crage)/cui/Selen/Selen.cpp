#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <zlib.h>
#include <stdio.h>

struct acui_information Selen_cui_information = {
	_T(""),		/* copyright */
	NULL,					/* system */
	_T(".exe"),				/* package */
	_T("0.1.0"),			/* revision */
	_T("痴汉公贼"),			/* author */
	_T("2007-10-22 18:07"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};	

#pragma pack (1)
typedef struct {
	s8 magic[4];	// "00JP" or "10JP"
	u32 xfir_offset;
	u32 unknown0;
	u32 unknown1;	// ?
	u32 total_compents;		// 总的组件数量，包括外部组件（和exe在同级目录）以及内藏组件（exe内）
	u32 internal_compents;	// 内藏组件点的个数
} exe_header_t;

// 前total_compents-internal_compents个组件属于外部组件
typedef struct {
	u32 offset;
	u32 length;
	s8 name[33];
	s8 short_suffix[4];	// 0x7a
	s8 long_suffix[7];
} exe_entry_t;
#pragma pack ()

extern int lzari_decompress(unsigned char *data_in, unsigned char *cpage_out, unsigned int srclen, unsigned int destlen);

/********************* exe *********************/

static int Selen_exe_match(struct package *pkg)
{
	u32 offset;
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->seek(pkg, -4, IO_SEEK_END)) {
		pkg->pio->close(pkg);
		return -CUI_ESEEK;
	}
	
	if (pkg->pio->read(pkg, &offset, 4)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}
	
	if (pkg->pio->seek(pkg, offset, IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_ESEEK;
	}
	
	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}
	
	if (strncmp(magic, "10JP", 4) && strncmp(magic, "00JP", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int Selen_exe_extract_directory(struct package *pkg,
									   struct package_directory *pkg_dir)
{
	exe_header_t exe_header;
	exe_entry_t *index_buffer;
	DWORD index_length;

	if (pkg->pio->read(pkg, (BYTE *)&exe_header + 4, sizeof(exe_header) - 4))
		return -CUI_EREAD;

	index_length = exe_header.index_entries * sizeof(exe_entry_t);
	index_buffer = (exe_entry_t *)malloc(index_length);
	if (!index_buffer)
		return -CUI_EMEM;
	
	if (pkg->pio->read(pkg, index_buffer, index_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = exe_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_length;
	pkg_dir->index_entry_length = sizeof(exe_entry_t);

	return 0;
}

static int Selen_exe_parse_resource_info(struct package *pkg,
										  struct package_resource *pkg_res)
{
	exe_entry_t *exe_entry;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	exe_entry = (exe_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, exe_entry->name);
	strcat(pkg_res->name, ".");
	strcat(pkg_res->name, exe_entry->suffix);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = exe_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = exe_entry->offset;

	return 0;
}

static int Selen_exe_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;
	
	if (!pkg || !pkg_res)
		return -CUI_EPARA;
	
	comprlen = pkg_res->raw_data_length - 4;
	if (pkg->pio->readvec(pkg, &uncomprlen, 4, pkg_res->offset, IO_SEEK_SET))
		return -CUI_EREADVEC;
	
	compr = (BYTE *)pkg->pio->readvec_only(pkg, comprlen, pkg_res->offset + 4, IO_SEEK_SET);
	if (!compr)
		return -CUI_EREADVECONLY;

	if (uncomprlen) {	
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr)
			return -CUI_EMEM;

		if (lzari_decompress(compr, uncompr, comprlen, uncomprlen)) {
			free(uncompr);
			return -CUI_EUNCOMPR;
		}
	} else {
		uncompr = NULL;
		uncomprlen = 0;
	}

	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}
static int Selen_exe_save_resource(struct resource *res, 
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

static void Selen_exe_release_resource(struct package *pkg, 
										struct package_resource *pkg_res)
{
	if (!pkg)
		return;

	if (pkg_res && pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res && pkg_res->raw_data)
		pkg_res->raw_data = NULL;
}

static void Selen_exe_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (!pkg || !pkg_dir)
		return;

	if (pkg_dir->directory && pkg_dir->directory_length) {		
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg);
}

static cui_ext_operation Selen_exe_operation = {
	Selen_exe_match,					/* match */
	Selen_exe_extract_directory,		/* extract_directory */
	Selen_exe_parse_resource_info,		/* parse_resource_info */
	Selen_exe_extract_resource,			/* extract_resource */
	Selen_exe_save_resource,			/* save_resource */
	Selen_exe_release_resource,			/* release_resource */
	Selen_exe_release					/* release */
};

int CALLBACK Selen_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".exe"), NULL, 
		NULL, &Selen_exe_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
