#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <crass_types.h>
#include <acui.h>
#include <cui.h>
#include <package.h>
#include <resource.h>
#include <cui_error.h>
#include <utility.h>
#include <stdio.h>
	
struct acui_information BaldrX_cui_information = {
	_T("戯画(GIGA)"),		/* copyright */
	_T(""),					/* system */
	_T(".pac, .grp .spr .dat"),	/* package */
	_T("0.9.1"),			/* revision */
	_T("痴漢公賊"),			/* author */
	_T("2008-3-13 22:24"),	/* date */
	NULL,					/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

#pragma pack (1)
typedef struct {
	s8 magic[4];	// "PAC" or "PACw"
	u32 entries;	// 76 bytes per entry
	u32 cmode;		// 0 - raw; 1 - lzss; 2 - con-huffman
} pac_header_t;

typedef struct {
	s8 name[64];
	u32 offset;		// from file begining
	u32 uncomprLen;
	u32 comprLen;
} pac_entry_t;

typedef struct {
	s8 name[32];
	u32 offset;		// from file begining
	u32 uncomprLen;
	u32 comprLen;
} pac_entry2_t;

/* grp structure:
	s8 magic[3];
	u16 bits_count;
	u32 width;
	u32 height;
	//GR2:
	BYTE *compr_flags;
	u32 comprlen;
	BYTE *compr;
	//!GR2:
	BYTE *dib
 */
typedef struct {
	s8 magic[3];	/* "GR2" */
	u16 bits_count;	/* 8, 15, 16, 24 and default */
	u32 width;
	u32 height;
} grp_header_t;

/* spr structure:
	s8 magic[3];
	u32 uncomprlen;
	u32 flag_bits;
	BYTE *compr_flags;
	u32 comprlen;
	BYTE *compr;
 */
typedef struct {
	s8 magic[3];	/* "SPR" */
	u32 uncomprlen;
	u32 flag_bits;
} spr_header_t;

typedef struct {
	s8 magic[3];		// "PAC"
	u32 data_offset;
} pac_old_header_t;

typedef struct {
	s8 *name;
	u32 offset;	
	u32 length;
} pac_old_entry_t;

typedef struct {
	u32 unknown;
	u32 uncomprlen;
} dat_header_t;
#pragma pack ()	

typedef struct {
	s8 name[MAX_PATH];
	u32 offset;	
	u32 length;
} my_pac_old_entry_t;


static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static int huffman_tree_create(struct bits *bits, u16 children[2][255], 
							   unsigned int *index, u16 *retval)
{
	unsigned int bitval;
	u16 child;

	if (bit_get_high(bits, &bitval))
		return -1;
	
	/* 非叶节点 */
	if (bitval) {
		unsigned int parent;

		parent = *index;
		if (parent >= 0x1ff)
			return -1;
		*index = parent + 1;
					
		if (huffman_tree_create(bits, children, index, &child))
			return -1;
		children[0][parent - 256] = child;
		
		if (huffman_tree_create(bits, children, index, &child))
			return -1;
		children[1][parent - 256] = child;

		child = parent;
	} else {
		unsigned int byteval;
		
		if (bits_get_high(bits, 8, &byteval))
			return -1;
		
		child = byteval;			
	}
	*retval = child;
	
	return 0;
}

static int huffman_decompress(unsigned char *uncompr, unsigned long *uncomprlen,
							  unsigned char *compr, unsigned long comprlen)
{
	struct bits bits;
	u16 children[2][255];
	unsigned int index = 256;
	unsigned long max_uncomprlen;
	unsigned long act_uncomprlen;
	unsigned int bitval;
	u16 retval;	
	
	bits_init(&bits, compr, comprlen);
	if (huffman_tree_create(&bits, children, &index, &retval))
		return -1;

	if (retval != 256)
		return -1;

	index = 0;	/* 从根结点开始遍历 */
	act_uncomprlen = 0;
	max_uncomprlen = *uncomprlen;
	while (!bit_get_high(&bits, &bitval)) {
		if (bitval)
			retval = children[1][index];
		else
			retval = children[0][index];
	
		if (retval >= 256)
			index = retval - 256;
		else {
			if (act_uncomprlen >= max_uncomprlen)
				break;
			uncompr[act_uncomprlen++] = (u8)retval;
			index = 0;	/* 返回到根结点 */
		}
	}
	*uncomprlen = act_uncomprlen;

	return 0;
}

static inline unsigned char getbit_le(unsigned char byte, unsigned int pos)
{
	return !!(byte & (1 << pos));
}

static DWORD lzss_decompress(unsigned char *uncompr, DWORD uncomprlen, 
							unsigned char *compr, DWORD comprlen)
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

static void baldrx_decompress(BYTE *uncompr, DWORD uncomprlen, BYTE *compr, 
							  DWORD comprlen, BYTE *flag_bitmap, DWORD flag_bits)
{
	DWORD act_uncomprlen = 0;

	for (DWORD i = 0; i < flag_bits; i++) {
		BYTE flag = flag_bitmap[i / 8];

		if (!getbit_le(flag, i & 7))
			uncompr[act_uncomprlen++] = *compr++;
		else {
			DWORD copy_bytes, win_offset;

			copy_bytes = *compr++;
			win_offset = *compr++ << 8;

			win_offset = (copy_bytes | win_offset) >> 5;
			copy_bytes = (copy_bytes & 0x1f) + 1;
			for (DWORD k = 0; k < copy_bytes; k++) {
				uncompr[act_uncomprlen] = uncompr[act_uncomprlen - win_offset - 1];
				act_uncomprlen++;
			}
		}
	}
}

/********************* pac *********************/

static int BaldrX_pac_old_match(struct package *pkg)
{
	s8 magic[3];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "PAC", 3)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int BaldrX_pac_old_extract_directory(struct package *pkg,
											struct package_directory *pkg_dir)
{
	pac_old_header_t pac_header;
	DWORD my_index_buffer_length;
	DWORD index_buffer_length;
	DWORD index_entries;
	my_pac_old_entry_t *my_index_buffer;

