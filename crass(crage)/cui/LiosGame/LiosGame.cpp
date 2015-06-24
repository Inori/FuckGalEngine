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

// 部分wav是需要Ogg Vorbis ACM codec才能支持（fmt tag字段是"qg"）

#define getbits	bits_get_high

struct acui_information LiosGame_cui_information = {
	_T("Liar"),				/* copyright */
	_T("LiosGame"),			/* system */
	_T(".xfl .lwg .wcg .lim"),	/* package */
	_T("0.9.7"),				/* revision */
	_T("痴汉公贼"),			/* author */
	_T("2009-4-10 23:07"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};


#pragma pack (1)
typedef struct {
	s8 magic[2];		/* "LB" */
	u16 version;		/* must be 1 */
	u32 index_length;		
	u32 index_entries;
} xfl_header_t;

typedef struct {
	s8 name[32];
	u32 offset;
	u32 length;
} xfl_entry_t;

/* 包含了一屏中所有组件的图像块 */
typedef struct {
	s8 magic[2];		/* "LG" */
	u16 version;		/* must be 1 */
	u32 height;
	u32 width;
	u32 entries;		/* 组件数 */
	u32 reserved;	
	u32 index_length;
} lwg_header_t;

typedef struct {
	u32 x_poisition;	/* x轴起点坐标 */
	u32 y_position;		/* x轴起点坐标 */
	u8 unknown;			/* 总是0x28 */
	u32 offset;
	u32 length;
	u8 name_length;
	s8 *name;
} lwg_entry_t;

typedef struct {
	s8 magic[2];		/* "WG" */
	u16 flags;
	u32 unknown;
	u32 width;
	u32 height;	
} wcg_header_t;

typedef struct {
	u32 uncomprlen;		/* 解压后的长度 */
	u32 comprlen;	
	u16 code_number;	/* 每个code 2字节 */
	u16 unknown;
} wcg_info_t;

typedef struct {
	s8 magic[2];		/* "LM" */
	u8 flag0;			// (flags & 0xf: 2 - 3 - ; etc - invalid）
	u8 flag1;
	u32 unknown1;
	u32 width;
	u32 height;	
} lim_header_t;

typedef struct {
	u32 uncomprlen;		// 解压后的长度
	u32 comprlen;		// lz+字典压缩的长度
	u16 code_number;	// 字典长度
	u16 unknown;		
} lim_info_t;
#pragma pack ()
	

static void *malloc_callback(DWORD len)
{
	return malloc(len);
}

static int getcode(struct bits *bits_struct, unsigned int bitval,
		unsigned int *ret_code)
{
	unsigned int code = 0;
	int ret = -1;
	
	if (!bitval)
		goto out;
	
	switch (bitval) {
	case 1:
		if (getbits(bits_struct, 1, &code))
			goto out;
		break;
	case 2:
		if (getbits(bits_struct, 1, &code))
			goto out;
		code |= 0x2;	
		break;
	case 3:
		if (getbits(bits_struct, 2, &code))
			goto out;
		code |= 0x4;	
		break;						
	case 4:
		if (getbits(bits_struct, 3, &code))
			goto out;
		code |= 0x8;	
		break;
	case 5:
		if (getbits(bits_struct, 4, &code))
			goto out;
		code |= 0x10;
		break;
	case 6:
		if (getbits(bits_struct, 5, &code))
			goto out;
		code |= 0x20;
		break;
	case 7:
		if (getbits(bits_struct, 1, &bitval))
			goto out;
		if (!bitval) {
			if (getbits(bits_struct, 6, &code))
				goto out;
			code |= 0x40;
			break;						
		}
			
		if (getbits(bits_struct, 1, &bitval))
			goto out;
		if (!bitval) {
			if (getbits(bits_struct, 7, &code))
				goto out;
			code |= 0x80;
			break;						
		}				
			
		if (getbits(bits_struct, 1, &bitval))
			goto out;
		if (!bitval) {
			if (getbits(bits_struct, 8, &code))
				goto out;
			code |= 0x100;	
			break;					
		}	
			
		if (getbits(bits_struct, 1, &bitval))
			goto out;
		if (!bitval) {
			if (getbits(bits_struct, 9, &code))
				goto out;
			code |= 0x200;
			break;						
		}	
			
		if (getbits(bits_struct, 1, &bitval))
			goto out;
		if (!bitval) {
			if (getbits(bits_struct, 10, &code))
				goto out;
			code |= 0x400;	
			break;					
		}				
			
		if (getbits(bits_struct, 1, &bitval))
			goto out;
		if (!bitval) {
			if (getbits(bits_struct, 11, &code))
				goto out;
			code |= 0x800;
			break;
		}
		goto out;				
	}		
	ret = 0;
	*ret_code = code;
out:
	return ret;
}
		
static int getcode2(struct bits *bits_struct, unsigned int bitval,
		unsigned int *ret_code)
{
	unsigned int code = 0;
	int ret = -1;
	
	if (!bitval || bitval > 15)
		goto out;
	
	switch (bitval) {
	case 1:
		if (getbits(bits_struct, 1, &code))
			goto out;
		break;
	case 2:
		if (getbits(bits_struct, 1, &code))
			goto out;
		code |= 0x2;	
		break;
	case 3:
		if (getbits(bits_struct, 2, &code))
			goto out;
		code |= 0x4;	
		break;						
	case 4:
		if (getbits(bits_struct, 3, &code))
			goto out;
		code |= 0x8;	
		break;
	case 5:
		if (getbits(bits_struct, 4, &code))
			goto out;
		code |= 0x10;
		break;
	case 6:
		if (getbits(bits_struct, 5, &code))
			goto out;
		code |= 0x20;
		break;
	case 7:
		if (getbits(bits_struct, 6, &code))
			goto out;
		code |= 0x40;
		break;
	case 8:
		if (getbits(bits_struct, 7, &code))
			goto out;
		code |= 0x80;
		break;
	case 9:
		if (getbits(bits_struct, 8, &code))
			goto out;
		code |= 0x100;
		break;
	case 10:
		if (getbits(bits_struct, 9, &code))
			goto out;
		code |= 0x200;
		break;
	case 11:
		if (getbits(bits_struct, 10, &code))
			goto out;
		code |= 0x400;
		break;
	case 12:
		if (getbits(bits_struct, 11, &code))
			goto out;
		code |= 0x800;
		break;
	case 13:
		if (getbits(bits_struct, 12, &code))
			goto out;
		code |= 0x1000;
		break;
	case 14:
		if (getbits(bits_struct, 13, &code))
			goto out;
		code |= 0x2000;
		break;
	case 15:
		if (getbits(bits_struct, 1, &bitval))
			goto out;
		if (!bitval) {
			if (getbits(bits_struct, 14, &code))
				goto out;
			code |= 0x4000;
			break;						
		}
			
		if (getbits(bits_struct, 1, &bitval))
			goto out;
		if (!bitval) {
			if (getbits(bits_struct, 15, &code))
				goto out;
			code |= 0x8000;
			break;						
		}				
		goto out;				
	}		
	ret = 0;
	*ret_code = code;
out:
	return ret;
}
	
static DWORD __wcg_decompress(BYTE *uncompr, DWORD uncomprlen, 
		BYTE *compr, DWORD comprlen, WORD *code_table,
		WORD code_number, unsigned int offset)
{
	struct bits bits_struct;
	DWORD pos = offset;
	DWORD act_uncomprlen = 0;	
	
	bits_init(&bits_struct, compr, comprlen);

	while (1) {
		unsigned int bitval;
		unsigned int code;
		WORD *__uncompr;
		
		if (getbits(&bits_struct, 3, &bitval))
			break;

		if (bitval) { /* 非重复位数据 */
			if (getcode(&bits_struct, bitval, &code))
				break;
		
			if (act_uncomprlen + 1 >= uncomprlen)
				break;
			
			__uncompr = (WORD *)&uncompr[pos * 2];	
			*__uncompr = code_table[code];
			pos += 2;
			act_uncomprlen += 2;
		} else {	 /* 重复位数据 */
			unsigned int count, code_len;
			unsigned int i;

			if (getbits(&bits_struct, 4, &count))
				break;

			if (getbits(&bits_struct, 3, &code_len))
				break;

			if (getcode(&bits_struct, code_len, &code))
				break;
			count += 2;

			for (i = 0; i < count; i++) {		
				if (act_uncomprlen + 1 >= uncomprlen)
					break;
				__uncompr = (WORD *)&uncompr[pos * 2];	
				*__uncompr = code_table[code];
				pos += 2;
				act_uncomprlen += 2;
			}
			if (i != count)
				break;
		}
	}

	//if (bits_struct.curbyte != comprlen)
	//	fprintf(stderr, "compression miss-match %x(%d bits) VS %x\n", bits_struct.curbyte, bits_struct.req_bits, comprlen);
	return act_uncomprlen;		
}

static DWORD __wcg_decompress2(BYTE *uncompr, DWORD uncomprlen, 
		BYTE *compr, DWORD comprlen, WORD *code_table,
		WORD code_number, unsigned int offset)
{
	struct bits bits_struct;
	DWORD pos = offset;
	DWORD act_uncomprlen = 0;	
	
	bits_init(&bits_struct, compr, comprlen);

	while (1) {
		unsigned int bitval;
		unsigned int code;
		WORD *__uncompr;
		
		if (getbits(&bits_struct, 4, &bitval))
			break;

		if (bitval) { /* 非重复位数据 */
			if (getcode2(&bits_struct, bitval, &code))
				break;
		
			if (act_uncomprlen + 1 >= uncomprlen)
				break;
			
			__uncompr = (WORD *)&uncompr[pos * 2];	
			*__uncompr = code_table[code];
			pos += 2;
			act_uncomprlen += 2;
		} else {	 /* 重复位数据 */
			unsigned int count, code_len;
			unsigned int i;

			if (getbits(&bits_struct, 4, &count))
				break;

			if (getbits(&bits_struct, 4, &code_len))
				break;

			if (getcode2(&bits_struct, code_len, &code))
				break;
			count += 2;

			for (i = 0; i < count; i++) {		
				if (act_uncomprlen + 1 >= uncomprlen)
					break;
				__uncompr = (WORD *)&uncompr[pos * 2];	
				*__uncompr = code_table[code];
				pos += 2;
				act_uncomprlen += 2;
			}
			if (i != count)
				break;
		}
	}

	//if (bits_struct.curbyte != comprlen)
	//	fprintf(stderr, "compression miss-match %x(%d bits) VS %x\n", bits_struct.curbyte, bits_struct.req_bits, comprlen);
	return act_uncomprlen;		
}

static DWORD wcg_decompress(BYTE *uncompr, DWORD uncomprlen, 
		BYTE *compr, DWORD comprlen, WORD *code_table,
		WORD code_number, unsigned int offset)
{
	if (code_number <= 4096)
		return __wcg_decompress(uncompr, uncomprlen, compr, comprlen, code_table, code_number, offset);
	else
		return __wcg_decompress2(uncompr, uncomprlen, compr, comprlen, code_table, code_number, offset);	
}

static DWORD __lim2_decompress(BYTE *uncompr, DWORD uncomprlen, 
		BYTE *compr, DWORD comprlen, WORD *code_table, WORD code_number)
{
	struct bits bits_struct;
	DWORD pos = 0;
	DWORD act_uncomprlen = 0;	
	
	bits_init(&bits_struct, compr, comprlen);

	while (1) {
		unsigned int bitval;
		unsigned int code;
		
		if (getbits(&bits_struct, 3, &bitval))
			break;

		if (bitval) { /* 非重复位数据 */
			if (getcode(&bits_struct, bitval, &code))
				break;
		
			if (act_uncomprlen + 1 >= uncomprlen)
				break;
			
			((u16 *)uncompr)[pos++] = code_table[code];
			act_uncomprlen += 2;
		} else {	 /* 重复位数据 */
			unsigned int count, code_len;
			unsigned int i;

			if (getbits(&bits_struct, 4, &count))
				break;

			if (getbits(&bits_struct, 3, &code_len))
				break;

			if (getcode(&bits_struct, code_len, &code))
				break;
			count += 2;

			for (i = 0; i < count; i++) {		
				if (act_uncomprlen + 1 >= uncomprlen)
					break;
				((u16 *)uncompr)[pos++] = code_table[code];
				act_uncomprlen += 2;
			}
			if (i != count)
				break;
		}
	}

	//if (bits_struct.curbyte != comprlen)
	//	fprintf(stderr, "compression miss-match %d VS %x\n", bits_struct.curbyte, comprlen);
	return act_uncomprlen;
}

static DWORD __lim2_decompress2(BYTE *uncompr, DWORD uncomprlen, 
		BYTE *compr, DWORD comprlen, WORD *code_table, WORD code_number)
{
	struct bits bits_struct;
	DWORD pos = 0;
	DWORD act_uncomprlen = 0;	
	
	bits_init(&bits_struct, compr, comprlen);

	while (1) {
		unsigned int bitval;
		unsigned int code;
		
		if (getbits(&bits_struct, 4, &bitval))
			break;

		if (bitval) { /* 非重复位数据 */
			if (getcode2(&bits_struct, bitval, &code))
				break;
		
			if (act_uncomprlen + 1 >= uncomprlen)
				break;
			
			((u16 *)uncompr)[pos++] = code_table[code];
			act_uncomprlen += 2;
		} else {	 /* 重复位数据 */
			unsigned int count, code_len;
			unsigned int i;

			if (getbits(&bits_struct, 4, &count))
				break;

			if (getbits(&bits_struct, 4, &code_len))
				break;

			if (getcode2(&bits_struct, code_len, &code))
				break;
			count += 2;

			for (i = 0; i < count; i++) {		
				if (act_uncomprlen + 1 >= uncomprlen)
					break;
				((u16 *)uncompr)[pos++] = code_table[code];
				act_uncomprlen += 2;
			}
			if (i != count)
				break;
		}
	}

	//if (bits_struct.curbyte != comprlen)
	//	fprintf(stderr, "[2]compression miss-match %d VS %x\n", bits_struct.curbyte, comprlen);
	return act_uncomprlen;
}

static DWORD __lim3_decompress(BYTE *uncompr, DWORD uncomprlen, 
		BYTE *compr, DWORD comprlen, BYTE *code_table,
		WORD code_number, unsigned int offset)
{
	struct bits bits_struct;
	DWORD pos = offset;
	DWORD act_uncomprlen = 0;	
	
	bits_init(&bits_struct, compr, comprlen);

	while (1) {
		unsigned int bitval;
		unsigned int code;
		
		if (getbits(&bits_struct, 3, &bitval))
			break;

		if (bitval) { /* 非重复位数据 */
			if (getcode(&bits_struct, bitval, &code))
				break;
		
			if (act_uncomprlen + 1 >= uncomprlen)
				break;
			
			uncompr[pos] = code_table[code];
			pos += 4;
			act_uncomprlen++;
		} else {	 /* 重复位数据 */
			unsigned int count, code_len;
			unsigned int i;

			if (getbits(&bits_struct, 4, &count))
				break;

			if (getbits(&bits_struct, 3, &code_len))
				break;

			if (getcode(&bits_struct, code_len, &code))
				break;
			count += 2;

			for (i = 0; i < count; i++) {		
				if (act_uncomprlen >= uncomprlen)
					break;
				uncompr[pos] = code_table[code];
				pos += 4;
				act_uncomprlen++;
			}
			if (i != count)
				break;
		}
	}

	//if (bits_struct.curbyte != comprlen)
	//	fprintf(stderr, "compression miss-match %d VS %x\n", bits_struct.curbyte, comprlen);
	return act_uncomprlen;		
}

static DWORD lim2_decompress(BYTE *uncompr, DWORD uncomprlen, 
		BYTE *compr, DWORD comprlen, WORD *code_table, WORD code_number)
{
	if (code_number <= 4096)
		return __lim2_decompress(uncompr, uncomprlen, compr, comprlen, code_table, code_number);
	else
		return __lim2_decompress2(uncompr, uncomprlen, compr, comprlen, code_table, code_number);	
}

/********************* xfl *********************/

static int LiosGame_xfl_match(struct package *pkg)
{
	s8 magic[2];
	u16 version;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, 2)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(magic, "LB", 2)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->pio->read(pkg, &version, 2)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (version != 1) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int LiosGame_xfl_extract_directory(struct package *pkg,
										  struct package_directory *pkg_dir)
{		
	xfl_entry_t *index_buffer;
	u32 index_length;		
	u32 index_entries;

