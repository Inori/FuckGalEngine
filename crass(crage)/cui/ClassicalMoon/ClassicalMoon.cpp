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
#include <openssl/md5.h>
#include <bzlib.h>

struct acui_information ClassicalMoon_cui_information = {
	_T("绝情电脑游戏创作群"),	/* copyright */
	_T("《古月》引擎"),			/* system */
	_T(".HAC *.HacPack"),		/* package */
	_T("1.0.0"),				/* revision */
	_T("痴漢公賊"),				/* author */
	_T("2007-10-22 18:07"),		/* date */
	NULL,						/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};	

#pragma pack (1)
typedef struct {
	s8 magic[6];		// "HAC-3\x1a"
	

} HAC_header_t;

typedef struct {
	u32 offset;
	u32 length;
} afs_info_t;

typedef struct {
	s8 name[32];
	u16 year;
	u16 month;
	u16 day;
	u16 hour;
	u16 minute;
	u16 second;
	u32 junk;
} afs_entry_t;

//P2T:pal_index(%d) is not the multiple of 256(para is 0x1000)
typedef struct {
	u32 header_size;
	u32 aligned_size;
	u32 index_offset;
	u32 index_entries;	//P2T:It is a larger value than the texture contained(must > 0)
	u32 data_offset;
	u32 reserved[3];
} p2t_header_t;

typedef struct {
	u32 tim2_size;
} klz_header_t;
#pragma pack ()

typedef struct {
	WCHAR name[MAX_PATH];
	WCHAR alg[MAX_PATH];
	BYTE md5[16];
	u32 offset;
	u32 comprlen;
	u32 uncomprlen;
	u32 flags;
} my_HAC_entry_t;

static void *my_malloc(DWORD len)
{
	return malloc(len);
}

/********************* HAC *********************/

static int ClassicalMoon_HAC_match(struct package *pkg)
{
	s8 magic[6];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "HAC-3\x1a", sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int ClassicalMoon_HAC_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{
	u32 index_offset, root_dir_name_len, dirs, index_entries, name_wchar_length;
	DWORD my_index_length;
	my_HAC_entry_t *my_index_buffer;
	WCHAR *root_dir_name;

	if (pkg->pio->readvec(pkg, &index_offset, 4, -4, IO_SEEK_END))
		return -CUI_EREADVEC;
	
	if (pkg->pio->readvec(pkg, &name_wchar_length, 4, index_offset, IO_SEEK_SET))
		return -CUI_EREADVEC;

	root_dir_name_len = name_wchar_length * 2;
	root_dir_name = (WCHAR *)malloc(root_dir_name_len + 2);
	if (!root_dir_name)
		return -CUI_EMEM;
	
	if (pkg->pio->read(pkg, root_dir_name, root_dir_name_len)) {
		free(root_dir_name);
		return -CUI_EREAD;
	}
	root_dir_name[root_dir_name_len] = 0;

	if (wcscmp(root_dir_name, L"root")) {
		free(root_dir_name);
		return -CUI_EMATCH;		
	}
	free(root_dir_name);
	
	if (pkg->pio->read(pkg, &dirs, 4))	/* 当前目录下子目录的个数 */
		return -CUI_EREAD;
	
	if (pkg->pio->read(pkg, &index_entries, 4))
		return -CUI_EREAD;

	my_index_length = index_entries * sizeof(my_HAC_entry_t);
	my_index_buffer = (my_HAC_entry_t *)malloc(my_index_length);
	if (!my_index_buffer)
		return -CUI_EMEM;

	for (DWORD i = 0; i < index_entries; i++) {
		u32 compralg_name_len;

		if (pkg->pio->read(pkg, &name_wchar_length, 4))
			break;
		
		if (pkg->pio->read(pkg, my_index_buffer[i].name, name_wchar_length * 2))
			break;
		my_index_buffer[i].name[name_wchar_length] = 0;
		
		if (pkg->pio->read(pkg, &compralg_name_len, 4))
			break;
		
		if (compralg_name_len) {
			if (pkg->pio->read(pkg, my_index_buffer[i].alg, compralg_name_len * 2))
				break;
			my_index_buffer[i].alg[compralg_name_len] = 0;
		}
		
		if (pkg->pio->read(pkg, &my_index_buffer[i].comprlen, 4))
			break;
		
		if (pkg->pio->read(pkg, &my_index_buffer[i].uncomprlen, 4))
			break;
		
		if (pkg->pio->read(pkg, my_index_buffer[i].md5, 16))
			break;

		if (pkg->pio->read(pkg, &my_index_buffer[i].offset, 4))
			break;
	}
	if (i != index_entries) {
		free(my_index_buffer);
		return -CUI_EREAD;
	}
	
	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = my_index_length;
	pkg_dir->index_entry_length = sizeof(my_HAC_entry_t);

	return 0;
}

static int ClassicalMoon_HAC_parse_resource_info(struct package *pkg,
									   struct package_resource *pkg_res)
{
	my_HAC_entry_t *my_HAC_entry;

	my_HAC_entry = (my_HAC_entry_t *)pkg_res->actual_index_entry;
	wcscpy((WCHAR *)pkg_res->name, my_HAC_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_HAC_entry->comprlen;
	pkg_res->actual_data_length = my_HAC_entry->uncomprlen;	/* 数据都是明文 */
	pkg_res->offset = my_HAC_entry->offset;
	pkg_res->flags = PKG_RES_FLAG_UNICODE;

	return 0;
}

static int ClassicalMoon_HAC_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)pkg->pio->readvec_only(pkg, comprlen, pkg_res->offset, IO_SEEK_SET);
	if (!compr)
		return -CUI_EREADVECONLY;

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = compr;
		return 0;
	}

	BYTE md5[16];
	MD5(compr, comprlen, md5);
	my_HAC_entry_t *my_HAC_entry = (my_HAC_entry_t *)pkg_res->actual_index_entry;
	if (memcmp(my_HAC_entry->md5, md5, 16))
		return -CUI_EMATCH;

	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr = NULL;
	pkg_res->actual_data_length = uncomprlen = 0;

	return 0;
}

static int ClassicalMoon_HAC_save_resource(struct resource *res, 
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

static void ClassicalMoon_HAC_release_resource(struct package *pkg, 
									 struct package_resource *pkg_res)
{
	if (pkg_res && pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res && pkg_res->raw_data)
		pkg_res->raw_data = NULL;
}

static void ClassicalMoon_HAC_release(struct package *pkg, 
							struct package_directory *pkg_dir)
{
	if (pkg_dir->directory && pkg_dir->directory_length) {		
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg);
}

static cui_ext_operation ClassicalMoon_HAC_operation = {
	ClassicalMoon_HAC_match,				/* match */
	ClassicalMoon_HAC_extract_directory,	/* extract_directory */
	ClassicalMoon_HAC_parse_resource_info,	/* parse_resource_info */
	ClassicalMoon_HAC_extract_resource,		/* extract_resource */
	ClassicalMoon_HAC_save_resource,		/* save_resource */
	ClassicalMoon_HAC_release_resource,		/* release_resource */
	ClassicalMoon_HAC_release				/* release */
};

int CALLBACK ClassicalMoon_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".HAC"), NULL, 
		NULL, &ClassicalMoon_HAC_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;
	
	return 0;
}
