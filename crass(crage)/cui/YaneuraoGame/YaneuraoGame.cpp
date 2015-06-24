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
#include <cui_common.h>

// M:\[unpack]\'[yanepkEx]乳はSHOCK!!～パイを取り戻せ！～

struct acui_information YaneuraoGame_cui_information = {
	NULL			,				/* copyright */
	_T("YaneuraoGame"),				/* system */
	_T(".dat .lay .scp"),			/* package */
	_T("0.9.1"),					/* revision */
	_T("痴漢公賊"),					/* author */
	_T(""),							/* date */
	NULL,							/* notion */
	ACUI_ATTRIBUTE_LEVEL_DEVELOP
};

#pragma pack (1)
typedef struct {
	s8 magic[8];
	u32 index_entries;
} dat_header_t;

typedef struct {	// ver3: 0x10c bytes
	s8 name[256];
	u32 offset;
	u32 uncomprlen;
	u32 comprlen;
} dat3_entry_t;

typedef struct {	// ver2: 0x2c bytes
	s8 name[32];
	u32 offset;
	u32 comprlen;
	u32 uncomprlen;
} dat2_entry_t;

typedef struct {	// ver1: 0x28 bytes
	s8 name[32];
	u32 offset;
	u32 length;
} dat1_entry_t;

typedef struct {	// ver1: 0x28 bytes
	s8 magic[4];	// "yga"
	u32 width;
	u32 height;
	u32 color_type;	// 1 - 32 bit
	u32 uncomprlen;	// width * height * 4
	u32 comprlen;
} yga_header_t;

typedef struct {
	s8 magic[8];		// "A-LAY"
	u32 length;
} lay_header_t;

typedef struct {
	s8 magic[8];		// "A-SCP"
	u32 length;
} scp_header_t;
#pragma pack ()


static void *my_malloc(DWORD len)
{
	return malloc(len);
}

static void lzss_uncompress(BYTE *uncompr, DWORD uncomprlen, 
							BYTE *compr, DWORD comprlen)
{
	unsigned int act_uncomprlen = 0;
	/* compr中的当前字节中的下一个扫描位的位置 */
	unsigned int curbit = 0;
	/* compr中的当前扫描字节 */
	unsigned int curbyte = 0;
	unsigned int nCurWindowByte = 0xfee;
	unsigned int win_size = 4096;
	BYTE win[4096];
	WORD flag = 0;

	memset(win, 0, nCurWindowByte);
	while (1) {
		flag >>= 1;
		if (!(flag & 0x0100))
			flag = compr[curbyte++] | 0xff00;

		if (flag & 1) {
			unsigned char data;

			data = compr[curbyte++];
			if (act_uncomprlen >= uncomprlen)
				break;
			win[nCurWindowByte++] = data;
			uncompr[act_uncomprlen++] = data;
			/* 输出的1字节放入滑动窗口 */
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

				if (act_uncomprlen >= uncomprlen)
					return;
				data = win[(win_offset + i) & (win_size - 1)];				
				uncompr[act_uncomprlen++] = data;
				/* 输出的1字节放入滑动窗口 */
				win[nCurWindowByte++] = data;
				nCurWindowByte &= win_size - 1;	
			}
		}
	}
}

/********************* dat *********************/

static int YaneuraoGame_dat_match(struct package *pkg)
{
	s8 magic[8];
	unsigned int ver;

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (!memcmp(magic, "yanepack", 8))	// ver 1
		ver = 1;
	else if (!memcmp(magic, "yanepkEx", 8))	// ver 2
		ver = 2;
	else if (!memcmp(magic, "yanepkDx", 8))	// ver 3
		ver = 3;
	else {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;	
	}
	package_set_private(pkg, (void *)ver);

	return 0;	
}

static int YaneuraoGame_dat_extract_directory(struct package *pkg,
											  struct package_directory *pkg_dir)
{
	BYTE *index_buffer;
	unsigned int index_buffer_length;	
	u32 index_entries;
	unsigned int ver, entry_size;

	if (pkg->pio->read(pkg, &index_entries, 4))
		return -CUI_EREAD;

	ver = (unsigned int)package_get_private(pkg);
	switch (ver) {
	case 1:
		entry_size = sizeof(dat1_entry_t);
		break;
	case 2:
		entry_size = sizeof(dat2_entry_t);
		break;
	case 3:
		entry_size = sizeof(dat3_entry_t);
	}

	index_buffer_length = index_entries * entry_size;
	index_buffer = (BYTE *)malloc(index_buffer_length);
	if (!index_buffer)
		return -CUI_EMEM;