	if (pkg->pio->read(pkg, &index_length, 4))
		return -CUI_EREAD;

	if (pkg->pio->read(pkg, &index_entries, 4))
		return -CUI_EREAD;

	index_buffer = (xfl_entry_t *)malloc(index_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->index_entries = index_entries;
	pkg_dir->index_entry_length = sizeof(xfl_entry_t);
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_length;
	/* ぼーん ふりーくす！体验版:
	9045.wcg: 无效的资源文件(0)
	grpo_dn.xfl：提取第416个时失败
	*/
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	package_set_private(pkg, (void *)index_length);

	return 0;
}

static int LiosGame_xfl_parse_resource_info(struct package *pkg,
											struct package_resource *pkg_res)
{
	u32 index_length;
	xfl_entry_t *xfl_entry;

	index_length = (u32)package_get_private(pkg);
	xfl_entry = (xfl_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, xfl_entry->name);
	pkg_res->name_length = 32;
	pkg_res->raw_data_length = xfl_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = xfl_entry->offset + index_length + sizeof(xfl_header_t);

	return 0;
}

static int LiosGame_xfl_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	pkg_res->raw_data = malloc(pkg_res->raw_data_length);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EREADVEC;
	}

	char *ext = PathFindExtensionA(pkg_res->name);
	if (ext && !strcmp(ext, ".msk")) {
		pkg_res->replace_extension = _T(".msk.bmp");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
	}

