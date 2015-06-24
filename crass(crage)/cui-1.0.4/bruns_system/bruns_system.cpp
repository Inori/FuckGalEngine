#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <utility.h>
#include <zlib.h>
#include <stdio.h>

struct acui_information bruns_system_cui_information = {
	_T(""),								/* copyright */
	_T("bruns-system"),					/* system */	
	_T(".um3 .brs .bmp .scm .bso .dat .cfg .png .brs"),/* package */
	_T("0.5.0"),								/* revision */
	_T("痴漢公賊"),						/* author */
	_T("2008-3-25 19:03"),				/* date */
	NULL,								/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

#pragma pack (1)
typedef struct {
	s32 magic;
} um3_header_t;

typedef struct {
	s8 magic[4];	/* "EENC" */
	u32 length;
} eenc_header_t;

typedef struct {
	s8 magic[4];	/* "EENZ" */
	u32 length;
} eenz_header_t;

typedef struct {
	u16 unknown_flag0;
	u16 entries;
	u32 unknown_flag1;	// low 8 bits
} dat_header_t;

typedef struct {
	s8 name[14];
	u16 width;
	u16 height;
	u32 bpp;			// low 16 bits
	u32 colors;	
} dat_entry_t;
#pragma pack ()

typedef struct {
	s8 name[MAX_PATH];
	u16 width;
	u16 height;
	u32 bpp;			// low 16 bits
	u32 colors;	
	u32 offset;
	u32 length;
} my_dat_entry_t;

static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static void xor_decode(BYTE *buffer, DWORD len, DWORD magic)
{
	BYTE xor_val[4];

	memcpy(xor_val, &magic, 4);
	for (unsigned int i = 0; i < len; i++)
		buffer[i] ^= xor_val[i & 3];
}

#if 0
/********************* dat *********************/

/* 封包匹配回调函数 */
static int bruns_system_dat_match(struct package *pkg)
{
	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

/* 封包索引目录提取函数 */
static int bruns_system_dat_extract_directory(struct package *pkg,
											  struct package_directory *pkg_dir)
{
	dat_header_t dat_header;
	my_dat_entry_t *my_index_buffer;
	DWORD index_buffer_length, offset;	

	if (!pkg || !pkg_dir)
		return -CUI_EPARA;

	if (pkg->pio->read(pkg, &dat_header, sizeof(dat_header)))
		return -CUI_EREAD;

	index_buffer_length = dat_header.entries * sizeof(my_dat_entry_t);
	my_index_buffer = (my_dat_entry_t *)malloc(index_buffer_length);
	if (!my_index_buffer)
		return -CUI_EMEM;

	offset = sizeof(dat_header_t);
	for (DWORD i = 0; i < dat_header.entries; i++) {
		dat_entry_t dat_entry;

		if (pkg->pio->readvec(pkg, &dat_entry, sizeof(dat_entry_t), offset, IO_SEEK_SET))
			break;

		offset += sizeof(dat_entry_t);

		strncpy(my_index_buffer[i].name, dat_entry.name, sizeof(dat_entry.name));

		if (dat_entry.name[sizeof(dat_entry.name) - 1]) {
			char *p = strstr(my_index_buffer[i].name, ".");

			if (p)
				strcpy(p + 1, "bmp");
			else
				strcpy(&my_index_buffer[i].name[sizeof(dat_entry.name)], ".bmp");
		}

		my_index_buffer[i].width = dat_entry.width;
		my_index_buffer[i].height = dat_entry.height;
		my_index_buffer[i].bpp = dat_entry.bpp;		

		u16 colors = (u16)dat_entry.colors;

		if (dat_header.unknown_flag0 & 2 || dat_header.unknown_flag0 & 0x10) {
			if (pkg->pio->read(pkg, &my_index_buffer[i].uncomprlen, 4))
				break;
			offset += 4;
			my_index_buffer[i].uncomprlen += 4 * colors;
		} 

		if ((u8)(dat_header.unknown_flag1) == 2)	// 是否是压缩
			my_index_buffer[i].comprlen = colors + (u16)dat_entry.bpp * 2;
		else
			my_index_buffer[i].comprlen = (u16)dat_entry.width * (u16)dat_entry.bpp * dat_entry.height + 4 * colors;

		offset += my_index_buffer[i].comprlen;

#if 0
		if (dat_header.unknown_flag0 & 2) {
			if (pkg->pio->read(pkg, &my_index_buffer[i].length, 4))
				break;
			offset += 4;
		} else
			my_index_buffer[i].length = dat_entry.width * dat_entry.height * dat_entry.bpp;
		my_index_buffer[i].offset = offset;
		if (!(dat_header.unknown_flag0 & 0x10))
			my_index_buffer[i].length += dat_entry.colors * 4;
#endif
		my_index_buffer[i].colors = colors;
		offset += my_index_buffer[i].length;
	}
	if (i != dat_header.entries) {
		free(my_index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = dat_header.entries;
	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entry_length = sizeof(my_dat_entry_t);
	package_set_private(pkg, (void *)dat_header.unknown_flag0);

	return 0;
}

/* 封包索引项解析函数 */
static int bruns_system_dat_parse_resource_info(struct package *pkg,
												struct package_resource *pkg_res)
{
	my_dat_entry_t *my_dat_entry;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	my_dat_entry = (my_dat_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_dat_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_dat_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_dat_entry->offset;

	return 0;
}

#if 0
BYTE *dat
{
	int v = (int)dat[0];
	tbl[0x08] = (v >> 7) & 1;
	tbl[0x58] = (v >> 6) & 1;
	tbl[0x10] = (v >> 5) & 1;
	tbl[0x40] = (v >> 4) & 1;
	tbl[0x18] = (v >> 3) & 1;
	tbl[0x60] = (v >> 2) & 1;
	tbl[0x20] = (v >> 1) & 1;
	tbl[0x48] = (v >> 0) & 1;
	v = (int)dat[1];
	tbl[0x84] = (v >> 7) & 1;
	tbl[0x78] = (v >> 6) & 1;
	tbl[0x8c] = (v >> 5) & 1;
	tbl[0xdc] = (v >> 4) & 1;
	tbl[0x94] = (v >> 3) & 1;
	tbl[0xc4] = (v >> 2) & 1;
	tbl[0x9c] = (v >> 1) & 1;
	tbl[0xec] = (v >> 0) & 1;
	v = (int)dat[2];
	tbl[0x58] = (v >> 0) & 1;
	tbl[0x84] = (v >> 7) & 1;
	tbl[0x78] = (v >> 6) & 1;
	tbl[0x8c] = (v >> 5) & 1;
	tbl[0xdc] = (v >> 4) & 1;
	tbl[0x58] = (v >> 3) & 1;
	tbl[0x10] = (v >> 2) & 1;
	tbl[0x08] = (v >> 1) & 1;
	tbl[0x58] = (v >> 0) & 1;
	tbl[0x84] = (v >> 7) & 1;
	tbl[0x78] = (v >> 6) & 1;
	tbl[0x8c] = (v >> 5) & 1;
	tbl[0xdc] = (v >> 4) & 1;
	tbl[0x58] = (v >> 3) & 1;
	tbl[0x10] = (v >> 2) & 1;
	tbl[0x08] = (v >> 1) & 1;
	tbl[0x58] = (v >> 0) & 1;
	tbl[0x84] = (v >> 7) & 1;
	tbl[0x78] = (v >> 6) & 1;
	tbl[0x8c] = (v >> 5) & 1;
	tbl[0xdc] = (v >> 4) & 1;
	tbl[0x58] = (v >> 3) & 1;
	tbl[0x10] = (v >> 2) & 1;
	tbl[0x08] = (v >> 1) & 1;
	tbl[0x58] = (v >> 0) & 1;
}

#define BLK_SIZE	51200
decompress(BYTE *uncompr, DWORD uncomprlen, BYTE *compr, DWORD comprlen)
{
	DWORD blk_count;
	DWORD act_uncomprlen = 0;
	DWORD curbyte = 0;
	
	blk_count = comprlen / BLK_SIZE;
	for (DWORD b = 0; b < blk_count; b++) {
		DWORD len;
		int rep;

		if ((b == blk_count - 1) && (comprlen % BLK_SIZE))
			len = comprlen % BLK_SIZE;
		else
			len = BLK_SIZE;

		rep = 0;
		for (DWORD k = 0; k < len; k++) {
			DWORD count;
			BYTE byte_val;

			switch (rep) {
			case 0:
				uncompr[act_uncomprlen++] = compr[curbyte++];
				rep = 1;
				break;
			case 1:
				uncompr[act_uncomprlen] = compr[curbyte++];
				if (uncompr[act_uncomprlen - 1] == uncompr[act_uncomprlen]) {
					byte_val = uncompr[act_uncomprlen];
					rep = 2;
				}
				act_uncomprlen++;
				break;
			case 2:
				count = compr[curbyte++];
				if (count > 2) {
					memset(&uncompr[act_uncomprlen], byte_val, count - 2);
					act_uncomprlen += count - 2;
				}
				rep = 0;
				break;
			}

			if ((unknown0 & 8) && (act_uncomprlen >= uncomprlen)
				break;
		}
	}
}
#endif
/* 封包资源提取函数 */
static int bruns_system_dat_extract_resource(struct package *pkg,
											 struct package_resource *pkg_res)
{
	BYTE *compr, *uncompr, *dib_data;
	DWORD uncomprlen, dib_len;
	my_dat_entry_t *my_dat_entry;
	u16 unknown_flag0;

	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	compr = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(compr);
			return -CUI_EREADVEC;
	}

	my_dat_entry = (my_dat_entry_t *)pkg_res->actual_index_entry;
	unknown_flag0 = (u16)package_get_private(pkg);
#if 0
	if (unknown_flag0 & 2) {
		/* TODO */
		uncompr = NULL;
		uncomprlen = 0;
		dib_data = compr;
		dib_len = pkg_res->raw_data_length;
	} else {
		dib_data = compr;
		dib_len = pkg_res->raw_data_length;
		if (!(unknown_flag0 & 0x10))
			dib_len -= my_dat_entry->colors * 4;

		if (MyBuildBMPFile(dib_data, dib_len, NULL, 0, my_dat_entry->width, my_dat_entry->height,
				my_dat_entry->bpp * 8, &uncompr, &uncomprlen, my_malloc)) {
			free(compr);
			return -CUI_EMEM;
		}
	}
#endif
	pkg_res->raw_data = compr;
	pkg_res->actual_data = uncompr = NULL;
	pkg_res->actual_data_length = uncomprlen = 0;

	return 0;
}

/* 资源保存函数 */
static int bruns_system_dat_save_resource(struct resource *res, 
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

/* 封包资源释放函数 */
static void bruns_system_dat_release_resource(struct package *pkg, 
											  struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->actual_data && pkg_res->actual_data_length) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
	if (pkg_res->raw_data && pkg_res->raw_data_length) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

/* 封包卸载函数 */
static void bruns_system_dat_release(struct package *pkg, 
									 struct package_directory *pkg_dir)
{
	if (!pkg || !pkg_dir)
		return;

	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

/* 封包处理回调函数集合 */
static cui_ext_operation bruns_system_dat_operation = {
	bruns_system_dat_match,					/* match */
	bruns_system_dat_extract_directory,		/* extract_directory */
	bruns_system_dat_parse_resource_info,	/* parse_resource_info */
	bruns_system_dat_extract_resource,		/* extract_resource */
	bruns_system_dat_save_resource,			/* save_resource */
	bruns_system_dat_release_resource,		/* release_resource */
	bruns_system_dat_release				/* release */
};
#endif

/********************* um3 *********************/

static int bruns_system_um3_match(struct package *pkg)
{
	um3_header_t um3_header;

	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &um3_header, sizeof(um3_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (um3_header.magic != 0xac9898b0) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int bruns_system_um3_extract_resource(struct package *pkg,
											 struct package_resource *pkg_res)
{
	SIZE_T um3_size;
	BYTE *um3_buf;
	
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	if (pkg->pio->length_of(pkg, &um3_size))
		return -CUI_ELEN;

	um3_buf = (BYTE *)malloc(um3_size);
	if (!um3_buf)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, um3_buf, um3_size, 0, IO_SEEK_SET)) {
		free(um3_buf);
		return -CUI_EREADVEC;
	}

	for (unsigned int i = 0; i < 0x800; i++)
		um3_buf[i] = ~um3_buf[i];

	pkg_res->raw_data = um3_buf;
	pkg_res->raw_data_length = um3_size;

	return 0;
}

static int bruns_system_um3_save_resource(struct resource *res, 
										  struct package_resource *pkg_res)
{
	if (!res || !pkg_res)
		return -CUI_EPARA;
	
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}
	res->rio->close(res);
	
	return 0;
}

static void bruns_system_um3_release_resource(struct package *pkg, 
											  struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void bruns_system_um3_release(struct package *pkg, 
									 struct package_directory *pkg_dir)
{
	if (!pkg)
		return;

	pkg->pio->close(pkg);
}

static cui_ext_operation bruns_system_um3_operation = {
	bruns_system_um3_match,				/* match */
	NULL,								/* extract_directory */
	NULL,								/* parse_resource_info */
	bruns_system_um3_extract_resource,	/* extract_resource */
	bruns_system_um3_save_resource,		/* save_resource */
	bruns_system_um3_release_resource,	/* release_resource */
	bruns_system_um3_release			/* release */
};

/********************* eenz *********************/

static int bruns_system_eenz_match(struct package *pkg)
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

	if (strncmp(magic, "EENZ", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int bruns_system_eenz_extract_resource(struct package *pkg,
											  struct package_resource *pkg_res)
{
	u32 eenz_length;
	SIZE_T eenz_size;
	BYTE *eenz_buf, *uncompr;
	DWORD act_uncomprlen;
	
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	if (pkg->pio->length_of(pkg, &eenz_size))
		return -CUI_ELEN;

	if (pkg->pio->readvec(pkg, &eenz_length, 4, 4, IO_SEEK_SET))
		return -CUI_EREADVEC;

	eenz_size -= sizeof(eenz_header_t);

	eenz_buf = (BYTE *)malloc(eenz_size);
	if (!eenz_buf)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, eenz_buf, eenz_size, sizeof(eenz_header_t), IO_SEEK_SET)) {
		free(eenz_buf);
		return -CUI_EREADVEC;
	}

	xor_decode(eenz_buf, eenz_size, eenz_length ^ 0xdeadbeef);

	uncompr = (BYTE *)malloc(eenz_length);
	if (!uncompr) {
		free(eenz_buf);
		return -CUI_EMEM;
	}
	
	act_uncomprlen = eenz_length;
	if (uncompress(uncompr, &act_uncomprlen, eenz_buf, eenz_size) != Z_OK) {
		free(uncompr);
		free(eenz_buf);
		return -CUI_EUNCOMPR;
	}
	free(eenz_buf);
	if (act_uncomprlen != eenz_length) {
		free(uncompr);
		return -CUI_EUNCOMPR;
	}

	pkg_res->raw_data_length = eenz_size + sizeof(eenz_header_t);
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = eenz_length;

	return 0;
}

static int bruns_system_eenz_save_resource(struct resource *res, 
										   struct package_resource *pkg_res)
{
	if (!res || !pkg_res)
		return -CUI_EPARA;
	
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (res->rio->write(res, pkg_res->actual_data, pkg_res->actual_data_length)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}
	res->rio->close(res);
	
	return 0;
}

static void bruns_system_eenz_release_resource(struct package *pkg, 
											   struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->actual_data && pkg_res->actual_data_length) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
}

static void bruns_system_eenz_release(struct package *pkg, 
									  struct package_directory *pkg_dir)
{
	if (!pkg)
		return;

	pkg->pio->close(pkg);
}

static cui_ext_operation bruns_system_eenz_operation = {
	bruns_system_eenz_match,			/* match */
	NULL,								/* extract_directory */
	NULL,								/* parse_resource_info */
	bruns_system_eenz_extract_resource,	/* extract_resource */
	bruns_system_eenz_save_resource,	/* save_resource */
	bruns_system_eenz_release_resource,	/* release_resource */
	bruns_system_eenz_release			/* release */
};

/********************* eenc *********************/

static int bruns_system_eenc_match(struct package *pkg)
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

	if (strncmp(magic, "EENC", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int bruns_system_eenc_extract_resource(struct package *pkg,
											  struct package_resource *pkg_res)
{
	u32 eenc_length;
	SIZE_T eenc_size;
	BYTE *eenc_buf;
	
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	if (pkg->pio->length_of(pkg, &eenc_size))
		return -CUI_ELEN;

	if (pkg->pio->readvec(pkg, &eenc_length, 4, 4, IO_SEEK_SET))
		return -CUI_EREADVEC;

	eenc_size -= sizeof(eenc_header_t);

	eenc_buf = (BYTE *)malloc(eenc_size);
	if (!eenc_buf)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, eenc_buf, eenc_size, sizeof(eenc_header_t), IO_SEEK_SET)) {
		free(eenc_buf);
		return -CUI_EREADVEC;
	}

	xor_decode(eenc_buf, eenc_size, eenc_length ^ 0xdeadbeef);

	pkg_res->raw_data = eenc_buf;
	pkg_res->raw_data_length = eenc_size;

	return 0;
}

static int bruns_system_eenc_save_resource(struct resource *res, 
										   struct package_resource *pkg_res)
{
	if (!res || !pkg_res)
		return -CUI_EPARA;
	
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}
	res->rio->close(res);
	
	return 0;
}

static void bruns_system_eenc_release_resource(struct package *pkg, 
											   struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->raw_data && pkg_res->raw_data_length) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void bruns_system_eenc_release(struct package *pkg, 
									  struct package_directory *pkg_dir)
{
	if (!pkg)
		return;

	pkg->pio->close(pkg);
}

static cui_ext_operation bruns_system_eenc_operation = {
	bruns_system_eenc_match,			/* match */
	NULL,								/* extract_directory */
	NULL,								/* parse_resource_info */
	bruns_system_eenc_extract_resource,	/* extract_resource */
	bruns_system_eenc_save_resource,	/* save_resource */
	bruns_system_eenc_release_resource,	/* release_resource */
	bruns_system_eenc_release			/* release */
};

/********************* png *********************/

static int bruns_system_png_match(struct package *pkg)
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

static int bruns_system_png_extract_resource(struct package *pkg,
											 struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	if (pkg->pio->length_of(pkg, &pkg_res->raw_data_length))
		return -CUI_ELEN;

	pkg_res->raw_data = pkg->pio->readvec_only(pkg, pkg_res->raw_data_length, 0, IO_SEEK_SET);
	if (!pkg_res->raw_data)
		return -CUI_EREADVEC;

	return 0;
}

static int bruns_system_png_save_resource(struct resource *res, 
										  struct package_resource *pkg_res)
{
	if (!res || !pkg_res)
		return -CUI_EPARA;
	
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}
	res->rio->close(res);
	
	return 0;
}

static void bruns_system_png_release_resource(struct package *pkg, 
											  struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->raw_data && pkg_res->raw_data_length)
		pkg_res->raw_data = NULL;
}

static void bruns_system_png_release(struct package *pkg, 
									 struct package_directory *pkg_dir)
{
	if (!pkg)
		return;

	pkg->pio->close(pkg);
}

static cui_ext_operation bruns_system_png_operation = {
	bruns_system_png_match,				/* match */
	NULL,								/* extract_directory */
	NULL,								/* parse_resource_info */
	bruns_system_png_extract_resource,	/* extract_resource */
	bruns_system_png_save_resource,		/* save_resource */
	bruns_system_png_release_resource,	/* release_resource */
	bruns_system_png_release			/* release */
};

int CALLBACK bruns_system_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".um3"), _T(".ogg"), 
		_T("Ultramarine3－Ogg"), &bruns_system_um3_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".brs"), _T(".bmp"), 
		NULL, &bruns_system_eenz_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".bmp"), NULL, 
		NULL, &bruns_system_eenz_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".scm"), _T(".txt"), 
		NULL, &bruns_system_eenz_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".bso"), _T(".txt"), 
		NULL, &bruns_system_eenz_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), _T(".txt"), 
		NULL, &bruns_system_eenz_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".cfg"), _T(".txt"), 
		NULL, &bruns_system_eenz_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".png"), NULL, 
		NULL, &bruns_system_eenc_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".brs"), _T(".png"), 
		NULL, &bruns_system_eenc_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".brs"), _T(".png"), 
		NULL, &bruns_system_png_operation, CUI_EXT_FLAG_PKG))
			return -1;

//	if (callback->add_extension(callback->cui, _T(".dat"), _T(".bmp"), 
//		NULL, &bruns_system_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_NO_MAGIC))
//			return -1;

	return 0;
}
