#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>

struct acui_information GameScripter_cui_information = {
	_T(""),					/* copyright */
	_T(""),					/* system */
	_T(".pak"),				/* package */
	_T("1.0.2"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2009-2-25 19:21"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_STABLE
};	

#pragma pack (1)
typedef struct {
	s8 magic[6];		// "\x5PACK2"
	u32 index_entries;	// 每项32字节
} pak_header_t;

typedef struct {
	u8 name_length;
	s8 *name;
	u32 offset;
	u32 length;
} pak_entry_t;

typedef struct {
	s8 magic[3];		// "PGA"
	u8 ihdr[20];
	u32 ihdr_crc;
} pga_header_t;

typedef struct {
	s8 magic[4];		// "char"
	u32 offset;			// face图的offset
} chr_header_t;

// 第一项前面有u32 index_entries;	// 表情个数
typedef struct {	
	u8 name_length;		// 包括NULL
	s8 *name;
	u32 length;
	u16 top_x;		// 0
	u16 top_y;		// 0
	u16 bottom_x;	// width
	u16 bottom_y;	// height
} chr_entry_t;
#pragma pack ()

typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 length;
} my_pak_entry_t;

typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 length;
} my_chr_entry_t;

/********************* pak *********************/

static int GameScripter_pak_match(struct package *pkg)
{
	s8 magic[6];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}
	
	if (strncmp(magic, "\x5PACK2", 6)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int GameScripter_pak_extract_directory(struct package *pkg,
											  struct package_directory *pkg_dir)
{
	u32 index_entries;
	my_pak_entry_t *index_buffer;
	DWORD index_length;
	
	if (pkg->pio->read(pkg, &index_entries, 4))
		return -CUI_EREAD;

	index_length = index_entries * sizeof(my_pak_entry_t);
	index_buffer = (my_pak_entry_t *)malloc(index_length);
	if (!index_buffer)
		return -CUI_EMEM;

	for (DWORD i = 0; i < index_entries; i++) {
		u8 name_length;

		if (pkg->pio->read(pkg, &name_length, 1))
			break;
		
		if (pkg->pio->read(pkg, &index_buffer[i].name, name_length))
			break;
		index_buffer[i].name[name_length] = 0;
		
		for (u8 k = 0; k < name_length; k++)
			index_buffer[i].name[k] = ~index_buffer[i].name[k];
		
		if (pkg->pio->read(pkg, &index_buffer[i].offset, 4))
			break;

		if (pkg->pio->read(pkg, &index_buffer[i].length, 4))
			break;
	}
	if (i != index_entries) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_length;
	pkg_dir->index_entry_length = sizeof(my_pak_entry_t);

	return 0;
}

static int GameScripter_pak_parse_resource_info(struct package *pkg,
												struct package_resource *pkg_res)
{
	my_pak_entry_t *my_pak_entry;

	my_pak_entry = (my_pak_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_pak_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_pak_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_pak_entry->offset;

	return 0;
}

static int GameScripter_pak_extract_resource(struct package *pkg,
											 struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen,	pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = compr;
		return 0;
	}
	
	uncompr = NULL;
	uncomprlen = 0;
	if (!strncmp((char *)compr, "PGA", 3)) {
		pga_header_t *pga_header = (pga_header_t *)compr;
		u8 pga_code[] = "PGAECODE";
		
		for (int i = 0; i < 8; i++)
			pga_header->ihdr[i] ^= pga_code[i];

		uncomprlen = comprlen + 5;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}
		memcpy(uncompr, "\x89PNG\xd\xa\x1a\xa", 8);
		memcpy(uncompr + 8, pga_header->ihdr, comprlen - 3);
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".png");
	} else if (!strncmp((char *)compr, "char", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".chr");
	} else if (!strncmp((char *)compr, "OggS", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".ogg");
	} else if (!strncmp((char *)compr, "jga", 3)) {
		uncomprlen = comprlen - 6;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}
		memcpy(uncompr, compr + 6, uncomprlen);
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
		pkg_res->replace_extension = _T(".jpg");
	}

	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