	return 0;
}

static int LiosGame_xfl_save_resource(struct resource *res, 
									  struct package_resource *pkg_res)
{
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}
	res->rio->close(res);
	
	return 0;
}

static void LiosGame_xfl_release_resource(struct package *pkg, 
										  struct package_resource *pkg_res)
{
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void LiosGame_xfl_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}
	package_set_private(pkg, NULL);
	pkg->pio->close(pkg);
}

static cui_ext_operation LiosGame_xfl_operation = {
	LiosGame_xfl_match,					/* match */
	LiosGame_xfl_extract_directory,		/* extract_directory */
	LiosGame_xfl_parse_resource_info,	/* parse_resource_info */
	LiosGame_xfl_extract_resource,		/* extract_resource */
	LiosGame_xfl_save_resource,			/* save_resource */
	LiosGame_xfl_release_resource,		/* release_resource */
	LiosGame_xfl_release				/* release */
};

/********************* lwg *********************/

static int LiosGame_lwg_match(struct package *pkg)
{
	lwg_header_t *lwg_header;

	lwg_header = (lwg_header_t *)malloc(sizeof(*lwg_header));
	if (!lwg_header)
		return -CUI_EMEM;

	if (pkg->pio->open(pkg, IO_READONLY)) {
		free(lwg_header);
		return -CUI_EOPEN;
	}

	if (pkg->pio->read(pkg, lwg_header, sizeof(*lwg_header))) {
		pkg->pio->close(pkg);
		free(lwg_header);
		return -CUI_EREAD;
	}

	if (memcmp(lwg_header->magic, "LG", 2)) {
		pkg->pio->close(pkg);
		free(lwg_header);
		return -CUI_EMATCH;
	}

	if (lwg_header->version != 1) {
		pkg->pio->close(pkg);
		free(lwg_header);
		return -CUI_EMATCH;
	}
	package_set_private(pkg, lwg_header);

	return 0;	
}

