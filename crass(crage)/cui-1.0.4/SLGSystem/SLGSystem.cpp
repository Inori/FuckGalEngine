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

struct acui_information SLGSystem_cui_information = {
	NULL,					/* copyright */
	NULL,					/* system */
	_T(".sz .alb .VOI .tig .SPD .SPL"),	/* package */
	_T("1.0.3"),			/* revision */
	_T("痴汉公贼"),			/* author */
	_T("2009-6-26 21:43"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};	

#pragma pack (1)
typedef struct {
	s8 magic[4];		// "SMZL"
	u32 uncompren;
} sz_header_t;

typedef struct {
	s8 magic[8];		// "ALB1.21"
	u32 uncompren;
	u32 comprlen;		// 运行时写入alb文件长度（文件时是0）
} alb_header_t;

typedef struct {
	s8 magic[4];		// "PHF"(第一块) or "PH\x1a"(中间块) or "PH\x1a"(最后一块)
	u16 length;
	u8 is_compressed;
	u8 is_continuous;
} alb_bfe_header_t;

typedef struct {
	s8 dummy0[30];
	u8 offset;
	s8 dummy1;
} VOI_header_t;

typedef struct {
	s8 magic[4];	// "SFP"
	u32 unknown0;	// 0
	u32 unknown1;	// 5
	u32 factor;
	u32 size;
	u32 reserved[3];// 0
} SPD_header_t;

typedef struct {
	u32 name_offset;
	u32 length;
	u32 offset;
	u32 reserved;
} SPL_entry_t;
#pragma pack ()


/********************* sz *********************/

static int SLGSystem_sz_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}
	
	if (strncmp(magic, "SMZL", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int SLGSystem_sz_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen, act_uncomprlen;
	u32 sz_size;
	
	if (pkg->pio->length_of(pkg, &sz_size))
		return -CUI_EREADVEC;
	
	if (pkg->pio->readvec(pkg, &uncomprlen, 4, 4, IO_SEEK_SET)) 
		return -CUI_EREADVEC;
	
	comprlen = sz_size - sizeof(sz_header_t);
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, 
			sizeof(sz_header_t), IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}
	
	uncompr = (BYTE *)malloc(uncomprlen);
	if (!uncompr) {
		free(compr);
		return -CUI_EMEM;
	}

	act_uncomprlen = uncomprlen;
	if (uncompress(uncompr, &act_uncomprlen, compr, comprlen) != Z_OK) {
		free(uncompr);
		free(compr);
		return -CUI_EUNCOMPR;
	}
	if (act_uncomprlen != uncomprlen) {
		free(uncompr);
		free(compr);
		return -CUI_EUNCOMPR;
	}

	if (!strncmp((char *)uncompr, "DDS ", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".dds");
	} else if (!strncmp((char *)uncompr, "\x89PNG", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".png");
	}

	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

static int SLGSystem_sz_save_resource(struct resource *res, 
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

static void SLGSystem_sz_release_resource(struct package *pkg, 
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

static void SLGSystem_sz_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;		
	}

	pkg->pio->close(pkg);
}

static cui_ext_operation SLGSystem_sz_operation = {
	SLGSystem_sz_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	SLGSystem_sz_extract_resource,	/* extract_resource */
	SLGSystem_sz_save_resource,		/* save_resource */
	SLGSystem_sz_release_resource,	/* release_resource */
	SLGSystem_sz_release			/* release */
};

/********************* alb *********************/

static int SLGSystem_alb_match(struct package *pkg)
{
	s8 magic[8];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}
	
	if (strncmp(magic, "ALB1.21", 8)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int SLGSystem_alb_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr;
	DWORD curbyte, comprlen, uncomprlen, act_uncomprlen;
	u32 alb_size;
	
	if (pkg->pio->length_of(pkg, &alb_size))
		return -CUI_EREADVEC;
	
	if (pkg->pio->readvec(pkg, &uncomprlen, 4, 8, IO_SEEK_SET)) 
		return -CUI_EREADVEC;

	if (pkg->pio->readvec(pkg, &curbyte, 4, 12, IO_SEEK_SET)) 
		return -CUI_EREADVEC;

	comprlen = alb_size - sizeof(alb_header_t);
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, 
			sizeof(alb_header_t), IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}
	
	uncompr = (BYTE *)malloc(uncomprlen);
	if (!uncompr) {
		free(compr);
		return -CUI_EMEM;
	}

	act_uncomprlen = 0;

	for (curbyte = 0; curbyte < comprlen; ) {
		alb_bfe_header_t bfe;
		BYTE left[256], right[256];
		BYTE stack[256];
		int index = 0;
		u32 c;

		memcpy(bfe.magic, &compr[curbyte], 4);
		curbyte += 4;
		bfe.length = *(u16 *)&compr[curbyte];
		curbyte += 2;
		bfe.is_compressed = compr[curbyte++];
		bfe.is_continuous = compr[curbyte++];

		if (bfe.is_compressed) {
			for (int i = 0; i < 256; ) {
				BYTE flag = compr[curbyte++];

				if (flag == bfe.is_continuous) {
					BYTE count = compr[curbyte++];

					for (int k = 0; k < count; k++) {
						left[i] = i;
						right[i] = 0;
						i++;
					}
				} else {
					left[i] = flag;
					right[i] = compr[curbyte++];
					i++;
				}
			}
		} else {
			for (int i = 0; i < 256; i++) {
				left[i] = compr[curbyte++];
				right[i] = compr[curbyte++];
			}
		}

		index = 0;
		for (int k = 0; k < bfe.length; ) {
			if (index) {
				c = stack[index - 1];
				index--;
			} else {
				c = compr[curbyte++];
				k++;
			}
			
			if (left[c] == c)
				uncompr[act_uncomprlen++] = c;
			else {
				stack[index++] = right[c];
				stack[index++] = left[c];
			}
		}

		while (index) {
			c = stack[index - 1];
			index--;

			if (left[c] == c)
				uncompr[act_uncomprlen++] = c;
			else {
				stack[index++] = right[c];
				stack[index++] = left[c];
			}
		}
	}

	if (!strncmp((char *)uncompr, "DDS ", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".dds");
	} else if (!strncmp((char *)uncompr, "\x89PNG", 4)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".png");
	} else if (!strncmp((char *)uncompr, "\xff\xd8\xff\xe0\x00\x10JFIF", 10)) {
		pkg_res->flags |= PKG_RES_FLAG_APEXT;
		pkg_res->replace_extension = _T(".jpg");
	}

	pkg_res->raw_data = compr;
	pkg_res->raw_data_length = comprlen;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;

	return 0;
}