static int GameScripter_pak_save_resource(struct resource *res, 
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

static void GameScripter_pak_release_resource(struct package *pkg, 
											  struct package_resource *pkg_res)
{
	if (pkg_res && pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res && pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void GameScripter_pak_release(struct package *pkg, 
									 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory && pkg_dir->directory_length) {		
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg);
}

static cui_ext_operation GameScripter_pak_operation = {
	GameScripter_pak_match,					/* match */
	GameScripter_pak_extract_directory,		/* extract_directory */
	GameScripter_pak_parse_resource_info,	/* parse_resource_info */
	GameScripter_pak_extract_resource,		/* extract_resource */
	GameScripter_pak_save_resource,			/* save_resource */
	GameScripter_pak_release_resource,		/* release_resource */
	GameScripter_pak_release				/* release */
};

/********************* chr *********************/

static int GameScripter_chr_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}
	
	if (strncmp(magic, "char", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int GameScripter_chr_extract_directory(struct package *pkg,
											  struct package_directory *pkg_dir)
{
	u32 offset, index_entries;
	my_chr_entry_t *index_buffer;
	DWORD index_length;
	
	if (pkg->pio->read(pkg, &offset, 4))
		return -CUI_EREAD;

	if (pkg->pio->readvec(pkg, &index_entries, 4, offset, IO_SEEK_SET))
		return -CUI_EREADVEC;

	index_entries++;
	index_length = index_entries * sizeof(my_chr_entry_t);
	index_buffer = (my_chr_entry_t *)malloc(index_length);
	if (!index_buffer)
		return -CUI_EMEM;

	strcpy(index_buffer->name, "0.png");
	index_buffer->offset = 0;
	index_buffer->length = offset;
	offset += 4;

	for (DWORD i = 1; i < index_entries; i++) {
		u8 name_length;

		if (pkg->pio->readvec(pkg, &name_length, 1, offset, IO_SEEK_SET))
			break;
		
		if (pkg->pio->read(pkg, &index_buffer[i].name, name_length))
			break;

		if (pkg->pio->read(pkg, &index_buffer[i].length, 4))
			break;

		if (index_buffer[i].length > 8) {
			offset += 1 + name_length + 4;
			index_buffer[i].offset = offset;
			offset += index_buffer[i].length;
		} else {
			index_entries--;
			i--;
		}
	}
	if (i != index_entries) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_entries * sizeof(my_chr_entry_t);
	pkg_dir->index_entry_length = sizeof(my_chr_entry_t);

	return 0;
}

static int GameScripter_chr_parse_resource_info(struct package *pkg,
												struct package_resource *pkg_res)
{
	my_chr_entry_t *my_chr_entry;

	my_chr_entry = (my_chr_entry_t *)pkg_res->actual_index_entry;
	strncpy(pkg_res->name, my_chr_entry->name, 16);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_chr_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_chr_entry->offset;

	return 0;
}

static int GameScripter_chr_extract_resource(struct package *pkg,
											 struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen,	pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	uncomprlen = comprlen + 12;	/* IEND */
	uncompr = (BYTE *)malloc(uncomprlen);
	if (!uncompr) {
		free(compr);
		return -CUI_EMEM;
	}
	memcpy(uncompr, "\x89PNG\xd\xa\x1a\xa", 8);
	memcpy(uncompr + 8, compr + 8, comprlen - 8);
	memcpy(uncompr + comprlen, "\x0\x0\x0\x0IEND\xae\x42\x60\x82", 12);
	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

static int GameScripter_chr_save_resource(struct resource *res, 
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

static void GameScripter_chr_release_resource(struct package *pkg, 
											  struct package_resource *pkg_res)
{
	if (pkg_res && pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res && pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void GameScripter_chr_release(struct package *pkg, 
									 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory && pkg_dir->directory_length) {		
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg);
}

static cui_ext_operation GameScripter_chr_operation = {
	GameScripter_chr_match,					/* match */
	GameScripter_chr_extract_directory,		/* extract_directory */
	GameScripter_chr_parse_resource_info,	/* parse_resource_info */
	GameScripter_chr_extract_resource,		/* extract_resource */
	GameScripter_chr_save_resource,			/* save_resource */
	GameScripter_chr_release_resource,		/* release_resource */
	GameScripter_chr_release				/* release */
};

int CALLBACK GameScripter_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".pak"), NULL, 
		NULL, &GameScripter_pak_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".chr"), _T(".png"), 
		NULL, &GameScripter_chr_operation, CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