static int LiosGame_lwg_extract_directory(struct package *pkg,
										  struct package_directory *pkg_dir)
{		
	lwg_header_t *lwg_header;
	BYTE *index_buffer;
	u32 data_length;

	lwg_header = (lwg_header_t *)package_get_private(pkg);
	index_buffer = (BYTE *)malloc(lwg_header->index_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, lwg_header->index_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	if (pkg->pio->read(pkg, &data_length, 4)) {
		free(index_buffer);
		return -CUI_EREAD;
	}
	if (!data_length) {
		free(index_buffer);
		return -CUI_EPARA;
	}
	pkg_dir->index_entries = lwg_header->entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = lwg_header->index_length;
	pkg_dir->flags = PKG_DIR_FLAG_VARLEN | PKG_DIR_FLAG_SKIP0;

	return 0;
}

static int LiosGame_lwg_parse_resource_info(struct package *pkg,
											struct package_resource *pkg_res)
{
	lwg_header_t *lwg_header;
	BYTE *xfl_entry;
	u32 x_pos, y_pos;	/* 该图像在整个画布中的位置（以左上上角为原点） */
	u8 unknown;	/* always 0x28 */

	lwg_header = (lwg_header_t *)package_get_private(pkg);
	xfl_entry = (BYTE *)pkg_res->actual_index_entry;
	x_pos = *(u32 *)xfl_entry;
	xfl_entry += 4;
	y_pos = *(u32 *)xfl_entry;
	xfl_entry += 4;
	unknown = *(u32 *)xfl_entry++;
	pkg_res->offset = *(u32 *)xfl_entry + sizeof(lwg_header_t) + lwg_header->index_length + 4;	/* plus data_length field */
	xfl_entry += 4;
	pkg_res->raw_data_length = *(u32 *)xfl_entry;
	xfl_entry += 4;
	pkg_res->name_length = *xfl_entry++;
	strncpy(pkg_res->name, (char *)xfl_entry, pkg_res->name_length);
	pkg_res->name[pkg_res->name_length] = 0;
	pkg_res->actual_index_entry_length = 17 + 1 + pkg_res->name_length;
	pkg_res->actual_data_length = 0;
	//if (unknown != 0x28)
		//printf("unkown field %x\n", unknown);
	return 0;
}

static int LiosGame_lwg_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	pkg_res->raw_data = malloc(pkg_res->raw_data_length);
	if (!pkg_res->raw_data)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, pkg_res->raw_data, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(pkg_res->raw_data);
			pkg_res->raw_data = NULL;
			return -CUI_EREADVEC;
	}

	if (!memcmp(pkg_res->raw_data, "WG", 2)) {
		//wcg_header_t *wcg_header = (wcg_header_t *)pkg_res->raw_data;
		pkg_res->replace_extension = _T(".wcg");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
	}

	return 0;
}

