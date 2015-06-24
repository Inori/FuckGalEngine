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
#include <utility.h>

struct acui_information DenSDK_cui_information = {
	_T(""),					/* copyright */
	_T(""),					/* system */
	_T(".dat"),				/* package */
	_T("1.1.1"),			/* revision */
	_T("痴汉公贼"),			/* author */
	_T("2009-1-4 19:30"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};	

#pragma pack (1)
// 256字节
typedef struct {
	s8 magic[4];		// "DAF1"
	u32 header_length;
	u32 index_entries;
	u32 unknown_entries;
	u32 data_offset;
	u8 reserved[236];
} daf1_header_t;

typedef struct {
	u32 entry_size;
	u32 offset;
	u32 comprlen;
	u32 uncomprlen;
	u32 hash_index;		/* 用于快速索引资源 */
	u32 is_compressed;
	s8 *name;
} daf1_entry_t;

// 48字节
typedef struct {
	s8 magic[4];				// "DAF2"
	u32 header_length;
	u32 index_entries;
	u32 unknown_entries;
	u32 compr_index_length;
	u32 uncompr_index_length;
	u32 index_is_compressed;	// may be...
	u32 uncompr_data_offset;
	u8 xor[16];
} daf2_header_t;

typedef struct {
	u32 entry_size;
	u32 offset;		// plus daf2_header->uncompr_data_offset
	u32 comprlen;
	u32 uncomprlen;
	u8 md5[32];
	u32 is_compressed;
	s8 *name;
} daf2_entry_t;
#pragma pack ()

typedef struct {
	s8 name[MAX_PATH];
	u32 offset;
	u32 comprlen;
	u32 uncomprlen;
	u32 is_compressed;
	u8 md5[32];
} my_daf_entry_t;

/********************* dat *********************/

static int DenSDK_dat_match(struct package *pkg)
{
	s8 magic[4];
	int daf_ver;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}
	
	if (!strncmp(magic, "DAF1", 4))
		daf_ver = 0x00010000;
	else if (!strncmp(magic, "DAF2", 4)) {
		daf_ver = 0x00020000;

		//u32 ver;
		//if (pkg->pio->read(pkg, &ver, 4)) {
		//	pkg->pio->close(pkg);
		//	return -CUI_EREAD;
		//}

		//if (ver == 0x0100)
		//	daf_ver |= ver;
	} else {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}
	package_set_private(pkg, (void *)daf_ver);

	return 0;	
}

