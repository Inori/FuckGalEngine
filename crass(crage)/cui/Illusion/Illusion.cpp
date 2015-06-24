#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <resource.h>
#include <package.h>
#include <cui_error.h>
#include <utility.h>
#include <stdio.h>

struct acui_information Illusion_cui_information = {
	_T("illusion"),					/* copyright */
	NULL,							/* system */
	_T(".pp"),						/* package */
	_T("0.0.2"),					/* revision */
	_T("痴汉公贼"),					/* author */
	_T("2007-11-15 22:18"),			/* date */
	NULL,							/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

#pragma pack (1)
typedef struct {
	u8 unknown;
	u32 index_entries;
} pp_header_t;

typedef struct {
	s8 name[260];
	u32 length;
	u32 offset;
} pp_entry_t;

typedef struct {
	u32 index_entries;
	u32 data_length;
} js3_base_pp_header_t;

typedef struct {
	u32 width;
	u32 height;
	s8 *suffix;
} ema_header_t;
#pragma pack ()	

typedef struct {
	s8 name[32];
	u32 length;
	u32 offset;
} my_js3_base_pp_entry_t;


static const char *Illusion_game[] = {
	"JS3T",
	"SchoolMate",
	NULL
};

static int Illusion_game_id;

static void js3_base_pp_decode(char *dat, DWORD dat_length)
{
	for (DWORD i = 0; i < dat_length; i++)
		dat[i] = (int)dat[i] * -1;
}

static void decode(BYTE *enc_buf, DWORD enc_buf_len)
{
	u8 decode_tbl[2][8] = {
		{ 0xfa, 0x49, 0x7b, 0x1c, 0xf9, 0x4d, 0x83, 0x0a },
		{ 0x3a, 0xe3, 0x87, 0xc2, 0xbd, 0x1e, 0xa6, 0xfe }
	};
	
	for (DWORD i = 0; i < enc_buf_len; i++) {
		unsigned int idx = i & 7;
		decode_tbl[0][idx] = decode_tbl[0][idx] + decode_tbl[1][idx];
		enc_buf[i] ^= decode_tbl[0][idx];
	}
}

static js3_decode_resource(WORD *enc_buf, DWORD enc_buf_len)
{
	u16 decode_tbl[2][4] = {
		{ 0x00ca, 0x006e, 0x000d, 0x00b3 },
		{ 0x009c, 0x0036, 0x001e, 0x00e8 }
	};
	u16 tbl[2][4];
	
	for (DWORD k = 0; k < 4; k++) {
		tbl[0][k] = decode_tbl[0][k];
		tbl[1][k] = decode_tbl[1][k];
	}
	
	DWORD dec_len = enc_buf_len / 2;
	
	k = 0;
	for (DWORD i = 0; i < dec_len; i++) {
		tbl[0][k] += tbl[1][k];
		enc_buf[i] ^= tbl[0][k];
		k++;
		k &= 3;
	}
}

static void decode_resource(DWORD *enc_buf, DWORD enc_buf_len)
{
	// SchoolMate体验版
	//u32 decode_tbl[8] = { 0x7735F27C, 0x6E201854, 0x859E7B9C, 0x4071B51F, 
	//	0x4371AD25, 0x7E202064, 0xC085E3CF, 0x1223DE41 };	// ver2
	// SchoolMate
	u32 decode_tbl[8] = { 0xDD135D1E, 0xA74F4C7D, 0x1429A7DB, 0xBEC0F810, 
		0x63D07F44, 0x9F7C221C, 0xBEF8B9E8, 0xF4EFB358 };	// ver2

	unsigned int dec_len = enc_buf_len / 4;

	for (unsigned int i = 0; i < dec_len; i++)
		enc_buf[i] ^= decode_tbl[i & 7];
}

/********************* pp *********************/

static int Illusion_pp_match(struct package *pkg)
{	
	u8 unknown;
	const char *game;

	if (!pkg)
		return -CUI_EPARA;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;
	
	if (pkg->pio->read(pkg, &unknown, 1)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}
	
	decode(&unknown, 1);
	game = get_options("game");
	if (!game)
		Illusion_game_id = 0;
	else {
		for (DWORD i = 0; Illusion_game[i]; i++) {
			if (!lstrcmpiA(Illusion_game[i], game)) {
				Illusion_game_id = i;
				break;
			}
		}
		if (!Illusion_game[i])
			Illusion_game_id = 0;
	}

	return 0;	
}

static int Illusion_pp_extract_directory(struct package *pkg,
										   struct package_directory *pkg_dir)
{		
	unsigned int index_buffer_len;
	pp_entry_t *index_buffer;
	u32 index_entries;