static int LiosGame_lwg_save_resource(struct resource *res, 
									  struct package_resource *pkg_res)
{
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (res->rio->write(res, pkg_res->raw_data, pkg_res->raw_data_length)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}
	res->rio->close(res);
	
	return 0;
}

static void LiosGame_lwg_release_resource(struct package *pkg, 
										  struct package_resource *pkg_res)
{
	if (pkg_res->raw_data) {
		free(pkg_res->raw_data);
		pkg_res->raw_data = NULL;
	}
}

static void LiosGame_lwg_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	lwg_header_t *lwg_header;

	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	lwg_header = (lwg_header_t *)package_get_private(pkg);
	if (lwg_header) {
		free(lwg_header);
		package_set_private(pkg, NULL);
	}
	pkg->pio->close(pkg);
}

static cui_ext_operation LiosGame_lwg_operation = {
	LiosGame_lwg_match,					/* match */
	LiosGame_lwg_extract_directory,		/* extract_directory */
	LiosGame_lwg_parse_resource_info,	/* parse_resource_info */
	LiosGame_lwg_extract_resource,		/* extract_resource */
	LiosGame_lwg_save_resource,			/* save_resource */
	LiosGame_lwg_release_resource,		/* release_resource */
	LiosGame_lwg_release				/* release */
};

/********************* wcg *********************/

static int LiosGame_wcg_match(struct package *pkg)
{
	wcg_header_t *wcg_header;

	wcg_header = (wcg_header_t *)malloc(sizeof(*wcg_header));
	if (!wcg_header)
		return -CUI_EMEM;

	if (pkg->pio->open(pkg, IO_READONLY)) {
		free(wcg_header);
		return -CUI_EOPEN;
	}

	if (pkg->pio->read(pkg, wcg_header, sizeof(*wcg_header))) {
		pkg->pio->close(pkg);
		free(wcg_header);
		return -CUI_EREAD;
	}

	if (memcmp(wcg_header->magic, "WG", 2)) {
		pkg->pio->close(pkg);
		free(wcg_header);
		return -CUI_EMATCH;
	}

	if ((wcg_header->flags & 0xf) != 1) {
		pkg->pio->close(pkg);
		free(wcg_header);
		fprintf(stderr, "unsupported flags %x\n", wcg_header->flags);
		return -CUI_EMATCH;		
	}
	if ((wcg_header->flags & 0x1c0) != 0x40) {
		pkg->pio->close(pkg);
		free(wcg_header);
		fprintf(stderr, "unsupported flag %x\n", wcg_header->flags);
		return -CUI_EMATCH;		
	}
	package_set_private(pkg, wcg_header);

	return 0;	
}

static int LiosGame_wcg_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	wcg_header_t *wcg_header;
	wcg_info_t wcg_info;
	unsigned int act_uncomprlen;
	BYTE *compr, *uncompr;
	u16 *code_table;
	long read_position;
	u32 wcg_size;
	
	if (pkg->pio->length_of(pkg, &wcg_size)) 
		return -CUI_ELEN;

	wcg_header = (wcg_header_t *)package_get_private(pkg);

	read_position = sizeof(*wcg_header);
	if (pkg->pio->readvec(pkg, &wcg_info, sizeof(wcg_info),	read_position, IO_SEEK_SET))
		return -CUI_EREADVEC;


	code_table = (u16 *)malloc(wcg_info.code_number * 2);
	if (!code_table)
		return -CUI_EMEM;

	read_position += sizeof(wcg_info);
	if (pkg->pio->readvec(pkg, code_table, wcg_info.code_number * 2, read_position, IO_SEEK_SET)) {
		free(code_table);
		return -CUI_EREADVEC;
	}	

	compr = (BYTE *)malloc(wcg_info.comprlen);
	if (!compr) {
		free(code_table);
		return -CUI_EMEM;
	}

	read_position += wcg_info.code_number * 2;
	if (pkg->pio->readvec(pkg, compr, wcg_info.comprlen, read_position, IO_SEEK_SET)) {			
		free(compr);
		free(code_table);
		return -CUI_EREADVEC;
	}

	uncompr = (BYTE *)malloc(wcg_info.uncomprlen * 2);
	if (!uncompr) {
		free(compr);
		free(code_table);
		return -CUI_EMEM;
	}
	memset(uncompr, 0, wcg_info.uncomprlen * 2);

	act_uncomprlen = wcg_decompress(uncompr, wcg_info.uncomprlen,
		compr, wcg_info.comprlen, code_table, wcg_info.code_number, 1);
	free(compr);
	free(code_table);
	if (act_uncomprlen != wcg_info.uncomprlen) {
		fprintf(stderr, "wcg (step 0) decompress miss-match "
				"(%d VS %d)\n", act_uncomprlen, wcg_info.uncomprlen);
		//free(uncompr);	
		//return -CUI_EUNCOMPR;	
	}

	read_position += wcg_info.comprlen;
	if (pkg->pio->readvec(pkg, &wcg_info, sizeof(wcg_info),	read_position, IO_SEEK_SET)) {
		free(uncompr);	
		return -CUI_EREADVEC;
	}

	code_table = (u16 *)malloc(wcg_info.code_number * 2);
	if (!code_table) {
		free(uncompr);
		return -CUI_EMEM;
	}
	
	read_position += sizeof(wcg_info);
	if (pkg->pio->readvec(pkg, code_table, wcg_info.code_number * 2, read_position, IO_SEEK_SET)) {
		free(code_table);
		free(uncompr);
		return -CUI_EREADVEC;
	}	

	compr = (BYTE *)malloc(wcg_info.comprlen);
	if (!compr) {
		free(code_table);
		free(uncompr);
		return -CUI_EMEM;
	}

	read_position += wcg_info.code_number * 2;
	if (pkg->pio->readvec(pkg, compr, wcg_info.comprlen, read_position, IO_SEEK_SET)) {			
		free(compr);
		free(code_table);
		return -CUI_EREADVEC;
	}

	act_uncomprlen = wcg_decompress(uncompr, wcg_info.uncomprlen,
		compr, wcg_info.comprlen, code_table, wcg_info.code_number, 0);
	free(compr);
	free(code_table);
	if (act_uncomprlen != wcg_info.uncomprlen) {
		fprintf(stderr, "wcg (step 1) decompress miss-match "
			"(%d VS %d)\n", act_uncomprlen, wcg_info.uncomprlen);
		//free(uncompr);	
		//return -CUI_EUNCOMPR;	
	}