static int DenSDK_dat_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	DWORD index_length, index_entries;
	int daf_ver;
	my_daf_entry_t *daf_index_buffer;

	daf_ver = (int)package_get_private(pkg);

	if (daf_ver == 0x00010000) {	
		daf1_header_t daf1_header;
	
		if (pkg->pio->readvec(pkg, &daf1_header, sizeof(daf1_header), 0, IO_SEEK_SET))
			return -CUI_EREADVEC;
	
		index_length = daf1_header.index_entries * sizeof(my_daf_entry_t);
		daf_index_buffer = (my_daf_entry_t *)malloc(index_length);
		if (!daf_index_buffer)
			return -CUI_EMEM;

		for (DWORD i = 0; i < daf1_header.index_entries; i++) {
			my_daf_entry_t *my_daf_entry = &daf_index_buffer[i];
			u32 entry_size, hash_index, name_length;
			
			if (pkg->pio->read(pkg, &entry_size, 4))
				break;
			
			if (pkg->pio->read(pkg, &my_daf_entry->offset, 4))
				break;
			
			if (pkg->pio->read(pkg, &my_daf_entry->comprlen, 4))
				break;
			
			if (pkg->pio->read(pkg, &my_daf_entry->uncomprlen, 4))
				break;
			
			if (pkg->pio->read(pkg, &hash_index, 4))
				break;
			
			if (pkg->pio->read(pkg, &my_daf_entry->is_compressed, 4))
				break;
			
			name_length = entry_size - 24;
			if (pkg->pio->read(pkg, my_daf_entry->name, name_length))
				break;
			my_daf_entry->name[name_length] = 0;
		}
		if (i != daf1_header.index_entries) {
			free(daf_index_buffer);
			return -CUI_EREAD;
		}
		index_entries = daf1_header.index_entries;
	} else if ((daf_ver & 0xffff0000) == 0x00020000) {
		daf2_header_t daf2_header;
		DWORD *p_entry;
		u32 xor;
		
		if (pkg->pio->readvec(pkg, &daf2_header, sizeof(daf2_header), 0, IO_SEEK_SET))
			return -CUI_EREADVEC;
		
		xor = (daf2_header.xor[0] << 24) | (daf2_header.xor[5] << 16) | (daf2_header.xor[10] << 8) | daf2_header.xor[15];
		daf2_header.index_entries ^= xor;
		daf2_header.unknown_entries ^= xor;
		daf2_header.compr_index_length ^= xor;
		daf2_header.uncompr_index_length ^= xor;
		// MapleColors2 体験版, 这个字段就是明文
		//if ((daf_ver & 0xffff) != 0x0100)	// 怀疑0x0100的判断是否需要、准确
		//	daf2_header.index_is_compressed ^= xor;
		daf2_header.uncompr_data_offset ^= xor;

		DWORD comprlen = daf2_header.compr_index_length;
		BYTE *compr = new BYTE[comprlen];
		if (!compr)
			return -CUI_EMEM;

		if (pkg->pio->read(pkg, compr, comprlen)) {
			delete [] compr;
			return -CUI_EREAD;
		}

		BYTE *index;
		DWORD index_len;
		if (daf2_header.index_is_compressed == 1) {
			DWORD uncomprlen = daf2_header.uncompr_index_length;
			BYTE *uncompr = new BYTE[uncomprlen];
			if (!uncompr) {
				delete [] compr;
				return -CUI_EMEM;
			}

			if (uncompress(uncompr, &uncomprlen, compr, comprlen) != Z_OK) {
				delete [] uncompr;
				delete [] compr;
				return -CUI_EUNCOMPR;		
			}
			delete [] compr;
			index = uncompr;
			index_len = uncomprlen;
		} else {
			index = compr;
			index_len = comprlen;
		}

		index_length = daf2_header.index_entries * sizeof(my_daf_entry_t);
		daf_index_buffer = (my_daf_entry_t *)malloc(index_length);
		if (!daf_index_buffer) {
			delete [] index;
			return -CUI_EMEM;
		}
		
		p_entry = (DWORD *)index;
		for (DWORD i = 0; i < daf2_header.index_entries; i++) {
			my_daf_entry_t *my_daf_entry = &daf_index_buffer[i];
			u32 entry_size, name_length;
			
			entry_size = *p_entry++ ^ xor;
			my_daf_entry->offset = (*p_entry++ ^ xor) + daf2_header.compr_index_length + sizeof(daf2_header);
			my_daf_entry->comprlen = *p_entry++ ^ xor;
			my_daf_entry->uncomprlen = *p_entry++ ^ xor;
			if (entry_size > 20 + 32 /* md5 */+ 4 /* min name len */) {	// 怀疑0x0100的判断是否需要、准确
				memcpy(my_daf_entry->md5, p_entry, 32);
				p_entry += 8;
				name_length = entry_size - 52;				
			} else {
				my_daf_entry->offset = (my_daf_entry->offset + 3) & ~3;
				name_length = entry_size - 20;
			}

			my_daf_entry->is_compressed = *p_entry++;
			memcpy(my_daf_entry->name, p_entry, name_length);
			my_daf_entry->name[name_length] = 0;
			p_entry = (DWORD *)((BYTE *)p_entry + name_length);
		}
		delete [] index;
		if (i != daf2_header.index_entries) {
			free(daf_index_buffer);
			return -CUI_EREAD;
		}
		index_entries = daf2_header.index_entries;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = daf_index_buffer;
	pkg_dir->directory_length = index_length;
	pkg_dir->index_entry_length = sizeof(my_daf_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

static int DenSDK_dat_parse_resource_info(struct package *pkg,
										  struct package_resource *pkg_res)
{
	my_daf_entry_t *my_daf_entry;

	my_daf_entry = (my_daf_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_daf_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_daf_entry->comprlen;
	pkg_res->actual_data_length = my_daf_entry->uncomprlen;
	pkg_res->offset = my_daf_entry->offset;

	return 0;
}

static int DenSDK_dat_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;
	my_daf_entry_t *my_daf_entry;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)pkg->pio->readvec_only(pkg, comprlen, pkg_res->offset, IO_SEEK_SET);
	if (!compr)
		return -CUI_EREADVECONLY;

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = compr;
		return 0;
	}

	uncomprlen = pkg_res->actual_data_length;
	my_daf_entry = (my_daf_entry_t *)pkg_res->actual_index_entry;
	if (my_daf_entry->is_compressed) {
		DWORD act_uncomprlen;
		
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr)
			return -CUI_EMEM;

		act_uncomprlen = uncomprlen;
		if (uncompress(uncompr, &act_uncomprlen, compr, comprlen) != Z_OK) {
			free(uncompr);
			return -CUI_EUNCOMPR;
		}
	} else
		uncompr = NULL;

	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;

	return 0;
}

static int DenSDK_dat_save_resource(struct resource *res, 
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

static void DenSDK_dat_release_resource(struct package *pkg, 
										struct package_resource *pkg_res)
{
	if (pkg_res && pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res && pkg_res->raw_data)
		pkg_res->raw_data = NULL;
}

static void DenSDK_dat_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory && pkg_dir->directory_length) {		
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}
	pkg->pio->close(pkg);
}

static cui_ext_operation DenSDK_dat_operation = {
	DenSDK_dat_match,					/* match */
	DenSDK_dat_extract_directory,		/* extract_directory */
	DenSDK_dat_parse_resource_info,		/* parse_resource_info */
	DenSDK_dat_extract_resource,		/* extract_resource */
	DenSDK_dat_save_resource,			/* save_resource */
	DenSDK_dat_release_resource,		/* release_resource */
	DenSDK_dat_release					/* release */
};

int CALLBACK DenSDK_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &DenSDK_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