	if (!pkg || !pkg_dir)
		return -CUI_EPARA;
	
	if (pkg->pio->read(pkg, &index_entries, 4))
		return -CUI_EREAD;

	decode((BYTE *)&index_entries, 4);

	index_buffer_len = index_entries * sizeof(pp_entry_t);		
	index_buffer = (pp_entry_t *)malloc(index_buffer_len);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_len)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	decode((BYTE *)index_buffer, index_buffer_len);

	pkg_dir->index_entries = index_entries;
	pkg_dir->index_entry_length = sizeof(pp_entry_t);
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_len;

	return 0;
}

static int Illusion_pp_parse_resource_info(struct package *pkg,
											 struct package_resource *pkg_res)
{
	pp_entry_t *pp_entry;
	
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	pp_entry = (pp_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, pp_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = pp_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = pp_entry->offset;

	return 0;
}

static int Illusion_pp_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{	
	//u8 decode_tbl2[8] = { 0x6b, 0x42, 0x6c, 0xa2, 0x72, 0x30, 0x7d, 0x22 };
	BYTE *data;
	void *act_data;
	DWORD act_data_len;
	
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	data = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, data, pkg_res->raw_data_length,	pkg_res->offset, IO_SEEK_SET)) {
		free(data);
		return -CUI_EREADVEC;
	}

	if (!Illusion_game_id)
		js3_decode_resource((WORD *)data, pkg_res->raw_data_length);
	else if (Illusion_game_id == 1)
		decode_resource((DWORD *)data, pkg_res->raw_data_length);
	
	act_data = NULL;
	act_data_len = 0;
	if (strstr(pkg_res->name, ".ema")) {
		int suffix_len;
		
		for (suffix_len = 0; ; suffix_len++) {
			if (!data[8 + suffix_len])
				break;
		}
		suffix_len++;
		if (!strcmp((char *)&data[8], ".tga")) {
			pkg_res->replace_extension = _T(".tga");
			pkg_res->flags |= PKG_RES_FLAG_APEXT;
		}
		act_data_len = pkg_res->raw_data_length - 8 - suffix_len;
		act_data = malloc(act_data_len);
		if (!act_data) {
			free(data);
			return -CUI_EMEM;
		}
		memcpy(act_data, &data[8 + suffix_len], act_data_len);		
	}

	pkg_res->raw_data = data;
	pkg_res->actual_data = act_data;
	pkg_res->actual_data_length = act_data_len;	

	return 0;
}

