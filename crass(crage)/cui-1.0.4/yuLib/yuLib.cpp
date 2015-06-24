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
	
/*找密钥的方法：
用常量'n'做搜索，凡是MOV BYTE [], 6e的都有可能。 
*/
struct acui_information yuLib_cui_information = {
	_T(""),					/* copyright */
	NULL,					/* system */
	_T(".arc .kthb .kmap"),	/* package */
	_T("1.0.2"),			/* revision */
	_T("痴汉公贼"),			/* author */
	_T("2007-11-24 21:53"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};	

#pragma pack (1)
typedef struct {
	u32 index_entries;
} arc_header_t;

typedef struct {
	s8 *name;
	u32 offset_lo;
	u32 offset_hi;
} arc_entry_t;
#pragma pack ()

typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 length;
} my_arc_entry_t;

static void decode(BYTE *buf, int len, const char *cipher)
{
	int cipher_len = strlen(cipher);
	int j = 0;

	for (int i = 0; i < len; i++) {
		for (int k = 0; k < cipher_len; k++)
			buf[i] ^= cipher[k];
		if (j >= 4)
			j = 0;
		buf[i] ^= cipher[j++];
	}
}

/********************* arc *********************/

static int yuLib_arc_match(struct package *pkg)
{
	if (!pkg)
		return -CUI_EPARA;
	
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;	

	return 0;
}

static int yuLib_arc_extract_directory(struct package *pkg,
									   struct package_directory *pkg_dir)
{
	arc_header_t arc_header;
	my_arc_entry_t *index_buffer;
	DWORD index_length;
	SIZE_T fsize;
	
	if (!pkg || !pkg_dir)
		return -CUI_EPARA;
	
	if (pkg->pio->length_of(pkg, &fsize))
		return -CUI_ELEN;

