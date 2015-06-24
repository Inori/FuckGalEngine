#include <windows.h>
#include <tchar.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <resource.h>
#include <package.h>
#include <cui_error.h>
#include <utility.h>
#include <stdio.h>

struct acui_information hcsystem_cui_information = {
	_T("Ｈ℃"),				/* copyright */
	_T("hcsystem"),			/* system */
	_T(".pak op2"),			/* package */
	_T("1.1.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-5-20 9:29"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_UNSTABLE
};

#pragma pack (1)
typedef struct {
	s8 magic[4];		// "PACK"
	u32 index_entries;
	u32 swap_flag;
} pak_header_t;

typedef struct {
	s8 name[32];		
	u32 uncomprlen;
	u32 comprlen;
	u32 offset;
} pak_entry_t;

typedef struct {
	u16 name[32];		
	u32 uncomprlen;
	u32 comprlen;
	u32 offset;
} pak_new_entry_t;

typedef struct {
	s8 magic[4];		// "OPF "
	u32 width;
	u32 height;
	u32 color_depth;
	u32 line_length;
	u32 dib_offset;
	u32 dib_length;
	u32 reserved;
} opf_header_t;

typedef struct {
	s8 magic[4];		// "OPF2"
	u32 width;
	u32 height;
	u32 color_depth;
	u32 line_length;
	u32 dib_offset;
	u32 dib_length;
	u32 comprlen;
} opf2_header_t;
#pragma pack ()	

#define PKG_DIR_FLAG_SWAP		0x80000000
#define PKG_DIR_FLAG_NO_COMPR	0x40000000


static void *my_malloc(DWORD len)
{
	return malloc(len);
}

#if 0
static inline unsigned char getbit_le(unsigned char byte, unsigned int pos)
{
	return !!(byte & (1 << pos));
}

static unsigned int lzss_decompress(unsigned char *uncompr, unsigned int uncomprlen, 
							unsigned char *compr, unsigned int comprlen)
{
	unsigned int act_uncomprlen = 0;
	/* compr中的当前字节中的下一个扫描位的位置 */
	unsigned int curbit = 0;
	/* compr中的当前扫描字节 */
	unsigned int curbyte = 0;
	unsigned int nCurWindowByte = 0xfee;
	unsigned int win_size = 4096;
	BYTE win[4096];
	
	memset(win, 0, nCurWindowByte);
	while (1) {
		/* 如果为0, 表示接下来的1个字节原样输出 */
		BYTE flag;

		if (curbyte >= comprlen)
			break;

		flag = compr[curbyte++];
		for (curbit = 0; curbit < 8; curbit++) {
			if (getbit_le(flag, curbit)) {
				unsigned char data;
	
				if (curbyte >= comprlen)
					goto out;

				if (act_uncomprlen >= uncomprlen)
					goto out;

				data = compr[curbyte++];
				uncompr[act_uncomprlen++] = data;
				/* 输出的1字节放入滑动窗口 */
				win[nCurWindowByte++] = data;
				nCurWindowByte &= win_size - 1;
			} else {
				unsigned int copy_bytes, win_offset;
				unsigned int i;

				if (curbyte >= comprlen)
					goto out;

				win_offset = compr[curbyte++];

				if (curbyte >= comprlen)
					goto out;
				copy_bytes = compr[curbyte++];
				win_offset |= (copy_bytes >> 4) << 8;
				copy_bytes &= 0x0f;
				copy_bytes += 3;

				for (i = 0; i < copy_bytes; i++) {	
					unsigned char data;

					if (act_uncomprlen >= uncomprlen)
						goto out;

					data = win[(win_offset + i) & (win_size - 1)];
					uncompr[act_uncomprlen++] = data;		
					/* 输出的1字节放入滑动窗口 */
					win[nCurWindowByte++] = data;
					nCurWindowByte &= win_size - 1;	
				}
			}
		}
	}
out:
	return act_uncomprlen;
}
#endif

static void lzss_uncompress(BYTE *uncompr, DWORD uncomprlen, 
							 BYTE *compr, DWORD comprlen)
{
	unsigned int act_uncomprlen = 0;
	/* compr中的当前字节中的下一个扫描位的位置 */
	unsigned int curbit = 0;
	/* compr中的当前扫描字节 */
	unsigned int curbyte = 0;
	unsigned int nCurWindowByte = 0xfee;
	const unsigned int win_size = 4096;
	BYTE win[4096];
	WORD flag = 0;

	memset(win, 0, win_size);
	while (1) {
		flag >>= 1;
		if (!(flag & 0x0100))
			flag = compr[curbyte++] | 0xff00;

		if (flag & 1) {
			unsigned char data;

			data = compr[curbyte++];
			uncompr[act_uncomprlen++] = data;
			if (act_uncomprlen >= uncomprlen)
				return;
			win[nCurWindowByte++] = data;
			nCurWindowByte &= win_size - 1;
		} else {
			unsigned int copy_bytes, win_offset;
			unsigned int i;

			win_offset = compr[curbyte++];
			copy_bytes = compr[curbyte++];
			win_offset |= (copy_bytes & 0xf0) << 4;
			copy_bytes &= 0x0f;
			copy_bytes += 3;

			for (i = 0; i < copy_bytes; i++) {	
				unsigned char data;

				data = win[(win_offset + i) & (win_size - 1)];
				uncompr[act_uncomprlen++] = data;
				if (act_uncomprlen >= uncomprlen)
					return;
				win[nCurWindowByte++] = data;
				nCurWindowByte &= win_size - 1;	
			}
		}
	}
}

/********************* pak *********************/

static int hcsystem_pak_match(struct package *pkg)
{
	pak_header_t pak_header;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, &pak_header, sizeof(pak_header))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(pak_header.magic, "PACK", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (!pak_header.index_entries) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	pak_entry_t pak_entry;
	if (pkg->pio->read(pkg, &pak_entry, sizeof(pak_entry))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	BYTE *p = (BYTE *)&pak_entry;
	if (pak_header.swap_flag == 1) {
		for (unsigned int i = 0; i < sizeof(pak_entry); i++)
			p[i] = ((p[i] & 0xf0) >> 4) | ((p[i] & 0x0f) << 4);
	}

	if (pak_entry.offset == sizeof(pak_header_t) + pak_header.index_entries * sizeof(pak_entry_t)) {
		package_set_private(pkg, sizeof(pak_entry_t));
		return 0;
	}

	pak_new_entry_t pak_new_entry;
	if (pkg->pio->readvec(pkg, &pak_new_entry, sizeof(pak_new_entry_t), sizeof(pak_header_t), IO_SEEK_SET)) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	p = (BYTE *)&pak_new_entry;
	if (pak_header.swap_flag == 1) {
		for (unsigned int i = 0; i <  sizeof(pak_new_entry_t); i++)
			p[i] = ((p[i] & 0xf0) >> 4) | ((p[i] & 0x0f) << 4);
	}

	if (pak_new_entry.offset != sizeof(pak_header_t) + pak_header.index_entries * sizeof(pak_new_entry_t)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}
	package_set_private(pkg, sizeof(pak_new_entry_t));

	return 0;	
}

static int hcsystem_pak_extract_directory(struct package *pkg,
										  struct package_directory *pkg_dir)
{
	pak_header_t pak_header;

	if (pkg->pio->readvec(pkg, &pak_header, sizeof(pak_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD entry_size = package_get_private(pkg);
	DWORD index_buffer_length;
	BYTE *index_buffer;
	if (entry_size == sizeof(pak_new_entry_t)) {
		index_buffer_length = pak_header.index_entries * sizeof(pak_new_entry_t);
		index_buffer = new BYTE[index_buffer_length];
		if (!index_buffer)
			return -CUI_EMEM;
		pkg_dir->index_entry_length = sizeof(pak_new_entry_t);
	} else if (entry_size == sizeof(pak_entry_t)) {
		index_buffer_length = pak_header.index_entries * sizeof(pak_entry_t);
		index_buffer = new BYTE[index_buffer_length];
		if (!index_buffer)
			return -CUI_EMEM;
		pkg_dir->index_entry_length = sizeof(pak_entry_t);
	}

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		delete [] index_buffer;
		return -CUI_EREAD;	
	}

	if (pak_header.swap_flag == 1) {
		for (unsigned int i = 0; i < index_buffer_length; i++)
			index_buffer[i] = ((index_buffer[i] & 0xf0) >> 4) | ((index_buffer[i] & 0x0f) << 4);
	}

	pkg_dir->index_entries = pak_header.index_entries;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;	
	pkg_dir->flags = pak_header.swap_flag ? PKG_DIR_FLAG_SWAP : 0;
	
	return 0;
}

static int hcsystem_pak_parse_resource_info(struct package *pkg,
											struct package_resource *pkg_res)
{
	DWORD entry_size = package_get_private(pkg);
	if (entry_size == sizeof(pak_new_entry_t)) {
		pak_new_entry_t *pak_entry = (pak_new_entry_t *)pkg_res->actual_index_entry;
		wcscpy((WCHAR *)pkg_res->name, pak_entry->name);
		pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
		pkg_res->actual_data_length = pak_entry->uncomprlen;	/* 数据都是明文 */
		pkg_res->raw_data_length = pak_entry->comprlen ? pak_entry->comprlen : pak_entry->uncomprlen;
		pkg_res->offset = pak_entry->offset;
		pkg_res->flags = pak_entry->comprlen ? 0 : PKG_DIR_FLAG_NO_COMPR;
		pkg_res->flags |= PKG_RES_FLAG_UNICODE;
	} else if (entry_size == sizeof(pak_entry_t)) {
		pak_entry_t *pak_entry = (pak_entry_t *)pkg_res->actual_index_entry;
		strcpy(pkg_res->name, pak_entry->name);
		pkg_res->name_length = -1;			/* -1表示名称以NULL结尾 */
		pkg_res->actual_data_length = pak_entry->uncomprlen;	/* 数据都是明文 */
		pkg_res->raw_data_length = pak_entry->comprlen ? pak_entry->comprlen : pak_entry->uncomprlen;
		pkg_res->offset = pak_entry->offset;
		pkg_res->flags = pak_entry->comprlen ? 0 : PKG_DIR_FLAG_NO_COMPR;
	}

	return 0;
}

static int hcsystem_pak_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	unsigned char *compr, *uncompr;
	unsigned int comprlen, act_uncomprlen;

	comprlen = pkg_res->raw_data_length;
	compr = (unsigned char *)malloc(pkg_res->raw_data_length);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, pkg_res->raw_data_length,
		pkg_res->offset, IO_SEEK_SET)) {
			free(compr);
			return -CUI_EREADVEC;
	}

	if (pkg_res->flags & PKG_RES_FLAG_RAW) {
		pkg_res->raw_data = compr;
		return 0;
	}

	if (!(pkg_res->flags & PKG_DIR_FLAG_NO_COMPR)) {
		uncompr = (unsigned char *)malloc(pkg_res->actual_data_length);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}
		//act_uncomprlen = lzss_decompress(uncompr, pkg_res->actual_data_length,
		//	compr, pkg_res->raw_data_length);
		lzss_uncompress(uncompr, pkg_res->actual_data_length, compr, pkg_res->raw_data_length);
		act_uncomprlen = pkg_res->actual_data_length;
		free(compr);
		compr = NULL;
		comprlen = 0;
		//if (act_uncomprlen != pkg_res->actual_data_length) {
		//	free(uncompr);
		//	return -CUI_EUNCOMPR;
		//}
	} else {
		uncompr = NULL;
		act_uncomprlen = 0;	
	}