	if (pkg->pio->read(pkg, index_buffer, index_buffer_length)) {
		free(index_buffer);
		return -CUI_EREAD;
	}
	pkg_dir->index_entries = index_entries;
	pkg_dir->index_entry_length = entry_size;
	pkg_dir->directory = index_buffer;
	pkg_dir->directory_length = index_buffer_length;
	pkg_dir->flags = PKG_DIR_FLAG_SKIP0;

	return 0;
}

static int YaneuraoGame_dat_parse_resource_info(struct package *pkg,
												struct package_resource *pkg_res)
{
	dat1_entry_t *dat1_entry;
	dat2_entry_t *dat2_entry;
	dat3_entry_t *dat3_entry;
	unsigned int ver;

	ver = (unsigned int)package_get_private(pkg);
	switch (ver) {
	case 1:
		dat1_entry = (dat1_entry_t *)pkg_res->actual_index_entry;
		strncpy(pkg_res->name, dat1_entry->name, sizeof(pkg_res->name));
		pkg_res->name_length = sizeof(dat1_entry->name);
		pkg_res->raw_data_length = dat1_entry->length;
		pkg_res->actual_data_length = 0;	/* 数据都是明文 */
		pkg_res->offset = dat1_entry->offset;
		break;
	case 2:
		dat2_entry = (dat2_entry_t *)pkg_res->actual_index_entry;
		strncpy(pkg_res->name, dat2_entry->name, sizeof(pkg_res->name));
		pkg_res->name_length = sizeof(dat2_entry->name);
		pkg_res->raw_data_length = dat2_entry->comprlen;
		pkg_res->actual_data_length = dat2_entry->comprlen == dat2_entry->uncomprlen ? 0 : dat2_entry->uncomprlen;	/* 数据都是明文 */
		pkg_res->offset = dat2_entry->offset;
		break;
	case 3:
		dat3_entry = (dat3_entry_t *)pkg_res->actual_index_entry;
		strncpy(pkg_res->name, dat3_entry->name, sizeof(pkg_res->name));
		pkg_res->name_length = sizeof(dat3_entry->name);
		pkg_res->raw_data_length = dat3_entry->comprlen;
		pkg_res->actual_data_length = dat3_entry->comprlen == dat3_entry->uncomprlen ? 0 : dat3_entry->uncomprlen;	/* 数据都是明文 */
		pkg_res->offset = dat3_entry->offset;
	}	

//	if (pkg_res->actual_data_length) {
//printf("%s: [%d] %x / %x\n", pkg_res->name, ver, pkg_res->raw_data_length, pkg_res->actual_data_length);
//	}
	return 0;
}

static int YaneuraoGame_dat_extract_resource(struct package *pkg,
											 struct package_resource *pkg_res)
{
	unsigned int uncomprlen, comprlen;
	BYTE *compr, *uncompr;

	comprlen = pkg_res->raw_data_length;
	compr = (BYTE *)malloc(comprlen);
	if (!compr)
		return -CUI_EMEM;

	if (pkg->pio->readvec(pkg, compr, comprlen, pkg_res->offset, IO_SEEK_SET)) {
		free(compr);
		return -CUI_EREADVEC;
	}

	if (!memcmp(compr, "yga", 4)) {
		yga_header_t *yga_header = (yga_header_t *)compr;
		BYTE *to_free;

		if (yga_header->color_type != 1) {
printf("color_type %x\n", yga_header->color_type);
exit(0);
		}

		uncomprlen = yga_header->uncomprlen;
		uncompr = (BYTE *)malloc(uncomprlen);
		if (!uncompr) {
			free(compr);
			return -CUI_EMEM;
		}
		lzss_uncompress(uncompr, uncomprlen, (BYTE *)(yga_header + 1), yga_header->comprlen);
		to_free = uncompr;
		if (MyBuildBMPFile(uncompr, uncomprlen, NULL, 0, yga_header->width, 0 - yga_header->height, 32,
				&uncompr, (DWORD *)&uncomprlen, my_malloc)) {
			free(uncompr);
			free(compr);
			return -CUI_EMEM;			
		}
		free(to_free);
		pkg_res->replace_extension = _T(".yga.bmp");
		pkg_res->flags |= PKG_RES_FLAG_REEXT;
	} else {
		uncompr = NULL;
		uncomprlen = 0;
	}
	pkg_res->actual_data = uncompr;
	pkg_res->actual_data_length = uncomprlen;
	pkg_res->raw_data = compr;
	pkg_res->raw_data_length = comprlen;

