#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <stdio.h>
#include <utility.h>

struct acui_information ISM_cui_information = {
	_T("Production StarHole"),	/* copyright */
	_T("ISM"),					/* system */
	_T(".isa"),					/* package */
	_T("1.0.2"),				/* revision */
	_T("痴汉公贼"),				/* author */
	_T("2009-7-23 12:29"),		/* date */
	NULL,						/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

#pragma pack (1)
typedef struct {
	s8 magic[12];
	u16 entries;
	u16 mode;		/* bit0 - 是否用二分法定位指定的资源；bit1 - ？？；bit15 - 是否需要对索引段解密 */
} isa_header_t;

#if 0
typedef struct {
	s8 name[x];		// 12. 16. 36
	u32 offset;
	u32 length;
	u32 reserved[x];// 1 or 2
} isa_entry_t;
#endif
#pragma pack ()

static DWORD entry_name_size, entry_extra_field_size;

/********************* isa *********************/

static int ISM_isa_match(struct package *pkg)
{
	isa_header_t header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &header, sizeof(header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(header.magic, "ISM ARCHIVED", 12)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	u8 tmp[128];
	if (pkg->pio->read(pkg, tmp, sizeof(tmp))) {
		pkg->pio->close(pkg);
		return -CUI_EREADVEC;
	}

	DWORD extra_size = 12, name_size;
	if (header.mode & 0x8000) {
		u8 bak[128];

		memcpy(bak, tmp, sizeof(bak));

		for (name_size = 12; name_size < 64 - 12; name_size += 4) {
			for (extra_size = 12; extra_size <= 16; extra_size += 4) {	
				DWORD index_buffer_length = header.entries * (name_size + extra_size);

				for (DWORD i = 0; i < sizeof(tmp) / 4; ++i)
					((u32 *)tmp)[i] ^= ~(((index_buffer_length / 4) - i) + index_buffer_length);
				
				// offset + length = next.offset
				// data0.isa(ひとゆめ-Autumnal Equinox Story 1.03补丁新增的文件)中
				// 资源的顺序(offset)是不连续的。
				if ((*(u32 *)(tmp + name_size) + *(u32 *)(tmp + name_size + 4)
						== *(u32 *)(tmp + name_size + extra_size + name_size))
						|| *(u32 *)(tmp + name_size) == sizeof(isa_header_t) + index_buffer_length 
						)
					break;

				memcpy(tmp, bak, sizeof(tmp));
			}
			if (extra_size <= 16)
				break;
		}
	} else {
		for (name_size = 12; name_size < 64 - extra_size; name_size += 4) {
			for (extra_size = 12; extra_size <= 16; extra_size += 4) {			
				if (*(u32 *)(tmp + name_size) + *(u32 *)(tmp + name_size + 4)
						== *(u32 *)(tmp + name_size + extra_size + name_size))
					break;
			}
			if (extra_size <= 16)
				break;
		}
	}
	if (name_size >= 64 - 12) {
		printf("sdfsdf\n");
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}
	
	entry_name_size = name_size;
	entry_extra_field_size = extra_size;

	return 0;	
}

static int ISM_isa_extract_directory(struct package *pkg,
									 struct package_directory *pkg_dir)
{
	isa_header_t header;
	if (pkg->pio->readvec(pkg, &header, sizeof(header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD entry_size = entry_name_size + entry_extra_field_size;
	DWORD index_buffer_length = header.entries * entry_size;
	DWORD *index_buffer = (DWORD *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	if (header.mode & 0x8000) {
		for (DWORD i = 0; i < index_buffer_length / 4; ++i)
			index_buffer[i] ^= ~(((index_buffer_length / 4) - i) + index_buffer_length);
	}

	pkg_dir->index_entries = header.entries;
	pkg_dir->index_entry_length = entry_size;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;

	return 0;
}

static int ISM_isa_parse_resource_info(struct package *pkg,
									   struct package_resource *pkg_res)
{
	BYTE *isa_entry = (BYTE *)pkg_res->actual_index_entry;

	strncpy(pkg_res->name, (char *)isa_entry, entry_name_size);
	pkg_res->name_length = entry_name_size;
	isa_entry += entry_name_size;
	pkg_res->offset = *(u32 *)isa_entry;
	isa_entry += 4;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */	
	pkg_res->raw_data_length = *(u32 *)isa_entry;

	return 0;
}

static int ISM_isa_extract_resource(struct package *pkg,
									struct package_resource *pkg_res)
{
	pkg_res->raw_data = malloc((unsigned int)pkg_res->raw_data_length);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EREADVEC;
	}

	if (!memcmp(pkg_res->raw_data, "OggS", 4)) {
		pkg_res->replace_extension = _T(".ogg");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
	} else if (!memcmp(pkg_res->raw_data, "RIFF", 4)) {
		pkg_res->replace_extension = _T(".wav");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
	} else if (!memcmp(pkg_res->raw_data, "\x89PNG", 4)) {
		pkg_res->replace_extension = _T(".png");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
	} 

	return 0;
}

static int ISM_isa_save_resource(struct resource *res, 
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

static void ISM_isa_release_resource(struct package *pkg, 
									 struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}

	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void ISM_isa_release(struct package *pkg, 
							struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

static cui_ext_operation ISM_isa_operation = {
	ISM_isa_match,					/* match */
	ISM_isa_extract_directory,		/* extract_directory */
	ISM_isa_parse_resource_info,	/* parse_resource_info */
	ISM_isa_extract_resource,		/* extract_resource */
	ISM_isa_save_resource,			/* save_resource */
	ISM_isa_release_resource,		/* release_resource */
	ISM_isa_release					/* release */
};

int CALLBACK ISM_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".isa"), NULL, 
		NULL, &ISM_isa_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