//printf("%x\n", wcg_info.unknown);

	alpha_blending_reverse(uncompr, wcg_header->width, wcg_header->height, 32);

	if (MyBuildBMPFile(uncompr, act_uncomprlen * 2, NULL, 0, wcg_header->width, 0 - wcg_header->height, 32,
			(BYTE **)&pkg_res->actual_data, (DWORD *)&pkg_res->actual_data_length, malloc_callback)) {
		free(uncompr);	
		return -CUI_EMEM;		
	}

	free(uncompr);	
	pkg_res->raw_data = NULL;
	pkg_res->raw_data_length = wcg_size;

	return 0;
}

static int LiosGame_wcg_save_resource(struct resource *res, 
									  struct package_resource *pkg_res)
{
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (pkg_res->actual_data && pkg_res->actual_data_length) {
		if (res->rio->write(res, pkg_res->actual_data, pkg_res->actual_data_length)) {
			res->rio->close(res);
			return -CUI_EWRITE;
		}
	}
	res->rio->close(res);
	
	return 0;
}

static void LiosGame_wcg_release_resource(struct package *pkg, 
										  struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
}

static void LiosGame_wcg_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	wcg_header_t *wcg_header;

	wcg_header = (wcg_header_t *)package_get_private(pkg);
	if (wcg_header) {
		free(wcg_header);
		package_set_private(pkg, NULL);
	}
	pkg->pio->close(pkg);
}

static cui_ext_operation LiosGame_wcg_operation = {
	LiosGame_wcg_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	LiosGame_wcg_extract_resource,	/* extract_resource */
	LiosGame_wcg_save_resource,		/* save_resource */
	LiosGame_wcg_release_resource,	/* release_resource */
	LiosGame_wcg_release			/* release */
};

/********************* lim *********************/

static int LiosGame_lim_match(struct package *pkg)
{
	lim_header_t *lim_header;
	u8 flag;

	lim_header = (lim_header_t *)malloc(sizeof(*lim_header));
	if (!lim_header)
		return -CUI_EMEM;

	if (pkg->pio->open(pkg, IO_READONLY)) {
		free(lim_header);
		return -CUI_EOPEN;
	}

	if (pkg->pio->read(pkg, lim_header, sizeof(*lim_header))) {
		pkg->pio->close(pkg);
		free(lim_header);
		return -CUI_EREAD;
	}

	if (memcmp(lim_header->magic, "LM", 2)) {
		pkg->pio->close(pkg);
		free(lim_header);
		return -CUI_EMATCH;
	}

	flag = lim_header->flag0 & 0x0f;
	if (flag != 2 && flag != 3) {
		pkg->pio->close(pkg);
		free(lim_header);
		return -CUI_EMATCH;	
	}
	package_set_private(pkg, lim_header);

	return 0;	
}

static int LiosGame_lim2_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	lim_header_t *lim_header;
	lim_info_t lim_info;
	DWORD act_uncomprlen;
	BYTE *compr, *uncompr;
	u16 *code_table;
	long read_position;

	lim_header = (lim_header_t *)package_get_private(pkg);
	
	read_position = sizeof(*lim_header);

	if (pkg->pio->readvec(pkg, &lim_info, sizeof(lim_info),	read_position, IO_SEEK_SET))
		return -CUI_EREADVEC;
		
	code_table = (u16 *)malloc(lim_info.code_number * 2);
	if (!code_table)
		return -CUI_EMEM;

	read_position += sizeof(lim_info);
	if (pkg->pio->readvec(pkg, code_table, lim_info.code_number * 2, read_position, IO_SEEK_SET)) {
		free(code_table);
		return -CUI_EREADVEC;
	}	

	compr = (BYTE *)malloc(lim_info.comprlen);
	if (!compr) {
		free(code_table);
		return -CUI_EMEM;
	}

	read_position += lim_info.code_number * 2;
	if (pkg->pio->readvec(pkg, compr, lim_info.comprlen, read_position, IO_SEEK_SET)) {			
		free(compr);
		free(code_table);
		return -CUI_EREADVEC;
	}

	act_uncomprlen = lim_info.uncomprlen;
	uncompr = (BYTE *)malloc(act_uncomprlen);
	if (!uncompr) {
		free(compr);
		free(code_table);
		return -CUI_EMEM;
	}
	memset(uncompr, 0, act_uncomprlen);
		
	act_uncomprlen = lim2_decompress(uncompr, lim_info.uncomprlen,
		compr, lim_info.comprlen, code_table, lim_info.code_number);
	free(compr);
	free(code_table);
	// FXIME：某个游戏的图4357.lim（decompress error occured (716768 VS 716800)）
	if (act_uncomprlen != lim_info.uncomprlen) {
		fprintf(stderr, "WARNNING: lim2 decompress miss-match "
				"(%d VS %d)\n", act_uncomprlen, lim_info.uncomprlen);
	//	free(uncompr);	
	//	return -CUI_EUNCOMPR;	
	}