	if (pkg->pio->readvec(pkg, &arc_header, sizeof(arc_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;
	
	index_length = arc_header.index_entries * sizeof(my_arc_entry_t);
	index_buffer = (my_arc_entry_t *)malloc(index_length);
	if (!index_buffer)
		return -CUI_EMEM;
	
	for (DWORD i = 0; i < arc_header.index_entries; i++) {
		my_arc_entry_t *entry = &index_buffer[i];
		DWORD name_len, offset_hi;

		for (name_len = 0; name_len < MAX_PATH; name_len++) {
			if (pkg->pio->read(pkg, &entry->name[name_len], 1)) {
				free(index_buffer);
				return -CUI_EREAD;
			}
			if (!entry->name[name_len])
				break;
		}
		if (name_len == MAX_PATH)
			break;
		
		if (pkg->pio->read(pkg, &entry->offset, 4))
			break;

		if (pkg->pio->read(pkg, &offset_hi, 4))
			break;
	}
	if (i != arc_header.index_entries) {
		free(index_buffer);
		return -CUI_EREAD;
	}
	
	for (i = 0; i < arc_header.index_entries - 1; i++)
		index_buffer[i].length = index_buffer[i + 1].offset - index_buffer[i].offset;
	index_buffer[i].length = fsize - index_buffer[i].offset;

	pkg_dir->index_entries = arc_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_length;
	pkg_dir->index_entry_length = sizeof(my_arc_entry_t);

	return 0;
}

static int yuLib_arc_parse_resource_info(struct package *pkg,
										 struct package_resource *pkg_res)
{
	my_arc_entry_t *my_arc_entry;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	my_arc_entry = (my_arc_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_arc_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_arc_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_arc_entry->offset;

	return 0;
}

static int yuLib_arc_extract_resource(struct package *pkg,
									  struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;
	
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen,	pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREAD;
	}
	
	if (strstr(pkg_res->name, ".nei") || strstr(pkg_res->name, ".ptn") 
			|| strstr(pkg_res->name, ".grw")|| strstr(pkg_res->name, ".imo")
			|| strstr(pkg_res->name, ".stu")|| strstr(pkg_res->name, ".wth")) {
		if (lstrcmpi(_T("Skill.arc"), pkg->name))
			decode(compr, comprlen, "neilove");
		else
			decode(compr, comprlen, "ango");
		if (!compr[8])
			compr[8] = ' ';
		if (!compr[9])
			compr[9] = ' ';
		if (!compr[14])
			compr[14] = '\r';
		if (!compr[15])
			compr[15] = '\n';
		if (!compr[16])
			compr[16] = '\r';
		if (!compr[17])
			compr[17] = '\n';
		if (!compr[18])
			compr[18] = '\r';
		if (!compr[19])
			compr[19] = '\n';
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".txt");
	}

	uncompr = NULL;
	uncomprlen = 0;

	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

static int yuLib_arc_save_resource(struct resource *res, 
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

static void yuLib_arc_release_resource(struct package *pkg, 
									   struct package_resource *pkg_res)
{
	if (!pkg)
		return;

	if (pkg_res && pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res && pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void yuLib_arc_release(struct package *pkg, 
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

static cui_ext_operation yuLib_arc_operation = {
	yuLib_arc_match,				/* match */
	yuLib_arc_extract_directory,	/* extract_directory */
	yuLib_arc_parse_resource_info,	/* parse_resource_info */
	yuLib_arc_extract_resource,		/* extract_resource */
	yuLib_arc_save_resource,		/* save_resource */
	yuLib_arc_release_resource,		/* release_resource */
	yuLib_arc_release				/* release */
};

/********************* kthb *********************/

static int yuLib_kthb_match(struct package *pkg)
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

	if (strncmp(magic, "\x89PNG", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;
}

static int yuLib_kthb_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	pkg_res->raw_data = pkg->pio->readvec_only(pkg, pkg_res->raw_data_length, 0, IO_SEEK_SET);
	if (!pkg_res->raw_data)
		return -CUI_EREADVECONLY;

	return 0;
}

static int yuLib_kthb_save_resource(struct resource *res, 
									struct package_resource *pkg_res)
{
	if (!res || !pkg_res)
		return -CUI_EPARA;
	
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (pkg_res->raw_data && pkg_res->raw_data_length) {
		if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
			res->rio->close(res);
			return -CUI_EWRITE;
		}
	}
	res->rio->close(res);
	
	return 0;
}

static void yuLib_kthb_release_resource(struct package *pkg, 
										struct package_resource *pkg_res)
{
	if (!pkg)
		return;

	if (pkg_res && pkg_res->raw_data) 
		pkg_res->raw_data = NULL;
}

static void yuLib_kthb_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (!pkg)
		return;

	pkg->pio->close(pkg);
}

static cui_ext_operation yuLib_kthb_operation = {
	yuLib_kthb_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	yuLib_kthb_extract_resource,/* extract_resource */
	yuLib_kthb_save_resource,	/* save_resource */
	yuLib_kthb_release_resource,/* release_resource */
	yuLib_kthb_release			/* release */
};
	
/********************* kmap *********************/

static int yuLib_kmap_match(struct package *pkg)
{

	if (!pkg)
		return -CUI_EPARA;
	
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;
}

static int yuLib_kmap_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	SIZE_T kmap_size;
	BYTE *kmap;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	if (pkg->pio->length_of(pkg, &kmap_size))
		return -CUI_ELEN;

	kmap = (BYTE *)malloc(kmap_size);
	if (!kmap)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, kmap, kmap_size, 0, IO_SEEK_SET)) {
		free(kmap);
		return -CUI_EREADVEC;
	}

	decode(kmap, kmap_size, "neiloki");
	pkg_res->raw_data = kmap;
	pkg_res->raw_data_length = kmap_size;
	pkg_res->actual_data = NULL;
	pkg_res->actual_data_length = 0;

	return 0;
}

static int yuLib_kmap_save_resource(struct resource *res, 
									struct package_resource *pkg_res)
{
	if (!res || !pkg_res)
		return -CUI_EPARA;
	
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (pkg_res->raw_data && pkg_res->raw_data_length) {
		if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
			res->rio->close(res);
			return -CUI_EWRITE;
		}
	}
	res->rio->close(res);
	
	return 0;
}

static void yuLib_kmap_release_resource(struct package *pkg, 
										struct package_resource *pkg_res)
{
	if (!pkg)
		return;

	if (pkg_res && pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void yuLib_kmap_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (!pkg)
		return;

	pkg->pio->close(pkg);
}

static cui_ext_operation yuLib_kmap_operation = {
	yuLib_kmap_match,			/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	yuLib_kmap_extract_resource,/* extract_resource */
	yuLib_kmap_save_resource,	/* save_resource */
	yuLib_kmap_release_resource,/* release_resource */
	yuLib_kmap_release			/* release */
};

int CALLBACK yuLib_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".arc"), NULL, 
		NULL, &yuLib_arc_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NO_MAGIC))
			return -1;
	
	if (callback->add_extension(callback->cui, _T(".kthb"), _T(".kthb.png"), 
		NULL, &yuLib_kthb_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".kmap"), NULL, 
		NULL, &yuLib_kmap_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;	

	return 0;
}
