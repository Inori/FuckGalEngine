#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <resource.h>
#include <package.h>
#include <cui_error.h>
#include <zlib.h>
#include <utility.h>
#include <stdio.h>
#include <cui_common.h>
	
struct acui_information AGSD_cui_information = {
	_T("yamaneko"),						/* copyright */
	_T("Advanced Game Script Decoder"),	/* system */
	_T(".gsp .bmz"),					/* package */
	_T("1.0.2"),						/* revison */
	_T("³Õh¹«Ù\"),						/* author */
	_T("2008-4-26 11:43"),				/* date */
	NULL,								/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};	

#pragma pack (1)
typedef struct {
	u32 index_entries;
} gsp_header_t;

typedef struct {
	u32 offset;
	u32 length;
	s8 name[56];
} gsp_entry_t;

typedef struct {
	s8 magic[4];	// "ZLC3"
	u32 length;		// data actual length
} bmz_header_t;
#pragma pack ()	

    
static void *my_malloc(DWORD len)
{
	return malloc(len);
}

/********************* gsp *********************/

static int AGSD_gsp_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

static int AGSD_gsp_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{
	gsp_header_t gsp_header;
	gsp_entry_t *index_buffer;
	unsigned int index_length;
	
	if (pkg->pio->read(pkg, &gsp_header, sizeof(gsp_header)))
		return -CUI_EREAD;
	
	if (!gsp_header.index_entries)
		return -CUI_EMATCH;
	
	index_length = gsp_header.index_entries * sizeof(gsp_entry_t);
	index_buffer = (gsp_entry_t *)malloc(index_length);
	if (!index_buffer)
		return -CUI_EMEM;
	
	if (pkg->pio->read(pkg, index_buffer, index_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = gsp_header.index_entries;
	pkg_dir->index_entry_length = sizeof(gsp_entry_t);
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_length;
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

static int AGSD_gsp_parse_resource_info(struct package *pkg,
										struct package_resource *pkg_res)
{
	gsp_entry_t *gsp_entry;

	gsp_entry = (gsp_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, gsp_entry->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = gsp_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = gsp_entry->offset;

	return 0;
}

static int AGSD_gsp_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{	
	BYTE *raw;
	
	raw = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, pkg_res->raw_data_length, pkg_res->offset, IO_SEEK_SET)) {
		free(raw);
		return -CUI_EREADVEC;
	}
	pkg_res->raw_data = raw;
	
	return 0;
}

static cui_ext_operation AGSD_gsp_operation = {
	AGSD_gsp_match,					/* match */
	AGSD_gsp_extract_directory,		/* extract_directory */
	AGSD_gsp_parse_resource_info,	/* parse_resource_info */
	AGSD_gsp_extract_resource,		/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* spt *********************/

static void convert_spt_mode0(BYTE *buf, DWORD buf_len)
{
	DWORD act_buf_len = buf_len - 2;
	BYTE byte[2];

	for (DWORD i = 0; i < act_buf_len / 2; i++) {
		byte[0] = buf[i * 2 + 0];
		byte[1] = buf[i * 2 + 1];
		buf[i * 2 + 0] = byte[1];
		buf[i * 2 + 1] = byte[0];
	}
}

static void convert_spt_mode1(BYTE *buf, DWORD buf_len)
{
	DWORD act_buf_len = buf_len - 4;
	BYTE byte[4];

	for (DWORD i = 0; i < act_buf_len / 4; i++) {
		byte[0] = buf[i * 4 + 0];
		byte[1] = buf[i * 4 + 1];
		byte[2] = buf[i * 4 + 2];
		byte[3] = buf[i * 4 + 3];
		buf[i * 4 + 0] = byte[2];
		buf[i * 4 + 1] = byte[3];
		buf[i * 4 + 2] = byte[0];
		buf[i * 4 + 3] = byte[1];
	}
}

static void convert_spt_mode2(BYTE *buf, DWORD buf_len)
{
	DWORD act_buf_len = buf_len - 8;
	BYTE byte[8];

	for (DWORD i = 0; i < act_buf_len / 8; i++) {
		byte[0] = buf[i * 8 + 0];
		byte[1] = buf[i * 8 + 1];
		byte[2] = buf[i * 8 + 2];
		byte[3] = buf[i * 8 + 3];
		byte[4] = buf[i * 8 + 4];
		byte[5] = buf[i * 8 + 5];
		byte[6] = buf[i * 8 + 6];
		byte[7] = buf[i * 8 + 7];
		buf[i * 8 + 0] = byte[6];
		buf[i * 8 + 1] = byte[4];
		buf[i * 8 + 2] = byte[5];
		buf[i * 8 + 3] = byte[7];
		buf[i * 8 + 4] = byte[1];
		buf[i * 8 + 5] = byte[2];
		buf[i * 8 + 6] = byte[0];
		buf[i * 8 + 7] = byte[3];
	}
}

static void convert_spt(BYTE *buf, DWORD buf_len, u8 mode)
{
	switch (mode) {
	case 0:
		convert_spt_mode0(buf, buf_len);
		break;
	case 1:
		convert_spt_mode1(buf, buf_len);
		break;
	case 2:
		convert_spt_mode2(buf, buf_len);
		break;
	}
}

static void decode_spt(s32 (*spt_decode_table)[8], BYTE *buf, DWORD buf_len, int index)
{
	if (index < 0 || index > 7)
		return;

	for (DWORD i = 0; i < buf_len; i++) {
		BYTE byteval;
		BYTE curbyte;		

		curbyte = buf[i];
		byteval = 0;

		for (int curbit = 0; curbit < 8; curbit++) {
			s32 code = spt_decode_table[index + 8][curbit];
			if (code < 0) 
				byteval |= (curbyte & (1 << curbit)) >> ((0 - code) & 0xff);
			else
				byteval |= ((curbyte & (1 << curbit)) << (code & 0xff));
		}
		buf[i] = byteval ^ 0xff;
	}
}

static void init_spt_decode_table(s32 (*spt_decode_table)[8])
{
	spt_decode_table[0][0] = 5;
	spt_decode_table[0][1] = 1;
	spt_decode_table[0][2] = -2;
	spt_decode_table[0][3] = 0;
	spt_decode_table[0][4] = 2;
	spt_decode_table[0][5] = -1;
	spt_decode_table[0][6] = 1;
	spt_decode_table[0][7] = -6;

	spt_decode_table[1][0] = 6;
	spt_decode_table[1][1] = 3;
	spt_decode_table[1][2] = 0;
	spt_decode_table[1][3] = -3;
	spt_decode_table[1][4] = 3;
	spt_decode_table[1][5] = -4;
	spt_decode_table[1][6] = -3;
	spt_decode_table[1][7] = -2;

	spt_decode_table[2][0] = 7;
	spt_decode_table[2][1] = 5;
	spt_decode_table[2][2] = 3;
	spt_decode_table[2][3] = 1;
	spt_decode_table[2][4] = -1;
	spt_decode_table[2][5] = -3;
	spt_decode_table[2][6] = -5;
	spt_decode_table[2][7] = -7;

	spt_decode_table[3][0] = 3;
	spt_decode_table[3][1] = 1;
	spt_decode_table[3][2] = -1;
	spt_decode_table[3][3] = -3;
	spt_decode_table[3][4] = 3;
	spt_decode_table[3][5] = 1;
	spt_decode_table[3][6] = -1;
	spt_decode_table[3][7] = -3;

	spt_decode_table[4][0] = 2;
	spt_decode_table[4][1] = 3;
	spt_decode_table[4][2] = 5;
	spt_decode_table[4][3] = 2;
	spt_decode_table[4][4] = -3;
	spt_decode_table[4][5] = 1;
	spt_decode_table[4][6] = -6;
	spt_decode_table[4][7] = -4;

	spt_decode_table[5][0] = 4;
	spt_decode_table[5][1] = 4;
	spt_decode_table[5][2] = 1;
	spt_decode_table[5][3] = 4;
	spt_decode_table[5][4] = 2;
	spt_decode_table[5][5] = -5;
	spt_decode_table[5][6] = -4;
	spt_decode_table[5][7] = -6;

	spt_decode_table[6][0] = 1;
	spt_decode_table[6][1] = 2;
	spt_decode_table[6][2] = 2;
	spt_decode_table[6][3] = 3;
	spt_decode_table[6][4] = 1;
	spt_decode_table[6][5] = 2;
	spt_decode_table[6][6] = -4;
	spt_decode_table[6][7] = -7;

	spt_decode_table[7][0] = 2;
	spt_decode_table[7][1] = -1;
	spt_decode_table[7][2] = 1;
	spt_decode_table[7][3] = 4;
	spt_decode_table[7][4] = 2;
	spt_decode_table[7][5] = -1;
	spt_decode_table[7][6] = -5;
	spt_decode_table[7][7] = -2;

	spt_decode_table[8][0] = 2;
	spt_decode_table[8][1] = 6;
	spt_decode_table[8][2] = -1;
	spt_decode_table[8][3] = 0;
	spt_decode_table[8][4] = 1;
	spt_decode_table[8][5] = -5;
	spt_decode_table[8][6] = -2;
	spt_decode_table[8][7] = -1;

	spt_decode_table[9][0] = 3;
	spt_decode_table[9][1] = 4;
	spt_decode_table[9][2] = 0;
	spt_decode_table[9][3] = 3;
	spt_decode_table[9][4] = -3;
	spt_decode_table[9][5] = 2;
	spt_decode_table[9][6] = -6;
	spt_decode_table[9][7] = -3;

	spt_decode_table[10][0] = 7;
	spt_decode_table[10][1] = 5;
	spt_decode_table[10][2] = 3;
	spt_decode_table[10][3] = 1;
	spt_decode_table[10][4] = -1;
	spt_decode_table[10][5] = -3;
	spt_decode_table[10][6] = -5;
	spt_decode_table[10][7] = -7;

	spt_decode_table[11][0] = 3;
	spt_decode_table[11][1] = 1;
	spt_decode_table[11][2] = -1;
	spt_decode_table[11][3] = -3;
	spt_decode_table[11][4] = 3;
	spt_decode_table[11][5] = 1;
	spt_decode_table[11][6] = -1;
	spt_decode_table[11][7] = -3;

	spt_decode_table[12][0] = 6;
	spt_decode_table[12][1] = 3;
	spt_decode_table[12][2] = -2;
	spt_decode_table[12][3] = 4;
	spt_decode_table[12][4] = -3;
	spt_decode_table[12][5] = -2;
	spt_decode_table[12][6] = -1;
	spt_decode_table[12][7] = -5;

	spt_decode_table[13][0] = 5;
	spt_decode_table[13][1] = 6;
	spt_decode_table[13][2] = 4;
	spt_decode_table[13][3] = -1;
	spt_decode_table[13][4] = -4;
	spt_decode_table[13][5] = -4;
	spt_decode_table[13][6] = -2;
	spt_decode_table[13][7] = -4;

	spt_decode_table[14][0] = 7;
	spt_decode_table[14][1] = -1;
	spt_decode_table[14][2] = 4;
	spt_decode_table[14][3] = -2;
	spt_decode_table[14][4] = -2;
	spt_decode_table[14][5] = -1;
	spt_decode_table[14][6] = -3;
	spt_decode_table[14][7] = -2;

	spt_decode_table[15][0] = 1;
	spt_decode_table[15][1] = 5;
	spt_decode_table[15][2] = -2;
	spt_decode_table[15][3] = -1;
	spt_decode_table[15][4] = 1;
	spt_decode_table[15][5] = 2;
	spt_decode_table[15][6] = -2;
	spt_decode_table[15][7] = -4;	
}

static int AGSD_spt_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

static int AGSD_spt_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{	
	BYTE *raw;
	u8 mode, table_index, unknown0, unknown1;
	s32 spt_decode_table[16][8];
	u32 spt_size;

	if (pkg->pio->length_of(pkg, &spt_size))
		return -CUI_ELEN;

	raw = (BYTE *)malloc(spt_size);
	if (!raw)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, raw, spt_size, 0, IO_SEEK_SET)) {
		free(raw);
		return -CUI_EREADVEC;
	}
	
	table_index = raw[0] ^ 0xf0;
	mode = raw[1] ^ 0xf0;
	unknown0 = raw[2];
	unknown1 = raw[3];

	convert_spt(raw + 4,  spt_size - 4, mode);
	init_spt_decode_table(spt_decode_table);
	decode_spt(spt_decode_table, raw + 4, spt_size - 4, table_index);	

	pkg_res->actual_data_length = spt_size - 4;
	pkg_res->actual_data = (BYTE *)malloc(spt_size - 4);
	if (!pkg_res->actual_data) {
		free(raw);
		return -CUI_EMEM;
	}
	memcpy(pkg_res->actual_data, raw + 4, pkg_res->actual_data_length);
	pkg_res->raw_data = raw;
	pkg_res->raw_data_length = spt_size;
	
	return 0;
}

static cui_ext_operation AGSD_spt_operation = {
	AGSD_spt_match,				/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	AGSD_spt_extract_resource,	/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* bmz *********************/

static int AGSD_bmz_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->readvec(pkg, magic, sizeof(magic), 0, IO_SEEK_SET)) 
		return -CUI_EREADVEC;

	if (memcmp(magic, "ZLC3", 4))
		return -CUI_EMATCH;

	return 0;	
}

static int AGSD_bmz_extract_resource(struct package *pkg,
									 struct package_resource *pkg_res)
{	
	BYTE *compr, *uncompr;
	u32 uncomprlen, comprlen;
	u32 bmz_size;
	int ret;
	
