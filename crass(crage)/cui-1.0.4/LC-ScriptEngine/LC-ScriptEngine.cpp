#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <resource.h>
#include <package.h>
#include <cui_error.h>
#include <stdio.h>
#include <utility.h>

struct acui_information LC_ScriptEngine_cui_information = {
	_T("TacticsNet"),		/* copyright */
	_T("LC-ScriptEngine"),	/* system */
	_T(".lst lcbody*"),		/* package */
	_T("1.0.2"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-10-23 20:21"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};

#pragma pack (1)
typedef struct {
	u32 index_entries;		
} lst_header_t;

typedef struct {
	u32 offset;
	u32 length;
	s8 name[64];
	s32 type;
} lst_entry_t;

typedef struct {
	u32 instructs;
	u32 text_length;	
} snx_header_t;
#pragma pack ()

static const TCHAR *suffix[6] = {
	_T(".lst"),
	_T(".SNX"),
	_T(".BMP"),
	_T(".PNG"),
	_T(".WAV"),
	_T(".OGG")
};

static u32 xor_magic;	// 旧版本是0x01010101，新版本是0x02020202

/*********************  *********************/

static int LC_ScriptEngine_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->open(pkg->lst, IO_READONLY)) {
		pkg->pio->close(pkg);
		return -CUI_EOPEN;
	}

	if (pkg->pio->readvec(pkg->lst, &xor_magic, 4, 4, IO_READONLY)) {
		pkg->pio->close(pkg->lst);
		pkg->pio->close(pkg);
		return -CUI_EREADVEC;
	}

	return 0;	
}

static int LC_ScriptEngine_extract_directory(struct package *pkg,
											 struct package_directory *pkg_dir)
{		
	lst_header_t lst_header;
	lst_entry_t *index_buffer;
	unsigned int index_buffer_len;

	if (pkg->pio->readvec(pkg->lst, &lst_header, sizeof(lst_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	lst_header.index_entries ^= xor_magic;

	index_buffer_len = lst_header.index_entries * sizeof(lst_entry_t);
	index_buffer = (lst_entry_t *)malloc(index_buffer_len);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg->lst, index_buffer, index_buffer_len)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = lst_header.index_entries;
	pkg_dir->index_entry_length = sizeof(lst_entry_t);
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_len;

	return 0;
}

static int LC_ScriptEngine_parse_resource_info(struct package *pkg,
											   struct package_resource *pkg_res)
{
	lst_entry_t *lst_entry;
	int str_len;

	lst_entry = (lst_entry_t *)pkg_res->actual_index_entry;

	str_len = strlen((char *)lst_entry->name);
	for (int i = 0; i < str_len; ++i)
		lst_entry->name[i] ^= xor_magic;
	lst_entry->offset ^= xor_magic;
	lst_entry->length ^= xor_magic;
	// for type == -1
	if (lst_entry->type < 0)
		lst_entry->type = -lst_entry->type;

	strcpy(pkg_res->name, lst_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = lst_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = lst_entry->offset;

	return 0;
}

static int LC_ScriptEngine_extract_resource(struct package *pkg,
											struct package_resource *pkg_res)
{
	lst_entry_t *lst_entry;	

	pkg_res->raw_data = malloc(pkg_res->raw_data_length);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EREADVEC;
	}

	lst_entry = (lst_entry_t *)pkg_res->actual_index_entry;
	if (lst_entry->type == 1) {
		/* 文本定位是在前半部分的指令区，由11 00 00 00 02 00 00 00 xx xx xx xx 指令标示，
		 * xx是偏移值（从0开始）
		 */
		for (unsigned int i = 0; i < lst_entry->length; ++i)
			((u8 *)pkg_res->raw_data)[i] ^= xor_magic + 1;
	}
	pkg_res->replace_extension = suffix[lst_entry->type];
	pkg_res->flags |= PKG_RES_FLAG_REEXT;

	return 0;
}

static int LC_ScriptEngine_save_resource(struct resource *res, 
										 struct package_resource *pkg_res)
{
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}
	res->rio->close(res);
	
	return 0;
}

static void LC_ScriptEngine_release_resource(struct package *pkg, 
											 struct package_resource *pkg_res)
{
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void LC_ScriptEngine_release(struct package *pkg, 
									struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg->lst);
	pkg->pio->close(pkg);
}

static cui_ext_operation LC_ScriptEngine_operation = {
	LC_ScriptEngine_match,					/* match */
	LC_ScriptEngine_extract_directory,		/* extract_directory */
	LC_ScriptEngine_parse_resource_info,	/* parse_resource_info */
	LC_ScriptEngine_extract_resource,		/* extract_resource */
	LC_ScriptEngine_save_resource,			/* save_resource */
	LC_ScriptEngine_release_resource,		/* release_resource */
	LC_ScriptEngine_release					/* release */
};

int CALLBACK LC_ScriptEngine_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, NULL, NULL, 
		NULL, &LC_ScriptEngine_operation, 
			CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_LST | CUI_EXT_FLAG_NOEXT | CUI_EXT_FLAG_NO_MAGIC))
				return -1;

	return 0;
}