	if (pkg->pio->readvec(pkg, &pac_header, sizeof(pac_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	index_buffer_length = pac_header.data_offset - sizeof(pac_header);
	index_entries = 0;
	for (DWORD i = 0; i < index_buffer_length; ) {
		u32 offset, length;

		for (int name_len = 0; ; name_len++) {
			s8 name;

			if (pkg->pio->read(pkg, &name, 1))
				return -CUI_EREAD;
			
			if (!name)
				break;
		}
		name_len++;

		if (pkg->pio->read(pkg, &offset, 4))
			return -CUI_EREAD;

		if (pkg->pio->read(pkg, &length, 4))
			return -CUI_EREAD;
		i += name_len + 8;
		index_entries++;
	}

	my_index_buffer_length = index_entries * sizeof(my_pac_old_entry_t);
	my_index_buffer = (my_pac_old_entry_t *)malloc(my_index_buffer_length);
	if (!my_index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->seek(pkg, sizeof(pac_old_header_t), IO_SEEK_SET)) {
		free(my_index_buffer);
		return -CUI_ESEEK;
	}

	for (i = 0; i < index_entries; i++) {
		for (int name_len = 0; ; name_len++) {
			if (pkg->pio->read(pkg, &my_index_buffer[i].name[name_len], 1)) {
				free(my_index_buffer);
				return -CUI_EREAD;
			}
			
			if (!my_index_buffer[i].name[name_len])
				break;
		}

		for (int k = 0; k < name_len; k++)
			my_index_buffer[i].name[k] = ~my_index_buffer[i].name[k];

		if (pkg->pio->read(pkg, &my_index_buffer[i].offset, 4))
			break;

		if (pkg->pio->read(pkg, &my_index_buffer[i].length, 4))
			break;

		my_index_buffer[i].offset += pac_header.data_offset;
	}
	if (i != index_entries) {
		free(my_index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->directory = my_index_buffer;
	pkg_dir->directory_length = my_index_buffer_length;
	pkg_dir->index_entries = index_entries;
	pkg_dir->index_entry_length= sizeof(my_pac_old_entry_t);

	return 0;
}

static int BaldrX_pac_old_parse_resource_info(struct package *pkg,
											  struct package_resource *pkg_res)
{
	my_pac_old_entry_t *my_pac_old_entry;

	my_pac_old_entry = (my_pac_old_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, my_pac_old_entry->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = my_pac_old_entry->length;
	pkg_res->actual_data_length = 0;
	pkg_res->offset = my_pac_old_entry->offset;

	return 0;
}

static int BaldrX_pac_old_extract_resource(struct package *pkg,
										   struct package_resource *pkg_res)
{
	DWORD comprlen;
	BYTE *compr;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	for (DWORD i = 0; i < 3; i++)
		compr[i] = ~compr[i];

	pkg_res->raw_data = compr;
	pkg_res->raw_data_length = comprlen;

	return 0;
}

static BaldrX_pac_old_save_resource(struct resource *res, 
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

static void BaldrX_pac_old_release_resource(struct package *pkg,
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

static void BaldrX_pac_old_release(struct package *pkg, 
								   struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

static cui_ext_operation BaldrX_pac_old_operation = {
	BaldrX_pac_old_match,				/* match */
	BaldrX_pac_old_extract_directory,	/* extract_directory */
	BaldrX_pac_old_parse_resource_info,	/* parse_resource_info */
	BaldrX_pac_old_extract_resource,	/* extract_resource */
	BaldrX_pac_old_save_resource,		/* save_resource */
	BaldrX_pac_old_release_resource,	/* release_resource */
	BaldrX_pac_old_release				/* release */
};

/********************* pac *********************/

static int BaldrX_pac_match(struct package *pkg)
{
	pac_header_t *pac_header;
	pac_entry_t pac_entry;	/* 用于和NeXAS互斥 */

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	pac_header = (pac_header_t *)malloc(sizeof(*pac_header));
	if (!pac_header) {
		pkg->pio->close(pkg);
		return -CUI_EMEM;		
	}

	if (pkg->pio->read(pkg, pac_header, sizeof(*pac_header))) {
		free(pac_header);
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(pac_header->magic, "PAC", 3)) {
		free(pac_header);
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (!pac_header->entries) {
		free(pac_header);
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pac_header->cmode != 0 && pac_header->cmode != 1 && pac_header->cmode != 2) {
		free(pac_header);
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->pio->read(pkg, &pac_entry, sizeof(pac_entry))) {
		free(pac_header);
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (pac_entry.offset != sizeof(pac_header_t) + sizeof(pac_entry_t) * pac_header->entries) {
		free(pac_header);
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}
	package_set_private(pkg, pac_header);

	return 0;	
}

static int BaldrX_pac_extract_directory(struct package *pkg,
										struct package_directory *pkg_dir)
{
	pac_header_t *pac_header;
	DWORD index_buffer_length;
	pac_entry_t *index_buffer;

	if (pkg->pio->seek(pkg, sizeof(pac_header_t), IO_SEEK_SET))
		return -CUI_ESEEK;

	pac_header = (pac_header_t *)package_get_private(pkg); 
	if (!pac_header)
		return -CUI_EPARA;

	index_buffer_length = pac_header->entries * sizeof(pac_entry_t);
	index_buffer = (pac_entry_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entries = pac_header->entries;
	pkg_dir->index_entry_length = sizeof(pac_entry_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

static int BaldrX_pac_parse_resource_info(struct package *pkg,
										  struct package_resource *pkg_res)
{
	pac_header_t *pac_header;
	pac_entry_t *pac_entry;

	pac_header = (pac_header_t *)package_get_private(pkg); 
	if (!pac_header)
		return -CUI_EPARA;

	pac_entry = (pac_entry_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, pac_entry->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = pac_entry->comprLen;
	pkg_res->actual_data_length = pac_entry->uncomprLen;
	pkg_res->offset = pac_entry->offset;

	return 0;
}

static int BaldrX_pac_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	pac_header_t *pac_header;
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen, act_uncomprLen;

	pac_header = (pac_header_t *)package_get_private(pkg); 
	if (!pac_header)
		return -CUI_EPARA;

	comprlen = (DWORD)pkg_res->raw_data_length;
	compr = (BYTE *)pkg->pio->readvec_only(pkg, comprlen, pkg_res->offset, IO_SEEK_SET);
	if (!compr)
		return -CUI_EREADVECONLY;

	uncomprlen = pkg_res->actual_data_length;
	if (pac_header->cmode == 2) {
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr)
			return -CUI_EMEM;
		act_uncomprLen = uncomprlen;
		if (huffman_decompress(uncompr, &act_uncomprLen, compr, comprlen)) {
			free(uncompr);
			return -CUI_EUNCOMPR;
		}
		if (act_uncomprLen != uncomprlen) {
			free(uncompr);
			return -CUI_EUNCOMPR;
		}
	} else if (pac_header->cmode == 1) {
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr)
			return -CUI_EMEM;
		act_uncomprLen = lzss_decompress(uncompr, uncomprlen, compr, comprlen);
		if (act_uncomprLen != uncomprlen) {
			free(uncompr);
			return -CUI_EUNCOMPR;
		}
	} else if (!pac_header->cmode) {
		uncompr = NULL;
		uncomprlen = 0;
	}
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;
	pkg_res->raw_data = compr;
	pkg_res->raw_data_length = comprlen;

	return 0;
}

static int BaldrX_pac_save_resource(struct resource *res, 
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

static void BaldrX_pac_release_resource(struct package *pkg, 
									 struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}

	if (pkg_res->raw_data)
		pkg_res->raw_data = NULL;
}

static void BaldrX_pac_release(struct package *pkg, 
							struct package_directory *pkg_dir)
{
	if (pkg_dir->directory) {
		free(pkg_dir->directory);
		pkg_dir->directory = NULL;
	}

	pkg->pio->close(pkg);
}

static cui_ext_operation BaldrX_pac_operation = {
	BaldrX_pac_match,				/* match */
	BaldrX_pac_extract_directory,	/* extract_directory */
	BaldrX_pac_parse_resource_info,	/* parse_resource_info */
	BaldrX_pac_extract_resource,	/* extract_resource */
	BaldrX_pac_save_resource,		/* save_resource */
	BaldrX_pac_release_resource,	/* release_resource */
	BaldrX_pac_release				/* release */
};

/********************** pac ************************/

static int BaldrX_pac_match2(struct package *pkg)
{
	pac_header_t *pac_header;
	pac_entry2_t pac_entry;	/* 用于和NeXAS互斥 */

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	pac_header = (pac_header_t *)malloc(sizeof(*pac_header));
	if (!pac_header) {
		pkg->pio->close(pkg);
		return -CUI_EMEM;		
	}

	if (pkg->pio->read(pkg, pac_header, sizeof(*pac_header))) {
		free(pac_header);
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (memcmp(pac_header->magic, "PAC", 3)) {
		free(pac_header);
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	if (!pac_header->entries) {
		free(pac_header);
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pac_header->cmode != 0 && pac_header->cmode != 1 && pac_header->cmode != 2) {
		free(pac_header);
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	if (pkg->pio->read(pkg, &pac_entry, sizeof(pac_entry))) {
		free(pac_header);
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (pac_entry.offset != sizeof(pac_header_t) + sizeof(pac_entry2_t) * pac_header->entries) {
		free(pac_header);
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}
	package_set_private(pkg, pac_header);

	return 0;	
}

static int BaldrX_pac_extract_directory2(struct package *pkg,
										 struct package_directory *pkg_dir)
{
	pac_header_t *pac_header;
	DWORD index_buffer_length;
	pac_entry2_t *index_buffer;

	if (pkg->pio->seek(pkg, sizeof(pac_header_t), IO_SEEK_SET))
		return -CUI_ESEEK;

	pac_header = (pac_header_t *)package_get_private(pkg); 
	if (!pac_header)
		return -CUI_EPARA;

	index_buffer_length = pac_header->entries * sizeof(pac_entry2_t);
	index_buffer = (pac_entry2_t *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}

	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->index_entries = pac_header->entries;
	pkg_dir->index_entry_length = sizeof(pac_entry2_t);
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

static int BaldrX_pac_parse_resource_info2(struct package *pkg,
										   struct package_resource *pkg_res)
{
	pac_header_t *pac_header;
	pac_entry2_t *pac_entry;

	pac_header = (pac_header_t *)package_get_private(pkg); 
	if (!pac_header)
		return -CUI_EPARA;

	pac_entry = (pac_entry2_t *)pkg_res->actual_index_entry;
	strcpy(pkg_res->name, pac_entry->name);
	pkg_res->name_length = -1;
	pkg_res->raw_data_length = pac_entry->comprLen;
	pkg_res->actual_data_length = pac_entry->uncomprLen;
	pkg_res->offset = pac_entry->offset;

	return 0;
}

static cui_ext_operation BaldrX_pac_operation2 = {
	BaldrX_pac_match2,					/* match */
	BaldrX_pac_extract_directory2,		/* extract_directory */
	BaldrX_pac_parse_resource_info2,	/* parse_resource_info */
	BaldrX_pac_extract_resource,		/* extract_resource */
	BaldrX_pac_save_resource,			/* save_resource */
	BaldrX_pac_release_resource,		/* release_resource */
	BaldrX_pac_release					/* release */
};

/********************** grp ************************/

static int BaldrX_grp_match(struct package *pkg)
{
	s8 magic[2];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {		
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "GR", 2)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int BaldrX_grp_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	grp_header_t grp_header;
	BYTE *dib, *uncompr, *palette;
	DWORD uncomprlen, palette_size, dib_size;

	if (pkg->pio->readvec(pkg, &grp_header, sizeof(grp_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	switch (grp_header.bits_count) {
	case 8:
		uncomprlen = grp_header.width * grp_header.height + 0x300;
		break;
	case 15:
	case 16:
		uncomprlen = grp_header.width * grp_header.height * 2;
		break;
	case 24:
		uncomprlen = grp_header.width * grp_header.height * 3;
		break;
	default:
		uncomprlen = 4;
	}

	uncompr = (BYTE *)malloc(uncomprlen);
	if (!uncompr)
		return -CUI_EMEM;

	if (grp_header.bits_count == 8) {
		dib = uncompr;
		dib_size = grp_header.width * grp_header.height;
		palette = dib + dib_size;
		palette_size = 0x300;
	} else {
		dib = uncompr;	
		dib_size = uncomprlen;
		palette_size = 0;
		palette = NULL;
	}	

	if (grp_header.magic[2] == '2') {
		u32 dib_len, flag_bits, comprlen;
		DWORD flag_bitmap_len;
		BYTE *flag_bitmap, *compr;

		if (pkg->pio->readvec(pkg, &dib_len, 4, sizeof(grp_header), IO_SEEK_SET)) {
			free(uncompr);
			return -CUI_EREADVEC;
		}

		if (pkg->pio->readvec(pkg, &flag_bits, 4, sizeof(grp_header) + 4, IO_SEEK_SET)) {
			free(uncompr);
			return -CUI_EREADVEC;
		}

		flag_bitmap_len = (flag_bits + 7) / 8;
		flag_bitmap = (BYTE *)malloc(flag_bitmap_len);
		if (!flag_bitmap) {
			free(uncompr);
			return -CUI_EMEM;
		}

		if (pkg->pio->readvec(pkg, flag_bitmap, flag_bitmap_len, sizeof(grp_header) + 8, IO_SEEK_SET)) {
			free(flag_bitmap);
			return -CUI_EREADVEC;
		}

		if (pkg->pio->readvec(pkg, &comprlen, 4, flag_bitmap_len + sizeof(grp_header) + 8, IO_SEEK_SET)) {
			free(flag_bitmap);
			return -CUI_EREADVEC;
		}

		compr = (BYTE *)malloc(comprlen);
		if (!compr) {
			free(flag_bitmap);
			return -CUI_EREADVEC;
		}

		if (pkg->pio->readvec(pkg, compr, comprlen, 
				flag_bitmap_len + sizeof(grp_header) + 12, IO_SEEK_SET)) {
			free(compr);
			free(flag_bitmap);
			return -CUI_EREADVEC;
		}

		baldrx_decompress(uncompr, uncomprlen, compr, comprlen, flag_bitmap, flag_bits);
		free(compr);
		free(flag_bitmap);
	} else {
		if (pkg->pio->readvec(pkg, uncompr, uncomprlen, sizeof(grp_header), IO_SEEK_SET)) {
			free(uncompr);
			return -CUI_EREADVEC;
		}
	}

	if (grp_header.bits_count == 16) {
		DWORD flag;

		// BALDR BULLET バルドバレット的16位图都是RGB555的
		if (get_options("force_rgb555"))
			flag = RGB555;
		else
			flag = RGB565;

		if (MyBuildBMP16File(dib, dib_size, grp_header.width, 
				0 - grp_header.height, (BYTE **)&pkg_res->actual_data,
					(DWORD *)&pkg_res->actual_data_length, flag, NULL, my_malloc)) {
			free(uncompr);
			return -CUI_EMEM;
		}
	} else if (grp_header.bits_count == 15) {
		if (MyBuildBMP16File(dib, dib_size, grp_header.width, 
				0 - grp_header.height, (BYTE **)&pkg_res->actual_data,
					(DWORD *)&pkg_res->actual_data_length, RGB555, NULL, my_malloc)) {
			free(uncompr);
			return -CUI_EMEM;
		}
	} else {
		if (grp_header.bits_count == 8) {
			BYTE *p = palette;
			for (int i = 0; i < 256; i++) {
				BYTE b;
				b = p[2];
				p[2] = p[0];
				p[0] = b;
				p += 3;
			}
		}

		if (MyBuildBMPFile(dib, dib_size, palette, palette_size, grp_header.width, 
				0 - grp_header.height, grp_header.bits_count, (BYTE **)&pkg_res->actual_data,
					(DWORD *)&pkg_res->actual_data_length, my_malloc)) {
			free(uncompr);
			return -CUI_EMEM;
		}
	}
	free(uncompr);
	
	return 0;
}

static int BaldrX_grp_save_resource(struct resource *res, 
									struct package_resource *pkg_res)
{
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (res->rio->write(res, pkg_res->actual_data, pkg_res->actual_data_length)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}
	res->rio->close(res);
	
	return 0;
}

static void BaldrX_grp_release_resource(struct package *pkg, 
										struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
}

static void BaldrX_grp_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

static cui_ext_operation BaldrX_grp_operation = {
	BaldrX_grp_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	BaldrX_grp_extract_resource,	/* extract_resource */
	BaldrX_grp_save_resource,		/* save_resource */
	BaldrX_grp_release_resource,	/* release_resource */
	BaldrX_grp_release				/* release */
};

/********************** spr ************************/

static int BaldrX_spr_match(struct package *pkg)
{
	s8 magic[3];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {		
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strncmp(magic, "SPR", 3)) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}

	return 0;	
}

static int BaldrX_spr_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	spr_header_t spr_header;
	DWORD flag_bitmap_len;
	BYTE *flag_bitmap, *compr, *uncompr;
	DWORD comprlen, uncomprlen;

	if (pkg->pio->readvec(pkg, &spr_header, sizeof(spr_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	flag_bitmap_len = (spr_header.flag_bits + 7) / 8;
	flag_bitmap = (BYTE *)malloc(flag_bitmap_len);
	if (!flag_bitmap)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, flag_bitmap, flag_bitmap_len, sizeof(spr_header), IO_SEEK_SET)) {
		free(flag_bitmap);
		return -CUI_EREADVEC;
	}

	if (pkg->pio->readvec(pkg, &comprlen, 4, flag_bitmap_len + sizeof(spr_header), IO_SEEK_SET)) {
		free(flag_bitmap);
		return -CUI_EREADVEC;
	}

	compr = (BYTE *)pkg->pio->readvec_only(pkg, comprlen, 
		flag_bitmap_len + sizeof(spr_header) + 4, IO_SEEK_SET);
	if (!compr) {
		free(flag_bitmap);
		return -CUI_EREADVECONLY;
	}

	uncomprlen = spr_header.uncomprlen;
	uncompr = (BYTE *)malloc(uncomprlen);
	if (!uncompr) {
		free(flag_bitmap);
		return -CUI_EMEM;
	}
	baldrx_decompress(uncompr, uncomprlen, compr, comprlen, flag_bitmap, spr_header.flag_bits);
	free(flag_bitmap);

	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;	
	return 0;
}

static int BaldrX_spr_save_resource(struct resource *res, 
									struct package_resource *pkg_res)
{
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (res->rio->write(res, pkg_res->actual_data, pkg_res->actual_data_length)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}
	res->rio->close(res);
	
	return 0;
}

static void BaldrX_spr_release_resource(struct package *pkg, 
										struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
}

static void BaldrX_spr_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

static cui_ext_operation BaldrX_spr_operation = {
	BaldrX_spr_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	BaldrX_spr_extract_resource,	/* extract_resource */
	BaldrX_spr_save_resource,		/* save_resource */
	BaldrX_spr_release_resource,	/* release_resource */
	BaldrX_spr_release				/* release */
};

/********************** dat ************************/

static int BaldrX_dat_match(struct package *pkg)
{
	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	return 0;	
}

static int BaldrX_dat_extract_resource(struct package *pkg,
									   struct package_resource *pkg_res)
{
	dat_header_t dat_header;
	BYTE *compr, *uncompr;
	DWORD comprlen, uncomprlen;
	u32 dat_size;

	if (pkg->pio->length_of(pkg, &dat_size))
		return -CUI_ELEN;

	if (pkg->pio->readvec(pkg, &dat_header, sizeof(dat_header), 0, IO_SEEK_SET))
		return -CUI_EREADVEC;

	comprlen = dat_size - sizeof(dat_header);
	compr = (BYTE *)pkg->pio->readvec_only(pkg, comprlen, sizeof(dat_header), IO_SEEK_SET);
	if (!compr)
		return -CUI_EREADVECONLY;

	uncompr = (BYTE *)malloc(dat_header.uncomprlen);
	if (!uncompr)
		return -CUI_EMEM;

	uncomprlen = dat_header.uncomprlen;
	if (huffman_decompress(uncompr, &uncomprlen, compr, comprlen)) {
		free(uncompr);
		return -CUI_EUNCOMPR;
	}

	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = dat_header.uncomprlen;	
	return 0;
}

static int BaldrX_dat_save_resource(struct resource *res, 
									struct package_resource *pkg_res)
{
	if (res->rio->create(res))
		return -CUI_ECREATE;

	if (res->rio->write(res, pkg_res->actual_data, pkg_res->actual_data_length)) {
		res->rio->close(res);
		return -CUI_EWRITE;
	}
	res->rio->close(res);
	
	return 0;
}

static void BaldrX_dat_release_resource(struct package *pkg, 
										struct package_resource *pkg_res)
{
	if (pkg_res->actual_data) {
		free(pkg_res->actual_data);
		pkg_res->actual_data = NULL;
	}
}

static void BaldrX_dat_release(struct package *pkg, 
							   struct package_directory *pkg_dir)
{
	pkg->pio->close(pkg);
}

static cui_ext_operation BaldrX_dat_operation = {
	BaldrX_dat_match,				/* match */
	NULL,							/* extract_directory */
	NULL,							/* parse_resource_info */
	BaldrX_dat_extract_resource,	/* extract_resource */
	BaldrX_dat_save_resource,		/* save_resource */
	BaldrX_dat_release_resource,	/* release_resource */
	BaldrX_dat_release				/* release */
};

int CALLBACK BaldrX_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".pac"), NULL, 
		NULL, &BaldrX_pac_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_SUFFIX_SENSE))
			return -1;

	if (callback->add_extension(callback->cui, _T(".pac"), NULL, 
		NULL, &BaldrX_pac_operation2, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_SUFFIX_SENSE))
			return -1;

//	if (callback->add_extension(callback->cui, _T(".pac"), NULL, 
//		NULL, &BaldrX_pac_old_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR | CUI_EXT_FLAG_IMPACT |CUI_EXT_FLAG_SUFFIX_SENSE))
//			return -1;

	if (callback->add_extension(callback->cui, _T(".grp"), _T(".bmp"), 
		NULL, &BaldrX_grp_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_RES))
			return -1;

	if (callback->add_extension(callback->cui, _T(".spr"), NULL, 
		NULL, &BaldrX_spr_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &BaldrX_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_NO_MAGIC))
			return -1;

	return 0;
}