//printf("%x\n", wcg_info.unknown);
	// BGR565(程序中将其转化为bmp 32bit，alpha的设定为：如果B=0且R=0且G=0x3f(全绿)，alpha=0xff；否则alpha=0
	if (MyBuildBMP16File(uncompr, act_uncomprlen, lim_header->width, 0 - lim_header->height, 
			(BYTE **)&pkg_res->actual_data, (DWORD *)&pkg_res->actual_data_length, RGB565, NULL, malloc_callback)) {
		free(uncompr);	
		return -CUI_EMEM;		
	}
	free(uncompr);	

	return 0;
}

static int LiosGame_lim2_with_alpha_extract_resource(struct package *pkg,
													 struct package_resource *pkg_res)
{
	lim_header_t *lim_header;
	lim_info_t lim_info;
	DWORD act_uncomprlen;
	BYTE *compr, *uncompr = NULL;
	u8 *code_table;
	BYTE *bmp16_dib;
	u32 read_position;
	int err;

	if (pkg->pio->locate(pkg, &read_position))
		return -CUI_ELOC;
	lim_header = (lim_header_t *)package_get_private(pkg);
	for (unsigned int p = 0; p < 1; p++) {
		if (pkg->pio->readvec(pkg, &lim_info, sizeof(lim_info),	read_position, IO_SEEK_SET)) {
			err = -CUI_EREADVEC;
			break;
		}
		
		code_table = (u8 *)malloc(lim_info.code_number * 1);
		if (!code_table) {
			err = -CUI_EMEM;
			break;
		}

		read_position += sizeof(lim_info);
		if (pkg->pio->readvec(pkg, code_table, lim_info.code_number * 1, read_position, IO_SEEK_SET)) {
			free(code_table);
			err = -CUI_EREADVEC;
			break;		
		}	

		compr = (BYTE *)malloc(lim_info.comprlen);
		if (!compr) {
			free(code_table);
			err = -CUI_EMEM;
			break;			
		}

		read_position += lim_info.code_number * 1;
		if (pkg->pio->readvec(pkg, compr, lim_info.comprlen, read_position, IO_SEEK_SET)) {			
			free(compr);
			free(code_table);
			err = -CUI_EREADVEC;
			break;			
		}

		if (!uncompr) {
			act_uncomprlen = lim_info.uncomprlen * 4;
			uncompr = (BYTE *)malloc(act_uncomprlen);
			if (!uncompr) {
				free(compr);
				free(code_table);
				err = -CUI_EMEM;
				break;
			}
			memset(uncompr, 0, act_uncomprlen);
		}
		
		act_uncomprlen = __lim3_decompress(uncompr, lim_info.uncomprlen,
			compr, lim_info.comprlen, code_table, lim_info.code_number, 3 - p);
		free(compr);
		free(code_table);
		// FIXME: CG_203.LIM、4157.lim解压返回长度不匹配
		if (act_uncomprlen != lim_info.uncomprlen) {
			fprintf(stderr, "WARNNING: lim2 (step %d) with alpha decompress miss-match "
					"(%d VS %d)\n", p, act_uncomprlen, lim_info.uncomprlen);
			//free(uncompr);	
			//return -CUI_EUNCOMPR;	
		}

		read_position += lim_info.comprlen;
	}

//printf("%x\n", wcg_info.unknown);

	bmp16_dib = (BYTE *)pkg_res->actual_data + 0x46;
	DWORD line_len = (lim_header->width * 2 + 3) & ~3;
	DWORD *bmp32_dib = (DWORD *)uncompr;
	for (unsigned int y = 0; y < lim_header->height; y++) {
		WORD *line = (WORD *)&bmp16_dib[line_len * y];
		for (unsigned int x = 0; x < lim_header->width; x++) {
			u8 b, g, r;

			b = (line[x] & 0x1f) << 3;
			g = ((line[x] >> 5) & 0x3f) << 2;
			r = ((line[x] >> 11) & 0x1f) << 3;
			*bmp32_dib = b | (g << 8) | (r << 16) | (*bmp32_dib & 0xff000000);
			bmp32_dib++;
		}
	}
	free(pkg_res->actual_data);

	alpha_blending_reverse(uncompr, lim_header->width, lim_header->height, 32);

	if (MyBuildBMPFile(uncompr, act_uncomprlen, NULL, 0, lim_header->width, 0 - lim_header->height, 32,
			(BYTE **)&pkg_res->actual_data, (DWORD *)&pkg_res->actual_data_length, malloc_callback)) {
		free(uncompr);	
		return -CUI_EMEM;		
	}
	free(uncompr);	