static int Illusion_pp_save_resource(struct resource *res, 
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

static void Illusion_pp_release_resource(struct package *pkg, 
										   struct package_resource *pkg_res)
{
	if (!pkg || !pkg_res)
		return;

	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void Illusion_pp_release(struct package *pkg, 
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

static cui_ext_operation Illusion_pp_operation = {
	Illusion_pp_match,				/* match */
	Illusion_pp_extract_directory,	/* extract_directory */
	Illusion_pp_parse_resource_info,	/* parse_resource_info */
	Illusion_pp_extract_resource,		/* extract_resource */
	Illusion_pp_save_resource,		/* save_resource */
	Illusion_pp_release_resource,		/* release_resource */
	Illusion_pp_release				/* release */
};

/********************* base.pp *********************/

static int Illusion_js3_base_pp_match(struct package *pkg)
{	
	if (!pkg)
		return -CUI_EPARA;
	
	if (lstrcmp(pkg->name, _T("base.pp")))
		return -CUI_EMATCH;
	
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;
}

static int Illusion_js3_base_pp_extract_directory(struct package *pkg,
											   struct package_directory *pkg_dir)
{
	js3_base_pp_header_t js3_base_pp_header;
	DWORD name_buffer_length, length_buffer_length, index_buffer_length;
	char *name_buffer;
	DWORD *length_buffer;
	my_js3_base_pp_entry_t *js3_base_pp_index_buffer;
	DWORD base_offset;

	if (!pkg || !pkg_dir)
		return -CUI_EPARA;
	
	if (pkg->pio->read(pkg, &js3_base_pp_header, sizeof(js3_base_pp_header)))
		return -CUI_EREAD;
	
	name_buffer_length = js3_base_pp_header.index_entries * 32;
	name_buffer = (char *)malloc(name_buffer_length);
	if (!name_buffer)
		return -CUI_EMEM;
	
	if (pkg->pio->read(pkg, name_buffer, name_buffer_length))
		return -CUI_EREAD;
	
	js3_base_pp_decode(name_buffer, name_buffer_length);
	
	length_buffer_length = js3_base_pp_header.index_entries * 4;
	length_buffer = (DWORD *)malloc(length_buffer_length);
	if (!length_buffer) {
		free(name_buffer);
		return -CUI_EMEM;
	}
	
	if (pkg->pio->read(pkg, length_buffer, length_buffer_length)) {
		free(name_buffer);
		return -CUI_EREAD;
	}
	
	index_buffer_length = js3_base_pp_header.index_entries * sizeof(my_js3_base_pp_entry_t);
	js3_base_pp_index_buffer = (my_js3_base_pp_entry_t *)malloc(index_buffer_length);
	if (!js3_base_pp_index_buffer) {
		free(length_buffer);
		free(name_buffer);
		return -CUI_EMEM;
	}
	
	base_offset = sizeof(js3_base_pp_header_t) + name_buffer_length + length_buffer_length;
	for (DWORD i = 0; i < js3_base_pp_header.index_entries; i++) {
		strcpy(js3_base_pp_index_buffer[i].name, name_buffer + i * 32);
		js3_base_pp_index_buffer[i].length = length_buffer[i];
		js3_base_pp_index_buffer[i].offset = base_offset;
		base_offset += length_buffer[i];
	}
	free(length_buffer);
	free(name_buffer);

	pkg_dir->index_entries = js3_base_pp_header.index_entries;
	pkg_dir->index_entry_length = sizeof(my_js3_base_pp_entry_t);
	pkg_dir->directory = js3_base_pp_index_buffer;
	pkg_dir->directory_length = index_buffer_length;

	return 0;
}

static int Illusion_js3_base_pp_parse_resource_info(struct package *pkg,
													struct package_resource *pkg_res)
{
	my_js3_base_pp_entry_t *my_js3_base_pp_entry;
	
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	my_js3_base_pp_entry = (my_js3_base_pp_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_js3_base_pp_entry->name);
	pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
	pkg_res->raw_data_length = my_js3_base_pp_entry->length;
	pkg_res->actual_data_length = 0;	/* 数据都是明文 */
	pkg_res->offset = my_js3_base_pp_entry->offset;

	return 0;
}

static int Illusion_js3_base_pp_extract_resource(struct package *pkg,
												 struct package_resource *pkg_res)
{	
	BYTE *data;
	
	if (!pkg || !pkg_res)
		return -CUI_EPARA;

	data = (BYTE *)malloc(pkg_res->raw_data_length);
	if (!data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, data, pkg_res->raw_data_length,	pkg_res->offset, IO_SEEK_SET)) {
		free(data);
		return -CUI_EREADVEC;
	}

	js3_base_pp_decode((char *)data, pkg_res->raw_data_length);

	pkg_res->raw_data = data;
	return 0;
}

static cui_ext_operation Illusion_js3_base_pp_operation = {
	Illusion_js3_base_pp_match,				/* match */
	Illusion_js3_base_pp_extract_directory,	/* extract_directory */
	Illusion_js3_base_pp_parse_resource_info,	/* parse_resource_info */
	Illusion_js3_base_pp_extract_resource,		/* extract_resource */
	Illusion_pp_save_resource,		/* save_resource */
	Illusion_pp_release_resource,		/* release_resource */
	Illusion_pp_release				/* release */
};

int CALLBACK Illusion_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".pp"), NULL, 
		NULL, &Illusion_js3_base_pp_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;
	
	if (callback->add_extension(callback->cui, _T(".pp"), NULL, 
		NULL, &Illusion_pp_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_WEAK_MAGIC))
			return -1;

	return 0;
}