static cui_ext_operation SLGSystem_alb_operation = {
	SLGSystem_alb_match,			/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	SLGSystem_alb_extract_resource,	/* extract_resource */
	SLGSystem_sz_save_resource,		/* save_resource */
	SLGSystem_sz_release_resource,	/* release_resource */
	SLGSystem_sz_release			/* release */
};

/********************* tig *********************/

static int SLGSystem_tig_match(struct package *pkg)
{
	u32 magic;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->read(pkg, &magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (magic != 0x7cf3c28b) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int SLGSystem_tig_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	u32 tig_size;
	
	if (pkg->pio->length_of(pkg, &tig_size))
		return -CUI_ELEN;

	BYTE *tig = new BYTE[tig_size];
	if (!tig)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, tig, tig_size, 0, IO_SEEK_SET)) {
		delete [] tig;
		return -CUI_EREADVEC;
	}

	srand(0x7F7F7F7F);
	for (DWORD i = 0; i < tig_size; ++i)
		tig[i] -= rand();

	pkg_res->raw_data = tig;
	pkg_res->raw_data_length = tig_size;

	return 0;
}

static void SLGSystem_tig_release_resource(struct package *pkg, 
										   struct package_resource *pkg_res)
{
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static cui_ext_operation SLGSystem_tig_operation = {
	SLGSystem_tig_match,			/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	SLGSystem_tig_extract_resource,	/* extract_resource */
	SLGSystem_sz_save_resource,		/* save_resource */
	SLGSystem_tig_release_resource,	/* release_resource */
	SLGSystem_sz_release			/* release */
};

/********************* VOI *********************/

static int SLGSystem_VOI_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;
}

static int SLGSystem_VOI_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	VOI_header_t VOI_header;
	u32 VOI_size;
	
	if (pkg->pio->length_of(pkg, &VOI_size))
		return -CUI_EREADVEC;
	
	if (pkg->pio->readvec(pkg, &VOI_header, sizeof(VOI_header), 0, IO_SEEK_SET)) 
		return -CUI_EREADVEC;
	
	pkg_res->raw_data_length = VOI_size - sizeof(VOI_header) - VOI_header.offset;
	pkg_res->raw_data = malloc(pkg_res->raw_data_length);
	if (!pkg_res->raw_data) 
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, pkg_res->raw_data_length, 
			sizeof(VOI_header) + VOI_header.offset, IO_SEEK_SET)) {
		free(pkg_res->raw_data);
		return -CUI_EREADVEC;
	}

	return 0;
}