	pkg_res->raw_data = compr;
	pkg_res->raw_data_length = comprlen;
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = act_uncomprlen;

	return 0;
}

static int hcsystem_pak_save_resource(struct resource *res, 
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

static void hcsystem_pak_release_resource(struct package *pkg, 
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

static void hcsystem_pak_release(struct package *pkg, 
								 struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		delete [] pkg_dir->directory;
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

static cui_ext_operation hcsystem_pak_operation = {
	hcsystem_pak_match,					/* match */
	hcsystem_pak_extract_directory,		/* extract_directory */
	hcsystem_pak_parse_resource_info,	/* parse_resource_info */
	hcsystem_pak_extract_resource,		/* extract_resource */
	hcsystem_pak_save_resource,			/* save_resource */
	hcsystem_pak_release_resource,		/* release_resource */
	hcsystem_pak_release				/* release */
};

/********************* opf *********************/

static int hcsystem_opf_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "OPF ", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int hcsystem_opf_extract_resource(struct package *pkg,
										 struct package_resource *pkg_res)
{
	opf_header_t opf_header;
	unsigned char *dib, *palette;
	unsigned int palette_size;

	if (pkg->pio->readvec(pkg, &opf_header, sizeof(opf_header),
		0, IO_SEEK_SET))
			return -CUI_EREADVEC;

	palette_size = opf_header.dib_offset - sizeof(opf_header);
	if (palette_size) {
		palette = (unsigned char *)malloc(palette_size);
		if (!palette)
			return -CUI_EMEM;
	} else
		palette = NULL;

	if (palette_size) {
		if (pkg->pio->readvec(pkg, palette, palette_size, 
			sizeof(opf_header), IO_SEEK_SET)) {
				free(palette);
				return -CUI_EREADVEC;
		}
	}

	dib = (unsigned char *)malloc(opf_header.dib_length);
	if (!dib) {
		if (palette)
			free(palette);
		return -CUI_EMEM;
	}

	if (pkg->pio->readvec(pkg, dib, opf_header.dib_length, 
		opf_header.dib_offset, IO_SEEK_SET)) {
			free(dib);
			if (palette)
				free(palette);
			return -CUI_EREADVEC;
	}

	if (MySaveAsBMP(dib, opf_header.dib_length, palette, palette_size,
			opf_header.width, 0 - opf_header.height, opf_header.color_depth, palette_size / 4,
				(unsigned char **)&pkg_res->actual_data, &pkg_res->actual_data_length,
					my_malloc)) {
		free(dib);
		if (palette)
			free(palette);
		return -CUI_EREADVEC;
	}
	free(dib);
	if (palette)
		free(palette);

	return 0;
}

static cui_ext_operation hcsystem_opf_operation = {
	hcsystem_opf_match,					/* match */
	NULL,								/* extract_directory */
	NULL,								/* parse_resource_info */
	hcsystem_opf_extract_resource,		/* extract_resource */
	hcsystem_pak_save_resource,			/* save_resource */
	hcsystem_pak_release_resource,		/* release_resource */
	hcsystem_pak_release				/* release */
};

/********************* opf2 *********************/

static int hcsystem_opf2_match(struct package *pkg)
{
	s8 magic[4];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "OPF2", 4)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int hcsystem_opf2_extract_resource(struct package *pkg,
										  struct package_resource *pkg_res)
{
	opf2_header_t opf2_header;
	if (pkg->pio->readvec(pkg, &opf2_header, sizeof(opf2_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	DWORD palette_size = opf2_header.dib_offset - sizeof(opf2_header);
	BYTE *palette;
	if (palette_size) {
		palette = new BYTE[palette_size];
		if (!palette)
			return -CUI_EMEM;
	} else
		palette = NULL;

	if (palette_size) {
		if (pkg->pio->readvec(pkg, palette, palette_size, sizeof(opf2_header), IO_SEEK_SET)) {
			delete [] palette;
			return -CUI_EREADVEC;
		}
	}

	BYTE *dib = new BYTE[opf2_header.dib_length];
	if (!dib) {
		delete [] palette;
		return -CUI_EMEM;
	}

	if (opf2_header.comprlen) {
		BYTE *compr = new BYTE[opf2_header.comprlen];
		if (!compr) {
			delete [] dib;
			delete [] palette;
			return -CUI_EMEM;
		}

		if (pkg->pio->readvec(pkg, compr, opf2_header.comprlen, opf2_header.dib_offset, IO_SEEK_SET)) {
			delete [] compr;
			delete [] dib;
			delete [] palette;
			return -CUI_EREADVEC;
		}
		lzss_uncompress(dib, opf2_header.dib_length, compr, opf2_header.comprlen);
		delete [] compr;
	} else {
		if (pkg->pio->readvec(pkg, dib, opf2_header.dib_length, opf2_header.dib_offset, IO_SEEK_SET)) {
			delete [] dib;
			delete [] palette;
			return -CUI_EREADVEC;
		}
	}

	if (MySaveAsBMP(dib, opf2_header.dib_length, palette, palette_size,
			opf2_header.width, 0 - opf2_header.height, opf2_header.color_depth, 
				palette_size / 4, (unsigned char **)&pkg_res->actual_data, &pkg_res->actual_data_length,
					my_malloc)) {
		delete [] dib;
		delete [] palette;
		return -CUI_EREADVEC;
	}
	delete [] dib;
	delete [] palette;

	return 0;
}

static cui_ext_operation hcsystem_opf2_operation = {
	hcsystem_opf2_match,				/* match */
	NULL,								/* extract_directory */
	NULL,								/* parse_resource_info */
	hcsystem_opf2_extract_resource,		/* extract_resource */
	hcsystem_pak_save_resource,			/* save_resource */
	hcsystem_pak_release_resource,		/* release_resource */
	hcsystem_pak_release				/* release */
};

int CALLBACK hcsystem_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".pak"), NULL, 
		NULL, &hcsystem_pak_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;
	
	if (callback->add_extension(callback->cui, _T(".opf"), _T(".bmp"), 
		NULL, &hcsystem_opf_operation, CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".op2"), _T(".bmp"), 
		NULL, &hcsystem_opf2_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	return 0;
}