	return 0;
}

static cui_ext_operation YaneuraoGame_dat_operation = {
	YaneuraoGame_dat_match,					/* match */
	YaneuraoGame_dat_extract_directory,		/* extract_directory */
	YaneuraoGame_dat_parse_resource_info,	/* parse_resource_info */
	YaneuraoGame_dat_extract_resource,		/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* lay *********************/

static int YaneuraoGame_lay_match(struct package *pkg)
{
	s8 magic[8];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strcmp(magic, "A-LAY")) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}

static int YaneuraoGame_lay_extract_resource(struct package *pkg,
											 struct package_resource *pkg_res)
{
	u32 lay_size;

	if (pkg->pio->length_of(pkg, &lay_size))
		return -CUI_ELEN;

	BYTE *lay = (BYTE *)pkg->pio->readvec_only(pkg, 
		lay_size, 0, IO_SEEK_SET);
	if (!lay)
		return -CUI_EREADVECONLY;

	lay_header_t *lay_header = (lay_header_t *)lay;
	BYTE *dec = (BYTE *)malloc(lay_header->length);
	if (!dec)
		return -CUI_EMEM;

	BYTE *enc = (BYTE *)(lay_header + 1);
	for (DWORD i = 0; i < lay_header->length; i++)
		dec[i] = ((enc[i] << 4) | ((enc[i] >> 4) & 0xf))
			^ (lay_header->length + i);

	pkg_res->actual_data = dec;
	pkg_res->actual_data_length = lay_header->length;

	return 0;
}

static cui_ext_operation YaneuraoGame_lay_operation = {
	YaneuraoGame_lay_match,				/* match */
	NULL,								/* extract_directory */
	NULL,								/* parse_resource_info */
	YaneuraoGame_lay_extract_resource,	/* extract_resource */
	CUI_COMMON_OPERATION
};

/********************* scp *********************/

static int YaneuraoGame_scp_match(struct package *pkg)
{
	s8 magic[8];

	if (pkg->pio->open(pkg, IO_READONLY))
		return -CUI_EOPEN;

	if (pkg->pio->read(pkg, magic, sizeof(magic))) {
		pkg->pio->close(pkg);
		return -CUI_EREAD;
	}

	if (strcmp(magic, "A-SCP")) {
		pkg->pio->close(pkg);
		return -CUI_EMATCH;
	}

	return 0;	
}


static int YaneuraoGame_scp_extract_resource(struct package *pkg,
											 struct package_resource *pkg_res)
{
	u32 scp_size;

	if (pkg->pio->length_of(pkg, &scp_size))
		return -CUI_ELEN;

	BYTE *scp = (BYTE *)pkg->pio->readvec_only(pkg, 
		scp_size, 0, IO_SEEK_SET);
	if (!scp)
		return -CUI_EREADVECONLY;

	scp_header_t *scp_header = (scp_header_t *)scp;
	BYTE *dec = (BYTE *)malloc(scp_header->length);
	if (!dec)
		return -CUI_EMEM;

	BYTE *enc = (BYTE *)(scp_header + 1);
	for (DWORD i = 0; i < scp_header->length; i++)
		dec[i] = ((enc[i] << 4) | ((enc[i] >> 4) & 0xf))
			^ (scp_header->length + i);

	pkg_res->actual_data = dec;
	pkg_res->actual_data_length = scp_header->length;

	return 0;
}

static cui_ext_operation YaneuraoGame_scp_operation = {
	YaneuraoGame_scp_match,				/* match */
	NULL,								/* extract_directory */
	NULL,								/* parse_resource_info */
	YaneuraoGame_scp_extract_resource,	/* extract_resource */
	CUI_COMMON_OPERATION
};

int CALLBACK YaneuraoGame_register_cui(struct cui_register_callback *callback)
{
	if (callback->add_extension(callback->cui, _T(".dat"), NULL, 
		NULL, &YaneuraoGame_dat_operation, CUI_EXT_FLAG_PKG | CUI_EXT_FLAG_DIR))
			return -1;

	if (callback->add_extension(callback->cui, _T(".lay"), _T(".lay.dump"), 
		NULL, &YaneuraoGame_lay_operation, CUI_EXT_FLAG_PKG))
			return -1;

	if (callback->add_extension(callback->cui, _T(".scp"), _T(".scp.dump"), 
		NULL, &YaneuraoGame_scp_operation, CUI_EXT_FLAG_PKG))
			return -1;

	return 0;
}
