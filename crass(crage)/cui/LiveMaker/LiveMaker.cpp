#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>
#include <zlib.h>
#include <utility.h>

// E:\LiveMaker
// http://prefetchnta.blog.ccidnet.com/blog-htm-do-showone-uid-20555-type-blog-itemid-202908.html

struct acui_information LiveMaker_cui_information = {
	_T(""),					/* copyright */
	_T("LiveMaker"),		/* system */
	_T(".gal .dat .exe .ext"),	/* package */
	_T(""),			/* revision */
	_T("痴汉公贼"),			/* author */
	_T(""),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};	

#pragma pack (1)
typedef struct {
	s8 magic[2];		// "vf"
	u32 version;
	u32 index_entries;
} dat_header_t;

typedef struct {
	s8 magic[7];		// "GaleXXX", XXX is version string
	u32 info_length;
} gal_header_t;

typedef struct {
	u32 version;		// 与magic中的XXX字段对应
	u32 width;
	u32 height;
	u32 bpp;
	u32 frames;
	u8 unknown			// 0 or 1
	u8 code;			// 0 识别code
	u8 compress_method;	// 0 - 通常压缩 1 - 无压缩 2 - 不可逆压缩
	u8 id;				// 0x00 - Gale105 0xc0 - Gale106
	u32 mask;			// 0x00ffffff(rgba)
	u32 block_width;	// 0x10(不可逆压缩为0)
	u32 block_height;	// 0x10(不可逆压缩为0)
	u32 quality;		// 品质(用于不可逆压缩): [0, 100]
} gal_infor_t;

typedef struct {
	u32 name_length;
	s8 *name;
	u32 mask;		// 0xXXffffff(rgba) XX在transparent enabled时为0，否则位ff
	u32 delay;		// ms
	u32 disposal;	// 0 - None 1 - No disposal 2 - Background 3 - Previous
	u8	// 0
	u32 layers;	// 1
	u32 width;
	u32 height;
	u32 bpp;
#if bpp <= 8 
　　u32 palette[2^bpp];
#endif 
	u32		// 0;
	u32 ; // 0
	u8 visible;
	u8 ; /0xff
	u32 // 0xffffffff
	u32 ; // 0x0
} frame_header_t;
#pragma pack ()

typedef struct {
	s8 name[MAX_PATH];
	u8 is_plain;
	u8 unknown;
	u32 offset;
	u32 comprlen;
	u32 time_stamp;
	u32 crc;
} my_dat_entry_t;

static u32 LiveMaker_version;

static s64 update_key(u32 *key, u32 seed)
{
	*key = (*key << 2) + *key + 0x75d6ee39;
	return *key;
}

/********************* dat *********************/

static int LiveMaker_dat_match(struct package *pkg)
{
	s8 magic[2];
	struct package *_pkg;
	
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->read(pkg, magic, 2)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}
	
	if (strncmp(magic, "vf", 2)) {
		if (pkg->pio->open(pkg->lst, IO_READONLY)) {
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}
		if (pkg->pio->read(pkg->lst, magic, 2)) {
			pkg->pio->close(pkg->lst);
			pkg->pio->close(pkg);
			return -CUI_EREAD;
		}
		if (strncmp(magic, "vf", 2)) {
			pkg->pio->close(pkg->lst);
			pkg->pio->close(pkg);
			return -CUI_EMATCH;
		}
		if (pkg->pio->seek(pkg, 0, IO_SEEK_SET)) {
			pkg->pio->close(pkg->lst);
			pkg->pio->close(pkg);
			return -CUI_ESEEK;
		}
		_pkg = pkg->lst;
	} else
		_pkg = pkg;
	
	if (pkg->pio->read(_pkg, &LiveMaker_version, 4)) {
		if (pkg->lst)
			pkg->pio->close(pkg->lst);
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}
	/* set base address */
	package_set_private(pkg, 0);
	
	return 0;
}

