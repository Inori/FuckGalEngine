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

struct acui_information gbyFlagGame_cui_information = {
	_T("FOSTER"),				/* copyright */
	_T("gbyFlagGame"),			/* system */
	_T(".FA2 .C24"),			/* package */
	_T("0.7.0"),				/* revision */
	_T("痴漢公賊"),				/* author */
	_T("2009-1-29 12:26"),		/* date */
	NULL,						/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

#pragma pack (1)
typedef struct {
	s8 magic[4];	/* "FA2" */
	u32 is_compressed;
	u32 index_offset;
	u32 index_entries;
} FA2_header_t;

typedef struct {
	s8 name[15];
	u8 flags;		// bit 1 - is_compressed, bit 0 - 好像非压缩数据就是1？
	u32 time_stamp0;
	u32 time_stamp1;
	u32 uncomprlen;
	u32 comprlen;
} FA2_entry_t;

typedef struct {
	s8 magic[4];		// "C24"
	u32 index_entries;
} C24_header_t;

typedef struct {
	s32 width;
	s32 height;
	s32 orig_x;
	s32 orig_y;
} C24_info_t;
#pragma pack ()

typedef struct {
	char name[8];
	DWORD length;
	DWORD offset;
} my_c24_entry_t;


static void *my_malloc(DWORD len)
{
	return new BYTE[len];
}

static int test_flag(u32 &curbits, u8 **compr, u32 &flags)
{
	if (!--curbits) {
		curbits = 32;
		flags = *(u32 *)(*compr);
		(*compr) += 4;
	}	
	u32 ret = flags & 0x80000000;
	flags <<= 1;
	return ret;
}

#define TEST_BIT()	test_flag(curbits, &compr, flags)

static DWORD FA2_uncompress(u8 *uncompr, u32 uncomprlen,
							u8 *compr, u32 comprlen)
{
	u32 curbits = 33;
	u32 flags = *(u32 *)compr;
	u32 offset, copy_bytes;
	u8 *org = uncompr;
	u8 *org_c = compr;

	compr += 4;
	while (1) {
		if (TEST_BIT())
			*uncompr++ = *compr++;
		else if (TEST_BIT()) {
			if (TEST_BIT()) {
				offset = *compr++;
				offset *= 2;
				if (TEST_BIT())
					++offset;

				offset *= 2;
				if (TEST_BIT())
					++offset;

				offset *= 2;
				if (TEST_BIT())
					++offset;
				
				offset += 256;
				if (offset >= 0x8FF)
					break;
			} else
				offset = *compr++;

			uncompr[0] = uncompr[0 - offset - 1];
			uncompr[1] = uncompr[0 - offset];
			uncompr += 2;
		} else {
			if (TEST_BIT())		
				offset = *compr++;
			else {
				offset = 256;
				if (TEST_BIT())
					*(u8 *)&offset = *compr++;
				else {
					if (TEST_BIT())
						*(u8 *)&offset = *compr++;
					else {
						if (!TEST_BIT()) {
							*(u8 *)&offset = *compr++;
							offset *= 2;
							if (TEST_BIT())
								++offset;
						} else
							*(u8 *)&offset = *compr++;

						offset *= 2;
						if (TEST_BIT())
							++offset;
					}

					offset *= 2;
					if (TEST_BIT())
						++offset;
				}
			}

			offset *= 2;
			if (TEST_BIT())
				++offset;

			copy_bytes = 0;

			if (TEST_BIT())
				copy_bytes = 3;
			else {
				if (TEST_BIT())
					copy_bytes = 4;
				else {
					if (TEST_BIT()) {
						copy_bytes += 5;
						if (TEST_BIT())
							++copy_bytes;
					} else {
						if (TEST_BIT()) {
							copy_bytes *= 2;
							if (TEST_BIT())
								++copy_bytes;
								
							copy_bytes *= 2;							
							if (TEST_BIT())
								++copy_bytes;

							copy_bytes += 7;
						} else {
							if (TEST_BIT()) {								
								copy_bytes *= 2;
								if (TEST_BIT())
									++copy_bytes;

								copy_bytes *= 2;
								if (TEST_BIT())
									++copy_bytes;							
										
								copy_bytes *= 2;
								if (TEST_BIT())
									++copy_bytes;

								copy_bytes *= 2;
								if (TEST_BIT())
									++copy_bytes;
								
								copy_bytes += 11;
							} else {
								copy_bytes = *compr++;
								copy_bytes += 27;
							}										
						}
					}
				}
			}
	
			if (!offset) {
				BYTE dat = uncompr[-1];
				for (DWORD i = 0; i < copy_bytes; ++i)
					*uncompr++ = dat;
			} else if (offset == 1) {
				BYTE dat[2];

				dat[0] = uncompr[-2];
				dat[1] = uncompr[-1];
				for (DWORD i = 0; i < copy_bytes; ++i) {
					*uncompr++ = dat[0];
					if (++i < copy_bytes)
						*uncompr++ = dat[1];
				}
			} else if (offset == 2) {
				BYTE dat[3];

				dat[0] = uncompr[-3];
				dat[1] = uncompr[-2];
				dat[2] = uncompr[-1];
				for (DWORD i = 0; i < copy_bytes; ++i) {
					*uncompr++ = dat[0];
					if (++i < copy_bytes)
						*uncompr++ = dat[1];
						if (++i < copy_bytes)
							*uncompr++ = dat[2];
				}
			} else {
				BYTE *src = uncompr - offset - 1;
				for (DWORD i = 0; i < copy_bytes; ++i)
					*uncompr++ = *src++;
			}
		}
	}
	//printf("%x / %x\n", compr-org_c, comprlen);
	return uncompr - org;
}

static void C24_uncompress(BYTE *img, BYTE **line_table, DWORD width, DWORD height)
{
	DWORD line_len = width * 3;
	for (DWORD y = 0; y < height; ++y) {
		BYTE *line = *line_table++;
		for (DWORD x = 0; x < width; ) {
			DWORD cnt = *line++;
			if (cnt == 0xff) {
				cnt = *(u16 *)line;
				line += 2;
			}
			img += cnt * 3;
			x += cnt;
			if (x >= width)
				break;
			cnt = *line++;
			if (!cnt) {
				cnt = *(u16 *)line;
				line += 2;
			}			
			DWORD len = cnt * 3;
			if (len >= 7) {
				DWORD align = ((width - x) * 3) & 3;
				if (align) {
					for (DWORD i = 0; i < align; ++i)
						*img++ = *line++;
					len -= align; 
				}
			}
			for (DWORD i = 0; i < len; ++i)
				*img++ = *line++;
			x += cnt;
		}
	}
}

/********************* FA2 *********************/

static int gbyFlagGame_FA2_match(struct package *pkg)
{
	FA2_header_t FA2_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &FA2_header, sizeof(FA2_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(FA2_header.magic, "FA2", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int gbyFlagGame_FA2_extract_directory(struct package *pkg,
											 struct package_directory *pkg_dir)
{
	FA2_header_t header;

	if (pkg->pio->readvec(pkg, &header, sizeof(header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	if (pkg->pio->seek(pkg, header.index_offset, IO_SEEK_SET))
		return -CUI_ESEEK;

	u32 FA2_size;
	pkg->pio->length_of(pkg, &FA2_size);
	DWORD index_len = FA2_size - header.index_offset;
	BYTE *index = new BYTE[index_len];
	if (!index)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index, index_len)) {
		delete [] index;
		return -CUI_EREAD;
	}

	if (header.is_compressed & 1) {
		DWORD uncomprlen = header.index_entries * sizeof(FA2_entry_t);
		BYTE *uncompr = new BYTE[uncomprlen];
		if (!uncompr) {
			delete [] index;
			return -CUI_EMEM;			
		}

		u32 act_uncomprlen = FA2_uncompress(uncompr, uncomprlen, index, index_len);
		delete [] index;
		if (act_uncomprlen != uncomprlen) {
			delete [] uncompr;
			return -CUI_EUNCOMPR;
		}
		index = uncompr;
		index_len = uncomprlen;
	}

	DWORD offset = sizeof(FA2_header_t);
	for (DWORD i = 0; i < header.index_entries; ++i) {
		((FA2_entry_t *)index)[i].time_stamp0 = offset;
		offset += (((FA2_entry_t *)index)[i].comprlen + 15) & ~15;
	}

	pkg_dir->index_entries = header.index_entries;
	pkg_dir->index_entry_length = sizeof(FA2_entry_t);
	pkg_dir->directory = index;
	pkg_dir->directory_length = index_len;

	return 0;
}

static int gbyFlagGame_FA2_parse_resource_info(struct package *pkg,
											   struct package_resource *pkg_res)
{
	FA2_entry_t *FA2_entry;

	FA2_entry = (FA2_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, FA2_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = FA2_entry->comprlen;
	pkg_res->actual_data_length = FA2_entry->uncomprlen;	/* 数据都是明文 */
	pkg_res->offset = FA2_entry->time_stamp0;

	return 0;
}

static int gbyFlagGame_FA2_extract_resource(struct package *pkg,
											struct package_resource *pkg_res)
{
	DWORD comprlen = pkg_res->raw_data_length;
	BYTE *compr = new BYTE[comprlen];
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, pkg_res->offset, IO_SEEK_SET)) {
		delete [] compr;
		return -CUI_EREADVEC;
	}

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = compr;
		return 0;
	}

	FA2_entry_t *FA2_entry = (FA2_entry_t *)pkg_res->actual_index_entry;
	BYTE *act_data;
	DWORD act_data_len;
	if (FA2_entry->flags & 2) {
		DWORD uncomprlen = pkg_res->actual_data_length;
		BYTE *uncompr = new BYTE[uncomprlen + 4096];
		if (!uncompr) {
			delete [] compr;
			return -CUI_EMEM;			
		}

		u32 act_uncomprlen = FA2_uncompress(uncompr, uncomprlen, compr, comprlen);
		delete [] compr;
		if (act_uncomprlen != uncomprlen) {
			//printf("%s %x %x %x %x @ %x\n", pkg_res->name, 
			//	FA2_entry->flags, act_uncomprlen, 
			//	uncomprlen, comprlen, pkg_res->offset);
			delete [] uncompr;
			return -CUI_EUNCOMPR;
		}
		pkg_res->actual_data = uncompr;
		pkg_res->actual_data_length = act_uncomprlen;
		act_data = uncompr;
		act_data_len = act_uncomprlen;
	} else {
		pkg_res->raw_data = compr;
		act_data = (BYTE *)pkg_res->raw_data;
		act_data_len = comprlen;
	}

	C24_header_t *C24 = (C24_header_t *)act_data;
	if (strncmp(C24->magic, "C24", 4) || C24->index_entries != 1)
		return 0;

	u32 info_offset = *(u32 *)(act_data + sizeof(C24_header_t));
	if (!info_offset)
		return 0;

	C24_info_t *info = (C24_info_t *)(act_data + info_offset);
	if (!info->width || !info->height)
		return 0;

	u32 *line_offset_table = (u32 *)(info + 1);
	for (int i = 0; i < info->height; ++i)
		line_offset_table[i] = (u32)(act_data + line_offset_table[i]);

	DWORD uncomprlen = info->width * info->height * 3;
	BYTE *uncompr = new BYTE[uncomprlen + 4096];
	if (!uncompr) {
		delete [] act_data;
		return -CUI_EMEM;
	}
	memset(uncompr, 0xff, uncomprlen);

	C24_uncompress(uncompr, (BYTE **)line_offset_table, info->width, info->height);

	if (MyBuildBMPFile(uncompr, uncomprlen, NULL, 0, info->width,
			-info->height, 24, (BYTE **)&pkg_res->actual_data,
			&pkg_res->actual_data_length, my_malloc)) {
		delete [] uncompr;
		delete [] act_data;
		return -CUI_EMEM;		
	}
	delete [] uncompr;
	delete [] act_data;

	pkg_res->flags |= PKG_RES_FLAG_REEXT;
	pkg_res->replace_extension = _T(".bmp");

	return 0;
}

static int gbyFlagGame_FA2_save_resource(struct resource *res, 
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

static void gbyFlagGame_FA2_release_resource(struct package *pkg, 
											 struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		delete [] pkg_res->actual_data;
		pkg_res->actual_data = NULL;
	}
	if (pkg_res->raw_data) {
		delete [] pkg_res->raw_data;
		pkg_res->raw_data = NULL;
	}
}

static void gbyFlagGame_FA2_release(struct package *pkg, 
									struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

static cui_ext_operation gbyFlagGame_FA2_operation = {
	gbyFlagGame_FA2_match,				/* match */
	gbyFlagGame_FA2_extract_directory,	/* extract_directory */
	gbyFlagGame_FA2_parse_resource_info,/* parse_resource_info */
	gbyFlagGame_FA2_extract_resource,	/* extract_resource */
	gbyFlagGame_FA2_save_resource,		/* save_resource */
	gbyFlagGame_FA2_release_resource,	/* release_resource */
	gbyFlagGame_FA2_release				/* release */
};

/********************* C24 *********************/

static int gbyFlagGame_C24_match(struct package *pkg)
{
	C24_header_t C24_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &C24_header, sizeof(C24_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(C24_header.magic, "C24", 4) || !C24_header.index_entries) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int gbyFlagGame_C24_extract_directory(struct package *pkg,
											 struct package_directory *pkg_dir)
{
	C24_header_t header;

	if (pkg->pio->readvec(pkg, &header, sizeof(header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	u32 *offset_table = new u32[header.index_entries + 1];
	if (!offset_table)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, offset_table, sizeof(u32) * header.index_entries)) {
		delete [] offset_table;
		return -CUI_EREAD;
	}
	pkg->pio->length_of(pkg, &offset_table[header.index_entries]);

	my_c24_entry_t *index = new my_c24_entry_t[header.index_entries];
	if (!index) {
		delete [] offset_table;
		return -CUI_EMEM;
	}

	int p = -1;
	for (DWORD i = 0; i < header.index_entries; ++i) {
		C24_info_t info;

		sprintf(index[i].name, "%04d", i);
		index[i].offset = offset_table[i];
		if (!index[i].offset)
			index[i].length = 0;
		else if (!pkg->pio->readvec(pkg, &info, sizeof(info), offset_table[i], IO_SEEK_SET)) {
			if (!info.width || !info.height)
				index[i].length = 0;
			else {
				if (p != -1)
					index[p].length = offset_table[i] - offset_table[p];
				p = i;
			}
		} else
			break;
	}
	if (p != -1)
		index[p].length = offset_table[i] - offset_table[p];
	delete [] offset_table;
	if (i != header.index_entries) {
		delete [] index;
		return -CUI_EREADVEC;
	}

	pkg_dir->index_entries = header.index_entries;
	pkg_dir->index_entry_length = sizeof(my_c24_entry_t);
	pkg_dir->directory = index;
	pkg_dir->directory_length = sizeof(my_c24_entry_t) * header.index_entries;
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

static int gbyFlagGame_C24_parse_resource_info(struct package *pkg,
											   struct package_resource *pkg_res)
{
	my_c24_entry_t *entry;

	entry = (my_c24_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = entry->offset;

	return 0;
}

static int gbyFlagGame_C24_extract_resource(struct package *pkg,
											struct package_resource *pkg_res)
{
	BYTE *C24 = new BYTE[pkg_res->raw_data_length];
	if (!C24)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, C24, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET)) {
		delete [] C24;
		return -CUI_EREADVEC;
	}

	C24_info_t *info = (C24_info_t *)C24;
	u32 *line_offset_table = (u32 *)(C24 + sizeof(C24_info_t));
	for (int i = 0; i < info->height; ++i)
		line_offset_table[i] = (u32)(C24 + line_offset_table[i] - pkg_res->offset);

	DWORD uncomprlen = info->width * info->height * 3;
	BYTE *uncompr = new BYTE[uncomprlen + 4096];
	if (!uncompr) {
		delete [] C24;
		return -CUI_EMEM;
	}
	memset(uncompr, 0xff, uncomprlen);

	C24_uncompress(uncompr, (BYTE **)line_offset_table, info->width, info->height);

	if (MyBuildBMPFile(uncompr, uncomprlen, NULL, 0, info->width,
			-info->height, 24, (BYTE **)&pkg_res->actual_data,
			&pkg_res->actual_data_length, my_malloc)) {
		delete [] uncompr;
		delete [] C24;
		return -CUI_EMEM;		
	}
	delete [] uncompr;
	delete [] C24;

	return 0;
}

static cui_ext_operation gbyFlagGame_C24_operation = {
	gbyFlagGame_C24_match,				/* match */
	gbyFlagGame_C24_extract_directory,	/* extract_directory */
	gbyFlagGame_C24_parse_resource_info,/* parse_resource_info */
	gbyFlagGame_C24_extract_resource,	/* extract_resource */
	gbyFlagGame_FA2_save_resource,		/* save_resource */
	gbyFlagGame_FA2_release_resource,	/* release_resource */
	gbyFlagGame_FA2_release				/* release */
};

int CALLBACK gbyFlagGame_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".FA2"), NULL, 
		NULL, &gbyFlagGame_FA2_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".C24"), _T(".bmp"), 
		NULL, &gbyFlagGame_C24_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES
		| CUI_EXT_FLAG_DIR))
			return -1;

	return 0;
}