	return 0;
}

static int LiosGame_lim3_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	lim_header_t *lim_header;
	lim_info_t lim_info;
	DWORD act_uncomprlen;
	BYTE *compr, *uncompr = NULL;
	u8 *code_table;
	unsigned long read_position;
	unsigned int bpp;
	int err;

	lim_header = (lim_header_t *)package_get_private(pkg);
	read_position = sizeof(*lim_header);
	bpp = lim_header->flag0 & 0x10 ? 4 : 1;
	for (unsigned int p = 0; p < bpp; p++) {
		if (pkg->pio->readvec(pkg, &lim_info, sizeof(lim_info),	read_position, IO_SEEK_SET)) {
			err = -CUI_EREADVEC;
			break;
		}
		
		code_table = (u8 *)malloc(lim_info.code_number * 1);
		if (!code_table) {
			err = -CUI_EMEM;
			break;
		}

		read_position += sizeof(lim_info);
		if (pkg->pio->readvec(pkg, code_table, lim_info.code_number * 1, read_position, IO_SEEK_SET)) {
			free(code_table);
			err = -CUI_EREADVEC;
			break;		
		}	

		compr = (BYTE *)malloc(lim_info.comprlen);
		if (!compr) {
			free(code_table);
			err = -CUI_EMEM;
			break;			
		}

		read_position += lim_info.code_number * 1;
		if (pkg->pio->readvec(pkg, compr, lim_info.comprlen, read_position, IO_SEEK_SET)) {			
			free(compr);
			free(code_table);
			err = -CUI_EREADVEC;
			break;			
		}

		if (!uncompr) {
			act_uncomprlen = lim_info.uncomprlen * 4;
			uncompr = (BYTE *)malloc(act_uncomprlen);
			if (!uncompr) {
				free(compr);
				free(code_table);
				err = -CUI_EMEM;
				break;
			}
			memset(uncompr, 0, act_uncomprlen);
		}
		
		act_uncomprlen = __lim3_decompress(uncompr, lim_info.uncomprlen,
			compr, lim_info.comprlen, code_table, lim_info.code_number, 3 - p);
		free(compr);
		free(code_table);
		if (act_uncomprlen != lim_info.uncomprlen) {
			fprintf(stderr, "lim3 (step %d) decompress miss-match "
					"(%d VS %d)\n", p, act_uncomprlen, lim_info.uncomprlen);
			//free(uncompr);	
			//return -CUI_EUNCOMPR;	
		}

		read_position += lim_info.comprlen;
	}

//printf("%x\n", wcg_info.unknown);
	alpha_blending_reverse(uncompr, lim_header->width, lim_header->height, 32);

	if (MyBuildBMPFile(uncompr, act_uncomprlen, NULL, 0, lim_header->width, 0 - lim_header->height, 32,
			(BYTE **)&pkg_res->actual_data, (DWORD *)&pkg_res->actual_data_length, malloc_callback)) {
		free(uncompr);	
		return -CUI_EMEM;		
	}
	free(uncompr);	

	return 0;
}

static int LiosGame_lim_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	lim_header_t *lim_header;
	u8 flag;
	u32 lim_size;
	int err = -CUI_EMATCH;
	
	if (pkg->pio->length_of(pkg, &lim_size)) 
		return -CUI_ELEN;

	lim_header = (lim_header_t *)package_get_private(pkg);
	flag = lim_header->flag0 & 0x0f;
	if (flag == 2) {
		if (lim_header->flag0 & 0x10) {
			err = LiosGame_lim2_extract_resource(pkg, pkg_res);	// 432d50
			if (err)
				return err;
		}		
		if (lim_header->flag1 & 1) {
			err = LiosGame_lim2_with_alpha_extract_resource(pkg, pkg_res);
			if (err)
				return err;
		}
	} else if (flag == 3)
		err = LiosGame_lim3_extract_resource(pkg, pkg_res);

	pkg_res->raw_data = NULL;
	pkg_res->raw_data_length = lim_size;
	
	return err;
}

static int LiosGame_lim_save_resource(struct resource *res, 
									  struct package_resource *pkg_res)
{
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (pkg_res->actual_data && pkg_res->actual_data_length) {
		if (res->rio->write(res, pkg_res->actual_data, pkg_res->actual_data_length)) {
			res->rio->close(res);
			return -CUI_EWRITE;
		}
	}
	res->rio->close(res);
	
	return 0;
}

static void LiosGame_lim_release_resource(struct package *pkg, 
										  struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
}

static void LiosGame_lim_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	lim_header_t *lim_header;

	lim_header = (lim_header_t *)package_get_private(pkg);
	if (lim_header) {
		free(lim_header);
		package_set_private(pkg, NULL);
	}
	pkg->pio->close(pkg);
}

static cui_ext_operation LiosGame_lim_operation = {
	LiosGame_lim_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	LiosGame_lim_extract_resource,	/* extract_resource */
	LiosGame_lim_save_resource,		/* save_resource */
	LiosGame_lim_release_resource,	/* release_resource */
	LiosGame_lim_release			/* release */
};
	
int CALLBACK LiosGame_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".xfl"), NULL, 
		NULL, &LiosGame_xfl_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES |CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".lwg"), NULL, 
		NULL, &LiosGame_lwg_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".wcg"), _T(".bmp"), 
		NULL, &LiosGame_wcg_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".lim"), _T(".bmp"), 
		NULL, &LiosGame_lim_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;
	
	return 0;
}