static cui_ext_operation SLGSystem_VOI_operation = {
	SLGSystem_VOI_match,			/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	SLGSystem_VOI_extract_resource,	/* extract_resource */
	SLGSystem_sz_save_resource,		/* save_resource */
	SLGSystem_sz_release_resource,	/* release_resource */
	SLGSystem_sz_release			/* release */
};

/********************* SPD *********************/

static int SLGSystem_SPD_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	SPD_header_t header;

	if (pkg->pio->read(pkg, &header, sizeof(SPD_header_t))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(header.magic, "SFP", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->pio->open(pkg->lst, IO_READONLY)) {
		pkg->pio->close(pkg);
		return -CUI_EOPEN;
	}

	if (pkg->pio->read(pkg->lst, &header, sizeof(SPD_header_t))) {
		pkg->pio->close(pkg->lst);
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(header.magic, "SFP", 4)) {
		pkg->pio->close(pkg->lst);
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;
}

static int SLGSystem_SPD_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{	
	SPD_header_t header;

	if (pkg->pio->readvec(pkg->lst, &header, sizeof(header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	SPL_entry_t entry;

	if (pkg->pio->read(pkg->lst, &entry, sizeof(entry)))
		return -CUI_EREAD;

	DWORD index_len = entry.name_offset - sizeof(header);
	DWORD index_entries = index_len / sizeof(SPL_entry_t);
	SPL_entry_t *index = (SPL_entry_t *)malloc(index_len);
	if (!index)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg->lst, index, index_len, sizeof(header), IO_SEEK_SET)) {
		free(index);
		return -CUI_EREADVEC;
	}

	BYTE *SPL = (BYTE *)malloc(header.size);
	if (!SPL) {
		free(index);
		return -CUI_EMEM;
	}

	if (pkg->pio->readvec(pkg->lst, SPL, header.size, 0, IO_SEEK_SET)) {
		free(SPL);
		free(index);
		return -CUI_EREADVEC;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->index_entry_length = sizeof(SPL_entry_t);
	pkg_dir->directory = index;
	pkg_dir->directory_length = index_len;
	package_set_private(pkg, SPL);

	return 0;
}

static int SLGSystem_SPD_parse_resource_info(struct package *pkg,
											 struct package_resource *pkg_res)
{
	SPL_entry_t *SPL_entry = (SPL_entry_t *)pkg_res->actual_index_entry;
	SPD_header_t *header = (SPD_header_t *)package_get_private(pkg);

	strcpy(pkg_res->name, (char *)header + SPL_entry->name_offset);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = SPL_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = SPL_entry->offset * header->factor;

	return 0;
}

static int SLGSystem_SPD_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	BYTE *VOI = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!VOI)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, VOI, pkg_res->raw_data_length, 
			pkg_res->offset, IO_SEEK_SET)) {
		free(VOI);
		return -CUI_EREADVEC;
	}
	
	pkg_res->raw_data = VOI;

	return 0;
}

static void SLGSystem_SPD_release(struct package *pkg, 
								  struct package_directory *pkg_dir)
{
	void *priv = (void *)package_get_private(pkg);
	if (priv) {
		free(priv);
		package_set_private(pkg, 0);
	}

	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;		
	}
	pkg->pio->close(pkg->lst);
	pkg->pio->close(pkg);
}

static cui_ext_operation SLGSystem_SPD_operation = {
	SLGSystem_SPD_match,				/* match */
	SLGSystem_SPD_extract_directory,	/* extract_directory */
	SLGSystem_SPD_parse_resource_info,	/* parse_resource_info */
	SLGSystem_SPD_extract_resource,		/* extract_resource */
	SLGSystem_sz_save_resource,			/* save_resource */
	SLGSystem_sz_release_resource,		/* release_resource */
	SLGSystem_SPD_release				/* release */
};

int CALLBACK SLGSystem_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".sz"), NULL, 
		NULL, &SLGSystem_sz_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".alb"), NULL, 
		NULL, &SLGSystem_alb_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".SPD"), _T(".VOI"), 
		NULL, &SLGSystem_SPD_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR
		| CUI_EXT_FLAG_LST))
			return -1;

	if (callback->add_extension(callback->cui, _T(".VOI"), _T(".ogg"), 
		NULL, &SLGSystem_VOI_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES 
		| CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	if (callback->add_extension(callback->cui, _T(".tig"), _T(".png"), 
		NULL, &SLGSystem_tig_operation, CUI_EXT_FLAG_PKG))
			return -1;

	return 0;
}