static int LiveMaker_dat_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{
	u32 index_entries;
	my_dat_entry_t *index_buffer;
	DWORD index_length;
	u32 key;
	u32 base_address;
	struct package *_pkg;

	if (!pkg || !pkg_dir)
		return -CUI_EPARA;

	if (pkg->lst)
		_pkg = pkg->lst;
	else
		_pkg = pkg;

	if (pkg->pio->read(_pkg, &index_entries, 4))
		return -CUI_EREAD;
	
	index_length = (index_entries + 1) * sizeof(my_dat_entry_t);
	index_buffer = (my_dat_entry_t *)malloc(index_length);
	if (!index_buffer)
		return -CUI_EMEM;

	key = 0;
	for (DWORD i = 0; i < index_entries; i++) {
		my_dat_entry_t *entry = &index_buffer[i];
		u32 name_len;
		
		if (pkg->pio->read(_pkg, &name_len, 4))
			break;
		
		if (pkg->pio->read(_pkg, entry->name, name_len))
			break;
		
		if (LiveMaker_version >= 100) {
			for (u32 n = 0; n < name_len; n++)
				entry->name[n] ^= (BYTE)update_key(&key, 0x75d6ee39);
		}
		entry->name[name_len] = 0;
	}
	if (i != index_entries) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	if (LiveMaker_version >= 101) {
		base_address = (u32)package_get_private(pkg);

		key = 0;
		for (i = 0; i < index_entries + 1; i++) {
			my_dat_entry_t *entry = &index_buffer[i];
			u64 offset;
			
			if (pkg->pio->read(_pkg, &offset, 8))
				break;
			
			offset ^= update_key(&key, 0x75d6ee39);
			entry->offset = base_address + (u32)offset;
		}
		if (i != index_entries + 1) {
			free(index_buffer);
			return -CUI_EREAD;
		}
	} else {
		printf("%d not supported2\n", LiveMaker_version);
		return -CUI_EMATCH;
	}

	if (LiveMaker_version >= 101) {
		for (i = 0; i < index_entries; i++) {
			my_dat_entry_t *entry = &index_buffer[i];
			
			if (pkg->pio->read(_pkg, &entry->is_plain, 1))
				break;
		}
		if (i != index_entries) {
			free(index_buffer);
			return -CUI_EREAD;
		}

		key = 0;
		for (i = 0; i < index_entries; i++) {
			my_dat_entry_t *entry = &index_buffer[i];
			
			if (pkg->pio->read(_pkg, &entry->time_stamp, 4))
				break;
		}
		if (i != index_entries) {
			free(index_buffer);
			return -CUI_EREAD;
		}

		key = 0;
		for (i = 0; i < index_entries; i++) {
			my_dat_entry_t *entry = &index_buffer[i];
			
			if (pkg->pio->read(_pkg, &entry->crc, 4))
				break;
		}
		if (i != index_entries) {
			free(index_buffer);
			return -CUI_EREAD;
		}

		if (LiveMaker_version < 102) {
			printf("%d not supported1\n", LiveMaker_version);
			return -CUI_EMATCH;
		} else {
			for (i = 0; i < index_entries; i++) {
				my_dat_entry_t *entry = &index_buffer[i];

				if (pkg->pio->read(_pkg, &entry->unknown, 1))
					break;
			}
			if (i != index_entries) {
				free(index_buffer);
				return -CUI_EREAD;
			}		
		}
	}

	for (i = 0; i < index_entries; i++)	
		index_buffer[i].comprlen = index_buffer[i + 1].offset - index_buffer[i].offset;
#if 0
	for (i = 0; i < index_entries+1; i++) {
		my_dat_entry_t *entry = &index_buffer[i];

		printf("%s: is_plain %d %d comprlen %x time_stamp %x crc %x @ %x\n", entry->name, entry->is_plain,entry->unknown,entry->comprlen,entry->time_stamp,entry->crc,entry->offset);
	}
#endif
	pkg_dir->index_entries = index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_length - sizeof(my_dat_entry_t);
	pkg_dir->index_entry_length = sizeof(my_dat_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

static int LiveMaker_dat_parse_resource_info(struct package *pkg,
											 struct package_resource *pkg_res)
{
	my_dat_entry_t *my_dat_entry;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	my_dat_entry = (my_dat_entry_t *)pkg_res->actual_index_entry;
	strncpy(pkg_res->name, my_dat_entry->name, 64);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_dat_entry->comprlen;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = my_dat_entry->offset;

	return 0;
}

static int LiveMaker_dat_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;
	my_dat_entry_t *my_dat_entry;
	
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)pkg->pio->readvec_only(pkg, comprlen, pkg_res->offset, IO_SEEK_SET);
	if (!compr)
		return -CUI_EREADVECONLY;

	my_dat_entry = (my_dat_entry_t *)pkg_res->actual_index_entry;
	if (crc32(0, compr, comprlen) != my_dat_entry->crc)
		return -CUI_EMATCH;

	if (!my_dat_entry->is_plain) {
		uncomprlen = comprlen;
		while (1) {
			uncomprlen <<= 1;
			uncompr = (BYTE *)malloc(uncomprlen);
			if (!uncompr)
				return -CUI_EMEM;

			if (uncompress(uncompr, &uncomprlen, compr, comprlen) == Z_OK)
				break;

			free(uncompr);
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

static int LiveMaker_dat_save_resource(struct resource *res, 
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

static void LiveMaker_dat_release_resource(struct package *pkg, 
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

static void LiveMaker_dat_release(struct package *pkg, 
								  struct package_directory *pkg_dir)
{
	if (!pkg || !pkg_dir)
		return;

	if (pkg_dir->directory && pkg_dir->directory_length) {		
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	if (pkg->lst)
		pkg->pio->close(pkg->lst);
	pkg->pio->close(pkg);
}

static cui_ext_operation LiveMaker_dat_operation = {
	LiveMaker_dat_match,				/* match */
	LiveMaker_dat_extract_directory,	/* extract_directory */
	LiveMaker_dat_parse_resource_info,	/* parse_resource_info */
	LiveMaker_dat_extract_resource,		/* extract_resource */
	LiveMaker_dat_save_resource,		/* save_resource */
	LiveMaker_dat_release_resource,		/* release_resource */
	LiveMaker_dat_release				/* release */
};

/********************* exe *********************/

static int LiveMaker_exe_match(struct package *pkg)
{
	s8 magic[2];
	u32 offset;
	
	if (!pkg)
		return -CUI_EPARA;
	
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->readvec(pkg, magic, 2, -2, IO_SEEK_END)) {
		pkg->pio->close(pkg);
		return -CUI_EREADVEC;
	}

	if (strncmp(magic, "lv", 2)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}
	
	if (pkg->pio->readvec(pkg, &offset, 4, -6, IO_SEEK_END)) {
		pkg->pio->close(pkg);
		return -CUI_EREADVEC;
	}
	
	if (pkg->pio->readvec(pkg, magic, 2, offset, IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_EREADVEC;
	}
	
	if (strncmp(magic, "vf", 2)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}
	
	if (pkg->pio->read(pkg, &LiveMaker_version, 4)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	/* set base address */
	package_set_private(pkg, (void *)offset);
	
	return 0;
}

static cui_ext_operation LiveMaker_exe_operation = {
	LiveMaker_exe_match,				/* match */
	LiveMaker_dat_extract_directory,	/* extract_directory */
	LiveMaker_dat_parse_resource_info,	/* parse_resource_info */
	LiveMaker_dat_extract_resource,		/* extract_resource */
	LiveMaker_dat_save_resource,		/* save_resource */
	LiveMaker_dat_release_resource,		/* release_resource */
	LiveMaker_dat_release				/* release */
};

int CALLBACK LiveMaker_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &LiveMaker_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".exe"), NULL, 
		NULL, &LiveMaker_exe_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