	if (pkg->pio->length_of(pkg, &bmz_size))
		return -CUI_ELEN;
	
	if (pkg->pio->readvec(pkg, &uncomprlen, 4, 4, IO_SEEK_SET))
		return -CUI_EREADVEC;	

	comprlen = bmz_size - sizeof(bmz_header_t);
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, sizeof(bmz_header_t), IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}	
	
	uncompr = (BYTE *)malloc(uncomprlen);
	if (!uncompr) {
		free(compr);
		return -CUI_EMEM;
	}

	DWORD act_uncomprlen = uncomprlen;
	ret = uncompress(uncompr, &act_uncomprlen, compr, comprlen);
	if ((ret != Z_OK) || (act_uncomprlen != uncomprlen)) {
		free(uncompr);
		free(compr);
		return -CUI_EUNCOMPR;
	}

	pkg_res->raw_data = compr;	
	pkg_res->raw_data_length = bmz_size;
	pkg_res->actual_data_length = act_uncomprlen;
	pkg_res->actual_data = uncompr;
	
	return 0;
}

static cui_ext_operation AGSD_bmz_operation = {
	AGSD_bmz_match,				/* match */
	NULL,						/* extract_directory */
	NULL,						/* parse_resource_info */
	AGSD_bmz_extract_resource,	/* extract_resource */
	CUI_COMMON_OPERATION
};
	
int CALLBACK AGSD_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".gsp"), NULL, 
		NULL, &AGSD_gsp_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;
	
	if (callback->add_extension(callback->cui, _T(".spt"), NULL, 
		NULL, &AGSD_spt_operation, CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".bmz"), _T(".bmp"), 
		NULL, &AGSD_bmz_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;
	
	return 0;
}
